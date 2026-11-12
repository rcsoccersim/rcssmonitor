// -*-c++-*-

/*!
  \file overlay_math.h
  \brief calculations of the overlays, without drawing. Header File.
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

#ifndef RCSSMONITOR_OVERLAY_MATH_H
#define RCSSMONITOR_OVERLAY_MATH_H

#include "vector_2d.h"

#include <rcss/rcg/types.h>

#include <QString>

#include <vector>

/*!
  \namespace OverlayMath
  \brief calculations of the overlays (the offside lines, ...), kept apart
  from the painters so that they can be tested and shared by the views.
*/
namespace OverlayMath {

/*!
  \struct OffsideLines
  \brief the x coordinates of the two offside lines
*/
struct OffsideLines {
    float left_; //!< the line of the left team, in x <= 0
    float right_; //!< the line of the right team, in x >= 0
};

/*!
  \brief get the offside lines.
  \param show the show data
  \return the lines

  The line of the left team is at the second last left player in x < 0,
  or at the ball when the ball is nearer to the left goal line, and not
  beyond the left goal line. That of the right team is the same on the
  other side.
*/
OffsideLines offsideLines( const rcss::rcg::ShowInfoT & show );

/*!
  \brief where the player looks with its focus (attentionto by the eyes).
  \param player the player
  \param point set to the focus point, if it has one
  \return false if it has no focus point (its focus distance is 0)
*/
bool focusPoint( const rcss::rcg::PlayerT & player,
                 Vector2D * point );

/*!
  \struct CatchArea
  \brief the goalie's catchable area: the radius reaching the far corners of
  the catch box, and with a stretch of the catch box over 1, the largest and
  the smallest radii by the stretch
*/
struct CatchArea {
    double catchable_; //!< the radius without the stretch
    bool stretched_; //!< the stretch makes it larger
    double max_; //!< the largest radius by the stretch, if stretched_
    double min_; //!< the smallest radius by the stretch, if stretched_
};

CatchArea catchArea( const rcss::rcg::ServerParamT & param,
                     const rcss::rcg::PlayerTypeT & type );

/*!
  \struct TackleChances
  \brief how likely a tackle and a foul succeed now (0 to 1)
*/
struct TackleChances {
    bool tackle_; //!< a tackle may succeed
    double tackle_chance_;
    bool foul_; //!< a foul may succeed
    double foul_chance_;

    //! either may succeed
    bool any() const { return tackle_ || foul_; }
};

/*!
  \brief the chances of a tackle and a foul of the player for the ball
  (none when the server has no tackle distance)
*/
TackleChances tackleChances( const rcss::rcg::ServerParamT & param,
                             const rcss::rcg::PlayerT & player,
                             const rcss::rcg::BallT & ball );

/*!
  \brief the text of the tackle area, as the 2D view writes it:
  "T=0.5,F=0.8", "Tackle=0.5" or "Foul=0.8"
*/
QString tackleText( const TackleChances & chances );

/*!
  \struct Arc
  \brief an arc: around center, from start (a field direction, degree) by
  span degrees toward larger directions (clockwise on the 2D screen)
*/
struct Arc {
    Vector2D center_;
    double radius_;
    double start_;
    double span_; //!< 0 to 360
};

/*!
  \struct KickAccel
  \brief where the selected player can kick the ball to
*/
struct KickAccel {
    bool kickable_; //!< the player can kick the ball (none of the others is set otherwise)
    Vector2D next_; //!< the ball's next place without noise
    double max_accel_; //!< the largest acceleration of a kick
    /*!
      the arcs that bound where the ball can be next: within ball_speed_max
      of the ball and within max_accel_ of next_; or, when the two circles do
      not cross, the circle of max_accel_ around next_ (one arc of 360 degrees)
    */
    std::vector< Arc > arcs_;
};

KickAccel kickAccel( const rcss::rcg::ServerParamT & param,
                     const rcss::rcg::PlayerTypeT & type,
                     const rcss::rcg::PlayerT & player,
                     const rcss::rcg::BallT & ball );

/*!
  \brief the ball's path without noise for up to cycles cycles (at most
  100), until it moves less than min_step meters in a cycle: the places
  after each cycle
*/
std::vector< Vector2D > ballPath( const rcss::rcg::ServerParamT & param,
                                  const rcss::rcg::BallT & ball,
                                  const int cycles,
                                  const double min_step );

}

#endif
