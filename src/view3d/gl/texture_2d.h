// -*-c++-*-

/*!
  \file texture_2d.h
  \brief a texture of the 3D view. Header File.
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

#ifndef RCSSMONITOR_GL_TEXTURE_2D_H
#define RCSSMONITOR_GL_TEXTURE_2D_H

#include <QOpenGLFunctions_3_3_Core>

class QImage;

/*!
  \class Texture2D
  \brief an image on the GPU (a "texture"), for a shader to paint onto
  triangles (the ground).

  A shader reads a texture through a numbered "texture unit": bind() puts the
  texture on a unit, and the shader's sampler2D uniform is set to the number
  of that unit (Scene3D::render()). The vertices tell where in the image they
  are (Vertex3D::uv, 0 to 1 across the image).

  Every function needs the OpenGL context current.
*/
class Texture2D {
public:

    Texture2D() = default;
    Texture2D( const Texture2D & ) = delete;
    Texture2D & operator=( const Texture2D & ) = delete;

    //! send the image to the GPU (the texture is made at the first time)
    void upload( QOpenGLFunctions_3_3_Core & gl,
                 const QImage & image );

    //! put it on the texture unit (0, 1, ...) for the draw calls that follow
    void bind( QOpenGLFunctions_3_3_Core & gl,
               const int unit ) const;

    //! delete it
    void destroy( QOpenGLFunctions_3_3_Core & gl );

private:
    GLuint M_id = 0; //!< OpenGL's number of the texture (0: none)
};

#endif
