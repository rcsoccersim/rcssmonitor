// -*-c++-*-

/*!
  \file gait_table.cpp
  \brief how far the players have walked, for the human figures. Source File.
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

#include "gait_table.h"

#include <algorithm>
#include <cmath>

/*-------------------------------------------------------------------*/
bool
GaitWays::operator==( const GaitWays & other ) const
{
    for ( std::size_t w = 0; w < strides.size(); ++w )
    {
        const std::vector< GaitStride > & a = strides[w];
        const std::vector< GaitStride > & b = other.strides[w];
        if ( a.size() != b.size()
             || ! std::equal( a.begin(), a.end(), b.begin(),
                              []( const GaitStride & x, const GaitStride & y )
                                {
                                    return x.speed == y.speed && x.stride == y.stride;
                                } ) )
        {
            return false;
        }
    }
    return true;
}

/*-------------------------------------------------------------------*/
float
GaitWays::strideAt( const std::vector< GaitStride > & strides,
                    const float speed )
{
    if ( strides.empty() ) return 1.0f;
    if ( speed <= strides.front().speed ) return strides.front().stride;
    for ( std::size_t k = 1; k < strides.size(); ++k )
    {
        const GaitStride & a = strides[k - 1];
        const GaitStride & b = strides[k];
        if ( speed <= b.speed )
        {
            const float t = ( speed - a.speed ) / std::max( 1.0e-6f, b.speed - a.speed );
            return a.stride + ( b.stride - a.stride ) * t;
        }
    }
    return strides.back().stride;
}

/*-------------------------------------------------------------------*/
namespace {

//! the twist and the clips' way of a way gone (radian, to the right positive)
void
twistAndWay( const float toward,
             float * twist,
             float * way )
{
    const float full = GaitWays::TWIST_FULL * float( M_PI ) / 180.0f;
    const float quarter = 0.5f * float( M_PI );
    const float r = std::remainder( toward, float( 2.0 * M_PI ) );
    const float side = ( r < 0.0f ? -1.0f : 1.0f );
    const float d = std::fabs( r );
    // from forward (e: from backward), the hips' turn up to full, then less
    // to none sideways; the clips' way from the hips what is left
    const float e = ( d <= quarter ? d : float( M_PI ) - d );
    const float turn = ( e <= full ? e : full * ( quarter - e ) / ( quarter - full ) );
    const float left = ( e <= full ? 0.0f : quarter * ( e - full ) / ( quarter - full ) );
    if ( d <= quarter )
    {
        *twist = side * turn;
        *way = side * left;
    }
    else
    {
        *twist = -side * turn; // the hips' back toward the way gone
        *way = side * ( float( M_PI ) - left );
    }
}

}

/*-------------------------------------------------------------------*/
float
GaitWays::twistAt( const float toward ) const
{
    if ( ! sideways() ) return 0.0f;
    float twist, way;
    twistAndWay( toward, &twist, &way );
    return twist;
}

/*-------------------------------------------------------------------*/
float
GaitWays::clipWay( const float toward )
{
    float twist, way;
    twistAndWay( toward, &twist, &way );
    return way;
}

/*-------------------------------------------------------------------*/
GaitWays::Mix
GaitWays::mixAt( const float speed,
                 const float toward ) const
{
    Mix mix;
    const std::vector< GaitStride > & back = ( strides[Backward].empty() ? strides[Forward] : strides[Backward] );
    const float forward_stride = strideAt( strides[Forward], speed );
    mix.stride = forward_stride;
    if ( ! directional() ) return mix;

    if ( ! sideways() )
    {
        // the way gone from the body direction, from 0 to 180 degrees
        const float d = std::fabs( std::remainder( toward, float( 2.0 * M_PI ) ) );
        // forward, or backward, mixed in a band about sideways
        const float band = BACK_BAND * float( M_PI ) / 180.0f;
        const float u = std::min( 1.0f, std::max( 0.0f, ( d - ( 0.5f * float( M_PI ) - band ) ) / ( 2.0f * band ) ) );
        mix.b = Backward;
        mix.t = u * u * ( 3.0f - 2.0f * u );
        mix.stride = forward_stride + ( strideAt( back, speed ) - forward_stride ) * mix.t;
        return mix;
    }

    // between the two ways around the clips' way from the hips turned:
    // forward and a side, or a side and backward
    const float w = std::fabs( clipWay( toward ) );
    const int side = ( std::remainder( toward, float( 2.0 * M_PI ) ) >= 0.0f ? Right : Left );
    float theta = w;
    if ( w <= 0.5f * float( M_PI ) )
    {
        mix.a = Forward;
        mix.b = side;
    }
    else
    {
        mix.a = side;
        mix.b = Backward;
        theta = w - 0.5f * float( M_PI );
    }
    const float sa = ( mix.a == Forward ? forward_stride : strideAt( strides[mix.a], speed ) );
    const float sb = ( mix.b == Backward ? strideAt( back, speed ) : strideAt( strides[mix.b], speed ) );
    const float sin_t = std::sin( theta );
    const float cos_t = std::cos( theta );
    const float den = std::max( 1.0e-6f, sa * sin_t + sb * cos_t );
    mix.t = std::min( 1.0f, std::max( 0.0f, sa * sin_t / den ) );
    mix.stride = sa * sb / den;
    return mix;
}

/*-------------------------------------------------------------------*/
void
GaitTable::setWays( const GaitWays & ways )
{
    if ( ways == M_ways ) return;
    M_ways = ways;
    clear();
}

/*-------------------------------------------------------------------*/
void
GaitTable::setStrides( const std::vector< GaitStride > & strides )
{
    GaitWays ways;
    ways.strides[GaitWays::Forward] = strides;
    setWays( ways );
}

/*-------------------------------------------------------------------*/
float
GaitTable::strideAt( const float speed ) const
{
    return GaitWays::strideAt( M_ways.strides[GaitWays::Forward], speed );
}

/*-------------------------------------------------------------------*/
void
GaitTable::clear()
{
    M_entries.clear();
    M_frames = 0;
    M_last.reset();
}

/*-------------------------------------------------------------------*/
void
GaitTable::update( const DispCont & frames,
                   const double step )
{
    if ( step != M_step )
    {
        // the speeds, and so the strides, are by the step
        clear();
        M_step = step;
    }

    if ( M_frames > 0
         && ( M_frames > frames.size() || frames[M_frames - 1] != M_last ) )
    {
        // Where the last frame walked is now: earlier if the oldest frames
        // were dropped (live), nowhere if these are other frames. (M_last
        // keeps it alive, so no other frame can be where it was.)
        std::size_t found = frames.size();
        for ( std::size_t i = std::min( M_frames, frames.size() ); i > 0; --i )
        {
            if ( frames[i - 1] == M_last )
            {
                found = i - 1;
                break;
            }
        }
        if ( found == frames.size() )
        {
            clear();
        }
        else
        {
            const std::size_t dropped = M_frames - 1 - found;
            M_entries.erase( M_entries.begin(), M_entries.begin() + dropped * PLAYERS );
            M_frames -= dropped;
        }
    }

    extend( frames );
}

/*-------------------------------------------------------------------*/
void
GaitTable::extend( const DispCont & frames )
{
    if ( frames.size() <= M_frames ) return;

    const double step = std::max( 1.0e-3, M_step );
    M_entries.resize( frames.size() * PLAYERS );
    for ( std::size_t i = M_frames; i < frames.size(); ++i )
    {
        const rcss::rcg::ShowInfoT & now = frames[i]->show_;
        for ( int k = 0; k < PLAYERS; ++k )
        {
            Entry & e = M_entries[i * PLAYERS + k];
            if ( i == 0 )
            {
                // the players out of step with each other from the start
                e = Entry{ k * 0.29, 0.0f, 0.0f };
                continue;
            }
            e = Entry{ entry( i - 1, k ).gait, 0.0f, 0.0f };
            const rcss::rcg::PlayerT & a = frames[i - 1]->show_.player_[k];
            const rcss::rcg::PlayerT & b = now.player_[k];
            if ( ! a.isAlive() || ! b.isAlive() ) continue;
            const double dx = b.x_ - a.x_;
            const double dy = b.y_ - a.y_;
            const double d = std::sqrt( dx * dx + dy * dy );
            if ( d < 1.0e-4 || d >= TELEPORT ) continue;
            const double toward = std::atan2( dy, dx ) - a.body_ * M_PI / 180.0;
            if ( M_ways.directional() )
            {
                // on whichever way, by the stride of the clips mixed for it
                e.gait += d / M_ways.mixAt( static_cast< float >( d / step ), static_cast< float >( toward ) ).stride;
            }
            else
            {
                // backward: away from the body direction, the cycles taken back
                const double sign = ( std::cos( toward ) < 0.0 ? -1.0 : 1.0 );
                e.gait += sign * d / strideAt( static_cast< float >( d / step ) );
            }
            e.vx = static_cast< float >( dx / step );
            e.vy = static_cast< float >( dy / step );
        }
    }
    M_frames = frames.size();
    M_last = frames.back();
}

/*-------------------------------------------------------------------*/
void
GaitTable::velocityAt( const int player,
                       const double time,
                       float * vx,
                       float * vy ) const
{
    // the move into frame k is at k - 0.5; held before the first and after the last
    const double u = std::max( 0.0, time + 0.5 );
    const std::size_t last = M_frames - 1;
    const std::size_t k = std::min( static_cast< std::size_t >( std::floor( u ) ), last );
    const std::size_t j = std::min( k + 1, last );
    const float f = ( k == last ? 0.0f : static_cast< float >( u - std::floor( u ) ) );
    const Entry & a = entry( k, player );
    const Entry & b = entry( j, player );
    *vx = a.vx + ( b.vx - a.vx ) * f;
    *vy = a.vy + ( b.vy - a.vy ) * f;
}

/*-------------------------------------------------------------------*/
FigureMotion
GaitTable::motion( const int player,
                   const std::size_t index,
                   const std::size_t next,
                   const double alpha ) const
{
    FigureMotion m;
    if ( index >= M_frames
         || player < 0
         || player >= PLAYERS )
    {
        return m;
    }

    // not moving toward a next frame: at the frame
    const double a = ( next != index && index + 1 < M_frames ? std::min( 1.0, std::max( 0.0, alpha ) ) : 0.0 );
    const double from = entry( index, player ).gait;
    const double to = ( a > 0.0 ? entry( index + 1, player ).gait : from );
    m.gait = from + ( to - from ) * a;

    const double time = index + a;
    velocityAt( player, time, &m.vx, &m.vy );

    // the change of the velocity over the cycles around, for the figure's lean
    float bx, by, fx, fy;
    velocityAt( player, time - ACCELERATION_SPAN, &bx, &by );
    velocityAt( player, time + ACCELERATION_SPAN, &fx, &fy );
    const float span = static_cast< float >( 2.0 * ACCELERATION_SPAN * std::max( 1.0e-3, M_step ) );
    m.ax = ( fx - bx ) / span;
    m.ay = ( fy - by ) / span;
    return m;
}
