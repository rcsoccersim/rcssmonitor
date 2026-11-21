// -*-c++-*-

/*!
  \file test_gltf_model.cpp
  \brief unit test of GltfModel (reading a skinned glTF model)
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

#include "gltf_model.h"
#include "test_gltf.h"

#include <QTemporaryDir>

#include <algorithm>
#include <array>
#include <cmath>
#include <random>

namespace {

//! the bundled model, read from the source tree
QByteArray
mannequin()
{
    QFile file( QStringLiteral( TEST_SOURCE_DIR "/resources/figures/mannequin/Mannequin.glb" ) );
    if ( ! file.open( QIODevice::ReadOnly ) ) return QByteArray();
    return file.readAll();
}

bool
near( const float a,
      const float b,
      const float tolerance = 1.0e-5f )
{
    return std::fabs( a - b ) <= tolerance;
}

//! an image of two colors side by side, left and right
QImage
twoColors( const QColor & left,
           const QColor & right,
           const int width = 8,
           const int height = 4 )
{
    QImage image( width, height, QImage::Format_RGBA8888 );
    for ( int y = 0; y < height; ++y )
    {
        for ( int x = 0; x < width; ++x ) image.setPixelColor( x, y, x < width / 2 ? left : right );
    }
    return image;
}

QVector3D
restOf( const SkinVertex & v )
{
    return QVector3D( v.rest[0], v.rest[1], v.rest[2] );
}

}

/*!
  \brief unit test of GltfModel
*/
class TestGltfModel
    : public QObject {
    Q_OBJECT

private:
    GltfModel M_model;

private slots:

    void initTestCase()
      {
          const QByteArray data = mannequin();
          QVERIFY( ! data.isEmpty() );
          QString error;
          QVERIFY2( M_model.load( data, &error ), qPrintable( error ) );
      }

    //! the skeleton, the skin, the mesh and the clips of the Mannequin
    void contents()
      {
          const GltfModel & m = M_model;
          QCOMPARE( m.jointCount(), 65 );
          QCOMPARE( m.inverse_bind.size(), size_t( 65 ) );
          QVERIFY( m.nodeIndex( "pelvis" ) >= 0 );
          QVERIFY( m.nodeIndex( "Head" ) >= 0 );
          QCOMPARE( m.nodeIndex( "no such bone" ), -1 );

          // the parents come before their children
          std::vector< bool > seen( m.node_names.size(), false );
          for ( const int i : m.order )
          {
              QVERIFY( m.parent[i] < 0 || seen[m.parent[i]] );
              seen[i] = true;
          }
          QCOMPARE( m.order.size(), m.node_names.size() );
          QCOMPARE( m.node_names[m.parent[m.nodeIndex( "calf_l" )]], QString( "thigh_l" ) );

          // two materials: the body and the joints, in their own colors
          QCOMPARE( m.primitives.size(), size_t( 2 ) );
          QCOMPARE( m.primitives[0].material, QString( "M_Main" ) );
          QCOMPARE( m.primitives[1].material, QString( "M_Joints" ) );
          QVERIFY( m.primitives[0].base_color.x() > m.primitives[0].base_color.z() ); // orange
          QVERIFY( m.primitives[1].base_color.z() > m.primitives[1].base_color.y() ); // purple

          // triangles of the vertices, each moved by joints of the skin, by weights summing to 1
          QCOMPARE( m.vertices.size(), size_t( 3389 + 5157 ) );
          QCOMPARE( m.indices.size() % 3, size_t( 0 ) );
          QCOMPARE( size_t( m.primitives[1].first_index + m.primitives[1].index_count ), m.indices.size() );
          for ( const std::uint32_t i : m.indices ) QVERIFY( i < m.vertices.size() );
          for ( const SkinVertex & v : m.vertices )
          {
              float sum = 0.0f;
              for ( int k = 0; k < 4; ++k )
              {
                  QVERIFY( v.joints[k] < 65 );
                  sum += v.weights[k];
              }
              QVERIFY( near( sum, 1.0f, 1.0e-4f ) );
          }

          // a person's height at rest, the feet near the ground (quantized
          // vertices: the size comes from the skin, not from the vertices)
          QVERIFY2( m.height() > 1.6f && m.height() < 2.0f, qPrintable( QString::number( m.height() ) ) );
          QVERIFY2( std::fabs( m.min_y ) < 0.1f, qPrintable( QString::number( m.min_y ) ) );

          // the four clips
          QCOMPARE( m.clips.size(), size_t( 4 ) );
          for ( const char * name : { "Idle_Loop", "Walk_Loop", "Jog_Fwd_Loop", "Sprint_Loop" } )
          {
              const int c = m.clipIndex( name );
              QVERIFY2( c >= 0, name );
              QVERIFY( m.clips[c].duration > 0.1f );
              QCOMPARE( m.clips[c].nodes.size(), m.node_names.size() );
          }
          QCOMPARE( m.clipIndex( "Dance_Loop" ), -1 );
      }

    //! a clip at a time: the keys, between them, and looped by the duration
    void sample()
      {
          const GltfModel & m = M_model;
          const GltfModel::Clip & walk = m.clips[m.clipIndex( "Walk_Loop" )];
          const int thigh = m.nodeIndex( "thigh_l" );
          const GltfModel::Channel & r = walk.nodes[thigh].r;
          QVERIFY( r.times.size() > 2 );

          std::vector< NodeTransform > local = m.rest;
          // at the first key
          m.sample( walk, r.times[0], local );
          QVERIFY( near( local[thigh].r.x(), r.values[0] ) );
          QVERIFY( near( local[thigh].r.scalar(), r.values[3] ) );
          // at the second
          m.sample( walk, r.times[1], local );
          QVERIFY( near( local[thigh].r.y(), r.values[5] ) );
          // halfway: between them, of length 1
          m.sample( walk, ( r.times[1] + r.times[2] ) / 2.0f, local );
          const QQuaternion a( r.values[7], r.values[4], r.values[5], r.values[6] );
          const QQuaternion b( r.values[11], r.values[8], r.values[9], r.values[10] );
          QVERIFY( near( local[thigh].r.length(), 1.0f ) );
          QVERIFY( QQuaternion::dotProduct( local[thigh].r, a ) > 0.9f );
          QVERIFY( QQuaternion::dotProduct( local[thigh].r, b ) > 0.9f );

          // looped: a duration later (or before) is the same
          std::vector< NodeTransform > later = m.rest;
          m.sample( walk, 0.3f, local );
          m.sample( walk, 0.3f + walk.duration, later );
          QVERIFY( near( local[thigh].r.x(), later[thigh].r.x(), 1.0e-4f ) );
          m.sample( walk, 0.3f - 2.0f * walk.duration, later );
          QVERIFY( near( local[thigh].r.x(), later[thigh].r.x(), 1.0e-4f ) );
      }

    /*!
      keys made by hand: a rotation the shorter way to a key written with
      the other sign (the same rotation), and a step channel held until the
      next key
    */
    void handMadeKeys()
      {
          GltfModel m;
          m.rest.resize( 1 );
          m.parent.assign( 1, -1 );
          m.order.assign( 1, 0 );
          GltfModel::Clip clip;
          clip.duration = 1.0f;
          clip.nodes.resize( 1 );
          GltfModel::Channel & r = clip.nodes[0].r;
          r.times = { 0.0f, 1.0f };
          // from no turn to 10 degrees about x, written negated
          const float sh = std::sin( 5.0f * float( M_PI ) / 180.0f ), ch = std::cos( 5.0f * float( M_PI ) / 180.0f ); // half the angle
          r.values = { 0.0f, 0.0f, 0.0f, 1.0f, -sh, 0.0f, 0.0f, -ch };
          std::vector< NodeTransform > local = m.rest;
          m.sample( clip, 0.5f, local );
          // 5 degrees about x, not a turn of nearly 180
          const QQuaternion half = QQuaternion::fromAxisAndAngle( 1.0f, 0.0f, 0.0f, 5.0f );
          QVERIFY2( std::fabs( QQuaternion::dotProduct( local[0].r, half ) ) > 0.9999f,
                    qPrintable( QString::number( local[0].r.x() ) + " " + QString::number( local[0].r.scalar() ) ) );

          // a step channel: the first key until the second
          GltfModel::Channel & t = clip.nodes[0].t;
          t.times = { 0.0f, 0.6f };
          t.values = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
          t.step = true;
          m.sample( clip, 0.5f, local );
          QCOMPARE( local[0].t, QVector3D( 1.0f, 2.0f, 3.0f ) );
          // linear: between them
          t.step = false;
          m.sample( clip, 0.3f, local );
          QVERIFY( ( local[0].t - QVector3D( 2.5f, 3.5f, 4.5f ) ).length() < 1.0e-5f );
          // after the last key: the last
          m.sample( clip, 0.8f, local );
          QCOMPARE( local[0].t, QVector3D( 4.0f, 5.0f, 6.0f ) );
      }

    //! the transforms through the parents, and the skin matrices
    void skin()
      {
          const GltfModel & m = M_model;
          std::vector< QMatrix4x4 > global;
          m.globals( m.rest, global );
          const int pelvis = m.nodeIndex( "pelvis" );
          const int calf = m.nodeIndex( "calf_l" );
          // the knee below the hips, both above the ground
          const float hips_y = global[pelvis].column( 3 ).y();
          const float knee_y = global[calf].column( 3 ).y();
          QVERIFY( hips_y > knee_y );
          QVERIFY( knee_y > m.min_y );

          // a skin matrix of the rest moved by a model matrix moves its vertices as much
          std::vector< float > rest( 65 * 16 ), moved( 65 * 16 );
          m.skin( m.rest, QMatrix4x4(), rest.data() );
          QMatrix4x4 model;
          model.translate( 3.0f, 0.0f, -2.0f );
          m.skin( m.rest, model, moved.data() );
          const QMatrix4x4 a( rest.data() ), b( moved.data() ); // read column by column (transposed)
          const QVector3D p( 0.1f, 0.2f, 0.3f );
          const QVector3D pa = a.transposed().map( p );
          const QVector3D pb = b.transposed().map( p );
          QVERIFY( ( pb - pa - QVector3D( 3.0f, 0.0f, -2.0f ) ).length() < 1.0e-4f );
      }

    //! what is not a glTF model is not read
    void broken()
      {
          GltfModel m;
          QString error;
          QVERIFY( ! m.load( QByteArray( "not a model" ), &error ) );
          QVERIFY( ! error.isEmpty() );
          // cut off in the middle
          const QByteArray data = mannequin();
          error.clear();
          QVERIFY( ! m.load( data.left( data.size() / 2 ), &error ) );
          QVERIFY( ! error.isEmpty() );
          // no file
          QVERIFY( ! m.loadFile( "/nonexistent/model.glb", &error ) );
          QCOMPARE( error, QString( "cannot open model.glb" ) );
      }

    /*!
      a texture inside a .glb: the image (its mean luminance where it is
      not transparent), the material's texture and color, the texture
      coordinates of the vertices; the vertices where the joints put them
    */
    void texture()
      {
          TestGltf::HumanoidOptions o;
          o.texture = twoColors( QColor( 255, 0, 0 ), QColor( 0, 0, 255 ) );
          const TestGltf::File f = TestGltf::humanoid( TestGltf::mannequin(), o );
          GltfModel m;
          QString error;
          QVERIFY2( m.load( f.glb(), &error ), qPrintable( error ) );
          QCOMPARE( int( m.images.size() ), 1 );
          QCOMPARE( m.images[0].size(), QSize( 8, 4 ) );
          QCOMPARE( m.images[0].pixelColor( 0, 0 ), QColor( 255, 0, 0 ) );
          QCOMPARE( m.images[0].pixelColor( 7, 3 ), QColor( 0, 0, 255 ) );
          QVERIFY( near( m.image_luminance[0], ( 0.299f + 0.114f ) * 0.5f, 0.01f ) );
          QCOMPARE( int( m.primitives.size() ), 1 );
          QCOMPARE( m.primitives[0].material, QString( "Body" ) );
          QCOMPARE( m.primitives[0].image, 0 );
          QCOMPARE( m.primitives[0].alpha, int( SkinPrimitive::Opaque ) );
          QVERIFY( ! m.primitives[0].double_sided );
          // the faces' corners at the image's corners
          QCOMPARE( m.uvs.size(), m.vertices.size() * 2 );
          QCOMPARE( m.uvs[0 * 2 + 0], 0.0f );
          QCOMPARE( m.uvs[1 * 2 + 0], 1.0f );
          QCOMPARE( m.uvs[2 * 2 + 1], 1.0f );
          // the joints at the Mannequin's
          QCOMPARE( m.jointCount(), 22 );
          QVERIFY( std::fabs( m.height() - TestGltf::mannequin().height() ) < 0.2f );
          // an image that is not one: no texture
          TestGltf::File bad = f;
          bad.images[0] = QImage();
          QVERIFY( m.load( bad.glb() ) );
          QCOMPARE( m.primitives[0].image, -1 );
      }

    /*!
      a .gltf: its buffer and its images in files beside it, or in data
      URIs; a missing buffer's file
    */
    void gltfFiles()
      {
          QTemporaryDir dir;
          TestGltf::HumanoidOptions o;
          o.texture = twoColors( QColor( 0, 255, 0 ), QColor( 255, 255, 0 ) );
          const TestGltf::File f = TestGltf::humanoid( TestGltf::mannequin(), o );
          QVERIFY( f.writeGltf( dir.path(), "body" ) );
          QVERIFY( QFile::exists( dir.filePath( "body.bin" ) ) );
          QVERIFY( QFile::exists( dir.filePath( "body_0.png" ) ) );
          GltfModel files, uris;
          QString error;
          QVERIFY2( files.loadFile( dir.filePath( "body.gltf" ), &error ), qPrintable( error ) );
          QCOMPARE( int( files.images.size() ), 1 );
          QCOMPARE( files.images[0].pixelColor( 7, 0 ), QColor( 255, 255, 0 ) );
          QCOMPARE( files.primitives[0].image, 0 );
          QVERIFY( f.writeGltf( dir.path(), "inline", true ) );
          QVERIFY( ! QFile::exists( dir.filePath( "inline.bin" ) ) );
          QVERIFY2( uris.loadFile( dir.filePath( "inline.gltf" ), &error ), qPrintable( error ) );
          QCOMPARE( uris.images[0].pixelColor( 0, 0 ), QColor( 0, 255, 0 ) );
          QCOMPARE( uris.vertices.size(), files.vertices.size() );
          // the same as the .glb
          GltfModel glb;
          QVERIFY( glb.load( f.glb() ) );
          QCOMPARE( files.vertices.size(), glb.vertices.size() );
          QCOMPARE( std::memcmp( files.vertices.data(), glb.vertices.data(), glb.vertices.size() * sizeof( SkinVertex ) ), 0 );
          // the image's file missing: no texture; the buffer's file missing: not read
          QVERIFY( QFile::remove( dir.filePath( "body_0.png" ) ) );
          QVERIFY( files.loadFile( dir.filePath( "body.gltf" ), &error ) );
          QCOMPARE( files.primitives[0].image, -1 );
          QVERIFY( QFile::remove( dir.filePath( "body.bin" ) ) );
          QVERIFY( ! files.loadFile( dir.filePath( "body.gltf" ), &error ) );
          QCOMPARE( error, QString( "a buffer's file is not found" ) );
          // from the bytes alone, the files beside it are not found
          QFile inline_file( dir.filePath( "inline.gltf" ) );
          QVERIFY( inline_file.open( QIODevice::ReadOnly ) );
          QVERIFY( uris.load( inline_file.readAll() ) ); // all inside
      }

    //! the materials' alpha (opaque, a mask with its cutoff, a blend) and their sides
    void materials()
      {
          TestGltf::File f = TestGltf::humanoid( TestGltf::mannequin(), TestGltf::HumanoidOptions() );
          GltfModel m;
          f.materials[0].alpha = "MASK";
          f.materials[0].cutoff = 0.3f;
          f.materials[0].double_sided = true;
          QVERIFY( m.load( f.glb() ) );
          QCOMPARE( m.primitives[0].alpha, int( SkinPrimitive::Mask ) );
          QVERIFY( near( m.primitives[0].alpha_cutoff, 0.3f ) );
          QVERIFY( m.primitives[0].double_sided );
          f.materials[0].alpha = "BLEND";
          QVERIFY( m.load( f.glb() ) );
          QCOMPARE( m.primitives[0].alpha, int( SkinPrimitive::Blend ) );
          QCOMPARE( m.primitives[0].alpha_cutoff, 0.5f );
          f.materials[0].color = QVector4D( 0.2f, 0.4f, 0.6f, 1.0f );
          QVERIFY( m.load( f.glb() ) );
          QCOMPARE( m.primitives[0].base_color, QVector4D( 0.2f, 0.4f, 0.6f, 1.0f ) );
      }

    /*!
      the extensions of materials and textures: the diffuse of the older
      specular-glossiness model as the base color, and the texture
      coordinates moved by KHR_texture_transform (of the set it tells); a
      file that needs an extension not read is not read
    */
    void extensions()
      {
          QImage texture( 2, 2, QImage::Format_RGBA8888 );
          texture.fill( Qt::white );
          TestGltf::HumanoidOptions o;
          o.texture = texture;
          TestGltf::File f = TestGltf::humanoid( TestGltf::mannequin(), o );
          GltfModel m;
          QString error;
          // the diffuse, not the base color (mid gray) of the metallic-roughness model
          f.materials[0].color = QVector4D( 0.2f, 0.4f, 0.6f, 1.0f );
          f.materials[0].spec_gloss = true;
          QVERIFY2( m.load( f.glb(), &error ), qPrintable( error ) );
          QCOMPARE( m.primitives[0].base_color, QVector4D( 0.2f, 0.4f, 0.6f, 1.0f ) );
          QCOMPARE( m.primitives[0].image, 0 );
          f.materials[0].spec_gloss = false;
          // scaled by (2, 3), turned a quarter, moved by (0.25, 0.5): u' = 3 v + 0.25, v' = -2 u + 0.5
          f.materials[0].transform = QJsonObject{ { "offset", QJsonArray{ 0.25, 0.5 } },
                                                  { "rotation", 3.14159265358979 / 2.0 },
                                                  { "scale", QJsonArray{ 2.0, 3.0 } } };
          QVERIFY2( m.load( f.glb(), &error ), qPrintable( error ) );
          const float written[4][2] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } }; // a face's corners
          for ( int k = 0; k < 4; ++k )
          {
              QVERIFY2( near( m.uvs[k * 2], 3.0f * written[k][1] + 0.25f, 1.0e-5f )
                        && near( m.uvs[k * 2 + 1], -2.0f * written[k][0] + 0.5f, 1.0e-5f ),
                        qPrintable( QString( "%1: %2 %3" ).arg( k ).arg( m.uvs[k * 2] ).arg( m.uvs[k * 2 + 1] ) ) );
          }
          // of another set (none in the file: all at (0, 0), moved)
          f.materials[0].transform.insert( "texCoord", 1 );
          QVERIFY2( m.load( f.glb(), &error ), qPrintable( error ) );
          for ( int k = 0; k < 4; ++k )
          {
              QVERIFY( near( m.uvs[k * 2], 0.25f, 1.0e-5f ) && near( m.uvs[k * 2 + 1], 0.5f, 1.0e-5f ) );
          }
          // needed: read or passed over; not read (a compression of the meshes)
          f.materials[0].transform = QJsonObject();
          f.required = QStringList{ "KHR_mesh_quantization", "KHR_materials_unlit", "KHR_texture_transform" };
          QVERIFY2( m.load( f.glb(), &error ), qPrintable( error ) );
          f.required = QStringList{ "KHR_mesh_quantization", "KHR_draco_mesh_compression" };
          QVERIFY( ! m.load( f.glb(), &error ) );
          QCOMPARE( error, QString( "it needs the extension KHR_draco_mesh_compression, which is not read" ) );
      }

    /*!
      the skins of a file joined: two meshes of two skins, a joint of the
      same node and matrix once, of another matrix twice; the vertices at
      rest where their skins put them
    */
    void skins()
      {
          TestGltf::File f = TestGltf::humanoid( TestGltf::mannequin(), TestGltf::HumanoidOptions() );
          // a second skin: the first joints in another order, one with another matrix
          TestGltf::Skin two;
          two.joints = { f.skins[0].joints[2], f.skins[0].joints[0], f.skins[0].joints[1] };
          two.inverse_bind = { f.skins[0].inverse_bind[2], f.skins[0].inverse_bind[0], f.skins[0].inverse_bind[1] };
          QMatrix4x4 moved = two.inverse_bind[2];
          moved.translate( 0.0f, 0.1f, 0.0f ); // the vertices 0.1 m lower in the joint's frame at bind
          two.inverse_bind[2] = moved;
          f.skins.push_back( two );
          f.nodes.push_back( TestGltf::Node{ "Body2", 0, QVector3D(), QQuaternion() } );
          TestGltf::Mesh mesh;
          mesh.node = int( f.nodes.size() ) - 1;
          mesh.skin = 1;
          mesh.material = 0;
          for ( int k = 0; k < 3; ++k )
          {
              mesh.positions.push_back( QVector3D( 0.0f, 1.0f, 0.0f ) );
              mesh.normals.push_back( QVector3D( 0.0f, 0.0f, 1.0f ) );
              mesh.joints.push_back( { k, 0, 0, 0 } );
              mesh.weights.push_back( QVector4D( 1.0f, 0.0f, 0.0f, 0.0f ) );
              mesh.indices.push_back( std::uint32_t( k ) );
          }
          f.meshes.push_back( mesh );
          GltfModel m;
          QString error;
          QVERIFY2( m.load( f.glb(), &error ), qPrintable( error ) );
          QCOMPARE( m.jointCount(), 23 ); // the second skin's third joint has another matrix
          QCOMPARE( m.uvs.size(), m.vertices.size() * 2 ); // a pair for each vertex, of any mesh (none here: 0)
          const std::size_t first = m.vertices.size() - 3;
          QCOMPARE( int( m.vertices[first].joints[0] ), 2 );
          QCOMPARE( int( m.vertices[first + 1].joints[0] ), 0 );
          QCOMPARE( int( m.vertices[first + 2].joints[0] ), 22 );
          QCOMPARE( m.joint_nodes[22], f.skins[0].joints[1] );
          // at rest: the joint's matrix times its inverse bind, times the vertex
          std::vector< QMatrix4x4 > global;
          m.globals( m.rest, global );
          for ( int k = 0; k < 3; ++k )
          {
              const int node = two.joints[k];
              const QVector3D want = ( global[node] * two.inverse_bind[k] ).map( QVector3D( 0.0f, 1.0f, 0.0f ) );
              QVERIFY( ( restOf( m.vertices[first + k] ) - want ).length() < 1.0e-4f );
          }
          QCOMPARE( int( m.primitives.size() ), 2 );
      }

    //! a mesh fixed to a node, without a skin: it moves with the node
    void fixedMesh()
      {
          TestGltf::File f = TestGltf::humanoid( TestGltf::mannequin(), TestGltf::HumanoidOptions() );
          const int head = f.skins[0].joints[5]; // the head
          QCOMPARE( f.nodes[head].name, QString( "Head" ) );
          TestGltf::Mesh hat;
          hat.node = head;
          hat.material = 0;
          hat.positions = { QVector3D( 0.0f, 0.2f, 0.0f ), QVector3D( 0.1f, 0.2f, 0.0f ), QVector3D( 0.0f, 0.2f, 0.1f ) };
          hat.normals = { QVector3D( 0.0f, 1.0f, 0.0f ), QVector3D( 0.0f, 1.0f, 0.0f ), QVector3D( 0.0f, 1.0f, 0.0f ) };
          hat.indices = { 0, 2, 1 };
          f.meshes.push_back( hat );
          GltfModel m;
          QString error;
          QVERIFY2( m.load( f.glb(), &error ), qPrintable( error ) );
          QCOMPARE( m.jointCount(), 23 ); // the head again, without a matrix
          std::vector< QMatrix4x4 > global;
          m.globals( m.rest, global );
          const SkinVertex & v = m.vertices[m.vertices.size() - 3];
          QCOMPARE( int( v.joints[0] ), 22 );
          QCOMPARE( v.weights[0], 1.0f );
          QVERIFY( ( restOf( v ) - global[head].map( QVector3D( 0.0f, 0.2f, 0.0f ) ) ).length() < 1.0e-5f );
          QVERIFY( m.max_y >= global[head].map( QVector3D( 0.0f, 0.2f, 0.0f ) ).y() - 1.0e-5f );
          // a pose of the head moves it
          std::vector< NodeTransform > posed = m.rest;
          posed[head].r = QQuaternion::fromAxisAndAngle( 1.0f, 0.0f, 0.0f, 90.0f ) * posed[head].r;
          std::vector< float > joints( m.jointCount() * 16 );
          m.skin( posed, QMatrix4x4(), joints.data() );
          QMatrix4x4 j22( joints.data() + 22 * 16 );
          m.globals( posed, global );
          QVERIFY( ( j22.transposed().map( QVector3D( 0.0f, 0.2f, 0.0f ) ) - global[head].map( QVector3D( 0.0f, 0.2f, 0.0f ) ) ).length() < 1.0e-5f );
      }

    //! without normals: smooth ones of the triangles, out of the boxes
    void normals()
      {
          TestGltf::HumanoidOptions o;
          o.normals = false;
          GltfModel with, without;
          QVERIFY( with.load( TestGltf::humanoid( TestGltf::mannequin(), TestGltf::HumanoidOptions() ).glb() ) );
          QVERIFY( without.load( TestGltf::humanoid( TestGltf::mannequin(), o ).glb() ) );
          QCOMPARE( without.vertices.size(), with.vertices.size() );
          for ( std::size_t i = 0; i < with.vertices.size(); ++i )
          {
              const QVector3D a( with.vertices[i].normal[0], with.vertices[i].normal[1], with.vertices[i].normal[2] );
              const QVector3D b( without.vertices[i].normal[0], without.vertices[i].normal[1], without.vertices[i].normal[2] );
              QVERIFY( near( b.length(), 1.0f, 1.0e-4f ) );
              QVERIFY2( QVector3D::dotProduct( a, b ) > 0.99f, qPrintable( QString::number( i ) ) ); // the faces' own (each face its own vertices)
          }
      }

    //! VRM's humanoid bones: 1.0 and 0.x
    void vrm()
      {
          for ( const int version : { 0, 1 } )
          {
              TestGltf::HumanoidOptions o;
              o.vrm = version;
              o.preset = "vroid";
              const TestGltf::File f = TestGltf::humanoid( TestGltf::mannequin(), o );
              GltfModel m;
              QString error;
              QVERIFY2( m.load( f.glb(), &error ), qPrintable( error ) );
              QCOMPARE( m.vrm_version, version );
              QCOMPARE( int( m.humanoid_bones.size() ), 22 );
              bool hips = false;
              for ( const auto & b : m.humanoid_bones )
              {
                  if ( b.first == "hips" )
                  {
                      hips = true;
                      QCOMPARE( m.node_names[b.second], QString( "J_Bip_C_Hips" ) );
                  }
              }
              QVERIFY( hips );
          }
          GltfModel plain;
          QVERIFY( plain.load( TestGltf::humanoid( TestGltf::mannequin(), TestGltf::HumanoidOptions() ).glb() ) );
          QCOMPARE( plain.vrm_version, -1 );
          QVERIFY( plain.humanoid_bones.empty() );
      }

    /*!
      a VRM Animation (.vrma): its humanoid bones (as VRM 1.0 writes them),
      not a VRM; no mesh, its clip; read though it says it needs the extension
    */
    void vrma()
      {
          const GltfModel & glb = TestGltf::mannequin();
          HumanoidRig rig;
          QVERIFY( rig.build( glb, quaterniusBoneNames() ) );
          TestGltf::File f = TestGltf::animationOf( glb, rig, "Walk_Loop" );
          f.required << "VRMC_vrm_animation";
          GltfModel m;
          QString error;
          QVERIFY( ! m.load( f.glb(), &error ) );
          QCOMPARE( error, QString( "no mesh" ) );
          QVERIFY2( m.load( f.glb(), &error, "walk.vrma", false ), qPrintable( error ) );
          QCOMPARE( m.vrm_version, -1 );
          QCOMPARE( int( m.humanoid_bones.size() ), 22 );
          for ( const auto & b : m.humanoid_bones )
          {
              QCOMPARE( b.second, rig.node( HumanBone( humanBoneOf( b.first ) ) ) );
          }
          QCOMPARE( int( m.clips.size() ), 1 );
          QCOMPARE( m.clips[0].name, QString( "Walk_Loop" ) );
          // its roles found by them (the bones' names are of no skeleton)
          HumanBoneNodes nodes;
          QVERIFY2( findHumanBones( m, QString(), {}, &nodes, &error ), qPrintable( error ) );
          QCOMPARE( m.node_names[nodes[int( HumanBone::Hips )]], QString( "bone%1" ).arg( glb.nodeIndex( "pelvis" ) ) );
          QVERIFY( findHumanBones( m, "vrm", {}, &nodes, &error ) );
      }

    /*!
      the model turned about its origin: a root above the roots, its
      vertices at rest turned, the bones and the clips kept
    */
    void turnAbout()
      {
          GltfModel m = TestGltf::mannequin();
          const GltfModel & before = TestGltf::mannequin();
          m.turnAbout( QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, 180.0f ) );
          QCOMPARE( m.rest.size(), before.rest.size() + 1 );
          QCOMPARE( m.order.front(), int( before.rest.size() ) );
          QCOMPARE( m.parent.back(), -1 );
          for ( std::size_t i = 0; i < before.parent.size(); ++i )
          {
              QCOMPARE( m.parent[i], before.parent[i] < 0 ? int( before.rest.size() ) : before.parent[i] );
          }
          QCOMPARE( m.nodeIndex( "pelvis" ), before.nodeIndex( "pelvis" ) );
          QCOMPARE( m.clips.size(), before.clips.size() );
          QCOMPARE( m.clips[0].nodes.size(), m.rest.size() );
          QVERIFY( near( m.min_y, before.min_y, 1.0e-4f ) && near( m.max_y, before.max_y, 1.0e-4f ) );
          for ( std::size_t i = 0; i < m.vertices.size(); i += 97 )
          {
              const QVector3D a = restOf( before.vertices[i] );
              const QVector3D b = restOf( m.vertices[i] );
              QVERIFY( ( b - QVector3D( -a.x(), a.y(), -a.z() ) ).length() < 1.0e-4f );
          }
          // a clip sampled turns with it
          std::vector< NodeTransform > local = m.rest, old = before.rest;
          m.sample( m.clips[1], 0.3f, local );
          before.sample( before.clips[1], 0.3f, old );
          std::vector< QMatrix4x4 > g, h;
          m.globals( local, g );
          before.globals( old, h );
          const int hand = m.nodeIndex( "hand_l" );
          const QVector3D a = h[hand].column( 3 ).toVector3D();
          QVERIFY( ( g[hand].column( 3 ).toVector3D() - QVector3D( -a.x(), a.y(), -a.z() ) ).length() < 1.0e-4f );
      }

    //! an image larger than MAX_IMAGE_SIZE: made smaller, its shape kept
    void largeImage()
      {
          TestGltf::HumanoidOptions o;
          o.texture = twoColors( Qt::red, Qt::blue, 2048, 1024 );
          GltfModel m;
          QVERIFY( m.load( TestGltf::humanoid( TestGltf::mannequin(), o ).glb() ) );
          QCOMPARE( m.images[0].size(), QSize( GltfModel::MAX_IMAGE_SIZE, GltfModel::MAX_IMAGE_SIZE / 2 ) );
      }

    /*!
      the triangles ordered for the GPU's cache of transformed vertices: a
      grid's triangles shuffled come back close together (far fewer
      vertices transformed again), the same triangles with their winding;
      too few to order as they are; and the models read are ordered, part by
      part (the bundled human body was not: 1.45 vertices a triangle)
    */
    void triangleOrder()
      {
          // a grid of 40 x 40 squares, two triangles each, shuffled and each turned round
          const int n = 40;
          std::vector< std::uint32_t > grid;
          for ( int y = 0; y < n; ++y )
          {
              for ( int x = 0; x < n; ++x )
              {
                  const std::uint32_t a = y * ( n + 1 ) + x, b = a + 1, c = a + n + 1, d = c + 1;
                  grid.insert( grid.end(), { a, b, d, a, d, c } );
              }
          }
          const std::size_t triangles = grid.size() / 3;
          std::mt19937 rng( 3 );
          std::vector< std::size_t > order( triangles );
          for ( std::size_t t = 0; t < triangles; ++t ) order[t] = t;
          std::shuffle( order.begin(), order.end(), rng );
          std::vector< std::uint32_t > shuffled;
          for ( const std::size_t t : order )
          {
              const int r = int( rng() % 3 );
              for ( int c = 0; c < 3; ++c ) shuffled.push_back( grid[t * 3 + ( c + r ) % 3] );
          }
          // a triangle as it is wound, from its least vertex
          const auto wound = []( const std::vector< std::uint32_t > & idx ) {
              std::vector< std::array< std::uint32_t, 3 > > all;
              for ( std::size_t t = 0; t < idx.size() / 3; ++t )
              {
                  std::array< std::uint32_t, 3 > tri = { idx[t * 3], idx[t * 3 + 1], idx[t * 3 + 2] };
                  while ( tri[0] > tri[1] || tri[0] > tri[2] ) tri = { tri[1], tri[2], tri[0] };
                  all.push_back( tri );
              }
              std::sort( all.begin(), all.end() );
              return all;
          };
          const double before = TestGltf::vertexCacheMisses( shuffled, 0, shuffled.size() );
          std::vector< std::uint32_t > ordered = shuffled;
          GltfModel::orderTriangles( ordered.data(), ordered.size(), std::size_t( ( n + 1 ) * ( n + 1 ) ) );
          const double after = TestGltf::vertexCacheMisses( ordered, 0, ordered.size() );
          QVERIFY2( before > 2.0 && after < 0.75, qPrintable( QString( "%1 -> %2" ).arg( before ).arg( after ) ) );
          QVERIFY( wound( ordered ) == wound( shuffled ) );

          // too few, and a triangle of a vertex twice: as they are
          std::vector< std::uint32_t > one = { 2, 0, 1 };
          GltfModel::orderTriangles( one.data(), one.size(), 3 );
          QVERIFY( ( one == std::vector< std::uint32_t >{ 2, 0, 1 } ) );
          std::vector< std::uint32_t > odd = { 0, 0, 1, 1, 2, 3, 3, 2, 1 };
          GltfModel::orderTriangles( odd.data(), odd.size(), 4 );
          QVERIFY( wound( odd ) == wound( std::vector< std::uint32_t >{ 0, 0, 1, 1, 2, 3, 3, 2, 1 } ) );

          // the models read: each part ordered; the human body in all, 0.708
          // (0.739 without going back to the vertices put out lately)
          GltfModel human;
          QVERIFY( human.loadFile( QStringLiteral( TEST_SOURCE_DIR "/resources/figures/human/BaseCharacterMale.glb" ) ) );
          for ( const GltfModel * m : { &human, &M_model } )
          {
              double all = 0.0;
              for ( const SkinPrimitive & p : m->primitives )
              {
                  const double misses = TestGltf::vertexCacheMisses( m->indices, std::size_t( p.first_index ), std::size_t( p.index_count ) );
                  QVERIFY2( misses < 0.9, qPrintable( QString( "%1: %2" ).arg( p.material ).arg( misses ) ) );
                  all += misses * ( p.index_count / 3 );
              }
              if ( m == &human )
              {
                  all /= double( human.indices.size() / 3 );
                  QVERIFY2( all < 0.72, qPrintable( QString::number( all ) ) );
              }
          }
      }
};

QTEST_MAIN( TestGltfModel )

#include "test_gltf_model.moc"
