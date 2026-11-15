// -*-c++-*-

/*!
  \file figure_actions.h
  \brief what the players do besides walking, for the human figures. Header File.
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

#ifndef RCSSMONITOR_FIGURE_ACTIONS_H
#define RCSSMONITOR_FIGURE_ACTIONS_H

#include "disp_holder.h"
#include "scene_content.h"

class BallFlight;

/*!
  \namespace FigureActionLog
  \brief what a player does besides walking at a time between two frames,
  read from the state bits of the frames around it (docs/PLAN_3D-VIEW.md
  §5.8): the kick, the tackle, being down after a foul, the goalie's catch,
  and from the frame drawn the neck and the pointing arm. Only from the
  frames, so that a frame looks the same however it is reached.

  An action's moment is when it meets the ball: rcssserver runs the
  commands of a cycle before it moves the ball, so the frame with the bit
  already has the ball moved, and the foot met it at the frame before. The
  figure starts its action that much (the lead) before the moment, so it
  looks two frames ahead; live, the game is drawn two cycles behind its
  newest frame (Interpolate::LIVE_DELAY), so the frames ahead are there.

  A run of frames with a bit is one action: a tackle and a foul last as
  long as the server holds the player, and the kicks of the cycles in a row
  (holding the ball, kicking it faster in steps) are one kick, at the one
  that sent the ball fastest. Of the actions around, the latest one begun
  is taken, while it has not ended.
*/
namespace FigureActionLog {

//! cycles before a kick's moment that the figure starts to swing, and after it that it ends, by KickKind
constexpr float KICK_LEAD[3] = { 0.6f, 0.9f, 1.0f };
constexpr float KICK_TAIL[3] = { 0.8f, 1.3f, 1.5f };
//! the ball's speed after a kick (m per cycle) from which it is a pass, and a shot (toward the goal)
constexpr float PASS_SPEED = 1.0f;
constexpr float SHOT_SPEED = 2.0f;
//! a shot goes toward the goal from at most this far (m), and passes the post by at most SHOT_MARGIN (m)
constexpr float SHOT_RANGE = 45.0f;
constexpr float SHOT_MARGIN = 2.0f;

//! cycles before a tackle's moment that the figure starts to slide, and after the bit's last cycle that it is up again
constexpr float TACKLE_LEAD = 0.6f;
constexpr float TACKLE_TAIL = 2.5f;
//! cycles after a foul's moment that the fouled figure starts to fall (once the tackler has slid in), how long it falls, and after the bit's last cycle that it is up again
constexpr float FOULED_START = 1.0f;
constexpr float FOULED_FALL = 0.8f;
constexpr float FOULED_TAIL = 2.8f;
//! cycles before a catch that the hands go up, and after they keep the ball that they are down again
constexpr float CATCH_LEAD = 1.0f;
constexpr float CATCH_TAIL = 2.0f;

//! frames looked at around the frame drawn
constexpr int LOOK_AHEAD = 2;
constexpr int LOOK_BACK = 16;
//! frames of a run of a bit looked through at most, longer than the server holds any
constexpr int MAX_RUN = 50;

/*!
  \brief what a player does at a time between two frames
  \param frames the frames (DispHolder::dispCont())
  \param player its index in the show (0 to 21)
  \param index the frame drawn
  \param next the frame toward which it moves (== index if none)
  \param alpha the fraction of the way to next
  \param drawn the player as drawn (Interpolate::show()): the neck and the arm
  \param sp the server's parameters (catch_ban_cycle, goal_width)
  \param ball_radius the radius of the ball as drawn (m), where a kicking foot meets it
*/
FigureActions read( const DispCont & frames,
                    const int player,
                    const std::size_t index,
                    const std::size_t next,
                    const double alpha,
                    const rcss::rcg::PlayerT & drawn,
                    const rcss::rcg::ServerParamT & sp,
                    const float ball_radius,
                    const BallFlight * flight = nullptr );

}

#endif
