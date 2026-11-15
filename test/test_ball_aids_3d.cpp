// -*-c++-*-

/*!
  \file test_ball_aids_3d.cpp
  \brief unit test of what helps to find the ball in the 3D view: the ring and the trail
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

#include "ball_aids_3d.h"
#include "interpolate.h"

#include <cmath>
#include <memory>

namespace {

constexpr float RADIUS = 0.1f;

//! a frame with the ball at (x, y), in a play mode
DispConstPtr
frame( const float x,
       const float y,
       const rcss::rcg::PlayMode mode = rcss::rcg::PM_PlayOn,
       const float z = 0.0f )
{
    auto d = std::make_shared< rcss::rcg::DispInfoT >();
    d->pmode_ = mode;
    d->show_.ball_.x_ = x;
    d->show_.ball_.y_ = y;
    d->show_.ball_.z_ = z;
    return d;
}

//! the frames of a ball going along x at 1 m a cycle from x = 0
DispCont
going( const int count )
{
    DispCont frames;
    for ( int i = 0; i < count; ++i ) frames.push_back( frame( float( i ), 2.0f ) );
    return frames;
}

bool
near( const QVector3D & a,
      const QVector3D & b )
{
    return ( a - b ).length() < 1.0e-4f;
}

bool
near( const float a,
      const float b )
{
    return std::fabs( a - b ) < 1.0e-4f;
}

}

/*!
  \brief unit test of BallAids3D
*/
class TestBallAids3D
    : public QObject {
    Q_OBJECT

private slots:

    /*!
      the trail's points: from the place drawn back as many cycles, the last
      one part of the way; the ball's centers (its radius high)
    */
    void trailPoints()
      {
          const DispCont frames = going( 20 );
          // at a frame: it and the frames before, cycles of them
          std::vector< BallAids3D::TrailPoint > p = BallAids3D::trailPoints( frames, 10, 11, 0.0, 5, RADIUS );
          QCOMPARE( p.size(), std::size_t( 6 ) );
          for ( std::size_t k = 0; k < p.size(); ++k )
          {
              QVERIFY( near( p[k].at, QVector3D( 10.0f - k, RADIUS, 2.0f ) ) );
              QVERIFY( near( p[k].age, float( k ) ) );
          }
          // between two frames: the place drawn, then the frames, the last part of the way
          p = BallAids3D::trailPoints( frames, 10, 11, 0.25, 5, RADIUS );
          QCOMPARE( p.size(), std::size_t( 7 ) );
          QVERIFY( near( p[0].at, QVector3D( 10.25f, RADIUS, 2.0f ) ) );
          QVERIFY( near( p[0].age, 0.0f ) );
          QVERIFY( near( p[1].at, QVector3D( 10.0f, RADIUS, 2.0f ) ) );
          QVERIFY( near( p[1].age, 0.25f ) );
          QVERIFY( near( p[5].age, 4.25f ) );
          QVERIFY( near( p[6].at, QVector3D( 5.25f, RADIUS, 2.0f ) ) );
          QVERIFY( near( p[6].age, 5.0f ) );
          // as long in time: one cycle
          p = BallAids3D::trailPoints( frames, 10, 11, 0.5, 1, RADIUS );
          QCOMPARE( p.size(), std::size_t( 3 ) );
          QVERIFY( near( p[2].at, QVector3D( 9.5f, RADIUS, 2.0f ) ) );
          QVERIFY( near( p[2].age, 1.0f ) );
          // no next frame: at the frame
          p = BallAids3D::trailPoints( frames, 19, 19, 0.5, 3, RADIUS );
          QCOMPARE( p.size(), std::size_t( 4 ) );
          QVERIFY( near( p[0].at, QVector3D( 19.0f, RADIUS, 2.0f ) ) );
          // near the start of the log: as far as there are frames
          QCOMPARE( BallAids3D::trailPoints( frames, 2, 3, 0.0, 5, RADIUS ).size(), std::size_t( 3 ) );
          QCOMPARE( BallAids3D::trailPoints( frames, 0, 1, 0.0, 5, RADIUS ).size(), std::size_t( 1 ) );
          // out of the frames, none
          QVERIFY( BallAids3D::trailPoints( frames, 20, 20, 0.0, 5, RADIUS ).empty() );
          QVERIFY( BallAids3D::trailPoints( DispCont(), 0, 0, 0.0, 5, RADIUS ).empty() );

          // in the air (z): the center as high
          DispCont air;
          air.push_back( frame( 0.0f, 0.0f, rcss::rcg::PM_PlayOn, 1.0f ) );
          air.push_back( frame( 1.0f, 0.0f, rcss::rcg::PM_PlayOn, 2.0f ) );
          p = BallAids3D::trailPoints( air, 1, 1, 0.0, 5, RADIUS );
          QVERIFY( near( p[0].at, QVector3D( 1.0f, 2.0f + RADIUS, 0.0f ) ) );
          QVERIFY( near( p[1].at, QVector3D( 0.0f, 1.0f + RADIUS, 0.0f ) ) );
      }

    /*!
      the trail breaks where the view does not move the ball: a placement
      (farther than the ball goes in a cycle), a play mode change
    */
    void trailBreaks()
      {
          DispCont frames = going( 6 );
          frames.push_back( frame( 20.0f, 2.0f ) ); // 6: placed
          frames.push_back( frame( 21.0f, 2.0f ) );
          frames.push_back( frame( 22.0f, 2.0f, rcss::rcg::PM_KickIn_Left ) ); // 8: another mode
          frames.push_back( frame( 23.0f, 2.0f, rcss::rcg::PM_KickIn_Left ) );
          QCOMPARE( BallAids3D::trailPoints( frames, 7, 8, 0.0, 10, RADIUS ).size(), std::size_t( 2 ) );
          QCOMPARE( BallAids3D::trailPoints( frames, 9, 9, 0.0, 10, RADIUS ).size(), std::size_t( 2 ) );
          // not on the way to a placement
          const std::vector< BallAids3D::TrailPoint > p = BallAids3D::trailPoints( frames, 5, 6, 0.5, 10, RADIUS );
          QVERIFY( near( p[0].at, QVector3D( 5.0f, RADIUS, 2.0f ) ) );
          QVERIFY( near( p[0].age, 0.0f ) );
          QCOMPARE( p.size(), std::size_t( 6 ) );
      }

    /*!
      meters per pixel: of the orthographic camera, its height over the
      screen's; of a perspective one, by the depth along its view
    */
    void metersPerPixel()
      {
          CameraPose top;
          top.position = QVector3D( 0, 100, 0 );
          top.target = QVector3D( 0, 0, 0 );
          top.up = QVector3D( 0, 0, -1 );
          top.ortho_half_height = 40.0f;
          QVERIFY( near( BallAids3D::metersPerPixel( top, QVector3D( 30, 0, 10 ), 800.0f ), 0.1f ) );

          CameraPose camera;
          camera.position = QVector3D( 0, 0, 10 );
          camera.target = QVector3D( 0, 0, 0 );
          camera.fov_y = 90.0f;
          // 10 m deep: 20 m across the screen's height; off the axis, as deep
          QVERIFY( near( BallAids3D::metersPerPixel( camera, QVector3D( 0, 0, 0 ), 200.0f ), 0.1f ) );
          QVERIFY( near( BallAids3D::metersPerPixel( camera, QVector3D( 5, 3, 0 ), 200.0f ), 0.1f ) );
          QVERIFY( near( BallAids3D::metersPerPixel( camera, QVector3D( 0, 0, 5 ), 200.0f ), 0.05f ) );
          // behind the camera: as at 0.1 m
          QVERIFY( near( BallAids3D::metersPerPixel( camera, QVector3D( 0, 0, 20 ), 200.0f ), 0.001f ) );
      }

    /*!
      the ring: on the ground around the ball, as large as its radius, in
      the ball's color; its line RING_WIDTH, or RING_PIXELS where wider, at
      most half the radius
    */
    void ring()
      {
          SceneContent c;
          BallAids3D::addRing( c, QVector3D( 3, 0.5f, -4 ), 0.7f, 0.001f, QColor( 255, 128, 0 ) );
          QCOMPARE( c.ball_rings.size(), std::size_t( 1 ) );
          const Instance3D & r = c.ball_rings[0];
          QVERIFY( near( r.model.map( QVector3D() ), QVector3D( 3, BallAids3D::RING_Y, -4 ) ) );
          QVERIFY( near( r.model.map( QVector3D( 1, 0, 0 ) ), QVector3D( 3.7f, BallAids3D::RING_Y, -4 ) ) );
          QVERIFY( near( r.model.map( QVector3D( 0, 0, 1 ) ), QVector3D( 3, BallAids3D::RING_Y, -3.3f ) ) );
          QVERIFY( near( r.color.toVector3D(), QVector3D( 1.0f, 128.0f / 255.0f, 0.0f ) ) );
          QVERIFY( near( c.ball_ring_radius, 0.7f ) );
          QVERIFY( near( c.ball_ring_width, BallAids3D::RING_WIDTH ) );
          // far away: some pixels wide
          c.clear();
          BallAids3D::addRing( c, QVector3D(), 0.7f, 0.1f, Qt::white );
          QVERIFY( near( c.ball_ring_width, BallAids3D::RING_PIXELS * 0.1f ) );
          // but at most half the radius
          c.clear();
          BallAids3D::addRing( c, QVector3D(), 0.2f, 0.1f, Qt::white );
          QVERIFY( near( c.ball_ring_width, 0.1f ) );
          QVERIFY( c.ball_rings.empty() == false );
          c.clear();
          QVERIFY( c.ball_rings.empty() );
          QCOMPARE( c.ball_ring_width, 0.0f );
      }

    /*!
      the trail's ribbon: two quads between two points, across the way and
      facing the eye, head_width wide at the head and narrower the older,
      opaque along the middle (fainter the older), clear at the edges
    */
    void trail()
      {
          const DispCont frames = going( 20 );
          const std::vector< BallAids3D::TrailPoint > points = BallAids3D::trailPoints( frames, 10, 11, 0.0, 5, RADIUS );
          const QVector3D eye( 10, 20, 2 ); // above the ball's way
          SceneContent c;
          BallAids3D::addTrail( c, points, eye, 0.6f, 5, QColor( 255, 0, 0 ) );
          QCOMPARE( c.ball_trail.size(), std::size_t( 12 * ( points.size() - 1 ) ) );

          // the vertices of each point: the middle on it with its opacity, the
          // edges either side across the way, clear
          float last_width = 1.0e9f, last_opacity = 2.0f;
          for ( std::size_t i = 0; i + 1 < points.size(); ++i )
          {
              const ColorVertex3D & left = c.ball_trail[12 * i];
              const ColorVertex3D & middle = c.ball_trail[12 * i + 1];
              const ColorVertex3D & right = c.ball_trail[12 * i + 7];
              QVERIFY( near( middle.position, points[i].at ) );
              QCOMPARE( left.color.w(), 0.0f );
              QCOMPARE( right.color.w(), 0.0f );
              QVERIFY( near( middle.color.toVector3D(), QVector3D( 1, 0, 0 ) ) );
              const float opacity = BallAids3D::TRAIL_OPACITY * ( 1.0f - points[i].age / 5.0f );
              QVERIFY( near( middle.color.w(), opacity ) );
              QVERIFY( opacity < last_opacity );
              // across: perpendicular to the way (x) and to the eye
              const QVector3D across = left.position - right.position;
              QVERIFY( std::fabs( across.x() ) < 1.0e-4f );
              QVERIFY( std::fabs( QVector3D::dotProduct( across, eye - points[i].at ) ) < 1.0e-3f );
              const float width = across.length();
              QVERIFY( near( width, 0.6f * std::pow( 1.0f - points[i].age / 5.0f, 0.6f ) ) );
              QVERIFY( width < last_width );
              QVERIFY( near( ( left.position + right.position ) * 0.5f, points[i].at ) );
              last_width = width;
              last_opacity = opacity;
          }
          // the tail: nothing wide, clear
          const ColorVertex3D & tail = c.ball_trail.back();
          QVERIFY( near( tail.position, points.back().at ) );
          QCOMPARE( tail.color.w(), 0.0f );

          // straight at the camera: the last direction across, not none
          c.clear();
          std::vector< BallAids3D::TrailPoint > at_eye = {
              { QVector3D( 0, 0.1f, 0 ), 0.0f }, { QVector3D( -1, 0.1f, 0 ), 1.0f }, { QVector3D( -2, 0.1f, 0 ), 2.0f } };
          BallAids3D::addTrail( c, at_eye, QVector3D( 10, 0.1f, 0 ), 0.6f, 3, Qt::white );
          QCOMPARE( c.ball_trail.size(), std::size_t( 24 ) );
          for ( const ColorVertex3D & v : c.ball_trail ) QVERIFY( std::isfinite( v.position.x() ) && std::isfinite( v.position.z() ) );
          QVERIFY( ( c.ball_trail[0].position - c.ball_trail[1].position ).length() > 0.2f );

          // one point: nothing
          c.clear();
          BallAids3D::addTrail( c, { points[0] }, eye, 0.6f, 5, Qt::white );
          QVERIFY( c.ball_trail.empty() );
      }
};

QTEST_MAIN( TestBallAids3D )

#include "test_ball_aids_3d.moc"
