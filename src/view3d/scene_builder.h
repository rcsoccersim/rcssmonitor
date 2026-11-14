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
#include "scene_content.h"
#include "sight.h"

#include <QColor>

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
  \brief the placement of the players, the ball and the goals of the 3D view
  (rcssmonitor-web render3d.ts). The world has x along the field's x, z along
  the field's y, and y up.
*/
namespace SceneBuilder {

constexpr float BODY_HEIGHT = 1.4f; //!< the cylinder of a player
constexpr float HEAD_RADIUS = 0.22f;
constexpr float HEAD_GAP = 0.03f; //!< between the body and the head
constexpr float HEAD_HEIGHT = BODY_HEIGHT + HEAD_RADIUS + HEAD_GAP; //!< the center of the head (of a player of the default height)
constexpr float GOAL_HEIGHT = 2.44f;

// the heights of the layers on the ground (§4.3)
constexpr float DISC_Y = 0.02f;
constexpr float BALL_SHADOW_Y = 0.03f;
constexpr float RING_Y = 0.03f;

// the labels (rcssmonitor-web render3d.ts)
constexpr float LABEL_BASE = HEAD_HEIGHT + HEAD_RADIUS; //!< the top of the head of a cylinder, where a label shrinks toward
constexpr float LABEL_ABOVE = 0.53f; //!< a number label's center above the top of the head (m)
constexpr float NUMBER_HEIGHT = LABEL_BASE + LABEL_ABOVE; //!< the center of a number label of a cylinder
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

// the human figures (Figures3D)
constexpr float PERSON_HEIGHT = 1.85f; //!< a person's height (m)

/*!
  \brief how tall a player is, of a player of the default type (1): the
  players of a longer reach are taller. The reach is the kickable area,
  player_size + kickable_margin, which the heterogeneous player types
  change (0.9 to 1.1 m by the default parameters: 0.9 to 1.1 times).
  When the simulator gives each player's height (a later extension, with
  the ball in the air), it comes in here.
  \param type the player's type
  \param param the server parameters: the default type
*/
float heightRatio( const rcss::rcg::PlayerTypeT & type,
                   const rcss::rcg::ServerParamT & param );

/*!
  \brief the height of a player's human figure (m): a person's height,
  times the player's height ratio, times the option figure_scale_3d (to be
  seen from afar)
  \param ratio heightRatio(), or 1 for a player whose height is not shown
  \param options the scale
*/
float figureHeight( const float ratio,
                    const Options & options );

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

/*!
  \brief the goals, which do not move, with the nets in the colors of the
  teams that keep them (rcssserver does not change ends: the left team
  keeps the left goal for the whole game), to tell the ends apart
*/
void addGoals( SceneContent & content,
               const QColor & left_net = QColor( Qt::white ),
               const QColor & right_net = QColor( Qt::white ) );

/*!
  \brief the players and the ball of a show
  \param content where to add them
  \param show the show (interpolated)
  \param holder the server parameters and the player types
  \param options what to draw, the sizes and the colors
  \param perceptions what the player camera draws (null: everything): the
  players its player does not see are not drawn, nor the ball; a player
  whose team it may not know fades to gray, without the highlights; one
  whose directions it does not know has no direction marks
  \param figures the players as human figures (SceneContent::figures, posed
  by Figures3D) in place of a cylinder and a sphere, without the direction
  marks, which the figure shows itself. A player whose directions the
  player camera does not know stays a cylinder: a figure would show them.

  A player is as tall as its height ratio (heightRatio()): the body of the
  cylinder, with the head on it, or the figure. A player whose number the
  player camera does not know for sure (the opacity of its number under 1)
  is of the default height, which would tell which player it is.
*/
void addShow( SceneContent & content,
              const rcss::rcg::ShowInfoT & show,
              const DispHolder & holder,
              const Options & options,
              const Perceptions * perceptions = nullptr,
              const bool figures = false );

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
  font. From the other cameras, it is LABEL_ABOVE above the top of the head
  (of the player as addShow() put it in content.picks, or of a player of
  the default height), in meters, but at most LABEL_MAX_FONT times the font
  high on the screen: a nearer label shrinks toward the top of the head. A yellow or red card is to the right of the number.
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
  \param perceptions what the player camera draws (null: everything): the
  number as likely as its player knows it (its opacity), the cards of the
  players whose team it may know, and the bars of its own only (the server
  does not tell the others' stamina)
*/
void addLabels( SceneContent & content,
                const rcss::rcg::ShowInfoT & show,
                const DispHolder & holder,
                const Options & options,
                LabelAtlas & atlas,
                const LabelView & view,
                const Perceptions * perceptions = nullptr );

}

#endif
