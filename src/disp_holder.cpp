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

#include <rcsslogplayer/util.h>
#include <rcsslogplayer/parser.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <algorithm>
#include <iterator>
#include <iostream>
#include <cstdio>
#include <cstring>

const size_t DispHolder::INVALID_INDEX = size_t( -1 );

namespace {
struct TimeCmp {
    bool operator()( const DispConstPtr & lhs,
                     const rcss::rcg::UInt32 rhs )
      {
          return lhs->show_.time_ < rhs;
      }
};
}

/*-------------------------------------------------------------------*/
/*!

 */
DispHolder::DispHolder()
    : M_rcg_version( 0 ),
      M_current_index( INVALID_INDEX )
{
    M_disp_cont.reserve( 65535 );
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
    M_rcg_version = 0;

    M_server_param = rcss::rcg::ServerParamT();
    M_player_param = rcss::rcg::PlayerParamT();
    M_player_types.clear();

    M_team_graphic_left.clear();
    M_team_graphic_right.clear();

    M_penalty_scores_left.clear();
    M_penalty_scores_right.clear();

    M_point_cont.clear();
    M_circle_cont.clear();
    M_line_cont.clear();

    M_playmode = rcss::rcg::PM_Null;
    M_teams[0].clear();
    M_teams[1].clear();

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
    std::map< int, rcss::rcg::PlayerTypeT >::const_iterator it
        = M_player_types.find( id );

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
    switch ( ntohs( disp.mode ) ) {
    case rcss::rcg::NO_INFO:
        break;
    case rcss::rcg::SHOW_MODE:
        {
            rcss::rcg::ShowInfoT new_show;
            M_playmode = static_cast< rcss::rcg::PlayMode >( disp.body.show.pmode );
            rcss::rcg::convert( disp.body.show.team[0], M_teams[0] );
            rcss::rcg::convert( disp.body.show.team[1], M_teams[1] );
            rcss::rcg::convert( disp.body.show, new_show );

            doHandleShowInfo( new_show );
        }
        break;
    case rcss::rcg::MSG_MODE:
        doHandleMsgInfo( 0,
                         ntohs( disp.body.msg.board ),
                         disp.body.msg.message );
        break;
    case rcss::rcg::DRAW_MODE:
        // TODO
        break;
    default:
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::addDispInfoV2( const rcss::rcg::dispinfo_t2 & disp )
{
    switch ( ntohs( disp.mode ) ) {
    case rcss::rcg::NO_INFO:
        break;
    case rcss::rcg::SHOW_MODE:
        {
            rcss::rcg::ShowInfoT new_show;
            M_playmode = static_cast< rcss::rcg::PlayMode >( disp.body.show.pmode );
            rcss::rcg::convert( disp.body.show.team[0], M_teams[0] );
            rcss::rcg::convert( disp.body.show.team[1], M_teams[1] );
            rcss::rcg::convert( disp.body.show, new_show );

            doHandleShowInfo( new_show );
        }
        break;

    case rcss::rcg::MSG_MODE:
        doHandleMsgInfo( 0,
                         ntohs( disp.body.msg.board ),
                         disp.body.msg.message );
        break;
    case rcss::rcg::PARAM_MODE:
        {
            rcss::rcg::ServerParamT new_params;
            rcss::rcg::convert( disp.body.sparams, new_params );

            doHandleServerParam( new_params );
        }
        break;
    case rcss::rcg::PPARAM_MODE:
        {
            rcss::rcg::PlayerParamT new_params;
            rcss::rcg::convert( disp.body.pparams, new_params );

            doHandlePlayerParam( new_params );
        }
        break;
    case rcss::rcg::PT_MODE:
        {
            rcss::rcg::PlayerTypeT new_type;
            convert( disp.body.ptinfo, new_type );

            doHandlePlayerType( new_type );
        }
        break;
    case rcss::rcg::DRAW_MODE:
    case rcss::rcg::BLANK_MODE:
    case rcss::rcg::PM_MODE:
    case rcss::rcg::TEAM_MODE:
        break;
    default:
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
DispHolder::addDispInfoV3( const char * msg )
{
    rcss::rcg::Parser parser( *this );

    return parser.parseLine( -1, msg );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
DispHolder::doHandleLogVersion( int ver )
{
    M_rcg_version = ver;
}

/*-------------------------------------------------------------------*/
/*!

 */
int
DispHolder::doGetLogVersion() const
{
    return M_rcg_version;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
DispHolder::doHandleShowInfo( const rcss::rcg::ShowInfoT & show )
{
    DispPtr disp( new rcss::rcg::DispInfoT );

    disp->pmode_ = M_playmode;
    disp->team_[0] = M_teams[0];
    disp->team_[1] = M_teams[1];
    disp->show_ = show;

    M_disp = disp;

    if ( Options::instance().bufferingMode() )
    {
        if ( (int)M_disp_cont.size() <= Options::instance().maxDispBuffer() )
        {
            M_disp_cont.push_back( disp );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
DispHolder::doHandleMsgInfo( const int,
                             const int,
                             const std::string & msg )
{
    if ( ! msg.compare( 0, std::strlen( "(team_graphic_" ), "(team_graphic_" ) )
    {
        analyzeTeamGraphic( msg );
        return;
    }

    if ( ! msg.compare( 0, std::strlen( "(change_player_type" ), "(change_player_type" ) )
    {
        return;
    }

    //     std::cerr << "handle message: time=" << time << " board=" << board
    //               << " [" << msg << "]" << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
DispHolder::doHandlePlayMode( const int time,
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
}

/*-------------------------------------------------------------------*/
/*!

 */
void
DispHolder::doHandleTeamInfo( const int,
                              const rcss::rcg::TeamT & team_l,
                              const rcss::rcg::TeamT & team_r )
{
    M_teams[0] = team_l;
    M_teams[1] = team_r;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
DispHolder::doHandleDrawClear( const int time )
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
}

/*-------------------------------------------------------------------*/
/*!

 */
void
DispHolder::doHandleDrawPointInfo( const int time,
                                   const rcss::rcg::PointInfoT & point )
{
    M_point_cont.insert( PointCont::value_type( time, point ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
 void
 DispHolder::doHandleDrawCircleInfo( const int time,
                                     const rcss::rcg::CircleInfoT & circle )
 {
     std::cerr << "doHandleDrawCircleInfo " << time << std::endl;
     M_circle_cont.insert( CircleCont::value_type( time, circle ) );
 }

/*-------------------------------------------------------------------*/
/*!

 */
void
DispHolder::doHandleDrawLineInfo( const int time,
                                  const rcss::rcg::LineInfoT & line )
{
    M_line_cont.insert( LineCont::value_type( time, line ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
DispHolder::doHandleServerParam( const rcss::rcg::ServerParamT & param )
{
    M_server_param = param;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
DispHolder::doHandlePlayerParam( const rcss::rcg::PlayerParamT & param )
{
    M_player_param = param;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
DispHolder::doHandlePlayerType( const rcss::rcg::PlayerTypeT & param )
{
    M_player_types.insert( std::pair< int, rcss::rcg::PlayerTypeT >( param.id_, param ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
DispHolder::doHandleEOF()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
void
DispHolder::analyzeTeamGraphic( const std::string & msg )
{
    char side = 'n';
    int x = -1, y = -1;

    if ( std::sscanf( msg.c_str(),
                      "(team_graphic_%c ( %d %d ",
                      &side, &x, &y ) != 3
         || ( side != 'l' && side != 'r' )
         || x < 0
         || y < 0 )
    {
        std::cerr << "Illegal team_graphic message ["
                  << msg << "]" << std::endl;
        return;
    }

    if ( side == 'l' )
    {
        if ( ! M_team_graphic_left.parse( msg.c_str() ) )
        {
            std::cerr << "Illegal team_graphic message ["
                      << msg << "]" << std::endl;
        }
        return;
    }

    if ( side == 'r' )
    {
        if ( ! M_team_graphic_right.parse( msg.c_str() ) )
        {
            std::cerr << "Illegal team_graphic message ["
                      << msg << "]" << std::endl;
        }
        return;
    }
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
    DispCont::const_iterator it
        = std::lower_bound( M_disp_cont.begin(),
                            M_disp_cont.end(),
                            rcss::rcg::UInt32( cycle ),
                            TimeCmp() );
    if ( it == M_disp_cont.end() )
    {
        return INVALID_INDEX;
    }

    return std::distance( M_disp_cont.begin(), it );
}
