// -*-c++-*-

/*!
  \file play_mode_position.h
  \brief the ball position for Change Playmode. Header File.
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

#ifndef RCSSMONITOR_PLAY_MODE_POSITION_H
#define RCSSMONITOR_PLAY_MODE_POSITION_H

#include <rcss/rcg/types.h>

/*!
  \brief get where Change Playmode moves the ball for a play mode.
  \param mode the new play mode
  \param x in: x of the point chosen on the field, out: x of the ball
  \param y in: y of the point chosen on the field, out: y of the ball
  \return false if Change Playmode does not offer the mode

  The point is first kept within the pitch. Then, for example, a kick in
  moves it to the touch line, a corner kick to the corner arc, and a back
  pass into the penalty area of the team that made it. The caller sends
  the ball move and the play mode change for every mode that this
  function accepts.
*/
bool playModeBallPosition( const rcss::rcg::PlayMode mode,
                           double * x,
                           double * y );

#endif
