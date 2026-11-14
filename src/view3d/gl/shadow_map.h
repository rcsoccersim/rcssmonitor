// -*-c++-*-

/*!
  \file shadow_map.h
  \brief the shadow map of the sun. Header File.
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

#ifndef RCSSMONITOR_GL_SHADOW_MAP_H
#define RCSSMONITOR_GL_SHADOW_MAP_H

#include <QOpenGLFunctions_3_3_Core>

/*!
  \class ShadowMap
  \brief the sun's "shadow map" (Sun3D): an image of only depths, how far
  from the sun the nearest solid is at each of its pixels, drawn as the sun
  sees the scene and then read by the plane of the shadows.

  It is a depth texture attached to a framebuffer of its own (a framebuffer
  is where OpenGL draws: the widget's, an offscreen one, or this, which has
  no colors). begin() draws into it, end() back into the frame's.

  The texture is read as a "shadow sampler" (sampler2DShadow in GLSL): the
  shader gives a point and its depth, and gets how much of it is lit: 1 if
  nothing is nearer to the sun there, 0 if something is. With linear
  filtering, the GPU compares the 4 pixels around the point and mixes the
  results, which softens the edges of the shadows a little; the shader
  averages a few such reads around the point for more (shadow.frag).
*/
class ShadowMap {
public:

    ShadowMap() = default;
    ShadowMap( const ShadowMap & ) = delete;
    ShadowMap & operator=( const ShadowMap & ) = delete;

    /*!
      \brief make the texture and the framebuffer: 4096 pixels square where
      the GPU takes 8192 (it could take 4096 then, with room to spare), else
      2048 (§4.3)
      \return false if the framebuffer cannot be drawn into
    */
    bool create( QOpenGLFunctions_3_3_Core & gl );
    void destroy( QOpenGLFunctions_3_3_Core & gl );
    bool created() const { return M_fbo != 0; }

    //! the pixels across (and down)
    int size() const { return M_size; }

    /*!
      \brief draw into the map from now: all of it, cleared to "farthest"
      (nothing casts a shadow yet)
    */
    void begin( QOpenGLFunctions_3_3_Core & gl ) const;

    /*!
      \brief draw into the frame's framebuffer again
      \param fbo OpenGL's number of the frame's framebuffer
      \param width, height its size in device pixels
    */
    void end( QOpenGLFunctions_3_3_Core & gl,
              const GLuint fbo,
              const int width,
              const int height ) const;

    //! put the map on a texture unit, to be read by a sampler2DShadow
    void bind( QOpenGLFunctions_3_3_Core & gl,
               const int unit ) const;

private:
    GLuint M_texture = 0; //!< the depths
    GLuint M_fbo = 0; //!< the framebuffer that draws into them (0: none)
    int M_size = 0;
};

#endif
