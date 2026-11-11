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

#ifdef HAVE_LIBDEFLATE
#include <libdeflate.h>
#endif

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>

namespace {

#if defined(HAVE_LIBDEFLATE) || ! defined(HAVE_LIBZ)
/*!
  \brief read a file into memory, as it is.
*/
bool
readRawFile( const std::string & path,
             std::string * data )
{
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
    return size == 0
        || static_cast< bool >( fin.read( &( *data )[0], size ) );
}
#endif

#if defined(HAVE_LIBZ) || defined(HAVE_LIBDEFLATE)
/*!
  \brief check if data begin with the magic number of gzip.
*/
bool
isGzip( const char * data,
        const std::size_t size )
{
    return size >= 2
        && static_cast< unsigned char >( data[0] ) == 0x1f
        && static_cast< unsigned char >( data[1] ) == 0x8b;
}

/*!
  \brief get the size of the decompressed data of a gzip stream, from its
  last 4 bytes (the size of the last member, modulo 2^32).
  \return 0 if the size is unknown
*/
std::size_t
gzipSizeHint( const unsigned char * isize )
{
    return static_cast< std::size_t >( isize[0] )
        | ( static_cast< std::size_t >( isize[1] ) << 8 )
        | ( static_cast< std::size_t >( isize[2] ) << 16 )
        | ( static_cast< std::size_t >( isize[3] ) << 24 );
}
#endif

#ifdef HAVE_LIBZ
/*!
  \brief get the size hint of a gzip file, to reserve the buffer.
  \return 0 if the file is not gzip or the size is unknown
*/
std::size_t
gzipSizeHint( const std::string & path )
{
    std::ifstream fin( path, std::ios::binary );
    char magic[2] = { 0, 0 };
    if ( ! fin.read( magic, 2 )
         || ! isGzip( magic, 2 ) )
    {
        return 0;
    }

    unsigned char isize[4];
    if ( ! fin.seekg( -4, std::ios::end )
         || ! fin.read( reinterpret_cast< char * >( isize ), 4 ) )
    {
        return 0;
    }

    return gzipSizeHint( isize );
}

/*!
  \brief read a file with zlib, which decompresses a gzip file and reads
  any other file as it is. A gzip file that ends in the middle (as the
  server stopped abnormally) is read up to there.
*/
bool
readWithZlib( const std::string & path,
              std::string * data )
{
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
}
#endif

#ifdef HAVE_LIBDEFLATE
/*!
  \brief decompress gzip data in memory with libdeflate, which is a few
  times faster than zlib. The data may have more than one member, and
  bytes that are not gzip after a member are ignored, as zlib does.
  \return false if the data are broken (also when they end in the middle)
*/
bool
inflateWithLibdeflate( const std::string & compressed,
                       std::string * data )
{
    // the largest ratio of deflate is about 1032:1
    const std::size_t max_size = compressed.size() * 1032 + ( 1u << 20 );

    struct Free {
        void operator()( libdeflate_decompressor * d ) const
          {
              libdeflate_free_decompressor( d );
          }
    };
    std::unique_ptr< libdeflate_decompressor, Free > decompressor( libdeflate_alloc_decompressor() );
    if ( ! decompressor )
    {
        return false;
    }

    const char * in = compressed.data();
    std::size_t in_size = compressed.size();
    std::size_t size = 0;

    const std::size_t hint
        = ( in_size >= 4
            ? gzipSizeHint( reinterpret_cast< const unsigned char * >( in + in_size - 4 ) )
            : 0 );
    data->resize( std::min( std::max( hint, in_size ), max_size ) );

    while ( isGzip( in, in_size ) )
    {
        std::size_t in_used = 0;
        std::size_t out_size = 0;
        const libdeflate_result result
            = libdeflate_gzip_decompress_ex( decompressor.get(),
                                             in, in_size,
                                             &( *data )[0] + size, data->size() - size,
                                             &in_used, &out_size );
        if ( result == LIBDEFLATE_INSUFFICIENT_SPACE )
        {
            if ( data->size() >= max_size )
            {
                return false;
            }
            data->resize( std::min( data->size() * 2, max_size ) );
            continue;
        }

        if ( result != LIBDEFLATE_SUCCESS )
        {
            return false;
        }

        size += out_size;
        in += in_used;
        in_size -= in_used;
    }

    data->resize( size );
    return true;
}
#endif

}

/*-------------------------------------------------------------------*/
/*!

 */
bool
readGameLogFile( const std::string & path,
                 std::string * data )
{
    data->clear();

#ifdef HAVE_LIBDEFLATE
    {
        std::string raw;
        if ( ! readRawFile( path, &raw ) )
        {
            return false;
        }

        if ( ! isGzip( raw.data(), raw.size() ) )
        {
            data->swap( raw );
            return true;
        }

        if ( inflateWithLibdeflate( raw, data ) )
        {
            return true;
        }

        data->clear();
# ifndef HAVE_LIBZ
        std::cerr << "(readGameLogFile) " << path << ": broken gzip data" << std::endl;
        return false;
# endif
        // zlib reads the data up to where they are broken.
    }
#endif

#ifdef HAVE_LIBZ
    return readWithZlib( path, data );
#else
    return readRawFile( path, data );
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
