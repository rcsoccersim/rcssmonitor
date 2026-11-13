// -*-c++-*-

/*!
  \file overlay_builder.h
  \brief the overlays on the ground of the 3D view. Header File.
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

#ifndef RCSSMONITOR_OVERLAY_BUILDER_H
#define RCSSMONITOR_OVERLAY_BUILDER_H

#include "scene_content.h"

#include <QColor>
#include <QFontMetrics>
#include <QPoint>
#include <QPointF>
#include <QSize>

class DispHolder;
class Options;

namespace rcss {
namespace rcg {
struct ShowInfoT;
}
}

/*!
  \brief what the 3D view draws on the ground as the 2D view does
  (rcssmonitor-web overlays.ts and the view areas of render3d.ts): lines,
  dotted lines, dots and the view areas, in the lists of SceneContent that
  VertexBatch draws. The points are given in the field's coordinates (x, y)
  and laid on the ground at a height. It knows nothing of OpenGL, so that
  the tests check it.
*/
namespace OverlayBuilder {

// the heights of the layers on the ground (§4.3): over the circles under the players (0.02) and their rings (0.03)
constexpr float SECTOR_Y = 0.035f; //!< the view areas
constexpr float LARGE_VIEW_Y = 0.04f; //!< the view of the selected player
constexpr float LINE_Y = 0.045f; //!< the other lines

constexpr float SECTOR_OPACITY = 0.25f; //!< of the view areas
constexpr int CIRCLE_SEGMENTS = 48; //!< of a whole circle
constexpr int SECTOR_SEGMENTS = 32; //!< of a view area, whatever its width
constexpr double LARGE_VIEW_RADII[] = { 20.0, 40.0, 60.0 }; //!< the arcs of the selected player's view (m): the numbers and the teams fade there
constexpr double POINTTO_CROSS = 0.4; //!< half the size of the cross where a player points (m)
constexpr double BALL_PATH_MIN_STEP = 0.01; //!< the ball's path ends when the ball moves less than this in a cycle (m)

/*!
  \brief add a line on the ground
  \param list where (lines or dotted_lines)
  \param a, b its ends in the field's coordinates
  \param y how high
*/
void addLine( std::vector< ColorVertex3D > & list,
              const QPointF & a,
              const QPointF & b,
              const QColor & color,
              const float y = LINE_Y );

/*!
  \brief add an arc of a circle on the ground, as lines whose distances go
  on along the arc (for the dashes of a dotted one)
  \param start the field direction (degree) where it starts
  \param span how far round it goes (degree), clockwise on the 2D screen
*/
void addArc( std::vector< ColorVertex3D > & list,
             const QPointF & center,
             const double radius,
             const QColor & color,
             const double start = 0.0,
             const double span = 360.0,
             const float y = LINE_Y );

/*!
  \brief the body and face directions of the players, from the center to
  the edge of the kickable area, as the 2D view draws them
  (PlayerPainter::drawDir): the body in the player pen's color, the face in
  the neck's unless the view areas show it
*/
void addDirections( SceneContent & content,
                    const rcss::rcg::ShowInfoT & show,
                    const DispHolder & holder,
                    const Options & options );

/*!
  \brief the view areas, when the Show tab asks for them: translucent
  sectors to visible_distance around the face direction, and for the
  selected player the lines of its large view (the arcs at 20, 40 and 60 m,
  the edges to 60 m, the circle of visible_distance), as
  PlayerPainter::drawViewArea
*/
void addViewAreas( SceneContent & content,
                   const rcss::rcg::ShowInfoT & show,
                   const DispHolder & holder,
                   const Options & options );

/*!
  \brief the overlays that the Show tab asks for, as the 2D view
  (PlayerPainter, BallPainter) with the calculations of OverlayMath:
  - the focus point of the selected player (a line to it and a circle)
  - the catch areas of the goalies (the catchable circle, and dotted the
    largest and smallest by the stretch)
  - the tackle area turned with the body, when a tackle or a foul may
    succeed, and its chances as a text
  - the kick acceleration area of the selected player when it can kick:
    the ball's next place and the arcs that bound it, and the largest
    acceleration as a text
  - the pointing: a line to where the player points, and a cross there
  - the offside lines across the pitch, in the team colors
  - the ball's path without noise (Ball Vel Cycle): a line and dots
  The players not connected have none.
*/
void addOverlays( SceneContent & content,
                  const rcss::rcg::ShowInfoT & show,
                  const DispHolder & holder,
                  const Options & options );

/*!
  \struct PlacedText
  \brief a text of an overlay where the HUD writes it
*/
struct PlacedText {
    QPoint baseline; //!< the left of its baseline (logical pixels)
    QString text;
    QColor color;
};

/*!
  \brief where to write the texts of the overlays on the screen: 10 pixels
  right of and 2 pixels below their points, as the 2D view; one that would
  cover another goes a line down (from afar, a player's tackle text and its
  kick's are close). The texts of points behind the camera or off the
  screen are left out.
  \param texts the texts (SceneContent::texts)
  \param view_projection the camera's matrix (CameraPose::projection() * view())
  \param size the size of the screen (logical pixels)
  \param metrics of the font that writes them (the 2D player font)
*/
std::vector< PlacedText > placeTexts( const std::vector< OverlayText > & texts,
                                      const QMatrix4x4 & view_projection,
                                      const QSize & size,
                                      const QFontMetrics & metrics );

}

#endif
