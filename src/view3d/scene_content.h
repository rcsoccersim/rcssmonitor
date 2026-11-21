// -*-c++-*-

/*!
  \file scene_content.h
  \brief what the 3D view draws in a frame. Header File.
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

#ifndef RCSSMONITOR_SCENE_CONTENT_H
#define RCSSMONITOR_SCENE_CONTENT_H

#include "figure_looks.h"
#include "instance_3d.h"

#include <rcss/rcg/types.h>

#include <QColor>
#include <QString>
#include <QVector3D>
#include <QVector4D>

#include <array>
#include <vector>

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
  cylinder of the radius from the ground to body_height, the head a sphere
  of HEAD_RADIUS at head_height (SceneBuilder)
*/
struct PlayerPick {
    rcss::rcg::Side side;
    int unum;
    float x; //!< on the field (m)
    float y;
    float radius; //!< of the body (m)
    float body_height; //!< the top of the body (m)
    float head_height; //!< the center of the head (m)
    bool figure = false; //!< drawn as a human figure (its radius is FIGURE_PICK_SCALE times as wide as it)
};

/*!
  \struct FigureMotion
  \brief how a figure moves at the time drawn, from the log (GaitTable):
  so that Figures3D poses it from the frame and the fraction to the next
  one only
*/
struct FigureMotion {
    double gait = 0.0; //!< the gait's cycles walked by the time drawn, for a figure of a person's height (fewer when going backward)
    float vx = 0.0f; //!< the velocity on the field (m/s), smoothed over the cycles around
    float vy = 0.0f;
    float ax = 0.0f; //!< the acceleration on the field (m/s^2), smoothed over the cycles around
    float ay = 0.0f;
    double steps = 0.0; //!< the steps taken on the spot by the time drawn, as the legs turn (GaitTable::TURN_STEP_ANGLE a step, at most TURN_STEPS_MAX a cycle)
    float lag = 0.0f; //!< how far the legs lag behind the body direction (degree, as the field's angles: turning right, positive)
    float turn_rate = 0.0f; //!< how fast the legs turn (degree/s; to the right, positive)
};

/*!
  \enum KickKind
  \brief how hard and where a kick goes, which decides how a figure swings
*/
enum class KickKind {
    Tap, //!< a touch: holding or dribbling the ball
    Pass, //!< a pass, with the side of the foot
    Shot, //!< hard toward the opponents' goal
};

/*!
  \struct FigureKick
  \brief a kick of a figure around the time drawn, from the log (FigureActionLog)
*/
struct FigureKick {
    bool on = false;
    float phase = 0.0f; //!< cycles from the foot meeting the ball (negative before it)
    KickKind kind = KickKind::Tap;
    bool right = true; //!< with the right foot: the ball on the right of the body
    float way = 0.0f; //!< where the ball goes, from the body direction (degree, clockwise on the screen positive)
    float ball_x = 0.0f; //!< where the ball was at the moment, on the field (m)
    float ball_y = 0.0f;
    float dir_x = 1.0f; //!< the way the ball goes, on the field (unit)
    float dir_y = 0.0f;
    float ball_radius = 0.0f; //!< the ball as drawn (m): the foot meets its back
};

/*!
  \struct FigureRun
  \brief an action that lasts as long as a state bit of the player does (a
  tackle, being down after a foul), around the time drawn
*/
struct FigureRun {
    bool on = false;
    float phase = 0.0f; //!< cycles from the action's moment (the cycle before the bit's first frame)
    float last = 0.0f; //!< how many cycles the bit lasted after its first frame
};

/*!
  \struct FigureCatch
  \brief a goalie's catch around the time drawn
*/
struct FigureCatch {
    bool on = false;
    float phase = 0.0f; //!< cycles from the ball coming into the hands
    float hold = 0.0f; //!< how many cycles the hands keep the ball up (catch_ban_cycle)
    float way = 0.0f; //!< where the ball was, from the body direction (degree)
    float side = 0.0f; //!< how far to the right of the body the ball was (m; left: negative)
    float height = 0.0f; //!< how high the ball was (m; 0 in the logs of today's server, which has no height)
};

/*!
  \struct FigureActions
  \brief what a figure does at the time drawn besides walking, from the log
  (FigureActionLog): so that Figures3D poses it from the frames only
*/
struct FigureActions {
    float neck = 0.0f; //!< the head's angle from the body (degree)
    bool pointing = false; //!< pointing with an arm (pointto)
    float point_way = 0.0f; //!< where, from the body direction (degree)
    float point_distance = 0.0f; //!< how far (m)
    FigureKick kick;
    FigureRun tackle;
    FigureRun fouled; //!< down after a foul by an opponent
    FigureCatch caught;
};

/*!
  \struct FigureInstance
  \brief a player drawn as a human figure (docs/plan/2026-11-11_3d-view.md §5.8):
  SceneBuilder puts it where the player is, and Figures3D poses it
*/
struct FigureInstance {
    int player; //!< its index in the show (0 to 21)
    float x; //!< on the field (m)
    float y;
    float body; //!< the body direction (degree)
    float height; //!< from the ground to the top of the head (m)
    QVector4D color; //!< the jersey: the team or state color, as the body of a cylinder
    FigureMotion motion = FigureMotion(); //!< how it moves (standing still unless told)
    FigureActions actions = FigureActions(); //!< what it does besides walking (nothing unless told)
    FigureLook look = FigureLook(); //!< how it looks as a human body (the standard looks unless told)
};

/*!
  \struct SceneContent
  \brief what the 3D view draws in a frame, by the shape (Scene3D draws each
  list with one instanced draw call). The shapes have unit sizes, and the
  model matrices place and scale them. It knows nothing of OpenGL, so that
  the tests can check it (docs/plan/2026-11-11_3d-view.md §5.1).

  The builders fill it (SceneBuilder: the goals, the players, the ball and
  the labels; OverlayBuilder: the overlays), and Scene3D draws it.
*/
struct SceneContent {
    // lit by the sun
    std::vector< Instance3D > cylinders; //!< radius 1, height 1 along y, centered: the bodies, the goal posts and crossbars
    std::vector< Instance3D > spheres; //!< radius 1: the heads
    std::vector< Instance3D > balls; //!< radius 1, turned as it rolled (BallSpin): the ball (casts no shadow of the sun: its own disc below tells its height)
    std::vector< Instance3D > cones; //!< base of radius 1 at the origin, tip at x = 1: the body and face directions
    std::vector< FigureInstance > figures; //!< the players drawn as human figures (in place of a cylinder and a sphere)
    std::vector< float > figure_joints; //!< the skin matrices of the figures, in their order (Figures3D::pose(); 16 floats a joint)
    std::vector< int > figure_bodies; //!< the body drawn of each figure (Figures3D::pose(): an index of Figures3D's bodies)

    //! the sky dome and the haze over the ground beyond (Scenery3D): the option sky_3d
    bool sky = false;
    //! the realistic grass over the ground image (Grass3D): the option grass_3d realistic
    bool realistic_grass = false;
    //! the stands along the touch lines (Stands3D): the option stands_3d
    bool stands = false;
    //! the ball's pattern, twelve pentagons (Ball3D): the option ball_pattern_3d
    bool ball_pattern = false;
    //! the color of the ground beyond the ground image (Scenery3D::farGroundColor())
    QVector4D far_ground = QVector4D( 0.1f, 0.54f, 0.1f, 1.0f );

    //! the sun casts the detailed shadows of the solids above (but the ball
    //! and the cones) on the ground with the shadow map (Sun3D): the option
    //! ShadowMode::Detailed, and not from the top camera
    bool shadows = false;

    // flat and translucent, in this order (the layers on the ground, low to high)
    std::vector< Instance3D > blob_shadows; //!< radius 1 on the xz plane, darkest in the middle: the simple shadows of the players (Sun3D::blobShadow())
    std::vector< Instance3D > discs; //!< radius 1 on the xz plane: the circles under the players, the ball's shadow
    std::vector< Instance3D > rings; //!< 0.92 to 1.08 on the xz plane: the outline of a highlighted player
    std::vector< Instance3D > decay_rings; //!< 1.1 to 1.22 on the xz plane: the decay of effort or recovery
    std::vector< Instance3D > quads; //!< 1 x 1 on the xy plane, centered, both sides: the nets and the floors of the goals
    std::vector< Instance3D > ball_rings; //!< 2 x 2 on the xz plane scaled by the radius: the ring around the ball (BallAids3D), over the circles
    float ball_ring_radius = 0.0f; //!< the ring's radius (m)
    float ball_ring_width = 0.0f; //!< its line's width as drawn (m)

    // unlit panels of the unit quad, by the end (0: left, 1: right): its
    // image on the front (+z), the back plain in the instance's color
    std::array< std::vector< Instance3D >, 2 > boards; //!< the boards around the pitch, the image once in every Stadium3D::BOARD_TILE m
    std::array< std::vector< Instance3D >, 2 > banners; //!< the banners behind the goals, the image once across

    // made again in every frame, translucent too (OverlayBuilder)
    std::vector< ColorVertex3D > sectors; //!< triangles: the view areas
    std::vector< ColorVertex3D > lines; //!< each two vertices, a line: the overlays on the ground
    std::vector< ColorVertex3D > dotted_lines; //!< each two vertices, a dotted line
    std::vector< ColorVertex3D > points; //!< dots
    std::vector< ColorVertex3D > ball_trail; //!< triangles, both sides: the trail behind the ball (BallAids3D), over the rest

    // written by the HUD (QPainter), not by OpenGL
    std::vector< OverlayText > texts; //!< the texts of the overlays

    // over everything, facing the camera, from far to near
    std::vector< Billboard3D > labels; //!< the numbers and the cards (LabelAtlas)

    // not drawn
    std::vector< PlayerPick > picks; //!< the players drawn, which a right click can aim at

    //! empty all the lists (their memory is kept for the next frame)
    void clear()
      {
          cylinders.clear();
          spheres.clear();
          balls.clear();
          cones.clear();
          figures.clear();
          figure_joints.clear();
          figure_bodies.clear();
          sky = false;
          realistic_grass = false;
          stands = false;
          ball_pattern = false;
          shadows = false;
          blob_shadows.clear();
          discs.clear();
          rings.clear();
          decay_rings.clear();
          quads.clear();
          for ( auto & b : boards ) b.clear();
          for ( auto & b : banners ) b.clear();
          sectors.clear();
          ball_trail.clear();
          ball_rings.clear();
          ball_ring_radius = 0.0f;
          ball_ring_width = 0.0f;
          lines.clear();
          dotted_lines.clear();
          points.clear();
          texts.clear();
          labels.clear();
          picks.clear();
      }
};

#endif
