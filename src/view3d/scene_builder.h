// -*-c++-*-

/*!
  \file scene_builder.h
  \brief what the 3D view draws and where. Header File.
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

#ifndef RCSSMONITOR_SCENE_BUILDER_H
#define RCSSMONITOR_SCENE_BUILDER_H

#include <QMatrix4x4>
#include <QVector4D>

#include <vector>

class DispHolder;
class Options;

namespace rcss {
namespace rcg {
struct BallT;
struct ShowInfoT;
}
}

/*!
  \struct Instance3D
  \brief one instance of a shape: its model matrix, and its color with the
  opacity in w
*/
struct Instance3D {
    QMatrix4x4 model;
    QVector4D color;
};

/*!
  \struct SceneContent
  \brief what the 3D view draws in a frame, by the shape (Scene3D draws each
  list with one instanced draw call). The shapes have unit sizes, and the
  model matrices place and scale them. It knows nothing of OpenGL, so that
  the tests can check it (docs/PLAN_3D-VIEW.md §5.1).
*/
struct SceneContent {
    // lit by the sun
    std::vector< Instance3D > cylinders; //!< radius 1, height 1 along y, centered: the bodies, the goal posts and crossbars
    std::vector< Instance3D > spheres; //!< radius 1: the heads and the ball
    std::vector< Instance3D > cones; //!< base of radius 1 at the origin, tip at x = 1: the body and face directions

    // flat and translucent, in this order (the layers on the ground, low to high)
    std::vector< Instance3D > discs; //!< radius 1 on the xz plane: the circles under the players, the ball's shadow
    std::vector< Instance3D > rings; //!< 0.92 to 1.08 on the xz plane: the outline of a highlighted player
    std::vector< Instance3D > decay_rings; //!< 1.1 to 1.22 on the xz plane: the decay of effort or recovery
    std::vector< Instance3D > quads; //!< 1 x 1 on the xy plane, centered, both sides: the nets and the floors of the goals

    void clear();
};

/*!
  \brief the placement of the players, the ball and the goals of the 3D view
  (rcssmonitor-web render3d.ts). The world has x along the field's x, z along
  the field's y, and y up.
*/
namespace SceneBuilder {

constexpr float BODY_HEIGHT = 1.4f; //!< the cylinder of a player
constexpr float HEAD_RADIUS = 0.22f;
constexpr float HEAD_HEIGHT = BODY_HEIGHT + HEAD_RADIUS + 0.03f; //!< the center of the head
constexpr float GOAL_HEIGHT = 2.44f;

// the heights of the layers on the ground (§4.3)
constexpr float DISC_Y = 0.02f;
constexpr float BALL_SHADOW_Y = 0.03f;
constexpr float RING_Y = 0.03f;

constexpr float DISC_OPACITY = 0.45f;
constexpr float NET_OPACITY = 0.3f;
constexpr float GOAL_FLOOR_OPACITY = 0.35f;

//! the rotation about y (degrees) that turns +x to a direction of the field (degrees)
inline float yaw( const float direction ) { return -direction; }

//! the ball's radius: the option, or the real size below 0.01 (as in 2D)
double ballRadius( const Options & options,
                   const double ball_size );

//! the opacity of the ball's shadow, fainter as the ball goes up
double ballShadowOpacity( const rcss::rcg::BallT & ball );

//! the goals, which do not move
void addGoals( SceneContent & content );

/*!
  \brief the players and the ball of a show
  \param content where to add them
  \param show the show (interpolated)
  \param holder the server parameters and the player types
  \param options what to draw, the sizes and the colors
*/
void addShow( SceneContent & content,
              const rcss::rcg::ShowInfoT & show,
              const DispHolder & holder,
              const Options & options );

}

#endif
