// -*-c++-*-

/*!
  \file stands_3d.h
  \brief the stands of the 3D view. Header File.
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

#ifndef RCSSMONITOR_STANDS_3D_H
#define RCSSMONITOR_STANDS_3D_H

#include "meshes_3d.h"
#include "stadium_3d.h"

#include <QImage>
#include <QVector3D>

#include <vector>

/*!
  \brief the stands of the 3D view (docs/PLAN_3D-VIEW.md §4.3, 8-3), apart
  from OpenGL: their shape and their images.

  Four stands of one tier, along both touch lines and behind both goals,
  the corners open (as at many old English grounds): simple, as the web
  version's, so that none stands out, and closed, so that seen from behind
  or from outside a stand is a building, not a see-through shell. Each
  rises from a low wall WALL high, STAND_ROWS rows of TREAD by RISER, with
  stairs every AISLE_EVERY m and vomitories (the tunnels to the concourse)
  between them, a wall at its back and its ends.

  The crowd is painted over the whole length of each stand (no tile
  repeating), after a real crowd: mostly dark and plain clothes; the fans
  in their team's color gathered toward their team's end (behind a goal,
  mostly the team that keeps it); fuller in the middle of a side than
  toward the corners; empty seats showing the seats' color; people seated,
  seen from the front.

  A camera inside a stand (below its rows: a close-up camera far and low
  near a touch line) would see it half, from within: insideStand() tells,
  and the stand is then not drawn at all.

  Each stand has its own image: the rows of seats in bands, then the plain
  bands of the front wall and of the concrete. Along it, the image goes the
  way that is right seen from the pitch (Stand::along).
*/
namespace Stands3D {

//! from the touch lines and the goal lines to the front walls of the stands (m): 2 m beyond the boards
constexpr float FRONT = Stadium3D::BOARD_OFFSET + 2.0f;
//! the low wall under the first row (m)
constexpr float WALL = 1.2f;
//! the rows, and how deep and high each is (m), as the web version's
constexpr int STAND_ROWS = 16;
constexpr float TREAD = 0.85f;
constexpr float RISER = 0.42f;
//! the stairs of the aisles: one in every this many meters along a stand, this wide (m)
constexpr float AISLE_EVERY = 14.0f;
constexpr float AISLE_WIDTH = 1.2f;
//! a seat is this wide (m)
constexpr float SEAT = 0.5f;
//! the number of stands
constexpr int STAND_COUNT = 4;

//! the image of a stand: pixels per meter along it, and per row
constexpr int PIXELS_PER_METER = 24;
constexpr int ROW_PIXELS = 24;

/*!
  \struct Stand
  \brief where a stand is: in the world, a point at (a along, height,
  depth back from its front wall) is out * (front + depth) + along * a,
  a within [-half_length, half_length]
*/
struct Stand {
    QVector3D out; //!< away from the pitch, level, of length 1
    QVector3D along; //!< to the right seen from the pitch, level, of length 1: the image goes this way
    float front; //!< from the center to the front wall (m)
    float half_length; //!< along it, from its middle (m)
};

/*!
  \brief a stand: 0 the far side (the field's -y), 1 the near side, 2 behind
  the left goal, 3 behind the right goal
*/
Stand stand( const int index );

//! how deep a stand is, from its front wall to its back wall (m)
float depth();
//! how high its top row is (m)
float top();

/*!
  \brief the triangles of a stand (Meshes3D's vertices: the world's
  positions, the normals, where in the stand's image): the rows, the
  front wall, the back wall and the ends, the faces outward of the solid
*/
std::vector< Vertex3D > mesh( const int index );

/*!
  \brief whether a camera is inside a stand (below its rows, within its
  length and depth): then the stand is not drawn
*/
bool insideStand( const int index,
                  const QVector3D & camera );

//! the size of a stand's image
int imageWidth( const int index );
int imageHeight();

//! the band of a row in the image (k: 0 at the front; pixels from the top)
int rowTop( const int k );

/*!
  \brief the image of a stand: the crowd and the seats, the stairs and the
  vomitories, the walls
  \param index the stand (its own crowd)
  \param left the left team (its fans toward the left end, x < 0)
  \param right the right team
*/
QImage image( const int index,
              const Stadium3D::TeamLook & left,
              const Stadium3D::TeamLook & right );

}

#endif
