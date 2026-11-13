// -*-c++-*-

/*!
  \file test_player_camera.cpp
  \brief unit test of the selected player's camera of the 3D view
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

#include "cameras_3d.h"
#include "options.h"
#include "player_camera_3d.h"

#include <rcss/rcg/types.h>

#include <cmath>

using namespace PlayerCameraStyle;

namespace {

constexpr float ASPECT = 16.0f / 9.0f;

//! a player of the left team at (x, y)
rcss::rcg::PlayerT
makePlayer( const float x,
            const float y,
            const float body,
            const float neck,
            const float view_width = 90.0f )
{
    rcss::rcg::PlayerT p;
    p.side_ = 'l';
    p.unum_ = 7;
    p.state_ = rcss::rcg::STAND;
    p.x_ = x;
    p.y_ = y;
    p.body_ = body;
    p.neck_ = neck;
    p.view_width_ = view_width;
    return p;
}

//! the field direction (degree) that a camera looks in, on the ground
float
lookDirection( const CameraPose & pose )
{
    const QVector3D d = pose.target - pose.position;
    return std::atan2( d.z(), d.x() ) * 180.0f / float( M_PI );
}

bool
near( const QVector3D & a,
      const QVector3D & b,
      const float eps = 1.0e-3f )
{
    return ( a - b ).length() < eps;
}

}

/*!
  \brief unit test of PlayerCamera3D, PlayerCameraState and the player
  camera of CameraController
*/
class TestPlayerCamera
    : public QObject {
    Q_OBJECT

private slots:

    //! the direction it follows: the face, the body, or between
    void reference()
      {
          const rcss::rcg::PlayerT p = makePlayer( 0, 0, 30.0f, 60.0f );
          QCOMPARE( PlayerCamera3D::referenceDirection( p, Direction::Head ), 90.0f );
          QCOMPARE( PlayerCamera3D::referenceDirection( p, Direction::Body ), 30.0f );
          QCOMPARE( PlayerCamera3D::referenceDirection( p, Direction::Middle ), 60.0f );
          // without the neck, the body
          rcss::rcg::PlayerT no_neck = p;
          no_neck.neck_ = rcss::rcg::SHOWINFO_SCALE2F;
          QCOMPARE( PlayerCamera3D::referenceDirection( no_neck, Direction::Head ), 30.0f );
      }

    //! the view width as a field of view: 40 to 120, 90 without one
    void viewFov()
      {
          QCOMPARE( PlayerCamera3D::viewFov( makePlayer( 0, 0, 0, 0, 60.0f ) ), 60.0f );
          QCOMPARE( PlayerCamera3D::viewFov( makePlayer( 0, 0, 0, 0, 120.0f ) ), 120.0f );
          QCOMPARE( PlayerCamera3D::viewFov( makePlayer( 0, 0, 0, 0, 180.0f ) ), 120.0f );
          QCOMPARE( PlayerCamera3D::viewFov( makePlayer( 0, 0, 0, 0, 20.0f ) ), 40.0f );
          QCOMPARE( PlayerCamera3D::viewFov( makePlayer( 0, 0, 0, 0, rcss::rcg::SHOWINFO_SCALE2F ) ), 90.0f );
      }

    //! the short way round, with the time constant, never faster than the largest turn
    void turnToward()
      {
          // at once without a time constant, within the largest turn
          QCOMPARE( PlayerCamera3D::turnToward( 0.0f, 30.0f, 1.0, 0.0, 90.0 ), 30.0f );
          // the short way round, through 180
          QVERIFY( std::fabs( PlayerCamera3D::turnToward( 170.0f, -170.0f, 1.0, 0.0, 90.0 ) - ( -170.0f ) ) < 1.0e-4f );
          // ... and by 4.5 degrees in 0.05 s at 90 a second: toward 180, not back
          QVERIFY( std::fabs( PlayerCamera3D::turnToward( 170.0f, -170.0f, 0.05, 0.0, 90.0 ) - 174.5f ) < 1.0e-3f );
          // the time constant: 1 - 1/e of the way after it
          const float k = 1.0f - std::exp( -1.0f );
          QVERIFY( std::fabs( PlayerCamera3D::turnToward( 0.0f, 20.0f, 0.5, 0.5, 3600.0 ) - 20.0f * k ) < 1.0e-3f );
          // never faster than the largest turn: a neck turned by 180 at once
          float d = 0.0f;
          for ( int i = 0; i < 100; ++i )
          {
              const float next = PlayerCamera3D::turnToward( d, 180.0f - 1.0f, 1.0 / 60.0, 0.5, 90.0 );
              QVERIFY2( std::fabs( PlayerCamera3D::normalizeAngle( next - d ) ) <= 90.0f / 60.0f + 1.0e-4f,
                        qPrintable( QString::number( next - d ) ) );
              d = next;
          }
          // 100 frames of 1/60 s at 90 degrees a second: 150 at most
          QVERIFY( d <= 150.0f + 1.0e-3f );
          QVERIFY( d > 100.0f );

          QCOMPARE( PlayerCamera3D::easeToward( 60.0f, 120.0f, 1.0, 0.0 ), 120.0f );
          QVERIFY( std::fabs( PlayerCamera3D::easeToward( 60.0f, 120.0f, 0.5, 0.5 ) - ( 60.0f + 60.0f * k ) ) < 1.0e-3f );
      }

    //! at the eyes, or over the shoulder, as wide as asked across the view
    void pose()
      {
          CameraPose eye = PlayerCamera3D::pose( 10.0f, 5.0f, Position::Eye, 90.0f, 120.0f, ASPECT );
          QVERIFY( near( eye.position, QVector3D( 10.0f, PlayerCamera3D::EYE_HEIGHT, 5.0f ) ) );
          QVERIFY( std::fabs( lookDirection( eye ) - 90.0f ) < 1.0e-3f );
          QCOMPARE( eye.ortho_half_height, 0.0f );
          // 120 degrees across a 16:9 view
          const float expected = 2.0f * std::atan( std::tan( 60.0f * float( M_PI ) / 180.0f ) / ASPECT ) * 180.0f / float( M_PI );
          QVERIFY( std::fabs( eye.fov_y - expected ) < 1.0e-3f );

          const CameraPose shoulder = PlayerCamera3D::pose( 10.0f, 5.0f, Position::Shoulder, 0.0f, 120.0f, ASPECT );
          QVERIFY( near( shoulder.position, QVector3D( 10.0f - PlayerCamera3D::SHOULDER_BEHIND, PlayerCamera3D::SHOULDER_HEIGHT, 5.0f ) ) );
          QVERIFY( near( shoulder.target, QVector3D( 10.0f + PlayerCamera3D::SHOULDER_AHEAD, PlayerCamera3D::SHOULDER_TARGET_HEIGHT, 5.0f ) ) );
      }

    /*!
      The state: at once at first, then Smooth turns no faster than the
      largest turn and settles there; Instant follows at once; Eased eases
      the field of view; another player or a jump starts again at once.
    */
    void state()
      {
          PlayerCameraOptions o;
          o.motion = Motion::Smooth;
          o.fov = Fov::Fixed;
          PlayerCameraState camera;
          bool settled = false;
          rcss::rcg::PlayerT p = makePlayer( 0, 0, 0.0f, 0.0f );
          camera.update( p, ASPECT, o, 0.0, &settled );
          QCOMPARE( camera.direction(), 0.0f );
          QCOMPARE( camera.fov(), float( o.fixed_fov ) );
          QVERIFY( settled );

          // the neck turns by 90 at once: the camera at most 90 degrees a second
          p.neck_ = 90.0f;
          float last = camera.direction();
          for ( int i = 0; i < 30; ++i )
          {
              camera.update( p, ASPECT, o, 1.0 / 60.0, &settled );
              QVERIFY( camera.direction() - last <= 90.0f / 60.0f + 1.0e-4f );
              last = camera.direction();
          }
          QVERIFY( ! settled );
          for ( int i = 0; i < 600 && ! settled; ++i )
          {
              camera.update( p, ASPECT, o, 1.0 / 60.0, &settled );
          }
          QVERIFY( settled );
          QCOMPARE( camera.direction(), 90.0f );

          // Instant: at once
          o.motion = Motion::Instant;
          p.neck_ = -90.0f;
          camera.update( p, ASPECT, o, 1.0 / 60.0, &settled );
          QCOMPARE( camera.direction(), -90.0f );

          // Eased: the field of view toward the view width
          o.fov = Fov::Eased;
          p.view_width_ = 60.0f;
          camera.update( p, ASPECT, o, 0.1, &settled );
          QVERIFY( camera.fov() < float( o.fixed_fov ) && camera.fov() > 60.0f );
          QVERIFY( ! settled );
          o.fov = Fov::View;
          camera.update( p, ASPECT, o, 0.1, &settled );
          QCOMPARE( camera.fov(), 60.0f );

          // another player, and a jump: at once, also with Smooth
          o.motion = Motion::Smooth;
          rcss::rcg::PlayerT other = p;
          other.unum_ = 8;
          other.neck_ = 90.0f;
          camera.update( other, ASPECT, o, 1.0 / 60.0, &settled );
          QCOMPARE( camera.direction(), 90.0f );
          other.x_ = 10.0f;
          other.neck_ = -90.0f;
          camera.update( other, ASPECT, o, 1.0 / 60.0, &settled );
          QCOMPARE( camera.direction(), -90.0f );
          // a step it can make: smoothly
          other.x_ = 11.0f;
          other.neck_ = 90.0f;
          camera.update( other, ASPECT, o, 1.0 / 60.0, &settled );
          QVERIFY( camera.direction() < -80.0f );

          // after reset(), at once
          camera.reset();
          camera.update( other, ASPECT, o, 1.0 / 60.0, &settled );
          QCOMPARE( camera.direction(), 90.0f );
      }

    /*!
      The player camera of CameraController: the selected player's, in its
      styles; the broadcast camera without one; no mouse
    */
    void controller()
      {
          CameraController camera;
          camera.setMode( CameraMode::Player );
          QVERIFY( ! camera.takesMouse() );
          QVERIFY( ! camera.orbit( 50.0f, 0.0f, 600.0f ) );
          QCOMPARE( camera.mode(), CameraMode::Player );

          PlayerCameraOptions o;
          o.position = Position::Eye;
          o.fov = Fov::Fixed;
          o.fixed_fov = 100.0;
          camera.setPlayerCamera( o );

          const rcss::rcg::PlayerT p = makePlayer( -20.0f, 10.0f, 45.0f, 0.0f );
          CameraSubject subject;
          subject.ball_x = 5.0f;
          subject.has_player = true;
          subject.player_x = p.x_;
          subject.player_y = p.y_;
          subject.player = &p;
          CameraPose pose = camera.update( 0.016, ASPECT, subject );
          const CameraPose expected = PlayerCamera3D::pose( -20.0f, 10.0f, Position::Eye, 45.0f, 100.0f, ASPECT );
          QVERIFY( near( pose.position, expected.position ) );
          QVERIFY( near( pose.target, expected.target ) );
          QVERIFY( std::fabs( pose.fov_y - expected.fov_y ) < 1.0e-3f );
          QVERIFY( camera.settled() );

          // a change of the styles: at once in the new ones, also in the
          // middle of a smooth turn
          rcss::rcg::PlayerT turned = p;
          turned.neck_ = 90.0f;
          subject.player = &turned;
          camera.update( 0.016, ASPECT, subject );
          QVERIFY( camera.playerCameraState().direction() < 50.0f ); // turning toward 135
          o.position = Position::Shoulder;
          camera.setPlayerCamera( o );
          QVERIFY( ! camera.settled() );
          pose = camera.update( 0.016, ASPECT, subject );
          QCOMPARE( camera.playerCameraState().direction(), 135.0f );
          QVERIFY( near( pose.position, PlayerCamera3D::pose( -20.0f, 10.0f, Position::Shoulder, 135.0f, 100.0f, ASPECT ).position ) );

          // without a selected player: the broadcast camera
          subject.player = nullptr;
          subject.has_player = false;
          camera.setMode( CameraMode::Broadcast );
          const CameraPose broadcast = camera.update( 0.0, ASPECT, subject );
          camera.setMode( CameraMode::Player );
          pose = camera.update( 0.0, ASPECT, subject );
          QVERIFY( near( pose.position, broadcast.position ) );
      }

    //! the options keep the numbers within their ranges, and the names go both ways
    void options()
      {
          Options & opt = Options::instance();
          const PlayerCameraOptions saved = opt.playerCamera();
          PlayerCameraOptions o;
          QCOMPARE( o.position, Position::Shoulder );
          QCOMPARE( o.direction, Direction::Head );
          QCOMPARE( o.motion, Motion::Smooth );
          QCOMPARE( o.fov, Fov::Fixed );
          QCOMPARE( o.sight, Sight::Server );
          o.time_constant = -1.0;
          o.max_turn = 10000.0;
          o.fixed_fov = 5.0;
          opt.setPlayerCamera( o );
          QCOMPARE( opt.playerCamera().time_constant, MIN_TIME_CONSTANT );
          QCOMPARE( opt.playerCamera().max_turn, MAX_MAX_TURN );
          QCOMPARE( opt.playerCamera().fixed_fov, MIN_FIXED_FOV );
          opt.setPlayerCamera( saved );

          for ( const auto & i : sights() )
          {
              Sight s = Sight::All;
              QVERIFY( fromName( sights(), QString( i.name ), &s ) );
              QCOMPARE( s, i.value );
              QCOMPARE( nameOf( sights(), i.value ), QString( i.name ) );
          }
          Fov f = Fov::View;
          QVERIFY( ! fromName( fovs(), "wide", &f ) );
          QCOMPARE( f, Fov::View );
          QCOMPARE( nameOf( positions(), Position::Eye ), QString( "eye" ) );
          QCOMPARE( nameOf( directions(), Direction::Middle ), QString( "middle" ) );
          QCOMPARE( nameOf( motions(), Motion::Instant ), QString( "instant" ) );
          QCOMPARE( nameOf( fovs(), Fov::Eased ), QString( "eased" ) );
      }
};

QTEST_MAIN( TestPlayerCamera )
#include "test_player_camera.moc"
