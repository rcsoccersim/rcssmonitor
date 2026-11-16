// -*-c++-*-

/*!
  \file test_bvh_model.cpp
  \brief unit test of BvhModel: BVH files read into a GltfModel
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

#include "bvh_model.h"
#include "gltf_model.h"
#include "humanoid.h"
#include "test_bvh.h"
#include "test_gltf.h"

#include <QTemporaryDir>

#include <cmath>

namespace {

/*
  a small skeleton: the hips (moved, from an OFFSET not at the origin, and turned, Z X Y), the spine (Z X Y),
  the neck (X Z Y) with an End Site, and another "Spine" (a name taken)
  moved along y only; 3 frames of 0.1 s
*/
const QByteArray SMALL =
    "HIERARCHY\n"
    "ROOT Hips\n"
    "{\n"
    "  OFFSET 4 90 -3\n"
    "  CHANNELS 6 Xposition Yposition Zposition Zrotation Xrotation Yrotation\n"
    "  JOINT Spine\n"
    "  {\n"
    "    OFFSET 0 10 0\n"
    "    CHANNELS 3 Zrotation Xrotation Yrotation\n"
    "    JOINT Neck\n"
    "    {\n"
    "      OFFSET 0 20 1\n"
    "      CHANNELS 3 Xrotation Zrotation Yrotation\n"
    "      End Site\n"
    "      {\n"
    "        OFFSET 0 5 0\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "  JOINT Spine\n"
    "  {\n"
    "    OFFSET 1 0 3\n"
    "    CHANNELS 1 Yposition\n"
    "    End Site\n"
    "    {\n"
    "      OFFSET 0 0 2\n"
    "    }\n"
    "  }\n"
    "}\n"
    "MOTION\n"
    "Frames: 3\n"
    "Frame Time: 0.1\n"
    // hips (x y z, z x y), spine (z x y), neck (x z y), the other spine (y)
    "1 91 2 0 0 0   90 90 0   90 90 0   7\n"
    "3 92 4 0 0 0   0 0 90    0 0 0     8\n"
    "5 93 6 0 0 0   0 0 0     0 0 0     9\n";

//! a node's rotation in a clip at a key
QQuaternion
keyOf( const GltfModel::Channel & r,
       const int k )
{
    return QQuaternion( r.values[k * 4 + 3], r.values[k * 4], r.values[k * 4 + 1], r.values[k * 4 + 2] );
}

QVector3D
moveOf( const GltfModel::Channel & t,
        const int k )
{
    return QVector3D( t.values[k * 3], t.values[k * 3 + 1], t.values[k * 3 + 2] );
}

bool
near( const QVector3D & a,
      const QVector3D & b,
      const float tolerance = 1.0e-4f )
{
    return ( a - b ).length() < tolerance;
}

}

/*!
  \brief unit test of BvhModel
*/
class TestBvhModel
    : public QObject {
    Q_OBJECT

private slots:

    //! a BVH file: by its header (after a byte order mark or white space), or by its name
    void isBvh()
      {
          QVERIFY( BvhModel::isBvh( SMALL, QString() ) );
          QVERIFY( BvhModel::isBvh( "\xEF\xBB\xBF\r\n  HIERARCHY\n", QString() ) );
          QVERIFY( BvhModel::isBvh( "anything", "dir/Walk.BVH" ) );
          QVERIFY( ! BvhModel::isBvh( "glTF", "dir/Walk.glb" ) );
          QVERIFY( ! BvhModel::isBvh( "HIERARCH", QString() ) );
      }

    /*!
      the nodes (End Sites too, and a name taken given another), their
      rest at their OFFSETs, not turned; the clip named after the file, a
      key a frame; a position channel in place of the OFFSET (its other
      axes kept); the rotation channels in the order written
    */
    void small()
      {
          GltfModel m;
          QString error;
          QVERIFY2( BvhModel::read( SMALL, "dir/small.bvh", m, &error ), qPrintable( error ) );
          QCOMPARE( m.node_names, std::vector< QString >( { "Hips", "Spine", "Neck", "Neck_End", "Spine.2", "Spine.2_End" } ) );
          QCOMPARE( m.parent, std::vector< int >( { -1, 0, 1, 2, 0, 4 } ) );
          QCOMPARE( m.order, std::vector< int >( { 0, 1, 2, 3, 4, 5 } ) );
          QVERIFY( near( m.rest[0].t, QVector3D( 4.0f, 90.0f, -3.0f ) ) );
          QVERIFY( near( m.rest[2].t, QVector3D( 0.0f, 20.0f, 1.0f ) ) );
          QVERIFY( near( m.rest[3].t, QVector3D( 0.0f, 5.0f, 0.0f ) ) );
          QVERIFY( near( m.rest[5].t, QVector3D( 0.0f, 0.0f, 2.0f ) ) );
          for ( const NodeTransform & r : m.rest ) QCOMPARE( r.r, QQuaternion() );
          QVERIFY( ! m.posed_rest );
          QVERIFY( m.vertices.empty() && m.joint_nodes.empty() );

          QCOMPARE( int( m.clips.size() ), 1 );
          const GltfModel::Clip & c = m.clips[0];
          QCOMPARE( c.name, QString( "small" ) );
          QVERIFY( std::fabs( c.duration - 0.2f ) < 1.0e-6f );
          QCOMPARE( int( c.nodes.size() ), 6 );
          QCOMPARE( c.nodes[0].r.times, std::vector< float >( { 0.0f, 0.1f, 0.2f } ) );
          // the hips placed by their channels, not their OFFSET
          QVERIFY( near( moveOf( c.nodes[0].t, 0 ), QVector3D( 1.0f, 91.0f, 2.0f ) ) );
          QVERIFY( near( moveOf( c.nodes[0].t, 2 ), QVector3D( 5.0f, 93.0f, 6.0f ) ) );
          // the other spine: y by its channel, x and z of its OFFSET; not turned
          QVERIFY( near( moveOf( c.nodes[4].t, 1 ), QVector3D( 1.0f, 8.0f, 3.0f ) ) );
          QVERIFY( c.nodes[4].r.empty() );
          QVERIFY( c.nodes[1].t.empty() && c.nodes[3].empty() && c.nodes[5].empty() );

          // Z X Y: R = Rz Rx Ry; X Z Y: R = Rx Rz Ry
          const QVector3D up( 0.0f, 1.0f, 0.0f );
          QVERIFY( near( keyOf( c.nodes[1].r, 0 ).rotatedVector( up ), QVector3D( 0.0f, 0.0f, 1.0f ) ) );
          QVERIFY( near( keyOf( c.nodes[2].r, 0 ).rotatedVector( up ), QVector3D( -1.0f, 0.0f, 0.0f ) ) );
          QVERIFY( near( keyOf( c.nodes[1].r, 1 ).rotatedVector( QVector3D( 1.0f, 0.0f, 0.0f ) ), QVector3D( 0.0f, 0.0f, -1.0f ) ) );
          QCOMPARE( keyOf( c.nodes[1].r, 2 ), QQuaternion() );

          // sampled: between the keys
          std::vector< NodeTransform > local = m.rest;
          m.sample( c, 0.05f, local );
          QVERIFY( near( local[0].t, QVector3D( 2.0f, 91.5f, 3.0f ) ) );

          // without a path: "motion"
          QVERIFY( BvhModel::read( SMALL, QString(), m, &error ) );
          QCOMPARE( m.clips[0].name, QString( "motion" ) );
      }

    //! a motion cut short keeps its whole frames; no frames: a skeleton without a clip
    void shortMotion()
      {
          GltfModel m;
          QString error;
          QByteArray cut = SMALL;
          cut.chop( cut.size() - cut.lastIndexOf( "0 0 0     9" ) ); // the third frame cut in the middle
          QVERIFY2( BvhModel::read( cut, QString(), m, &error ), qPrintable( error ) );
          QCOMPARE( int( m.clips.size() ), 1 );
          QCOMPARE( int( m.clips[0].nodes[0].r.times.size() ), 2 );
          QVERIFY( std::fabs( m.clips[0].duration - 0.1f ) < 1.0e-6f );

          QByteArray none = SMALL.left( SMALL.indexOf( "MOTION" ) ) + "MOTION\nFrames: 0\nFrame Time: 0.0333333\n";
          QVERIFY2( BvhModel::read( none, QString(), m, &error ), qPrintable( error ) );
          QVERIFY( m.clips.empty() );
          QCOMPARE( int( m.rest.size() ), 6 );
      }

    //! what is wrong, told; nothing kept of the file
    void broken()
      {
          const auto why = []( const QByteArray & text ) {
              GltfModel m;
              QString error;
              if ( BvhModel::read( text, QString(), m, &error ) ) return QString( "read" );
              if ( ! m.rest.empty() || ! m.clips.empty() ) return QString( "kept" );
              return error;
          };
          const auto replaced = []( const char * from, const char * to ) { return QByteArray( SMALL ).replace( from, to ); };
          QCOMPARE( why( "MOTION" ), QString( "not a BVH file (no HIERARCHY)" ) );
          QCOMPARE( why( "HIERARCHY\nMOTION\n" ), QString( "no ROOT in the hierarchy" ) );
          QCOMPARE( why( replaced( "Xrotation Zrotation", "Xrotation Wrotation" ) ), QString( "an unknown channel at Neck" ) );
          QCOMPARE( why( replaced( "CHANNELS 3 Zrotation", "CHANNELS 7 Zrotation" ) ), QString( "a wrong number of channels at Spine" ) );
          QCOMPARE( why( replaced( "End Site\n      {", "End Sight\n      {" ) ), QString( "a broken End Site at Neck" ) );
          QCOMPARE( why( replaced( "OFFSET 0 20 1", "OFFSET 0 20" ) ), QString( "a broken OFFSET at Neck" ) );
          QCOMPARE( why( replaced( "JOINT Neck", "JOINT" ) ), QString( "a joint without a name" ) );
          QCOMPARE( why( replaced( "JOINT Neck\n    {", "JOINT Neck\n    (" ) ), QString( "no { after the joint Neck" ) );
          QCOMPARE( why( SMALL.left( SMALL.indexOf( "  JOINT Spine\n  {\n    OFFSET 1" ) ) ), QString( "the hierarchy is cut short" ) );
          QCOMPARE( why( replaced( "MOTION", "MOTIONS" ) ), QString( "no MOTION after the hierarchy" ) );
          QCOMPARE( why( replaced( "Frames: 3", "Frames: x" ) ), QString( "no number of frames" ) );
          QCOMPARE( why( replaced( "Frame Time: 0.1", "Frame Time: x" ) ), QString( "no frame time" ) );
          QCOMPARE( why( replaced( "Frame Time: 0.1", "FrameTime: 0.1" ) ), QString( "no frame time" ) );
          QCOMPARE( why( replaced( "Frame Time: 0.1", "Frame Time: 0" ) ), QString( "a frame time not above 0" ) );
          QCOMPARE( why( replaced( "  JOINT Spine\n  {\n    OFFSET 1", "  BONE Spine\n  {\n    OFFSET 1" ) ), QString( "\"BONE\" in the joint Hips" ) );
      }

    //! by GltfModel: a BVH file has no mesh; read as a file of clips
    void loaded()
      {
          GltfModel m;
          QString error;
          QVERIFY( ! m.load( SMALL, &error ) );
          QCOMPARE( error, QString( "no mesh (a BVH file has a motion only)" ) );
          QVERIFY2( m.load( SMALL, &error, QString(), false ), qPrintable( error ) );
          QCOMPARE( int( m.rest.size() ), 6 );
          QCOMPARE( m.height(), 0.0f );

          QTemporaryDir dir;
          const QString path = dir.filePath( "Walking.bvh" );
          QVERIFY( TestGltf::File::write( path, SMALL ) );
          QVERIFY2( m.loadFile( path, &error, false ), qPrintable( error ) );
          QCOMPARE( m.clips[0].name, QString( "Walking" ) );
      }

    /*!
      the Mannequin's walk written as BVH (its role bones as motion capture
      has them, by Mixamo's or XSens's names: not turned at rest), in meters
      or centimeters, the rotations Z X Y or X Y Z, positions on the root
      only or on every joint: found as the skeleton of the names, put on
      the Mannequin the same walk as its own
    */
    void mannequin()
      {
          const GltfModel & glb = TestGltf::mannequin();
          const GltfModel::Clip & walk = glb.clips[glb.clipIndex( "Walk_Loop" )];
          HumanoidRig glb_rig;
          QVERIFY( glb_rig.build( glb, quaterniusBoneNames() ) );
          struct Case {
              const char * name;
              float scale;
              const char * order;
              bool positions;
              const char * preset;
              const char * left_thigh; //!< the left upper leg's bone found
          };
          for ( const Case & k : { Case{ "meters", 1.0f, "ZXY", false, "mixamo", "LeftUpLeg" },
                                   Case{ "centimeters", 100.0f, "ZXY", false, "mixamo", "LeftUpLeg" },
                                   Case{ "XYZ, positions", 100.0f, "XYZ", true, "mixamo", "LeftUpLeg" },
                                   Case{ "XSens", 100.0f, "ZXY", false, "xsens", "LeftHip" } } )
          {
              TestBvh::Options o;
              o.preset = QString::fromLatin1( k.preset );
              o.scale = k.scale;
              o.order = QString::fromLatin1( k.order );
              o.positions = k.positions;
              GltfModel m;
              QString error;
              QVERIFY2( m.load( TestBvh::bvhOf( glb, &walk, o ), &error, "Walk_Loop.bvh", false ), qPrintable( error ) );
              QCOMPARE( int( m.rest.size() ), 22 + 5 ); // the role bones and 5 End Sites (head, hands, toes)
              QCOMPARE( int( m.clips.size() ), 1 );
              QVERIFY2( std::fabs( m.clips[0].duration - walk.duration ) < 0.04f, k.name );
              HumanBoneNodes nodes;
              QVERIFY2( findHumanBones( m, QString(), {}, &nodes, &error ), qPrintable( error ) );
              QCOMPARE( m.node_names[nodes[int( HumanBone::LeftUpperLeg )]], QString( k.left_thigh ) ); // the preset of the names found
              HumanoidRig rig;
              QVERIFY( rig.build( m, nodes ) );
              QCOMPARE( rig.upAxis(), QVector3D( 0.0f, 1.0f, 0.0f ) );
              QVERIFY2( std::fabs( rig.hipHeight() / k.scale - glb_rig.hipHeight() ) < 0.01f,
                        qPrintable( QString( "%1: %2" ).arg( k.name ).arg( rig.hipHeight() ) ) );
              const GltfModel::Clip put = retargetClip( m, rig, m.clips[0], glb, glb_rig, false );
              for ( float t = 0.0f; t < walk.duration - 0.05f; t += 0.1f )
              {
                  std::vector< NodeTransform > local = glb.rest;
                  glb.sample( put, t, local );
                  const HumanoidPose got = glb_rig.poseOf( local );
                  local = glb.rest;
                  glb.sample( walk, t, local );
                  const HumanoidPose wanted = glb_rig.poseOf( local );
                  for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
                  {
                      if ( glb_rig.node( HumanBone( b ) ) < 0 ) continue;
                      const float dot = std::fabs( QQuaternion::dotProduct( got.rotations[b], wanted.rotations[b] ) );
                      QVERIFY2( dot > 0.9995f, qPrintable( QString( "%1 %2 %3: %4" ).arg( k.name ).arg( t ).arg( humanBoneName( HumanBone( b ) ) ).arg( dot ) ) );
                  }
                  QVERIFY2( ( got.hips - wanted.hips ).length() < 0.005f, qPrintable( QString( "%1 %2" ).arg( k.name ).arg( t ) ) );
              }
          }

          // written with other axes up (Figures3D turns it upright): its up the nearest of the six axes
          struct Up {
              bool z_up;
              QQuaternion turn;
              QVector3D up;
          };
          const auto about = []( const float x, const float y, const float z, const float angle ) {
              return QQuaternion::fromAxisAndAngle( x, y, z, angle );
          };
          for ( const Up & u : { Up{ true, QQuaternion(), QVector3D( 0.0f, 0.0f, 1.0f ) },
                                 Up{ false, about( 1.0f, 0.0f, 0.0f, -90.0f ), QVector3D( 0.0f, 0.0f, -1.0f ) },
                                 Up{ false, about( 1.0f, 0.0f, 0.0f, 180.0f ), QVector3D( 0.0f, -1.0f, 0.0f ) },
                                 Up{ false, about( 0.0f, 0.0f, 1.0f, -90.0f ), QVector3D( 1.0f, 0.0f, 0.0f ) },
                                 Up{ false, about( 0.0f, 0.0f, 1.0f, 90.0f ), QVector3D( -1.0f, 0.0f, 0.0f ) },
                                 Up{ false, about( 1.0f, 0.0f, 0.0f, 20.0f ), QVector3D( 0.0f, 1.0f, 0.0f ) } } ) // leaning: still +y
          {
              TestBvh::Options o;
              o.z_up = u.z_up;
              o.turn = u.turn;
              GltfModel m;
              QString error;
              QVERIFY2( m.load( TestBvh::bvhOf( glb, &walk, o ), &error, QString(), false ), qPrintable( error ) );
              HumanBoneNodes nodes;
              HumanoidRig rig;
              QVERIFY( findHumanBones( m, "mixamo", {}, &nodes, &error ) && rig.build( m, nodes ) );
              QCOMPARE( rig.upAxis(), u.up );
          }
      }

    /*!
      the walk as Blender's BVH exporter writes it (test/data/bvh/walk.bvh,
      made by make_bvh.py): the root "root" above the role bones, End Sites
      at the head, the hands and the toes; z up (Blender's axes), in
      centimeters; positions on every joint
    */
    void blender()
      {
          GltfModel m;
          QString error;
          QVERIFY2( m.loadFile( QStringLiteral( TEST_SOURCE_DIR "/test/data/bvh/walk.bvh" ), &error, false ), qPrintable( error ) );
          QCOMPARE( int( m.rest.size() ), 1 + 22 + 5 );
          QCOMPARE( m.node_names[0], QString( "root" ) );
          QCOMPARE( m.clips[0].name, QString( "walk" ) );
          const GltfModel::Clip & want = TestGltf::mannequin().clips[TestGltf::mannequin().clipIndex( "Walk_Loop" )];
          QVERIFY2( std::fabs( m.clips[0].duration - want.duration ) < 1.0e-3f, qPrintable( QString::number( m.clips[0].duration ) ) );
          HumanBoneNodes nodes;
          QVERIFY2( findHumanBones( m, QString(), {}, &nodes, &error ), qPrintable( error ) ); // the Mannequin's names
          HumanoidRig rig;
          QVERIFY( rig.build( m, nodes ) );
          QCOMPARE( rig.upAxis(), QVector3D( 0.0f, 0.0f, 1.0f ) );
          const QVector3D hips = m.rest[nodes[int( HumanBone::Hips )]].t;
          QVERIFY2( hips.z() > 85.0f && hips.z() < 95.0f, qPrintable( QString::number( hips.z() ) ) ); // cm, z up
          QVERIFY( ! m.clips[0].nodes[nodes[int( HumanBone::Hips )]].t.empty() ); // the hips move
      }

    /*!
      a motion capture settled: written as one (the root's OFFSET at the
      origin, the legs below it; the motion 3 m and 2 m across the room,
      going 70 degrees off +z, in centimeters), its rest stands on y = 0
      above the origin, its clip starts above the origin, faces +z and comes
      down to y = 0; put on the Mannequin, the same walk as its own. Settled
      again, nothing moves. The same with a root "Reference" above the hips
      placing the motion in the room (it is moved, not the hips), turning
      it or not (the hips turned: the Reference gets keys of turning).
    */
    void settled()
      {
          const GltfModel & glb = TestGltf::mannequin();
          const GltfModel::Clip & walk = glb.clips[glb.clipIndex( "Walk_Loop" )];
          HumanoidRig glb_rig;
          QVERIFY( glb_rig.build( glb, quaterniusBoneNames() ) );
          for ( const int reference : { 0, 1, 2 } ) // the hips the root; a Reference above them, moving and turning; only moving
          {
              TestBvh::Options o;
              o.origin_root = true;
              o.room = QVector3D( 3.0f, 0.0f, -2.0f );
              o.heading = 70.0f;
              o.scale = 100.0f;
              o.reference = ( reference > 0 );
              o.reference_moves_only = ( reference == 2 );
              GltfModel m;
              QString error;
              QVERIFY2( m.load( TestBvh::bvhOf( glb, &walk, o ), &error, QString(), false ), qPrintable( error ) );
              QCOMPARE( m.node_names[0], reference ? QString( "Reference" ) : QString( "Hips" ) );
              QVERIFY( m.capture );
              HumanBoneNodes nodes;
              HumanoidRig rig;
              QVERIFY( findHumanBones( m, QString(), {}, &nodes, &error ) && rig.build( m, nodes ) );
              const int hips = nodes[int( HumanBone::Hips )];
              QVERIFY( m.rest[0].t.length() < 1.0e-4f ); // the root at the origin
              QVERIFY( settleCapture( m, rig ) );
              QVERIFY( rig.build( m, nodes ) );

              // the body: the hips and the bones below them (not the Reference)
              std::vector< bool > body( m.rest.size(), false );
              for ( const int i : m.order ) body[i] = ( i == hips || ( m.parent[i] >= 0 && body[m.parent[i]] ) );
              const auto lowestOf = [&body]( const std::vector< QMatrix4x4 > & g ) {
                  float y = 1.0e9f;
                  for ( std::size_t i = 0; i < g.size(); ++i )
                  {
                      if ( body[i] ) y = std::min( y, g[i]( 1, 3 ) );
                  }
                  return y;
              };
              const auto headingOf = [&]( const std::vector< QMatrix4x4 > & g ) {
                  const QVector3D l = ( g[nodes[int( HumanBone::LeftUpperLeg )]].column( 3 ) - g[nodes[int( HumanBone::RightUpperLeg )]].column( 3 ) ).toVector3D();
                  return std::atan2( -l.z(), l.x() ) * 180.0f / 3.14159265f;
              };
              // the rest: above the origin, standing on y = 0, the hips at their height (cm)
              std::vector< QMatrix4x4 > g;
              m.globals( m.rest, g );
              QVERIFY2( std::fabs( g[hips]( 0, 3 ) ) < 1.0e-3f && std::fabs( g[hips]( 2, 3 ) ) < 1.0e-3f, "the rest above the origin" );
              QVERIFY2( std::fabs( lowestOf( g ) ) < 1.0e-3f, qPrintable( QString::number( lowestOf( g ) ) ) );
              QVERIFY2( rig.hipHeight() > 80.0f && rig.hipHeight() < 100.0f, qPrintable( QString::number( rig.hipHeight() ) ) );
              // the clip: starting above the origin, coming down to y = 0, facing +z (its mean heading)
              float lowest = 1.0e9f;
              float heading = 0.0f;
              int samples = 0;
              for ( float t = 0.0f; t < m.clips[0].duration; t += 1.0f / 30.0f )
              {
                  std::vector< NodeTransform > local = m.rest;
                  m.sample( m.clips[0], t, local );
                  m.globals( local, g );
                  if ( t == 0.0f )
                  {
                      QVERIFY2( std::hypot( g[hips]( 0, 3 ), g[hips]( 2, 3 ) ) < 0.01f, qPrintable( QString( "%1 %2" ).arg( g[hips]( 0, 3 ) ).arg( g[hips]( 2, 3 ) ) ) );
                  }
                  lowest = std::min( lowest, lowestOf( g ) );
                  heading += headingOf( g );
                  ++samples;
              }
              QVERIFY2( std::fabs( lowest ) < 0.01f, qPrintable( QString::number( lowest ) ) );
              QVERIFY2( std::fabs( heading / samples ) < 2.0f, qPrintable( QString::number( heading / samples ) ) );
              // on the Mannequin: its walk (the hips a little off: the clip starts above the origin)
              const GltfModel::Clip put = retargetClip( m, rig, m.clips[0], glb, glb_rig, false );
              for ( float t = 0.0f; t < walk.duration - 0.05f; t += 0.1f )
              {
                  std::vector< NodeTransform > local = glb.rest;
                  glb.sample( put, t, local );
                  const HumanoidPose got = glb_rig.poseOf( local );
                  local = glb.rest;
                  glb.sample( walk, t, local );
                  const HumanoidPose wanted = glb_rig.poseOf( local );
                  for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
                  {
                      if ( glb_rig.node( HumanBone( b ) ) < 0 ) continue;
                      const float dot = std::fabs( QQuaternion::dotProduct( got.rotations[b], wanted.rotations[b] ) );
                      QVERIFY2( dot > 0.9995f, qPrintable( QString( "%1 %2: %3" ).arg( t ).arg( humanBoneName( HumanBone( b ) ) ).arg( dot ) ) );
                  }
                  QVERIFY2( ( got.hips - wanted.hips ).length() < 0.06f, qPrintable( QString( "%1: %2" ).arg( t ).arg( ( got.hips - wanted.hips ).length() ) ) );
              }

              // settled again: nothing moves (the root moved: the hips, or the Reference above them)
              const std::vector< NodeTransform > rest = m.rest;
              const std::vector< float > keys = m.clips[0].nodes[0].t.values;
              const std::vector< float > turns = m.clips[0].nodes[0].r.values;
              QVERIFY( settleCapture( m, rig ) );
              QVERIFY( ( m.rest[0].t - rest[0].t ).length() < 1.0e-3f );
              for ( std::size_t k = 0; k < keys.size(); ++k ) QVERIFY( std::fabs( m.clips[0].nodes[0].t.values[k] - keys[k] ) < 1.0e-3f );
              for ( std::size_t k = 0; k < turns.size(); ++k ) QVERIFY( std::fabs( m.clips[0].nodes[0].r.values[k] - turns[k] ) < 1.0e-4f );
          }
      }
};

QTEST_MAIN( TestBvhModel )
#include "test_bvh_model.moc"
