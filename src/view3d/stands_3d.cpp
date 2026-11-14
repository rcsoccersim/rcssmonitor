// -*-c++-*-

/*!
  \file stands_3d.cpp
  \brief the stands of the 3D view. Source File.
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

#include "stands_3d.h"

#include <QVector2D>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace Stands3D {

namespace {

constexpr float HALF_WIDTH = 34.0f; //!< the touch line from the center (m)
constexpr float HALF_LENGTH = 52.5f; //!< the goal line from the center (m)

//! the plain bands of the image, under the rows of seats (pixels high)
constexpr int WALL_PX = 36;
constexpr int CONCRETE_PX = 12;
int wallTop() { return STAND_ROWS * ROW_PIXELS; }
int concreteTop() { return wallTop() + WALL_PX; }

//! within a row's band: the riser (behind the seated, their heads and bodies) above the tread (their legs)
constexpr int RISER_PX = 14;

//! a triangle pair a-b-c-d (around the quad) facing n, with where in the image (pixels) each corner is
void
addQuad( std::vector< Vertex3D > & v,
         const int width,
         const std::array< QVector3D, 4 > & p,
         const QVector3D & n,
         const std::array< QVector2D, 4 > & px )
{
    // counterclockwise seen from where it faces (as Meshes3D's shapes)
    const bool facing = QVector3D::dotProduct( QVector3D::crossProduct( p[1] - p[0], p[2] - p[0] ), n ) > 0.0f;
    static const int ccw[6] = { 0, 1, 2, 0, 2, 3 };
    static const int cw[6] = { 0, 2, 1, 0, 3, 2 };
    const float w = float( width ), h = float( imageHeight() );
    for ( const int i : ( facing ? ccw : cw ) )
    {
        Vertex3D vx;
        vx.position[0] = p[i].x(); vx.position[1] = p[i].y(); vx.position[2] = p[i].z();
        vx.normal[0] = n.x(); vx.normal[1] = n.y(); vx.normal[2] = n.z();
        vx.uv[0] = px[i].x() / w; vx.uv[1] = px[i].y() / h;
        v.push_back( vx );
    }
}

//! a little hash: the same crowd every time, with nothing repeating
std::uint64_t
mix( std::uint64_t x )
{
    x += 0x9e3779b97f4a7c15ull;
    x = ( x ^ ( x >> 30 ) ) * 0xbf58476d1ce4e5b9ull;
    x = ( x ^ ( x >> 27 ) ) * 0x94d049bb133111ebull;
    return x ^ ( x >> 31 );
}

//! a number in [0, 1) of a seat and a purpose
float
chance( const int stand,
        const int row,
        const int seat,
        const int what )
{
    const std::uint64_t key = ( std::uint64_t( stand + 1 ) << 56 ) ^ ( std::uint64_t( row ) << 40 )
        ^ ( std::uint64_t( seat ) << 16 ) ^ std::uint64_t( what );
    return float( mix( key ) >> 40 ) / float( 1u << 24 );
}

QColor
scaled( const QColor & c,
        const double k )
{
    const auto f = [k]( const int v ) { return std::min( 255, std::max( 0, int( std::lround( v * k ) ) ) ); };
    return QColor( f( c.red() ), f( c.green() ), f( c.blue() ) );
}

//! what the others in a crowd wear: mostly dark and plain (a color and its weight)
struct Wear {
    QRgb color;
    float weight;
};
const Wear CLOTHES[] = {
    { qRgb( 0x1c, 0x1c, 0x1e ), 18 }, // black
    { qRgb( 0x3a, 0x3c, 0x40 ), 15 }, // charcoal
    { qRgb( 0x23, 0x2c, 0x45 ), 14 }, // navy
    { qRgb( 0x3f, 0x55, 0x75 ), 12 }, // denim
    { qRgb( 0x7c, 0x7e, 0x80 ), 10 }, // gray
    { qRgb( 0xd8, 0xd6, 0xd0 ), 9 },  // white
    { qRgb( 0xa8, 0x98, 0x7a ), 6 },  // beige
    { qRgb( 0x5a, 0x40, 0x2e ), 5 },  // brown
    { qRgb( 0x4d, 0x55, 0x38 ), 4 },  // olive
    { qRgb( 0x8a, 0x26, 0x26 ), 3 },  // red
    { qRgb( 0x5c, 0x1f, 0x2e ), 2 },  // maroon
    { qRgb( 0xc8, 0xa8, 0x32 ), 2 },  // mustard
};
const QRgb SKIN[] = { qRgb( 0xe9, 0xc4, 0xa6 ), qRgb( 0xd2, 0xa0, 0x7c ), qRgb( 0xa8, 0x74, 0x50 ), qRgb( 0x6e, 0x46, 0x2e ),
                      qRgb( 0x48, 0x2e, 0x22 ) };
const QRgb HAIR = qRgb( 0x22, 0x1b, 0x16 );
const QRgb PANTS[] = { qRgb( 0x1e, 0x22, 0x2a ), qRgb( 0x2e, 0x3d, 0x58 ), qRgb( 0x3a, 0x3a, 0x3a ), qRgb( 0x6a, 0x62, 0x54 ) };

const QRgb SEAT_COLOR = qRgb( 0x2b, 0x40, 0x6c ); //!< the seats' blue
const QRgb SEAT_PAN = qRgb( 0x22, 0x33, 0x58 );
const QRgb STEP_RISER = qRgb( 0x55, 0x55, 0x58 ); //!< the concrete behind the seats
const QRgb STEP_TREAD = qRgb( 0x38, 0x39, 0x3c ); //!< the floor in front of the seats, in the shade of the row (it faces the sky: lit more than the risers)
const QRgb STAIR = qRgb( 0x74, 0x73, 0x70 ); //!< the stairs of the aisles
const QRgb STAIR_EDGE = qRgb( 0x9c, 0x94, 0x52 ); //!< their nosings, painted yellow (worn)
const QRgb TUNNEL = qRgb( 0x12, 0x12, 0x13 );
const QRgb CONCRETE = qRgb( 0x8b, 0x88, 0x83 );

QRgb
pick( const float r )
{
    float total = 0.0f;
    for ( const Wear & w : CLOTHES ) total += w.weight;
    float at = r * total;
    for ( const Wear & w : CLOTHES )
    {
        if ( at < w.weight ) return w.color;
        at -= w.weight;
    }
    return CLOTHES[0].color;
}

//! a point of a stand in the world (a along, y up, depth back from its front wall)
QVector3D
place( const Stand & s,
       const float a,
       const float y,
       const float depth )
{
    return s.out * ( s.front + depth ) + s.along * a + QVector3D( 0.0f, y, 0.0f );
}

}

/*-------------------------------------------------------------------*/
Stand
stand( const int index )
{
    const float side_length = HALF_LENGTH + Stadium3D::BOARD_OFFSET; // as far as the boards
    const float end_length = HALF_WIDTH + Stadium3D::BOARD_OFFSET;
    switch ( index ) {
    case 0: return { QVector3D( 0, 0, -1 ), QVector3D( 1, 0, 0 ), HALF_WIDTH + FRONT, side_length };
    case 1: return { QVector3D( 0, 0, 1 ), QVector3D( -1, 0, 0 ), HALF_WIDTH + FRONT, side_length };
    case 2: return { QVector3D( -1, 0, 0 ), QVector3D( 0, 0, -1 ), HALF_LENGTH + FRONT, end_length };
    default: return { QVector3D( 1, 0, 0 ), QVector3D( 0, 0, 1 ), HALF_LENGTH + FRONT, end_length };
    }
}

/*-------------------------------------------------------------------*/
float depth() { return STAND_ROWS * TREAD; }
float top() { return WALL + ( STAND_ROWS - 1 ) * RISER; }

int imageWidth( const int index ) { return int( std::lround( 2.0f * stand( index ).half_length * PIXELS_PER_METER ) ); }
int imageHeight() { return concreteTop() + CONCRETE_PX; }
int rowTop( const int k ) { return k * ROW_PIXELS; }

/*-------------------------------------------------------------------*/
bool
insideStand( const int index,
             const QVector3D & camera )
{
    const Stand s = stand( index );
    const float a = QVector3D::dotProduct( camera, s.along );
    const float d = QVector3D::dotProduct( QVector3D( camera.x(), 0.0f, camera.z() ), s.out ) - s.front;
    if ( std::fabs( a ) > s.half_length || d < 0.0f || d > depth() )
    {
        return false;
    }
    // below the row over that depth (and a little over it: the near plane)
    const int k = std::min( STAND_ROWS - 1, int( d / TREAD ) );
    return camera.y() < WALL + k * RISER + 0.5f;
}

/*-------------------------------------------------------------------*/
std::vector< Vertex3D >
mesh( const int index )
{
    std::vector< Vertex3D > v;
    const Stand s = stand( index );
    const float L = s.half_length;
    const int W = imageWidth( index );
    const QVector3D to_pitch = -s.out, up( 0.0f, 1.0f, 0.0f );
    const auto P = [&s]( const float a, const float y, const float d ) { return place( s, a, y, d ); };
    const auto band = [W]( const float top_px, const float bottom_px ) {
        return std::array< QVector2D, 4 >{ { QVector2D( 0, bottom_px ), QVector2D( float( W ), bottom_px ),
                                             QVector2D( float( W ), top_px ), QVector2D( 0, top_px ) } };
    };

    // the front wall under the first row
    addQuad( v, W, { { P( -L, 0.0f, 0.0f ), P( L, 0.0f, 0.0f ), P( L, WALL, 0.0f ), P( -L, WALL, 0.0f ) } },
             to_pitch, band( float( wallTop() ), float( wallTop() + WALL_PX ) ) );

    // the rows: each tread (its front edge at the bottom of the row's band,
    // the legs), and the riser up to the next (the bodies and heads); the
    // last tread goes back to the back wall
    for ( int k = 0; k < STAND_ROWS; ++k )
    {
        const float y = WALL + k * RISER;
        const float t = float( rowTop( k ) );
        const float d0 = k * TREAD, d1 = ( k + 1 ) * TREAD;
        addQuad( v, W, { { P( -L, y, d0 ), P( L, y, d0 ), P( L, y, d1 ), P( -L, y, d1 ) } }, up,
                 { { QVector2D( 0, t + ROW_PIXELS ), QVector2D( float( W ), t + ROW_PIXELS ),
                     QVector2D( float( W ), t + RISER_PX ), QVector2D( 0, t + RISER_PX ) } } );
        if ( k < STAND_ROWS - 1 )
        {
            addQuad( v, W, { { P( -L, y, d1 ), P( L, y, d1 ), P( L, y + RISER, d1 ), P( -L, y + RISER, d1 ) } },
                     to_pitch, band( t, t + RISER_PX ) );
        }
    }

    // closed: the back wall, out, and the ends, under each row down to the
    // ground, in concrete
    const float c0 = float( concreteTop() ) + 1.0f, c1 = float( concreteTop() + CONCRETE_PX ) - 1.0f;
    const std::array< QVector2D, 4 > concrete = { { QVector2D( 1, c1 ), QVector2D( 2, c1 ), QVector2D( 2, c0 ), QVector2D( 1, c0 ) } };
    addQuad( v, W, { { P( -L, 0.0f, depth() ), P( L, 0.0f, depth() ), P( L, top(), depth() ), P( -L, top(), depth() ) } },
             s.out, concrete );
    for ( const float e : { -1.0f, 1.0f } )
    {
        const QVector3D end = s.along * e;
        for ( int k = 0; k < STAND_ROWS; ++k )
        {
            const float d0 = k * TREAD, d1 = ( k + 1 ) * TREAD, y = WALL + k * RISER;
            addQuad( v, W, { { P( e * L, 0.0f, d0 ), P( e * L, 0.0f, d1 ), P( e * L, y, d1 ), P( e * L, y, d0 ) } },
                     end, concrete );
        }
    }
    return v;
}

/*-------------------------------------------------------------------*/
QImage
image( const int index,
       const Stadium3D::TeamLook & left,
       const Stadium3D::TeamLook & right )
{
    const Stand st = stand( index );
    const int W = imageWidth( index );
    const float L = st.half_length;
    QImage img( W, imageHeight(), QImage::Format_RGB32 );
    img.fill( QColor( STEP_RISER ) );
    const float ppm = float( PIXELS_PER_METER );
    const auto fill = [&]( const int x0, const int x1, const int y0, const int y1, const QRgb c ) {
        for ( int y = std::max( 0, y0 ); y < std::min( img.height(), y1 ); ++y )
        {
            QRgb * line = reinterpret_cast< QRgb * >( img.scanLine( y ) );
            for ( int x = std::max( 0, x0 ); x < std::min( W, x1 ); ++x ) line[x] = c;
        }
    };

    // the rows of seats, and the crowd
    const bool behind_goal = ( index >= 2 );
    const int seats = int( std::lround( 2.0f * L / SEAT ) );
    const auto in_aisle = [&]( const float a, const float margin ) {
        const float k = std::round( a / AISLE_EVERY );
        return std::fabs( a - k * AISLE_EVERY ) < AISLE_WIDTH * 0.5f + margin;
    };
    const auto in_tunnel = [&]( const float a, const int row ) {
        if ( row < 5 || row > 7 ) return false;
        for ( const float c : { -35.0f, -7.0f, 7.0f, 35.0f } )
        {
            if ( std::fabs( c ) + 1.5f < L && std::fabs( a - c ) < 1.3f ) return true;
        }
        return false;
    };
    for ( int r = 0; r < STAND_ROWS; ++r )
    {
        const int top_px = rowTop( r );
        fill( 0, W, top_px, top_px + RISER_PX, STEP_RISER );
        fill( 0, W, top_px + RISER_PX, top_px + ROW_PIXELS, STEP_TREAD );
        for ( int i = 0; i < seats; ++i )
        {
            const float a = -L + ( i + 0.5f ) * SEAT;
            const int p0 = int( std::floor( ( a - SEAT * 0.5f + L ) * ppm ) );
            const int p1 = std::min( W, int( std::floor( ( a + SEAT * 0.5f + L ) * ppm ) ) );
            const int w = p1 - p0;
            if ( w <= 0 ) continue;
            if ( in_aisle( a, 0.0f ) )
            {
                // the stairs: two steps a row, their nosings yellow
                fill( p0, p1, top_px, top_px + ROW_PIXELS, STAIR );
                fill( p0, p1, top_px + RISER_PX - 1, top_px + RISER_PX, STAIR_EDGE );
                fill( p0, p1, top_px + ROW_PIXELS - 1, top_px + ROW_PIXELS, STAIR_EDGE );
                continue;
            }
            if ( in_aisle( a, SEAT * 0.5f ) ) continue; // the gap by the stairs
            if ( in_tunnel( a, r ) )
            {
                // the mouth of a vomitory, a lintel over it in its top row
                fill( p0, p1, top_px, top_px + ROW_PIXELS, TUNNEL );
                if ( r == 7 ) fill( p0, p1, top_px, top_px + 3, qRgb( 0x6a, 0x6a, 0x6c ) );
                continue;
            }
            // the seat: its back against the riser, its pan on the tread
            fill( p0 + 1, p1 - 1, top_px + 6, top_px + RISER_PX, SEAT_COLOR );
            fill( p0 + 1, p1 - 1, top_px + RISER_PX, top_px + RISER_PX + 4, SEAT_PAN );

            // taken: along a side, more in the middle than toward the
            // corners; behind a goal, about as full all along
            const float edge = std::fabs( a ) / L;
            const float taken = ( behind_goal ? 0.88f - 0.25f * edge * edge : 0.93f - 0.5f * edge * edge * edge )
                - 0.006f * r;
            if ( chance( index, r, i, 1 ) > taken ) continue;

            // a fan, of the team whose end is nearer (behind a goal, mostly
            // the team that keeps it), or one of the others
            const QVector3D at = place( st, a, 0.0f, r * TREAD );
            const float fan_share = ( behind_goal ? 0.7f : 0.45f + 0.2f * edge );
            QRgb shirt;
            if ( chance( index, r, i, 2 ) < fan_share )
            {
                const bool of_left = chance( index, r, i, 3 ) < 1.0f / ( 1.0f + std::exp( at.x() / 14.0f ) );
                const QColor team = ( of_left ? left : right ).color;
                const float t = chance( index, r, i, 4 );
                shirt = ( t < 0.7f ? scaled( team, 0.9 + 0.2 * chance( index, r, i, 5 ) )
                          : t < 0.85f ? scaled( team, 0.6 ) : pick( chance( index, r, i, 5 ) ) ).rgb();
            }
            else
            {
                shirt = pick( chance( index, r, i, 5 ) );
            }
            const QRgb skin = SKIN[int( chance( index, r, i, 6 ) * 5 ) % 5];
            const QRgb pants = PANTS[int( chance( index, r, i, 7 ) * 4 ) % 4];
            const QRgb shade = scaled( QColor( shirt ), 0.72 ).rgb();
            const int jitter = int( chance( index, r, i, 8 ) * 3.0f ) - 1;
            const int cx = p0 + w / 2 + jitter;
            const int t = top_px;
            // seated, seen from the pitch: the legs and the shoes on the
            // tread, the body, the arms and the hands on the lap, the neck
            // and the head (its corners cut round) up the riser
            fill( cx - 4, cx + 4, t + RISER_PX, t + RISER_PX + 3, pants ); // the thighs and the knees
            fill( cx - 3, cx - 1, t + RISER_PX, t + RISER_PX + 6, pants );
            fill( cx + 1, cx + 3, t + RISER_PX, t + RISER_PX + 6, pants );
            fill( cx - 3, cx - 1, t + RISER_PX + 6, t + RISER_PX + 7, qRgb( 0x18, 0x18, 0x18 ) );
            fill( cx + 1, cx + 3, t + RISER_PX + 6, t + RISER_PX + 7, qRgb( 0x18, 0x18, 0x18 ) );
            fill( cx - 3, cx + 3, t + 7, t + RISER_PX + 1, shirt );
            fill( cx - 4, cx + 4, t + 8, t + 10, shirt ); // the shoulders
            fill( cx - 4, cx - 3, t + 8, t + RISER_PX, shade ); // the arms, in the shade a little
            fill( cx + 3, cx + 4, t + 8, t + RISER_PX, shade );
            fill( cx - 3, cx - 1, t + RISER_PX - 1, t + RISER_PX + 1, skin ); // the hands
            fill( cx - 1, cx + 1, t + 6, t + 7, skin ); // the neck
            fill( cx - 2, cx + 2, t + 2, t + 6, skin );
            fill( cx - 1, cx + 1, t + 1, t + 2, skin );
            if ( chance( index, r, i, 9 ) < 0.6f )
            {
                fill( cx - 2, cx + 2, t + 2, t + 3, HAIR );
                fill( cx - 1, cx + 1, t + 1, t + 2, HAIR );
            }
        }
    }

    // the front wall: dark, a pale line at its top; the concrete
    fill( 0, W, wallTop(), wallTop() + WALL_PX, qRgb( 0x24, 0x2e, 0x2a ) );
    fill( 0, W, wallTop(), wallTop() + 3, qRgb( 0xb8, 0xb8, 0xb0 ) );
    fill( 0, W, concreteTop(), concreteTop() + CONCRETE_PX, CONCRETE );
    return img;
}

}
