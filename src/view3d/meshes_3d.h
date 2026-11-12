// -*-c++-*-

/*!
  \file meshes_3d.h
  \brief the shapes of the 3D view. Header File.
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

#ifndef RCSSMONITOR_MESHES_3D_H
#define RCSSMONITOR_MESHES_3D_H

#include <vector>

/*!
  \struct Vertex3D
  \brief a vertex of a shape: the position, the normal, and the texture
  coordinates. The vertex buffers hold them as they are, 8 floats each
  (InstancedMesh::create()).
*/
struct Vertex3D {
    float position[3]; //!< x, y, z in the shape's own space
    float normal[3]; //!< the direction the surface faces here, of length 1: how it is lit
    float uv[2]; //!< where in a texture image (0 to 1), for a textured shape
};

/*!
  \brief the shapes of the 3D view, as lists of triangles (3 vertices each).

  GPUs draw only triangles: a round shape is cut into many flat ones.
  Each triangle has a front: the side from which its 3 corners go
  counterclockwise. The normals of its corners face that side. A solid
  shows only the fronts, outward, and OpenGL skips the backs ("culling",
  RenderState). So the order of the corners matters; add_triangle()
  (meshes_3d.cpp) puts them in that order from the normals.

  The shapes have unit sizes at the origin; the model matrices of the
  instances place and scale them (SceneBuilder). They know nothing of
  OpenGL, so that the tests (test_meshes_3d) can check them.
*/
namespace Meshes3D {

//! radius 1, height 1 along y, centered, with its ends: the bodies, the goal posts
std::vector< Vertex3D > cylinder( const int segments );

//! radius 1: the heads, the ball
std::vector< Vertex3D > sphere( const int segments,
                                const int rings );

//! the base of radius 1 on the yz plane at the origin, the tip at x = 1: the directions
std::vector< Vertex3D > cone( const int segments );

//! from inner to outer radius on the xz plane, facing up: a disc if inner is 0
std::vector< Vertex3D > ring( const float inner,
                              const float outer,
                              const int segments );

//! 1 x 1 on the xy plane, centered, facing +z: the nets
std::vector< Vertex3D > quad();

/*!
  \brief a length (x) by width (z) rectangle on the xz plane, centered,
  facing up, with the texture coordinates from (0, 0) at its -x -z corner to
  (1, 1) at its +x +z corner: the ground
*/
std::vector< Vertex3D > ground( const float length,
                                const float width );

}

#endif
