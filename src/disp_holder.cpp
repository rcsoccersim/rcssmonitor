// -*-c++-*-

/*!
  \file disp_holder.cpp
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

#include "disp_holder.h"

#include "options.h"
#include "rcg_handler.h"

#include <rcss/rcg/util.h>
#include <rcss/rcg/parser_v1.h>
#include <rcss/rcg/parser_v4.h>
#include <rcss/rcg/parser_simdjson.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#if !defined(HAVE_NETINET_IN_H) && !defined(HAVE_WINDOWS_H)
#include <QtEndian>
#endif

#include <algorithm>
#include <iterator>
#include <iostream>
#include <cstdio>
#include <cstring>

const size_t DispHolder::INVALID_INDEX = size_t( -1 );
const size_t DispHolder::MAX_SIZE = 65535;

namespace {
// struct TimeCmp {
//     bool operator()( const DispConstPtr & lhs,
//                      const rcss::rcg::UInt32 rhs )
//       {
//           return lhs->show_.time_ < rhs;
//       }
// };

inline
rcss::rcg::Int16
my_ntohs( const rcss::rcg::Int16 val )
{
#if defined(HAVE_NETINET_IN_H) || defined(HAVE_WINDOS_H)
    return static_cast< rcss::rcg::Int16 >( ntohs( val ) );
#else
    return qFromBigEndian( val );
#endif
}

}

/*-------------------------------------------------------------------*/
/*!

 */
DispHolder::DispHolder()
    : M_current_index( INVALID_INDEX )
{
    M_disp_cont.reserve( MAX_SIZE );
}

/*-------------------------------------------------------------------*/
/*!

 */
DispHolder::~DispHolder()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
void
DispHolder::clear()
{
    M_server_param.copyFrom( rcss::rcg::ServerParamT() );
    M_player_param.copyFrom( rcss::rcg::PlayerParamT() );
    M_player_types.clear();

    M_team_graphic_left.clear();
    M_team_graphic_right.clear();

    M_point_cont.clear();
    M_circle_cont.clear();
    M_line_cont.clear();

    M_playmode = rcss::rcg::PM_Null;
    M_teams[0].clear();
    M_teams[1].clear();

    M_score_changed_index.clear();
    M_penalty_scores_left.clear();
    M_penalty_scores_right.clear();

    M_disp.reset();
    M_disp_cont.clear();

    M_current_index = INVALID_INDEX;
}

/*-------------------------------------------------------------------*/
/*!

 */
const
rcss::rcg::PlayerTypeT &
DispHolder::playerType( const int id ) const
{
    std::unordered_map< int, rcss::rcg::PlayerTypeT >::const_iterator it = M_player_types.find( id );

    if ( it == M_player_types.end() )
    {
        return M_default_player_type;
    }

    return it->second;
}

/*-------------------------------------------------------------------*/
/*!

 */
DispConstPtr
DispHolder::currentDisp() const
{
    if ( M_current_index != INVALID_INDEX
         && M_current_index < M_disp_cont.size() )
    {
        return M_disp_cont[M_current_index];
    }

    return M_disp;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::addDispInfoV1( const rcss::rcg::dispinfo_t & disp )
{
    RCGHandler handler( nullptr, *this );
    return handler.handleDispInfo( disp );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::addDispInfoV2( const rcss::rcg::dispinfo_t2 & disp )
{
    RCGHandler handler( nullptr, *this );
    return handler.handleDispInfo2( disp );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::addDispInfoV3( const char * msg )
{
    rcss::rcg::ParserV4 parser;
    RCGHandler handler( nullptr, *this );
    return parser.parseLine( -1, msg, handler );
}


/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::addJSON( const char * msg )
{
    rcss::rcg::ParserSimdJSON parser;
    RCGHandler handler( nullptr, *this );
    return parser.parseData( msg, handler );
}


/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::handleShow( const rcss::rcg::ShowInfoT & show )
{
    DispPtr disp( new rcss::rcg::DispInfoT );

    disp->pmode_ = M_playmode;
    disp->team_[0] = M_teams[0];
    disp->team_[1] = M_teams[1];
    disp->show_ = show;

#if 0
    if ( ! M_disp_cont.empty() )
    {
        if ( ( M_playmode == rcss::rcg::PM_BeforeKickOff
               && M_disp_cont.back()->pmode_ == rcss::rcg::PM_BeforeKickOff )
             || ( M_playmode == rcss::rcg::PM_TimeOver
                  && M_disp_cont.back()->pmode_ == rcss::rcg::PM_TimeOver ) )
        {
            M_disp_cont.pop_back();
        }
    }
#endif

    if ( M_disp_cont.size() <= MAX_SIZE )
    {
        M_disp_cont.push_back( disp );
    }

    M_disp = disp;

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::handleMsg( const int /* time */,
                       const int /* board */,
                       const std::string & msg )
{
    if ( ! msg.compare( 0, std::strlen( "(change_player_type" ), "(change_player_type" ) )
    {
        return true;
    }

    //     std::cerr << "handle message: time=" << time << " board=" << board
    //               << " [" << msg << "]" << std::endl;

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::handlePlayMode( const int time,
                            const rcss::rcg::PlayMode pmode )
{
    if ( M_playmode != pmode )
    {
        if ( pmode == rcss::rcg::PM_PenaltyScore_Left
             || pmode == rcss::rcg::PM_PenaltyMiss_Left )
        {
            M_penalty_scores_left.push_back( std::make_pair( time, pmode ) );
        }
        else if ( pmode == rcss::rcg::PM_PenaltyScore_Right
                  || pmode == rcss::rcg::PM_PenaltyMiss_Right )
        {
            M_penalty_scores_right.push_back( std::make_pair( time, pmode ) );
        }
    }

    M_playmode = pmode;
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::handleTeam( const int /* time */,
                        const rcss::rcg::TeamT & team_l,
                        const rcss::rcg::TeamT & team_r )
{
    if ( M_teams[0].score_ != team_l.score_
         || M_teams[1].score_ != team_r.score_ )
    {
        M_score_changed_index.push_back( M_disp_cont.size() - 1 );
    }

    M_teams[0] = team_l;
    M_teams[1] = team_r;
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::handleDrawClear( const int time )
{
    M_point_cont.erase( time );
    M_circle_cont.erase( time );
    M_line_cont.erase( time );

//     {
//         PointCont::iterator first = M_point_cont.lower_bound( 0 );
//         PointCont::iterator last = M_point_cont.upper_bound( time );
//         M_point_cont.erase( first, last );
//     }
//     {
//         CircleCont::iterator first = M_circle_cont.lower_bound( 0 );
//         CircleCont::iterator last = M_circle_cont.upper_bound( time );
//         M_circle_cont.erase( first, last );
//     }
//     {
//         LineCont::iterator first = M_line_cont.lower_bound( 0 );
//         LineCont::iterator last = M_line_cont.upper_bound( time );
//         M_line_cont.erase( first, last );
//     }
    return true;
}

/*-------------------------------------------------------------------*/
bool
DispHolder::handleDrawPoint( const int time,
                             const rcss::rcg::PointT & point )
{
    M_point_cont[time].push_back( point );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::handleDrawCircle( const int time,
                              const rcss::rcg::CircleT & circle )
{
    M_circle_cont[time].push_back( circle );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::handleDrawLine( const int time,
                            const rcss::rcg::LineT & line )
{
    M_line_cont[time].push_back( line );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::handleServerParam( const rcss::rcg::ServerParamT & param )
{
    M_server_param.copyFrom( param );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::handlePlayerParam( const rcss::rcg::PlayerParamT & param )
{
    M_player_param.copyFrom( param );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::handlePlayerType( const rcss::rcg::PlayerTypeT & param )
{
    M_player_types.insert( std::pair< int, rcss::rcg::PlayerTypeT >( param.id_, param ) );
    return true;
}

/*-------------------------------------------------------------------*/
bool
DispHolder::handleTeamGraphic( const char side,
                               const int x,
                               const int y,
                               const std::vector< std::string > & xpm_tile )
{
    if ( side == 'l' )
    {
        return M_team_graphic_left.addXpmTile( x, y, xpm_tile );
    }

    if ( side == 'r' )
    {
        return M_team_graphic_right.addXpmTile( x, y, xpm_tile );
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::handleEOF()
{
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::setIndexFirst()
{
    if ( M_disp_cont.empty() )
    {
        M_current_index = INVALID_INDEX;
        return false;
    }

    M_current_index = 0;
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::setIndexLast()
{
    if ( M_disp_cont.empty() )
    {
        M_current_index = INVALID_INDEX;
        return false;
    }

    M_current_index = M_disp_cont.size() - 1;
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::setIndexStepBack()
{
    if ( M_disp_cont.empty() )
    {
        M_current_index = INVALID_INDEX;
        return false;
    }

    if ( M_current_index == INVALID_INDEX )
    {
        M_current_index = 0;
        return true;
    }

    if ( 0 < M_current_index )
    {
        --M_current_index;
        return true;
    }

    if ( Options::instance().autoLoopMode() )
    {
        M_current_index = M_disp_cont.size() - 1;
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::setIndexStepForward()
{
    if ( M_disp_cont.empty() )
    {
        M_current_index = INVALID_INDEX;
        return false;
    }

    if ( M_current_index == INVALID_INDEX )
    {
        M_current_index = 0;
        return true;
    }

    if ( M_current_index < M_disp_cont.size() - 1 )
    {
        ++M_current_index;
        return true;
    }

    if ( Options::instance().autoLoopMode() )
    {
        M_current_index = 0;
        return true;
    }

    return false;
}


/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::setIndex( const size_t idx )
{
    if ( M_current_index == idx
         || idx == INVALID_INDEX
         || M_disp_cont.size() <= idx )
    {
        return false;
    }

    M_current_index = idx;
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::setCycle( const int cycle )
{
    std::size_t idx = getIndex( cycle );

    if ( idx == M_current_index
         || idx == INVALID_INDEX )
    {
        return false;
    }

    M_current_index = idx;
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
size_t
DispHolder::getIndex( const int cycle ) const
{
    DispCont::const_iterator it = std::lower_bound( M_disp_cont.begin(),
                                                    M_disp_cont.end(),
                                                    rcss::rcg::UInt32( cycle ),
                                                    //TimeCmp() );
                                                    []( const DispConstPtr & lhs,
                                                        const rcss::rcg::UInt32 rhs )
                                                    {
                                                        return lhs->show_.time_ < rhs;
                                                    } );
    if ( it == M_disp_cont.end() )
    {
        return INVALID_INDEX;
    }

    return std::distance( M_disp_cont.begin(), it );
}
