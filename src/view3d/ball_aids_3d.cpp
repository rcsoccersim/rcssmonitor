// -*-c++-*-

/*!
  \file ball_aids_3d.cpp
  \brief what helps to find the ball in the 3D view: a ring and a trail. Source File.
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

#include "ball_aids_3d.h"

#include "ball_3d.h"

#include <algorithm>
#include <cmath>

namespace BallAids3D {

namespace {

//! the ball's center in the world at a frame
QVector3D
center( const rcss::rcg::BallT & ball,
        const float radius )
{
    return QVector3D( ball.x_, std::max( 0.0f, ball.z_ ) + radius, ball.y_ );
}

}

/*-------------------------------------------------------------------*/
std::vector< TrailPoint >
trailPoints( const DispCont & frames,
             const std::size_t index,
             const std::size_t next,
             const double alpha,
             const int cycles,
             const float radius )
{
    std::vector< TrailPoint > points;
    if ( index >= frames.size() || cycles <= 0 ) return points;

    const rcss::rcg::DispInfoT & here = *frames[index];
    // the place drawn, on the way to the next frame
    float later_age = 0.0f;
    if ( alpha > 0.0
         && next != index
         && next < frames.size()
         && Ball3D::rolls( here, *frames[next] ) )
    {
        const float t = static_cast< float >( std::min( 1.0, alpha ) );
        const QVector3D a = center( here.show_.ball_, radius );
        const QVector3D b = center( frames[next]->show_.ball_, radius );
        points.push_back( TrailPoint{ a + ( b - a ) * t, 0.0f } );
        later_age = t;
    }
    points.push_back( TrailPoint{ center( here.show_.ball_, radius ), later_age } );

    // back through the frames before, the last one part of the way to it
    const rcss::rcg::DispInfoT * later = &here;
    for ( std::size_t k = 1; later_age < float( cycles ) && k <= index; ++k )
    {
        const rcss::rcg::DispInfoT & earlier = *frames[index - k];
        if ( ! Ball3D::rolls( earlier, *later ) ) break;
        const float age = later_age + 1.0f;
        const QVector3D a = center( later->show_.ball_, radius );
        const QVector3D b = center( earlier.show_.ball_, radius );
        if ( age > float( cycles ) )
        {
            const float t = ( float( cycles ) - later_age ) / ( age - later_age );
            points.push_back( TrailPoint{ a + ( b - a ) * t, float( cycles ) } );
            break;
        }
        points.push_back( TrailPoint{ b, age } );
        later = &earlier;
        later_age = age;
    }
    return points;
}

/*-------------------------------------------------------------------*/
float
metersPerPixel( const CameraPose & camera,
                const QVector3D & at,
                const float height )
{
    const float h = std::max( 1.0f, height );
    if ( camera.ortho_half_height > 0.0f )
    {
        return 2.0f * camera.ortho_half_height / h;
    }
    const QVector3D forward = ( camera.target - camera.position ).normalized();
    const float depth = std::max( 0.1f, QVector3D::dotProduct( at - camera.position, forward ) );
    return depth * 2.0f * std::tan( camera.fov_y * 0.5f * float( M_PI ) / 180.0f ) / h;
}

/*-------------------------------------------------------------------*/
void
addRing( SceneContent & content,
         const QVector3D & center,
         const float radius,
         const float meters_per_pixel,
         const QColor & color )
{
    Instance3D ring;
    ring.model.translate( center.x(), RING_Y, center.z() );
    ring.model.scale( radius, 1.0f, radius );
    ring.color = QVector4D( float( color.redF() ), float( color.greenF() ), float( color.blueF() ), 1.0f );
    content.ball_rings.push_back( ring );
    content.ball_ring_radius = radius;
    // never thinner than RING_PIXELS, nor wider than half the radius
    content.ball_ring_width = std::min( radius * 0.5f, std::max( RING_WIDTH, RING_PIXELS * meters_per_pixel ) );
}

/*-------------------------------------------------------------------*/
void
addTrail( SceneContent & content,
          const std::vector< TrailPoint > & points,
          const QVector3D & eye,
          const float head_width,
          const int cycles,
          const QColor & color )
{
    if ( points.size() < 2 ) return;

    const float r = float( color.redF() ), g = float( color.greenF() ), b = float( color.blueF() );
    const float length = std::max( 1.0e-6f, float( cycles ) );

    // three vertices a point: an edge, the middle, the other edge
    struct Cross {
        QVector3D left, middle, right;
        float opacity;
    };
    std::vector< Cross > cross;
    cross.reserve( points.size() );
    QVector3D last( 0.0f, 0.0f, 1.0f );
    for ( std::size_t i = 0; i < points.size(); ++i )
    {
        const TrailPoint & p = points[i];
        const TrailPoint & a = points[i == 0 ? 0 : i - 1];
        const TrailPoint & c = points[std::min( points.size() - 1, i + 1 )];
        // across the way, facing the eye; as the last where the ball goes
        // straight at the camera, or does not move
        QVector3D across = QVector3D::crossProduct( c.at - a.at, eye - p.at );
        if ( across.lengthSquared() < 1.0e-12f )
        {
            across = last;
        }
        else
        {
            across.normalize();
            last = across;
        }
        const float left = std::max( 0.0f, 1.0f - p.age / length );
        const float half = 0.5f * head_width * std::pow( left, 0.6f );
        cross.push_back( Cross{ p.at + across * half, p.at, p.at - across * half, TRAIL_OPACITY * left } );
    }

    const auto vertex = [&]( const QVector3D & at, const float opacity ) {
        content.ball_trail.push_back( ColorVertex3D{ at, QVector4D( r, g, b, opacity ), 0.0f } );
    };
    for ( std::size_t i = 1; i < cross.size(); ++i )
    {
        const Cross & p = cross[i - 1];
        const Cross & q = cross[i];
        // two quads between the points: the edges clear, the middle opaque
        vertex( p.left, 0.0f ); vertex( p.middle, p.opacity ); vertex( q.left, 0.0f );
        vertex( p.middle, p.opacity ); vertex( q.middle, q.opacity ); vertex( q.left, 0.0f );
        vertex( p.middle, p.opacity ); vertex( p.right, 0.0f ); vertex( q.middle, q.opacity );
        vertex( p.right, 0.0f ); vertex( q.right, 0.0f ); vertex( q.middle, q.opacity );
    }
}

}
