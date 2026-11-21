// -*-c++-*-

/*!
  \file sun_3d.h
  \brief the sun of the 3D view and its shadows. Header File.
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

#ifndef RCSSMONITOR_SUN_3D_H
#define RCSSMONITOR_SUN_3D_H

#include <QMatrix4x4>
#include <QVector3D>

/*!
  \brief the sun of the 3D view (docs/plan/2026-11-11_3d-view.md §4.3, the web
  version's render3d.ts): where it is, and the camera of its shadow map.
  No OpenGL.

  The shadows are drawn in two steps (Scene3D::render()). First, the solids
  that cast them (the players' bodies and heads, the figures, the goal
  posts and crossbars) are drawn as the sun sees them, into a "shadow map":
  for every pixel, only how far from the sun the nearest thing is. Then a
  plane just over the ground, the size of the area of the shadows, looks
  every pixel of itself up in the map: where something is nearer to the sun
  than the plane, it is in the shadow, and the plane is darkened there. The
  ground alone receives the shadows (as in the web version).

  The sun is far, so its rays are parallel: its camera is an orthographic
  one, looking along the rays, and just big enough for a box: the ground
  image (GroundTexture) and SHADOW_MARGIN beyond, up to SHADOW_HEIGHT.
*/
namespace Sun3D {

//! the shadows reach this far beyond the ground image (m)
constexpr float SHADOW_MARGIN = 4.0f;
/*!
  \brief the highest thing that casts a shadow (m): a goalkeeper's hands
  raised to catch, of the tallest type (1.1) at the largest scale (1.5),
  are about 3.6 m high (3 m in the web version, whose figures are smaller)
*/
constexpr float SHADOW_HEIGHT = 4.0f;
//! how dark a full shadow is: the opacity of black over the ground
constexpr float SHADOW_OPACITY = 0.4f;
//! the plane of the shadows, just over the ground (§4.3: under the circles of the players)
constexpr float SHADOW_PLANE_Y = 0.01f;

/*!
  \brief toward the sun, of length 1: beyond the far touch line on the
  left, 44 degrees up, so that from the near side (the broadcast and
  close-up cameras) the shadows fall to the right and toward the camera,
  about as long as a player is high (§11: 8)
*/
QVector3D direction();

//! the length (x) of the area of the shadows, the ground image and the margins (m)
float shadowLength();
//! its width (z) (m)
float shadowWidth();

/*!
  \brief the sun's camera of the shadow map: takes a point of the world to
  -1 to 1 in x, y and z (OpenGL's cube) where it is in the box of the
  shadows (the area, from the ground up to SHADOW_HEIGHT), z growing away
  from the sun
*/
QMatrix4x4 shadowViewProj();

/*!
  \brief the simple shadow of a standing player (ShadowMode::Simple): the
  model matrix of the unit disc (radius 1 on the xz plane) as an ellipse
  on the plane of the shadows, from the feet away from the sun as far as
  the sun's ray over the top of the head reaches the ground, as wide as
  the body. Scene3D draws it darkest in the middle, fading out to the edge.
  \param x, z where the feet are (m)
  \param height the top of the head (m)
  \param width how wide the body is (m)
*/
QMatrix4x4 blobShadow( const float x,
                       const float z,
                       const float height,
                       const float width );

}

#endif
