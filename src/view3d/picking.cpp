// -*-c++-*-

/*!
  \file picking.cpp
  \brief what a click on the 3D view aims at. Source File.
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

#include "picking.h"

#include "ground_texture.h"

#include <QMatrix4x4>
#include <QVector4D>

#include <cmath>
#include <limits>

namespace Picking {

namespace {

constexpr float EPS = 1.0e-6f;

/*!
  the smallest t > 0 where the ray meets the body of a player (a cylinder
  from the ground to BODY_HEIGHT), or infinity
*/
float
hit_body( const Ray & ray,
          const PlayerPick & pick )
{
    const float H = SceneBuilder::BODY_HEIGHT;
    const float r2 = pick.radius * pick.radius;
    float best = std::numeric_limits< float >::infinity();

    // the side: where the ray, seen from above (on xz), is the radius away
    // from the center: a t^2 + b t + c = 0
    const float ox = ray.origin.x() - pick.x;
    const float oz = ray.origin.z() - pick.y;
    const float dx = ray.direction.x();
    const float dz = ray.direction.z();
    const float a = dx * dx + dz * dz;
    if ( a > EPS )
    {
        const float b = 2.0f * ( ox * dx + oz * dz );
        const float c = ox * ox + oz * oz - r2;
        const float disc = b * b - 4.0f * a * c;
        if ( disc >= 0.0f )
        {
            const float root = std::sqrt( disc );
            for ( const float t : { ( -b - root ) / ( 2.0f * a ), ( -b + root ) / ( 2.0f * a ) } )
            {
                const float y = ray.origin.y() + t * ray.direction.y();
                if ( t > 0.0f && 0.0f <= y && y <= H )
                {
                    best = std::min( best, t );
                }
            }
        }
    }

    // the top: from above (the top camera looks straight down)
    if ( std::fabs( ray.direction.y() ) > EPS )
    {
        const float t = ( H - ray.origin.y() ) / ray.direction.y();
        const float x = ox + t * dx;
        const float z = oz + t * dz;
        if ( t > 0.0f && x * x + z * z <= r2 )
        {
            best = std::min( best, t );
        }
    }

    return best;
}

/*!
  the smallest t > 0 where the ray meets the head of a player (a sphere),
  or infinity
*/
float
hit_head( const Ray & ray,
          const PlayerPick & pick )
{
    const QVector3D center( pick.x, SceneBuilder::HEAD_HEIGHT, pick.y );
    const float r = SceneBuilder::HEAD_RADIUS;
    // |o + t d - center|^2 = r^2, with |d| = 1: t^2 + b t + c = 0
    const QVector3D oc = ray.origin - center;
    const float b = 2.0f * QVector3D::dotProduct( oc, ray.direction );
    const float c = oc.lengthSquared() - r * r;
    const float disc = b * b - 4.0f * c;
    if ( disc < 0.0f )
    {
        return std::numeric_limits< float >::infinity();
    }
    const float root = std::sqrt( disc );
    for ( const float t : { ( -b - root ) / 2.0f, ( -b + root ) / 2.0f } )
    {
        if ( t > 0.0f )
        {
            return t;
        }
    }
    return std::numeric_limits< float >::infinity();
}

}

/*-------------------------------------------------------------------*/
/*!
  The point of the screen goes back through the camera's matrices (the
  inverse of projection() * view()) to two points of the world: one on the
  near plane (depth -1 in the cube of OpenGL) and one on the far plane
  (depth 1). The ray goes from the first to the second. This works for the
  orthographic top camera too, whose rays are parallel.
*/
Ray
rayAt( const CameraPose & camera,
       const QSizeF & view,
       const QPointF & point )
{
    const float w = float( std::max( 1.0, view.width() ) );
    const float h = float( std::max( 1.0, view.height() ) );
    // from the pixels (y down) to -1 .. 1 (y up)
    const float nx = 2.0f * float( point.x() ) / w - 1.0f;
    const float ny = 1.0f - 2.0f * float( point.y() ) / h;

    const QMatrix4x4 inverse = ( camera.projection( w / h ) * camera.view() ).inverted();
    QVector4D on_near = inverse.map( QVector4D( nx, ny, -1.0f, 1.0f ) );
    QVector4D on_far = inverse.map( QVector4D( nx, ny, 1.0f, 1.0f ) );
    on_near /= on_near.w();
    on_far /= on_far.w();

    Ray ray;
    ray.origin = on_near.toVector3D();
    ray.direction = ( on_far.toVector3D() - ray.origin ).normalized();
    return ray;
}

/*-------------------------------------------------------------------*/
bool
groundAt( const Ray & ray,
          QVector2D * field )
{
    if ( ray.direction.y() > -EPS )
    {
        return false;
    }
    const float t = -ray.origin.y() / ray.direction.y();
    if ( t < 0.0f )
    {
        return false;
    }
    const QVector3D p = ray.origin + t * ray.direction;
    *field = QVector2D( p.x(), p.z() );
    return true;
}

/*-------------------------------------------------------------------*/
int
playerAt( const Ray & ray,
          const std::vector< PlayerPick > & picks )
{
    int found = -1;
    float nearest = std::numeric_limits< float >::infinity();
    for ( size_t i = 0; i < picks.size(); ++i )
    {
        const float t = std::min( hit_body( ray, picks[i] ), hit_head( ray, picks[i] ) );
        if ( t < nearest )
        {
            nearest = t;
            found = int( i );
        }
    }
    return found;
}

/*-------------------------------------------------------------------*/
RefereeTarget
targetAt( const CameraPose & camera,
          const QSizeF & view,
          const QPointF & point,
          const std::vector< PlayerPick > & picks )
{
    const Ray ray = rayAt( camera, view, point );

    const int i = playerAt( ray, picks );
    if ( i >= 0 )
    {
        const PlayerPick & p = picks[i];
        return RefereeTarget::player( p.side, p.unum, p.x, p.y );
    }

    QVector2D ground;
    if ( groundAt( ray, &ground )
         && std::fabs( ground.x() ) <= GroundTexture::length() * 0.5
         && std::fabs( ground.y() ) <= GroundTexture::width() * 0.5 )
    {
        return RefereeTarget::point( ground.x(), ground.y() );
    }

    return RefereeTarget();
}

}
