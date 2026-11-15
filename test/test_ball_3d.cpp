// -*-c++-*-

/*!
  \file test_ball_3d.cpp
  \brief unit test of the ball of the 3D view: its pattern and its rolling
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

#include "ball_3d.h"
#include "interpolate.h"

#include <cmath>
#include <memory>

namespace {

constexpr float RADIUS = 0.1f;

//! a frame with the ball at (x, y), in a play mode
DispConstPtr
frame( const float x,
       const float y,
       const rcss::rcg::PlayMode mode = rcss::rcg::PM_PlayOn )
{
    auto d = std::make_shared< rcss::rcg::DispInfoT >();
    d->pmode_ = mode;
    d->show_.ball_.x_ = x;
    d->show_.ball_.y_ = y;
    return d;
}

//! the angle between two turns (degree)
float
angleBetween( const QQuaternion & a,
              const QQuaternion & b )
{
    // of the turn from b to a, without acos (not fine near 0)
    const QQuaternion q = a.normalized() * b.normalized().conjugated();
    return 2.0f * std::atan2( q.vector().length(), std::fabs( q.scalar() ) ) * 180.0f / float( M_PI );
}

//! the frames of a ball rolling along x at 0.2 m a cycle
DispCont
rolling( const int count )
{
    DispCont frames;
    for ( int i = 0; i < count; ++i ) frames.push_back( frame( 0.2f * i, 0.0f ) );
    return frames;
}

}

/*!
  \brief unit test of Ball3D and BallSpin
*/
class TestBall3D
    : public QObject {
    Q_OBJECT

private slots:

    /*!
      the pentagons: at the 12 corners of an icosahedron, of length 1, each
      corner toward a neighbor 63.4 degrees away
    */
    void pentagons()
      {
          const Ball3D::Pentagons & p = Ball3D::pentagons();
          for ( int i = 0; i < Ball3D::PENTAGONS; ++i )
          {
              QVERIFY( std::fabs( p.centers[i].length() - 1.0f ) < 1.0e-5f );
              const float neighbor = std::acos( QVector3D::dotProduct( p.centers[i], p.toward[i] ) );
              QVERIFY2( std::fabs( neighbor - std::atan( 2.0f ) ) < 1.0e-4f, qPrintable( QString::number( neighbor ) ) );
              // five neighbors each, as on an icosahedron, none nearer
              int neighbors = 0;
              for ( int j = 0; j < Ball3D::PENTAGONS; ++j )
              {
                  if ( j == i ) continue;
                  const float a = std::acos( std::min( 1.0f, QVector3D::dotProduct( p.centers[i], p.centers[j] ) ) );
                  QVERIFY( a > neighbor - 1.0e-4f );
                  if ( a < neighbor + 1.0e-4f ) ++neighbors;
              }
              QCOMPARE( neighbors, 5 );
          }
          // the pentagons' corners a third of the way to the neighbors
          QVERIFY( std::fabs( Ball3D::PENTAGON - std::atan( 2.0f ) / 3.0f ) < 0.003f );
      }

    /*!
      the pattern: dark at the pentagons' centers, out to their corners
      (toward the neighbors) and to the middles of their edges, light
      beyond, and between two neighbors; about 30% of the ball dark
    */
    void pattern()
      {
          const Ball3D::Pentagons & p = Ball3D::pentagons();
          const float shade = Ball3D::PENTAGON_SHADE;
          for ( int i = 0; i < Ball3D::PENTAGONS; ++i )
          {
              const QVector3D c = p.centers[i];
              QCOMPARE( Ball3D::patternAt( c ), shade );
              QCOMPARE( Ball3D::patternAt( c * 3.0f ), shade ); // any length
              // toward the corner (the neighbor), out to PENTAGON
              const QVector3D e1 = ( p.toward[i] - c * QVector3D::dotProduct( p.toward[i], c ) ).normalized();
              const auto at = [&]( const float angle, const float around ) {
                  const QVector3D e2 = QVector3D::crossProduct( c, e1 );
                  const QVector3D way = e1 * std::cos( around ) + e2 * std::sin( around );
                  return Ball3D::patternAt( c * std::cos( angle ) + way * std::sin( angle ) );
              };
              QCOMPARE( at( 0.97f * Ball3D::PENTAGON, 0.0f ), shade );
              QCOMPARE( at( 1.03f * Ball3D::PENTAGON, 0.0f ), 1.0f );
              // the five corners: every 72 degrees round
              for ( int k = 0; k < 5; ++k )
              {
                  QCOMPARE( at( 0.97f * Ball3D::PENTAGON, k * 2.0f * float( M_PI ) / 5.0f ), shade );
              }
              // the middle of an edge: nearer, cos(36 degrees) as far
              const float apothem = Ball3D::PENTAGON * std::cos( float( M_PI ) / 5.0f );
              QCOMPARE( at( 0.97f * apothem, float( M_PI ) / 5.0f ), shade );
              QCOMPARE( at( 1.03f * apothem, float( M_PI ) / 5.0f ), 1.0f );
              // halfway to the neighbor: white
              QCOMPARE( Ball3D::patternAt( c + p.toward[i] ), 1.0f );
          }

          // dark over about 30% of the ball (12 pentagons of a truncated icosahedron)
          int dark = 0, count = 0;
          const int n = 200;
          for ( int a = 0; a < n; ++a )
          {
              const float z = -1.0f + ( a + 0.5f ) * 2.0f / n; // equal areas
              for ( int b = 0; b < 2 * n; ++b )
              {
                  const float t = ( b + 0.5f ) * float( M_PI ) / n;
                  const float s = std::sqrt( 1.0f - z * z );
                  if ( Ball3D::patternAt( QVector3D( s * std::cos( t ), s * std::sin( t ), z ) ) < 0.5f ) ++dark;
                  ++count;
              }
          }
          const double share = double( dark ) / count;
          QVERIFY2( share > 0.27 && share < 0.33, qPrintable( QString::number( share ) ) );
      }

    /*!
      rolling: along the field's x the top goes on toward +x, along the
      field's y (the world's +z) toward +z; as far as the distance over the
      radius, at most MAX_ROLL; the point on the ground does not slide; no
      turn without a move
    */
    void roll()
      {
          const QVector3D top( 0.0f, 1.0f, 0.0f );
          const float d = 0.1f; // a radian on a ball of RADIUS
          QVector3D q = Ball3D::roll( 0.0f, 0.0f, d, 0.0f, RADIUS ).rotatedVector( top );
          QVERIFY( std::fabs( q.x() - std::sin( 1.0f ) ) < 1.0e-5f );
          QVERIFY( std::fabs( q.y() - std::cos( 1.0f ) ) < 1.0e-5f );
          QVERIFY( std::fabs( q.z() ) < 1.0e-5f );
          q = Ball3D::roll( 0.0f, 0.0f, 0.0f, d, RADIUS ).rotatedVector( top );
          QVERIFY( std::fabs( q.z() - std::sin( 1.0f ) ) < 1.0e-5f );
          QVERIFY( std::fabs( q.x() ) < 1.0e-5f );
          q = Ball3D::roll( 3.0f, 1.0f, 3.0f - d, 1.0f, RADIUS ).rotatedVector( top );
          QVERIFY( std::fabs( q.x() + std::sin( 1.0f ) ) < 1.0e-5f );

          // the angle, its limit and the fraction
          QVERIFY( std::fabs( angleBetween( Ball3D::roll( 0.0f, 0.0f, 0.03f, 0.04f, RADIUS ), QQuaternion() )
                              - 0.5f * 180.0f / float( M_PI ) ) < 0.01f );
          QVERIFY( std::fabs( angleBetween( Ball3D::roll( 0.0f, 0.0f, 2.0f, 0.0f, RADIUS ), QQuaternion() )
                              - Ball3D::MAX_ROLL * 180.0f / float( M_PI ) ) < 0.01f );
          QVERIFY( std::fabs( angleBetween( Ball3D::roll( 0.0f, 0.0f, d, 0.0f, RADIUS, 0.25f ), QQuaternion() )
                              - 0.25f * 180.0f / float( M_PI ) ) < 0.01f );
          QCOMPARE( Ball3D::roll( 1.0f, 2.0f, 1.0f, 2.0f, RADIUS ), QQuaternion() );

          // the point on the ground stays where it was (no slide) a small way on
          const float small = 0.002f;
          const QVector3D bottom = Ball3D::roll( 0.0f, 0.0f, small * 0.6f, small * 0.8f, RADIUS )
              .rotatedVector( QVector3D( 0.0f, -RADIUS, 0.0f ) );
          QVERIFY( std::fabs( small * 0.6f + bottom.x() ) < 1.0e-6f );
          QVERIFY( std::fabs( small * 0.8f + bottom.z() ) < 1.0e-6f );
      }

    /*!
      rolls only as the view moves the ball: not over a play mode change,
      nor over a jump farther than the ball goes in a cycle
    */
    void rolls()
      {
          QVERIFY( Ball3D::rolls( *frame( 0.0f, 0.0f ), *frame( 2.9f, 0.0f ) ) );
          QVERIFY( Ball3D::rolls( *frame( 0.0f, 0.0f ), *frame( 0.0f, Interpolate::MAX_BALL_STEP ) ) );
          QVERIFY( ! Ball3D::rolls( *frame( 0.0f, 0.0f ), *frame( 0.0f, Interpolate::MAX_BALL_STEP + 0.1f ) ) );
          QVERIFY( ! Ball3D::rolls( *frame( 0.0f, 0.0f ), *frame( 0.5f, 0.0f, rcss::rcg::PM_KickIn_Left ) ) );
      }

    /*!
      the table: the turns add up frame by frame, between two frames by the
      fraction; the same however the frames came; no turn over a jump or a
      play mode change
    */
    void spin()
      {
          const DispCont frames = rolling( 30 );
          BallSpin spin;
          spin.update( frames, RADIUS );
          QCOMPARE( spin.frameCount(), std::size_t( 30 ) );
          QCOMPARE( spin.turnAt( frames, 0, 1, 0.0 ), QQuaternion() );
          // 2 radians a cycle about -z
          const QQuaternion step = QQuaternion::fromAxisAndAngle( 0.0f, 0.0f, -1.0f, 2.0f * 180.0f / float( M_PI ) );
          QQuaternion expected;
          for ( std::size_t i = 0; i < 30; ++i )
          {
              QVERIFY2( angleBetween( spin.turnAt( frames, i, std::min( i + 1, std::size_t( 29 ) ), 0.0 ), expected ) < 0.01f,
                        qPrintable( QString::number( i ) ) );
              expected = step * expected;
          }
          // a quarter of the way on
          const QQuaternion at5 = spin.turnAt( frames, 5, 5, 0.0 );
          const QQuaternion quarter = QQuaternion::fromAxisAndAngle( 0.0f, 0.0f, -1.0f, 0.5f * 180.0f / float( M_PI ) );
          QVERIFY( angleBetween( spin.turnAt( frames, 5, 6, 0.25 ), quarter * at5 ) < 0.01f );
          QCOMPARE( spin.turnAt( frames, 5, 5, 0.25 ), at5 ); // no next
          QCOMPARE( spin.turnAt( frames, 99, 99, 0.0 ), spin.turnAt( frames, 29, 29, 0.0 ) ); // beyond the end

          // frame by frame, as they come live: the same
          BallSpin live;
          DispCont part;
          for ( const DispConstPtr & f : frames )
          {
              part.push_back( f );
              live.update( part, RADIUS );
          }
          for ( std::size_t i = 0; i < 30; ++i )
          {
              QCOMPARE( live.turnAt( frames, i, i, 0.0 ), spin.turnAt( frames, i, i, 0.0 ) );
          }

          // a jump and a play mode change: no turn
          DispCont broken;
          broken.push_back( frame( 0.0f, 0.0f ) );
          broken.push_back( frame( 0.2f, 0.0f ) );
          broken.push_back( frame( 10.0f, 0.0f ) ); // a placement
          broken.push_back( frame( 10.2f, 0.0f, rcss::rcg::PM_FreeKick_Left ) );
          broken.push_back( frame( 10.4f, 0.0f, rcss::rcg::PM_FreeKick_Left ) );
          BallSpin other;
          other.update( broken, RADIUS );
          const QQuaternion one = other.turnAt( broken, 1, 1, 0.0 );
          QVERIFY( angleBetween( one, step ) < 0.01f );
          QCOMPARE( other.turnAt( broken, 2, 2, 0.0 ), one );
          QCOMPARE( other.turnAt( broken, 1, 2, 0.5 ), one ); // not on the way to a placement
          QCOMPARE( other.turnAt( broken, 3, 3, 0.0 ), one );
          QVERIFY( angleBetween( other.turnAt( broken, 4, 4, 0.0 ), step * one ) < 0.01f );
      }

    /*!
      a live game drops its oldest frames: the turns of the frames left
      stay; other frames, or another radius, start again
    */
    void follow()
      {
          const DispCont frames = rolling( 40 );
          BallSpin spin;
          spin.update( frames, RADIUS );
          const QQuaternion at30 = spin.turnAt( frames, 30, 30, 0.0 );

          DispCont dropped( frames.begin() + 10, frames.end() );
          spin.update( dropped, RADIUS );
          QCOMPARE( spin.frameCount(), std::size_t( 30 ) );
          QCOMPARE( spin.turnAt( dropped, 20, 20, 0.0 ), at30 );
          // more frames after the drop
          DispCont more = dropped;
          more.push_back( frame( 8.0f, 0.0f ) );
          spin.update( more, RADIUS );
          QCOMPARE( spin.frameCount(), std::size_t( 31 ) );
          QCOMPARE( spin.turnAt( more, 20, 20, 0.0 ), at30 );

          // other frames: from the start again
          const DispCont another = rolling( 5 );
          spin.update( another, RADIUS );
          QCOMPARE( spin.frameCount(), std::size_t( 5 ) );
          QCOMPARE( spin.turnAt( another, 0, 0, 0.0 ), QQuaternion() );

          // another radius: rolled again, by half as much on a ball twice as large
          spin.update( another, 2.0f * RADIUS );
          QVERIFY( std::fabs( angleBetween( spin.turnAt( another, 1, 1, 0.0 ), QQuaternion() )
                              - 1.0f * 180.0f / float( M_PI ) ) < 0.01f );
          spin.clear();
          QCOMPARE( spin.frameCount(), std::size_t( 0 ) );
          QCOMPARE( spin.turnAt( another, 3, 3, 0.0 ), QQuaternion() );
      }
};

QTEST_MAIN( TestBall3D )

#include "test_ball_3d.moc"
