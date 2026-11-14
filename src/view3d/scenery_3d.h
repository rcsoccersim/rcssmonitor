// -*-c++-*-

/*!
  \file scenery_3d.h
  \brief the scenery of the 3D view: the sky, the haze and the ground beyond. Header File.
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

#ifndef RCSSMONITOR_SCENERY_3D_H
#define RCSSMONITOR_SCENERY_3D_H

#include <QColor>
#include <QVector3D>
#include <QVector4D>

/*!
  \brief the scenery of the 3D view (docs/PLAN_3D-VIEW.md §4.3, the web
  version's scenery.ts), apart from OpenGL: what Scene3D draws around the
  stadium, and the same computations as its shaders, for the tests.

  - The ground beyond: a plain plane FAR_GROUND around the center, a
    little under the ground image, in a darker shade of the field color.
  - The sky (the option sky_3d): a dome of SKY_RADIUS around the camera,
    from the pale horizon to the blue above (sky.frag), and the haze that
    melts the ground beyond into the horizon from HAZE_FROM to HAZE_TO
    away from the camera (outside.frag). Without the sky, the background
    is CLEAR and there is no haze (as in the web version).

  The grass stripes are drawn by the 2D field painter into the ground image
  (FieldPainter::drawGround()).
*/
namespace Scenery3D {

//! the sky: the color straight up, at the horizon, and below the horizon beyond the ground
const QColor SKY_ZENITH( 0x3d, 0x7c, 0xcc );
const QColor SKY_HORIZON( 0xd4, 0xe4, 0xf0 );
const QColor SKY_BELOW( 0x6f, 0x9f, 0x7f );
//! the background without the sky
const QColor CLEAR( 0x87, 0xa9, 0x6b );
//! the dome's radius (m), within the cameras' far plane (CameraPose::FAR)
constexpr float SKY_RADIUS = 1900.0f;
//! the haze: none nearer than this (m) from the camera, all of it at that
constexpr float HAZE_FROM = 600.0f;
constexpr float HAZE_TO = 1500.0f;
//! the ground beyond reaches this far from the center each way (m), to melt into the haze
constexpr float FAR_GROUND = HAZE_TO;
//! and is this far under the ground image (m), drawn before it
constexpr float FAR_GROUND_Y = -0.02f;
//! its color: the field color times this (#1a8a1a for the default #1fa01f, as the web version)
constexpr float FAR_GROUND_SHADE = 0.86f;

/*!
  \brief the color of the sky in a direction (sky.frag)
  \param direction from the camera (any length)
*/
QColor skyColor( const QVector3D & direction );

/*!
  \brief how much of the haze is over a point (outside.frag): 0 to 1, by
  its distance from the camera, linearly from HAZE_FROM to HAZE_TO
*/
float haze( const float distance );

//! the color of the ground beyond (RGBA 0 to 1)
QVector4D farGroundColor( const QColor & field );

}

#endif
