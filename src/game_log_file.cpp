// -*-c++-*-

/*!
  \file game_log_file.cpp
  \brief reading and parsing a game log file. Source File.
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

#include "game_log_file.h"

#include <rcss/rcg/parser.h>

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#include <fstream>
#include <iostream>

namespace {

/*!
  \brief get the size of the decompressed data of a gzip file, from its
  last 4 bytes (the size modulo 2^32), to reserve the buffer.
  \return 0 if the file is not gzip or the size is unknown
*/
std::size_t
gzipSizeHint( const std::string & path )
{
    std::ifstream fin( path, std::ios::binary );
    unsigned char magic[2] = { 0, 0 };
    if ( ! fin.read( reinterpret_cast< char * >( magic ), 2 )
         || magic[0] != 0x1f
         || magic[1] != 0x8b )
    {
        return 0;
    }

    unsigned char isize[4];
    if ( ! fin.seekg( -4, std::ios::end )
         || ! fin.read( reinterpret_cast< char * >( isize ), 4 ) )
    {
        return 0;
    }

    return static_cast< std::size_t >( isize[0] )
        | ( static_cast< std::size_t >( isize[1] ) << 8 )
        | ( static_cast< std::size_t >( isize[2] ) << 16 )
        | ( static_cast< std::size_t >( isize[3] ) << 24 );
}

}

/*-------------------------------------------------------------------*/
/*!

 */
bool
readGameLogFile( const std::string & path,
                 std::string * data )
{
    data->clear();

#ifdef HAVE_LIBZ
    // gzread() also reads a file that is not compressed, as it is.
    gzFile file = gzopen( path.c_str(), "rb" );
    if ( ! file )
    {
        return false;
    }

    const unsigned int chunk = 1u << 20;
    gzbuffer( file, chunk );
    data->reserve( gzipSizeHint( path ) + chunk );

    std::size_t size = 0;
    while ( true )
    {
        data->resize( size + chunk );
        const int n = gzread( file, &( *data )[size], chunk );
        if ( n < 0 )
        {
            int errnum = 0;
            std::cerr << "(readGameLogFile) " << path << ": " << gzerror( file, &errnum ) << std::endl;
            gzclose( file );
            data->clear();
            return false;
        }

        size += static_cast< std::size_t >( n );
        if ( static_cast< unsigned int >( n ) < chunk )
        {
            break;
        }
    }

    data->resize( size );
    gzclose( file );
    return true;
#else
    std::ifstream fin( path, std::ios::binary );
    if ( ! fin )
    {
        return false;
    }

    fin.seekg( 0, std::ios::end );
    const std::streamoff size = fin.tellg();
    fin.seekg( 0, std::ios::beg );
    if ( size < 0 )
    {
        return false;
    }

    data->resize( static_cast< std::size_t >( size ) );
    return static_cast< bool >( fin.read( &( *data )[0], size ) );
#endif
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
loadGameLogFile( const std::string & path,
                 rcss::rcg::Handler & handler )
{
    std::string data;
    if ( ! readGameLogFile( path, &data ) )
    {
        std::cerr << "failed to read the rcg file. [" << path << "]" << std::endl;
        return false;
    }

    rcss::rcg::Parser::Ptr parser = rcss::rcg::Parser::create( data.data(), data.size() );
    if ( ! parser )
    {
        return false;
    }

    return parser->parse( data.data(), data.size(), handler );
}
