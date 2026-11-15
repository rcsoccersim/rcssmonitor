// -*-c++-*-

/*!
  \file ball_3d.h
  \brief the ball of the 3D view: its pattern, and how it has rolled. Header File.
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

#ifndef RCSSMONITOR_BALL_3D_H
#define RCSSMONITOR_BALL_3D_H

#include "disp_holder.h"

#include <QQuaternion>
#include <QVector3D>

#include <array>
#include <vector>

/*!
  \brief the ball of the 3D view (docs/PLAN_3D-VIEW.md §4.3, 8-4,
  rcssmonitor-web ball.ts).

  The pattern of a football: twelve dark pentagons at the corners of an
  icosahedron, their corners a third of the way to the neighbors (as on a
  truncated icosahedron). The shader (ball.frag) finds them from the
  direction on the ball at each pixel, so that the edges stay sharp however
  near the camera is; patternAt() is the same on the CPU (for the tests).

  The ball rolls as far as it is drawn moving (BallSpin): from the frames of
  the log alone, so that a paused, stepped or rewound view shows it turned
  the same, as the figures (§5.2).
*/
namespace Ball3D {

//! at most this much a cycle (radians), so that a fast ball does not seem to turn back at 60 fps
constexpr float MAX_ROLL = 0.8f * 3.14159265f;
//! the pentagons' corners from their centers (radians: about 21 degrees, a third of the way to the neighbors)
constexpr float PENTAGON = 0.368f;
//! how dark a pentagon is (of the ball's color)
constexpr float PENTAGON_SHADE = 0.12f;
//! the pentagons
constexpr int PENTAGONS = 12;

/*!
  \struct Pentagons
  \brief the pentagons' centers (the icosahedron's corners, of length 1),
  and for each the direction of a corner: its nearest neighbor
*/
struct Pentagons {
    std::array< QVector3D, PENTAGONS > centers;
    std::array< QVector3D, PENTAGONS > toward;
};

//! the pentagons on a ball of radius 1
const Pentagons & pentagons();

/*!
  \brief the pattern at a direction on the ball (of the ball itself, before
  it is turned): PENTAGON_SHADE in a pentagon, 1 outside (ball.frag smooths
  the edge over a pixel)
*/
float patternAt( const QVector3D & direction );

/*!
  \brief the turn of rolling on the ground from (ax, ay) to (bx, by) (on
  the field) on a ball of radius: about the axis up x the way, as far as
  the distance over the radius, at most MAX_ROLL
  \param fraction the fraction of the way rolled
  \return no turn if it did not move
*/
QQuaternion roll( const float ax,
                  const float ay,
                  const float bx,
                  const float by,
                  const float radius,
                  const float fraction = 1.0f );

/*!
  \brief whether the ball rolls from frame a to frame b: as the view moves
  it between them (Interpolate::show), not over a play mode change nor a
  jump farther than Interpolate::MAX_BALL_STEP (a placement)
*/
bool rolls( const rcss::rcg::DispInfoT & a,
            const rcss::rcg::DispInfoT & b );

}

/*!
  \class BallSpin
  \brief how the ball is turned at each frame: rolled along its path from
  the first frame, read from the frames once and kept (as GaitTable).

  A live game adds frames, and drops the oldest ones (DispHolder): update()
  follows both (the ball stays turned as it was), and starts again for
  another log or another radius.
*/
class BallSpin {
public:

    /*!
      \brief follow the frames: roll the new ones, drop the ones dropped
      from the front (live), or start again for other frames
      \param frames the frames (DispHolder::dispCont())
      \param radius the ball as drawn (m)
    */
    void update( const DispCont & frames,
                 const float radius );

    //! forget the frames
    void clear();

    //! the number of the frames rolled
    std::size_t frameCount() const { return M_turns.size(); }

    /*!
      \brief how the ball is turned at a time between two frames
      \param frames the frames, as given to update()
      \param index the frame drawn
      \param next the frame toward which it moves (== index if none)
      \param alpha the fraction of the way to next
    */
    QQuaternion turnAt( const DispCont & frames,
                        const std::size_t index,
                        const std::size_t next,
                        const double alpha ) const;

private:

    std::vector< QQuaternion > M_turns; //!< by frame
    DispConstPtr M_last; //!< the last frame rolled, to find it again after the oldest are dropped
    float M_radius = 0.0f;
};

#endif
