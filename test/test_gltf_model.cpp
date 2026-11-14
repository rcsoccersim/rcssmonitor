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

#include <cmath>

namespace {

//! the bundled model, read from the source tree
QByteArray
mannequin()
{
    QFile file( QStringLiteral( TEST_SOURCE_DIR "/resources/models/Mannequin.glb" ) );
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
      }
};

QTEST_MAIN( TestGltfModel )

#include "test_gltf_model.moc"
