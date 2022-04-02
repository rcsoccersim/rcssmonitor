// -*-c++-*-

/*!
  \file parser_v3.cpp
  \brief rcg v3 parser Source File.
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

#include "parser_v3.h"

#include "handler.h"
#include "types.h"
#include "util.h"

#include <iostream>
#include <cstring>

namespace rcss {
namespace rcg {

/*-------------------------------------------------------------------*/
ParserV3::ParserV3()
    : M_time( -1 )
{

}

/*-------------------------------------------------------------------*/
bool
ParserV3::parse( std::istream & is,
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

    if ( static_cast< int >( header[3] ) != REC_VERSION_3
         || ! handler.handleLogVersion( REC_VERSION_3 ) )
    {
        return false;
    }

    while ( is.good() )
    {
        //std::cerr << "loop... " << std::endl;
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
bool
ParserV3::parseData( std::istream & is,
                     Handler & handler )
{
    // chedk data mode.
    Int16 mode;
    is.read( reinterpret_cast< char * >( &mode ), sizeof( Int16 ) );

    if ( ! is.good() )
    {
        //std::cerr << "istream become bad " << std::endl;
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
        return parseShortShow( is, handler );

    case MSG_MODE:
        return parseMsg( is, handler );

    case PM_MODE:
        return parsePlayMode( is, handler );

    case TEAM_MODE:
        return parseTeam( is, handler );

    case PT_MODE:
        return parsePlayerType( is, handler );

    case PARAM_MODE:
        return parseServerParam( is, handler );

    case PPARAM_MODE:
        return parsePlayerParam( is, handler );

    case BLANK_MODE:
        std::cerr << "(rcss::rcg::ParserV3) BLANK mode is not supported." << std::endl;;
        break;

    case DRAW_MODE:
        std::cerr << "(rcss::rcg::ParserV3) DRAW mode is not supported." << std::endl;;
        break;

    default:
        std::cerr << "(rcss::rcg::ParserV3) Unknown mode" << ntohs( mode ) << std::endl;;
        break;

    }

    return false;
}

/*-------------------------------------------------------------------*/
bool
ParserV3::parseShortShow( std::istream & is,
                          Handler & handler )
{
    short_showinfo_t2 short_show;
    is.read( reinterpret_cast< char * >( &short_show ), sizeof( short_showinfo_t2 ) );

    if ( is.gcount() != sizeof( short_showinfo_t2 ) )
    {
        return false;
    }

    ShowInfoT new_show;
    convert( short_show, new_show );

    M_time = static_cast< int >( new_show.time_ );

    return handler.handleShow( new_show );
}

/*-------------------------------------------------------------------*/
bool
ParserV3::parseMsg( std::istream & is,
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
ParserV3::parsePlayMode( std::istream & is,
                         Handler & handler )
{
    char pmode;
    is.read( reinterpret_cast< char * >( &pmode ), sizeof( char ) );

    if ( is.gcount() != sizeof( char ) )
    {
        return false;
    }

    return handler.handlePlayMode( M_time, static_cast< PlayMode >( pmode ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
ParserV3::parseTeam( std::istream & is,
                     Handler & handler )
{
    team_t team[2];
    is.read( reinterpret_cast< char * >( team ), sizeof( team_t ) * 2 );

    if ( is.gcount() != sizeof( team_t ) * 2 )
    {
        return false;
    }

    TeamT team_l, team_r;
    convert( team[0], team_l );
    convert( team[1], team_r );

    return handler.handleTeam( M_time, team_l, team_r );
}

/*-------------------------------------------------------------------*/
bool
ParserV3::parsePlayerType( std::istream & is,
                           Handler & handler )
{
    player_type_t ptype;
    std::memset( &ptype, 0, sizeof( player_type_t ) );
    is.read( reinterpret_cast< char * >( &ptype ), sizeof( player_type_t ) );

    if ( is.gcount() != sizeof( player_type_t ) )
    {
        return false;
    }

    PlayerTypeT new_type;
    convert( ptype, new_type );

    return handler.handlePlayerType( new_type );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
ParserV3::parseServerParam( std::istream & is,
                            Handler & handler )
{
    server_params_t sparams;
    std::memset( &sparams, 0, sizeof( server_params_t ) );
    is.read( reinterpret_cast< char * >( &sparams ), sizeof( server_params_t ) );

    if ( is.gcount() != sizeof( server_params_t ) )
    {
        return false;
    }

    ServerParamT new_params;
    convert( sparams, new_params );

    return handler.handleServerParam( new_params );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
ParserV3::parsePlayerParam( std::istream & is,
                            Handler & handler )
{
    player_params_t pparams;
    std::memset( &pparams, 0, sizeof( player_params_t ) );
    is.read( reinterpret_cast< char * >( &pparams ), sizeof( pparams ) );

    if ( is.gcount() != sizeof( player_params_t ) )
    {
        return false;
    }

    PlayerParamT new_params;
    convert( pparams, new_params );

    return handler.handlePlayerParam( new_params );
}

} // end of namespace
} // end of namespace
