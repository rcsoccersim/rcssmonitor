// -*-c++-*-

/*!
  \file parser.cpp
  \brief abstract rcg parser class Source File.
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

#include "parser.h"

#include "types.h"

#include "parser_v1.h"
#include "parser_v2.h"
#include "parser_v3.h"
#include "parser_v4.h"
#include "parser_simdjson.h"

#include <iostream>
#include <fstream>
#include <streambuf>

namespace rcss {
namespace rcg {

namespace {

/*!
  \class MemoryBuf
  \brief a read-only stream buffer over data in memory, without copying it.
*/
class MemoryBuf
    : public std::streambuf {
public:

    MemoryBuf( const char * data,
               const std::size_t size )
      {
          char * begin = const_cast< char * >( data );
          setg( begin, begin, begin + size );
      }

protected:

    pos_type seekoff( off_type off,
                      std::ios_base::seekdir dir,
                      std::ios_base::openmode which ) override
      {
          if ( ! ( which & std::ios_base::in ) )
          {
              return pos_type( off_type( -1 ) );
          }

          char * base = ( dir == std::ios_base::beg ? eback()
                          : dir == std::ios_base::cur ? gptr()
                          : egptr() );
          if ( off < eback() - base
               || egptr() - base < off )
          {
              return pos_type( off_type( -1 ) );
          }

          setg( eback(), base + off, egptr() );
          return pos_type( gptr() - eback() );
      }

    pos_type seekpos( pos_type pos,
                      std::ios_base::openmode which ) override
      {
          return seekoff( off_type( pos ), std::ios_base::beg, which );
      }
};

}

/*-------------------------------------------------------------------*/
/*!

*/
Parser::Ptr
Parser::create( std::istream & is )
{
    char header[5];
    int version = REC_OLD_VERSION;

    is.read( header, 4 ); // read 'U', 'L', 'G', <version>

    if ( is.gcount() != 4 )
    {
        std::cerr << "(rcss::rcg::Parser::create) no header." << std::endl;
        return Parser::Ptr();
    }

    if ( header[0] == '[' )
    {
        version = REC_VERSION_JSON;
    }
    else if ( header[0] == 'U'
              && header[1] == 'L'
              && header[2] == 'G' )
    {
        version = static_cast< int >( header[3] );
    }

    std::cerr << "(rcss::rcg::Parser::create) rcg version = ";
    if ( version == -1 )
    {
        std::cerr << "json";
    }
    else
    {
        std::cerr << ( version == static_cast< int >( '0' ) + REC_VERSION_6 ? REC_VERSION_6
                       : version == static_cast< int >( '0' ) + REC_VERSION_5 ? REC_VERSION_5
                       : version == static_cast< int >( '0' ) + REC_VERSION_4 ? REC_VERSION_4
                       : version );
    }
    std::cerr << std::endl;

    Parser::Ptr ptr;

    if ( version == static_cast< int >( '0' ) + REC_VERSION_6 )
    {
        ptr = Parser::Ptr( new ParserV4() );
    }
    else if ( version == static_cast< int >( '0' ) + REC_VERSION_5 )
    {
        ptr = Parser::Ptr( new ParserV4() );
    }
    else if ( version == static_cast< int >( '0' ) + REC_VERSION_4 )
    {
        ptr = Parser::Ptr( new ParserV4() );
    }
    else if ( version == REC_VERSION_3 )
    {
        ptr = Parser::Ptr( new ParserV3() );
    }
    else if ( version == REC_VERSION_2 )
    {
        ptr = Parser::Ptr( new ParserV2() );
    }
    else if ( version == REC_OLD_VERSION )
    {
        ptr = Parser::Ptr( new ParserV1() );
    }
    else if ( version == REC_VERSION_JSON )
    {
        ptr = Parser::Ptr( new ParserSimdJSON() );
    }
    else
    {
        std::cerr << "(rcss::rcg::Parser::create) Unsupported version." << std::endl;
    }

    return ptr;
}

/*-------------------------------------------------------------------*/
/*!

*/
Parser::Ptr
Parser::create( const char * data,
                const std::size_t size )
{
    MemoryBuf buf( data, size );
    std::istream is( &buf );
    return create( is );
}


bool
Parser::parse( const std::string & filepath,
               Handler & handler ) const
{
    std::ifstream fin( filepath );
    if ( ! fin )
    {
        return false;
    }

    return parse( fin, handler );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Parser::parse( const char * data,
               const std::size_t size,
               Handler & handler ) const
{
    MemoryBuf buf( data, size );
    std::istream is( &buf );
    return parse( is, handler );
}


}
}
