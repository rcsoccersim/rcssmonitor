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
};

/*!
  \struct FigureInstance
  \brief a player drawn as a human figure (docs/PLAN_3D-VIEW.md §5.8):
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
};

/*!
  \struct SceneContent
  \brief what the 3D view draws in a frame, by the shape (Scene3D draws each
  list with one instanced draw call). The shapes have unit sizes, and the
  model matrices place and scale them. It knows nothing of OpenGL, so that
  the tests can check it (docs/PLAN_3D-VIEW.md §5.1).

  The builders fill it (SceneBuilder: the goals, the players, the ball and
  the labels; OverlayBuilder: the overlays), and Scene3D draws it.
*/
struct SceneContent {
    // lit by the sun
    std::vector< Instance3D > cylinders; //!< radius 1, height 1 along y, centered: the bodies, the goal posts and crossbars
    std::vector< Instance3D > spheres; //!< radius 1: the heads and the ball
    std::vector< Instance3D > cones; //!< base of radius 1 at the origin, tip at x = 1: the body and face directions
    std::vector< FigureInstance > figures; //!< the players drawn as human figures (in place of a cylinder and a sphere)
    std::vector< float > figure_joints; //!< the skin matrices of the figures, in their order (Figures3D::pose(); 16 floats a joint)

    // flat and translucent, in this order (the layers on the ground, low to high)
    std::vector< Instance3D > discs; //!< radius 1 on the xz plane: the circles under the players, the ball's shadow
    std::vector< Instance3D > rings; //!< 0.92 to 1.08 on the xz plane: the outline of a highlighted player
    std::vector< Instance3D > decay_rings; //!< 1.1 to 1.22 on the xz plane: the decay of effort or recovery
    std::vector< Instance3D > quads; //!< 1 x 1 on the xy plane, centered, both sides: the nets and the floors of the goals

    // unlit panels of the unit quad, by the end (0: left, 1: right): its
    // image on the front (+z), the back plain in the instance's color
    std::array< std::vector< Instance3D >, 2 > boards; //!< the boards around the pitch, the image once in every Stadium3D::BOARD_TILE m
    std::array< std::vector< Instance3D >, 2 > banners; //!< the banners behind the goals, the image once across

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

    //! empty all the lists (their memory is kept for the next frame)
    void clear()
      {
          cylinders.clear();
          spheres.clear();
          cones.clear();
          figures.clear();
          figure_joints.clear();
          discs.clear();
          rings.clear();
          decay_rings.clear();
          quads.clear();
          for ( auto & b : boards ) b.clear();
          for ( auto & b : banners ) b.clear();
          sectors.clear();
          lines.clear();
          dotted_lines.clear();
          points.clear();
          texts.clear();
          labels.clear();
          picks.clear();
      }
};

#endif
