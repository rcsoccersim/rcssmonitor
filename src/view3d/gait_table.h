// -*-c++-*-

/*!
  \file gait_table.h
  \brief how far the players have walked, for the human figures. Header File.
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

#ifndef RCSSMONITOR_GAIT_TABLE_H
#define RCSSMONITOR_GAIT_TABLE_H

#include "disp_holder.h"
#include "scene_content.h"

#include <vector>

/*!
  \struct GaitStride
  \brief how far a figure goes in one cycle of its gait (two steps) at a
  speed: a clip of walking or running, at its own speed
*/
struct GaitStride {
    float speed; //!< m/s
    float stride; //!< m per cycle of the gait
};

/*!
  \class GaitTable
  \brief how far each player has walked by each frame, in cycles of the
  gait, and how it moves (docs/PLAN_3D-VIEW.md §5.8): read from the frames
  of the log once, and kept, so that a figure's legs are where the log
  says whenever the frame is drawn (paused, stepped back, jumped to), and
  the clips of walking and running go on together, at one phase.

  A cycle of the gait is as long as the stride at the player's speed
  (strideAt(): the strides of the clips, between their speeds), so that the
  feet on the ground do not slide. Going backward (away from the body
  direction) takes the cycles back, so that the clips play backward. A move
  farther than TELEPORT in a cycle (a free kick's placing, a substitution)
  is not walked. The strides are of a figure of a person's height
  (SceneBuilder::PERSON_HEIGHT); a taller figure takes longer strides, and
  so fewer cycles (Figures3D).

  A live game adds frames, and drops the oldest ones (DispHolder): update()
  follows both, and starts again for another log.
*/
class GaitTable {
public:

    //! farther than this in a cycle (m) is a move, not walking
    static constexpr float TELEPORT = 2.0f;
    //! the cycles before and after, over which the acceleration is taken
    static constexpr double ACCELERATION_SPAN = 2.0;

    /*!
      \brief the strides of the clips of walking and running, in the order of
      their speeds (the table starts again when they change)
    */
    void setStrides( const std::vector< GaitStride > & strides );

    const std::vector< GaitStride > & strides() const { return M_strides; }

    /*!
      \brief the stride at a speed: between the clips' strides by the speed,
      the slowest one's below it, the fastest one's above it
      \param speed m/s
      \return m per cycle (1 without strides)
    */
    float strideAt( const float speed ) const;

    /*!
      \brief follow the frames: walk the new ones, drop the ones dropped
      from the front (live), or start again for other frames
      \param frames the frames (DispHolder::dispCont())
      \param step the simulation step (s)
    */
    void update( const DispCont & frames,
                 const double step );

    //! forget the frames
    void clear();

    //! the number of the frames walked
    std::size_t frameCount() const { return M_frames; }

    /*!
      \brief how a player moves at a time between two frames
      \param player its index in the show (0 to 21)
      \param index the frame drawn
      \param next the frame toward which it moves (== index if none)
      \param alpha the fraction of the way to next
    */
    FigureMotion motion( const int player,
                         const std::size_t index,
                         const std::size_t next,
                         const double alpha ) const;

private:

    //! the players of a frame
    static constexpr int PLAYERS = rcss::rcg::MAX_PLAYER * 2;

    //! a player at a frame
    struct Entry {
        double gait; //!< cycles walked by the frame
        float vx; //!< the velocity of the move from the frame before (m/s)
        float vy;
    };

    std::vector< Entry > M_entries; //!< by frame * PLAYERS + player
    std::size_t M_frames = 0;
    DispConstPtr M_last; //!< the last frame walked, to find it again after the oldest are dropped
    double M_step = 0.0;
    std::vector< GaitStride > M_strides;

    const Entry & entry( const std::size_t frame,
                         const int player ) const
      {
          return M_entries[frame * PLAYERS + player];
      }

    //! walk the frames from M_frames to the end
    void extend( const DispCont & frames );

    //! the velocity at a time (frames, with the fraction): through the moves' velocities, each at the middle of its move
    void velocityAt( const int player,
                     const double time,
                     float * vx,
                     float * vy ) const;
};

#endif
