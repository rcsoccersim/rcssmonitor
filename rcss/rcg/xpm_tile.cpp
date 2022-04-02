// -*-c++-*-

/*!
  \file xpm_tile.cpp
  \brief XPM tile for team_graphic Source File.
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xpm_tile.h"

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include <cctype>

namespace rcss {
namespace rcg {

/*-------------------------------------------------------------------*/
bool
XpmTile::isValid() const
{
    return ( ! M_header.empty()
             && M_color_size > 0
             && colors().size() == M_color_size
             && pixelLines().size() == HEIGHT );
}

/*-------------------------------------------------------------------*/
bool
XpmTile::addData( const char * data )
{
    if ( M_header.empty() )
    {
        return setHeader( data );
    }

    if ( M_colors.size() < M_color_size )
    {
        return addColor( data );
    }

    return addPixelLine( data );
}

/*-------------------------------------------------------------------*/
bool
XpmTile::setHeader( const char * data )
{
    size_t width, height, color, cpp;

    if ( std::sscanf( data, " %zd %zd %zd %zd ",
                      &width, &height, &color, &cpp ) != 4
         || width != WIDTH
         || height != HEIGHT
         || color < 1
         || cpp != CPP )
    {
        std::cerr << "Illegal xpm header line " << std::quoted( data ) << std::endl;
        return false;
    }

    M_header = data;
    M_color_size = static_cast< size_t >( color );
    return true;
}

/*-------------------------------------------------------------------*/
bool
XpmTile::addColor( const char * data )
{
    if ( M_colors.size() >= M_color_size )
    {
        return false;
    }

    // An example of color data.
    // "x c #F0F0F0"

    // The first byte means the pixel type.
    // The second byte is always a white space.
    // The third byte is always 'c'.
    // The forth byte is always a white space or a tab.
    // The rest part is a color name or a HEX code.

    if ( std::strlen( data ) < CPP + 4 )
    {
        return false;
    }

    if ( ! std::isspace( data[CPP] )
         && data[CPP+1] != 'c'
         && ! std::isspace( data[CPP+2] ) )
    {
        return false;
    }

    M_colors.push_back( data );
    return true;
}

/*-------------------------------------------------------------------*/
bool
XpmTile::addPixelLine( const char * data )
{
    if ( M_pixel_lines.size() >= HEIGHT )
    {
        return false;
    }

    if ( std::strlen( data ) != WIDTH * CPP )
    {
        return false;
    }

    M_pixel_lines.push_back( data );
    return true;
}

/*-------------------------------------------------------------------*/
std::ostream &
XpmTile::print( std::ostream & os,
                const char sep ) const
{
    os << '"' << WIDTH
       << ' ' << HEIGHT
       << ' ' << colors().size()
       << ' ' << CPP
       << '"';

    for ( const std::string & c : colors() )
    {
        os << sep << std::quoted( c );
    }

    for ( const std::string & v : pixelLines() )
    {
        os << sep << std::quoted( v );
    }

    return os;
}

} // end namespace
} // end namespace
