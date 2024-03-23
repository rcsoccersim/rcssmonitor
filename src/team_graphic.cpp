// -*-c++-*-

/*!
  \file team_graphic.cpp
  \brief team graphic data class Source File
*/

/*
 *Copyright:

 Copyright (C) Hidehisa Akiyama

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

#include "team_graphic.h"

#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cctype> // isspace


namespace {

/*-------------------------------------------------------------------*/
/*!

 */
static
bool
read_xpm_string( std::istream & is,
                 std::string & buf )
{
    char ch;
    bool quated = false;

    buf.clear();
    while ( is.get( ch ) )
    {
        if ( ch == '"' )
        {
            if ( quated )
            {
                return true;
            }
            else
            {
                quated = true;
            }
        }
        else
        {
            if ( quated )
            {
                buf += ch;
            }
        }
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
static
bool
read_xpm_header( std::istream & is,
                 std::vector< std::string > & xpm_strings,
                 int * width,
                 int * height,
                 int * n_color,
                 int * cpp )
{
    std::string buf;

    if ( ! read_xpm_string( is, buf ) )
    {
        std::cerr << __FILE__
                  << ": (read_xpm_header) could not read xpm line. ["
                  << buf << ']'
                  << std::endl;
        return false;
    }

    if ( std::sscanf( buf.c_str(), " %d %d %d %d ",
                      width, height, n_color, cpp ) != 4 )
    {
        std::cerr << __FILE__
                  << ": (read_xpm_header) Illegal xpm header [" << buf << "]"
                  << std::endl;
        return false;
    }

    if ( *width % TeamGraphic::TILE_SIZE != 0
         || *width > TeamGraphic::MAX_WIDTH
         || *height % TeamGraphic::TILE_SIZE != 0
         || *height > TeamGraphic::MAX_HEIGHT
         || *n_color < 1
         || *cpp != TeamGraphic::CPP )
    {
        std::cerr << __FILE__
                  << ": (read_xpm_header) Unsupported xpm data. [" << buf << "]\n"
                  << " width=" << *width
                  << " height=" << *height
                  << " nr_colors=" << *n_color
                  << " cpp=" << *cpp
                  << "\n  Width must be " << TeamGraphic::TILE_SIZE
                  << " x N and less then equal " << TeamGraphic::MAX_WIDTH
                  << "\n  Height must be " << TeamGraphic::TILE_SIZE
                  << " x N and less then equel " << TeamGraphic::MAX_HEIGHT
                  << "\n  The number of color must be less than equal " << TeamGraphic::MAX_COLOR
                  << "\n  And, cpp(characters per pixel) must be 1."
                  << std::endl;
        return false;
    }

    xpm_strings.push_back( buf );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
static
bool
read_xpm_colors( std::istream & is,
                 const int n_color,
                 std::vector< std::string > & xpm_strings )
{
    std::string buf;

    for ( int i = 0; i < n_color; ++i )
    {
        if ( ! read_xpm_string( is, buf )
             || buf.empty() )
        {
            std::cerr << __FILE__
                      << ": (read_xpm_colors) Illegal xpm color [" << buf << "]"
                      << std::endl;
            return false;
        }

        xpm_strings.push_back( buf );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
static
bool
read_xpm_body( std::istream & is,
               const size_t width,
               const size_t height,
               std::vector< std::string > & xpm_strings )
{
    std::string buf;

    for ( size_t i = 0; i < height; ++i )
    {
        if ( ! read_xpm_string( is, buf )
             || buf.length() != width )
        {
            std::cerr << __FILE__
                      << ": (read_xpm_body) Illegal xpm line [" << buf << "]"
                      << std::endl;
            return false;
        }

        xpm_strings.push_back( buf );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
static
bool
xpm_file_to_strings( const std::string & filepath,
                     std::vector< std::string > & xpm_strings )
{
    std::ifstream fin( filepath );
    if ( ! fin )
    {
        std::cerr << __FILE__": (xpm_file_to_strings) Could not open the xpm file [" << filepath << "]" << std::endl;
        return false;
    }

    int width, height, n_color, cpp;
    if ( ! read_xpm_header( fin,
                            xpm_strings,
                            &width, &height, &n_color, &cpp ) )
    {
        return false;
    }

    if ( ! read_xpm_colors( fin, n_color, xpm_strings ) )
    {
        return false;
    }

    if ( ! read_xpm_body( fin, width, height, xpm_strings ) )
    {
        return false;
    }

    return true;
}

}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
TeamGraphic::XpmTile::XpmTile( const int width,
                               const int height,
                               const int cpp )
    : M_width( width ),
      M_height( height ),
      M_cpp( cpp )
{
    M_pixel_lines.reserve( TILE_SIZE );
}

/*-------------------------------------------------------------------*/
std::ostream &
TeamGraphic::XpmTile::print( std::ostream & os,
                             const char separator ) const
{
    os << '"' << width()
       << ' ' << height()
       << ' ' << colors().size()
       << ' ' << cpp()
       << '"';

    for ( const std::shared_ptr< std::string > & c : colors() )
    {
        os << separator << '"' << *c << '"';
    }

    for ( const std::string & line : pixelLines() )
    {
        os << separator << '"' << line << '"';
    }

    return os;
}

/*-------------------------------------------------------------------*/
TeamGraphic::TeamGraphic()
    : M_width( 0 ),
      M_height( 0 ),
      M_cpp( CPP )
{

}

/*-------------------------------------------------------------------*/
void
TeamGraphic::clear()
{
    M_width = 0;
    M_height = 0;
    M_cpp = CPP;

    M_colors.clear();
    M_tiles.clear();
}

/*-------------------------------------------------------------------*/
bool
TeamGraphic::isValid() const
{
    if ( colors().empty()
         || tiles().empty()
         || width() <= 0
         || height() <= 0
         || M_cpp != 1 )
    {
        return false;
    }

    int max_x = 0;
    int max_y = 0;

    for ( const Map::value_type & tile : tiles() )
    {
        if ( tile.first.first > max_x ) max_x = tile.first.first;
        if ( tile.first.second > max_y ) max_y = tile.first.second;
    }

    if ( static_cast< int >( tiles().size() ) != ( max_x + 1 ) * ( max_y + 1 ) )
    {
        return false;
    }

    return true;
}


/*-------------------------------------------------------------------*/
bool
TeamGraphic::readXpmFile( const std::string & filepath )
{
    //
    // read file stream and store to string vector
    //
    std::vector< std::string > xpm_strings;
    if ( ! xpm_file_to_strings( filepath, xpm_strings ) )
    {
        std::cerr << "Illegal xpm file [" << filepath << "]"
                  << std::endl;
        return false;
    }

    //
    // create the char array
    //
    const size_t array_size = xpm_strings.size();
    char ** xpm;
    xpm = new char*[ array_size ];
    for ( size_t i = 0; i < array_size; ++i )
    {
        xpm[i] = new char[ xpm_strings[i].length() + 1 ];
        std::strcpy( xpm[i], xpm_strings[i].c_str() );
    }

    //
    // create xpm tiles
    //
    bool result = true;
    if ( ! fromRawXpm( xpm ) )
    {
        result = false;
    }

    //
    // release the char array
    //
    for ( size_t i = 0; i < array_size; ++i )
    {
        delete [] xpm[i];
    }
    delete [] xpm;

    return result;
}

/*-------------------------------------------------------------------*/
bool
TeamGraphic::fromRawXpm( const char * const * xpm_data )
{
    if ( ! xpm_data ) return false;

    int xpm_width = 0, xpm_height = 0, xpm_n_color = 0, xpm_cpp = 0;

    if ( std::sscanf( *xpm_data, " %d %d %d %d ",
                      &xpm_width, &xpm_height, &xpm_n_color, &xpm_cpp ) != 4
         || xpm_width < 1
         || xpm_height < 1
         || xpm_n_color < 1
         || xpm_cpp < 1 )
    {
        std::cerr << "Illegal xpm header line [" << *xpm_data << "]"
                  << std::endl;
        return false;
    }

    if ( xpm_width % TILE_SIZE != 0
         || xpm_width > MAX_WIDTH
         || xpm_height % TILE_SIZE != 0
         || xpm_height > MAX_HEIGHT
         || xpm_n_color > MAX_COLOR
         || xpm_cpp != CPP )
    {
        std::cerr << "Unsupported xpm data. \n"
                  << "  width=" << xpm_width
                  << " height=" << xpm_height
                  << " nr_colors=" << xpm_n_color
                  << " cpp=" << xpm_cpp
                  << "\n  Width must be " << TILE_SIZE << " x N and less then equal " << MAX_WIDTH
                  << "\n  Height must be " << TILE_SIZE << " x N and less then equel " << MAX_HEIGHT
                  << "\n  The number of color must be less than equal " << MAX_COLOR
                  << "\n  And, characters per pixel must be 1."
                  << std::endl;
        return false;
    }

    ++xpm_data; // skip header line

    // check color data size
    for ( int i = 0; i < xpm_n_color; ++i )
    {
        if ( ! xpm_data[i] )
        {
            std::cerr << "Illegal xpm data. The number of color is not valid."
                      << std::endl;
            return false;
        }
    }

    //
    // check pixel data size
    //
    for ( int i = xpm_n_color; i < xpm_height + xpm_n_color; ++i )
    {
        if ( ! xpm_data[i]
             || static_cast< int >( std::strlen( xpm_data[i] ) ) != xpm_width )
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
    M_cpp = xpm_cpp;
    M_colors.clear();
    M_tiles.clear();

    //
    // register color strings
    //
    for ( int i = 0; i < xpm_n_color; ++i, ++xpm_data )
    {
        std::shared_ptr< std::string > ptr( new std::string( *xpm_data ) );
        M_colors.push_back( ptr );
    }

    //
    // create xpm tiles
    //
    const int tile_width = TILE_SIZE * xpm_cpp;

    int index_y = 0;
    for ( int start_y = 0;
          start_y < xpm_height;
          start_y += TILE_SIZE, ++index_y )
    {
        int index_x = 0;
        for ( int start_x = 0;
              start_x < xpm_width * xpm_cpp;
              start_x += tile_width, ++index_x )
        {
            const int max_y = std::min( start_y + TILE_SIZE, xpm_height );
            const int max_x = std::min( start_x + tile_width, xpm_width );

            Index index( index_x, index_y );

            XpmTile::Ptr tile( new XpmTile( ( max_x - start_x ) / xpm_cpp, // width
                                            max_y - start_y, // height
                                            xpm_cpp ) );

            for ( int y = start_y; y < max_y; ++y )
            {
                tile->addPixelLine( std::string( xpm_data[y],
                                                 start_x,
                                                 ( max_x - start_x ) ) );
                //std::cout << '['
                //          << std::string( xpm_data[y], start_x, ( max_x - start_x ) )
                //          << "] : " << y
                //          << std::endl;
            }

            //
            // set shared color data strings
            //
            for ( const std::shared_ptr< std::string > & color : M_colors )
            {
                for ( const std::string & line : tile->pixelLines() )
                {
                    if ( line.find( (*color)[0] ) != std::string::npos )
                    {
                        tile->addColor( color );
                        break;
                    }
                }
            }

            M_tiles.insert( Map::value_type( index, tile ) );
            // std::cout << "---- tile " << M_tiles.size()
            //           << " (" << index_x << ' ' << index_y << ')'
            //           << " size=" << max_x - start_x << "x" << max_y - start_y
            //           << " (start_x,start_y)=(" << start_x << ',' << start_y << ")"
            //           << std::endl;
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
bool
TeamGraphic::addXpmTile( const int x,
                         const int y,
                         const std::vector< std::string > & xpm_tile )
{
    if ( x < 0
        || y < 0
        || ( x + 1 ) * TILE_SIZE > MAX_WIDTH
        || ( y + 1 ) * TILE_SIZE > MAX_HEIGHT )
    {
        std::cerr << "(TeamGraphic::addXpmTile) Unsupported xpm tile index [" << x << ',' << y << "]" << std::endl;
        return false;
    }

    if ( xpm_tile.empty() )
    {
        std::cerr << "(TeamGraphic::addXpmTile) Empty data." << std::endl;
        return false;
    }

    //
    // header
    //
    int n_read = 0;
    int xpm_width = 0, xpm_height = 0, xpm_n_color = 0, xpm_cpp = 0;
    if ( std::sscanf( xpm_tile[0].c_str(),
                      "  %d %d %d %d %n ",
                      &xpm_width, &xpm_height, &xpm_n_color, &xpm_cpp,
                      &n_read ) != 4
         || xpm_width != TILE_SIZE
         || xpm_height != TILE_SIZE
         || xpm_n_color < 1
         || xpm_cpp != 1
         || n_read == 0 )
    {
        std::cerr << "(TeamGraphic::addXpmTile) Illegal xpm header [" << xpm_tile[0] << "]" << std::endl;
        return false;
    }

    if ( xpm_tile.size() != size_t( 1 + xpm_n_color + xpm_height ) )
    {
        std::cerr << "(TeamGraphic::addXpmTile) Illegal xpm size. defined=" << xpm_tile[0] << " size=" << xpm_tile.size() << std::endl;
        return false;
    }

    const Index index( x, y );
    XpmTile::Ptr tile( new XpmTile( xpm_width, xpm_height, xpm_cpp ) );

    //
    // colors
    //
    const size_t color_max( 1 + xpm_n_color );
    for ( size_t i = 1; i < color_max; ++i )
    {
        std::shared_ptr< std::string > new_col( new std::string( xpm_tile[i] ) );
        std::shared_ptr< std::string > col = findColor( *new_col );
        if ( col )
        {
            tile->addColor( col );
        }
        else
        {
            M_colors.push_back( new_col );
            tile->addColor( new_col );
        }
    }

    //
    // pixels
    //
    const size_t pixel_max( 1 + xpm_n_color + xpm_height );
    const size_t line_width( xpm_width * xpm_cpp );

    for ( size_t i = color_max; i < pixel_max; ++i )
    {
        if ( xpm_tile[i].length() != line_width )
        {
            return false;
        }

        tile->addPixelLine( xpm_tile[i] );
    }

    // insert new tile
    M_tiles.insert( std::pair< Index, XpmTile::Ptr >( index, tile ) );

    if ( M_width < ( x + 1 ) * TILE_SIZE )
    {
        M_width = ( x + 1 ) * TILE_SIZE;
    }

    if ( M_height < ( y + 1 ) * TILE_SIZE )
    {
        M_height = ( y + 1 ) * TILE_SIZE;
    }

    return true;
}

/*-------------------------------------------------------------------*/
std::shared_ptr< std::string >
TeamGraphic::findColor( const std::string & str )
{
    for ( std::shared_ptr< std::string > color : M_colors )
    {
        if ( *color == str )
        {
            return color;
        }
    }

    return std::shared_ptr< std::string >();
}
