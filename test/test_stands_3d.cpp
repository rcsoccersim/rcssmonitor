// -*-c++-*-

/*!
  \file test_stands_3d.cpp
  \brief unit test of Stands3D
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

#include "stands_3d.h"

#include <cmath>

using namespace Stands3D;

namespace {

QVector3D
pos( const Vertex3D & v )
{
    return QVector3D( v.position[0], v.position[1], v.position[2] );
}

QVector3D
normal( const Vertex3D & v )
{
    return QVector3D( v.normal[0], v.normal[1], v.normal[2] );
}

int
distance( const QColor & a,
          const QColor & b )
{
    return std::abs( a.red() - b.red() ) + std::abs( a.green() - b.green() ) + std::abs( a.blue() - b.blue() );
}

}

/*!
  \brief test of the stands of the 3D view
*/
class TestStands3D
    : public QObject {
    Q_OBJECT

private slots:

    /*!
      the four stands: along both touch lines and behind both goals, 2 m
      behind the boards, as long as the boards, the corners open; to the
      right seen from the pitch along them
    */
    void stands()
      {
          const Stand far = stand( 0 ), near = stand( 1 ), left = stand( 2 ), right = stand( 3 );
          QCOMPARE( far.out, QVector3D( 0, 0, -1 ) );
          QCOMPARE( near.out, QVector3D( 0, 0, 1 ) );
          QCOMPARE( left.out, QVector3D( -1, 0, 0 ) );
          QCOMPARE( right.out, QVector3D( 1, 0, 0 ) );
          for ( int i = 0; i < STAND_COUNT; ++i )
          {
              const Stand s = stand( i );
              // to the right seen from the pitch: looking out, along = out x up
              QCOMPARE( s.along, QVector3D::crossProduct( s.out, QVector3D( 0, 1, 0 ) ) );
          }
          QCOMPARE( far.front, 34.0f + Stadium3D::BOARD_OFFSET + 2.0f );
          QCOMPARE( left.front, 52.5f + Stadium3D::BOARD_OFFSET + 2.0f );
          QCOMPARE( far.half_length, 52.5f + Stadium3D::BOARD_OFFSET );
          QCOMPARE( left.half_length, 34.0f + Stadium3D::BOARD_OFFSET );
          // the corners open: a side stand ends before an end stand begins
          QVERIFY( far.half_length < left.front );
          QVERIFY( left.half_length < far.front );
          // as the web version's: 16 rows, 7.5 m high
          QVERIFY( std::fabs( top() - 7.5f ) < 0.01f );
      }

    /*!
      the triangles: of an area, counterclockwise seen from where they face;
      the solid closed (the treads up, the risers and the front wall to the
      pitch, the back wall out, the ends out along the stand); within it;
      their places in the image within it
    */
    void mesh()
      {
          for ( int index = 0; index < STAND_COUNT; ++index )
          {
              const Stand s = stand( index );
              const std::vector< Vertex3D > v = Stands3D::mesh( index );
              QVERIFY( ! v.empty() );
              QCOMPARE( v.size() % 3, std::size_t( 0 ) );
              int up = 0, pitch = 0, back = 0, ends = 0;
              for ( std::size_t i = 0; i < v.size(); i += 3 )
              {
                  const QVector3D n = normal( v[i] );
                  QVERIFY( std::fabs( n.length() - 1.0f ) < 1.0e-5f );
                  const QVector3D cross = QVector3D::crossProduct( pos( v[i + 1] ) - pos( v[i] ), pos( v[i + 2] ) - pos( v[i] ) );
                  QVERIFY( cross.length() > 1.0e-6f );
                  QVERIFY( QVector3D::dotProduct( cross, n ) > 0.0f );
                  if ( n == QVector3D( 0, 1, 0 ) ) ++up;
                  else if ( n == -s.out ) ++pitch;
                  else if ( n == s.out ) ++back;
                  else if ( n == s.along || n == -s.along )
                  {
                      // at an end, facing out of the stand along it
                      ++ends;
                      QVERIFY( QVector3D::dotProduct( n, s.along ) * QVector3D::dotProduct( pos( v[i] ), s.along ) > 0.0f );
                  }
                  else QFAIL( "a face of another direction" );
                  for ( int k = 0; k < 3; ++k )
                  {
                      const QVector3D p = pos( v[i + k] );
                      const float a = QVector3D::dotProduct( p, s.along );
                      const float d = QVector3D::dotProduct( p, s.out ) - s.front;
                      QVERIFY( std::fabs( a ) <= s.half_length + 1.0e-3f );
                      QVERIFY( d >= -1.0e-3f && d <= depth() + 1.0e-3f );
                      QVERIFY( p.y() >= -1.0e-4f && p.y() <= top() + 1.0e-4f );
                      QVERIFY( v[i + k].uv[0] >= -1.0e-5f && v[i + k].uv[0] <= 1.0f + 1.0e-5f );
                      QVERIFY( v[i + k].uv[1] >= -1.0e-5f && v[i + k].uv[1] <= 1.0f + 1.0e-5f );
                  }
              }
              QCOMPARE( up, 2 * STAND_ROWS ); // a tread a row
              QCOMPARE( pitch, 2 * STAND_ROWS ); // the front wall and the risers between the rows
              QCOMPARE( back, 2 ); // the back wall: seen from behind, not see-through
              QCOMPARE( ends, 2 * 2 * STAND_ROWS );
          }
      }

    /*!
      inside a stand: below its rows, within its length and depth; not above
      it, in front of it, behind it, nor beyond its ends
    */
    void inside()
      {
          const Stand near = stand( 1 );
          const float front = near.front;
          QVERIFY( insideStand( 1, QVector3D( 0, 1.0f, front + 1.0f ) ) );
          QVERIFY( insideStand( 1, QVector3D( 10, 4.0f, front + 10.0f ) ) );
          QVERIFY( ! insideStand( 1, QVector3D( 0, 9.0f, front + 10.0f ) ) ); // above
          QVERIFY( ! insideStand( 1, QVector3D( 0, 1.0f, front - 0.5f ) ) ); // in front
          QVERIFY( ! insideStand( 1, QVector3D( 0, 1.0f, front + depth() + 0.5f ) ) ); // behind
          QVERIFY( ! insideStand( 1, QVector3D( near.half_length + 1.0f, 1.0f, front + 5.0f ) ) ); // beyond its end
          QVERIFY( ! insideStand( 0, QVector3D( 0, 1.0f, front + 1.0f ) ) ); // not the other one
          // behind a goal: the goal camera (x = 70.5, 10 m up) is above the rows
          QVERIFY( ! insideStand( 2, QVector3D( -70.5f, 10.0f, 0.0f ) ) );
          QVERIFY( insideStand( 2, QVector3D( -70.5f, 2.0f, 0.0f ) ) );
      }

    /*!
      the images: of their sizes, the same every time; along each stand to
      the right seen from the pitch; the fans of each team toward its end,
      behind a goal mostly the team that keeps it; no one on the stairs,
      which are painted every row
    */
    void images()
      {
          Stadium3D::TeamLook left, right;
          left.color = QColor( 0xe0, 0x20, 0xc0 ); // magenta: no one else wears it
          left.name = "LEFT";
          right.color = QColor( 0x10, 0xe0, 0xe0 ); // cyan
          right.name = "RIGHT";
          std::vector< QImage > all;
          for ( int index = 0; index < STAND_COUNT; ++index )
          {
              const Stand s = stand( index );
              const QImage img = Stands3D::image( index, left, right );
              QCOMPARE( img.size(), QSize( imageWidth( index ), imageHeight() ) );
              QCOMPARE( Stands3D::image( index, left, right ), img );
              all.push_back( img );
              // the people in a color along a part of the stand (a along it)
              const auto count = [&]( const QColor & color, const float a0, const float a1 ) {
                  int n = 0;
                  const int x0 = int( ( a0 + s.half_length ) * PIXELS_PER_METER );
                  const int x1 = int( ( a1 + s.half_length ) * PIXELS_PER_METER );
                  for ( int y = 0; y < rowTop( STAND_ROWS - 1 ) + ROW_PIXELS; ++y )
                  {
                      for ( int x = std::max( 0, x0 ); x < std::min( img.width(), x1 ); ++x )
                      {
                          if ( distance( img.pixelColor( x, y ), color ) < 60 ) ++n;
                      }
                  }
                  return n;
              };
              const float L = s.half_length;
              if ( index < 2 )
              {
                  // a side: the left team's fans where the stand is at x < 0
                  // (the far one along +x, the near one along -x)
                  const float a_left = ( s.along.x() > 0 ? -L : L * 2.0f / 3.0f );
                  const float a_right = ( s.along.x() > 0 ? L * 2.0f / 3.0f : -L );
                  const int ll = count( left.color, a_left, a_left + L / 3.0f );
                  const int lr = count( left.color, a_right, a_right + L / 3.0f );
                  const int rr = count( right.color, a_right, a_right + L / 3.0f );
                  const int rl = count( right.color, a_left, a_left + L / 3.0f );
                  QVERIFY2( ll > 3 * lr && rr > 3 * rl, qPrintable( QString( "%1 %2 %3 %4" ).arg( ll ).arg( lr ).arg( rr ).arg( rl ) ) );
              }
              else
              {
                  // behind a goal: mostly the team that keeps it
                  const QColor & keeper = ( index == 2 ? left : right ).color;
                  const QColor & other = ( index == 2 ? right : left ).color;
                  const int k = count( keeper, -L, L ), o = count( other, -L, L );
                  QVERIFY2( k > 10 * o, qPrintable( QString( "%1 %2" ).arg( k ).arg( o ) ) );
              }
              QVERIFY( count( QColor( 0x1c, 0x1c, 0x1e ), -L, L ) > 0 ); // the plain clothes
              // the stairs at a = 0: no one, and painted every row
              const int stair = int( L * PIXELS_PER_METER );
              for ( int y = 0; y < rowTop( STAND_ROWS - 1 ) + ROW_PIXELS; ++y )
              {
                  for ( int x = stair - 4; x <= stair + 4; ++x )
                  {
                      QVERIFY( distance( img.pixelColor( x, y ), left.color ) > 60 );
                      QVERIFY( distance( img.pixelColor( x, y ), right.color ) > 60 );
                  }
              }
              for ( int r = 0; r < STAND_ROWS; ++r )
              {
                  const QColor c = img.pixelColor( stair, rowTop( r ) + 4 );
                  QVERIFY2( c.saturation() < 40 && c.lightness() > 90, qPrintable( c.name() ) );
              }
          }
          // each stand its own crowd
          QVERIFY( all[0] != all[1] );
          QVERIFY( all[2] != all[3] );
      }
};

QTEST_MAIN( TestStands3D )
#include "test_stands_3d.moc"
