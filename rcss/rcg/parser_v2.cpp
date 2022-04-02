// -*-c++-*-

/*!
  \file parser_v2.cpp
  \brief rcg v2 parser Source File.
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

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include "parser_v2.h"

#include "handler.h"
#include "types.h"

#include <iostream>
#include <cstring>

namespace rcss {
namespace rcg {

/*-------------------------------------------------------------------*/
/*!

*/
bool
ParserV2::parse( std::istream & is,
                 Handler & handler )
{
    // streampos must be the first point!!!
    is.seekg( 0 );

    if ( ! is.good() )
    {
        return false;
    }

    // skip header
    char header[4];
    is.read( header, 4 ); // read 'U', 'L', 'G', <version>

    if ( static_cast< int >( header[3] ) != REC_VERSION_2
         || ! handler.handleLogVersion( REC_VERSION_2 ) )
    {
        return false;
    }

    // main loop
    while ( is.good() )
    {
        // read data
        if ( ! parseData( is, handler ) )
        {
            return false;
        }
    }

    if ( ! is.eof() )
    {
        return false;
    }

    return handler.handleEOF();
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
ParserV2::parseData( std::istream & is,
                     Handler & handler )
{
    Int16 mode;
    if ( ! is.read( reinterpret_cast< char * >( &mode ), sizeof( Int16 ) ) )
    {
        if ( is.eof() )
        {
            return true;
        }
        return false;
    }

    switch ( ntohs( mode ) ) {
    case NO_INFO:
        return true;

    case SHOW_MODE:
        return parseShow( is, handler );
        break;

    case MSG_MODE:
        return parseMsg( is, handler );

    case DRAW_MODE:
        return parseDraw( is, handler );

    default:
        std::cerr << "(rcss::rcg::ParserV2::parseData) Unknown mode" << ntohs( mode ) << std::endl;;
        break;
    }

    return false;
}

/*-------------------------------------------------------------------*/
bool
ParserV2::parseShow( std::istream & is,
                     Handler & handler )
{
    showinfo_t show;
    is.read( reinterpret_cast< char * >( &show ), sizeof( showinfo_t ) );

    if ( is.gcount() != sizeof( showinfo_t ) )
    {
        return false;
    }

    return handleShow( show, handler );
}

/*-------------------------------------------------------------------*/
bool
ParserV2::parseMsg( std::istream & is,
                    Handler & handler )
{
    bool result = false;

    Int16 board;
    is.read( reinterpret_cast< char * >( &board ), sizeof( Int16 ) );
    if ( is.gcount() != sizeof( Int16 ) )
    {
        return false;
    }
    board = ntohs( board );

    Int16 len;
    is.read( reinterpret_cast< char * >( &len ), sizeof( Int16 ) );
    if ( is.gcount() != sizeof( Int16 ) )
    {
        return false;
    }
    len = ntohs( len );

    char * msg = new char[len];
    is.read( msg, len );
    if ( is.gcount() == len )
    {
        if ( msg[len - 1] == 0 )
        {
            len = std::strlen( msg );
        }

        result = handler.handleMsg( M_time, board, std::string( msg, len ) );
    }

    delete [] msg;
    return result;
}


/*-------------------------------------------------------------------*/
bool
ParserV2::parseDraw( std::istream & is,
                     Handler & handler )
{
    drawinfo_t draw;
    is.read( reinterpret_cast< char * >( &draw ), sizeof( drawinfo_t ) );

    if ( is.gcount() != sizeof( drawinfo_t ) )
    {
        return false;
    }

    return handleDraw( draw, handler );
}

} // end of namespace
} // end of namespace
