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
void
GaitTable::setStrides( const std::vector< GaitStride > & strides )
{
    const bool same = ( strides.size() == M_strides.size()
                        && std::equal( strides.begin(), strides.end(), M_strides.begin(),
                                       []( const GaitStride & a, const GaitStride & b )
                                         {
                                             return a.speed == b.speed && a.stride == b.stride;
                                         } ) );
    if ( same ) return;
    M_strides = strides;
    clear();
}

/*-------------------------------------------------------------------*/
float
GaitTable::strideAt( const float speed ) const
{
    if ( M_strides.empty() ) return 1.0f;
    if ( speed <= M_strides.front().speed ) return M_strides.front().stride;
    for ( std::size_t k = 1; k < M_strides.size(); ++k )
    {
        const GaitStride & a = M_strides[k - 1];
        const GaitStride & b = M_strides[k];
        if ( speed <= b.speed )
        {
            const float t = ( speed - a.speed ) / std::max( 1.0e-6f, b.speed - a.speed );
            return a.stride + ( b.stride - a.stride ) * t;
        }
    }
    return M_strides.back().stride;
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
            // backward: away from the body direction
            const double toward = std::atan2( dy, dx ) - a.body_ * M_PI / 180.0;
            const double sign = ( std::cos( toward ) < 0.0 ? -1.0 : 1.0 );
            e.gait += sign * d / strideAt( static_cast< float >( d / step ) );
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
