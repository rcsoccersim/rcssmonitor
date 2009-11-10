// -*-c++-*-

/*!
  \file player_painer.h
  \brief player painter class Header File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef RCSSLOGPLAYER_PLAYER_PAINTER_H
#define RCSSLOGPLAYER_PLAYER_PAINTER_H

#include <QPen>
#include <QBrush>
#include <QFont>

#include "painter_interface.h"

#include <rcsslogplayer/types.h>

class QPainter;
class QPixmap;

class DispHolder;

class PlayerPainter
    : public PainterInterface {
private:

    struct Param {
        int x_; //!< screen X coordinates
        int y_; //!< screen Y coordinates
        int body_radius_; //!< pixel body radius
        int kick_radius_; //!< pixel kick area radius
        int draw_radius_; //!< pixel main draw radius.
        bool have_full_effort_; //!< flag to check effort value
        const rcss::rcg::PlayerT & player_;
        const rcss::rcg::BallT & ball_;
        const rcss::rcg::PlayerTypeT & player_type_;

        Param( const rcss::rcg::PlayerT & player,
               const rcss::rcg::BallT & ball,
               const rcss::rcg::ServerParamT & sparam,
               const rcss::rcg::PlayerTypeT & ptype );
    private:
        //! not used
        Param();
    };


    const DispHolder & M_disp_holder;

    QFont M_player_font;

    QPen M_player_pen;
    QPen M_selected_player_pen;
    QPen M_left_team_pen;
    QBrush M_left_team_brush;
    QPen M_left_goalie_pen;
    QPen M_left_goalie_stretch_pen;
    QBrush M_left_goalie_brush;
    QPen M_right_team_pen;
    QBrush M_right_team_brush;
    QPen M_right_goalie_pen;
    QPen M_right_goalie_stretch_pen;
    QBrush M_right_goalie_brush;
    QPen M_player_number_pen;
    QPen M_player_number_inner_pen;
    QPen M_neck_pen;
    QPen M_view_area_pen;
    QPen M_large_view_area_pen;
    QBrush M_ball_collide_brush;
    QBrush M_player_collide_brush;
    QPen M_kick_pen;
    QBrush M_kick_fault_brush;
    QPen M_kick_accel_pen;
    QBrush M_catch_brush;
    QBrush M_catch_fault_brush;
    QPen M_tackle_pen;
    QBrush M_tackle_brush;
    QBrush M_tackle_fault_brush;
    QBrush M_foul_charged_brush;
    QPen M_pointto_pen;

    // not used
    PlayerPainter();
    PlayerPainter( const PlayerPainter & );
    const PlayerPainter operator=( const PlayerPainter & );
public:

    explicit
    PlayerPainter( const DispHolder & disp_holder );

    ~PlayerPainter();

    void draw( QPainter & dc );

private:

    void readSettings();
    void writeSettings();


    void drawAll( QPainter & painter,
                  const rcss::rcg::PlayerT & player,
                  const rcss::rcg::BallT & ball ) const;
    void drawBody( QPainter & painter,
                   const PlayerPainter::Param & param ) const;
    void drawDir( QPainter & painter,
                  const PlayerPainter::Param & param ) const;
    void drawViewArea( QPainter & painter,
                       const PlayerPainter::Param & param ) const;
    void drawCatchArea( QPainter & painter,
                        const PlayerPainter::Param & param ) const;
    void drawTackleArea( QPainter & painter,
                         const PlayerPainter::Param & param ) const;
    void drawPointto( QPainter & painter,
                      const PlayerPainter::Param & param ) const;
    void drawKickAccelArea( QPainter & painter,
                            const PlayerPainter::Param & param ) const;
    void drawText( QPainter & painter,
                   const PlayerPainter::Param & param ) const;

    void drawOffsideLine( QPainter & painter,
                          const rcss::rcg::ShowInfoT & show ) const;

};

#endif
