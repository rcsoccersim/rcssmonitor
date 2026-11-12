// -*-c++-*-

/*!
  \file player_selection.h
  \brief the automatic selection of a player, in 2D and 3D. Header File.
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

#ifndef RCSSMONITOR_PLAYER_SELECTION_H
#define RCSSMONITOR_PLAYER_SELECTION_H

class Options;

namespace rcss {
namespace rcg {
struct ShowInfoT;
}
}

/*!
  \brief select the player nearest to the ball, of the team (or of both)
  that the automatic selection asks for (Options::playerAutoSelect()).
  The 2D view and the 3D view call it whenever they are drawn, so that the
  selection follows the ball whichever is shown.
  \param show the show (the current frame)
  \param options where the selection is
  \return true if a player was selected
*/
bool selectPlayerNearestBall( const rcss::rcg::ShowInfoT & show,
                              Options & options );

#endif
