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

#include "camera_3d.h"
#include "instance_3d.h"

#include <vector>

class DispHolder;
class LabelAtlas;
class Options;

namespace rcss {
namespace rcg {
struct BallT;
struct ShowInfoT;
}
}

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

    // over everything, facing the camera, from far to near
    std::vector< Billboard3D > labels; //!< the numbers and the cards (LabelAtlas)

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

// the labels (rcssmonitor-web render3d.ts)
constexpr float NUMBER_HEIGHT = HEAD_HEIGHT + 0.75f; //!< the center of a number label
constexpr float LABEL_BASE = HEAD_HEIGHT + HEAD_RADIUS; //!< the top of the head, where a label shrinks toward
constexpr float LABEL_GAP = 0.25f; //!< from the top camera, between the circle on the ground and the number (m)
constexpr float LABEL_MAX_FONT = 2.0f; //!< a number's text is at most this many times the 2D player font high on the screen
constexpr float CARD_WIDTH = 0.42f; //!< a card (m), to the right of the number
constexpr float CARD_HEIGHT = 0.6f;
constexpr float CARD_GAP = 0.08f; //!< between the number's text and the card (m)

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

/*!
  \struct LabelView
  \brief what the size of the labels depends on: the camera, and the screen
*/
struct LabelView {
    CameraPose camera;
    float height; //!< of the view (logical pixels)
    double font_pixels; //!< the height of the 2D player font (the Font tab) on the screen (pixels)
};

/*!
  \brief the number labels and the cards of a show (rcssmonitor-web
  fitLabels(), labelPlace()). A label has the number and the player type,
  as the 2D view writes them (Show tab), white, or in the color of illegal
  defense. From the top camera, it is where the 2D view writes it, to the
  right of the circle on the ground, as high on the screen as the 2D player
  font. From the other cameras, it is above the head, in meters, but at most
  LABEL_MAX_FONT times the font high on the screen: a nearer label shrinks
  toward the head. A yellow or red card is to the right of the number.
  \param content where to add them
  \param show the show (interpolated)
  \param holder the server parameters and the player types
  \param options what to draw, the sizes and the colors
  \param atlas the texts of the labels, drawn again when they change
  \param view the camera and the screen
*/
void addLabels( SceneContent & content,
                const rcss::rcg::ShowInfoT & show,
                const DispHolder & holder,
                const Options & options,
                LabelAtlas & atlas,
                const LabelView & view );

}

#endif
