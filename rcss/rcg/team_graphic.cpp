// -*-c++-*-

/*!
  \file team_graphic.cpp
  \brief team graphic data holder class Source File
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

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

#include "team_graphic.h"

#include <iostream>
#include <set>
#include <cstring>

namespace rcss {
namespace rcg {

/*-------------------------------------------------------------------*/
TeamGraphic::TeamGraphic()
    : M_width( 0 ),
      M_height( 0 )
{

}

/*-------------------------------------------------------------------*/
void
TeamGraphic::clear()
{
    M_width = 0;
    M_height = 0;

    M_tiles.clear();
}

/*-------------------------------------------------------------------*/
bool
TeamGraphic::addXpmTile( const int x,
                         const int y,
                         std::shared_ptr< XpmTile > tile )
{
    if ( x < 0
         || static_cast< int >( MAX_WIDTH / XpmTile::WIDTH ) <= x
         || y < 0
         || static_cast< int >( MAX_HEIGHT / XpmTile::HEIGHT ) <= y )
    {
        std::cerr << "(TeamGraphic::addXpmTile) Over the max index x=" << x << " y=" << y << std::endl;
        return false;
    }

    const Index idx( x, y );

    if ( M_tiles.find( idx ) != M_tiles.end() )
    {
        std::cerr << "(TeamGraphic::addXpmTile) The data already exists at (" << x << "," << y << ")" << std::endl;
        return false;
    }

    M_tiles.insert( std::pair< Index, XpmTile::Ptr >( idx, tile ) );

    if ( M_width < ( x + 1 ) * XpmTile::WIDTH )
    {
        M_width = ( x + 1 ) * XpmTile::WIDTH;
    }

    if ( M_height < ( x + 1 ) * XpmTile::HEIGHT )
    {
        M_height = ( x + 1 ) * XpmTile::HEIGHT;
    }

    return true;
}

/*-------------------------------------------------------------------*/
bool
TeamGraphic::createFromRawXpm( const char * const * xpm_data )
{
    if ( ! xpm_data ) return false;

    size_t xpm_width = 0, xpm_height = 0, xpm_colors = 0, xpm_cpp = 0;

    if ( std::sscanf( *xpm_data, " %zd %zd %zd %zd ",
                      &xpm_width, &xpm_height, &xpm_colors, &xpm_cpp ) != 4
         || xpm_width < 1
         || xpm_height < 1
         || xpm_colors < 1
         || xpm_cpp < 1 )
    {
        std::cerr << "(TeamGraphic::createXpmTiles) Illegal xpm header line [" << *xpm_data << "]"
                  << std::endl;
        return false;
    }

    if ( xpm_width % XpmTile::WIDTH != 0
         || xpm_width > MAX_WIDTH
         || xpm_height % XpmTile::HEIGHT != 0
         || xpm_height > MAX_HEIGHT
         || xpm_colors > MAX_COLOR
         || xpm_cpp != 1 )
    {
        std::cerr << "(TeamGraphic::createXpmTiles) Unsupported xpm data. \n"
                  << "  width=" << xpm_width
                  << " height=" << xpm_height
                  << " colors=" << xpm_colors
                  << " cpp=" << xpm_cpp
                  << "\n  Width must be " << XpmTile::WIDTH << " x N and less than or equal to " << MAX_WIDTH
                  << "\n  Height must be " << XpmTile::HEIGHT << " x N and less than or equel to " << MAX_HEIGHT
                  << "\n  The number of color must be less than or equal to " << MAX_COLOR
                  << "\n  And, characters per pixel must be 1."
                  << std::endl;
        return false;
    }

    ++xpm_data; // skip header line

    // check color data size
    for ( size_t i = 0; i < xpm_colors; ++i )
    {
        if ( ! xpm_data[i] )
        {
            std::cerr << "(TeamGraphic::createXpmTiles) Illegal xpm data. The number of color is illegal."
                      << std::endl;
            return false;
        }
    }

    //
    // check pixel data size
    //
    for ( size_t i = xpm_colors; i < xpm_height + xpm_colors; ++i )
    {
        if ( ! xpm_data[i]
             || std::strlen( xpm_data[i] ) != xpm_width )
        {
            std::cerr << "Illegal xpm data format." << std::endl;
            return false;
        }
    }

    //
    // clear old data
    //
    M_width = xpm_width;
    M_height = xpm_height;
    M_tiles.clear();

    std::map< char, std::string > color_map;

    //
    // register color strings
    //
    for ( size_t i = 0; i < xpm_colors; ++i, ++xpm_data )
    {
        color_map.insert( std::pair< char, std::string >( (*xpm_data)[0], std::string( *xpm_data ) ) );
    }

    //
    // create xpm tiles
    //
    const int tile_width = XpmTile::WIDTH * XpmTile::CPP;

    int index_y = 0;
    for ( size_t start_y = 0;
          start_y < xpm_height;
          start_y += XpmTile::HEIGHT, ++index_y )
    {
        size_t index_x = 0;
        for ( size_t start_x = 0;
              start_x < xpm_width * xpm_cpp;
              start_x += tile_width, ++index_x )
        {
            const size_t max_y = std::min( start_y + XpmTile::HEIGHT, xpm_height );
            const size_t max_x = std::min( start_x + tile_width, xpm_width );

            Index index( index_x, index_y );
            XpmTile::Ptr tile( new XpmTile() );

            std::set< std::string > color_set;

            for ( size_t y = start_y; y < max_y; ++y )
            {
                std::string buf( xpm_data[y], start_x, ( max_x - start_x ) );
                for ( size_t i = 0; i < buf.size(); ++i )
                {
                    color_set.insert( color_map[ buf[i] ] );
                }

                tile->addPixelLine( buf.c_str() );
            }

            char header[32];
            std::snprintf( header, 31, "%zd %zd %zd 1", XpmTile::WIDTH, XpmTile::HEIGHT, color_set.size() );
            tile->setHeader( header );

            for ( const std::string col : color_set )
            {
                tile->addColor( col.c_str() );
            }

            M_tiles.insert( std::pair< Index, XpmTile::Ptr >( index, tile ) );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
TeamGraphic::createFromServerMessage( const char * server_msg )
{
    int n_read = 0;
    char side = '?';
    int x = -1, y = -1;

    if ( std::sscanf( server_msg, "(team_graphic_%c ( %d %d %n ",
                      &side, &x, &y,
                      &n_read ) != 3
         || ( side != 'l' && side != 'r' )
         || x < 0
         || y < 0
         || n_read == 0 )
    {
        return false;
    }
    server_msg += n_read;

    if ( ( x + 1 ) * XpmTile::WIDTH > MAX_WIDTH
         || ( y + 1 ) * XpmTile::HEIGHT > MAX_HEIGHT )
    {
        std::cerr << "Unsupported xpm tile index ["
                  << x << ',' << y << "]"
                  << std::endl;
        return false;
    }

    //
    // header
    //
    n_read = 0;
    int xpm_width = 0, xpm_height = 0, xpm_colors = 0, xpm_cpp = 0;
    if ( std::sscanf( server_msg,
                      " \" %d %d %d %d \" %n ",
                      &xpm_width, &xpm_height, &xpm_colors, &xpm_cpp,
                      &n_read ) != 4
         || xpm_width != XpmTile::WIDTH
         || xpm_height != XpmTile::HEIGHT
         || xpm_colors < 1
         || xpm_cpp != 1
         || n_read == 0 )
    {
        std::cerr << "Illegal xpm header [" << server_msg << "]"
                  << std::endl;
        return false;
    }
    server_msg += n_read;

    Index index( x, y );
    XpmTile::Ptr tile( new XpmTile() );

    {
        char header[32];
        std::snprintf( header, 31, "%d %d %d %d", xpm_width, xpm_height, xpm_colors, xpm_cpp );
        tile->setHeader( header );
    }

    // colors
    for ( int i = 0; i < xpm_colors; ++i )
    {
        char color[64];
        if ( std::sscanf( server_msg, " \"%63[^\"]\" %n ",
                          color, &n_read ) != 1 )
        {
            return false;
        }
        server_msg += n_read;

        tile->addColor( color );
    }

    // pixels
    for ( int i = 0; i < xpm_height; ++i )
    {
        char line[64];
        if ( std::sscanf( server_msg, " \"%63[^\"]\" %n ",
                          line, &n_read ) != 1 )
        {
            return false;
        }
        server_msg += n_read;

        if ( static_cast< int >( std::strlen( line ) ) != xpm_width * xpm_cpp )
        {
            return false;
        }

        tile->addPixelLine( line );
    }

    // insert new tile
    M_tiles.insert( std::pair< Index, XpmTile::Ptr >( index, tile ) );

    if ( M_width < ( x + 1 ) * XpmTile::WIDTH )
    {
        M_width = ( x + 1 ) * XpmTile::WIDTH;
    }

    if ( M_height < ( y + 1 ) * XpmTile::HEIGHT )
    {
        M_height = ( y + 1 ) * XpmTile::HEIGHT;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
TeamGraphic::print( std::ostream & os ) const
{
    for ( const Map::value_type & tile : tiles() )
    {
        os << '(' << tile.first.first
           << ' ' << tile.first.second << ' ';
        tile.second->print( os, ' ' );
        os << ")\n";
    }

    return os;
}

}
}
