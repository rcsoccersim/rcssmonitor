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
  \struct FocusLook
  \brief how the focus point is drawn, the same in 2D and 3D: a reticle (a
  circle of the focus point size with four ticks, one toward the player)
  over a soft spot (discs of the focus color, darker to the center), the
  line from the player, and the lines of the same noise (focusNoiseLines(),
  fainter outward). The other players' focus points (the option) are
  smaller reticles in their team colors.
*/
struct FocusLook {
    static constexpr double TICK_IN = 0.5; //!< the ticks of the reticle from, times its radius
    static constexpr double TICK_OUT = 1.5; //!< to
    static constexpr double SPOT_RADII[] = { 1.0, 0.66, 0.33 }; //!< the discs of the spot, times the radius
    static constexpr double SPOT_OPACITY = 0.12; //!< of each disc
    static constexpr double NOISE_OPACITY[] = { 0.8, 0.55, 0.3 }; //!< of the lines of the same noise, inward first
    static constexpr double OTHER_SCALE = 0.6; //!< the other players' reticles, times the radius
};

/*!
  \brief the four ticks of the focus point's reticle (FocusLook), one
  pointing along the direction
  \param center the focus point
  \param radius the radius of the reticle
  \param direction the direction from the player to the focus point (degree)
  \return the ends of the ticks, two for each
*/
std::vector< Vector2D > focusTicks( const Vector2D & center,
                                    const double radius,
                                    const double direction );

/*!
  \brief how much farther than the shortest the lines of the same noise
  around the focus point go (m): FocusNoiseLines
*/
constexpr double FOCUS_NOISE_STEPS[] = { 10.0, 20.0, 40.0 };

/*!
  \brief the lines of the same observation noise around the focus point,
  within the view of the player.

  The noise of a distance that the player sees grows with the distance
  from the player and that from the focus point (rcssserver 18 and later;
  in 19's gaussian model, its deviation is dist_noise_rate times the first
  plus focus_dist_noise_rate times the second, of the player type). The
  points of the same noise, where a |q - player| + b |q - focus| is the
  same, make an ellipse with the player and the focus point as its foci
  when the rates are the same (the default). A line goes where that sum is
  larger than its smallest by (a + b) / 2 times a step of
  FOCUS_NOISE_STEPS: with the same rates, where the way from the player
  through the point to the focus point is that much longer than the
  straight way. Inside, the player sees better. Only the parts within the
  view angle are kept, as nothing outside is seen.

  \param type the player type (the rates)
  \param player the player, with its focus point (focusPoint())
  \return for each step, the polylines of its line (none without a focus
  point or a view, or with both rates 0)
*/
std::vector< std::vector< std::vector< Vector2D > > >
focusNoiseLines( const rcss::rcg::PlayerTypeT & type,
                 const rcss::rcg::PlayerT & player );

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
    double catchable_length_; //!< the length of the catch box without the stretch
    double max_length_; //!< the length of the catch box for max_
    double min_length_; //!< the length of the catch box for min_
};

CatchArea catchArea( const rcss::rcg::ServerParamT & param,
                     const rcss::rcg::PlayerTypeT & type );

/*!
  \struct CatchOutline
  \brief the outline of where a goalie can catch the ball with a catch box
  of a length.

  The goalie turns the catch box (the length ahead, catchable_area_w wide)
  to its body direction plus the catch direction, which the server keeps
  within min_catch_angle and max_catch_angle (from rcssserver 17.0.0, -90
  and +90 by default: not behind the goalie). The far corners of the boxes
  draw the arc; the sides of the boxes at the two ends of the directions
  close the outline behind the goalie.

  The outline goes round: arcs_[0], between_ (only with two arcs), arcs_[1],
  then edges_ back to the start of arcs_[0].
*/
struct CatchOutline {
    bool circle_; //!< the goalie can catch all around: arcs_ is one full circle, and the others are empty
    /*!
      the far edge, from the far corner at the smallest direction. When the
      directions are so close that the far corners leave a gap in the
      middle (less than twice the angle of a far corner apart), two arcs,
      and the far sides of the two boxes at the ends join them
    */
    std::vector< Arc > arcs_;
    std::vector< Vector2D > between_; //!< from the end of arcs_[0] to the start of arcs_[1], if two
    /*!
      the rest of the outline, from the end of the last arc (the far corner
      at the largest direction) round behind the goalie to the start of
      arcs_[0]
    */
    std::vector< Vector2D > edges_;
};

/*!
  \brief the outline of where the goalie can catch with a catch box of the
  length (one of CatchArea's lengths)
*/
CatchOutline catchOutline( const rcss::rcg::ServerParamT & param,
                           const rcss::rcg::PlayerT & player,
                           const double length );

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
