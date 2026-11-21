// -*-c++-*-

/*!
  \file test_cameras_3d.cpp
  \brief unit test of the cameras of the 3D view (Cameras3D, CameraController, CameraMode)
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

#include "camera_mode.h"
#include "cameras_3d.h"
#include "options.h"

#include <QVector4D>

#include <cmath>

namespace {

constexpr float ASPECT = 16.0f / 9.0f;

//! where a point of the world is on the screen (-1 to 1, +y up)
QVector3D
toScreen( const CameraPose & pose,
          const float aspect,
          const QVector3D & p )
{
    const QVector4D clip = pose.projection( aspect ) * pose.view() * QVector4D( p, 1.0f );
    return clip.toVector3D() / clip.w();
}

//! the corners of the pitch and 2 m around it
const QVector3D CORNERS[] = {
    QVector3D( -54.5f, 0.0f, -36.0f ), QVector3D( 54.5f, 0.0f, -36.0f ),
    QVector3D( -54.5f, 0.0f, 36.0f ), QVector3D( 54.5f, 0.0f, 36.0f ),
};

bool
near( const QVector3D & a,
      const QVector3D & b,
      const float eps = 1.0e-3f )
{
    return ( a - b ).length() < eps;
}

//! the angle (radian) of the camera above the horizontal plane of its target
float
elevation( const CameraPose & pose )
{
    const QVector3D offset = pose.position - pose.target;
    return std::asin( offset.y() / offset.length() );
}

}

class TestCameras3D
    : public QObject {
    Q_OBJECT

private slots:

    void modes()
      {
          // the names in the options, both ways
          for ( const CameraModeInfo & m : cameraModes() )
          {
              CameraMode mode = CameraMode::Free;
              QVERIFY( cameraModeFromName( m.name, &mode ) );
              QCOMPARE( mode, m.mode );
              QCOMPARE( cameraModeName( m.mode ), QString( m.name ) );
          }
          // an unknown name (of a later version) leaves the camera as it was
          CameraMode mode = CameraMode::Top;
          QVERIFY( ! cameraModeFromName( "spectator", &mode ) );
          QCOMPARE( mode, CameraMode::Top );

          // E goes through all of them and back
          mode = CameraMode::Free;
          for ( std::size_t i = 1; i < cameraModes().size(); ++i )
          {
              mode = nextCameraMode( mode );
              QCOMPARE( mode, cameraModes()[i].mode );
          }
          QCOMPARE( nextCameraMode( mode ), CameraMode::Free );
          QCOMPARE( DEFAULT_CAMERA_MODE, CameraMode::Broadcast );

          // the close-up ones after the broadcast camera, as in the web version
          QCOMPARE( nextCameraMode( CameraMode::Broadcast ), CameraMode::CloseupBall );
          QCOMPARE( nextCameraMode( CameraMode::CloseupBall ), CameraMode::CloseupPlayer );
          QCOMPARE( nextCameraMode( CameraMode::CloseupPlayer ), CameraMode::Top );
          QVERIFY( isCloseup( CameraMode::CloseupBall ) );
          QVERIFY( isCloseup( CameraMode::CloseupPlayer ) );
          QVERIFY( ! isCloseup( CameraMode::Broadcast ) );
      }

    //! the close-up camera: the distance and the elevation from what it looks at, within the ground image
    void closeup()
      {
          const CameraPose pose = Cameras3D::closeup( 10.0f, -5.0f, 1.0f, 12.0f, 20.0f );
          QVERIFY( near( pose.target, QVector3D( 10.0f, 1.0f, -5.0f ) ) );
          QVERIFY( std::fabs( ( pose.position - pose.target ).length() - 12.0f ) < 1.0e-4f );
          QVERIFY( std::fabs( elevation( pose ) - 20.0f * float( M_PI ) / 180.0f ) < 1.0e-5f );
          // from the side of the near touch line (+y of the field)
          QCOMPARE( pose.position.x(), pose.target.x() );
          QVERIFY( pose.position.z() > pose.target.z() );
          QCOMPARE( pose.fov_y, Cameras3D::FOV );
          QCOMPARE( pose.ortho_half_height, 0.0f );

          // what it looks at stays over the ground image
          const CameraPose out = Cameras3D::closeup( 80.0f, -60.0f, 0.0f, 12.0f, 20.0f );
          QCOMPARE( out.target.x(), float( Options::PITCH_HALF_LENGTH + Options::PITCH_MARGIN ) );
          QCOMPARE( out.target.z(), -float( Options::PITCH_HALF_WIDTH + Options::PITCH_MARGIN ) );

          // nearer, closer behind
          QVERIFY( std::fabs( Cameras3D::closeupTimeConstant( 12.0f ) - 0.0089f * 12.0f ) < 1.0e-6f );
          QCOMPARE( Cameras3D::closeupTimeConstant( 3.0f ), Cameras3D::CLOSEUP_MIN_TIME_CONSTANT );
          QCOMPARE( Cameras3D::closeupTimeConstant( 60.0f ), Cameras3D::CLOSEUP_MAX_TIME_CONSTANT );
      }

    //! the close-up cameras follow the ball or the selected player, closer behind
    void controllerCloseup()
      {
          CameraController camera;
          camera.setMode( CameraMode::CloseupBall );
          const float d = float( Options::CLOSEUP_DISTANCE_3D );
          const float e = float( Options::CLOSEUP_ELEVATION_3D );
          QCOMPARE( camera.closeupDistance(), d );

          CameraSubject subject;
          subject.ball_x = 5.0f;
          subject.ball_y = 2.0f;
          subject.has_player = true;
          subject.player_x = -20.0f;
          subject.player_y = 10.0f;
          CameraPose pose = camera.update( 0.0, ASPECT, subject );
          QVERIFY( near( pose.position, Cameras3D::closeup( 5.0f, 2.0f, 0.0f, d, e ).position ) );

          // after the ball, by the close-up's time constant
          subject.ball_x = 15.0f;
          pose = camera.update( 0.05, ASPECT, subject );
          const float k = 1.0f - std::exp( -0.05f / Cameras3D::closeupTimeConstant( d ) );
          QVERIFY( std::fabs( pose.target.x() - ( 5.0f + 10.0f * k ) ) < 1.0e-3f );

          // the selected player's: at once to its middle; the ball's without one
          camera.setMode( CameraMode::CloseupPlayer );
          pose = camera.update( 0.016, ASPECT, subject );
          QVERIFY( near( pose.target, QVector3D( -20.0f, Cameras3D::CLOSEUP_PLAYER_HEIGHT, 10.0f ) ) );
          subject.has_player = false;
          pose = camera.update( 0.05, ASPECT, subject ); // on the way
          QVERIFY( ! camera.settled() );
          for ( int i = 0; i < 200 && ! camera.settled(); ++i )
          {
              pose = camera.update( 0.05, ASPECT, subject );
          }
          QVERIFY( camera.settled() );
          QVERIFY( near( pose.target, QVector3D( 15.0f, 0.0f, 2.0f ) ) );

          // the keys change its distance, within the range, not the zoom
          camera.zoom( 1.0f / 1.5f );
          QVERIFY( std::fabs( camera.closeupDistance() - d / 1.5f ) < 1.0e-5f );
          QCOMPARE( camera.zoomFactor(), 1.0f );
          QVERIFY( ! camera.settled() );
          for ( int i = 0; i < 200 && ! camera.settled(); ++i )
          {
              pose = camera.update( 0.05, ASPECT, subject );
          }
          QVERIFY( std::fabs( ( pose.position - pose.target ).length() - d / 1.5f ) < 1.0e-3f );
          camera.zoom( 0.0001f );
          QCOMPARE( camera.closeupDistance(), float( Options::MIN_CLOSEUP_DISTANCE_3D ) );
          camera.zoom( 100000.0f );
          QCOMPARE( camera.closeupDistance(), float( Options::MAX_CLOSEUP_DISTANCE_3D ) );

          // a setting: it stays with another camera, and I sets it back to the default
          camera.setMode( CameraMode::Broadcast );
          camera.setMode( CameraMode::CloseupBall );
          QCOMPARE( camera.closeupDistance(), float( Options::MAX_CLOSEUP_DISTANCE_3D ) );
          camera.resetZoom();
          QCOMPARE( camera.closeupDistance(), d );

          // a new distance and elevation glide there
          for ( int i = 0; i < 200 && ! camera.settled(); ++i )
          {
              pose = camera.update( 0.05, ASPECT, subject );
          }
          camera.setCloseup( 30.0f, 60.0f );
          QVERIFY( ! camera.settled() );
          for ( int i = 0; i < 200 && ! camera.settled(); ++i )
          {
              pose = camera.update( 0.05, ASPECT, subject );
          }
          QVERIFY( std::fabs( ( pose.position - pose.target ).length() - 30.0f ) < 1.0e-3f );
          QVERIFY( std::fabs( elevation( pose ) - 60.0f * float( M_PI ) / 180.0f ) < 1.0e-4f );
      }

    //! the options keep the close-up settings within their ranges
    void closeupOptions()
      {
          Options & opt = Options::instance();
          opt.setCloseupDistance3D( 1.0 );
          QCOMPARE( opt.closeupDistance3D(), Options::MIN_CLOSEUP_DISTANCE_3D );
          opt.setCloseupDistance3D( 100.0 );
          QCOMPARE( opt.closeupDistance3D(), Options::MAX_CLOSEUP_DISTANCE_3D );
          opt.setCloseupDistance3D( 20.0 );
          QCOMPARE( opt.closeupDistance3D(), 20.0 );
          opt.setCloseupElevation3D( 0.0 );
          QCOMPARE( opt.closeupElevation3D(), Options::MIN_CLOSEUP_ELEVATION_3D );
          opt.setCloseupElevation3D( 90.0 );
          QCOMPARE( opt.closeupElevation3D(), Options::MAX_CLOSEUP_ELEVATION_3D );
          opt.setCloseupDistance3D( Options::CLOSEUP_DISTANCE_3D );
          opt.setCloseupElevation3D( Options::CLOSEUP_ELEVATION_3D );
      }

    void fitView_data()
      {
          QTest::addColumn< float >( "azimuth" );
          QTest::addColumn< float >( "elevation" );
          QTest::addColumn< float >( "aspect" );
          // the viewpoints of the whole pitch, in three shapes
          for ( const float aspect : { 16.0f / 9.0f, 4.0f / 3.0f, 21.0f / 9.0f } )
          {
              const QByteArray shape = QByteArray::number( aspect, 'f', 2 ) + " ";
              for ( const ViewpointInfo & v : viewpoints() )
              {
                  QTest::newRow( shape + v.label ) << v.azimuth << v.elevation << aspect;
              }
          }
      }

    void fitView()
      {
          QFETCH( float, azimuth );
          QFETCH( float, elevation );
          QFETCH( float, aspect );
          const CameraPose pose = Cameras3D::fitView( azimuth, elevation, aspect );

          // looking at the center from the direction
          QVERIFY( near( pose.target, QVector3D() ) );
          QCOMPARE( pose.fov_y, Cameras3D::FOV );
          QVERIFY( std::fabs( ::elevation( pose ) - elevation * float( M_PI ) / 180.0f ) < 1.0e-4f );

          // every corner within 97% across and 85% up and down, and one at the edge
          float edge = 0.0f;
          for ( const QVector3D & c : CORNERS )
          {
              const QVector3D s = toScreen( pose, aspect, c );
              QVERIFY2( std::fabs( s.x() ) <= 0.97f + 1.0e-4f, qPrintable( QString::number( s.x() ) ) );
              QVERIFY2( std::fabs( s.y() ) <= 0.85f + 1.0e-4f, qPrintable( QString::number( s.y() ) ) );
              edge = std::max( { edge, std::fabs( s.x() ) / 0.97f, std::fabs( s.y() ) / 0.85f } );
          }
          QVERIFY2( std::fabs( edge - 1.0f ) < 1.0e-3f, qPrintable( QString::number( edge ) ) );
      }

    //! the viewpoints of the web version, in the order of Ctrl+I
    void viewpointList()
      {
          QCOMPARE( viewpoints().size(), std::size_t( 9 ) );
          QCOMPARE( viewpoints()[0].viewpoint, Viewpoint::Side );
          // the first is where the free camera starts: from the side, as high as the broadcast camera
          QCOMPARE( viewpoints()[0].azimuth, 90.0f );
          QCOMPARE( viewpoints()[0].elevation, Cameras3D::ELEVATION );
          QCOMPARE( viewpointInfo( Viewpoint::High ).elevation, 62.0f );
          QCOMPARE( viewpointInfo( Viewpoint::FarRight ).azimuth, -45.0f );
          QStringList labels;
          for ( const ViewpointInfo & v : viewpoints() ) labels << v.label;
          QCOMPARE( labels.removeDuplicates(), 0 );

          // Ctrl+I goes through all of them and back
          Viewpoint v = Viewpoint::Side;
          for ( std::size_t i = 1; i < viewpoints().size(); ++i )
          {
              v = nextViewpoint( v );
              QCOMPARE( v, viewpoints()[i].viewpoint );
          }
          QCOMPARE( nextViewpoint( v ), Viewpoint::Side );

          // the camera of a viewpoint fits the pitch from its angles
          const CameraPose pose = Cameras3D::viewpoint( Viewpoint::NearLeft, ASPECT );
          QVERIFY( near( pose.position, Cameras3D::fitView( 135.0f, 35.0f, ASPECT ).position ) );
      }

    /*!
      A viewpoint makes the camera the free camera, there in the shape of
      the view at the next update, from where it stays and the mouse moves it
    */
    void controllerViewFrom()
      {
          CameraController camera;
          camera.update( 0.0, ASPECT, 0.0f, 0.0f );
          camera.zoom( 0.5f );
          camera.viewFrom( Viewpoint::High );
          QCOMPARE( camera.mode(), CameraMode::Free );
          QCOMPARE( camera.zoomFactor(), 1.0f );

          const float aspect = 4.0f / 3.0f;
          CameraPose pose = camera.update( 0.016, aspect, 10.0f, 0.0f );
          const CameraPose high = Cameras3D::viewpoint( Viewpoint::High, aspect );
          QVERIFY( near( pose.position, high.position ) );
          QVERIFY( near( pose.target, high.target ) );
          QVERIFY( camera.settled() );
          // it stays, whatever the ball does
          QVERIFY( near( camera.update( 0.016, aspect, -30.0f, 20.0f ).position, high.position ) );
          // the mouse moves it on, still the free camera
          QVERIFY( ! camera.orbit( 40.0f, 0.0f, 600.0f ) );
          pose = camera.update( 0.016, aspect, 0.0f, 0.0f );
          QVERIFY( ! near( pose.position, high.position ) );
          QVERIFY( near( pose.target, high.target ) );

          // from the top camera too, and again
          camera.setMode( CameraMode::Top );
          camera.update( 0.016, aspect, 0.0f, 0.0f );
          camera.viewFrom( Viewpoint::LeftEnd );
          pose = camera.update( 0.016, aspect, 0.0f, 0.0f );
          QCOMPARE( pose.ortho_half_height, 0.0f );
          QVERIFY( near( pose.position, Cameras3D::viewpoint( Viewpoint::LeftEnd, aspect ).position ) );
          QVERIFY( pose.position.x() < -60.0f ); // behind the left goal
      }

    void fitViewFromTheSide()
      {
          // the closed form of the web version's fitCameraPosition()
          const float tan_v = std::tan( 17.5f * float( M_PI ) / 180.0f ), tan_h = tan_v * ASPECT;
          const float e = 32.0f * float( M_PI ) / 180.0f, a = 54.5f, b = 36.0f;
          const float distance = std::max( a / ( 0.97f * tan_h ) + b * std::cos( e ),
                                           b * std::sin( e ) / ( 0.85f * tan_v ) + b * std::cos( e ) );
          const CameraPose pose = Cameras3D::fitView( 90.0f, 32.0f, ASPECT );
          QVERIFY( near( pose.position, QVector3D( 0.0f, distance * std::sin( e ), distance * std::cos( e ) ) ) );
      }

    void broadcast()
      {
          const float full = Cameras3D::fitView( 90.0f, 32.0f, ASPECT ).position.length();
          CameraPose pose = Cameras3D::broadcast( ASPECT, 10.0f, -5.0f );
          // looking at the ball from the near side, nearer than the whole pitch
          QVERIFY( near( pose.target, QVector3D( 10.0f, 0.0f, -5.0f ) ) );
          QVERIFY( std::fabs( ( pose.position - pose.target ).length() - full * Cameras3D::BROADCAST_ZOOM ) < 1.0e-3f );
          QVERIFY( std::fabs( elevation( pose ) - 32.0f * float( M_PI ) / 180.0f ) < 1.0e-4f );
          QCOMPARE( pose.position.x(), 10.0f );
          QVERIFY( pose.position.z() > pose.target.z() );

          // not after the ball beyond 10 m inside the lines
          pose = Cameras3D::broadcast( ASPECT, -60.0f, 40.0f );
          QVERIFY( near( pose.target, QVector3D( -42.5f, 0.0f, 24.0f ) ) );
      }

    void goal()
      {
          CameraPose pose = Cameras3D::goal( -1, 20.0f, 10.0f );
          QVERIFY( near( pose.position, QVector3D( -70.5f, 10.0f, 0.0f ) ) );
          // halfway between the goal and the ball
          QVERIFY( near( pose.target, QVector3D( ( -52.5f + 20.0f ) / 2.0f, 0.0f, 6.0f ) ) );
          QCOMPARE( pose.fov_y, Cameras3D::GOAL_FOV );

          pose = Cameras3D::goal( 1, 80.0f, -50.0f );
          QVERIFY( near( pose.position, QVector3D( 70.5f, 10.0f, 0.0f ) ) );
          QVERIFY( near( pose.target, QVector3D( 52.5f, 0.0f, -34.0f * 0.6f ) ) );
      }

    void top()
      {
          for ( const float aspect : { 16.0f / 9.0f, 4.0f / 3.0f, 1.0f } )
          {
              const CameraPose pose = Cameras3D::top( aspect );
              QVERIFY( pose.ortho_half_height > 0.0f );
              // the ground image, within 85% of the height
              const float hx = float( Options::PITCH_HALF_LENGTH + Options::PITCH_MARGIN );
              const float hz = float( Options::PITCH_HALF_WIDTH + Options::PITCH_MARGIN );
              for ( const float x : { -hx, hx } )
              {
                  for ( const float z : { -hz, hz } )
                  {
                      const QVector3D s = toScreen( pose, aspect, QVector3D( x, 0.0f, z ) );
                      QVERIFY( std::fabs( s.x() ) <= 1.0f + 1.0e-5f );
                      QVERIFY( std::fabs( s.y() ) <= 0.85f + 1.0e-5f );
                  }
              }
              // as in 2D: the field's +x to the right, its -y up
              QVERIFY( toScreen( pose, aspect, QVector3D( 30.0f, 0.0f, 0.0f ) ).x() > 0.1f );
              QVERIFY( toScreen( pose, aspect, QVector3D( 0.0f, 0.0f, -20.0f ) ).y() > 0.1f );
          }
      }

    void smoothToward()
      {
          const QVector3D from( 0, 0, 0 ), to( 10, 0, 0 );
          QCOMPARE( Cameras3D::smoothToward( from, to, 0.0 ), from );
          // after the time constant, 1 - 1/e of the way
          QVERIFY( std::fabs( Cameras3D::smoothToward( from, to, Cameras3D::TIME_CONSTANT ).x() - 10.0f * ( 1.0f - std::exp( -1.0f ) ) ) < 1.0e-4f );
          QVERIFY( near( Cameras3D::smoothToward( from, to, 100.0 ), to ) );
          // two steps of dt go as far as one of 2 dt
          const QVector3D twice = Cameras3D::smoothToward( Cameras3D::smoothToward( from, to, 0.05 ), to, 0.05 );
          QVERIFY( near( twice, Cameras3D::smoothToward( from, to, 0.1 ) ) );
      }

    void orbit()
      {
          CameraPose pose = Cameras3D::fitView( 90.0f, 32.0f, ASPECT );
          const CameraPose start = pose;
          const float distance = pose.position.length();

          // around the target, at the same distance
          Cameras3D::orbit( pose, 100.0f, 30.0f, 800.0f );
          QVERIFY( near( pose.target, start.target ) );
          QVERIFY( std::fabs( pose.position.length() - distance ) < 1.0e-3f );
          QVERIFY( ! near( pose.position, start.position ) );
          // and back
          Cameras3D::orbit( pose, -100.0f, -30.0f, 800.0f );
          QVERIFY( near( pose.position, start.position, 1.0e-2f ) );

          // a drag across the height: once round
          Cameras3D::orbit( pose, 800.0f, 0.0f, 800.0f );
          QVERIFY( near( pose.position, start.position, 1.0e-2f ) );

          // dragged up far: down to the ground, but above it
          Cameras3D::orbit( pose, 0.0f, -5000.0f, 800.0f );
          QVERIFY( pose.position.y() > 0.0f );
          QVERIFY( std::fabs( elevation( pose ) - 0.03f ) < 1.0e-3f );
          // dragged down far: up to straight above, but not quite
          Cameras3D::orbit( pose, 0.0f, 5000.0f, 800.0f );
          QVERIFY( std::fabs( elevation( pose ) - ( float( M_PI / 2.0 ) - Cameras3D::MIN_POLAR ) ) < 1.0e-3f );
      }

    void pan()
      {
          CameraPose pose = Cameras3D::broadcast( ASPECT, 0.0f, 0.0f );
          const CameraPose start = pose;
          const float view_height = 2.0f * ( pose.position - pose.target ).length() * std::tan( pose.fov_y * float( M_PI ) / 360.0f );

          // a drag to the right across the height: the camera and its target
          // go left by the height of the view at the target
          Cameras3D::pan( pose, 800.0f, 0.0f, 800.0f );
          const QVector3D move = pose.target - start.target;
          QVERIFY( near( pose.position - start.position, move ) );
          QVERIFY( std::fabs( move.length() - view_height ) < 1.0e-2f );
          QVERIFY( move.x() < 0.0f ); // this camera's right is +x
          QVERIFY( std::fabs( move.y() ) < 1.0e-4f );

          // a drag down: the camera goes up the screen
          pose = start;
          Cameras3D::pan( pose, 0.0f, 100.0f, 800.0f );
          QVERIFY( pose.target.y() > start.target.y() );
          // the ground at the target stays at the center of the view: the
          // direction does not change
          QVERIFY( near( ( pose.target - pose.position ).normalized(), ( start.target - start.position ).normalized() ) );
      }

    void dolly()
      {
          CameraPose pose;
          pose.position = QVector3D( 0.0f, 60.0f, 80.0f ); // 100 m
          pose.target = QVector3D( 0.0f, 0.0f, 0.0f );
          const QVector3D direction = pose.position.normalized();

          Cameras3D::dolly( pose, 0.5f );
          QVERIFY( near( pose.position, direction * 50.0f ) );
          QCOMPARE( pose.target, QVector3D() );

          Cameras3D::dolly( pose, 0.001f );
          QVERIFY( near( pose.position, direction * Cameras3D::MIN_DISTANCE ) );
          Cameras3D::dolly( pose, 1000.0f );
          QVERIFY( near( pose.position, direction * Cameras3D::MAX_DISTANCE ) );
      }

    void controllerFollows()
      {
          CameraController camera;
          QCOMPARE( camera.mode(), CameraMode::Broadcast );

          // at once at the first frame
          CameraPose pose = camera.update( 0.0, ASPECT, 0.0f, 0.0f );
          QVERIFY( near( pose.target, Cameras3D::broadcast( ASPECT, 0.0f, 0.0f ).target ) );

          // after the ball: on the way, then there
          pose = camera.update( 0.016, ASPECT, 10.0f, 0.0f );
          QVERIFY( pose.target.x() > 0.0f && pose.target.x() < 1.0f );
          QVERIFY( ! camera.settled() );
          for ( int i = 0; i < 100 && ! camera.settled(); ++i )
          {
              pose = camera.update( 0.05, ASPECT, 10.0f, 0.0f );
          }
          QVERIFY( camera.settled() );
          QCOMPARE( pose.target, Cameras3D::broadcast( ASPECT, 10.0f, 0.0f ).target );

          // a long pause goes as far as MAX_STEP only
          pose = camera.update( 5.0, ASPECT, 20.0f, 0.0f );
          const float k = 1.0f - std::exp( -float( Cameras3D::MAX_STEP ) / Cameras3D::TIME_CONSTANT );
          QVERIFY( std::fabs( pose.target.x() - ( 10.0f + 10.0f * k ) ) < 1.0e-3f );

          // another camera: at once
          camera.setMode( CameraMode::GoalLeft );
          pose = camera.update( 0.016, ASPECT, 20.0f, 0.0f );
          QVERIFY( near( pose.position, Cameras3D::goal( -1, 20.0f, 0.0f ).position ) );
          QCOMPARE( pose.fov_y, Cameras3D::GOAL_FOV );
      }

    void controllerMouse()
      {
          CameraController camera;
          const CameraPose broadcast = camera.update( 0.0, ASPECT, 5.0f, 5.0f );

          // the mouse on a following camera: the free camera, from there
          QVERIFY( camera.takesMouse() );
          QVERIFY( camera.dolly( 0.5f ) );
          QCOMPARE( camera.mode(), CameraMode::Free );
          CameraPose pose = camera.update( 0.016, ASPECT, 30.0f, 0.0f );
          QVERIFY( near( pose.target, broadcast.target ) );
          QVERIFY( std::fabs( ( pose.position - pose.target ).length() - 0.5f * ( broadcast.position - broadcast.target ).length() ) < 1.0e-3f );
          // it stays, whatever the ball does
          QVERIFY( camera.settled() );
          QVERIFY( ! camera.orbit( 10.0f, 0.0f, 800.0f ) ); // already free
          QVERIFY( near( camera.update( 0.016, ASPECT, -30.0f, 0.0f ).target, broadcast.target ) );

          // the top camera does not take the mouse
          camera.setMode( CameraMode::Top );
          const CameraPose top = camera.update( 0.016, ASPECT, 0.0f, 0.0f );
          QVERIFY( top.ortho_half_height > 0.0f );
          QVERIFY( ! camera.takesMouse() );
          QVERIFY( ! camera.orbit( 100.0f, 100.0f, 800.0f ) );
          QCOMPARE( camera.mode(), CameraMode::Top );
          QCOMPARE( camera.update( 0.016, ASPECT, 0.0f, 0.0f ).position, top.position );

          // the free camera after the top one: the whole pitch, in perspective
          camera.setMode( CameraMode::Free );
          pose = camera.update( 0.016, ASPECT, 0.0f, 0.0f );
          QVERIFY( near( pose.position, Cameras3D::fitView( 90.0f, 32.0f, ASPECT ).position ) );
          QCOMPARE( pose.ortho_half_height, 0.0f );
          QCOMPARE( pose.up, QVector3D( 0.0f, 1.0f, 0.0f ) );
      }

    void controllerZoom()
      {
          // a following camera: nearer by the zoom, still after the ball
          CameraController camera;
          const CameraPose start = camera.update( 0.0, ASPECT, 0.0f, 0.0f );
          const float distance = ( start.position - start.target ).length();
          camera.zoom( 1.0f / 1.5f );
          QVERIFY( ! camera.settled() );
          CameraPose pose = start;
          for ( int i = 0; i < 200 && ! camera.settled(); ++i )
          {
              pose = camera.update( 0.05, ASPECT, 10.0f, 0.0f );
          }
          QVERIFY( camera.settled() );
          QVERIFY( std::fabs( ( pose.position - pose.target ).length() - distance / 1.5f ) < 1.0e-3f );
          QVERIFY( near( pose.target, QVector3D( 10.0f, 0.0f, 0.0f ) ) );
          QCOMPARE( camera.mode(), CameraMode::Broadcast );

          // within its limits
          camera.zoom( 0.0001f );
          QCOMPARE( camera.zoomFactor(), Cameras3D::MIN_ZOOM );
          camera.zoom( 100000.0f );
          QCOMPARE( camera.zoomFactor(), Cameras3D::MAX_ZOOM );

          // back to its own distance, and with another camera
          camera.resetZoom();
          QCOMPARE( camera.zoomFactor(), 1.0f );
          camera.zoom( 0.5f );
          camera.setMode( CameraMode::GoalRight );
          QCOMPARE( camera.zoomFactor(), 1.0f );
      }

    void topZoom()
      {
          CameraController camera;
          camera.setMode( CameraMode::Top );
          const CameraPose whole = camera.update( 0.0, ASPECT, 20.0f, 10.0f );
          QVERIFY( near( whole.target, QVector3D() ) );

          // half the height, around the ball
          camera.zoom( 0.5f );
          CameraPose pose = camera.update( 0.0, ASPECT, 10.0f, 5.0f );
          QVERIFY( std::fabs( pose.ortho_half_height - whole.ortho_half_height / 2.0f ) < 1.0e-4f );
          QVERIFY( near( pose.target, QVector3D( 10.0f, 0.0f, 5.0f ) ) );
          QVERIFY( near( pose.position - pose.target, QVector3D( 0.0f, 300.0f, 0.0f ) ) );

          // near a corner: within the ground image
          pose = camera.update( 0.0, ASPECT, 52.0f, 33.0f );
          const float hx = float( Options::PITCH_HALF_LENGTH + Options::PITCH_MARGIN );
          const float hz = float( Options::PITCH_HALF_WIDTH + Options::PITCH_MARGIN );
          QVERIFY( std::fabs( pose.target.x() + pose.ortho_half_height * ASPECT - hx ) < 1.0e-3f );
          QVERIFY( std::fabs( pose.target.z() + pose.ortho_half_height - hz ) < 1.0e-3f );
      }

    void freeZoom()
      {
          // the free camera: toward what it looks at
          CameraController camera;
          camera.setMode( CameraMode::Free );
          const CameraPose start = camera.update( 0.0, ASPECT, 0.0f, 0.0f );
          camera.zoom( 0.5f );
          const CameraPose pose = camera.update( 0.0, ASPECT, 0.0f, 0.0f );
          QVERIFY( near( pose.target, start.target ) );
          QVERIFY( std::fabs( ( pose.position - pose.target ).length() - 0.5f * ( start.position - start.target ).length() ) < 1.0e-3f );
          // back: the whole pitch
          camera.resetZoom();
          QVERIFY( near( camera.update( 0.0, ASPECT, 0.0f, 0.0f ).position, start.position ) );
      }

    void controllerFirstFree()
      {
          // the free camera from the start: the whole pitch in the shape of the view
          CameraController camera;
          camera.setMode( CameraMode::Free );
          const CameraPose pose = camera.update( 0.0, 4.0f / 3.0f, 0.0f, 0.0f );
          QVERIFY( near( pose.position, Cameras3D::fitView( 90.0f, 32.0f, 4.0f / 3.0f ).position ) );
      }
};

QTEST_MAIN( TestCameras3D )
#include "test_cameras_3d.moc"
