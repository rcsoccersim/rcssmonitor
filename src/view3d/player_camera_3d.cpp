// -*-c++-*-

/*!
  \file player_camera_3d.cpp
  \brief the selected player's camera of the 3D view. Source File.
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

#include "player_camera_3d.h"

#include "interpolate.h"

#include <rcss/rcg/types.h>

#include <algorithm>
#include <cmath>

namespace PlayerCamera3D {

namespace {

constexpr float DEG2RAD = float( M_PI / 180.0 );

float
clamp( const float v,
       const float lo,
       const float hi )
{
    return std::min( hi, std::max( lo, v ) );
}

}

/*-------------------------------------------------------------------*/
float
normalizeAngle( const float degree )
{
    float d = std::fmod( degree + 180.0f, 360.0f );
    if ( d < 0.0f ) d += 360.0f;
    return d - 180.0f;
}

/*-------------------------------------------------------------------*/
float
referenceDirection( const rcss::rcg::PlayerT & player,
                    const PlayerCameraStyle::Direction direction )
{
    const float neck = ( player.hasNeck() ? player.neck_ : 0.0f );
    switch ( direction ) {
    case PlayerCameraStyle::Direction::Body:
        return player.body_;
    case PlayerCameraStyle::Direction::Middle:
        return player.body_ + neck * 0.5f;
    default:
        return player.body_ + neck;
    }
}

/*-------------------------------------------------------------------*/
float
viewFov( const rcss::rcg::PlayerT & player )
{
    const float width = ( player.hasView() ? std::fabs( player.view_width_ ) : 0.0f );
    return clamp( width > 0.0f ? width : 90.0f, 40.0f, 120.0f );
}

/*-------------------------------------------------------------------*/
float
turnToward( const float from,
            const float to,
            const double dt,
            const double time_constant,
            const double max_turn )
{
    const float diff = normalizeAngle( to - from );
    const double t = std::max( 0.0, dt );
    const double k = ( time_constant > 0.0 ? 1.0 - std::exp( -t / time_constant ) : 1.0 );
    const float cap = float( max_turn * t );
    return normalizeAngle( from + clamp( float( diff * k ), -cap, cap ) );
}

/*-------------------------------------------------------------------*/
float
easeToward( const float from,
            const float to,
            const double dt,
            const double time_constant )
{
    const double k = ( time_constant > 0.0 ? 1.0 - std::exp( -std::max( 0.0, dt ) / time_constant ) : 1.0 );
    return float( from + ( to - from ) * k );
}

/*-------------------------------------------------------------------*/
CameraPose
pose( const float x,
      const float y,
      const PlayerCameraStyle::Position position,
      const float direction,
      const float fov,
      const float aspect )
{
    const float c = std::cos( direction * DEG2RAD );
    const float s = std::sin( direction * DEG2RAD );
    // the horizontal field of view across the width, as a vertical one
    const float half = fov * 0.5f * DEG2RAD;
    CameraPose p;
    p.fov_y = 2.0f * std::atan( std::tan( half ) / std::max( 0.1f, aspect ) ) / DEG2RAD;
    if ( position == PlayerCameraStyle::Position::Shoulder )
    {
        p.position = QVector3D( x - SHOULDER_BEHIND * c, SHOULDER_HEIGHT, y - SHOULDER_BEHIND * s );
        p.target = QVector3D( x + SHOULDER_AHEAD * c, SHOULDER_TARGET_HEIGHT, y + SHOULDER_AHEAD * s );
    }
    else
    {
        p.position = QVector3D( x, EYE_HEIGHT, y );
        p.target = QVector3D( x + EYE_AHEAD * c, EYE_TARGET_HEIGHT, y + EYE_AHEAD * s );
    }
    return p;
}

}

/*-------------------------------------------------------------------*/
CameraPose
PlayerCameraState::update( const rcss::rcg::PlayerT & player,
                           const float aspect,
                           const PlayerCameraOptions & o,
                           const double dt,
                           bool * settled )
{
    using namespace PlayerCamera3D;
    using PlayerCameraStyle::Fov;

    const float goal = referenceDirection( player, o.direction );
    const float goal_fov = ( o.fov == Fov::Fixed ? float( o.fixed_fov ) : viewFov( player ) );
    const bool jumped = std::hypot( player.x_ - M_x, player.y_ - M_y ) > Interpolate::MAX_PLAYER_STEP;
    if ( ! M_started
         || player.side_ != M_side
         || player.unum_ != M_unum
         || jumped )
    {
        M_direction = goal;
        M_fov = goal_fov;
        M_started = true;
    }
    else
    {
        M_direction = ( o.motion == PlayerCameraStyle::Motion::Smooth
                        ? turnToward( M_direction, goal, dt, o.time_constant, o.max_turn )
                        : goal );
        M_fov = ( o.fov == Fov::Eased
                  ? easeToward( M_fov, goal_fov, dt, o.time_constant )
                  : goal_fov );
    }
    M_side = player.side_;
    M_unum = player.unum_;
    M_x = player.x_;
    M_y = player.y_;

    *settled = ( std::fabs( normalizeAngle( goal - M_direction ) ) < SETTLED_DEGREES
                 && std::fabs( goal_fov - M_fov ) < SETTLED_DEGREES );
    // once still, exactly there
    if ( *settled )
    {
        M_direction = goal;
        M_fov = goal_fov;
    }
    return PlayerCamera3D::pose( player.x_, player.y_, o.position, M_direction, M_fov, aspect );
}
