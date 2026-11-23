// -*-c++-*-

/*!
  \file whistle_rules.h
  \brief the referee's whistle at the changes of the play mode. Header File.
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

#ifndef RCSSMONITOR_WHISTLE_RULES_H
#define RCSSMONITOR_WHISTLE_RULES_H

#include <rcss/rcg/types.h>

/*!
  \enum WhistleKind
  \brief the kinds of the referee's whistle, from the weakest
*/
enum class WhistleKind {
    None, //!< no whistle
    Short, //!< a short whistle: to stop for a foul, to restart after it, a penalty kick
    Long, //!< a long whistle: a kick-off
    LongTwice, //!< two long whistles: the end of a half
    LongThrice, //!< three long whistles: the end of the game
};

/*!
  \brief the whistle at a change of the play mode, after the guidelines of
  the Laws of the Game for referees (IFAB) and how rcssserver changes the
  modes (docs/plan/2026-11-22-00_v20-features.md C.3.4):

  - a kick-off (the start of the game, of a half, after a goal): long, once.
    In the game logs, a half ends with a kick-off right after the play
    (no `before_kick_off`): only the kick-off's whistle
  - `before_kick_off` after the time 0 (the end of a half, live),
    `first_half_over`: two long
  - `time_over`: three long
  - a foul, an offside, a back pass and so on (the modes that stop the
    play for a free kick): short; and the free kick (or the indirect free
    kick) after one of them: short (the restart, after the defenders kept
    the distance)
  - `penalty_ready` (the kicker may kick): short
  - no whistle for the others (e.g. a goal: a clear goal needs none; a free
    kick after a catch, a kick-in, a corner kick, a goal kick, a dropped
    ball, `play_on`)

  \param prev the mode of the frame before
  \param next the mode of the frame
  \param next_time the time (cycle) of the frame
  \return the whistle. None if the mode does not change, or if the mode
  before is unknown (PM_Null).
*/
WhistleKind whistleFor( const rcss::rcg::PlayMode prev,
                        const rcss::rcg::PlayMode next,
                        const int next_time );

/*!
  \brief whether a mode stops the play for a free kick (a foul, an offside,
  a back pass, a fault of a free kick or a catch, an illegal defense)
*/
bool isWhistleStopMode( const rcss::rcg::PlayMode mode );

/*!
  \brief the stronger of two whistles (the one to blow when a live update
  has more than one change)
*/
WhistleKind strongerWhistle( const WhistleKind a,
                             const WhistleKind b );

/*!
  \brief the whistle when the play starts from the first frame of a game
  log: the kick-off of the game if the log starts with one (the logs have
  no `before_kick_off` before it), or none
  \param first the mode of the first frame
*/
WhistleKind whistleAtFirstFrame( const rcss::rcg::PlayMode first );

#endif
