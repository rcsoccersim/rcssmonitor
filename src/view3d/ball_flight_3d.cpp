// -*-c++-*-

/*!
  \file ball_flight_3d.cpp
  \brief the ball flying in the air in log replay, virtually. Source File.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ball_flight_3d.h"

#include "interpolate.h"

#include "options.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace {

//! the upward speeds tried for a lofted kick, from 0 to the one reaching BallFlight::CEILING
constexpr int SPEED_STEPS = 1500; // fine: a window of speeds meeting the constraints may be narrow
//! a change of the ball's move (m/cycle) beyond its decay larger than this, of its speed and more, is a collision (ball_rand makes some)
constexpr float COLLISION_SHARE = 0.2f;
constexpr float COLLISION_MIN = 0.08f;
//! the opponents near the ball's way are looked for from this many cycles after the kick to this many before the next touch
constexpr std::size_t LANE_MARGIN = 3;
//! a cross: from a wing (|y| at least this) in the attacking third (x at least this, toward the goal attacked)
constexpr float CROSS_FROM_X = 25.0f;
constexpr float CROSS_FROM_Y = 18.0f;
//! into the penalty area
constexpr float CROSS_TO_X = 36.0f;
constexpr float CROSS_TO_Y = 20.0f;

float
distance( const rcss::rcg::BallT & a,
          const rcss::rcg::BallT & b )
{
    return std::hypot( b.x_ - a.x_, b.y_ - a.y_ );
}

}

/*-------------------------------------------------------------------*/
void
BallFlight::clear()
{
    M_segments.clear();
    M_segment_of.clear();
    M_frames = 0;
    M_front.reset();
    M_back.reset();
    M_active = false;
}

/*-------------------------------------------------------------------*/
void
BallFlight::update( const DispCont & frames,
                    const rcss::rcg::ServerParamT & sp,
                    const float ball_radius )
{
    const double step = std::max( 1, sp.simulator_step_ ) * 1.0e-3; // s
    const double g = GRAVITY * step * step; // m/cycle^2
    const double decay = ( sp.ball_decay_ > 0.0 ? sp.ball_decay_ : 0.94 );
    if ( frames.size() == M_frames
         && ( frames.empty() || ( frames.front() == M_front && frames.back() == M_back ) )
         && g == M_g
         && decay == M_decay
         && ball_radius == M_radius
         && sp.goal_width_ == M_goal_width )
    {
        return;
    }
    clear();
    M_g = g;
    M_decay = decay;
    M_radius = ball_radius;
    M_goal_width = sp.goal_width_;
    M_frames = frames.size();
    if ( ! frames.empty() )
    {
        M_front = frames.front();
        M_back = frames.back();
    }
    build( frames );
}

/*-------------------------------------------------------------------*/
void
BallFlight::build( const DispCont & frames )
{
    const std::size_t n = frames.size();
    if ( n == 0 ) return;
    // a log with heights of its own (a 2.5D server): those are used
    for ( const DispConstPtr & f : frames )
    {
        if ( f->show_.ball_.z_ != 0.0f ) return;
    }
    M_active = true;

    // the touches: at the frame before a bit (its moment; FigureActionLog)
    std::vector< Touch > touch( n, Touch::None );
    std::vector< char > placed( n, 0 ); // the ball placed at the frame: a play mode change, a jump
    constexpr int PLAYERS = rcss::rcg::MAX_PLAYER * 2;
    for ( std::size_t k = 0; k + 1 < n; ++k )
    {
        const rcss::rcg::ShowInfoT & now = frames[k]->show_;
        const rcss::rcg::ShowInfoT & next = frames[k + 1]->show_;
        Touch found = Touch::None;
        for ( int p = 0; p < PLAYERS; ++p )
        {
            const auto s = next.player_[p].state_;
            if ( ! next.player_[p].isAlive() ) continue;
            if ( s & ( rcss::rcg::CATCH | rcss::rcg::CATCH_FAULT ) )
            {
                found = Touch::Catch;
                break; // the hands first
            }
            if ( found == Touch::None
                 && ( s & ( rcss::rcg::KICK | rcss::rcg::KICK_FAULT ) ) )
            {
                found = Touch::Kick;
            }
            // a tackle at the start of its bit's run (it stays for tackle_cycles)
            if ( found == Touch::None
                 && ( s & ( rcss::rcg::TACKLE | rcss::rcg::TACKLE_FAULT ) )
                 && ! ( now.player_[p].state_ & ( rcss::rcg::TACKLE | rcss::rcg::TACKLE_FAULT ) ) )
            {
                found = Touch::Tackle;
            }
        }
        if ( found != Touch::None )
        {
            touch[k] = found;
        }
        // placed between k and k + 1: the next segment from k + 1, from the
        // ground. Not a kick taking a set piece, which changes the play mode
        // itself (a corner kick to play on).
        const bool kicked = ( found == Touch::Kick );
        if ( ( frames[k]->pmode_ != frames[k + 1]->pmode_ && ! kicked )
             || distance( now.ball_, next.ball_ ) > Interpolate::MAX_BALL_STEP )
        {
            placed[k + 1] = 1;
            if ( touch[k + 1] == Touch::None ) touch[k + 1] = Touch::PlayMode;
        }
    }
    // the ball hit a player: its bit at the frame after the collision (the
    // server moves the objects, then resolves the collisions, then writes
    // the frame), so the frame of the bit is the moment, the ball already
    // back from the body. A kick after it keeps the touch a kick.
    for ( std::size_t k = 1; k < n; ++k )
    {
        if ( touch[k] != Touch::None ) continue;
        for ( const rcss::rcg::PlayerT & p : frames[k]->show_.player_ )
        {
            if ( p.isAlive() && ( p.state_ & rcss::rcg::BALL_COLLIDE ) )
            {
                touch[k] = Touch::Collision;
                break;
            }
        }
    }
    // the changes of the ball's move that its decay does not explain (a
    // post, which has no bit; an old log without the bits)
    for ( std::size_t k = 1; k + 1 < n; ++k )
    {
        if ( touch[k] != Touch::None || placed[k] || placed[k + 1] ) continue;
        const rcss::rcg::BallT & a = frames[k - 1]->show_.ball_;
        const rcss::rcg::BallT & b = frames[k]->show_.ball_;
        const rcss::rcg::BallT & c = frames[k + 1]->show_.ball_;
        const float dx0 = b.x_ - a.x_, dy0 = b.y_ - a.y_;
        const float dx1 = c.x_ - b.x_, dy1 = c.y_ - b.y_;
        const float ex = dx1 - float( M_decay ) * dx0;
        const float ey = dy1 - float( M_decay ) * dy0;
        if ( std::hypot( ex, ey ) > COLLISION_MIN + COLLISION_SHARE * std::hypot( dx0, dy0 ) )
        {
            touch[k] = Touch::Collision;
        }
    }

    // the segments between the touches
    std::size_t first = 0;
    for ( std::size_t k = 1; k < n; ++k )
    {
        if ( touch[k] == Touch::None && k + 1 < n ) continue;
        Segment s;
        s.first = first;
        s.last = k;
        s.start = touch[first];
        s.end = touch[k];
        M_segments.push_back( s );
        first = k;
    }
    if ( M_segments.empty() )
    {
        Segment s; // a single frame
        M_segments.push_back( s );
    }

    // the highest the ball may be at the end of each: the next toucher's
    // reach. A collision followed soon by a kick, a tackle or a catch is a
    // trap (the ball stopped by a foot, or into the hands), not the ball
    // stopped high by the body and kicked a moment later.
    for ( std::size_t i = 0; i < M_segments.size(); ++i )
    {
        Segment & s = M_segments[i];
        const auto reachOf = []( const Touch t ) {
            switch ( t ) {
            case Touch::Kick: return FOOT_REACH;
            case Touch::Tackle: return TACKLE_REACH;
            case Touch::Catch: return CATCH_REACH;
            case Touch::Collision: return BODY_REACH;
            default: return -1.0f;
            }
        };
        s.reach = reachOf( s.end );
        if ( s.end == Touch::Collision
             && i + 1 < M_segments.size() )
        {
            const Segment & after = M_segments[i + 1];
            if ( after.last - after.first <= TRAP_CYCLES
                 && ( after.end == Touch::Kick || after.end == Touch::Tackle || after.end == Touch::Catch ) )
            {
                s.reach = reachOf( after.end );
            }
        }
    }

    // their heights, in order: each from where the one before ended
    float h = 0.0f;
    for ( Segment & s : M_segments )
    {
        s.h0 = ( placed[s.first] || s.first == 0 ? 0.0f : h );
        s.v = 0.0f;
        s.loft = Loft::None;
        s.bounce = false;
        if ( s.start == Touch::Kick ) loft( frames, s );
        h = height( s, double( s.last - s.first ), M_g );
    }
    // the segment of each frame
    M_segment_of.assign( n, 0 );
    for ( std::size_t i = 0; i < M_segments.size(); ++i )
    {
        const Segment & s = M_segments[i];
        for ( std::size_t f = s.first; f < s.last; ++f ) M_segment_of[f] = int( i );
    }
    M_segment_of[n - 1] = int( M_segments.size() - 1 );
}

/*-------------------------------------------------------------------*/
void
BallFlight::loft( const DispCont & frames,
                  Segment & s ) const
{
    const std::size_t span = s.last - s.first;
    if ( span < 2 ) return;
    const auto ball = [&]( const std::size_t j ) -> const rcss::rcg::BallT & { return frames[s.first + j]->show_.ball_; };
    // where the ball went: the last frame, or the one before when the ball
    // was placed at the last (a jump: after a goal, to the center)
    const std::size_t end = ( distance( ball( span - 1 ), ball( span ) ) > Interpolate::MAX_BALL_STEP ? span - 1 : span );
    const float speed = distance( ball( 0 ), ball( 1 ) );
    const float far = distance( ball( 0 ), ball( end ) );
    if ( speed < LIFT_SPEED || far < LIFT_DISTANCE ) return;
    // fast and far enough to be lofted for a reason; else a lift at most
    const bool loftable = ( speed >= LOFT_SPEED && far >= MIN_LOFT_DISTANCE );

    // the kicker: who has the kick bit after the touch, nearest the ball
    const rcss::rcg::ShowInfoT & kicked = frames[s.first + 1]->show_;
    int kicker = -1;
    float nearest = std::numeric_limits< float >::max();
    for ( int p = 0; p < rcss::rcg::MAX_PLAYER * 2; ++p )
    {
        const rcss::rcg::PlayerT & pl = kicked.player_[p];
        if ( ! pl.isAlive() || ! ( pl.state_ & ( rcss::rcg::KICK | rcss::rcg::KICK_FAULT ) ) ) continue;
        const float d = std::hypot( pl.x_ - ball( 0 ).x_, pl.y_ - ball( 0 ).y_ );
        if ( d < nearest )
        {
            nearest = d;
            kicker = p;
        }
    }
    if ( kicker < 0 ) return;
    const rcss::rcg::Side side = kicked.player_[kicker].side();
    const float attack = ( side == rcss::rcg::LEFT ? 1.0f : -1.0f ); // the left team attacks +x

    // the opponents near the ball's way that did not touch it, and when
    // (a lift passes them low, as on the ground)
    std::vector< std::size_t > over;
    for ( std::size_t j = LANE_MARGIN; loftable && j + LANE_MARGIN <= span; ++j )
    {
        const rcss::rcg::ShowInfoT & show = frames[s.first + j]->show_;
        for ( int p = 0; p < rcss::rcg::MAX_PLAYER * 2; ++p )
        {
            const rcss::rcg::PlayerT & pl = show.player_[p];
            if ( ! pl.isAlive() || pl.side() == side ) continue;
            if ( std::hypot( pl.x_ - show.ball_.x_, pl.y_ - show.ball_.y_ ) < LANE_REACH )
            {
                over.push_back( j );
                break;
            }
        }
    }

    // why it is lofted, if at all
    const rcss::rcg::PlayMode mode = frames[s.first]->pmode_;
    const rcss::rcg::BallT & from = ball( 0 );
    const rcss::rcg::BallT & to = ball( end );
    Loft loft = Loft::None;
    if ( ! loftable )
    {
        loft = Loft::Lift;
    }
    else if ( mode == rcss::rcg::PM_CornerKick_Left || mode == rcss::rcg::PM_CornerKick_Right
         || ( ( mode == rcss::rcg::PM_GoalKick_Left || mode == rcss::rcg::PM_GoalKick_Right )
              && far >= GOAL_KICK_LOFT ) )
    {
        loft = Loft::SetPiece;
    }
    else if ( attack * from.x_ >= CROSS_FROM_X && std::fabs( from.y_ ) >= CROSS_FROM_Y
              && attack * to.x_ >= CROSS_TO_X && std::fabs( to.y_ ) <= CROSS_TO_Y )
    {
        loft = Loft::Cross;
    }
    else if ( far >= SHOT_FROM
              && attack * ( to.x_ - from.x_ ) > 0.0f
              && ( s.end == Touch::PlayMode || s.end == Touch::Catch )
              && attack * to.x_ >= Options::PITCH_HALF_LENGTH - 3.0
              && std::fabs( to.y_ ) <= Options::GOAL_HALF_WIDTH + SHOT_NEAR_POST )
    {
        loft = Loft::Shot;
    }
    else if ( ! over.empty() )
    {
        loft = Loft::Lane;
    }
    else if ( far >= LONG_BALL )
    {
        loft = Loft::LongBall;
    }
    else
    {
        loft = Loft::Lift;
    }

    // the constraints: at the next touch, no higher than the toucher reaches
    // (found in build(): a trap's collision as the touch after it)
    const float reach = ( s.reach >= 0.0f ? s.reach : std::numeric_limits< float >::max() );
    // under the crossbar where it crosses the goal line between the posts (the ball's top under it)
    double crossing = -1.0;
    const double half_length = Options::PITCH_HALF_LENGTH;
    const double half_goal = ( M_goal_width > 0.0 ? M_goal_width : Options::GOAL_WIDTH ) * 0.5;
    for ( std::size_t j = 0; j < end && crossing < 0.0; ++j )
    {
        const rcss::rcg::BallT & a = ball( j );
        const rcss::rcg::BallT & b = ball( j + 1 );
        for ( const double line : { half_length, -half_length } )
        {
            if ( ( a.x_ - line ) * ( b.x_ - line ) > 0.0 || a.x_ == b.x_ ) continue;
            const double u = ( line - a.x_ ) / ( b.x_ - a.x_ );
            const double y = a.y_ + u * ( b.y_ - a.y_ );
            if ( std::fabs( y ) <= half_goal )
            {
                crossing = double( j ) + u;
                break;
            }
        }
    }
    const float under_bar = CROSSBAR - 2.0f * M_radius - 0.02f;

    // the upward speed whose highest point is nearest the one preferred,
    // among those that meet them; or, for a long one that bounces, whose
    // first landing is nearest the time preferred. Which, and how high or
    // when, varies from kick to kick (variety()).
    const rcss::rcg::DispInfoT & touch = *frames[s.first];
    const double vary = variety( touch.show_.time_, touch.show_.stime_, kicker, 0 );
    const double vary_more = variety( touch.show_.time_, touch.show_.stime_, kicker, 1 );
    const bool long_one = ( loft == Loft::LongBall || loft == Loft::SetPiece || loft == Loft::Cross );
    const bool bounce = ( long_one && vary < BOUNCE_SHARE );
    const double landing = span * ( MIN_LANDING + ( MAX_LANDING - MIN_LANDING ) * vary_more );
    const float preferred = ( loft == Loft::Lane
                              ? HEAD_CLEAR + 0.3f
                              : loft == Loft::Shot
                              ? std::min( MAX_SHOT_APEX, std::max( MIN_SHOT_APEX, SHOT_APEX_PER_DISTANCE * far ) )
                              : loft == Loft::Lift
                              ? float( MIN_LIFT + ( MAX_LIFT - MIN_LIFT ) * vary )
                              : std::min( MAX_APEX, std::max( MIN_APEX, APEX_PER_DISTANCE * far ) ) );
    const double top_speed = std::sqrt( 2.0 * M_g * CEILING );
    Segment trial = s;
    float best = -1.0f;
    float best_miss = std::numeric_limits< float >::max();
    for ( int i = 1; i <= SPEED_STEPS; ++i )
    {
        trial.v = float( top_speed * i / SPEED_STEPS );
        if ( height( trial, double( span ), M_g ) > reach ) continue;
        if ( crossing >= 0.0 && height( trial, crossing, M_g ) > under_bar ) continue;
        bool cleared = true;
        for ( const std::size_t j : over )
        {
            if ( height( trial, double( j ), M_g ) < HEAD_CLEAR )
            {
                cleared = false;
                break;
            }
        }
        if ( ! cleared ) continue;
        const float apex = s.h0 + float( trial.v * trial.v / ( 2.0 * M_g ) );
        const double first_landing = ( trial.v + std::sqrt( double( trial.v ) * trial.v + 2.0 * M_g * s.h0 ) ) / M_g;
        const float miss = ( bounce
                             ? float( std::fabs( first_landing - landing ) )
                             : std::fabs( apex - preferred ) );
        if ( miss < best_miss )
        {
            best_miss = miss;
            best = trial.v;
        }
    }
    if ( best > 0.0f )
    {
        s.v = best;
        s.loft = loft;
        s.bounce = bounce;
    }
}

/*-------------------------------------------------------------------*/
double
BallFlight::variety( const unsigned int time,
                     const unsigned int stime,
                     const int kicker,
                     const int which )
{
    // a hash of the kick (mixed as MurmurHash3's finalizer), to [0, 1)
    std::uint32_t h = std::uint32_t( time ) * 0x9E3779B1u
        ^ std::uint32_t( stime ) * 0x85EBCA77u
        ^ std::uint32_t( kicker + 1 ) * 0xC2B2AE3Du
        ^ std::uint32_t( which + 1 ) * 0x27D4EB2Fu;
    h ^= h >> 16;
    h *= 0x85EBCA6Bu;
    h ^= h >> 13;
    h *= 0xC2B2AE35u;
    h ^= h >> 16;
    return double( h & 0xFFFFFFu ) / double( 1u << 24 );
}

/*-------------------------------------------------------------------*/
float
BallFlight::height( const Segment & s,
                    const double t,
                    const double g )
{
    if ( t <= 0.0 ) return s.h0;
    if ( s.h0 <= 0.0f && s.v <= 0.0f ) return 0.0f;
    // the first flight, from h0 up at v, down to the ground
    const double land = std::sqrt( double( s.v ) * s.v + 2.0 * g * s.h0 ); // how fast it comes down
    double tau = ( s.v + land ) / g;
    if ( t <= tau )
    {
        return float( std::max( 0.0, s.h0 + s.v * t - 0.5 * g * t * t ) );
    }
    // the bounces, each RESTITUTION times as fast up, then rolling
    double rest = t - tau;
    double w = RESTITUTION * land;
    while ( w * w / ( 2.0 * g ) >= MIN_BOUNCE )
    {
        tau = 2.0 * w / g;
        if ( rest <= tau )
        {
            return float( std::max( 0.0, w * rest - 0.5 * g * rest * rest ) );
        }
        rest -= tau;
        w *= RESTITUTION;
    }
    return 0.0f;
}

/*-------------------------------------------------------------------*/
float
BallFlight::heightAt( const std::size_t index,
                      const double alpha ) const
{
    if ( ! M_active || index >= M_segment_of.size() ) return 0.0f;
    const Segment & s = M_segments[std::size_t( M_segment_of[index] )];
    const double t = double( index - s.first ) + std::max( 0.0, std::min( 1.0, alpha ) );
    return height( s, t, M_g );
}
