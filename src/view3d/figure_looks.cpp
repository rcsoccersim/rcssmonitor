// -*-c++-*-

/*!
  \file figure_looks.cpp
  \brief each player's looks as a human body. Source File.
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

#include "figure_looks.h"

#include <algorithm>
#include <cmath>

namespace FigureLooks {

namespace {

/*!
  \brief the colors of the skin, from light to dark: a player's is
  somewhere between two of them next to each other
*/
const QVector4D SKINS[] = {
    QVector4D( 0.96f, 0.80f, 0.69f, 1.0f ),
    QVector4D( 0.91f, 0.72f, 0.59f, 1.0f ),
    QVector4D( 0.84f, 0.64f, 0.50f, 1.0f ), // the standard
    QVector4D( 0.72f, 0.52f, 0.38f, 1.0f ),
    QVector4D( 0.56f, 0.38f, 0.26f, 1.0f ),
    QVector4D( 0.40f, 0.26f, 0.18f, 1.0f ),
};
constexpr int SKIN_COUNT = sizeof( SKINS ) / sizeof( SKINS[0] );

//! the colors of the hair, and how often each is drawn
struct HairColor {
    QVector4D color;
    double weight;
};
const HairColor HAIRS[] = {
    { QVector4D( 0.05f, 0.04f, 0.035f, 1.0f ), 0.30 }, // black
    { QVector4D( 0.10f, 0.06f, 0.035f, 1.0f ), 0.30 }, // dark brown (the standard)
    { QVector4D( 0.26f, 0.16f, 0.09f, 1.0f ), 0.18 }, // brown
    { QVector4D( 0.60f, 0.46f, 0.27f, 1.0f ), 0.08 }, // dark blond
    { QVector4D( 0.80f, 0.67f, 0.40f, 1.0f ), 0.08 }, // blond
    { QVector4D( 0.46f, 0.19f, 0.08f, 1.0f ), 0.06 }, // red
};

/*!
  \class Lot
  \brief numbers drawn by lot from a seed (splitmix64): the same seed, the
  same numbers
*/
class Lot {
public:
    explicit Lot( std::uint64_t seed ) : M_state( seed ) { }
    //! from 0 up to 1
    double next()
      {
          std::uint64_t z = ( M_state += 0x9E3779B97F4A7C15ull );
          z = ( z ^ ( z >> 30 ) ) * 0xBF58476D1CE4E5B9ull;
          z = ( z ^ ( z >> 27 ) ) * 0x94D049BB133111EBull;
          z ^= z >> 31;
          return double( z >> 11 ) * ( 1.0 / 9007199254740992.0 );
      }
private:
    std::uint64_t M_state;
};

//! a hash (FNV-1a, 64 bits) of the team, the side and the number
std::uint64_t
seedOf( const rcss::rcg::Side side,
        const int unum,
        const std::string & team )
{
    std::uint64_t h = 0xcbf29ce484222325ull;
    auto add = [&h]( const unsigned char c ) {
        h ^= c;
        h *= 0x100000001b3ull;
    };
    for ( const char c : team ) add( static_cast< unsigned char >( c ) );
    add( '|' );
    add( side == rcss::rcg::LEFT ? 'l' : side == rcss::rcg::RIGHT ? 'r' : 'n' );
    add( static_cast< unsigned char >( unum & 0xff ) );
    return h;
}

/*!
  \brief the bodies of a sex: its own and anyone's, in the order of the
  pack; all of them if none is
*/
std::vector< int >
bodiesOf( const FigureCast & cast,
          const FigureSex sex )
{
    std::vector< int > list;
    for ( std::size_t i = 0; i < cast.bodies.size(); ++i )
    {
        if ( cast.bodies[i].sex == sex
             || cast.bodies[i].sex == FigureSex::Any )
        {
            list.push_back( int( i ) );
        }
    }
    if ( list.empty() )
    {
        for ( std::size_t i = 0; i < cast.bodies.size(); ++i ) list.push_back( int( i ) );
    }
    return list;
}

//! one of a list by a number drawn from 0 up to 1
template < typename T >
const T &
pick( const std::vector< T > & list,
      const double along )
{
    const int n = static_cast< int >( list.size() );
    return list[std::min( n - 1, std::max( 0, int( along * n ) ) )];
}

QVector4D
mix( const QVector4D & a,
     const QVector4D & b,
     const float t )
{
    return a + ( b - a ) * t;
}

}

/*-------------------------------------------------------------------*/
FigureLook
standard( const FigureBodies bodies,
          const FigureCast & cast )
{
    FigureLook look; // the standard colors
    look.body = 0;
    look.hair = 0;
    if ( ! cast.bodies.empty() )
    {
        look.body = bodiesOf( cast, bodies == FigureBodies::Female ? FigureSex::Female : FigureSex::Male ).front();
        look.hair = cast.bodies[look.body].standard_hair;
    }
    return look;
}

/*-------------------------------------------------------------------*/
FigureLook
of( const rcss::rcg::Side side,
    const int unum,
    const std::string & team,
    const FigureBodies bodies,
    const FigureCast & cast )
{
    Lot lot( seedOf( side, unum, team ) );
    FigureLook look;

    // the body, by the option: of men and women (each half), or of one of
    // them; one of the bodies of that sex by the same number, so that as
    // many numbers are drawn for any pack
    const double sex = lot.next();
    look.body = 0;
    if ( ! cast.bodies.empty() )
    {
        if ( bodies == FigureBodies::Female )
        {
            look.body = pick( bodiesOf( cast, FigureSex::Female ), sex );
        }
        else if ( bodies == FigureBodies::Male )
        {
            look.body = pick( bodiesOf( cast, FigureSex::Male ), sex );
        }
        else if ( sex < 0.5 )
        {
            look.body = pick( bodiesOf( cast, FigureSex::Female ), sex / 0.5 );
        }
        else
        {
            look.body = pick( bodiesOf( cast, FigureSex::Male ), ( sex - 0.5 ) / 0.5 );
        }
    }

    // one of the body's hairstyles; the first extra (a man's beard) by the
    // next number, the others after the colors
    const double style = lot.next();
    const double first_extra = lot.next();
    const FigureCast::Body * body = ( cast.bodies.empty() ? nullptr : &cast.bodies[look.body] );
    look.hair = 0;
    if ( body && ! body->hairstyles.empty() )
    {
        look.hair = pick( body->hairstyles, style );
    }

    // the skin anywhere along its colors
    const double along = lot.next() * ( SKIN_COUNT - 1 );
    const int k = std::min( SKIN_COUNT - 2, int( along ) );
    look.skin = mix( SKINS[k], SKINS[k + 1], float( along - k ) );

    // the hair's color by how often each is
    double total = 0.0;
    for ( const HairColor & h : HAIRS ) total += h.weight;
    double draw = lot.next() * total;
    look.hair_color = HAIRS[0].color;
    for ( const HairColor & h : HAIRS )
    {
        if ( draw < h.weight )
        {
            look.hair_color = h.color;
            break;
        }
        draw -= h.weight;
    }

    if ( body )
    {
        for ( std::size_t e = 0; e < body->extras.size(); ++e )
        {
            const double chance = ( e == 0 ? first_extra : lot.next() );
            if ( chance < body->extras[e].chance ) look.hair |= body->extras[e].bit;
        }
    }
    return look;
}

/*-------------------------------------------------------------------*/
FigureLook
blend( const FigureLook & own,
       const FigureLook & standard,
       const float certainty )
{
    // the own ones for sure, the standard ones not at all
    if ( certainty >= 1.0f ) return own;
    if ( certainty <= 0.0f ) return standard;
    const float t = certainty;
    FigureLook look;
    // the body and the hairstyle cannot be between: the standard ones
    look.body = standard.body;
    look.hair = standard.hair;
    look.skin = mix( standard.skin, own.skin, t );
    look.hair_color = mix( standard.hair_color, own.hair_color, t );
    return look;
}

}
