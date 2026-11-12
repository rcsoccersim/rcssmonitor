// -*-c++-*-

/*!
  \file cameras_3d.h
  \brief the cameras of the 3D view. Header File.
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

#ifndef RCSSMONITOR_CAMERAS_3D_H
#define RCSSMONITOR_CAMERAS_3D_H

#include "camera_3d.h"
#include "camera_mode.h"

#include <QVector3D>

#include <cmath>

/*!
  \brief where the cameras of the 3D view are (rcssmonitor-web cameras.ts,
  docs/PLAN_3D-VIEW.md §4.3), and how the mouse moves the free camera (as
  three.js's OrbitControls, which the web version uses). Only arithmetic: no
  OpenGL, so that the tests check it.

  The world has x along the field's x, z along the field's y, and y up, in
  meters. A camera that follows the ball follows where it is on the ground,
  so that a ball in the air does not shake the picture.
*/
namespace Cameras3D {

constexpr float FOV = 35.0f; //!< the vertical field of view (degree) of the cameras, except behind the goals
constexpr float ELEVATION = 32.0f; //!< how high (degree) the broadcast camera and the first free camera look down
constexpr float BROADCAST_ZOOM = 0.55f; //!< the broadcast camera is this much of the distance of the whole pitch
constexpr float GOAL_FOV = 45.0f; //!< the field of view behind the goals
constexpr float TIME_CONSTANT = 0.35f; //!< how slowly (s) the following cameras catch up (smoothToward())
constexpr double MAX_STEP = 0.1; //!< at most this time (s) passes in one frame, so that a long pause does not jump
constexpr float SETTLED = 0.02f; //!< a following camera this near (m) to where it goes has arrived

// the limits of the free camera
constexpr float MIN_DISTANCE = 3.0f; //!< from what it turns around (m)
constexpr float MAX_DISTANCE = 400.0f;
constexpr float MIN_POLAR = 0.01f; //!< from straight above (radian): not quite, where the camera's up would be along its view
constexpr float MAX_POLAR = float( M_PI / 2.0 ) - 0.03f; //!< from straight above (radian): stays above the ground
constexpr float WHEEL_ZOOM = 0.95f; //!< the distance by a notch of the wheel toward

/*!
  \brief the camera that sees the whole pitch and 2 m around it, as close as
  it can, from azimuth and elevation, looking at the center. Horizontally
  within 97% of the view, vertically within 85% (15% left for the score
  board and the logos).
  \param azimuth the field direction (degree) from the center to the camera:
  90 is the near touch line (+y, below in 2D), 0 behind the right goal
  \param elevation how high (degree) above the ground
  \param aspect the width of the view divided by its height
*/
CameraPose fitView( const float azimuth,
                    const float elevation,
                    const float aspect );

/*!
  \brief the broadcast camera: from beyond the near touch line, ELEVATION
  degrees up, looking at the ball on the ground, which it follows; not so
  near the touch lines and the goal lines (10 m) that the pitch leaves the view
*/
CameraPose broadcast( const float aspect,
                      const float ball_x,
                      const float ball_y );

/*!
  \brief behind a goal, 18 m behind the goal line and 10 m high, looking
  halfway between the goal and the ball, so that both stay in view
  \param side -1 for the left goal, +1 for the right
*/
CameraPose goal( const int side,
                 const float ball_x,
                 const float ball_y );

/*!
  \brief straight above, without perspective, the whole ground image as the
  2D view fits it (the field's -y up on the screen), with 15% of the height
  left for the score board and the logos
*/
CameraPose top( const float aspect );

/*!
  \brief move from toward to by the fraction of the time step: after the
  time constant tau, 63% of the way (1 - e^-1), whatever the frame rate
  \param dt the time step (s)
*/
QVector3D smoothToward( const QVector3D & from,
                        const QVector3D & to,
                        const double dt,
                        const float tau = TIME_CONSTANT );

/*!
  \brief turn the free camera around its target by a drag of the mouse: a
  drag across the height of the view turns it once round. It stays above
  the ground (MAX_POLAR), and not quite straight above (MIN_POLAR).
  \param dx, dy the drag (pixels: right and down are positive)
  \param height the height of the view (pixels)
*/
void orbit( CameraPose & pose,
            const float dx,
            const float dy,
            const float height );

/*!
  \brief move the free camera and its target in the plane of the screen, so
  that the ground at the target follows the mouse
*/
void pan( CameraPose & pose,
          const float dx,
          const float dy,
          const float height );

/*!
  \brief move the free camera toward its target (scale < 1) or away (> 1),
  within MIN_DISTANCE and MAX_DISTANCE
*/
void dolly( CameraPose & pose,
            const float scale );

}

/*!
  \class CameraController
  \brief the camera of the 3D view: which one, where it is now, and how it
  moves from frame to frame. FieldView3D calls update() in every frame, and
  the mouse functions on input.

  The following cameras (broadcast, behind the goals) glide after the ball;
  when the mode changes, the camera jumps to the new place at once. The free
  camera stays where the mouse put it. Moving the mouse on a following
  camera makes it the free camera, from where it was. The top camera does
  not take the mouse.
*/
class CameraController {
public:

    CameraController();

    CameraMode mode() const { return M_mode; }

    //! change the camera. The free camera starts where the camera was, or with the whole pitch after the top camera
    void setMode( const CameraMode mode );

    /*!
      \brief move the camera for a frame.
      \param dt the time since the last frame (s; MAX_STEP at most is used)
      \param aspect the width of the view divided by its height
      \param ball_x, ball_y the ball on the ground
      \return where the camera is
    */
    CameraPose update( const double dt,
                       const float aspect,
                       const float ball_x,
                       const float ball_y );

    //! the camera does not move unless the ball does (it has arrived)
    bool settled() const { return M_settled; }

    //! the mouse moves the camera (all but the top camera)
    bool takesMouse() const { return M_mode != CameraMode::Top; }

    /*!
      \brief turn, move or zoom by the mouse (Cameras3D::orbit(), pan(), dolly()).
      \return true if the mode changed to the free camera
    */
    bool orbit( const float dx, const float dy, const float height );
    bool pan( const float dx, const float dy, const float height );
    bool dolly( const float scale );

private:
    CameraMode M_mode;
    CameraPose M_pose; //!< where the camera is now
    bool M_fitted; //!< the free camera has been placed for the view (at the first update)
    bool M_snap; //!< jump to the next place instead of gliding
    bool M_settled;
    float M_aspect; //!< of the last update

    //! before a move by the mouse: false if the camera does not take it
    bool takeMouse( bool * changed );
};

#endif
