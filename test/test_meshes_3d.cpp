// -*-c++-*-

/*!
  \file test_meshes_3d.cpp
  \brief unit test of Meshes3D, the shapes of the 3D view
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

#include "meshes_3d.h"

#include <QVector3D>

#include <cmath>
#include <functional>

namespace {

QVector3D
position( const Vertex3D & v )
{
    return QVector3D( v.position[0], v.position[1], v.position[2] );
}

QVector3D
normal( const Vertex3D & v )
{
    return QVector3D( v.normal[0], v.normal[1], v.normal[2] );
}

/*!
  \brief check the triangles: whole, of some area, turned counterclockwise
  as seen from where their normals face, with unit normals
*/
void
checkTriangles( const std::vector< Vertex3D > & v )
{
    QVERIFY( ! v.empty() );
    QCOMPARE( v.size() % 3, size_t( 0 ) );
    for ( size_t i = 0; i < v.size(); i += 3 )
    {
        const QVector3D a = position( v[i] ), b = position( v[i + 1] ), c = position( v[i + 2] );
        const QVector3D face = QVector3D::crossProduct( b - a, c - a );
        QVERIFY2( face.length() > 1.0e-6f, qPrintable( QString( "triangle %1 has no area" ).arg( i / 3 ) ) );
        for ( int k = 0; k < 3; ++k )
        {
            const QVector3D n = normal( v[i + k] );
            QVERIFY( std::fabs( n.length() - 1.0f ) < 1.0e-5f );
            QVERIFY2( QVector3D::dotProduct( face, n ) > 0.0f,
                      qPrintable( QString( "triangle %1 turns clockwise" ).arg( i / 3 ) ) );
        }
    }
}

//! check that every vertex satisfies the condition
bool
all( const std::vector< Vertex3D > & v,
     const std::function< bool( const QVector3D & p, const QVector3D & n ) > & f )
{
    for ( const Vertex3D & x : v )
    {
        if ( ! f( position( x ), normal( x ) ) )
        {
            return false;
        }
    }
    return true;
}

constexpr float EPS = 1.0e-5f;

}

class TestMeshes3D
    : public QObject {
    Q_OBJECT

private slots:

    void cylinder()
      {
          const std::vector< Vertex3D > v = Meshes3D::cylinder( 20 );
          checkTriangles( v );
          // the side and the two ends of each segment
          QCOMPARE( v.size(), size_t( 20 * 4 * 3 ) );
          QVERIFY( all( v, []( const QVector3D & p, const QVector3D & ) {
                      return std::fabs( p.y() ) <= 0.5f + EPS
                          && std::hypot( p.x(), p.z() ) <= 1.0f + EPS; } ) );
          // the side faces outward, the ends up and down
          QVERIFY( all( v, []( const QVector3D & p, const QVector3D & n ) {
                      return std::fabs( n.y() ) > 0.5f
                          ? ( std::fabs( p.y() ) > 0.5f - EPS && n.y() * p.y() > 0.0f )
                          : QVector3D::dotProduct( n, QVector3D( p.x(), 0.0f, p.z() ) ) > 0.9f; } ) );
      }

    void sphere()
      {
          const std::vector< Vertex3D > v = Meshes3D::sphere( 24, 16 );
          checkTriangles( v );
          // the normal is the position on the unit sphere
          QVERIFY( all( v, []( const QVector3D & p, const QVector3D & n ) {
                      return std::fabs( p.length() - 1.0f ) < EPS
                          && ( p - n ).length() < EPS; } ) );
          // from the bottom to the top
          float lo = 0.0f, hi = 0.0f;
          for ( const Vertex3D & x : v ) { lo = std::min( lo, x.position[1] ); hi = std::max( hi, x.position[1] ); }
          QCOMPARE( lo, -1.0f );
          QCOMPARE( hi, 1.0f );
      }

    void cone()
      {
          const std::vector< Vertex3D > v = Meshes3D::cone( 12 );
          checkTriangles( v );
          // between the base at x = 0 and the tip at x = 1, within radius 1
          QVERIFY( all( v, []( const QVector3D & p, const QVector3D & ) {
                      return p.x() >= -EPS && p.x() <= 1.0f + EPS
                          && std::hypot( p.y(), p.z() ) <= 1.0f - p.x() + EPS; } ) );
          bool tip = false;
          for ( const Vertex3D & x : v ) tip = tip || ( position( x ) - QVector3D( 1, 0, 0 ) ).length() < EPS;
          QVERIFY( tip );
          // the base faces back
          QVERIFY( all( v, []( const QVector3D &, const QVector3D & n ) {
                      return n.x() > 0.5f || ( n - QVector3D( -1, 0, 0 ) ).length() < EPS; } ) );
      }

    void ring()
      {
          const std::vector< Vertex3D > ring = Meshes3D::ring( 0.92f, 1.08f, 40 );
          checkTriangles( ring );
          QCOMPARE( ring.size(), size_t( 40 * 2 * 3 ) );
          QVERIFY( all( ring, []( const QVector3D & p, const QVector3D & n ) {
                      const float r = std::hypot( p.x(), p.z() );
                      return p.y() == 0.0f && n == QVector3D( 0, 1, 0 )
                          && r > 0.92f - EPS && r < 1.08f + EPS; } ) );

          // a disc from the center
          const std::vector< Vertex3D > disc = Meshes3D::ring( 0.0f, 1.0f, 32 );
          checkTriangles( disc );
          QCOMPARE( disc.size(), size_t( 32 * 3 ) );
      }

    void quad()
      {
          const std::vector< Vertex3D > v = Meshes3D::quad();
          checkTriangles( v );
          QCOMPARE( v.size(), size_t( 6 ) );
          QVERIFY( all( v, []( const QVector3D & p, const QVector3D & n ) {
                      return std::fabs( p.x() ) == 0.5f && std::fabs( p.y() ) == 0.5f && p.z() == 0.0f
                          && n == QVector3D( 0, 0, 1 ); } ) );
      }

    void ground()
      {
          const std::vector< Vertex3D > v = Meshes3D::ground( 115.0f, 78.0f );
          checkTriangles( v );
          QCOMPARE( v.size(), size_t( 6 ) );
          // the corners of the image at the corners of the area
          for ( const Vertex3D & x : v )
          {
              QCOMPARE( std::fabs( x.position[0] ), 57.5f );
              QCOMPARE( x.position[1], 0.0f );
              QCOMPARE( std::fabs( x.position[2] ), 39.0f );
              QCOMPARE( x.uv[0], x.position[0] < 0.0f ? 0.0f : 1.0f );
              QCOMPARE( x.uv[1], x.position[2] < 0.0f ? 0.0f : 1.0f );
          }
      }
};

QTEST_MAIN( TestMeshes3D )
#include "test_meshes_3d.moc"
