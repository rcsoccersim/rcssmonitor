// -*-c++-*-

/*!
  \file rcg_handler.cpp
  \brief display data holder class Source File.
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
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.	If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "rcg_handler.h"

#include "rcss/rcg/util.h"
#include "disp_holder.h"

#include <iostream>

#include <QtGlobal>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

namespace {

//! the progress dialog is shown only after this [ms]
const int PROGRESS_DELAY_MS = 500;

//! the shortest interval of the progress updates [ms]
const qint64 PROGRESS_INTERVAL_MS = 100;

}

/*-------------------------------------------------------------------*/
RCGHandler::RCGHandler( QWidget * parent,
                        DispHolder & holder )
    : M_holder( holder ),
      M_progress( nullptr ),
      M_next_progress( PROGRESS_INTERVAL_MS )
{
    M_timer.start();

    if ( parent )
    {
        M_progress = new QProgressDialog( parent );
        M_progress->setWindowTitle( QObject::tr( "parsing a game log file..." ) );
        M_progress->setRange( 0, 6000 );
        M_progress->setValue( 0 );
        M_progress->setLabelText( QObject::tr( "Time: 0" ) );
        M_progress->setCancelButton( 0 ); // no cancel button
        // shown only when loading takes longer than this
        M_progress->setMinimumDuration( PROGRESS_DELAY_MS );
    }
}

/*-------------------------------------------------------------------*/
RCGHandler::~RCGHandler()
{
    if ( M_progress )
    {
        delete M_progress;
        M_progress = nullptr;
    }
}

/*-------------------------------------------------------------------*/
bool
RCGHandler::handleEOF()
{
    return M_holder.handleEOF();
}

/*-------------------------------------------------------------------*/
bool
RCGHandler::handleShow( const rcss::rcg::ShowInfoT & show )
{
    // Update the progress at most every PROGRESS_INTERVAL_MS. Updating it
    // every 32 cycles (and processing the events, which repaints the main
    // window, every 512) took longer than parsing the log.
    if ( M_progress
         && M_timer.elapsed() >= M_next_progress )
    {
        M_next_progress = M_timer.elapsed() + PROGRESS_INTERVAL_MS;

        const int time = static_cast< int >( show.time_ );
        if ( time > M_progress->maximum() )
        {
            M_progress->setMaximum( ( time / 6000 + 1 ) * 6000 );
        }
        M_progress->setValue( time );
        M_progress->setLabelText( QString( "Time: %1" ).arg( time ) );

        qApp->processEvents();
        std::fprintf( stdout, "parsing... %d\r", time );
        std::fflush( stdout );
    }

    return M_holder.handleShow( show );
}

/*-------------------------------------------------------------------*/
bool
RCGHandler::handlePlayMode( const int time,
                            const rcss::rcg::PlayMode pmode )
{
    return M_holder.handlePlayMode( time, pmode );
}

/*-------------------------------------------------------------------*/
bool
RCGHandler::handleTeam( const int time,
                        const rcss::rcg::TeamT & team_l,
                        const rcss::rcg::TeamT & team_r )
{
    return M_holder.handleTeam( time, team_l, team_r );
}

/*-------------------------------------------------------------------*/
bool
RCGHandler::handleMsg( const int time,
                       const int board,
                       const std::string & msg )
{
    return M_holder.handleMsg( time, board, msg );
}

/*-------------------------------------------------------------------*/
bool
RCGHandler::handleDraw( const int time,
                        const rcss::rcg::drawinfo_t & draw )
{
    switch ( ntohs( draw.mode ) ) {
    case rcss::rcg::DrawClear:
        M_holder.handleDrawClear( time );
        return true;

    case rcss::rcg::DrawPoint:
        M_holder.handleDrawPoint( time,
                                  rcss::rcg::PointT( rcss::rcg::nstohf( draw.object.pinfo.x ),
                                                     rcss::rcg::nstohf( draw.object.pinfo.y ),
                                                     draw.object.pinfo.color ) );
        return true;
    case rcss::rcg::DrawCircle:
        M_holder.handleDrawCircle( time,
                                   rcss::rcg::CircleT( rcss::rcg::nstohf( draw.object.cinfo.x ),
                                                       rcss::rcg::nstohf( draw.object.cinfo.y ),
                                                       rcss::rcg::nstohf( draw.object.cinfo.r ),
                                                       draw.object.cinfo.color ) );
        return true;
    case rcss::rcg::DrawLine:
        M_holder.handleDrawLine( time,
                                 rcss::rcg::LineT( rcss::rcg::nstohf( draw.object.linfo.x1 ),
                                                   rcss::rcg::nstohf( draw.object.linfo.y1 ),
                                                   rcss::rcg::nstohf( draw.object.linfo.x2 ),
                                                   rcss::rcg::nstohf( draw.object.linfo.y2 ),
                                                   draw.object.linfo.color ) );
        return true;
    default:
        std::cerr << "(RCGHandler::handleDraw) Unknown draw mode " << ntohs( draw.mode )
                  << std::endl;
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
bool
RCGHandler::handleServerParam( const rcss::rcg::ServerParamT & sparam )
{
    return M_holder.handleServerParam( sparam );
}

/*-------------------------------------------------------------------*/
bool
RCGHandler::handlePlayerParam( const rcss::rcg::PlayerParamT & pparam )
{
    return M_holder.handlePlayerParam( pparam );
}

/*-------------------------------------------------------------------*/
bool
RCGHandler::handlePlayerType( const rcss::rcg::PlayerTypeT & ptype )
{
    return M_holder.handlePlayerType( ptype );
}

/*-------------------------------------------------------------------*/
bool
RCGHandler::handleTeamGraphic( const char side,
                               const int x,
                               const int y,
                               const std::vector< std::string > & xpm_tile )
{
    return M_holder.handleTeamGraphic( side, x, y, xpm_tile );
}
