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

#include <cmath>

namespace {

QByteArray
mannequin()
{
    QFile file( QStringLiteral( TEST_SOURCE_DIR "/resources/models/Mannequin.glb" ) );
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
