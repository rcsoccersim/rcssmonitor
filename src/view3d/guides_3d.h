// -*-c++-*-

/*!
  \file guides_3d.h
  \brief the compass and the minimap over the 3D view. Header File.
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


#ifndef RCSSMONITOR_GUIDES_3D_H
#define RCSSMONITOR_GUIDES_3D_H

#include "camera_3d.h"
#include "camera_mode.h"

#include <QImage>
#include <QPointF>
#include <QRectF>

class QPainter;
class Options;

namespace rcss {
namespace rcg {
struct ShowInfoT;
}
}

/*!
  \brief the compass and the minimap over the 3D view (rcssmonitor-web
  hud/guides.ts, docs/PLAN_3D-VIEW.md §5.5), optional, to tell which way
  the camera looks:
  - The compass: a strip at the top with the 360 degrees around the camera
    direction, the directions of the goals (L, R, in the team colors) and
    of the ball, and the camera's horizontal field of view brighter. Angles
    are field directions (clockwise on the 2D screen), so clockwise from the
    camera direction is to the right.
  - The minimap: the pitch at the bottom right as the 2D view shows it,
    with the players, the ball, and where the camera is and what it sees. It
    shows everything, whatever the player camera's sight: it is the map of
    the game.
  The HUD draws them with QPainter over the scene. Here, the arithmetic and
  the drawing on any QPainter: no OpenGL, so that the tests check them on
  an image.
*/
namespace Guides3D {

/*!
  \struct CameraGuide
  \brief where the camera is on the field and which way it looks
*/
struct CameraGuide {
    float x; //!< on the field (m)
    float y;
    float direction; //!< the field direction it looks in (degree)
    float fov; //!< the horizontal field of view (degree)
};

//! whether a guide in the mode shows with the camera (never with the top one: it is a map already)
bool shown( const GuideMode mode,
            const CameraMode camera );

//! where the camera of a pose is and looks, for a view of the aspect
CameraGuide guideOf( const CameraPose & pose,
                     const float aspect );

//! the field direction from a point to another (degree)
float bearing( const QPointF & from,
               const QPointF & to );

//! where a field direction is on the compass: -1 (left end) to 1 (right end), the camera's direction at 0
float compassOffset( const float direction,
                     const float camera_direction );

//! the compass strip in a view of the width: at the top, in the middle, at most 480 pixels wide
QRectF compassRect( const double width );

//! the minimap at the bottom right: the pitch and its margins, its scale in pixels per meter
QRectF minimapRect( const double width,
                    const double height,
                    double * scale );

//! a field point on the minimap
QPointF minimapPoint( const QRectF & rect,
                      const double scale,
                      const QPointF & field );

/*!
  \struct CompassMarks
  \brief where the marks are on the compass (x in the view), for the tests
*/
struct CompassMarks {
    double left; //!< the left goal
    double right; //!< the right goal
    double ball; //!< the ball; < 0 without it
};

/*!
  \brief draw the compass
  \param painter the HUD's
  \param rect where (compassRect())
  \param guide the camera
  \param ball the ball on the field, if any
  \param options the team and ball colors
*/
CompassMarks drawCompass( QPainter & painter,
                          const QRectF & rect,
                          const CameraGuide & guide,
                          const QPointF * ball,
                          const Options & options );

/*!
  \brief draw the minimap
  \param painter the HUD's
  \param rect where (minimapRect())
  \param scale its pixels per meter
  \param guide the camera
  \param show the players and the ball (null: none)
  \param field the pitch drawn by the 2D field painter at the minimap's size
  \param options the colors and the selected player
*/
void drawMinimap( QPainter & painter,
                  const QRectF & rect,
                  const double scale,
                  const CameraGuide & guide,
                  const rcss::rcg::ShowInfoT * show,
                  const QImage & field,
                  const Options & options );

}

#endif
