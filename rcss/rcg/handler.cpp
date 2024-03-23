// -*-c++-*-

/*!
  \file handler.cpp
  \brief rcg data handler Source File.
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

#include "handler.h"

#include "util.h"

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <iostream>

namespace rcss {
namespace rcg {

/*-------------------------------------------------------------------*/
/*!

*/
Handler::Handler()
    : M_log_version( 0 ),
      M_read_time( 0 )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
Handler::~Handler()
{

}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Handler::handleDispInfo( const dispinfo_t & dinfo )
{
    handleLogVersion( REC_VERSION_2 );

    switch ( ntohs( dinfo.mode ) ) {
    case SHOW_MODE:
        return handleShowInfo( dinfo.body.show );
        break;
    case MSG_MODE:
        return handleMsg( M_read_time,
                          ntohs( dinfo.body.msg.board ),
                          std::string( dinfo.body.msg.message ) );
        break;
    case DRAW_MODE:
        return handleDrawInfo( dinfo.body.draw );
        break;
    default:
        std::cerr << __FILE__ << ':' << __LINE__
                  << " detect unsupported mode ["
                  << static_cast< int >( ntohs( dinfo.mode ) ) << ']'
                  << std::endl;
        break;
    }
    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Handler::handleDispInfo2( const dispinfo_t2 & dinfo2 )
{
    handleLogVersion( REC_VERSION_3 );

    switch ( ntohs( dinfo2.mode ) ) {
    case SHOW_MODE:
        return handleShowInfo2( dinfo2.body.show );
        break;
    case MSG_MODE:
        return handleMsg( M_read_time,
                          ntohs( dinfo2.body.msg.board ),
                          std::string( dinfo2.body.msg.message ) );
        break;
    case PT_MODE:
        return handlePlayerType( dinfo2.body.ptinfo );
        break;
    case PARAM_MODE:
        return handleServerParam( dinfo2.body.sparams );
        break;
    case PPARAM_MODE:
        return handlePlayerParam( dinfo2.body.pparams );
        break;
    default:
        std::cerr << __FILE__ << ':' << __LINE__
                  << " detect unsupported mode ["
                  << static_cast< int >( ntohs( dinfo2.mode ) ) << ']'
                  << std::endl;
        break;
    }
    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Handler::handleShowInfo( const showinfo_t & info )
{
    ShowInfoT show;
    convert( info, show );

    M_read_time = static_cast< int >( show.time_ );

    return ( handlePlayMode( info.pmode )
             && handleTeamInfo( info.team[0], info.team[1] )
             && handleShow( show ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Handler::handleShowInfo2( const showinfo_t2 & info )
{
    ShowInfoT show;
    convert( info, show );
    M_read_time = static_cast< int >( show.time_ );

    return ( handlePlayMode( info.pmode )
             && handleTeamInfo( info.team[0], info.team[1] )
             && handleShow( show ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Handler::handleShortShowInfo2( const short_showinfo_t2 & info )
{
    ShowInfoT show;
    convert( info, show );

    M_read_time = static_cast< int >( show.time_ );

    return handleShow( show );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Handler::handleMsgInfo( const Int16 board,
                        const std::string & msg )
{
    return handleMsg( M_read_time, board, msg );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Handler::handleDrawInfo( const drawinfo_t & draw )
{
    return handleDraw( M_read_time, draw );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Handler::handlePlayMode( char playmode )
{
    return handlePlayMode( M_read_time, static_cast< PlayMode >( playmode ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Handler::handleTeamInfo( const team_t & team_left,
                         const team_t & team_right )
{
    return handleTeam( M_read_time, TeamT( team_left ), TeamT( team_right ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Handler::handlePlayerType( const player_type_t & type )
{
    return handlePlayerType( PlayerTypeT( type ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Handler::handleServerParam( const server_params_t & param )
{
    return handleServerParam( ServerParamT( param ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Handler::handlePlayerParam( const player_params_t & param )
{
    return handlePlayerParam( PlayerParamT( param ) );
}

/*-------------------------------------------------------------------*/
bool
Handler::handlePlayMode( const int time,
                         const std::string & playmode )
{
    return handlePlayMode( time, to_playmode_enum( playmode ) );
}

} // end namespace
} // end namespace
