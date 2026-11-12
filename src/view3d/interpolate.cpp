// -*-c++-*-

/*!
  \file interpolate.cpp
  \brief the interpolation between frames for the 3D view. Source File.
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

#include "interpolate.h"

#include <algorithm>
#include <cmath>

namespace Interpolate {

namespace {

float
lerp( const float a,
      const float b,
      const float t )
{
    return a + ( b - a ) * t;
}

float
normalize_angle( float deg )
{
    deg = std::fmod( deg + 180.0f, 360.0f );
    if ( deg < 0.0f ) deg += 360.0f;
    return deg - 180.0f;
}

bool
moved( const float ax,
       const float ay,
       const float bx,
       const float by,
       const float max )
{
    return std::hypot( bx - ax, by - ay ) > max;
}

}

/*-------------------------------------------------------------------*/
float
lerpAngle( const float a,
           const float b,
           const float t )
{
    return normalize_angle( a + normalize_angle( b - a ) * t );
}

/*-------------------------------------------------------------------*/
rcss::rcg::ShowInfoT
show( const rcss::rcg::DispInfoT & a,
      const rcss::rcg::DispInfoT * b,
      const double alpha )
{
    rcss::rcg::ShowInfoT out = a.show_;
    if ( ! b
         || alpha <= 0.0
         || a.pmode_ != b->pmode_ )
    {
        return out;
    }
    const float t = static_cast< float >( std::min( 1.0, alpha ) );

    const rcss::rcg::BallT & ba = a.show_.ball_;
    const rcss::rcg::BallT & bb = b->show_.ball_;
    if ( ! moved( ba.x_, ba.y_, bb.x_, bb.y_, MAX_BALL_STEP ) )
    {
        rcss::rcg::BallT & o = out.ball_;
        o.x_ = lerp( ba.x_, bb.x_, t );
        o.y_ = lerp( ba.y_, bb.y_, t );
        if ( ba.hasVelocity() && bb.hasVelocity() )
        {
            o.vx_ = lerp( ba.vx_, bb.vx_, t );
            o.vy_ = lerp( ba.vy_, bb.vy_, t );
        }
        o.z_ = lerp( ba.z_, bb.z_, t );
        o.vz_ = lerp( ba.vz_, bb.vz_, t );
    }

    for ( int i = 0; i < rcss::rcg::MAX_PLAYER * 2; ++i )
    {
        const rcss::rcg::PlayerT & pa = a.show_.player_[i];
        const rcss::rcg::PlayerT & pb = b->show_.player_[i];
        if ( ! pa.isAlive()
             || ! pb.isAlive()
             || pa.unum_ != pb.unum_
             || pa.side_ != pb.side_
             || moved( pa.x_, pa.y_, pb.x_, pb.y_, MAX_PLAYER_STEP ) )
        {
            continue;
        }
        rcss::rcg::PlayerT & o = out.player_[i];
        o.x_ = lerp( pa.x_, pb.x_, t );
        o.y_ = lerp( pa.y_, pb.y_, t );
        if ( pa.hasVelocity() && pb.hasVelocity() )
        {
            o.vx_ = lerp( pa.vx_, pb.vx_, t );
            o.vy_ = lerp( pa.vy_, pb.vy_, t );
        }
        o.body_ = lerpAngle( pa.body_, pb.body_, t );
        if ( pa.hasNeck() && pb.hasNeck() )
        {
            // relative to the body, within the neck's range: no wrap around
            o.neck_ = lerp( pa.neck_, pb.neck_, t );
        }
    }
    return out;
}

/*-------------------------------------------------------------------*/
Position
position( const std::size_t frame_count,
          const std::size_t current,
          const bool live,
          const double play_alpha,
          const double since_newest )
{
    Position p;
    if ( frame_count == 0 )
    {
        return p;
    }
    if ( live && frame_count >= 2 )
    {
        p.index = frame_count - 2;
        p.next = frame_count - 1;
        p.alpha = std::max( 0.0, std::min( 1.0, since_newest ) );
        return p;
    }
    p.index = std::min( current, frame_count - 1 );
    p.next = std::min( p.index + 1, frame_count - 1 );
    p.alpha = ( p.next != p.index ? std::max( 0.0, std::min( 1.0, play_alpha ) ) : 0.0 );
    return p;
}

}
