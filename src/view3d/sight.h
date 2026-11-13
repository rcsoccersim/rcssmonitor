// -*-c++-*-

/*!
  \file sight.h
  \brief what the selected player perceives, for the player camera. Header File.
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


#ifndef RCSSMONITOR_SIGHT_H
#define RCSSMONITOR_SIGHT_H

#include "player_camera_options.h"

#include <rcss/rcg/types.h>

#include <QColor>

#include <array>

/*!
  \struct Perception
  \brief how the player camera's player perceives another player
*/
struct Perception {
    bool seen; //!< drawn at all
    float team; //!< how likely the team is known (0 to 1): the team colors fade to gray
    float unum; //!< how likely the number is known (0 to 1): the number's opacity
    bool facing; //!< the body and face directions are known

    //! the team is known at all (the highlights, the cards)
    bool known() const { return team > 0.0f; }
};

/*!
  \brief what the selected player perceives, for the player camera of the
  3D view (rcssmonitor-web sight.ts, docs/PLAN_3D-VIEW.md §4.3):

  - View: what is inside the view cone (the view width around the face
    direction, body + neck).
  - Server: rcssserver's visual sensor (visualsenderplayer.cpp,
    VisualSenderPlayerV1::sendHighPlayer / sendHighBall, with the player
    type's values):
    - Players and the ball in the cone are seen up to
      player_max_observation_length / ball_max_observation_length.
    - The server leaves out the team with the probability
      (dist - team_far_length) / (team_too_far_length - team_far_length),
      and otherwise the number with the probability of unum_far_length and
      unum_too_far_length. Without the number, it does not send the body and
      face directions either. Here the probabilities are not drawn at
      random: they are how likely the team and the number are known, which
      the view shows as the team colors fading to gray and as the number's
      opacity.
    - Outside the cone, objects within visible_distance are felt: only that
      something is there.

  Not as the server: it decides with the noisy distance (this uses the true
  one), visual information comes once in a few cycles by the view width
  (this decides every cycle), and a low view quality is not handled.
  Only arithmetic: no OpenGL, so that the tests check it.
*/
namespace PlayerSight {

//! drawn as it is
constexpr Perception FULL = { true, 1.0f, 1.0f, true };
//! not drawn
constexpr Perception UNSEEN = { false, 0.0f, 0.0f, false };
//! felt: outside the view but near, only that a player is there
constexpr Perception FELT = { true, 0.0f, 0.0f, false };

//! the probability that the server leaves the team or the number out at a distance
double omitProbability( const double dist,
                        const double far,
                        const double too_far );

/*!
  \brief how the observer (of its player type) perceives another player
*/
Perception perceivePlayer( const PlayerCameraStyle::Sight sight,
                           const rcss::rcg::PlayerT & observer,
                           const rcss::rcg::PlayerT & other,
                           const rcss::rcg::PlayerTypeT & type,
                           const rcss::rcg::ServerParamT & param );

//! whether the observer perceives the ball
bool perceiveBall( const PlayerCameraStyle::Sight sight,
                   const rcss::rcg::PlayerT & observer,
                   const rcss::rcg::BallT & ball,
                   const rcss::rcg::PlayerTypeT & type,
                   const rcss::rcg::ServerParamT & param );

}

/*!
  \struct Perceptions
  \brief what the player camera draws of a show: how its player perceives
  each player and the ball, and a player not drawn at all (its own, at the
  eyes, which would hide the view). The builders draw everything without it.
*/
struct Perceptions {
    bool active = false; //!< by the player's sight (not everything): players[] and ball are set
    std::array< Perception, rcss::rcg::MAX_PLAYER * 2 > players; //!< by the index in the show
    bool ball = true; //!< the ball is drawn
    int hidden = -1; //!< the index of a player not drawn, or -1

    //! how a player of the show is drawn
    Perception of( const int index ) const
      {
          if ( index == hidden ) return PlayerSight::UNSEEN;
          return ( active ? players[index] : PlayerSight::FULL );
      }

    /*!
      \brief what the player camera's player perceives of a show
      \param sight the style
      \param show the show of the cycle (the decision is not interpolated)
      \param observer the index of the player in the show
      \param type its player type
      \param param the server parameters
      \param hide_observer the camera is at its eyes: do not draw it
    */
    static Perceptions perceive( const PlayerCameraStyle::Sight sight,
                                 const rcss::rcg::ShowInfoT & show,
                                 const int observer,
                                 const rcss::rcg::PlayerTypeT & type,
                                 const rcss::rcg::ServerParamT & param,
                                 const bool hide_observer );
};

/*!
  \brief a color faded toward the gray of an unknown team (#9a9a9a, as the
  web version): as it is when the team is known (1), the gray when not (0)
*/
QColor fadeTeam( const QColor & color,
                 const float team );

#endif
