// -*-c++-*-

/*!
  \file ball_flight_3d.h
  \brief the ball flying in the air in log replay, virtually. Header File.
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

#ifndef RCSSMONITOR_BALL_FLIGHT_3D_H
#define RCSSMONITOR_BALL_FLIGHT_3D_H

#include "disp_holder.h"

#include <rcss/rcg/types.h>

#include <set>
#include <utility>

#include <vector>

/*!
  \class BallFlight
  \brief the ball flying in the air, virtually, in the replay of a log of
  today's 2D server, which has no height (docs/plan/2026-11-11_3d-view.md 8-6, the
  option ball_flight_3d).

  The ball's way on the ground is the log's; only its height is added. The
  log is cut into segments at the touches: a kick, a tackle or a catch (at
  the moment of the touch, the frame before its bit, as FigureActionLog),
  the ball hitting a player (the player's BALL_COLLIDE bit, at its frame:
  the server resolves the collisions after the move), a change of the
  ball's velocity that its decay does not explain (a post, which has no
  bit), a play mode change and a jump (a placement).

  A segment from a kick is lofted when the log gives a reason, as a real
  player would loft it: the ball fast and going far (a long ball), a set
  piece (a corner kick, a goal kick), a cross from a wing into the penalty
  area, a shot (a little), or opponents near the ball's way that did not
  touch it (it went over them). Some of the long ones land halfway and
  bounce on to the receiver, the others fly to it. A fast kick without a
  reason goes a little up (a lift), lands and bounces low. Which, and how
  high, varies from kick to kick by its time and its kicker (variety()),
  not at random: the same every time. Its height is a flight under gravity, then bounces (each
  RESTITUTION times as fast up), then rolling. The upward speed at the
  kick is the one, among those that meet the segment's constraints, whose
  highest point is nearest the one preferred for it:
  - at the next touch, no higher than the toucher reaches (a foot, a tackle,
    a goalkeeper's hands, a body);
  - under the crossbar where it crosses the goal line between the posts
    (a goal in the 2D server);
  - over the head of each opponent it went past.
  Without one, it stays on the ground. A segment starts at the height the
  one before ended at (a volley), except after a play mode change.

  The idea comes from a paper of Osaka Metropolitan University, whose 3D
  log viewer lofts the ball between the kicks of a 2D game log: Hatakeyama
  et al., a log visualization system for soccer analysis, JSAI SIG
  technical report SAI-047, 2024 (in Japanese only),
  https://doi.org/10.11517/jsaisigtwo.2023.SAI-047_05. Which kicks fly and
  how high are found here in rcssmonitor's own way.

  A kick's segment also ends where the ball almost stops, without a touch
  before (rolling no farther than the kickable distance, by its decay:
  §5.10), the ball on the ground there: the next touch long after is not
  what the kick was aimed at.

  It is found from the frames alone, once (as BallSpin), so that a paused,
  stepped or rewound view shows the ball at the same height (§5.2). Not
  for a live game as it comes (the next touch is not known yet), but for
  the delayed live view (§5.10): drawn some seconds behind, most segments
  have ended before they are drawn; one that the view enters before its
  end is known stays on the ground for good (grounded), so that the ball
  does not rise when its end comes. Not when the log has heights of its
  own (a 2.5D server): those are used.
*/
class BallFlight {
public:

    //! gravity (m/s^2)
    static constexpr double GRAVITY = 9.8;
    //! how fast the ball goes up after a bounce, of how fast it came down
    static constexpr float RESTITUTION = 0.6f;
    //! a bounce lower than this (m) is rolling
    static constexpr float MIN_BOUNCE = 0.02f;

    //! a kick at least this fast (m/cycle) may be lofted
    static constexpr float LOFT_SPEED = 1.5f;
    //! and going at least this far (m) to the next touch
    static constexpr float MIN_LOFT_DISTANCE = 10.0f;
    //! a kick going this far (m) is a long ball, lofted (about a tenth of the fast kicks in a game)
    static constexpr float LONG_BALL = 20.0f;
    //! a shot: from this far (m), toward the goal (ending at its line, near the posts, or in a goalie's hands)
    static constexpr float SHOT_FROM = 12.0f;
    static constexpr float SHOT_NEAR_POST = 4.0f;
    //! a shot's highest point preferred: this much of its distance, within MIN_SHOT_APEX and MAX_SHOT_APEX
    static constexpr float SHOT_APEX_PER_DISTANCE = 0.06f;
    static constexpr float MIN_SHOT_APEX = 0.8f;
    static constexpr float MAX_SHOT_APEX = 2.0f;
    //! a goal kick going this far (m) is lofted
    static constexpr float GOAL_KICK_LOFT = 20.0f;
    //! an opponent this near (m) the ball's way that did not touch it: the ball went over it
    static constexpr float LANE_REACH = 2.5f;
    //! over an opponent's head (m)
    static constexpr float HEAD_CLEAR = 2.0f;

    //! the highest the next toucher reaches (m): a foot, a sliding tackle, a goalkeeper's hands, a body
    static constexpr float FOOT_REACH = 0.5f;
    static constexpr float TACKLE_REACH = 0.3f;
    static constexpr float CATCH_REACH = 2.3f;
    static constexpr float BODY_REACH = 1.8f;
    //! a collision followed by a kick, a tackle or a catch within this many cycles: a trap, the ball stopped by a foot
    static constexpr std::size_t TRAP_CYCLES = 3;
    //! the crossbar's height (m), the goal's
    static constexpr float CROSSBAR = 2.44f;

    //! the highest point preferred for a lofted kick: this much of its distance, within MIN_APEX and MAX_APEX
    static constexpr float APEX_PER_DISTANCE = 0.1f;
    static constexpr float MIN_APEX = 1.5f;
    static constexpr float MAX_APEX = 8.0f;
    //! the highest point considered (m)
    static constexpr float CEILING = 12.0f;

    //! of the long ones (a long ball, a set piece, a cross), so many land halfway and bounce on
    static constexpr double BOUNCE_SHARE = 0.6;
    //! where they land first, of the time to the next touch
    static constexpr double MIN_LANDING = 0.5;
    static constexpr double MAX_LANDING = 0.75;
    //! a kick at least this fast (m/cycle), going at least this far (m), without a reason to loft: a lift
    static constexpr float LIFT_SPEED = 1.2f;
    static constexpr float LIFT_DISTANCE = 6.0f;
    //! a lift's highest point (m)
    static constexpr float MIN_LIFT = 0.2f;
    static constexpr float MAX_LIFT = 0.7f;

    //! what starts or ends a segment
    enum class Touch {
        None, //!< the start or the end of the log
        Kick,
        Tackle,
        Catch,
        Collision, //!< the ball hit a player's body (its bit) or a post (a change of velocity without a touch)
        PlayMode, //!< a play mode change or a jump: the ball placed
        Stop, //!< no touch: a kick's ball almost stopped (stopCycles())
    };

    //! why a kick was lofted
    enum class Loft {
        None,
        LongBall,
        SetPiece,
        Cross,
        Shot,
        Lane, //!< over opponents
        Lift, //!< a little up, without a reason to loft
    };

    /*!
      \struct Segment
      \brief the ball from a touch to the next one
    */
    struct Segment {
        std::size_t first = 0; //!< the frame of the touch (its moment: the ball leaves after it)
        std::size_t last = 0; //!< the frame of the next touch (or the last frame)
        Touch start = Touch::None;
        Touch end = Touch::None;
        Loft loft = Loft::None;
        float h0 = 0.0f; //!< the height at first (m)
        float v = 0.0f; //!< the upward speed at first (m/cycle)
        bool bounce = false; //!< a long one landing halfway (MIN_LANDING to MAX_LANDING of the way in time), bouncing on
        float reach = 0.0f; //!< the highest the ball may be at last (m; the next toucher's reach), negative: anywhere
        bool grounded = false; //!< live, the view entered it before its end was known: not lofted
    };

    //! update() for log replay: no frame drawn live
    static constexpr std::size_t NOT_LIVE = static_cast< std::size_t >( -1 );

    /*!
      \brief the cycles from a kick until the ball almost stops: until it
      rolls no farther than the kickable distance, its speed decaying by
      decay a cycle (6.2 s from ball_speed_max 3.0 with the default
      parameters: the largest delay of the delayed live view, §5.10)
      \param speed the ball's move in the cycle after the kick (m)
      \param decay ball_decay
      \param kickable the kickable distance (m): player_size + kickable_margin + ball_size
      \return 0 if it is already that slow
    */
    static std::size_t stopCycles( const double speed,
                                   const double decay,
                                   const double kickable );

    //! the kickable distance of the parameters (player_size + kickable_margin + ball_size; the defaults if not given)
    static double kickableDistance( const rcss::rcg::ServerParamT & sp );

    /*!
      \brief a number from 0 to 1 that varies from kick to kick, by its
      time and its kicker: which way and how high a kick flies (not at
      random, so that a frame looks the same every time)
      \param time the game's time of the kick (cycle)
      \param stime the stopped time
      \param kicker the kicker's index in the show
      \param which another number for the same kick (0, 1, ...)
    */
    static double variety( const unsigned int time,
                           const unsigned int stime,
                           const int kicker,
                           const int which );

    /*!
      \brief find the segments and their heights for the frames, again only
      when they changed (other frames, more frames, other parameters)
      \param frames the frames (DispHolder::dispCont())
      \param sp the server parameters (ball_decay, simulator_step, goal_width, the kickable distance)
      \param ball_radius the ball as drawn (m): under the crossbar
      \param live_drawn the delayed live view: the frame drawn. The kick's
      segment it is in stays on the ground for good if its end is not
      known yet (decided()). NOT_LIVE for log replay.
    */
    void update( const DispCont & frames,
                 const rcss::rcg::ServerParamT & sp,
                 const float ball_radius,
                 const std::size_t live_drawn = NOT_LIVE );

    //! forget the frames, and the segments grounded live
    void clear();

    /*!
      \brief a segment's end is known, and will not change with more frames:
      not the last one, and if it ends at a collision, the touch that may
      follow it soon (a trap: its reach) is known too
    */
    bool decided( const std::size_t i ) const;

    //! the heights are there: frames without heights of their own
    bool active() const { return M_active; }

    const std::vector< Segment > & segments() const { return M_segments; }

    /*!
      \brief the height of the ball (m, its bottom above the ground) at a
      time between two frames
      \param index the frame
      \param alpha the fraction of the way to the next one (0 where the view does not move the ball to it)
      \return 0 when not active
    */
    float heightAt( const std::size_t index,
                    const double alpha ) const;

    //! the height at a frame
    float heightAtFrame( const std::size_t index ) const { return heightAt( index, 0.0 ); }

    /*!
      \brief the height of a segment's ball t cycles after its first frame:
      the flight from h0 with the upward speed v, then the bounces, then
      rolling
      \param g gravity (m/cycle^2)
    */
    static float height( const Segment & segment,
                         const double t,
                         const double g );

private:

    std::vector< Segment > M_segments;
    //! more frames of the same (live): the segments before, whose heights are kept where they are the same
    std::vector< Segment > M_previous;
    std::vector< int > M_segment_of; //!< by frame: the segment it starts or is in
    std::vector< char > M_placed; //!< by frame: the ball placed there (a play mode change, a jump)
    //! live: the segments grounded (grounded), by the time and the stopped time of their first frames
    std::set< std::pair< unsigned int, unsigned int > > M_grounded;
    std::size_t M_frames = 0;
    DispConstPtr M_front; //!< the first and the last frame read, to see that they are the same
    DispConstPtr M_back;
    double M_g = 0.0; //!< m/cycle^2
    double M_decay = 0.0;
    float M_radius = 0.0f;
    double M_goal_width = 0.0;
    double M_kickable = 0.0; //!< m
    bool M_active = false;

    //! forget the segments, not those grounded
    void reset();
    void build( const DispCont & frames );
    //! the heights of the segments, in order (each from where the one before ended)
    void heights( const DispCont & frames );
    //! live: ground the kick's segment drawn if its end is not known yet (the heights again)
    void groundDrawn( const DispCont & frames,
                      const std::size_t live_drawn );
    void loft( const DispCont & frames,
               Segment & segment ) const;
};

#endif
