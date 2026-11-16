// -*-c++-*-

/*!
  \file test_humanoid.cpp
  \brief unit test of HumanoidRig (the bones by their roles, retargeting)
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#include <QtTest>

#include "humanoid.h"
#include "test_gltf.h"

#include <cmath>

namespace {

QByteArray
mannequin()
{
    QFile file( QStringLiteral( TEST_SOURCE_DIR "/resources/figures/mannequin/Mannequin.glb" ) );
    if ( ! file.open( QIODevice::ReadOnly ) ) return QByteArray();
    return file.readAll();
}

//! the same rotation (a quaternion and its negation are one rotation)
bool
same( const QQuaternion & a,
      const QQuaternion & b,
      const float tolerance = 1.0e-4f )
{
    return std::fabs( std::fabs( QQuaternion::dotProduct( a.normalized(), b.normalized() ) ) - 1.0f ) < tolerance;
}

//! the rotations of the nodes in the model, through their parents
std::vector< QQuaternion >
modelRotations( const GltfModel & m,
                const std::vector< NodeTransform > & local )
{
    std::vector< QQuaternion > r( m.rest.size() );
    for ( const int i : m.order )
    {
        r[i] = ( m.parent[i] >= 0 ? r[m.parent[i]] : QQuaternion() ) * local[i].r;
    }
    return r;
}

QVector3D
modelPosition( const GltfModel & m,
               const std::vector< NodeTransform > & local,
               const int node )
{
    std::vector< QMatrix4x4 > g;
    m.globals( local, g );
    return g[node].column( 3 ).toVector3D();
}

//! a humanoid of boxes (test_gltf.h), read
GltfModel
boxes( const TestGltf::HumanoidOptions & o )
{
    GltfModel m;
    m.load( TestGltf::humanoid( TestGltf::mannequin(), o ).glb() );
    return m;
}

//! a rig of a model by a preset (empty: found)
HumanoidRig
rigOf( const GltfModel & m,
       const QString & preset = QString() )
{
    HumanBoneNodes nodes;
    HumanoidRig rig;
    QString error;
    if ( findHumanBones( m, preset, {}, &nodes, &error ) ) rig.build( m, nodes, &error );
    return rig;
}

/*!
  \brief where a pose puts each role's joint of a rig, from the hips (the
  hips' own place differs by the hip height of a model)
*/
std::vector< QVector3D >
rolePlaces( const HumanoidRig & rig,
            const HumanoidPose & pose )
{
    std::vector< NodeTransform > local = rig.model()->rest;
    rig.apply( pose, local );
    std::vector< QMatrix4x4 > g;
    rig.model()->globals( local, g );
    std::vector< QVector3D > at;
    const QVector3D hips = g[rig.node( HumanBone::Hips )].column( 3 ).toVector3D();
    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
    {
        at.push_back( rig.node( HumanBone( b ) ) < 0 ? QVector3D() : g[rig.node( HumanBone( b ) )].column( 3 ).toVector3D() - hips );
    }
    return at;
}

}

/*!
  \brief unit test of HumanoidRig
*/
class TestHumanoid
    : public QObject {
    Q_OBJECT

private:
    GltfModel M_model;
    HumanoidRig M_rig;

private slots:

    void initTestCase()
      {
          QVERIFY( M_model.load( mannequin() ) );
          QString error;
          QVERIFY2( M_rig.build( M_model, quaterniusBoneNames(), &error ), qPrintable( error ) );
      }

    //! the roles and the bones of the Quaternius rig
    void roles()
      {
          QCOMPARE( QString( humanBoneName( HumanBone::Hips ) ), QString( "hips" ) );
          QCOMPARE( QString( humanBoneName( HumanBone::LeftUpperArm ) ), QString( "leftUpperArm" ) );
          QCOMPARE( QString( humanBoneName( HumanBone::RightToes ) ), QString( "rightToes" ) );
          QVERIFY( humanBoneRequired( HumanBone::Head ) );
          QVERIFY( ! humanBoneRequired( HumanBone::Neck ) );

          // every role has its bone in the Mannequin
          for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
          {
              QVERIFY2( M_rig.node( HumanBone( b ) ) >= 0, humanBoneName( HumanBone( b ) ) );
          }
          QCOMPARE( M_model.node_names[M_rig.node( HumanBone::LeftLowerLeg )], QString( "calf_l" ) );
          QVERIFY( M_rig.hipHeight() > 0.8f && M_rig.hipHeight() < 1.2f );

          // a required role without its bone: not built, with the role named
          HumanBoneNames names = quaterniusBoneNames();
          names[int( HumanBone::Head )] = "no head";
          names[int( HumanBone::Neck )].clear(); // optional: fine
          HumanoidRig rig;
          QString error;
          QVERIFY( ! rig.build( M_model, names, &error ) );
          QVERIFY2( error.contains( "head" ), qPrintable( error ) );
          QVERIFY( ! error.contains( "neck" ) );
      }

    //! at rest, no role is turned and the hips have not moved
    void rest()
      {
          const HumanoidPose pose = M_rig.poseOf( M_model.rest );
          for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
          {
              QVERIFY2( same( pose.rotations[b], QQuaternion() ), humanBoneName( HumanBone( b ) ) );
          }
          QVERIFY( pose.hips.length() < 1.0e-5f );
      }

    /*!
      a pose of the skeleton, read and put back on the rest of the same
      skeleton: every role's bone turned in the model as before, and the
      hips where they were
    */
    void roundTrip()
      {
          for ( const GltfModel::Clip & clip : M_model.clips )
          {
              for ( const float t : { 0.0f, 0.37f, 0.81f } )
              {
                  std::vector< NodeTransform > motion = M_model.rest;
                  M_model.sample( clip, t * clip.duration, motion );
                  const HumanoidPose pose = M_rig.poseOf( motion );

                  std::vector< NodeTransform > local = M_model.rest;
                  M_rig.apply( pose, local );

                  const std::vector< QQuaternion > a = modelRotations( M_model, motion );
                  const std::vector< QQuaternion > b = modelRotations( M_model, local );
                  for ( int r = 0; r < HUMAN_BONE_COUNT; ++r )
                  {
                      const int n = M_rig.node( HumanBone( r ) );
                      QVERIFY2( same( a[n], b[n] ), qPrintable( clip.name + " " + humanBoneName( HumanBone( r ) ) ) );
                  }
                  const int hips = M_rig.node( HumanBone::Hips );
                  QVERIFY( ( modelPosition( M_model, motion, hips ) - modelPosition( M_model, local, hips ) ).length() < 1.0e-4f );
              }
          }
          // a walk turns the legs: not the rest
          std::vector< NodeTransform > motion = M_model.rest;
          M_model.sample( M_model.clips[M_model.clipIndex( "Walk_Loop" )], 0.2f, motion );
          QVERIFY( ! same( M_rig.poseOf( motion )[HumanBone::LeftUpperLeg], QQuaternion(), 1.0e-3f ) );
      }

    /*!
      retargeting: on a skeleton whose bones are differently turned at rest
      and twice as large, every role's bone turns in the model as much as
      on the first one, and the hips move twice as far
    */
    void retarget()
      {
          GltfModel other = M_model;
          // the bones of some roles turned differently at rest: the bones
          // below them keep their places in the model (their rest turned back)
          for ( const char * name : { "upperarm_l", "spine_02", "thigh_r" } )
          {
              const int n = other.nodeIndex( name );
              const QQuaternion extra = QQuaternion::fromAxisAndAngle( QVector3D( 0.3f, 1.0f, -0.4f ).normalized(), 40.0f );
              other.rest[n].r = other.rest[n].r * extra;
              for ( std::size_t c = 0; c < other.parent.size(); ++c )
              {
                  if ( other.parent[c] == n )
                  {
                      other.rest[c].r = extra.conjugated() * other.rest[c].r;
                      other.rest[c].t = extra.conjugated().rotatedVector( other.rest[c].t );
                  }
              }
          }
          // twice as large
          for ( NodeTransform & t : other.rest ) t.t *= 2.0f;
          other.min_y *= 2.0f;
          other.max_y *= 2.0f;
          HumanoidRig rig;
          QVERIFY( rig.build( other, quaterniusBoneNames() ) );
          QVERIFY( std::fabs( rig.hipHeight() - 2.0f * M_rig.hipHeight() ) < 1.0e-3f );

          // the bones turned at rest are where they were in the model
          {
              const int hand = other.nodeIndex( "hand_l" );
              QVERIFY( ( modelPosition( other, other.rest, hand ) - 2.0f * modelPosition( M_model, M_model.rest, hand ) ).length() < 1.0e-3f );
          }

          std::vector< NodeTransform > motion = M_model.rest;
          M_model.sample( M_model.clips[M_model.clipIndex( "Jog_Fwd_Loop" )], 0.3f, motion );
          const HumanoidPose pose = M_rig.poseOf( motion );
          std::vector< NodeTransform > local = other.rest;
          rig.apply( pose, local );

          // D = (rotation in the pose) * (rotation at rest)^-1, the same on both
          const std::vector< QQuaternion > a = modelRotations( M_model, motion );
          const std::vector< QQuaternion > a0 = modelRotations( M_model, M_model.rest );
          const std::vector< QQuaternion > b = modelRotations( other, local );
          const std::vector< QQuaternion > b0 = modelRotations( other, other.rest );
          for ( int r = 0; r < HUMAN_BONE_COUNT; ++r )
          {
              const int n = M_rig.node( HumanBone( r ) );
              QVERIFY2( same( a[n] * a0[n].conjugated(), b[n] * b0[n].conjugated() ),
                        humanBoneName( HumanBone( r ) ) );
          }
          // so the hand is where it was, twice as far, relative to the hips
          const int hand = other.nodeIndex( "hand_l" );
          const int hips = other.nodeIndex( "pelvis" );
          const QVector3D da = modelPosition( M_model, motion, hand ) - modelPosition( M_model, motion, hips );
          const QVector3D db = modelPosition( other, local, hand ) - modelPosition( other, local, hips );
          QVERIFY( ( db - 2.0f * da ).length() < 1.0e-3f );
          // the hips moved from rest twice as far
          const QVector3D ma = modelPosition( M_model, motion, hips ) - modelPosition( M_model, M_model.rest, hips );
          const QVector3D mb = modelPosition( other, local, hips ) - modelPosition( other, other.rest, hips );
          QVERIFY( ma.length() > 1.0e-3f );
          QVERIFY( ( mb - 2.0f * ma ).length() < 1.0e-3f );
      }

    //! between two poses: the ends, halfway the shorter way, the hips in a line
    void mix()
      {
          HumanoidPose a, b;
          a[HumanBone::Head] = QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, 10.0f );
          b[HumanBone::Head] = -QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, 50.0f ); // written negated
          a.hips = QVector3D( 0.0f, 0.1f, 0.0f );
          b.hips = QVector3D( 0.2f, -0.1f, 0.0f );
          QVERIFY( same( HumanoidPose::mix( a, b, 0.0f )[HumanBone::Head], a[HumanBone::Head] ) );
          QVERIFY( same( HumanoidPose::mix( a, b, 1.0f )[HumanBone::Head], b[HumanBone::Head] ) );
          const HumanoidPose half = HumanoidPose::mix( a, b, 0.5f );
          QVERIFY2( same( half[HumanBone::Head], QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, 30.0f ), 1.0e-3f ),
                    qPrintable( QString::number( half[HumanBone::Head].scalar() ) ) );
          QVERIFY( same( half[HumanBone::Hips], QQuaternion() ) );
          QVERIFY( ( half.hips - QVector3D( 0.1f, 0.0f, 0.0f ) ).length() < 1.0e-6f );
      }

    /*!
      a role's bone turned in the model: it and every role below it turn as
      much in the model, the roles above do not
    */
    void turnInModel()
      {
          std::vector< NodeTransform > motion = M_model.rest;
          M_model.sample( M_model.clips[M_model.clipIndex( "Jog_Fwd_Loop" )], 0.4f, motion );
          const HumanoidPose before = M_rig.poseOf( motion );
          std::vector< NodeTransform > a = M_model.rest;
          M_rig.apply( before, a );
          const std::vector< QQuaternion > ra = modelRotations( M_model, a );

          // the hips: all the roles turn
          const QQuaternion turn = QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, 30.0f );
          HumanoidPose pose = before;
          M_rig.turnInModel( pose, HumanBone::Hips, turn );
          std::vector< NodeTransform > b = M_model.rest;
          M_rig.apply( pose, b );
          const std::vector< QQuaternion > rb = modelRotations( M_model, b );
          for ( int r = 0; r < HUMAN_BONE_COUNT; ++r )
          {
              const int n = M_rig.node( HumanBone( r ) );
              QVERIFY2( same( rb[n] * ra[n].conjugated(), turn ), humanBoneName( HumanBone( r ) ) );
          }

          // then the chest back: the chest and above as before, the legs and the spine turned
          M_rig.turnInModel( pose, HumanBone::Chest, turn.conjugated() );
          std::vector< NodeTransform > c = M_model.rest;
          M_rig.apply( pose, c );
          const std::vector< QQuaternion > rc = modelRotations( M_model, c );
          for ( const HumanBone r : { HumanBone::Chest, HumanBone::Head, HumanBone::LeftHand, HumanBone::RightUpperArm } )
          {
              QVERIFY2( same( rc[M_rig.node( r )], ra[M_rig.node( r )] ), humanBoneName( r ) );
          }
          for ( const HumanBone r : { HumanBone::Hips, HumanBone::Spine, HumanBone::LeftFoot } )
          {
              QVERIFY2( same( rc[M_rig.node( r )] * ra[M_rig.node( r )].conjugated(), turn ), humanBoneName( r ) );
          }
      }

    /*!
      the bones of the roles found: by the presets' names (also after a
      prefix ending with ':' or '_'), by VRM's roles, by a pack's map, or
      by the first preset that fits; why not
    */
    void findBones()
      {
          QCOMPARE( skeletonPresets(), QStringList( { "quaternius", "mixamo", "vroid", "xsens", "vrm" } ) );
          QCOMPARE( humanBoneOf( "leftUpperArm" ), int( HumanBone::LeftUpperArm ) );
          QCOMPARE( humanBoneOf( "lefthand" ), -1 );
          HumanBoneNames names;
          QVERIFY( skeletonBoneNames( "mixamo", &names ) );
          QCOMPARE( names[int( HumanBone::LeftUpperArm )], QString( "LeftArm" ) );
          QCOMPARE( names[int( HumanBone::Chest )], QString( "Spine1" ) );
          QVERIFY( skeletonBoneNames( "xsens", &names ) );
          QCOMPARE( names[int( HumanBone::LeftShoulder )], QString( "LeftCollar" ) );
          QCOMPARE( names[int( HumanBone::LeftUpperArm )], QString( "LeftShoulder" ) );
          QCOMPARE( names[int( HumanBone::UpperChest )], QString( "Chest4" ) );
          QVERIFY( ! skeletonBoneNames( "vrm", &names ) );
          QVERIFY( ! skeletonBoneNames( "biped", &names ) );

          struct Case {
              const char * preset;
              const char * prefix;
              const char * hips;
          };
          for ( const Case & c : { Case{ "quaternius", "", "pelvis" }, Case{ "mixamo", "", "Hips" },
                                   Case{ "mixamo", "mixamorig:", "mixamorig:Hips" }, Case{ "mixamo", "mixamorig_", "mixamorig_Hips" },
                                   Case{ "vroid", "", "J_Bip_C_Hips" }, Case{ "xsens", "", "Hips" } } )
          {
              TestGltf::HumanoidOptions o;
              o.preset = c.preset;
              o.prefix = c.prefix;
              const GltfModel m = boxes( o );
              HumanBoneNodes nodes;
              QString error;
              // by the preset, and found
              for ( const QString & preset : { QString( c.preset ), QString() } )
              {
                  QVERIFY2( findHumanBones( m, preset, {}, &nodes, &error ), qPrintable( error ) );
                  QCOMPARE( m.node_names[nodes[int( HumanBone::Hips )]], QString( c.hips ) );
                  for ( int b = 0; b < HUMAN_BONE_COUNT; ++b ) QVERIFY( nodes[b] >= 0 );
              }
              // another preset: not (VRoid's J_Bip_C_Hips is Mixamo's Hips after a prefix, but not its arms)
              QVERIFY( ! findHumanBones( m, QString( c.preset ) == "vroid" ? "mixamo" : "vroid", {}, &nodes, &error ) );
              QVERIFY2( error.startsWith( "no bone for " ) && error.contains( "leftUpperArm" ) && error.contains( "(skeleton " ),
                        qPrintable( error ) );
          }

          // VRM's roles, by the preset or found before the names
          for ( const int version : { 0, 1 } )
          {
              TestGltf::HumanoidOptions o;
              o.vrm = version;
              o.preset = "mixamo";
              const GltfModel m = boxes( o );
              HumanBoneNodes nodes;
              QString error;
              QVERIFY( findHumanBones( m, "vrm", {}, &nodes, &error ) );
              QCOMPARE( m.node_names[nodes[int( HumanBone::LeftUpperArm )]], QString( "LeftArm" ) );
              QVERIFY( findHumanBones( m, QString(), {}, &nodes, &error ) );
              QCOMPARE( m.node_names[nodes[int( HumanBone::RightFoot )]], QString( "RightFoot" ) );
              // its roles found, its bones of no preset's names
              TestGltf::File odd = TestGltf::humanoid( TestGltf::mannequin(), o );
              for ( TestGltf::Node & n : odd.nodes ) n.name = "B-" + n.name;
              GltfModel om;
              QVERIFY( om.load( odd.glb() ) );
              QVERIFY2( findHumanBones( om, QString(), {}, &nodes, &error ), qPrintable( error ) );
              QCOMPARE( om.node_names[nodes[int( HumanBone::LeftUpperArm )]], QString( "B-LeftArm" ) );
          }
          HumanBoneNodes nodes;
          QString error;
          const GltfModel plain = boxes( TestGltf::HumanoidOptions() );
          QVERIFY( ! findHumanBones( plain, "vrm", {}, &nodes, &error ) );
          QCOMPARE( error, QString( "not a VRM model (no humanoid bones)" ) );
          QVERIFY( ! findHumanBones( plain, "biped", {}, &nodes, &error ) );
          QCOMPARE( error, QString( "no skeleton \"biped\"" ) );

          // a pack's map, over a preset
          std::vector< std::pair< QString, QString > > map;
          for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
          {
              if ( humanBoneRequired( HumanBone( b ) ) ) map.emplace_back( humanBoneName( HumanBone( b ) ), quaterniusBoneNames()[b] );
          }
          QVERIFY2( findHumanBones( plain, "mixamo", map, &nodes, &error ), qPrintable( error ) );
          QCOMPARE( plain.node_names[nodes[int( HumanBone::LeftHand )]], QString( "hand_l" ) );
          QCOMPARE( nodes[int( HumanBone::Neck )], -1 ); // not in the map
          map[0].second = "Pelvis";
          QVERIFY( ! findHumanBones( plain, QString(), map, &nodes, &error ) );
          QCOMPARE( error, QString( "no bone \"Pelvis\" for hips" ) );
          map[0].first = "pelvis";
          QVERIFY( ! findHumanBones( plain, QString(), map, &nodes, &error ) );
          QCOMPARE( error, QString( "no role \"pelvis\"" ) );
          map.erase( map.begin() );
          QVERIFY( ! findHumanBones( plain, QString(), map, &nodes, &error ) );
          QCOMPARE( error, QString( "no bone for hips" ) );

          // nothing known
          TestGltf::File odd = TestGltf::humanoid( TestGltf::mannequin(), TestGltf::HumanoidOptions() );
          for ( TestGltf::Node & n : odd.nodes ) n.name = "bone_" + n.name;
          for ( TestGltf::Node & n : odd.nodes ) n.name.replace( "_", "-" );
          GltfModel m;
          QVERIFY( m.load( odd.glb() ) );
          QVERIFY( ! findHumanBones( m, QString(), {}, &nodes, &error ) );
          QVERIFY2( error.startsWith( "the bones are of no skeleton known (quaternius, mixamo, vroid, xsens, vrm): no bone for hips," ), qPrintable( error ) );
      }

    /*!
      the T pose of a model at rest in an A pose: its upper arms taken as
      straight out sideways (and its lower arms with them), none of the
      others; a T pose (and the Mannequin, within ARM_POSE_TOLERANCE) as it is
    */
    void restTurn()
      {
          for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
          {
              QVERIFY( M_rig.restTurn( HumanBone( b ) ).isIdentity() );
          }
          TestGltf::HumanoidOptions a;
          a.arm_down = 40.0f;
          const GltfModel m = boxes( a );
          const HumanoidRig rig = rigOf( m );
          std::vector< QMatrix4x4 > g;
          m.globals( m.rest, g );
          const auto at = [&]( const HumanBone bone ) { return g[rig.node( bone )].column( 3 ).toVector3D(); };
          const HumanBone arms[2][3] = {
              { HumanBone::LeftUpperArm, HumanBone::LeftLowerArm, HumanBone::LeftHand },
              { HumanBone::RightUpperArm, HumanBone::RightLowerArm, HumanBone::RightHand },
          };
          for ( int side = 0; side < 2; ++side )
          {
              const QVector3D out( side == 0 ? 1.0f : -1.0f, 0.0f, 0.0f );
              const QQuaternion turn = rig.restTurn( arms[side][0] );
              QVERIFY( ! turn.isIdentity() );
              // the upper arm out, the lower arm with it
              QVERIFY( ( turn.rotatedVector( ( at( arms[side][1] ) - at( arms[side][0] ) ).normalized() ) - out ).length() < 1.0e-4f );
              QVERIFY( same( rig.restTurn( arms[side][1] ), turn ) );
              QVERIFY( same( rig.restTurn( arms[side][2] ), turn ) );
              // (the lower arm as straight as the Mannequin's, within the tolerance of out)
              const float lower = QVector3D::dotProduct( turn.rotatedVector( ( at( arms[side][2] ) - at( arms[side][1] ) ).normalized() ), out );
              QVERIFY( lower > std::cos( HumanoidRig::ARM_POSE_TOLERANCE * 3.14159265f / 180.0f ) );
          }
          for ( const HumanBone b : { HumanBone::Hips, HumanBone::Head, HumanBone::LeftShoulder, HumanBone::LeftUpperLeg, HumanBone::RightFoot } )
          {
              QVERIFY( rig.restTurn( b ).isIdentity() );
          }
          // the elbows bent too: the lower arms straightened
          a.elbow = 30.0f;
          const GltfModel bent = boxes( a );
          const HumanoidRig bent_rig = rigOf( bent );
          bent.globals( bent.rest, g );
          const QVector3D lower = ( g[bent_rig.node( HumanBone::LeftHand )].column( 3 ) - g[bent_rig.node( HumanBone::LeftLowerArm )].column( 3 ) ).toVector3D();
          QVERIFY( ( bent_rig.restTurn( HumanBone::LeftLowerArm ).rotatedVector( lower.normalized() ) - QVector3D( 1.0f, 0.0f, 0.0f ) ).length() < 1.0e-4f );
          QVERIFY( ! same( bent_rig.restTurn( HumanBone::LeftLowerArm ), bent_rig.restTurn( HumanBone::LeftUpperArm ) ) );
          // a little down: as it is
          a.arm_down = HumanoidRig::ARM_POSE_TOLERANCE - 2.0f;
          a.elbow = 0.0f;
          const GltfModel little = boxes( a );
          QVERIFY( rigOf( little ).restTurn( HumanBone::LeftUpperArm ).isIdentity() );
      }

    /*!
      which way a model faces: +z (0), -z (180, a VRM 0.x model); turned to
      the front (GltfModel::turnAbout()), +z
    */
    void facing()
      {
          QVERIFY( std::fabs( M_rig.facingAngle() ) < 1.0e-3f );
          TestGltf::HumanoidOptions o;
          o.facing = 180.0f;
          GltfModel m = boxes( o );
          HumanBoneNodes nodes;
          QVERIFY( findHumanBones( m, QString(), {}, &nodes ) );
          HumanoidRig rig;
          QVERIFY( rig.build( m, nodes ) );
          QVERIFY2( std::fabs( std::fabs( rig.facingAngle() ) - 180.0f ) < 1.0e-3f, qPrintable( QString::number( rig.facingAngle() ) ) );
          m.turnAbout( QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, -rig.facingAngle() ) );
          QVERIFY( rig.build( m, nodes ) );
          QVERIFY( std::fabs( rig.facingAngle() ) < 1.0e-3f );
          // a quarter turned: 90 to the right of +z faces +x
          GltfModel side = boxes( TestGltf::HumanoidOptions() );
          side.turnAbout( QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, 90.0f ) );
          HumanoidRig side_rig;
          QVERIFY( side_rig.build( side, quaterniusBoneNames() ) );
          QVERIFY2( std::fabs( side_rig.facingAngle() - 90.0f ) < 1.0e-3f, qPrintable( QString::number( side_rig.facingAngle() ) ) );
      }

    /*!
      a motion of the Mannequin put on its joints in other skeletons: of
      other names, without its rest rotations, in an A pose (its elbows bent
      too), turned to the front from -z: every role's joint where the
      Mannequin's is
    */
    void retargetPoses()
      {
          struct Case {
              float arm_down;
              float elbow;
              float facing;
              const char * preset;
          };
          for ( const Case & c : { Case{ 0.0f, 0.0f, 0.0f, "quaternius" }, Case{ 40.0f, 0.0f, 0.0f, "mixamo" },
                                   Case{ 45.0f, 25.0f, 0.0f, "vroid" }, Case{ 35.0f, 0.0f, 180.0f, "mixamo" },
                                   Case{ 0.0f, 0.0f, -70.0f, "quaternius" } } )
          {
              TestGltf::HumanoidOptions o;
              o.arm_down = c.arm_down;
              o.elbow = c.elbow;
              o.facing = c.facing;
              o.preset = c.preset;
              GltfModel m = boxes( o );
              HumanBoneNodes nodes;
              QVERIFY( findHumanBones( m, QString(), {}, &nodes ) );
              HumanoidRig rig;
              QVERIFY( rig.build( m, nodes ) );
              if ( c.facing != 0.0f )
              {
                  m.turnAbout( QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, -rig.facingAngle() ) );
                  QVERIFY( rig.build( m, nodes ) );
              }
              const QString what = QString( "%1 %2 %3 %4" ).arg( c.arm_down ).arg( c.elbow ).arg( c.facing ).arg( c.preset );
              for ( const char * clip : { "Idle_Loop", "Walk_Loop", "Sprint_Loop" } )
              {
                  for ( const float time : { 0.0f, 0.37f, 0.81f } )
                  {
                      std::vector< NodeTransform > motion = M_model.rest;
                      M_model.sample( M_model.clips[M_model.clipIndex( clip )], time, motion );
                      const HumanoidPose pose = M_rig.poseOf( motion );
                      const std::vector< QVector3D > want = rolePlaces( M_rig, pose );
                      const std::vector< QVector3D > got = rolePlaces( rig, pose );
                      for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
                      {
                          // an arm of an A pose taken as straight out, the Mannequin's 1 to 3 degrees
                          // from it: its elbow and hand within 2.5 cm; all else within 2 mm
                          const bool arm = ( c.arm_down > 0.0f
                                             && ( b == int( HumanBone::LeftLowerArm ) || b == int( HumanBone::LeftHand )
                                                  || b == int( HumanBone::RightLowerArm ) || b == int( HumanBone::RightHand ) ) );
                          QVERIFY2( ( got[b] - want[b] ).length() < ( arm ? 0.025f : 2.0e-3f ),
                                    qPrintable( what + QString( " %1 %2 %3: %4 m" ).arg( clip ).arg( time ).arg( humanBoneName( HumanBone( b ) ) )
                                                .arg( ( got[b] - want[b] ).length() ) ) );
                      }
                  }
              }
              // and back: a pose read of it put on the Mannequin
              std::vector< NodeTransform > local = m.rest;
              const HumanoidPose rest_pose = rig.poseOf( local );
              const std::vector< QVector3D > back = rolePlaces( M_rig, rest_pose );
              const std::vector< QVector3D > own = rolePlaces( rig, rest_pose );
              QVERIFY( ( back[int( HumanBone::LeftHand )] - own[int( HumanBone::LeftHand )] ).length() < 0.025f );
              // its rest is its own A pose: the Mannequin's arms put down as far
              if ( c.arm_down > 0.0f )
              {
                  const QVector3D arm = back[int( HumanBone::LeftLowerArm )] - back[int( HumanBone::LeftUpperArm )];
                  const float down = std::atan2( -arm.y(), std::hypot( arm.x(), arm.z() ) ) * 180.0f / 3.14159265f;
                  QVERIFY2( std::fabs( down - c.arm_down ) < 3.0f, qPrintable( what + QString( ": %1" ).arg( down ) ) );
              }
          }
      }

    //! a bone's name without its skeleton's prefix
    void boneKeys()
      {
          QCOMPARE( boneKey( "mixamorig:Hips" ), QString( "Hips" ) );
          QCOMPARE( boneKey( "mixamorig1:LeftArm" ), QString( "LeftArm" ) );
          QCOMPARE( boneKey( "a:b:c" ), QString( "c" ) );
          QCOMPARE( boneKey( "pelvis" ), QString( "pelvis" ) );
          QCOMPARE( boneKey( "J_Bip_C_Hips" ), QString( "J_Bip_C_Hips" ) );
      }

    /*!
      a clip put on another skeleton (retargetClip()): keys at the rate to
      the clip's end; at each, every role's bone turned and the hips placed
      as the pose put on it directly (apply()); on the same skeleton, the
      bones without a role of the same names (the fingers) as the clip's
    */
    void retargetClips()
      {
          const GltfModel::Clip & walk = M_model.clips[M_model.clipIndex( "Walk_Loop" )];
          TestGltf::HumanoidOptions o;
          o.preset = "mixamo";
          o.arm_down = 40.0f;
          const GltfModel box = boxes( o );
          const HumanoidRig rig = rigOf( box );
          const GltfModel::Clip put = retargetClip( M_model, M_rig, walk, box, rig, false );
          QCOMPARE( put.duration, walk.duration );
          const int steps = int( std::ceil( walk.duration * 30.0f - 1.0e-3f ) );
          const int hips = rig.node( HumanBone::Hips );
          QCOMPARE( int( put.nodes[hips].t.times.size() ), steps + 1 );
          QCOMPARE( put.nodes[hips].r.times.back(), walk.duration );
          QVERIFY( put.nodes[box.nodeIndex( "Body" )].empty() ); // no role, no name of the clip's
          const float end = walk.duration - 1.0e-4f;
          for ( const int k : { 0, 7, steps / 2, steps } )
          {
              const float t = ( k == steps ? end : walk.duration * k / steps );
              std::vector< NodeTransform > local = M_model.rest;
              M_model.sample( walk, t, local );
              std::vector< NodeTransform > want = box.rest;
              rig.apply( M_rig.poseOf( local ), want );
              std::vector< NodeTransform > got = box.rest;
              box.sample( put, t, got );
              for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
              {
                  const int n = rig.node( HumanBone( b ) );
                  if ( n < 0 ) continue;
                  QVERIFY2( std::fabs( QQuaternion::dotProduct( got[n].r, want[n].r ) ) > 0.99999f,
                            qPrintable( QString( "%1 %2" ).arg( k ).arg( humanBoneName( HumanBone( b ) ) ) ) );
              }
              QVERIFY( ( got[hips].t - want[hips].t ).length() < 1.0e-4f );
          }
          // on itself: the roles as they were, the fingers too
          const GltfModel::Clip self = retargetClip( M_model, M_rig, walk, M_model, M_rig, false );
          const int finger = M_model.nodeIndex( "index_02_l" );
          QVERIFY( finger >= 0 && ! self.nodes[finger].r.empty() );
          std::vector< NodeTransform > want = M_model.rest;
          M_model.sample( walk, 0.4f, want );
          std::vector< NodeTransform > got = M_model.rest;
          M_model.sample( self, 0.4f, got );
          for ( const int n : { finger, M_rig.node( HumanBone::LeftLowerArm ), M_rig.node( HumanBone::RightFoot ) } )
          {
              QVERIFY2( std::fabs( QQuaternion::dotProduct( got[n].r, want[n].r ) ) > 0.9999f, qPrintable( M_model.node_names[n] ) );
          }
      }

    /*!
      a clip not in place (its root going forward a stride: a root motion):
      its drift (clipDrift()) the stride in hip heights; put in place, none,
      the rest of the motion as it was (the hips' height and sway kept);
      not in place, the drift kept
    */
    void inPlace()
      {
          const GltfModel::Clip & walk = M_model.clips[M_model.clipIndex( "Walk_Loop" )];
          QVERIFY( clipDrift( M_model, M_rig, walk ) < 1.0e-3f );
          GltfModel::Clip forward = walk;
          const int root = M_model.nodeIndex( "root" );
          QVERIFY( root >= 0 );
          // +z of the model in the root's parent's frame
          std::vector< QMatrix4x4 > global;
          M_model.globals( M_model.rest, global );
          const int p = M_model.parent[root];
          const QVector3D ahead = ( p >= 0 ? global[p].inverted().mapVector( QVector3D( 0.0f, 0.0f, 1.28f ) )
                                    : QVector3D( 0.0f, 0.0f, 1.28f ) );
          // (its keys sampled, the stride added in a line)
          GltfModel::Channel & ch = forward.nodes[root].t;
          ch = GltfModel::Channel();
          for ( int k = 0; k <= 40; ++k )
          {
              const float t = walk.duration * k / 40;
              std::vector< NodeTransform > local = M_model.rest;
              M_model.sample( walk, std::min( t, walk.duration - 1.0e-4f ), local );
              const QVector3D at = local[root].t + ahead * ( float( k ) / 40 );
              ch.times.push_back( t );
              ch.values.insert( ch.values.end(), { at.x(), at.y(), at.z() } );
          }
          const float drift = clipDrift( M_model, M_rig, forward ) * M_rig.hipHeight();
          QVERIFY2( std::fabs( drift - 1.28f ) < 0.01f, qPrintable( QString::number( drift ) ) );

          const GltfModel::Clip put = retargetClip( M_model, M_rig, forward, M_model, M_rig, true );
          QVERIFY( clipDrift( M_model, M_rig, put ) < 1.0e-3f );
          for ( const float t : { 0.0f, 0.4f, 0.8f, 1.2f } )
          {
              std::vector< NodeTransform > local = M_model.rest;
              M_model.sample( put, t, local );
              const HumanoidPose got = M_rig.poseOf( local );
              local = M_model.rest;
              M_model.sample( walk, t, local );
              const HumanoidPose want = M_rig.poseOf( local );
              QVERIFY2( ( got.hips - want.hips ).length() < 1.0e-3f, qPrintable( QString::number( t ) ) );
          }
          const GltfModel::Clip kept = retargetClip( M_model, M_rig, forward, M_model, M_rig, false );
          QVERIFY( std::fabs( clipDrift( M_model, M_rig, kept ) * M_rig.hipHeight() - 1.28f ) < 0.01f );
      }

    /*!
      a skeleton saved in a pose, turned as another at rest (restAs()):
      each bone of the same name (the prefix aside) turned and placed as
      the other's; a bone of its own kept
    */
    void restLike()
      {
          TestGltf::HumanoidOptions o;
          o.preset = "mixamo";
          o.prefix = "mixamorig:";
          const TestGltf::File f = TestGltf::humanoid( TestGltf::mannequin(), o );
          GltfModel rest;
          QVERIFY( rest.load( f.glb() ) );
          TestGltf::File posed = f;
          for ( TestGltf::Node & n : posed.nodes )
          {
              n.name.replace( "mixamorig:", "mixamorig1:" );
              n.r = n.r * QQuaternion::fromAxisAndAngle( 1.0f, 0.2f, 0.0f, 25.0f );
              n.t += QVector3D( 0.01f, 0.02f, 0.0f );
          }
          const QQuaternion own = QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, 30.0f );
          posed.nodes.push_back( TestGltf::Node{ "Extra", 2, QVector3D( 0.0f, 0.1f, 0.0f ), own } );
          GltfModel m;
          QVERIFY( m.load( posed.glb() ) );
          QCOMPARE( restAs( m, rest ), int( rest.rest.size() ) );
          std::vector< QMatrix4x4 > want, got;
          rest.globals( rest.rest, want );
          m.globals( m.rest, got );
          for ( std::size_t i = 0; i < rest.rest.size(); ++i )
          {
              const int j = m.nodeIndex( QString( rest.node_names[i] ).replace( "mixamorig:", "mixamorig1:" ) );
              QVERIFY( j >= 0 );
              for ( int k = 0; k < 16; ++k )
              {
                  QVERIFY2( std::fabs( got[j].constData()[k] - want[i].constData()[k] ) < 1.0e-4f, qPrintable( rest.node_names[i] ) );
              }
          }
          const int extra = m.nodeIndex( "Extra" );
          QVERIFY( std::fabs( QQuaternion::dotProduct( m.rest[extra].r, own ) ) > 0.99999f );
          QVERIFY( ( m.rest[extra].t - QVector3D( 0.0f, 0.1f, 0.0f ) ).length() < 1.0e-6f );
      }

    //! the bones without a role keep what the caller gave them
    void otherBones()
      {
          std::vector< NodeTransform > motion = M_model.rest;
          M_model.sample( M_model.clips[M_model.clipIndex( "Idle_Loop" )], 0.5f, motion );
          std::vector< NodeTransform > local = motion; // the fingers of the clip
          const int finger = M_model.nodeIndex( "index_02_l" );
          local[finger].r = QQuaternion::fromAxisAndAngle( 1.0f, 0.0f, 0.0f, 30.0f );
          M_rig.apply( M_rig.poseOf( motion ), local );
          QVERIFY( same( local[finger].r, QQuaternion::fromAxisAndAngle( 1.0f, 0.0f, 0.0f, 30.0f ) ) );
      }
};

QTEST_MAIN( TestHumanoid )

#include "test_humanoid.moc"
