// -*-c++-*-

/*!
  \file player_style.h
  \brief how a player is colored by its state, in 2D and 3D. Header File.
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
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef RCSSMONITOR_PLAYER_STYLE_H
#define RCSSMONITOR_PLAYER_STYLE_H

#include <QBrush>
#include <QColor>
#include <QPen>

class Options;

namespace rcss {
namespace rcg {
struct PlayerT;
struct PlayerTypeT;
struct ServerParamT;
}
}

/*!
  \struct PlayerStyle
  \brief how a player is colored by its state (PlayerPainter::drawBody),
  shared by the 2D view and the 3D view
*/
struct PlayerStyle {
    QPen pen; //!< the outline of the outer circle: width 2 when highlighted (selected, kicking, tackling, illegal defense)
    QBrush brush; //!< the fill of the outer circle: the team color, or the color of a state
    QBrush body; //!< the body: the brush darkened as the stamina goes down
    QPen decay; //!< the ring around the circle when effort or recovery has decayed, Qt::NoPen if not

    bool highlighted() const { return pen.width() >= 2; }
    bool decayed() const { return decay.style() != Qt::NoPen; }
};

/*!
  \brief the style of a player.
  \param player the player
  \param type its player type
  \param param the server parameters (stamina_max)
  \param options the colors and the selection
*/
PlayerStyle playerStyle( const rcss::rcg::PlayerT & player,
                         const rcss::rcg::PlayerTypeT & type,
                         const rcss::rcg::ServerParamT & param,
                         const Options & options );

#endif
