// -*-c++-*-

/*!
  \file billboard_mesh.h
  \brief the billboards of the 3D view. Header File.
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

#ifndef RCSSMONITOR_GL_BILLBOARD_MESH_H
#define RCSSMONITOR_GL_BILLBOARD_MESH_H

#include "instance_3d.h"

#include <QOpenGLFunctions_3_3_Core>

#include <vector>

/*!
  \class BillboardMesh
  \brief the billboards (Billboard3D: the number labels and the cards),
  drawn all at once, as InstancedMesh draws its shapes.

  The shape is one unit square (Meshes3D::quad()). Each instance has its
  place, size and part of the texture, and the vertex shader
  (shaders/billboard.vert) spreads the square along the camera's right and
  up, so that it faces the camera. The instances go to the attributes
  A_BILLBOARD_CENTER, A_BILLBOARD_SIZE, A_BILLBOARD_UV and A_COLOR
  (GLAttribute).

  Every function needs the OpenGL context current.
*/
class BillboardMesh {
public:

    BillboardMesh() = default;
    BillboardMesh( const BillboardMesh & ) = delete;
    BillboardMesh & operator=( const BillboardMesh & ) = delete;

    //! send the square to the GPU, and describe the buffers in the VAO
    void create( QOpenGLFunctions_3_3_Core & gl );

    //! delete the buffers and the VAO
    void destroy( QOpenGLFunctions_3_3_Core & gl );

    /*!
      \brief send the billboards to the GPU and draw them in their order,
      with the shader program that is bound and the texture on its unit
    */
    void draw( QOpenGLFunctions_3_3_Core & gl,
               const std::vector< Billboard3D > & billboards );

private:
    GLuint M_vao = 0; //!< the vertex array object (0: none)
    GLuint M_vertex_buffer = 0; //!< the square
    GLuint M_instance_buffer = 0; //!< the billboards of the frame
    GLsizei M_vertex_count = 0;
    std::size_t M_capacity = 0; //!< the billboards that the instance buffer has room for
    std::vector< float > M_instance_data; //!< the billboards as the buffer holds them, reused in every frame
};

#endif
