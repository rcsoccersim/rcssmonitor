// -*-c++-*-

/*!
  \file grass_3d.h
  \brief the realistic grass of the 3D view. Header File.
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

#ifndef RCSSMONITOR_GRASS_3D_H
#define RCSSMONITOR_GRASS_3D_H

#include <QImage>
#include <QVector3D>

#include <cstdint>

/*!
  \brief the realistic grass of the 3D view (GrassMode::Realistic,
  docs/PLAN_3D-VIEW.md 8-2b), apart from OpenGL. What does not depend on the
  camera is painted once into the ground image (paintGround(): the patches
  and the wear), and the ground shader (ground.frag) multiplies its colors
  by the rest (the stripes, and the blades near the camera), as
  stripeShade() and bladesFade() compute. Reading textures is what costs
  on a weak GPU (about 0.25 ms each over the screen of an integrated one).

  - The mowed stripes, by the way the camera looks: a mower bends the
    blades its way, and they look lighter seen along it, darker seen
    against it (seen from the other end, a stripe's light and dark swap).
    The stripes (FieldPainter::GRASS_STRIPE_WIDTH, one edge on the halfway
    line) are mowed along the field's y, one way and back in turn; the
    even ones away from the near side, so that from the main cameras they
    are light as the plain stripes. A part of it, STRIPE_FIXED, does not
    depend on the way, so that they show from the ends of the pitch too.
    Looking straight down, only that part.
  - The blades and the patches of the grass: a noise image made at the
    start (noiseImage(), no material to bundle). The patches, one tile in
    PATCH_TILE meters, are painted into the ground image. The blades, one
    tile in DETAIL_TILE meters, are read by the shader only nearer than
    BLADES_FAR (far away they would average out anyway).
  - The wear where the players tread most: the goal mouths, the penalty
    spots and the center, toward dry grass and soil (WEAR_COLOR), patchy by
    the noise, painted into the ground image.
*/
namespace Grass3D {

//! how much lighter or darker a stripe is (times 1 + or -): 1.065 and 0.935
constexpr float STRIPE_CONTRAST = 0.065f;
//! of that, the part that does not depend on the way the camera looks
constexpr float STRIPE_FIXED = 0.35f;
//! the blades: the noise one tile in this many meters, the colors times 1 +- half of DETAIL_AMOUNT
constexpr float DETAIL_TILE = 1.7f;
constexpr float DETAIL_AMOUNT = 0.16f;
//! the patches: one tile in this many meters, times 1 +- half of PATCH_AMOUNT
constexpr float PATCH_TILE = 23.0f;
constexpr float PATCH_AMOUNT = 0.12f;
//! the worn grass: the colors toward this, dry grass and soil (RGB 0 to 1), as far as worn
const QVector3D WEAR_COLOR( 0.50f, 0.50f, 0.28f );
//! at most this much worn
constexpr float WEAR_MAX = 0.6f;
//! the pixels of the noise image, across and down
constexpr int NOISE_SIZE = 256;
//! the blades in full nearer than this from the camera (m), none farther than that
constexpr float BLADES_NEAR = 15.0f;
constexpr float BLADES_FAR = 30.0f;

/*!
  \brief the noise of the grass: a gray image of values about 0.5 (0 to 1),
  of a few octaves, that tiles (its right edge goes on to its left, its
  bottom to its top)
  \param size the pixels across and down (a power of 2)
  \param seed the same seed, the same image
*/
QImage noiseImage( const int size = NOISE_SIZE,
                   const std::uint32_t seed = 20261115u );

/*!
  \brief how the mowed stripes shade a point of the ground (ground.frag)
  \param x, z the point (the world's, m)
  \param camera where the camera is
  \return the factor of the colors, about 1 - STRIPE_CONTRAST to 1 + STRIPE_CONTRAST
*/
float stripeShade( const float x,
                   const float z,
                   const QVector3D & camera );

/*!
  \brief how worn the grass is at a point, before the noise (ground.frag):
  0 to 1, most in the goal mouths, at the penalty spots and the center
  \param x, z the point (the world's, m)
*/
float wearAt( const float x,
              const float z );

/*!
  \brief how much of the blades shows at a distance from the camera
  (ground.frag): 1 nearer than BLADES_NEAR, 0 farther than BLADES_FAR
*/
float bladesFade( const float distance );

/*!
  \brief paint the patches and the wear into the ground image (drawn by the
  2D field painter, GroundTexture): each pixel toward WEAR_COLOR as far as
  worn (wearAt(), patchy by the noise), times 1 +- half of PATCH_AMOUNT by
  the noise at PATCH_TILE meters a tile
  \param image the ground image (RGBA8888), the center of the pitch at its
  center, +y down
  \param scale its pixels per meter
*/
void paintGround( QImage & image,
                  const double scale );

}

#endif
