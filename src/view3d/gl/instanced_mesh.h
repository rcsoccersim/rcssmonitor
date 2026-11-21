// -*-c++-*-

/*!
  \file instanced_mesh.h
  \brief a shape of the 3D view, drawn in instances. Header File.
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

#ifndef RCSSMONITOR_GL_INSTANCED_MESH_H
#define RCSSMONITOR_GL_INSTANCED_MESH_H

#include "instance_3d.h"
#include "meshes_3d.h"

#include <QOpenGLFunctions_3_3_Core>

#include <vector>

/*!
  \class InstancedMesh
  \brief a shape, and its copies ("instances") drawn all at once.

  The 22 bodies of the players are the same cylinder, only moved, scaled
  and colored differently. So the cylinder is sent to the GPU once
  (create()), and in every frame only a list of instances, a model matrix
  and a color for each, is sent and drawn with one draw call (draw()).
  Each draw call costs some time on the CPU, so one per shape is much
  cheaper than one per player.

  On the GPU, the data is in "buffers" (blocks of GPU memory):
  - the vertex buffer: the triangles of the shape (Vertex3D), sent once.
  - the instance buffer: the instances (Instance3D), sent in every frame.
  A "vertex array object" (VAO) remembers how the vertex shader reads them:
  which buffer, where in each element, into which attribute (GLAttribute).
  Binding the VAO before drawing brings all that back at once.

  Every function needs the OpenGL context current.
*/
class InstancedMesh {
public:

    InstancedMesh() = default;
    InstancedMesh( const InstancedMesh & ) = delete;
    InstancedMesh & operator=( const InstancedMesh & ) = delete;

    /*!
      \brief send the shape to the GPU, and describe the buffers in the VAO.
      \param gl the OpenGL functions
      \param vertices the triangles of the shape (Meshes3D)
    */
    void create( QOpenGLFunctions_3_3_Core & gl,
                 const std::vector< Vertex3D > & vertices );

    //! delete the buffers and the VAO
    void destroy( QOpenGLFunctions_3_3_Core & gl );

    /*!
      \brief send the instances to the GPU and draw them, with the shader
      program that is bound (ShaderProgram::bind()) and the render state
      that is applied (RenderState::apply()).
      \param gl the OpenGL functions
      \param instances where and in which color to draw the shape; nothing is drawn if empty
    */
    void draw( QOpenGLFunctions_3_3_Core & gl,
               const std::vector< Instance3D > & instances );

private:
    GLuint M_vao = 0; //!< the vertex array object (0: none). OpenGL names its objects by numbers
    GLuint M_vertex_buffer = 0; //!< the triangles of the shape
    GLuint M_instance_buffer = 0; //!< the instances of the frame
    GLsizei M_vertex_count = 0; //!< the vertices of the shape (3 per triangle)
    std::size_t M_capacity = 0; //!< the instances that the instance buffer has room for
    std::vector< float > M_instance_data; //!< the instances as the buffer holds them, reused in every frame
};

#endif
