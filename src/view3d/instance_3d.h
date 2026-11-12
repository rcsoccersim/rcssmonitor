// -*-c++-*-

/*!
  \file instance_3d.h
  \brief an instance of a shape in the 3D view. Header File.
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

#ifndef RCSSMONITOR_INSTANCE_3D_H
#define RCSSMONITOR_INSTANCE_3D_H

#include <QMatrix4x4>
#include <QRectF>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

/*!
  \struct Instance3D
  \brief one copy of a shape: its model matrix, and its color with the
  opacity in w.

  The shapes have unit sizes at the origin (Meshes3D). The model matrix
  moves, turns and scales a copy to where it is in the world. It is built
  with QMatrix4x4's translate(), rotate() and scale(), which apply to the
  shape in the reverse order of the calls: in

      m.translate( x, y, z );
      m.rotate( angle, 0, 1, 0 );
      m.scale( r, h, r );

  the shape is first scaled, then turned about y, then moved to (x, y, z)
  (SceneBuilder).
*/
struct Instance3D {
    QMatrix4x4 model; //!< the shape's space to the world
    QVector4D color; //!< red, green, blue, opacity: 0 to 1
};

/*!
  \struct Billboard3D
  \brief a "billboard": a rectangle that always faces the camera, with a
  part of a texture on it (the number labels and the cards, LabelAtlas).
  It stays upright on the screen, and its size is in meters of the world
  (so it is smaller far away), as three.js's sprites.
*/
struct Billboard3D {
    QVector3D center; //!< where it is in the world
    QVector2D size; //!< width and height (m)
    QVector2D offset; //!< from center to the middle of the rectangle, along the camera's right and up (m)
    QRectF uv; //!< the part of the texture (LabelAtlas::uv())
    QVector4D color; //!< multiplies the texture's color: the tint, and the opacity in w
};

#endif
