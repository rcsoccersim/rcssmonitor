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

#include "parser_v2.h"

#include "handler.h"
#include "types.h"

#include <iostream>
#include <cstring>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

namespace rcss {
namespace rcg {

/*-------------------------------------------------------------------*/
/*!

*/
bool
ParserV2::parse( std::istream & is,
                 Handler & handler ) const
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

    if ( is.eof() )
    {
        return handler.handleEOF();
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
ParserV2::parseData( std::istream & is,
                     Handler & handler ) const
{
    // chedk data mode.
    dispinfo_t info;
    is.read( reinterpret_cast< char * >( &info.mode ), sizeof( Int16 ) );

    if ( ! is.good() )
    {
        if ( is.eof() )
        {
            return true;
        }
        return false;
    }

    // read each data block
    switch ( ntohs( info.mode ) ) {
    case NO_INFO:
        return true;
    case SHOW_MODE:
        is.read( reinterpret_cast< char* >( &info.body.show ),
                 sizeof( showinfo_t ) );
        if ( is.gcount() == sizeof( showinfo_t ) )
        {
            return handler.handleShowInfo( info.body.show );
        }
        break;
    case MSG_MODE:
        return parseMsgInfo( is, handler );
        break;
    case DRAW_MODE:
        is.read( reinterpret_cast< char* >( &info.body.draw ),
                 sizeof( drawinfo_t ) );
        if ( is.gcount() == sizeof( drawinfo_t ) )
        {
            return true;
        }
        break;
    default:
        std::cerr << __FILE__ << ':' << __LINE__
                  << " Unknown mode" << htons( info.mode )
                  << std::endl;;
        break;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
ParserV2::parseMsgInfo( std::istream & is,
                        Handler & handler ) const
{
    bool result = false;

    Int16 board;
    is.read( reinterpret_cast< char* >( &board ), sizeof( Int16 ) );
    if ( is.gcount() != sizeof( Int16 ) )
    {
        return false;
    }
    board = ntohs( board );

    Int16 len;
    is.read( reinterpret_cast< char* >( &len ), sizeof( Int16 ) );
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

        result = handler.handleMsgInfo( board, std::string( msg, len ) );
    }

    delete [] msg;
    return result;
}

} // end of namespace
} // end of namespace
