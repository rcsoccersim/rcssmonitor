// -*-c++-*-

/*!
  \file meshes_3d.cpp
  \brief the shapes of the 3D view. Source File.
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

#include "meshes_3d.h"

#include <QVector2D>
#include <QVector3D>

#include <cmath>
#include <utility>

namespace {

void
push( std::vector< Vertex3D > & v,
      const QVector3D & p,
      const QVector3D & n,
      const QVector2D & uv )
{
    v.push_back( Vertex3D{ { p.x(), p.y(), p.z() },
                           { n.x(), n.y(), n.z() },
                           { uv.x(), uv.y() } } );
}

/*!
  \brief add a triangle, turned counterclockwise as seen from the side that
  its normals face (its front). The cross product of two edges points to the
  side from which the corners go counterclockwise; if it is against the
  normals, two corners are swapped.
  \param a, b, c the corners
  \param na, nb, nc their normals
  \param ta, tb, tc their texture coordinates
*/
void
add_triangle( std::vector< Vertex3D > & v,
              QVector3D a, QVector3D na,
              QVector3D b, QVector3D nb,
              QVector3D c, QVector3D nc,
              QVector2D ta = QVector2D(), QVector2D tb = QVector2D(), QVector2D tc = QVector2D() )
{
    if ( QVector3D::dotProduct( QVector3D::crossProduct( b - a, c - a ), na + nb + nc ) < 0.0f )
    {
        std::swap( b, c );
        std::swap( nb, nc );
        std::swap( tb, tc );
    }
    push( v, a, na, ta );
    push( v, b, nb, tb );
    push( v, c, nc, tc );
}

//! the point at the angle around the y axis
QVector3D
around_y( const double angle,
          const float r,
          const float y )
{
    return QVector3D( r * float( std::cos( angle ) ), y, r * float( std::sin( angle ) ) );
}

}

namespace Meshes3D {

/*-------------------------------------------------------------------*/
std::vector< Vertex3D >
cylinder( const int segments )
{
    std::vector< Vertex3D > v;
    const QVector3D up( 0.0f, 1.0f, 0.0f ), down( 0.0f, -1.0f, 0.0f );
    // around the axis, segment by segment: from angle a0 to a1, the bottom
    // corners b0 b1 and the top corners t0 t1
    for ( int i = 0; i < segments; ++i )
    {
        const double a0 = 2.0 * M_PI * i / segments, a1 = 2.0 * M_PI * ( i + 1 ) / segments;
        const QVector3D b0 = around_y( a0, 1.0f, -0.5f ), b1 = around_y( a1, 1.0f, -0.5f );
        const QVector3D t0 = around_y( a0, 1.0f, 0.5f ), t1 = around_y( a1, 1.0f, 0.5f );
        const QVector3D n0 = around_y( a0, 1.0f, 0.0f ), n1 = around_y( a1, 1.0f, 0.0f );
        // the side: a rectangle of two triangles, the normals outward (smooth
        // shading: each corner has the normal of the round surface there)
        add_triangle( v, b0, n0, b1, n1, t1, n1 );
        add_triangle( v, b0, n0, t1, n1, t0, n0 );
        // the ends: a slice of the top and of the bottom, from the center
        add_triangle( v, QVector3D( 0.0f, 0.5f, 0.0f ), up, t0, up, t1, up );
        add_triangle( v, QVector3D( 0.0f, -0.5f, 0.0f ), down, b0, down, b1, down );
    }
    return v;
}

/*-------------------------------------------------------------------*/
std::vector< Vertex3D >
sphere( const int segments,
        const int rings )
{
    std::vector< Vertex3D > v;
    // a grid of longitude (i, around y) and latitude (j, from the bottom
    // pole j = 0 to the top j = rings). On a unit sphere, the normal is the
    // position itself.
    auto at = [&]( const int i, const int j ) {
        const double a = 2.0 * M_PI * i / segments;
        const double b = M_PI * j / rings - M_PI / 2.0;
        return around_y( a, float( std::cos( b ) ), float( std::sin( b ) ) );
    };
    for ( int j = 0; j < rings; ++j )
    {
        for ( int i = 0; i < segments; ++i )
        {
            const QVector3D p00 = at( i, j ), p10 = at( i + 1, j ), p01 = at( i, j + 1 ), p11 = at( i + 1, j + 1 );
            // each cell of the grid is two triangles, except at the poles,
            // where a cell is a single triangle (the other has no area)
            if ( j > 0 ) add_triangle( v, p00, p00, p10, p10, p11, p11 );
            if ( j < rings - 1 ) add_triangle( v, p00, p00, p11, p11, p01, p01 );
        }
    }
    return v;
}

/*-------------------------------------------------------------------*/
std::vector< Vertex3D >
cone( const int segments )
{
    std::vector< Vertex3D > v;
    const QVector3D tip( 1.0f, 0.0f, 0.0f ), back( -1.0f, 0.0f, 0.0f );
    auto rim = [&]( const int i ) {
        const double a = 2.0 * M_PI * i / segments;
        return QVector3D( 0.0f, float( std::cos( a ) ), float( std::sin( a ) ) );
    };
    // around the x axis, segment by segment: the rim corners r0 r1 at x = 0
    for ( int i = 0; i < segments; ++i )
    {
        const QVector3D r0 = rim( i ), r1 = rim( i + 1 );
        // the side: from the rim to the tip. It leans 45 degrees (radius 1,
        // length 1), so its normal is halfway between outward and +x
        const QVector3D n0 = ( r0 + tip ).normalized(), n1 = ( r1 + tip ).normalized();
        add_triangle( v, r0, n0, r1, n1, tip, ( n0 + n1 ).normalized() );
        // the base: a slice of the disc at x = 0, facing back
        add_triangle( v, QVector3D(), back, r0, back, r1, back );
    }
    return v;
}

/*-------------------------------------------------------------------*/
std::vector< Vertex3D >
ring( const float inner,
      const float outer,
      const int segments )
{
    std::vector< Vertex3D > v;
    const QVector3D up( 0.0f, 1.0f, 0.0f );
    for ( int i = 0; i < segments; ++i )
    {
        const double a0 = 2.0 * M_PI * i / segments, a1 = 2.0 * M_PI * ( i + 1 ) / segments;
        const QVector3D o0 = around_y( a0, outer, 0.0f ), o1 = around_y( a1, outer, 0.0f );
        const QVector3D i0 = around_y( a0, inner, 0.0f ), i1 = around_y( a1, inner, 0.0f );
        // a segment between the inner and the outer circle: two triangles,
        // or one from the center for a disc
        add_triangle( v, i0, up, o0, up, o1, up );
        if ( inner > 0.0f )
        {
            add_triangle( v, i0, up, o1, up, i1, up );
        }
    }
    return v;
}

/*-------------------------------------------------------------------*/
std::vector< Vertex3D >
quad()
{
    std::vector< Vertex3D > v;
    const QVector3D n( 0.0f, 0.0f, 1.0f );
    const QVector3D a( -0.5f, -0.5f, 0.0f ), b( 0.5f, -0.5f, 0.0f ), c( 0.5f, 0.5f, 0.0f ), d( -0.5f, 0.5f, 0.0f );
    add_triangle( v, a, n, b, n, c, n, QVector2D( 0, 0 ), QVector2D( 1, 0 ), QVector2D( 1, 1 ) );
    add_triangle( v, a, n, c, n, d, n, QVector2D( 0, 0 ), QVector2D( 1, 1 ), QVector2D( 0, 1 ) );
    return v;
}

/*-------------------------------------------------------------------*/
std::vector< Vertex3D >
ground( const float length,
        const float width )
{
    std::vector< Vertex3D > v;
    const QVector3D up( 0.0f, 1.0f, 0.0f );
    const float x0 = -length / 2, x1 = length / 2;
    const float z0 = -width / 2, z1 = width / 2;
    // the corners, and where they are in the image: the image's (0, 0) is
    // its top left, at the field's -x -y corner (GroundTexture)
    const QVector3D p00( x0, 0, z0 ), p10( x1, 0, z0 ), p01( x0, 0, z1 ), p11( x1, 0, z1 );
    add_triangle( v, p00, up, p11, up, p10, up, QVector2D( 0, 0 ), QVector2D( 1, 1 ), QVector2D( 1, 0 ) );
    add_triangle( v, p00, up, p01, up, p11, up, QVector2D( 0, 0 ), QVector2D( 0, 1 ), QVector2D( 1, 1 ) );
    return v;
}

}
