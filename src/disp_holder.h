// -*-c++-*-

/*!
  \file disp_holder.h
  \brief display data holder class Header File.
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

#ifndef RCSSMONITOR_DISP_HOLDER_H
#define RCSSMONITOR_DISP_HOLDER_H

#include "team_graphic.h"

#include <rcsslogplayer/types.h>
#include <rcsslogplayer/handler.h>

#include <boost/shared_ptr.hpp>

#include <vector>
#include <map>

typedef boost::shared_ptr< rcss::rcg::DispInfoT > DispPtr;
typedef boost::shared_ptr< const rcss::rcg::DispInfoT > DispConstPtr;
typedef std::vector< DispConstPtr > DispCont;
typedef std::multimap< int, rcss::rcg::PointInfoT > PointCont;
typedef std::multimap< int, rcss::rcg::CircleInfoT > CircleCont;
typedef std::multimap< int, rcss::rcg::LineInfoT > LineCont;

class DispHolder
    : public rcss::rcg::Handler {
public:
    static const size_t INVALID_INDEX;

private:

    int M_rcg_version;

    rcss::rcg::ServerParamT M_server_param;
    rcss::rcg::PlayerParamT M_player_param;
    rcss::rcg::PlayerTypeT M_default_player_type;
    std::map< int, rcss::rcg::PlayerTypeT > M_player_types;

    TeamGraphic M_team_graphic_left;
    TeamGraphic M_team_graphic_right;

    // the record of penalty score/miss, first: time, second: playmode
    std::vector< std::pair< int, rcss::rcg::PlayMode > > M_penalty_scores_left;
    std::vector< std::pair< int, rcss::rcg::PlayMode > > M_penalty_scores_right;

    PointCont M_point_cont;
    CircleCont M_circle_cont;
    LineCont M_line_cont;

    rcss::rcg::PlayMode M_playmode; //!< last handled playmode
    rcss::rcg::TeamT M_teams[2]; //!< last handled team info

    DispPtr M_disp; //! last handled display data
    DispCont M_disp_cont;

    size_t M_current_index;

    // not used
    DispHolder( const DispHolder & );
    DispHolder operator=( const DispHolder & );

public:

    DispHolder();
    ~DispHolder();

    void clear();

    const rcss::rcg::ServerParamT & serverParam() const { return M_server_param; }
    const rcss::rcg::PlayerParamT & playerParam() const { return M_player_param; }
    const std::map< int, rcss::rcg::PlayerTypeT > & playerTypes() const { return M_player_types; }
    const rcss::rcg::PlayerTypeT & playerType( const int id ) const;

    rcss::rcg::PlayMode playmode() const { return M_playmode; }

    const TeamGraphic & teamGraphicLeft() const { return M_team_graphic_left; }
    const TeamGraphic & teamGraphicRight() const { return M_team_graphic_right; }

    const std::vector< std::pair< int, rcss::rcg::PlayMode > > & penaltyScoresLeft() const
      {
          return M_penalty_scores_left;
      }
    const std::vector< std::pair< int, rcss::rcg::PlayMode > > & penaltyScoresRight() const
      {
          return M_penalty_scores_right;
      }

    const PointCont & pointCont() const { return M_point_cont; }
    const CircleCont & circleCont() const { return M_circle_cont; }
    const LineCont & lineCont() const { return M_line_cont; }

    size_t currentIndex() const { return M_current_index; }
    DispConstPtr currentDisp() const;
    const DispCont & dispCont() const { return M_disp_cont; }

    bool addDispInfoV1( const rcss::rcg::dispinfo_t & disp );
    bool addDispInfoV2( const rcss::rcg::dispinfo_t2 & disp );
    bool addDispInfoV3( const char * msg );

protected:
    virtual
    void doHandleLogVersion( int ver );

    virtual
    int doGetLogVersion() const;

    virtual
    void doHandleShowInfo( const rcss::rcg::ShowInfoT & );

    virtual
    void doHandleMsgInfo( const int,
                          const int,
                          const std::string & );

    virtual
    void doHandlePlayMode( const int,
                           const rcss::rcg::PlayMode );

    virtual
    void doHandleTeamInfo( const int,
                           const rcss::rcg::TeamT &,
                           const rcss::rcg::TeamT & );

    virtual
    void doHandleDrawClear( const int );

    virtual
    void doHandleDrawPointInfo( const int,
                                const rcss::rcg::PointInfoT & );

    virtual
    void doHandleDrawCircleInfo( const int,
                                 const rcss::rcg::CircleInfoT & );

    virtual
    void doHandleDrawLineInfo( const int,
                               const rcss::rcg::LineInfoT & );

    virtual
    void doHandleServerParam( const rcss::rcg::ServerParamT & );

    virtual
    void doHandlePlayerParam( const rcss::rcg::PlayerParamT & );

    virtual
    void doHandlePlayerType( const rcss::rcg::PlayerTypeT & );

    virtual
    void doHandleEOF();


private:
    void analyzeTeamGraphic( const std::string & msg );

public:

    bool setIndexFirst();
    bool setIndexLast();
    bool setIndexStepBack();
    bool setIndexStepForward();
    bool setIndex( const size_t idx );
    bool setCycle( const int cycle );
    size_t getIndex( const int cycle ) const;


};

#endif
