// -*-c++-*-

/*!
  \file parser_v1.cpp
  \brief rcg v1 parser Source File.
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

#include "parser_v1.h"

#include "handler.h"
#include "types.h"
#include "util.h"

#include <iostream>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

namespace rcss {
namespace rcg {

/*-------------------------------------------------------------------*/
ParserV1::ParserV1()
    : M_time( -1 )
{

}

/*-------------------------------------------------------------------*/
bool
ParserV1::parse( std::istream & is,
                 Handler & handler )
{
    // streampos must be the first point!!!
    is.seekg( 0 );

    if ( ! is.good() )
    {
        return false;
    }

    // register log version
    // Because rcg v1 does not contain a version information,
    // REC_OLD_VERSION is always set.
    if ( ! handler.handleLogVersion( REC_OLD_VERSION ) )
    {
        return false;
    }

    while ( is.good() )
    {
        dispinfo_t disp;
        is.read( reinterpret_cast< char * >( &disp ), sizeof( dispinfo_t ) );
        if ( is.gcount() != sizeof( dispinfo_t ) )
        {
            if ( is.eof() )
            {
                break;
            }

            return false;
        }

        if ( ! handleDisp( disp, handler ) )
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
ParserV1::handleDisp( const dispinfo_t & disp,
                      Handler & handler )

{
    const Int16 mode = ntohs( disp.mode );
    switch ( mode ) {
    case NO_INFO:
        return true;

    case SHOW_MODE:
        return handleShow( disp.body.show, handler  );

    case MSG_MODE:
        return handleMsg( disp.body.msg, handler );

    case DRAW_MODE:
        return handleDraw( disp.body.draw, handler );

    case BLANK_MODE:
        return true;

    default:
        std::cerr << "(rcss::rcg::ParserV1::parseDisp) Unknown mode [" << mode <<  "]" << std::endl;
        break;
    }

    return false;
}

/*-------------------------------------------------------------------*/
bool
ParserV1::handleShow( const showinfo_t & show,
                      Handler & handler )

{
    ShowInfoT new_show;
    TeamT team_l, team_r;

    const PlayMode pmode = static_cast< PlayMode >( show.pmode );
    convert( show, new_show );
    convert( show.team[0], team_l );
    convert( show.team[1], team_r );

    M_time = new_show.time_;

    return ( handler.handlePlayMode( new_show.time_, pmode )
             && handler.handleTeam( new_show.time_, team_l, team_r )
             && handler.handleShow( new_show ) );
}

/*-------------------------------------------------------------------*/
bool
ParserV1::handleMsg( const msginfo_t & msg,
                     Handler & handler )
{
    handler.handleMsg( M_time,
                       ntohs( msg.board ),
                       msg.message );
    return true;
}

/*-------------------------------------------------------------------*/
bool
ParserV1::handleDraw( const drawinfo_t & draw,
                      Handler & handler )

{
    switch ( ntohs( draw.mode ) ) {
    case DrawClear:
        handler.handleDrawClear( M_time );
        return true;

    case DrawPoint:
        handler.handleDrawPoint( M_time,
                                 PointT( nstohf( draw.object.pinfo.x ),
                                         nstohf( draw.object.pinfo.y ),
                                         draw.object.pinfo.color ) );
        return true;
    case DrawCircle:
        handler.handleDrawCircle( M_time,
                                  CircleT( nstohf( draw.object.cinfo.x ),
                                           nstohf( draw.object.cinfo.y ),
                                           nstohf( draw.object.cinfo.r ),
                                           draw.object.cinfo.color ) );
        return true;
    case DrawLine:
        handler.handleDrawLine( M_time,
                                LineT( nstohf( draw.object.linfo.x1 ),
                                       nstohf( draw.object.linfo.y1 ),
                                       nstohf( draw.object.linfo.x2 ),
                                       nstohf( draw.object.linfo.y2 ),
                                       draw.object.linfo.color ) );
        return true;
    default:
        std::cerr << "(rcss::rcg::ParserV1::parseDraw) Unknown draw mode " << ntohs( draw.mode )
                  << std::endl;
        return false;
    }

    return true;
}

} // end of namespace
} // end of namespace
