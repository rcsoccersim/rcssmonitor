// -*-c++-*-

/*!
  \file overlay_math.cpp
  \brief calculations of the overlays, without drawing. Source File.
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

#include "overlay_math.h"

#include "options.h"

#include "angle_deg.h"
#include "circle_2d.h"

#include <algorithm>
#include <cmath>

namespace OverlayMath {

/*-------------------------------------------------------------------*/
/*!

 */
OffsideLines
offsideLines( const rcss::rcg::ShowInfoT & show )
{
    const float ball_x = show.ball_.x_;

    // PlayerT::side_ is a character ('l', 'r'); compare side() with the
    // SideID values. Comparing side_ with them matched no player, and the
    // lines stayed between x = 0 and the ball.

    float offside_l = 0.0f;
    {
        float min_x = 0.0f;
        for ( int i = 0; i < rcss::rcg::MAX_PLAYER*2; ++i )
        {
            if ( show.player_[i].state_ != 0
                 && show.player_[i].side() == rcss::rcg::LEFT )
            {
                float x = show.player_[i].x_;
                if ( x < offside_l )
                {
                    if ( x < min_x )
                    {
                        offside_l = min_x;
                        min_x = x;
                    }
                    else
                    {
                        offside_l = x;
                    }
                }
            }
        }
        offside_l = std::min( offside_l, ball_x );
        offside_l = std::max( offside_l, - float( Options::PITCH_HALF_LENGTH ) );
    }

    float offside_r = 0.0f;
    {
        float max_x = 0.0f;
        for ( int i = 0; i < rcss::rcg::MAX_PLAYER*2; ++i )
        {
            if ( show.player_[i].state_ != 0
                 && show.player_[i].side() == rcss::rcg::RIGHT )
            {
                float x = show.player_[i].x_;
                if ( offside_r < x )
                {
                    if ( max_x < x )
                    {
                        offside_r = max_x;
                        max_x = x;
                    }
                    else
                    {
                        offside_r = x;
                    }
                }
            }
        }
        offside_r = std::max( offside_r, ball_x );
        offside_r = std::min( offside_r, float( Options::PITCH_HALF_LENGTH ) );
    }

    return OffsideLines{ offside_l, offside_r };
}

/*-------------------------------------------------------------------*/
bool
focusPoint( const rcss::rcg::PlayerT & player,
            Vector2D * point )
{
    if ( player.focusDist() <= 1.0e-5 )
    {
        return false;
    }
    const AngleDeg focus_angle = player.body_ + player.neck_ + player.focus_dir_;
    *point = Vector2D( player.x(), player.y() ) + Vector2D::from_polar( player.focusDist(), focus_angle );
    return true;
}

/*-------------------------------------------------------------------*/
CatchArea
catchArea( const rcss::rcg::ServerParamT & SP,
           const rcss::rcg::PlayerTypeT & type )
{
    CatchArea area;
    area.catchable_ = std::sqrt( std::pow( SP.catchable_area_w_ * 0.5, 2.0 )
                                 + std::pow( SP.catchable_area_l_, 2.0 ) );

    const double max_catchable_area_l = SP.catchable_area_l_ * type.catchable_area_l_stretch_;
    area.max_ = std::sqrt( std::pow( SP.catchable_area_w_ * 0.5, 2.0 )
                           + std::pow( max_catchable_area_l, 2.0 ) );

    const double min_catchable_area_l = SP.catchable_area_l_ * ( 1.0 - ( type.catchable_area_l_stretch_ - 1.0 ) );
    area.min_ = std::sqrt( std::pow( SP.catchable_area_w_ * 0.5, 2.0 )
                           + std::pow( min_catchable_area_l, 2.0 ) );

    area.stretched_ = ( area.max_ > area.catchable_ );
    area.catchable_length_ = SP.catchable_area_l_;
    area.max_length_ = max_catchable_area_l;
    area.min_length_ = min_catchable_area_l;
    return area;
}

/*-------------------------------------------------------------------*/
CatchOutline
catchOutline( const rcss::rcg::ServerParamT & SP,
              const rcss::rcg::PlayerT & player,
              const double length )
{
    const Vector2D center( player.x(), player.y() );
    const double half_width = SP.catchable_area_w_ * 0.5;
    const double radius = std::sqrt( std::pow( half_width, 2.0 ) + std::pow( length, 2.0 ) );
    // the far corners are this much off the direction of the box
    const double diagonal = AngleDeg::atan2_deg( half_width, length );

    // the server keeps the catch direction within [min, max] (bound())
    const double min_dir = SP.min_catch_angle_;
    const double span = std::max( 0.0, SP.max_catch_angle_ - min_dir );
    const double body = player.body_;

    CatchOutline outline;
    outline.circle_ = ( span + diagonal * 2.0 >= 360.0 );
    if ( outline.circle_ )
    {
        outline.arcs_.push_back( Arc{ center, radius, 0.0, 360.0 } );
        return outline;
    }

    const AngleDeg max_angle = body + min_dir + span;
    const AngleDeg min_angle = body + min_dir;
    const Vector2D far_max = center + Vector2D::from_polar( radius, max_angle + diagonal );
    const Vector2D far_min = center + Vector2D::from_polar( radius, min_angle - diagonal );

    if ( span >= diagonal * 2.0 )
    {
        outline.arcs_.push_back( Arc{ center, radius, body + min_dir - diagonal, span + diagonal * 2.0 } );
    }
    else
    {
        // the corners on one side of the boxes, and those on the other; the
        // far sides of the boxes at the two ends cross between them, on the
        // middle direction
        outline.arcs_.push_back( Arc{ center, radius, body + min_dir - diagonal, span } );
        outline.arcs_.push_back( Arc{ center, radius, body + min_dir + diagonal, span } );
        outline.between_.push_back( center + Vector2D::from_polar( radius, max_angle - diagonal ) );
        outline.between_.push_back( center + Vector2D::from_polar( length / std::cos( span * 0.5 * M_PI / 180.0 ),
                                                                   min_angle + span * 0.5 ) );
        outline.between_.push_back( center + Vector2D::from_polar( radius, min_angle + diagonal ) );
    }

    outline.edges_.push_back( far_max );
    if ( span < 180.0 - 1.0e-6 )
    {
        // the boxes leave a gap behind the goalie: the sides of the two
        // boxes at the ends go back to their near corners, which meet at
        // the goalie
        outline.edges_.push_back( center + Vector2D::from_polar( half_width, max_angle + 90.0 ) );
        outline.edges_.push_back( center );
        outline.edges_.push_back( center + Vector2D::from_polar( half_width, min_angle - 90.0 ) );
    }
    else
    {
        // the sides of the two boxes at the ends cross behind the goalie
        // (on one line when the directions are 180 degrees apart)
        const Vector2D u_max = Vector2D::from_polar( 1.0, max_angle );
        const Vector2D u_min = Vector2D::from_polar( 1.0, min_angle );
        const double cross = u_max.outerProduct( u_min );
        if ( std::fabs( cross ) > 1.0e-6 )
        {
            // far_max + t * u_max == far_min + s * u_min
            const double t = ( far_min - far_max ).outerProduct( u_min ) / cross;
            outline.edges_.push_back( far_max + u_max * t );
        }
    }
    outline.edges_.push_back( far_min );
    return outline;
}

/*-------------------------------------------------------------------*/
TackleChances
tackleChances( const rcss::rcg::ServerParamT & SP,
               const rcss::rcg::PlayerT & player,
               const rcss::rcg::BallT & ball )
{
    TackleChances chances = { false, 0.0, false, 0.0 };

    // the ball relative to the player's body
    Vector2D player_to_ball = Vector2D( ball.x_, ball.y_ ) - Vector2D( player.x_, player.y_ );
    player_to_ball.rotate( - player.body_ );

    const double tackle_dist = ( player_to_ball.x > 0.0
                                 ? SP.tackle_dist_
                                 : SP.tackle_back_dist_ );
    if ( tackle_dist < 1.0e-5 )
    {
        return chances;
    }

    const double tackle_fail_prob = ( std::pow( player_to_ball.absX() / tackle_dist, SP.tackle_exponent_ )
                                      + std::pow( player_to_ball.absY() / SP.tackle_width_, SP.tackle_exponent_ ) );
    const double foul_fail_prob = ( std::pow( player_to_ball.absX() / tackle_dist, SP.foul_exponent_ )
                                    + std::pow( player_to_ball.absY() / SP.tackle_width_, SP.foul_exponent_ ) );
    chances.tackle_ = ( tackle_fail_prob < 1.0 );
    chances.tackle_chance_ = 1.0 - tackle_fail_prob;
    chances.foul_ = ( foul_fail_prob < 1.0 );
    chances.foul_chance_ = 1.0 - foul_fail_prob;
    return chances;
}

/*-------------------------------------------------------------------*/
QString
tackleText( const TackleChances & chances )
{
    if ( chances.tackle_ && chances.foul_ )
    {
        return QString( "T=%1,F=%2" ).arg( chances.tackle_chance_, 0, 'g', 3 ).arg( chances.foul_chance_, 0, 'g', 3 );
    }
    if ( chances.tackle_ )
    {
        return QString( "Tackle=%1" ).arg( chances.tackle_chance_, 0, 'g', 3 );
    }
    if ( chances.foul_ )
    {
        return QString( "Foul=%1" ).arg( chances.foul_chance_, 0, 'g', 3 );
    }
    return QString();
}

namespace {

/*!
  \brief the arc from one of two directions to the other that passes
  toward: it starts at the one to the left of toward
*/
Arc
arc_through( const Vector2D & center,
             const double radius,
             const AngleDeg & a1,
             const AngleDeg & a2,
             const AngleDeg & toward )
{
    const bool first = a1.isLeftOf( toward );
    const AngleDeg & start = ( first ? a1 : a2 );
    const AngleDeg & end = ( first ? a2 : a1 );
    double span = ( end - start ).degree();
    if ( span < 0.0 )
    {
        span += 360.0;
    }
    return Arc{ center, radius, start.degree(), span };
}

}

/*-------------------------------------------------------------------*/
KickAccel
kickAccel( const rcss::rcg::ServerParamT & SP,
           const rcss::rcg::PlayerTypeT & type,
           const rcss::rcg::PlayerT & player,
           const rcss::rcg::BallT & ball )
{
    KickAccel kick;
    kick.kickable_ = false;
    kick.max_accel_ = 0.0;
    if ( ! ball.hasVelocity() )
    {
        return kick;
    }

    const Vector2D bpos( ball.x_, ball.y_ );
    Vector2D player_to_ball = bpos - Vector2D( player.x_, player.y_ );
    player_to_ball.rotate( - player.body_ );

    const double ball_dist = player_to_ball.r();
    if ( ball_dist > type.player_size_ + type.kickable_margin_ + SP.ball_size_ )
    {
        return kick;
    }
    kick.kickable_ = true;

    kick.max_accel_ = std::min( SP.max_power_
                                * type.kick_power_rate_
                                * ( 1.0 - 0.25 * player_to_ball.th().abs() / 180.0
                                    - 0.25 * ( ball_dist - type.player_size_ - SP.ball_size_ ) / type.kickable_margin_ ),
                                SP.ball_accel_max_ );
    kick.next_ = Vector2D( bpos.x + ball.vx_, bpos.y + ball.vy_ );

    const Circle2D max_speed_circle( bpos, SP.ball_speed_max_ );
    const Circle2D max_accel_circle( kick.next_, kick.max_accel_ );
    Vector2D i1, i2;
    if ( max_speed_circle.intersection( max_accel_circle, &i1, &i2 ) != 2 )
    {
        // the circles do not cross: the reach of the largest acceleration
        kick.arcs_.push_back( Arc{ kick.next_, kick.max_accel_, 0.0, 360.0 } );
        return kick;
    }

    // the arc of the fastest ball, on the side of the next place, and the
    // arc of the largest acceleration, on the side of the ball
    const AngleDeg bpos_to_bnext = ( kick.next_ - bpos ).th();
    kick.arcs_.push_back( arc_through( bpos, SP.ball_speed_max_,
                                       ( i1 - bpos ).th(), ( i2 - bpos ).th(), bpos_to_bnext ) );
    kick.arcs_.push_back( arc_through( kick.next_, kick.max_accel_,
                                       ( i1 - kick.next_ ).th(), ( i2 - kick.next_ ).th(), bpos_to_bnext + 180.0 ) );
    return kick;
}

/*-------------------------------------------------------------------*/
std::vector< Vector2D >
ballPath( const rcss::rcg::ServerParamT & SP,
          const rcss::rcg::BallT & ball,
          const int cycles,
          const double min_step )
{
    std::vector< Vector2D > path;
    if ( ! ball.hasVelocity() )
    {
        return path;
    }
    Vector2D pos( ball.x_, ball.y_ );
    Vector2D vel( ball.vx_, ball.vy_ );
    for ( int i = 0; i < std::min( 100, cycles ); ++i )
    {
        if ( vel.r() < min_step )
        {
            break;
        }
        pos += vel;
        vel *= SP.ball_decay_;
        path.push_back( pos );
    }
    return path;
}

}
