// -*-c++-*-

/*!
  \file player_camera_3d.h
  \brief the selected player's camera of the 3D view. Header File.
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


#ifndef RCSSMONITOR_PLAYER_CAMERA_3D_H
#define RCSSMONITOR_PLAYER_CAMERA_3D_H

#include "camera_3d.h"
#include "player_camera_options.h"

namespace rcss {
namespace rcg {
struct PlayerT;
}
}

/*!
  \brief the selected player's camera of the 3D view (rcssmonitor-web
  player-camera.ts, docs/PLAN_3D-VIEW.md §4.3): where it stands, which
  direction it follows, how it turns and how wide it sees, by the styles of
  PlayerCameraOptions. Only arithmetic: no OpenGL, so that the tests check it.

  A player turns its neck up to 180 degrees a cycle to look around, so a
  camera that follows the face at once may swing 1800 degrees a second;
  Smooth turns toward the direction with a time constant and a largest
  angular speed.
*/
namespace PlayerCamera3D {

constexpr float EYE_HEIGHT = 1.65f; //!< at the eyes (m)
constexpr float EYE_AHEAD = 10.0f; //!< at the eyes, it looks at a point this far ahead (m)
constexpr float EYE_TARGET_HEIGHT = 1.0f; //!< and this high
constexpr float SHOULDER_BEHIND = 6.0f; //!< over the shoulder: this far behind the player (m)
constexpr float SHOULDER_HEIGHT = 3.0f; //!< and this high
constexpr float SHOULDER_AHEAD = 15.0f; //!< looking at a point this far ahead
constexpr float SHOULDER_TARGET_HEIGHT = 1.0f; //!< and this high
constexpr float SETTLED_DEGREES = 0.05f; //!< the direction and the field of view count as reached within this (degree)

//! a direction (degree) within -180 to 180
float normalizeAngle( const float degree );

//! the direction the camera follows (degree): the face, the body, or between
float referenceDirection( const rcss::rcg::PlayerT & player,
                          const PlayerCameraStyle::Direction direction );

/*!
  \brief the view width as a horizontal field of view (degree): 60, 120 or
  180 (90 if unknown), within 40 and 120 (wider would distort too much)
*/
float viewFov( const rcss::rcg::PlayerT & player );

/*!
  \brief one step from a direction toward another (degree), the short way
  round: with the time constant (0: at once), but never faster than the
  largest turn
  \param dt the time step (s)
  \param time_constant (s)
  \param max_turn (degree / s)
*/
float turnToward( const float from,
                  const float to,
                  const double dt,
                  const double time_constant,
                  const double max_turn );

//! one step from a value toward another with the time constant (s; 0: at once)
float easeToward( const float from,
                  const float to,
                  const double dt,
                  const double time_constant );

/*!
  \brief the camera at a player's place, looking in a direction
  \param x, y the player on the field
  \param position at the eyes or over the shoulder
  \param direction the field direction it looks in (degree)
  \param fov the horizontal field of view (degree)
  \param aspect the width of the view divided by its height
  \param scale at the eyes, how tall the player is drawn, of a player of the
  default height: the eyes and the point it looks at are as much higher
  (over the shoulder, the same view for any player)
*/
CameraPose pose( const float x,
                 const float y,
                 const PlayerCameraStyle::Position position,
                 const float direction,
                 const float fov,
                 const float aspect,
                 const float scale = 1.0f );

}

/*!
  \class PlayerCameraState
  \brief the player camera's direction and field of view, kept from frame to
  frame so that Smooth and Eased can follow. It starts again at once (no
  smoothing) after reset(), for another player, or when the player jumped
  farther than it can move in a cycle.
*/
class PlayerCameraState {
public:

    //! start again at once at the next update()
    void reset() { M_started = false; }

    /*!
      \brief the camera for this frame
      \param player the selected player (interpolated)
      \param aspect the width of the view divided by its height
      \param options the styles
      \param dt the time since the last frame (s)
      \param settled set to whether it no longer moves by itself
      \param scale how tall the player is drawn (pose())
    */
    CameraPose update( const rcss::rcg::PlayerT & player,
                       const float aspect,
                       const PlayerCameraOptions & options,
                       const double dt,
                       bool * settled,
                       const float scale = 1.0f );

    //! the direction now (degree)
    float direction() const { return M_direction; }
    //! the horizontal field of view now (degree)
    float fov() const { return M_fov; }

private:
    bool M_started = false;
    float M_direction = 0.0f;
    float M_fov = 90.0f;
    char M_side = 0;
    int M_unum = 0;
    float M_x = 0.0f;
    float M_y = 0.0f;
};

#endif
