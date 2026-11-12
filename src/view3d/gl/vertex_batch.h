// -*-c++-*-

/*!
  \file vertex_batch.h
  \brief lines, points and triangles of the 3D view, made in every frame. Header File.
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

#ifndef RCSSMONITOR_GL_VERTEX_BATCH_H
#define RCSSMONITOR_GL_VERTEX_BATCH_H

#include "instance_3d.h"

#include <QOpenGLFunctions_3_3_Core>

#include <vector>

/*!
  \class VertexBatch
  \brief vertices with their own colors (ColorVertex3D), drawn as lines,
  points or triangles with one draw call: what changes shape in every frame
  (the overlays on the ground, the view areas), which InstancedMesh cannot
  draw as copies of one shape.

  The vertices are sent to the GPU in every frame into one buffer, which
  grows when it has to and is reused otherwise (as InstancedMesh). They go
  to the attributes A_POSITION, A_COLOR and A_DISTANCE (GLAttribute).

  Every function needs the OpenGL context current.
*/
class VertexBatch {
public:

    //! how the vertices make shapes
    enum class Primitive {
        Lines, //!< each two vertices, a line of one pixel
        Points, //!< each vertex, a square dot (its size: the shader's uPointSize)
        Triangles, //!< each three vertices, a triangle
    };

    VertexBatch() = default;
    VertexBatch( const VertexBatch & ) = delete;
    VertexBatch & operator=( const VertexBatch & ) = delete;

    //! make the buffer and describe it in the VAO
    void create( QOpenGLFunctions_3_3_Core & gl );

    //! delete the buffer and the VAO
    void destroy( QOpenGLFunctions_3_3_Core & gl );

    /*!
      \brief send the vertices to the GPU and draw them, with the shader
      program that is bound and the render state that is applied
      \param primitive what shapes they make
    */
    void draw( QOpenGLFunctions_3_3_Core & gl,
               const std::vector< ColorVertex3D > & vertices,
               const Primitive primitive );

private:
    GLuint M_vao = 0; //!< the vertex array object (0: none)
    GLuint M_buffer = 0; //!< the vertices of the frame
    std::size_t M_capacity = 0; //!< the vertices that the buffer has room for
    std::vector< float > M_data; //!< the vertices as the buffer holds them, reused in every frame
};

#endif
