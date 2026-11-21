// -*-c++-*-

/*!
  \file ball_3d.cpp
  \brief the ball of the 3D view: its pattern, and how it has rolled. Source File.
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

#include "ball_3d.h"

#include "interpolate.h"

#include <algorithm>
#include <cmath>

namespace Ball3D {

/*-------------------------------------------------------------------*/
const Pentagons &
pentagons()
{
    static const Pentagons p = []()
        {
            Pentagons out;
            // the corners of an icosahedron: (0, ±1, ±phi) and its cyclic turns
            const float phi = ( 1.0f + std::sqrt( 5.0f ) ) * 0.5f;
            int n = 0;
            for ( const float a : { -1.0f, 1.0f } )
            {
                for ( const float b : { -phi, phi } )
                {
                    out.centers[n++] = QVector3D( 0.0f, a, b ).normalized();
                    out.centers[n++] = QVector3D( a, b, 0.0f ).normalized();
                    out.centers[n++] = QVector3D( b, 0.0f, a ).normalized();
                }
            }
            // a corner of each pentagon points to the nearest neighbor
            for ( int i = 0; i < PENTAGONS; ++i )
            {
                float nearest = -2.0f;
                for ( int j = 0; j < PENTAGONS; ++j )
                {
                    const float d = QVector3D::dotProduct( out.centers[i], out.centers[j] );
                    if ( j != i && d > nearest )
                    {
                        nearest = d;
                        out.toward[i] = out.centers[j];
                    }
                }
            }
            return out;
        }();
    return p;
}

/*-------------------------------------------------------------------*/
float
patternAt( const QVector3D & direction )
{
    // as ball.frag
    const Pentagons & p = pentagons();
    const QVector3D n = direction.normalized();
    int best = 0;
    float nearest = -2.0f;
    for ( int i = 0; i < PENTAGONS; ++i )
    {
        const float d = QVector3D::dotProduct( n, p.centers[i] );
        if ( d > nearest )
        {
            nearest = d;
            best = i;
        }
    }
    const QVector3D v = p.centers[best];
    const QVector3D e1 = ( p.toward[best] - v * QVector3D::dotProduct( p.toward[best], v ) ).normalized();
    const QVector3D e2 = QVector3D::crossProduct( v, e1 );
    // around the center from a corner, in sectors of 72 degrees: the edges'
    // normals are 36 degrees off the corners
    const float sector = 2.0f * 3.14159265f / 5.0f;
    const float t = std::atan2( QVector3D::dotProduct( n, e2 ), QVector3D::dotProduct( n, e1 ) );
    const float a = t - sector * std::floor( t / sector ) - 0.5f * sector;
    const float r = std::acos( std::clamp( nearest, -1.0f, 1.0f ) ) * std::cos( a );
    const float edge = PENTAGON * std::cos( 0.5f * sector );
    return ( r < edge ? PENTAGON_SHADE : 1.0f );
}

/*-------------------------------------------------------------------*/
QQuaternion
roll( const float ax,
      const float ay,
      const float bx,
      const float by,
      const float radius,
      const float fraction )
{
    // the world: X = the field's x, Z = the field's y, Y up. Rolling along
    // the way d turns about up x d = (dz, 0, -dx).
    const float dx = bx - ax;
    const float dz = by - ay;
    const float d = std::hypot( dx, dz );
    if ( d < 1.0e-6f ) return QQuaternion();
    const float angle = std::min( MAX_ROLL, d / std::max( 1.0e-3f, radius ) ) * fraction;
    return QQuaternion::fromAxisAndAngle( QVector3D( dz / d, 0.0f, -dx / d ),
                                          angle * 180.0f / 3.14159265f );
}

/*-------------------------------------------------------------------*/
bool
rolls( const rcss::rcg::DispInfoT & a,
       const rcss::rcg::DispInfoT & b )
{
    const rcss::rcg::BallT & ba = a.show_.ball_;
    const rcss::rcg::BallT & bb = b.show_.ball_;
    return ( a.pmode_ == b.pmode_
             && std::hypot( bb.x_ - ba.x_, bb.y_ - ba.y_ ) <= Interpolate::MAX_BALL_STEP );
}

}

/*-------------------------------------------------------------------*/
void
BallSpin::clear()
{
    M_turns.clear();
    M_last.reset();
}

/*-------------------------------------------------------------------*/
void
BallSpin::update( const DispCont & frames,
                  const float radius )
{
    if ( radius != M_radius )
    {
        clear();
        M_radius = radius;
    }

    std::size_t count = M_turns.size();
    if ( count > 0
         && ( count > frames.size() || frames[count - 1] != M_last ) )
    {
        // Where the last frame rolled is now: earlier if the oldest frames
        // were dropped (live), nowhere if these are other frames (as
        // GaitTable::update()).
        std::size_t found = frames.size();
        for ( std::size_t i = std::min( count, frames.size() ); i > 0; --i )
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
            M_turns.erase( M_turns.begin(), M_turns.begin() + ( count - 1 - found ) );
        }
        count = M_turns.size();
    }

    if ( frames.size() <= count ) return;

    M_turns.reserve( frames.size() );
    for ( std::size_t i = count; i < frames.size(); ++i )
    {
        if ( i == 0 )
        {
            M_turns.push_back( QQuaternion() );
            continue;
        }
        QQuaternion q = M_turns[i - 1];
        const rcss::rcg::DispInfoT & a = *frames[i - 1];
        const rcss::rcg::DispInfoT & b = *frames[i];
        if ( Ball3D::rolls( a, b ) )
        {
            q = ( Ball3D::roll( a.show_.ball_.x_, a.show_.ball_.y_,
                                b.show_.ball_.x_, b.show_.ball_.y_, M_radius ) * q ).normalized();
        }
        M_turns.push_back( q );
    }
    M_last = frames.back();
}

/*-------------------------------------------------------------------*/
QQuaternion
BallSpin::turnAt( const DispCont & frames,
                  const std::size_t index,
                  const std::size_t next,
                  const double alpha ) const
{
    if ( M_turns.empty() ) return QQuaternion();
    const std::size_t i = std::min( index, M_turns.size() - 1 );
    QQuaternion q = M_turns[i];
    if ( alpha > 0.0
         && next != i
         && next < frames.size()
         && i < frames.size()
         && Ball3D::rolls( *frames[i], *frames[next] ) )
    {
        const rcss::rcg::BallT & a = frames[i]->show_.ball_;
        const rcss::rcg::BallT & b = frames[next]->show_.ball_;
        q = ( Ball3D::roll( a.x_, a.y_, b.x_, b.y_, M_radius,
                            static_cast< float >( std::min( 1.0, alpha ) ) ) * q ).normalized();
    }
    return q;
}
