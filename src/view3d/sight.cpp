// -*-c++-*-

/*!
  \file sight.cpp
  \brief what the selected player perceives, for the player camera. Source File.
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

#include "sight.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr double RAD2DEG = 180.0 / M_PI;

//! a direction (degree) within -180 to 180
double
normalize( const double degree )
{
    double d = std::fmod( degree + 180.0, 360.0 );
    if ( d < 0.0 ) d += 360.0;
    return d - 180.0;
}

//! the distance from the observer, and whether a point is inside its view cone
void
relative( const rcss::rcg::PlayerT & observer,
          const double x,
          const double y,
          const rcss::rcg::ServerParamT & param,
          double * dist,
          bool * in_view )
{
    const double dx = x - observer.x_;
    const double dy = y - observer.y_;
    const double face = observer.body_ + ( observer.hasNeck() ? observer.neck_ : 0.0 );
    const double dir = normalize( std::atan2( dy, dx ) * RAD2DEG - face );
    const double view = ( observer.hasView() ? std::fabs( observer.view_width_ ) : 0.0 );
    const double width = ( view > 0.0 ? view : param.visible_angle_ );
    *dist = std::hypot( dx, dy );
    *in_view = std::fabs( dir ) < width * 0.5;
}

}

namespace PlayerSight {

/*-------------------------------------------------------------------*/
double
omitProbability( const double dist,
                 const double far,
                 const double too_far )
{
    if ( too_far <= far )
    {
        return ( dist > far ? 1.0 : 0.0 );
    }
    return std::min( 1.0, std::max( 0.0, ( dist - far ) / ( too_far - far ) ) );
}

/*-------------------------------------------------------------------*/
Perception
perceivePlayer( const PlayerCameraStyle::Sight sight,
                const rcss::rcg::PlayerT & observer,
                const rcss::rcg::PlayerT & other,
                const rcss::rcg::PlayerTypeT & type,
                const rcss::rcg::ServerParamT & param )
{
    if ( sight == PlayerCameraStyle::Sight::All )
    {
        return FULL;
    }
    double dist = 0.0;
    bool in_view = false;
    relative( observer, other.x_, other.y_, param, &dist, &in_view );
    if ( sight == PlayerCameraStyle::Sight::View )
    {
        return ( in_view ? FULL : UNSEEN );
    }
    if ( in_view && dist < type.player_max_observation_length_ )
    {
        const float team = float( 1.0 - omitProbability( dist, type.team_far_length_, type.team_too_far_length_ ) );
        // without the team, the number is left out too
        const float unum = team * float( 1.0 - omitProbability( dist, type.unum_far_length_, type.unum_too_far_length_ ) );
        return Perception{ true, team, unum, unum > 0.0f };
    }
    return ( dist <= param.visible_distance_ ? FELT : UNSEEN );
}

/*-------------------------------------------------------------------*/
bool
perceiveBall( const PlayerCameraStyle::Sight sight,
              const rcss::rcg::PlayerT & observer,
              const rcss::rcg::BallT & ball,
              const rcss::rcg::PlayerTypeT & type,
              const rcss::rcg::ServerParamT & param )
{
    if ( sight == PlayerCameraStyle::Sight::All )
    {
        return true;
    }
    double dist = 0.0;
    bool in_view = false;
    relative( observer, ball.x_, ball.y_, param, &dist, &in_view );
    if ( sight == PlayerCameraStyle::Sight::View )
    {
        return in_view;
    }
    return ( in_view && dist < type.ball_max_observation_length_ )
        || dist <= param.visible_distance_;
}

}

/*-------------------------------------------------------------------*/
Perceptions
Perceptions::perceive( const PlayerCameraStyle::Sight sight,
                       const rcss::rcg::ShowInfoT & show,
                       const int observer,
                       const rcss::rcg::PlayerTypeT & type,
                       const rcss::rcg::ServerParamT & param,
                       const bool hide_observer )
{
    Perceptions p;
    p.hidden = ( hide_observer ? observer : -1 );
    if ( sight == PlayerCameraStyle::Sight::All
         || observer < 0 )
    {
        return p;
    }
    p.active = true;
    const rcss::rcg::PlayerT & me = show.player_[observer];
    for ( int i = 0; i < rcss::rcg::MAX_PLAYER * 2; ++i )
    {
        p.players[i] = ( i == observer
                         ? PlayerSight::FULL
                         : PlayerSight::perceivePlayer( sight, me, show.player_[i], type, param ) );
    }
    p.ball = PlayerSight::perceiveBall( sight, me, show.ball_, type, param );
    return p;
}

/*-------------------------------------------------------------------*/
QColor
fadeTeam( const QColor & color,
          const float team )
{
    const float k = std::min( 1.0f, std::max( 0.0f, team ) );
    const QColor gray( 0x9a, 0x9a, 0x9a );
    return QColor::fromRgbF( float( color.redF() * k + gray.redF() * ( 1.0f - k ) ),
                             float( color.greenF() * k + gray.greenF() * ( 1.0f - k ) ),
                             float( color.blueF() * k + gray.blueF() * ( 1.0f - k ) ),
                             color.alphaF() );
}
