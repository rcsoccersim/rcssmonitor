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

#include <array>
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
  \struct GaitWays
  \brief the strides of the clips of walking and running by the way a
  figure goes from its body direction (docs/PLAN_3D-VIEW.md §5.8, §5.9):
  forward, to its right, backward, to its left, each slowest first.

  With the forward ones only (the classic way), a figure going backward
  plays them backward (its gait taken back), and going sideways it turns
  its hips (Figures3D). With the backward ones (clips of walking backward,
  then the forward ones played backward faster than those), or the
  sideways ones too, the gait goes on whichever way the figure goes, and
  the clips of the two ways nearest to the way it goes are mixed (mixAt()):
  between forward and backward in a band about sideways without sideways
  clips; with them, between the two ways around it, by weights that move
  a foot on the ground along the way it goes, so that it does not slide.
  With sideways clips, the hips turn toward the way gone up to TWIST_FULL
  from forward or backward (twistAt()), so that a figure running nearly
  forward runs forward with its hips turned, the clips mixed for what is
  left (clipWay()): none to TWIST_FULL, all sideways.
*/
struct GaitWays {
    //! the ways, as the strides' indices
    enum Way {
        Forward = 0,
        Right = 1,
        Backward = 2,
        Left = 3,
    };
    //! without sideways clips, forward and backward are mixed in this band about sideways (degree, each side)
    static constexpr float BACK_BAND = 20.0f;
    //! with sideways clips, the hips turn toward the way gone to this far from forward or backward (degree)
    static constexpr float TWIST_FULL = 30.0f;

    std::array< std::vector< GaitStride >, 4 > strides; //!< by the way, slowest first

    bool operator==( const GaitWays & other ) const;
    bool operator!=( const GaitWays & other ) const { return ! ( *this == other ); }

    //! with clips of the ways besides forward: the gait goes on whichever way the figure goes
    bool directional() const { return ! strides[Backward].empty() || sideways(); }
    //! with sideways clips (both sides)
    bool sideways() const { return ! strides[Right].empty() && ! strides[Left].empty(); }

    /*!
      \brief a stride of clips at a speed: between their strides by the
      speed, the slowest one's below it, the fastest one's above it
      \return m per cycle (1 without strides)
    */
    static float strideAt( const std::vector< GaitStride > & strides,
                           const float speed );

    /*!
      \brief with sideways clips, how far the hips turn toward the way gone
      (radian, to the right positive): as far as the way from forward (or
      backward) to TWIST_FULL, then less to none sideways; without them, 0
      \param toward the way gone from the body direction (radian, to the right positive)
    */
    float twistAt( const float toward ) const;

    /*!
      \brief the way that the clips go from the hips turned by twistAt():
      forward (or backward) to TWIST_FULL, then on to sideways (radian,
      to the right positive); the hips' turn and it make the way gone
    */
    static float clipWay( const float toward );

    /*!
      \struct Mix
      \brief how the ways are mixed going one way: b by t over a, and the
      stride of the mix along the way gone
    */
    struct Mix {
        int a = Forward;
        int b = Forward;
        float t = 0.0f; //!< how far toward b (0 to 1)
        float stride = 1.0f; //!< m per cycle along the way gone
    };

    /*!
      \brief how the ways are mixed at a speed going a way: forward only
      (the classic way: the stride forward's, whichever way); else between
      forward and backward, in a band about sideways (BACK_BAND); else
      (sideways clips) between the two ways around the way of the clips
      from the hips turned (clipWay(), 90 degrees apart), b by
      w_b = s_a sin θ / (s_a sin θ + s_b cos θ), θ from a, so that a foot on
      the ground goes along the way gone, a stride of
      s_a s_b / (s_a sin θ + s_b cos θ) (s: each way's stride at the speed)
      \param speed m/s
      \param toward the way gone from the body direction (radian; to the
      right: positive, as the field's angles)
    */
    Mix mixAt( const float speed,
               const float toward ) const;
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
  direction) takes the cycles back, so that the clips play backward; with
  clips of other ways (GaitWays::directional()), the cycles go on
  whichever way, each as long as the stride of the clips mixed for the way
  gone (GaitWays::mixAt()). A move
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
      \brief the strides of the clips of walking and running by the way, each
      in the order of their speeds (the table starts again when they change)
    */
    void setWays( const GaitWays & ways );

    //! the strides of the clips of walking and running forward only (setWays())
    void setStrides( const std::vector< GaitStride > & strides );

    const GaitWays & ways() const { return M_ways; }
    const std::vector< GaitStride > & strides() const { return M_ways.strides[GaitWays::Forward]; }

    /*!
      \brief the stride forward at a speed: between the clips' strides by
      the speed, the slowest one's below it, the fastest one's above it
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
    GaitWays M_ways;

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
