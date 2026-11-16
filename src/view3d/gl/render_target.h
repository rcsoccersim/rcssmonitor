// -*-c++-*-

/*!
  \file render_target.h
  \brief the framebuffer of smooth edges (multisampling) of the 3D view. Header File.
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

#ifndef RCSSMONITOR_GL_RENDER_TARGET_H
#define RCSSMONITOR_GL_RENDER_TARGET_H

#include <QOpenGLFunctions_3_3_Core>

/*!
  \class RenderTarget
  \brief where a frame of smooth edges is drawn (multisampling), before it
  goes to the view's framebuffer.

  With multisampling, the GPU keeps a few samples of color and depth in
  each pixel, and a triangle covers those of its own: at its edges, a pixel
  is partly the triangle's and partly the background's. resolve() averages
  each pixel's samples into the view's framebuffer (a "blit"), where the
  edges are then smooth instead of stairs.

  The view's own framebuffer (the window's) has no samples, so that this
  can be turned on and off as the frames go (the option antialias_3d),
  without the window made again.
*/
class RenderTarget {
public:

    RenderTarget() = default;
    RenderTarget( const RenderTarget & ) = delete;
    RenderTarget & operator=( const RenderTarget & ) = delete;

    /*!
      \brief make (or make again) the framebuffer for the size and the
      samples, if they changed: the color (8 bits each, with alpha) and the
      depth (24 bits, with a stencil of 8)
      \param width, height in device pixels
      \param samples per pixel: as many as the GPU takes, at most
      \return false if it cannot be drawn into (then there is none)
    */
    bool prepare( QOpenGLFunctions_3_3_Core & gl,
                  const int width,
                  const int height,
                  const int samples );

    //! delete it, in the current context
    void destroy( QOpenGLFunctions_3_3_Core & gl );

    bool created() const { return M_fbo != 0; }

    //! OpenGL's number of the framebuffer to draw a frame into (0: none)
    GLuint framebuffer() const { return M_fbo; }

    //! the samples per pixel it has
    int samples() const { return M_samples; }

    /*!
      \brief average each pixel's samples into another framebuffer, of the
      same size, and draw into that one from now
      \param to OpenGL's number of the framebuffer (the view's)
    */
    void resolve( QOpenGLFunctions_3_3_Core & gl,
                  const GLuint to ) const;

private:
    GLuint M_fbo = 0;
    GLuint M_color = 0; //!< the renderbuffers (images only to draw into, not textures)
    GLuint M_depth = 0;
    int M_width = 0;
    int M_height = 0;
    int M_samples = 0;
};

#endif
