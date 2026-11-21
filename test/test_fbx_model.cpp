// -*-c++-*-

/*!
  \file test_fbx_model.cpp
  \brief unit test of FbxModel: FBX files read into a GltfModel
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

#include "fbx_model.h"
#include "gltf_model.h"
#include "humanoid.h"
#include "test_gltf.h"

#include <QTemporaryDir>

#include <cmath>
#include <set>

/*
  The files (test/data/fbx, made from the bundled Mannequin by
  make_fbx.py with Blender): figure.fbx, the Mannequin's body (fewer
  triangles, a texture embedded) with Mixamo's names of the role bones,
  written as Blender writes (the root nodes scaled 100, the unit 1 cm);
  walk.fbx, its skeleton of the role bones only, its walk going forward
  (not in place), in centimeters as Mixamo writes; plane.fbx, a square on
  a moved node, its texture (Holes.png) beside it; plane_zup.fbx, the same
  written z up.
*/

namespace {

QString
data( const char * name )
{
    return QStringLiteral( TEST_SOURCE_DIR "/test/data/fbx/" ) + QLatin1String( name );
}

//! a node's place at rest in the model
QVector3D
placeOf( const GltfModel & m,
         const QString & name )
{
    std::vector< QMatrix4x4 > global;
    m.globals( m.rest, global );
    const int i = m.nodeIndex( name );
    return ( i < 0 ? QVector3D( 1.0e6f, 1.0e6f, 1.0e6f ) : global[i].column( 3 ).toVector3D() );
}

//! the Mannequin's role bones and their names in the files (Mixamo's)
const char * const NAMES[][2] = {
    { "pelvis", "mixamorig:Hips" }, { "spine_02", "mixamorig:Spine1" }, { "Head", "mixamorig:Head" },
    { "upperarm_l", "mixamorig:LeftArm" }, { "hand_l", "mixamorig:LeftHand" }, { "hand_r", "mixamorig:RightHand" },
    { "thigh_l", "mixamorig:LeftUpLeg" }, { "calf_r", "mixamorig:RightLeg" }, { "foot_r", "mixamorig:RightFoot" },
    { "ball_l", "mixamorig:LeftToeBase" },
};

}

/*!
  \brief unit test of FbxModel
*/
class TestFbxModel
    : public QObject {
    Q_OBJECT

private slots:

    //! an FBX file told by its header or its name
    void isFbx()
      {
          QVERIFY( FbxModel::isFbx( "Kaydara FBX Binary  \0\x1a", QString() ) );
          QVERIFY( FbxModel::isFbx( "; FBX 7.4.0 project file", QString() ) );
          QVERIFY( FbxModel::isFbx( "anything", "Walk.FBX" ) );
          QVERIFY( ! FbxModel::isFbx( "glTF", "a.glb" ) );
          QVERIFY( ! FbxModel::isFbx( "{ \"asset\": {} }", "a.gltf" ) );
      }

    /*!
      the body in meters, y up, facing +z, its role bones where the
      Mannequin's are (the file of another unit and axes, and its nodes
      scaled); its joints, its materials, its texture embedded
    */
    void figure()
      {
          const GltfModel & glb = TestGltf::mannequin();
          GltfModel m;
          QString error;
          QVERIFY2( m.loadFile( data( "figure.fbx" ), &error ), qPrintable( error ) );
          QVERIFY( ! m.posed_rest ); // the bind pose of its skin
          QCOMPARE( int( m.rest.size() ), int( glb.rest.size() ) );
          QCOMPARE( m.jointCount(), glb.jointCount() );
          QVERIFY( m.vertices.size() > 300 && m.vertices.size() < glb.vertices.size() / 2 );
          QCOMPARE( m.uvs.size(), m.vertices.size() * 2 );
          QVERIFY( m.clips.empty() );
          // the triangles of each part ordered for the GPU's cache
          // (GltfModel::orderTriangles()): this body shares few vertices (made
          // with fewer triangles), so near the least, each vertex once (2.10 as read)
          for ( const SkinPrimitive & p : m.primitives )
          {
              const auto from = m.indices.begin() + p.first_index;
              const std::set< std::uint32_t > used( from, from + p.index_count );
              const double least = double( used.size() ) / ( p.index_count / 3 );
              const double misses = TestGltf::vertexCacheMisses( m.indices, std::size_t( p.first_index ), std::size_t( p.index_count ) );
              QVERIFY2( misses - least < 0.15, qPrintable( QString( "%1: %2 of %3" ).arg( p.material ).arg( misses ).arg( least ) ) );
          }
          QVERIFY2( std::fabs( m.height() - glb.height() ) < 0.01f, qPrintable( QString::number( m.height() ) ) );
          QVERIFY( std::fabs( m.min_y ) < 0.01f );
          for ( const auto & n : NAMES )
          {
              const QVector3D want = placeOf( glb, n[0] );
              const QVector3D got = placeOf( m, n[1] );
              QVERIFY2( ( got - want ).length() < 1.0e-3f,
                        qPrintable( QString( "%1: %2 %3 %4" ).arg( n[1] ).arg( got.x() ).arg( got.y() ).arg( got.z() ) ) );
          }
          // at rest, the skinned vertices where the Mannequin's are: within its bounds
          float lo[3] = { 1.0e6f, 1.0e6f, 1.0e6f }, hi[3] = { -1.0e6f, -1.0e6f, -1.0e6f };
          for ( const SkinVertex & v : glb.vertices )
          {
              for ( int k = 0; k < 3; ++k ) { lo[k] = std::min( lo[k], v.rest[k] ); hi[k] = std::max( hi[k], v.rest[k] ); }
          }
          for ( const SkinVertex & v : m.vertices )
          {
              for ( int k = 0; k < 3; ++k ) QVERIFY( v.rest[k] > lo[k] - 0.02f && v.rest[k] < hi[k] + 0.02f );
              QVERIFY( std::fabs( v.weights[0] + v.weights[1] + v.weights[2] + v.weights[3] - 1.0f ) < 1.0e-4f );
          }
          // facing +z: its roles found by Mixamo's names
          HumanBoneNodes nodes;
          HumanoidRig rig;
          QVERIFY2( findHumanBones( m, QString(), {}, &nodes, &error ), qPrintable( error ) );
          QVERIFY( rig.build( m, nodes ) );
          QVERIFY( std::fabs( rig.facingAngle() ) < 0.5f );
          // the materials: the body's texture embedded (one color; its own colors), the joints' color
          QCOMPARE( int( m.primitives.size() ), 2 );
          QCOMPARE( m.primitives[0].material, QString( "M_Main" ) );
          QCOMPARE( m.primitives[0].image, 0 );
          QCOMPARE( m.primitives[0].base_color, QVector4D( 1.0f, 1.0f, 1.0f, 1.0f ) );
          QCOMPARE( m.primitives[0].alpha, int( SkinPrimitive::Opaque ) );
          QCOMPARE( int( m.images.size() ), 1 );
          QCOMPARE( m.images[0].size(), QSize( 4, 4 ) );
          QCOMPARE( m.images[0].pixelColor( 0, 0 ), QColor( 204, 128, 51 ) );
          QCOMPARE( m.primitives[1].material, QString( "M_Joints" ) );
          QCOMPARE( m.primitives[1].image, -1 );
          const SkinPrimitive & joints = m.primitives[1];
          const SkinPrimitive & original = glb.primitives[glb.primitives[0].material == "M_Joints" ? 0 : 1];
          QVERIFY( ( joints.base_color - original.base_color ).length() < 0.02f );
      }

    /*!
      a file of a clip: a skeleton without a mesh (refused when a mesh is
      required), in meters, its rest as saved (a pose); its clip baked
      from 0; its rest turned as the body's (restAs()), the clip read on the
      body is the Mannequin's walk, once in place (its root motion taken
      off: the hips went forward by a stride)
    */
    void walk()
      {
          const GltfModel & glb = TestGltf::mannequin();
          GltfModel m;
          QString error;
          QVERIFY( ! m.loadFile( data( "walk.fbx" ), &error ) );
          QCOMPARE( error, QString( "no mesh" ) );
          QVERIFY2( m.loadFile( data( "walk.fbx" ), &error, false ), qPrintable( error ) );
          QVERIFY( m.posed_rest );
          QVERIFY( m.vertices.empty() && m.primitives.empty() );
          QCOMPARE( m.height(), 0.0f );
          QCOMPARE( int( m.rest.size() ), 24 ); // the armature's node, the root and the 22 role bones
          QCOMPARE( int( m.clips.size() ), 1 );
          QVERIFY2( std::fabs( m.clips[0].duration - 32.0f / 24.0f ) < 1.0e-3f, qPrintable( QString::number( m.clips[0].duration ) ) );
          const int hips = m.nodeIndex( "mixamorig:Hips" );
          QVERIFY( hips >= 0 );
          QVERIFY( ! m.clips[0].nodes[hips].t.empty() && ! m.clips[0].nodes[hips].r.empty() );
          QCOMPARE( m.clips[0].nodes[hips].r.times.front(), 0.0f );
          // in meters, not centimeters (the hips at the height of the Mannequin's walking)
          const float y = placeOf( m, "mixamorig:Hips" ).y();
          QVERIFY2( y > 0.8f && y < 1.0f, qPrintable( QString::number( y ) ) );

          // its rest as the body's (the same skeleton): every bone matched
          GltfModel body;
          QVERIFY( body.loadFile( data( "figure.fbx" ), &error ) );
          QCOMPARE( restAs( m, body ), 24 ); // the armature's node, the root, the role bones
          HumanBoneNodes nodes;
          HumanoidRig rig, body_rig, glb_rig;
          QVERIFY( findHumanBones( m, "mixamo", {}, &nodes, &error ) && rig.build( m, nodes ) );
          QVERIFY( findHumanBones( body, "mixamo", {}, &nodes, &error ) && body_rig.build( body, nodes ) );
          QVERIFY( glb_rig.build( glb, quaterniusBoneNames() ) );
          // not in place: about a stride (1.28 m) forward across the cycle
          const float drift = clipDrift( m, rig, m.clips[0] ) * rig.hipHeight();
          QVERIFY2( std::fabs( drift - 1.28f ) < 0.03f, qPrintable( QString::number( drift ) ) );
          const GltfModel::Clip put = retargetClip( m, rig, m.clips[0], body, body_rig, true );
          QVERIFY( clipDrift( body, body_rig, put ) < 1.0e-3f );
          const GltfModel::Clip & want = glb.clips[glb.clipIndex( "Walk_Loop" )];
          for ( float t = 0.0f; t < want.duration; t += 0.1f )
          {
              std::vector< NodeTransform > local = body.rest;
              body.sample( put, t, local );
              const HumanoidPose got = body_rig.poseOf( local );
              local = glb.rest;
              glb.sample( want, t, local );
              const HumanoidPose wanted = glb_rig.poseOf( local );
              for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
              {
                  const float dot = std::fabs( QQuaternion::dotProduct( got.rotations[b], wanted.rotations[b] ) );
                  QVERIFY2( dot > 0.9995f, qPrintable( QString( "%1 %2: %3" ).arg( t ).arg( humanBoneName( HumanBone( b ) ) ).arg( dot ) ) );
              }
              QVERIFY2( ( got.hips - wanted.hips ).length() < 0.005f, qPrintable( QString::number( t ) ) );
          }
      }

    /*!
      a mesh without a skin, fixed to its node (moved: its rest there), its
      texture a file beside it (found also in "<name>.fbm", not otherwise):
      with transparent pixels, a mask seen from both sides; the texture
      coordinates from the image's top left
    */
    void plane()
      {
          GltfModel m;
          QString error;
          QVERIFY2( m.loadFile( data( "plane.fbx" ), &error ), qPrintable( error ) );
          QCOMPARE( int( m.vertices.size() ), 4 );
          QCOMPARE( int( m.indices.size() ), 6 );
          QCOMPARE( m.jointCount(), 1 );
          QCOMPARE( m.node_names[m.joint_nodes[0]], QString( "Square" ) );
          QVERIFY( m.inverse_bind[0].isIdentity() );
          for ( const SkinVertex & v : m.vertices )
          {
              QVERIFY( std::fabs( std::fabs( v.rest[0] - 1.0f ) - 0.5f ) < 1.0e-4f ); // x 0.5 or 1.5
              QVERIFY( std::fabs( v.rest[1] ) < 1.0e-4f );
              QVERIFY( std::fabs( std::fabs( v.rest[2] - 2.0f ) - 0.5f ) < 1.0e-4f ); // z 1.5 or 2.5
          }
          // the left (-x) of the texture's left, the far (-z: the image's top) at v 0
          for ( std::size_t k = 0; k < m.vertices.size(); ++k )
          {
              QCOMPARE( m.uvs[k * 2] < 0.5f, m.vertices[k].rest[0] < 1.0f );
              QCOMPARE( m.uvs[k * 2 + 1] < 0.5f, m.vertices[k].rest[2] < 2.0f );
          }
          QCOMPARE( int( m.primitives.size() ), 1 );
          const SkinPrimitive & p = m.primitives[0];
          QCOMPARE( p.material, QString( "Holes" ) );
          QCOMPARE( p.image, 0 );
          QCOMPARE( p.alpha, int( SkinPrimitive::Mask ) );
          QVERIFY( p.double_sided );
          QCOMPARE( m.images[0].pixelColor( 0, 0 ).alpha(), 255 );
          QCOMPARE( m.images[0].pixelColor( 3, 0 ).alpha(), 0 );

          // written z up: converted to y up, the same
          GltfModel z;
          QVERIFY2( z.loadFile( data( "plane_zup.fbx" ), &error ), qPrintable( error ) );
          QCOMPARE( z.vertices.size(), m.vertices.size() );
          for ( std::size_t k = 0; k < m.vertices.size(); ++k )
          {
              bool found = false;
              for ( const SkinVertex & v : z.vertices )
              {
                  found = found || ( QVector3D( v.rest[0], v.rest[1], v.rest[2] )
                                     - QVector3D( m.vertices[k].rest[0], m.vertices[k].rest[1], m.vertices[k].rest[2] ) ).length() < 1.0e-4f;
              }
              QVERIFY( found );
          }

          // the texture elsewhere: in "<name>.fbm" found; not at all: no texture, opaque
          QTemporaryDir dir;
          QVERIFY( QFile::copy( data( "plane.fbx" ), dir.filePath( "plane.fbx" ) ) );
          QVERIFY( m.loadFile( dir.filePath( "plane.fbx" ), &error ) );
          QCOMPARE( m.primitives[0].image, -1 );
          QCOMPARE( m.primitives[0].alpha, int( SkinPrimitive::Opaque ) );
          QVERIFY( QDir( dir.path() ).mkpath( "plane.fbm" ) );
          QVERIFY( QFile::copy( data( "Holes.png" ), dir.filePath( "plane.fbm/Holes.png" ) ) );
          QVERIFY( m.loadFile( dir.filePath( "plane.fbx" ), &error ) );
          QCOMPARE( m.primitives[0].image, 0 );
          // read from memory: no file beside it
          QFile file( data( "plane.fbx" ) );
          QVERIFY( file.open( QIODevice::ReadOnly ) );
          QVERIFY( m.load( file.readAll(), &error, QString() ) );
          QCOMPARE( m.primitives[0].image, -1 );
      }

    //! not an FBX file: why
    void broken()
      {
          GltfModel m;
          QString error;
          QVERIFY( ! m.load( "Kaydara FBX Binary  \0broken", &error ) );
          QVERIFY2( error.startsWith( "not an FBX file (" ), qPrintable( error ) );
          QTemporaryDir dir;
          QVERIFY( TestGltf::File::write( dir.filePath( "text.fbx" ), "not an FBX file at all" ) );
          QVERIFY( ! m.loadFile( dir.filePath( "text.fbx" ), &error ) );
          QVERIFY2( error.startsWith( "not an FBX file (" ), qPrintable( error ) );
          // the file read cleared: nothing left of the one before
          QVERIFY( m.loadFile( data( "plane.fbx" ), &error ) );
          QVERIFY( ! m.loadFile( dir.filePath( "text.fbx" ), &error ) );
          QVERIFY( m.vertices.empty() && m.rest.empty() && m.images.empty() );
      }
};

QTEST_MAIN( TestFbxModel )
#include "test_fbx_model.moc"
