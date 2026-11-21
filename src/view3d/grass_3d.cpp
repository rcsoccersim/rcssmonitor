// -*-c++-*-

/*!
  \file grass_3d.cpp
  \brief the realistic grass of the 3D view. Source File.
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

#include "grass_3d.h"

#include "field_painter.h"

#include <QVector2D>

#include <algorithm>
#include <cmath>
#include <thread>
#include <vector>

namespace Grass3D {

namespace {

//! the smooth step of GLSL
float
smoothstep( const float e0,
            const float e1,
            const float x )
{
    const float t = std::min( 1.0f, std::max( 0.0f, ( x - e0 ) / ( e1 - e0 ) ) );
    return t * t * ( 3.0f - 2.0f * t );
}

//! a worn spot: where, how far out (m) it fades (an ellipse: rx along x, rz along z), how worn in the middle
struct Spot {
    float x, z, rx, rz, amount;
};

//! the worn spots (the same as WEAR_SPOTS in ground.frag)
const Spot SPOTS[] = {
    { -49.5f, 0.0f, 7.0f, 9.0f, 1.0f },  // the left goal mouth
    { 49.5f, 0.0f, 7.0f, 9.0f, 1.0f },   // the right goal mouth
    { -41.5f, 0.0f, 2.5f, 2.5f, 0.6f },  // the left penalty spot
    { 41.5f, 0.0f, 2.5f, 2.5f, 0.6f },   // the right penalty spot
    { 0.0f, 0.0f, 4.0f, 4.0f, 0.5f },    // the center
};

}

/*-------------------------------------------------------------------*/
QImage
noiseImage( const int size,
            const std::uint32_t seed )
{
    // values on lattices of 4, 8, 16, 32 and 64 cells across (each wraps
    // around, so the image tiles), smoothly interpolated and summed with
    // halving weights (fractal noise)
    std::uint32_t state = seed;
    const auto next = [&state]() {
        state = state * 1664525u + 1013904223u; // an LCG: enough for a texture
        return float( ( state >> 8 ) & 0xFFFFFF ) / float( 0xFFFFFF );
    };
    std::vector< float > sum( std::size_t( size ) * size, 0.0f );
    float total = 0.0f;
    float weight = 1.0f;
    for ( int cells = 4; cells <= 64 && cells <= size; cells *= 2 )
    {
        std::vector< float > lattice( std::size_t( cells ) * cells );
        for ( float & v : lattice ) v = next();
        const auto at = [&]( int i, int j ) {
            i = ( i % cells + cells ) % cells;
            j = ( j % cells + cells ) % cells;
            return lattice[std::size_t( j ) * cells + i];
        };
        for ( int y = 0; y < size; ++y )
        {
            for ( int x = 0; x < size; ++x )
            {
                const float fx = float( x ) * cells / size;
                const float fy = float( y ) * cells / size;
                const int ix = int( std::floor( fx ) );
                const int iy = int( std::floor( fy ) );
                const float tx = smoothstep( 0.0f, 1.0f, fx - ix );
                const float ty = smoothstep( 0.0f, 1.0f, fy - iy );
                const float top = at( ix, iy ) * ( 1.0f - tx ) + at( ix + 1, iy ) * tx;
                const float bottom = at( ix, iy + 1 ) * ( 1.0f - tx ) + at( ix + 1, iy + 1 ) * tx;
                sum[std::size_t( y ) * size + x] += weight * ( top * ( 1.0f - ty ) + bottom * ty );
            }
        }
        total += weight;
        weight *= 0.6f;
    }

    // stretched to use the gray levels around 0.5
    float lo = 1.0e9f, hi = -1.0e9f, mean = 0.0f;
    for ( float & v : sum )
    {
        v /= total;
        lo = std::min( lo, v );
        hi = std::max( hi, v );
        mean += v;
    }
    mean /= float( sum.size() );
    const float half = std::max( mean - lo, hi - mean );
    QImage image( size, size, QImage::Format_Grayscale8 );
    for ( int y = 0; y < size; ++y )
    {
        uchar * line = image.scanLine( y );
        for ( int x = 0; x < size; ++x )
        {
            const float v = 0.5f + 0.5f * ( sum[std::size_t( y ) * size + x] - mean ) / std::max( 1.0e-6f, half );
            line[x] = uchar( std::lround( std::min( 1.0f, std::max( 0.0f, v ) ) * 255.0f ) );
        }
    }
    return image;
}

/*-------------------------------------------------------------------*/
float
stripeShade( const float x,
             const float z,
             const QVector3D & camera )
{
    // the stripe, and the way it was mowed: the even ones toward -z (away
    // from the near side, where the main cameras are), the odd ones back
    const int k = int( std::floor( x / float( FieldPainter::GRASS_STRIPE_WIDTH ) ) );
    const float light = ( k % 2 == 0 ? 1.0f : -1.0f );
    const QVector2D mowed( 0.0f, -light );

    // seen along it (the view from the camera to the point, level): lighter
    const QVector3D view = QVector3D( x, 0.0f, z ) - camera;
    const float length = view.length();
    const float along = ( length > 1.0e-6f
                          ? QVector2D::dotProduct( QVector2D( view.x(), view.z() ) / length, mowed )
                          : 0.0f );
    return 1.0f + STRIPE_CONTRAST * ( STRIPE_FIXED * light + ( 1.0f - STRIPE_FIXED ) * along );
}

/*-------------------------------------------------------------------*/
float
bladesFade( const float distance )
{
    return 1.0f - smoothstep( BLADES_NEAR, BLADES_FAR, distance );
}

/*-------------------------------------------------------------------*/
void
paintGround( QImage & image,
             const double scale )
{
    if ( image.format() != QImage::Format_RGBA8888 )
    {
        image = image.convertToFormat( QImage::Format_RGBA8888 );
    }
    static const QImage noise = noiseImage();
    const int n = noise.width(); // a power of 2: wraps by a mask
    const int mask = n - 1;

    const float wear_rgb[3] = { WEAR_COLOR.x() * 255.0f, WEAR_COLOR.y() * 255.0f, WEAR_COLOR.z() * 255.0f };
    const float cx = image.width() * 0.5f;
    const float cy = image.height() * 0.5f;
    const float per_pixel = float( n ) / ( PATCH_TILE * float( scale ) ); // texels of the noise a pixel
    const auto paintRows = [&]( const int first, const int last ) {
    for ( int py = first; py < last; ++py )
    {
        uchar * line = image.scanLine( py );
        const float z = ( py + 0.5f - cy ) / float( scale );
        // the noise's row of this line (bilinear between two rows), once a line
        const float v = ( z / PATCH_TILE + 0.71f ) * n - 0.5f;
        const int j = int( std::floor( v ) );
        const float tv = v - j;
        const uchar * row0 = noise.constScanLine( j & mask );
        const uchar * row1 = noise.constScanLine( ( j + 1 ) & mask );
        float u = ( ( 0.5f - cx ) / float( scale ) / PATCH_TILE + 0.37f ) * n - 0.5f;
        for ( int px = 0; px < image.width(); ++px, u += per_pixel )
        {
            const int i = int( std::floor( u ) );
            const float tu = u - i;
            const int i0 = i & mask, i1 = ( i + 1 ) & mask;
            const float patch = ( ( row0[i0] * ( 1.0f - tu ) + row0[i1] * tu ) * ( 1.0f - tv )
                                  + ( row1[i0] * ( 1.0f - tu ) + row1[i1] * tu ) * tv ) * ( 1.0f / 255.0f );
            const float x = ( px + 0.5f - cx ) / float( scale );
            const float base = wearAt( x, z );
            const float wear = ( base > 0.0f
                                 ? std::min( 1.0f, std::max( 0.0f, base * WEAR_MAX * ( 0.4f + 1.2f * patch ) ) )
                                 : 0.0f );
            const float factor = 1.0f + PATCH_AMOUNT * ( patch - 0.5f );
            uchar * c = line + px * 4;
            for ( int k = 0; k < 3; ++k )
            {
                const float value = ( c[k] + ( wear_rgb[k] - c[k] ) * wear ) * factor;
                c[k] = uchar( std::min( 255.0f, std::max( 0.0f, value ) ) + 0.5f );
            }
        }
    }
    };

    // the rows in bands, each by a thread (about 80 ms by one, on 5 million pixels)
    image.bits(); // detached once, before the threads write into it
    const int threads = int( std::min( 8u, std::max( 1u, std::thread::hardware_concurrency() ) ) );
    const int band = ( image.height() + threads - 1 ) / threads;
    std::vector< std::thread > workers;
    for ( int t = 1; t < threads; ++t )
    {
        workers.emplace_back( paintRows, std::min( image.height(), t * band ), std::min( image.height(), ( t + 1 ) * band ) );
    }
    paintRows( 0, std::min( image.height(), band ) );
    for ( std::thread & w : workers ) w.join();
}

/*-------------------------------------------------------------------*/
float
wearAt( const float x,
        const float z )
{
    // none but beyond the penalty spots' reach or within the center's (the
    // spots reach no farther): most of the pitch is out at once
    if ( std::fabs( x ) <= 39.0f && x * x + z * z >= 16.0f )
    {
        return 0.0f;
    }
    float wear = 0.0f;
    for ( const Spot & s : SPOTS )
    {
        const float d = std::hypot( ( x - s.x ) / s.rx, ( z - s.z ) / s.rz );
        wear = std::max( wear, s.amount * ( 1.0f - smoothstep( 0.0f, 1.0f, d ) ) );
    }
    return wear;
}

}
