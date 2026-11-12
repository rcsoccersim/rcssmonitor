// -*-c++-*-

/*!
  \file frame.h
  \brief the start and the end of a frame of the 3D view. Header File.
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

#ifndef RCSSMONITOR_GL_FRAME_H
#define RCSSMONITOR_GL_FRAME_H

#include <QColor>
#include <QOpenGLFunctions_3_3_Core>

/*!
  \brief the start and the end of drawing a frame (Scene3D::render()).

  A frame is drawn into a "framebuffer": the image that the widget shows
  (QOpenGLWidget::defaultFramebufferObject()), or an offscreen one in the
  tests. OpenGL keeps its settings from frame to frame, and QPainter (the
  HUD drawn over the last frame) may have changed some, so a frame starts
  from a known state, and ends in the state that QPainter expects.
*/
namespace GLFrame {

/*!
  \brief start a frame: draw into the framebuffer, over all of it, and
  clear it to the color and the depth to "farthest". The clipping and the
  masks that QPainter may leave are turned off first, as clearing obeys
  them.
  \param gl the OpenGL functions
  \param fbo OpenGL's number of the framebuffer
  \param width, height its size in device pixels
  \param background the color it is cleared to
*/
void begin( QOpenGLFunctions_3_3_Core & gl,
            const GLuint fbo,
            const int width,
            const int height,
            const QColor & background );

/*!
  \brief end a frame: no shader program, shape or texture bound, and the
  render state as QPainter expects it (RenderState::painter()), so that the
  HUD can be drawn over the scene
*/
void end( QOpenGLFunctions_3_3_Core & gl );

}

#endif
