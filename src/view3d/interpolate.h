// -*-c++-*-

/*!
  \file interpolate.h
  \brief the interpolation between frames for the 3D view. Header File.
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

#ifndef RCSSMONITOR_INTERPOLATE_H
#define RCSSMONITOR_INTERPOLATE_H

#include <rcss/rcg/types.h>

#include <cstddef>

/*!
  \brief the interpolation between two frames for the 3D view
  (docs/PLAN_3D-VIEW.md §5.2, rcssmonitor-web interpolate.ts). Positions are
  linear, the body direction turns the short way round. A play mode change
  is not interpolated (the server may have moved everyone), nor is an
  object that jumped further than it can move in a cycle (a move command,
  the placement of a free kick).
*/
namespace Interpolate {

//! farther than a player can go in a cycle (player_speed_max is about 1.05-1.2 m)
constexpr float MAX_PLAYER_STEP = 3.0f;
//! farther than the ball can go in a cycle (ball_speed_max is 3 m)
constexpr float MAX_BALL_STEP = 4.0f;

/*!
  \brief from a toward b the short way round.
  \param a, b degrees
  \param t 0 to 1
  \return degrees within [-180, 180]
*/
float lerpAngle( const float a,
                 const float b,
                 const float t );

/*!
  \brief the positions alpha (0 to 1) of the way from a to b. Everything
  else (the states, the counts, ...) is a's.
  \param a the frame
  \param b the next frame, or null
  \param alpha the fraction of the way to b
*/
rcss::rcg::ShowInfoT show( const rcss::rcg::DispInfoT & a,
                           const rcss::rcg::DispInfoT * b,
                           const double alpha );

/*!
  \struct Position
  \brief which frames the 3D view draws, and how far between them
*/
struct Position {
    std::size_t index = 0; //!< the frame
    std::size_t next = 0; //!< the next frame (== index if none)
    double alpha = 0.0; //!< the fraction of the way to next
};

/*!
  \brief the frames to draw. A log shows the current frame, moving toward
  the next one while playing. Live, it shows the frame before the newest,
  moving toward the newest over one simulation step: one cycle late, but
  smooth (§5.2).
  \param frame_count the number of the frames
  \param current the current frame
  \param live following the newest frame of a live game
  \param play_alpha while playing a log, the fraction of the interval since the last step (0 when stopped)
  \param since_newest live, the fraction of the simulation step since the newest frame came
*/
Position position( const std::size_t frame_count,
                   const std::size_t current,
                   const bool live,
                   const double play_alpha,
                   const double since_newest );

}

#endif
