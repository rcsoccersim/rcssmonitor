// -*-c++-*-

/*!
  \file ground_texture.h
  \brief the image of the ground of the 3D view. Header File.
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

#ifndef RCSSMONITOR_GROUND_TEXTURE_H
#define RCSSMONITOR_GROUND_TEXTURE_H

#include <QImage>
#include <QString>

class FieldPainter;

namespace rcss {
namespace rcg {
struct ServerParamT;
}
}

/*!
  \brief the ground of the 3D view: the 2D field painter draws the pitch and
  its margins into an image (rcssmonitor-web field-texture.ts).
*/
namespace GroundTexture {

//! the length (along x) of the area of the image (m): the pitch and its margins
double length();
//! the width (along y) of the area of the image (m)
double width();

//! pixels per meter
constexpr double SCALE = 24.0;
//! the width of the pens in pixels per pixel of the 2D view: 3 / 24 m = 12.5 cm
constexpr double PEN_SCALE = 3.0;

/*!
  \brief draw the ground: the field without the goals, which the 3D view
  builds itself. The image is length() * SCALE by width() * SCALE pixels,
  with the center of the pitch at its center and +y down.
*/
QImage draw( FieldPainter & painter );

/*!
  \brief draw the same area at another size: scale pixels per meter, the
  pens pen_scale times as thick (the minimap of the HUD)
*/
QImage draw( FieldPainter & painter,
             const double scale,
             const double pen_scale );

/*!
  \brief what the image depends on (colors, options): the image is drawn
  again when this changes.
*/
QString key( const rcss::rcg::ServerParamT & param );

}

#endif
