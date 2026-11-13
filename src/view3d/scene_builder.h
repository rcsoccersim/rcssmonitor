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

#include <rcss/rcg/types.h>

#include <QColor>
#include <QString>

#include <vector>

class DispHolder;
class LabelAtlas;
class Options;

namespace rcss {
namespace rcg {
struct BallT;
struct PlayerT;
struct ServerParamT;
struct ShowInfoT;
}
}

/*!
  \struct OverlayText
  \brief a text of an overlay (the chance of a tackle, the largest
  acceleration of a kick), at a point on the ground, which the HUD writes
  beside it in the 2D player font, as the 2D view does
*/
struct OverlayText {
    QVector3D at; //!< the point in the world
    QString text;
    QColor color;
};

/*!
  \struct PlayerPick
  \brief a player as drawn, for a click to find it (Picking): the body is a
  cylinder of the radius from the ground to BODY_HEIGHT, the head a sphere
  of HEAD_RADIUS at HEAD_HEIGHT (SceneBuilder)
*/
struct PlayerPick {
    rcss::rcg::Side side;
    int unum;
    float x; //!< on the field (m)
    float y;
    float radius; //!< of the body (m)
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

    // made again in every frame, translucent too (OverlayBuilder)
    std::vector< ColorVertex3D > sectors; //!< triangles: the view areas
    std::vector< ColorVertex3D > lines; //!< each two vertices, a line: the overlays on the ground
    std::vector< ColorVertex3D > dotted_lines; //!< each two vertices, a dotted line
    std::vector< ColorVertex3D > points; //!< dots

    // written by the HUD (QPainter), not by OpenGL
    std::vector< OverlayText > texts; //!< the texts of the overlays

    // over everything, facing the camera, from far to near
    std::vector< Billboard3D > labels; //!< the numbers and the cards (LabelAtlas)

    // not drawn
    std::vector< PlayerPick > picks; //!< the players drawn, which a right click can aim at


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

// the stamina bars under the number (rcssmonitor-web render3d.ts BAR), in meters at the label's size
constexpr float BAR_WIDTH = 1.4f;
constexpr float BAR_STAMINA = 0.2f; //!< the height of the stamina bar
constexpr float BAR_CAPACITY = 0.15f; //!< the height of the capacity bar, under it (thicker than the web version's 0.1, to be seen)
constexpr float BAR_GAP = 0.08f; //!< between the number's text and the bars, and between the bars
constexpr float BAR_FRAME = 0.03f; //!< the dark background around the fill
constexpr float BAR_LIFT = 0.45f; //!< the number goes up as much with the bars, so that they are above the head from the side
constexpr float BAR_BACK_OPACITY = 0.6f; //!< of the background
const QColor CAPACITY_COLOR( 0x4a, 0xa3, 0xff ); //!< the fill of the capacity bar

//! the stamina left, of stamina_max (0 to 1): the length of the stamina bar
double staminaRate( const rcss::rcg::PlayerT & player,
                    const rcss::rcg::ServerParamT & param );

//! the stamina capacity left, of stamina_capacity (0 to 1); -1 without it (old logs)
double capacityRate( const rcss::rcg::PlayerT & player,
                     const rcss::rcg::ServerParamT & param );

/*!
  \brief the hue of the stamina bar (degree): red at or below the stamina
  where the recovery or the effort starts to fall (recover_dec_thr,
  effort_dec_thr of stamina_max), to green when full
*/
double staminaHue( const double rate,
                   const rcss::rcg::ServerParamT & param );

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
  The stamina and the stamina capacity, when the Show tab asks for them
  (S), are bars under the number along the camera's down (the number goes
  up by BAR_LIFT with them); from the top camera, from the left end of the
  number as the 2D view writes them.
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
