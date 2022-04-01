// -*-c++-*-

/*!
  \file rcg_handler.h
  \brief rcg handler class Header File.
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

#ifndef RCSSMONITOR_RCG_HANDLER_H
#define RCSSMONITOR_RCG_HANDLER_H

#include <rcss/rcg/team_graphic.h>
#include <rcss/rcg/types.h>
#include <rcss/rcg/handler.h>

class QWidget;
class QProgressDialog;
class DispHolder;

class RCGHandler
    : public rcss::rcg::Handler {
private:

    DispHolder & M_holder;

    QProgressDialog * M_progress;

    // not used
    RCGHandler( const RCGHandler & ) = delete;
    RCGHandler operator=( const RCGHandler & ) = delete;

public:

    RCGHandler( QWidget * parent,
                DispHolder & holder );
    ~RCGHandler();

protected:

    virtual
    bool handleEOF() override;

    virtual
    bool handleShow( const rcss::rcg::ShowInfoT & show ) override;

    virtual
    bool handlePlayMode( const int time,
                         const rcss::rcg::PlayMode pmode ) override;

    virtual
    bool handleTeam( const int time,
                     const rcss::rcg::TeamT & team_l,
                     const rcss::rcg::TeamT & team_r ) override;
    virtual
    bool handleMsg( const int time,
                    const int board,
                    const std::string & msg ) override;

    virtual
    bool handleDrawClear( const int time ) override;

    virtual
    bool handleDrawPoint( const int time,
                          const rcss::rcg::PointT & point ) override;

    virtual
    bool handleDrawCircle( const int time,
                           const rcss::rcg::CircleT & circle ) override;

    virtual
    bool handleDrawLine( const int time,
                         const rcss::rcg::LineT & line ) override;

    virtual
    bool handleServerParam( const rcss::rcg::ServerParamT & sparam ) override;

    virtual
    bool handlePlayerParam( const rcss::rcg::PlayerParamT & pparam ) override;

    virtual
    bool handlePlayerType( const rcss::rcg::PlayerTypeT & ptype ) override;

    virtual
    bool handleTeamGraphic( const rcss::rcg::Side side,
                            const int x,
                            const int y,
                            rcss::rcg::XpmTile::Ptr tile ) override;

};

#endif
