// -*-c++-*-

/*!
  \file cameras_3d.cpp
  \brief the cameras of the 3D view. Source File.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cameras_3d.h"

#include "options.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float DEG2RAD = float( M_PI / 180.0 );

float
clamp( const float v,
       const float lo,
       const float hi )
{
    return std::min( hi, std::max( lo, v ) );
}

// the pitch and the area of the ground image (m)
const float HALF_LENGTH = float( Options::PITCH_HALF_LENGTH );
const float HALF_WIDTH = float( Options::PITCH_HALF_WIDTH );
const float AREA_HALF_LENGTH = float( Options::PITCH_HALF_LENGTH + Options::PITCH_MARGIN );
const float AREA_HALF_WIDTH = float( Options::PITCH_HALF_WIDTH + Options::PITCH_MARGIN );

/*!
  \brief the camera's right and up, as QMatrix4x4::lookAt() makes them:
  right = forward x up, level for a level up
*/
void
camera_axes( const CameraPose & pose,
             QVector3D * right,
             QVector3D * up )
{
    const QVector3D forward = ( pose.target - pose.position ).normalized();
    *right = QVector3D::crossProduct( forward, pose.up ).normalized();
    *up = QVector3D::crossProduct( *right, forward );
}

}

namespace Cameras3D {

/*-------------------------------------------------------------------*/
CameraPose
fitView( const float azimuth,
         const float elevation,
         const float aspect )
{
    const float tan_v = std::tan( FOV * DEG2RAD / 2.0f );
    const float tan_h = tan_v * aspect;
    const float a = azimuth * DEG2RAD;
    const float e = elevation * DEG2RAD;

    // back: from the center toward the camera; f: forward, the other way;
    // r: right, level; u: up on the screen
    const QVector3D back( std::cos( e ) * std::cos( a ), std::sin( e ), std::cos( e ) * std::sin( a ) );
    const QVector3D f = -back;
    const QVector3D r = QVector3D( -f.z(), 0.0f, f.x() ) / std::cos( e );
    const QVector3D u = QVector3D::crossProduct( r, f );

    // With the camera at the distance D from the center, a corner c of the
    // ground is c.f + D in front of it, c.r across and c.u up, which do not
    // change with D. It is in the view if across / depth <= 0.97 tan h and
    // up / depth <= 0.85 tan v: D is the smallest that keeps all 4 corners.
    const float hx = HALF_LENGTH + 2.0f;
    const float hz = HALF_WIDTH + 2.0f;
    float distance = 0.0f;
    for ( const float x : { -hx, hx } )
    {
        for ( const float z : { -hz, hz } )
        {
            const QVector3D c( x, 0.0f, z );
            const float depth = QVector3D::dotProduct( c, f );
            distance = std::max( { distance,
                                   std::fabs( QVector3D::dotProduct( c, r ) ) / ( 0.97f * tan_h ) - depth,
                                   std::fabs( QVector3D::dotProduct( c, u ) ) / ( 0.85f * tan_v ) - depth } );
        }
    }

    CameraPose pose;
    pose.position = back * distance;
    pose.target = QVector3D( 0.0f, 0.0f, 0.0f );
    pose.fov_y = FOV;
    return pose;
}

/*-------------------------------------------------------------------*/
CameraPose
broadcast( const float aspect,
           const float ball_x,
           const float ball_y )
{
    const float distance = fitView( 90.0f, ELEVATION, aspect ).position.length() * BROADCAST_ZOOM;
    const float e = ELEVATION * DEG2RAD;
    const float tx = clamp( ball_x, -HALF_LENGTH + 10.0f, HALF_LENGTH - 10.0f );
    const float tz = clamp( ball_y, -HALF_WIDTH + 10.0f, HALF_WIDTH - 10.0f );

    CameraPose pose;
    pose.position = QVector3D( tx, distance * std::sin( e ), tz + distance * std::cos( e ) );
    pose.target = QVector3D( tx, 0.0f, tz );
    pose.fov_y = FOV;
    return pose;
}

/*-------------------------------------------------------------------*/
CameraPose
closeup( const float at_x,
         const float at_y,
         const float height,
         const float distance,
         const float elevation )
{
    const float e = elevation * DEG2RAD;
    const QVector3D target( clamp( at_x, -AREA_HALF_LENGTH, AREA_HALF_LENGTH ),
                            height,
                            clamp( at_y, -AREA_HALF_WIDTH, AREA_HALF_WIDTH ) );
    CameraPose pose;
    pose.target = target;
    // toward +z: the side of the near touch line (below in 2D)
    pose.position = target + QVector3D( 0.0f, distance * std::sin( e ), distance * std::cos( e ) );
    pose.fov_y = FOV;
    return pose;
}

/*-------------------------------------------------------------------*/
float
closeupTimeConstant( const float distance )
{
    return clamp( CLOSEUP_LAG * distance, CLOSEUP_MIN_TIME_CONSTANT, CLOSEUP_MAX_TIME_CONSTANT );
}

/*-------------------------------------------------------------------*/
CameraPose
goal( const int side,
      const float ball_x,
      const float ball_y )
{
    const float s = ( side < 0 ? -1.0f : 1.0f );
    CameraPose pose;
    pose.position = QVector3D( s * ( HALF_LENGTH + 18.0f ), 10.0f, 0.0f );
    pose.target = QVector3D( ( s * HALF_LENGTH + clamp( ball_x, -HALF_LENGTH, HALF_LENGTH ) ) / 2.0f,
                             0.0f,
                             clamp( ball_y, -HALF_WIDTH, HALF_WIDTH ) * 0.6f );
    pose.fov_y = GOAL_FOV;
    return pose;
}

/*-------------------------------------------------------------------*/
CameraPose
top( const float aspect )
{
    CameraPose pose;
    pose.position = QVector3D( 0.0f, 300.0f, 0.0f );
    pose.target = QVector3D( 0.0f, 0.0f, 0.0f );
    pose.up = QVector3D( 0.0f, 0.0f, -1.0f ); // the field's -y up on the screen, as in 2D
    pose.ortho_half_height = std::max( AREA_HALF_WIDTH / 0.85f, AREA_HALF_LENGTH / aspect );
    return pose;
}

/*-------------------------------------------------------------------*/
QVector3D
smoothToward( const QVector3D & from,
              const QVector3D & to,
              const double dt,
              const float tau )
{
    const float k = float( 1.0 - std::exp( -std::max( 0.0, dt ) / tau ) );
    return from + ( to - from ) * k;
}

/*-------------------------------------------------------------------*/
void
orbit( CameraPose & pose,
       const float dx,
       const float dy,
       const float height )
{
    // the camera on a sphere around the target: theta around y (from +z),
    // phi down from straight above
    const QVector3D offset = pose.position - pose.target;
    const float radius = offset.length();
    if ( radius <= 0.0f || height <= 0.0f )
    {
        return;
    }
    float theta = std::atan2( offset.x(), offset.z() );
    float phi = std::acos( clamp( offset.y() / radius, -1.0f, 1.0f ) );

    // a drag across the height of the view: once round
    theta -= 2.0f * float( M_PI ) * dx / height;
    phi -= 2.0f * float( M_PI ) * dy / height;
    phi = clamp( phi, MIN_POLAR, MAX_POLAR );

    pose.position = pose.target + radius * QVector3D( std::sin( phi ) * std::sin( theta ),
                                                      std::cos( phi ),
                                                      std::sin( phi ) * std::cos( theta ) );
}

/*-------------------------------------------------------------------*/
void
pan( CameraPose & pose,
     const float dx,
     const float dy,
     const float height )
{
    if ( height <= 0.0f )
    {
        return;
    }
    // the meters per pixel at the target: the view is 2 tan(fov / 2)
    // times the distance high
    const float meters_per_pixel = 2.0f * ( pose.position - pose.target ).length()
        * std::tan( pose.fov_y * DEG2RAD / 2.0f ) / height;
    QVector3D right, up;
    camera_axes( pose, &right, &up );
    // the camera moves against the drag, so that the ground follows the mouse
    const QVector3D move = ( -right * dx + up * dy ) * meters_per_pixel;
    pose.position += move;
    pose.target += move;
}

/*-------------------------------------------------------------------*/
void
dolly( CameraPose & pose,
       const float scale )
{
    const QVector3D offset = pose.position - pose.target;
    const float radius = offset.length();
    if ( radius <= 0.0f )
    {
        return;
    }
    pose.position = pose.target + offset * ( clamp( radius * scale, MIN_DISTANCE, MAX_DISTANCE ) / radius );
}

}

/*-------------------------------------------------------------------*/
CameraController::CameraController()
    : M_mode( DEFAULT_CAMERA_MODE ),
      M_fitted( false ),
      M_snap( true ),
      M_settled( false ),
      M_aspect( 16.0f / 9.0f ),
      M_zoom( 1.0f ),
      M_closeup_distance( float( Options::CLOSEUP_DISTANCE_3D ) ),
      M_closeup_elevation( float( Options::CLOSEUP_ELEVATION_3D ) )
{
    M_pose = Cameras3D::fitView( 90.0f, Cameras3D::ELEVATION, M_aspect );
}

/*-------------------------------------------------------------------*/
void
CameraController::setMode( const CameraMode mode )
{
    if ( mode == M_mode )
    {
        return;
    }
    // from above, the free camera starts with the whole pitch
    if ( mode == CameraMode::Free
         && M_mode == CameraMode::Top )
    {
        M_pose = Cameras3D::fitView( 90.0f, Cameras3D::ELEVATION, M_aspect );
    }
    M_mode = mode;
    M_snap = true;
    M_settled = false;
    M_zoom = 1.0f;
}

/*-------------------------------------------------------------------*/
void
CameraController::setCloseup( const float distance,
                              const float elevation )
{
    if ( distance != M_closeup_distance
         || elevation != M_closeup_elevation )
    {
        M_closeup_distance = distance;
        M_closeup_elevation = elevation;
        M_settled = false;
    }
}

/*-------------------------------------------------------------------*/
CameraPose
CameraController::update( const double dt,
                          const float aspect,
                          const CameraSubject & subject )
{
    const float ball_x = subject.ball_x;
    const float ball_y = subject.ball_y;
    M_aspect = aspect;
    // the first free camera sees the whole pitch, in the shape of the view
    if ( ! M_fitted )
    {
        M_fitted = true;
        M_pose = Cameras3D::fitView( 90.0f, Cameras3D::ELEVATION, aspect );
    }

    // the free camera: where the mouse put it, with the usual field of view
    if ( M_mode == CameraMode::Free )
    {
        M_pose.fov_y = Cameras3D::FOV;
        M_pose.up = QVector3D( 0.0f, 1.0f, 0.0f );
        M_pose.ortho_half_height = 0.0f;
        M_snap = false;
        M_settled = true;
        return M_pose;
    }

    // the top camera: fixed for the shape of the view; zoomed in, around
    // the ball, but within the ground image (as the 2D view with its focus on the ball)
    if ( M_mode == CameraMode::Top )
    {
        M_pose = Cameras3D::top( aspect );
        M_pose.ortho_half_height *= M_zoom;
        if ( M_zoom < 1.0f )
        {
            const float half_height = M_pose.ortho_half_height;
            const float room_x = std::max( 0.0f, AREA_HALF_LENGTH - half_height * aspect );
            const float room_z = std::max( 0.0f, AREA_HALF_WIDTH - half_height );
            const QVector3D center( clamp( ball_x, -room_x, room_x ), 0.0f, clamp( ball_y, -room_z, room_z ) );
            M_pose.target = center;
            M_pose.position = center + QVector3D( 0.0f, 300.0f, 0.0f );
        }
        M_snap = false;
        M_settled = true;
        return M_pose;
    }

    // the following cameras: where they go now
    CameraPose goal_pose;
    switch ( M_mode ) {
    case CameraMode::GoalLeft:
        goal_pose = Cameras3D::goal( -1, ball_x, ball_y );
        break;
    case CameraMode::GoalRight:
        goal_pose = Cameras3D::goal( 1, ball_x, ball_y );
        break;
    case CameraMode::CloseupPlayer:
        // the selected player's middle, or the ball without one
        goal_pose = ( subject.has_player
                      ? Cameras3D::closeup( subject.player_x, subject.player_y, Cameras3D::CLOSEUP_PLAYER_HEIGHT,
                                            M_closeup_distance, M_closeup_elevation )
                      : Cameras3D::closeup( ball_x, ball_y, 0.0f, M_closeup_distance, M_closeup_elevation ) );
        break;
    case CameraMode::CloseupBall:
        goal_pose = Cameras3D::closeup( ball_x, ball_y, 0.0f, M_closeup_distance, M_closeup_elevation );
        break;
    default:
        goal_pose = Cameras3D::broadcast( aspect, ball_x, ball_y );
        break;
    }
    // at the zoomed distance from what it looks at (the close-up cameras have their own distance)
    if ( M_zoom != 1.0f
         && ! isCloseup( M_mode ) )
    {
        Cameras3D::dolly( goal_pose, M_zoom );
    }

    // jump after a change of the camera, glide after the ball otherwise
    if ( M_snap )
    {
        M_snap = false;
        M_pose = goal_pose;
        M_settled = true;
        return M_pose;
    }
    // the close-up ones closer behind
    const double step = std::min( std::max( 0.0, dt ), Cameras3D::MAX_STEP );
    const float tau = ( isCloseup( M_mode )
                        ? Cameras3D::closeupTimeConstant( M_closeup_distance )
                        : Cameras3D::TIME_CONSTANT );
    const QVector3D position = Cameras3D::smoothToward( M_pose.position, goal_pose.position, step, tau );
    const QVector3D target = Cameras3D::smoothToward( M_pose.target, goal_pose.target, step, tau );
    M_settled = ( ( position - goal_pose.position ).length() < Cameras3D::SETTLED
                  && ( target - goal_pose.target ).length() < Cameras3D::SETTLED );
    // once still, exactly where it goes
    M_pose = goal_pose;
    if ( ! M_settled )
    {
        M_pose.position = position;
        M_pose.target = target;
    }
    return M_pose;
}

/*-------------------------------------------------------------------*/
void
CameraController::zoom( const float factor )
{
    if ( M_mode == CameraMode::Free )
    {
        Cameras3D::dolly( M_pose, factor );
    }
    else if ( isCloseup( M_mode ) )
    {
        M_closeup_distance = clamp( M_closeup_distance * factor,
                                    float( Options::MIN_CLOSEUP_DISTANCE_3D ),
                                    float( Options::MAX_CLOSEUP_DISTANCE_3D ) );
    }
    else
    {
        M_zoom = clamp( M_zoom * factor, Cameras3D::MIN_ZOOM, Cameras3D::MAX_ZOOM );
    }
    M_settled = false;
}

/*-------------------------------------------------------------------*/
void
CameraController::resetZoom()
{
    if ( M_mode == CameraMode::Free )
    {
        M_pose = Cameras3D::fitView( 90.0f, Cameras3D::ELEVATION, M_aspect );
    }
    else if ( isCloseup( M_mode ) )
    {
        M_closeup_distance = float( Options::CLOSEUP_DISTANCE_3D );
    }
    M_zoom = 1.0f;
    M_settled = false;
}

/*-------------------------------------------------------------------*/
bool
CameraController::takeMouse( bool * changed )
{
    *changed = false;
    if ( M_mode == CameraMode::Top )
    {
        return false;
    }
    if ( M_mode != CameraMode::Free )
    {
        // the free camera, from where the camera is
        M_mode = CameraMode::Free;
        M_pose.fov_y = Cameras3D::FOV;
        M_snap = false;
        *changed = true;
    }
    M_settled = true;
    return true;
}

/*-------------------------------------------------------------------*/
bool
CameraController::orbit( const float dx,
                         const float dy,
                         const float height )
{
    bool changed = false;
    if ( takeMouse( &changed ) )
    {
        Cameras3D::orbit( M_pose, dx, dy, height );
    }
    return changed;
}

/*-------------------------------------------------------------------*/
bool
CameraController::pan( const float dx,
                       const float dy,
                       const float height )
{
    bool changed = false;
    if ( takeMouse( &changed ) )
    {
        Cameras3D::pan( M_pose, dx, dy, height );
    }
    return changed;
}

/*-------------------------------------------------------------------*/
bool
CameraController::dolly( const float scale )
{
    bool changed = false;
    if ( takeMouse( &changed ) )
    {
        Cameras3D::dolly( M_pose, scale );
    }
    return changed;
}
