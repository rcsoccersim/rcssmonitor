// -*-c++-*-

/*!
  \file figure_actions.cpp
  \brief what the players do besides walking, for the human figures. Source File.
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

#include "figure_actions.h"

#include "ball_flight_3d.h"

#include "options.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

//! frames with a bit, from first to last
struct Run {
    std::size_t first;
    std::size_t last;
};

//! degrees in [-180, 180)
float
normalize( float degree )
{
    return static_cast< float >( std::remainder( degree, 360.0 ) );
}

//! the frames around the time drawn, and a player's bits in them
class Frames {
public:
    Frames( const DispCont & frames,
            const int player )
        : M_frames( frames ),
          M_player( player )
      { }

    const rcss::rcg::ShowInfoT & show( const std::size_t i ) const { return M_frames[i]->show_; }
    const rcss::rcg::PlayerT & player( const std::size_t i ) const { return show( i ).player_[M_player]; }

    bool has( const std::size_t i,
              const int bit ) const
      {
          // (a player not connected has no bits)
          return ( i < M_frames.size()
                   && ( player( i ).state_ & bit ) != 0 );
      }

    /*!
      \brief the runs of a bit near frame f, the latest first: those with a
      frame from f - LOOK_BACK to f + LOOK_AHEAD
    */
    std::vector< Run > runs( const std::size_t f,
                             const int bit ) const
      {
          std::vector< Run > found;
          if ( M_frames.empty() ) return found;
          const std::size_t top = std::min( f + FigureActionLog::LOOK_AHEAD, M_frames.size() - 1 );
          const std::size_t bottom = ( f > std::size_t( FigureActionLog::LOOK_BACK ) ? f - FigureActionLog::LOOK_BACK : 0 );
          for ( std::size_t j = top + 1; j > bottom; )
          {
              --j;
              if ( ! has( j, bit ) ) continue;
              Run run{ j, j };
              while ( run.first > 0 && j - run.first < std::size_t( FigureActionLog::MAX_RUN ) && has( run.first - 1, bit ) ) --run.first;
              while ( run.last - run.first < std::size_t( FigureActionLog::MAX_RUN ) && has( run.last + 1, bit ) ) ++run.last;
              found.push_back( run );
              j = run.first;
          }
          return found;
      }

    //! the ball's move into frame i (m), none at the first frame
    void ballMove( const std::size_t i,
                   float * dx,
                   float * dy ) const
      {
          const rcss::rcg::BallT & b = show( i ).ball_;
          if ( b.hasVelocity() )
          {
              // the velocity after the move (decayed once): the way, and a speed near the kick's
              *dx = b.vx_;
              *dy = b.vy_;
              return;
          }
          *dx = ( i > 0 ? b.x_ - show( i - 1 ).ball_.x_ : 0.0f );
          *dy = ( i > 0 ? b.y_ - show( i - 1 ).ball_.y_ : 0.0f );
      }

    //! where the ball is at frame i, from the player at frame i, turned to its body direction: x ahead, y to its right
    void ballFromPlayer( const std::size_t i,
                         float * ahead,
                         float * right ) const
      {
          const rcss::rcg::PlayerT & p = player( i );
          const float dx = show( i ).ball_.x_ - p.x_;
          const float dy = show( i ).ball_.y_ - p.y_;
          const float rad = p.body_ * float( M_PI ) / 180.0f;
          *ahead = dx * std::cos( rad ) + dy * std::sin( rad );
          *right = -dx * std::sin( rad ) + dy * std::cos( rad );
      }

private:
    const DispCont & M_frames;
    const int M_player;
};

/*!
  \brief a kick from a run of frames with the kick bit: at the frame that
  sent the ball fastest
*/
FigureKick
kickOf( const Frames & frames,
        const Run & run,
        const rcss::rcg::ServerParamT & sp,
        const float ball_radius )
{
    std::size_t strongest = run.first;
    float speed = -1.0f, dx = 0.0f, dy = 0.0f;
    for ( std::size_t k = run.first; k <= run.last; ++k )
    {
        float x, y;
        frames.ballMove( k, &x, &y );
        const float s = std::hypot( x, y );
        if ( s >= speed ) // the later one if as fast
        {
            strongest = k;
            speed = s;
            dx = x;
            dy = y;
        }
    }

    FigureKick kick;
    kick.on = true;
    kick.phase = static_cast< float >( strongest ) - 1.0f; // the moment, made relative by the caller
    // before the kick, where the ball was: the foot on its side
    const std::size_t before = ( strongest > 0 ? strongest - 1 : 0 );
    const rcss::rcg::PlayerT & p = frames.player( before );
    float ahead, right;
    frames.ballFromPlayer( before, &ahead, &right );
    kick.right = ( right >= 0.0f );
    // where the foot meets the ball, and the way it sends it
    kick.ball_x = frames.show( before ).ball_.x_;
    kick.ball_y = frames.show( before ).ball_.y_;
    if ( speed > 1.0e-4f )
    {
        kick.dir_x = dx / speed;
        kick.dir_y = dy / speed;
    }
    else
    {
        // not moved: ahead of the body
        kick.dir_x = std::cos( p.body_ * float( M_PI ) / 180.0f );
        kick.dir_y = std::sin( p.body_ * float( M_PI ) / 180.0f );
    }
    kick.ball_radius = ball_radius;
    kick.way = ( speed > 1.0e-4f ? normalize( std::atan2( dy, dx ) * 180.0f / float( M_PI ) - p.body_ ) : 0.0f );

    kick.kind = ( speed < FigureActionLog::PASS_SPEED ? KickKind::Tap : KickKind::Pass );
    if ( speed >= FigureActionLog::SHOT_SPEED )
    {
        // toward the opponents' goal: the left team attacks +x
        const rcss::rcg::BallT & b = frames.show( strongest ).ball_;
        const float half_length = float( Options::PITCH_HALF_LENGTH );
        const float goal_x = ( p.side() == rcss::rcg::LEFT ? half_length : -half_length );
        const float to_goal = goal_x - b.x_;
        if ( dx * to_goal > 0.0f
             && std::fabs( to_goal ) <= FigureActionLog::SHOT_RANGE )
        {
            const float y = b.y_ + dy / dx * to_goal;
            if ( std::fabs( y ) <= sp.goal_width_ * 0.5f + FigureActionLog::SHOT_MARGIN )
            {
                kick.kind = KickKind::Shot;
            }
        }
    }
    return kick;
}

}

/*-------------------------------------------------------------------*/
FigureActions
FigureActionLog::read( const DispCont & frames,
                       const int player,
                       const std::size_t index,
                       const std::size_t next,
                       const double alpha,
                       const rcss::rcg::PlayerT & drawn,
                       const rcss::rcg::ServerParamT & sp,
                       const float ball_radius,
                       const BallFlight * flight )
{
    FigureActions actions;
    if ( drawn.hasNeck() ) actions.neck = drawn.neck_;
    if ( drawn.hasArm() )
    {
        actions.pointing = true;
        actions.point_distance = std::hypot( drawn.point_x_ - drawn.x_, drawn.point_y_ - drawn.y_ );
        actions.point_way = normalize( std::atan2( drawn.point_y_ - drawn.y_, drawn.point_x_ - drawn.x_ ) * 180.0f / float( M_PI )
                                       - drawn.body_ );
    }
    if ( index >= frames.size()
         || player < 0
         || player >= rcss::rcg::MAX_PLAYER * 2 )
    {
        return actions;
    }

    const Frames log( frames, player );
    const double t = static_cast< double >( index ) + ( next != index ? alpha : 0.0 );

    // the kick: the latest begun
    for ( const Run & run : log.runs( index, rcss::rcg::KICK | rcss::rcg::KICK_FAULT ) )
    {
        FigureKick kick = kickOf( log, run, sp, ball_radius );
        const int kind = static_cast< int >( kick.kind );
        const double phase = t - kick.phase;
        if ( phase < -KICK_LEAD[kind] ) continue; // not yet: an earlier one
        if ( phase <= KICK_TAIL[kind] )
        {
            kick.phase = static_cast< float >( phase );
            actions.kick = kick;
        }
        break;
    }

    // a run of a bit from its moment, the frame before its first one,
    // begun at the phase begin (negative: before the moment)
    auto runOf = [&]( const int bit,
                      const float begin,
                      const float tail,
                      FigureRun * out )
        {
            for ( const Run & run : log.runs( index, bit ) )
            {
                const double phase = t - ( static_cast< double >( run.first ) - 1.0 );
                const float last = static_cast< float >( run.last - run.first );
                if ( phase < begin ) continue;
                if ( phase <= last + tail )
                {
                    out->on = true;
                    out->phase = static_cast< float >( phase );
                    out->last = last;
                }
                break;
            }
        };
    runOf( rcss::rcg::TACKLE | rcss::rcg::TACKLE_FAULT, -TACKLE_LEAD, TACKLE_TAIL, &actions.tackle );
    // the fouled one falls after the tackler slid in: the bit comes with the tackle's
    runOf( rcss::rcg::FOUL_CHARGED, FOULED_START, FOULED_TAIL, &actions.fouled );

    // the catch: the hands keep the ball up for catch_ban_cycle
    const float hold = static_cast< float >( sp.catch_ban_cycle_ > 0 ? sp.catch_ban_cycle_ : 5 );
    for ( const Run & run : log.runs( index, rcss::rcg::CATCH | rcss::rcg::CATCH_FAULT ) )
    {
        const double phase = t - ( static_cast< double >( run.first ) - 1.0 );
        if ( phase < -CATCH_LEAD ) continue;
        if ( phase <= hold + CATCH_TAIL )
        {
            actions.caught.on = true;
            actions.caught.phase = static_cast< float >( phase );
            actions.caught.hold = hold;
            // where the ball came from: the frame before
            const std::size_t before = ( run.first > 0 ? run.first - 1 : 0 );
            float ahead, right;
            log.ballFromPlayer( before, &ahead, &right );
            actions.caught.side = right;
            // (in the air, virtually, with the option ball_flight_3d)
            actions.caught.height = ( flight && flight->active()
                                      ? flight->heightAtFrame( before )
                                      : log.show( before ).ball_.z_ );
            actions.caught.way = ( std::hypot( ahead, right ) > 1.0e-4f
                                   ? std::atan2( right, ahead ) * 180.0f / float( M_PI )
                                   : 0.0f );
        }
        break;
    }
    return actions;
}
