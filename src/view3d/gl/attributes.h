// -*-c++-*-

/*!
  \file attributes.h
  \brief the vertex attributes of the shaders of the 3D view. Header File.
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

#ifndef RCSSMONITOR_GL_ATTRIBUTES_H
#define RCSSMONITOR_GL_ATTRIBUTES_H

/*!
  \brief the locations of the vertex attributes.

  A vertex shader (a .vert file in src/view3d/shaders) runs once for every vertex
  that is drawn, and reads its inputs, the "vertex attributes", from slots
  numbered 0, 1, 2, ... ("locations"). The C++ side fills those slots from
  buffers (InstancedMesh::create()), and the shader declares which slot
  holds what, e.g.

      layout( location = A_POSITION ) in vec3 aPos;

  Both sides must agree on the numbers, so they are decided only here:
  ShaderProgram adds "#define A_POSITION 0" etc. to the top of every
  shader. To add an attribute, add its number here, add its #define in
  shader_header() (shader_program.cpp), and fill it in
  InstancedMesh::create().

  There are two kinds of attributes:
  - of a vertex: the same for every instance of a shape (Vertex3D).
  - of an instance: one value per copy of the shape (Instance3D), e.g. where
    a player is and its color. InstancedMesh sets them with a "divisor" of
    1, which makes OpenGL step to the next value once per instance instead
    of once per vertex.

  The billboards (BillboardMesh, Billboard3D) have other attributes of an
  instance. They are read by other shaders from other buffers, so they
  reuse the locations of MODEL.
*/
namespace GLAttribute {

// of a vertex (Vertex3D)
constexpr unsigned int POSITION = 0; //!< vec3: the position in the shape's own space
constexpr unsigned int NORMAL = 1; //!< vec3: the direction the surface faces, for lighting
constexpr unsigned int UV = 2; //!< vec2: where in a texture image the vertex is (0 to 1)

// of an instance (Instance3D)
constexpr unsigned int MODEL = 3; //!< mat4: where to put the shape. A mat4 takes 4 locations, 3 to 6, one per column
constexpr unsigned int COLOR = 7; //!< vec4: the color, with the opacity in w

// of a billboard (Billboard3D), in place of MODEL
constexpr unsigned int BILLBOARD_CENTER = 3; //!< vec3: where it is in the world
constexpr unsigned int BILLBOARD_SIZE = 4; //!< vec4: width, height, and the offset along right and up (m)
constexpr unsigned int BILLBOARD_UV = 5; //!< vec4: the part of the texture: left, top, right, bottom (0 to 1)

}

#endif
