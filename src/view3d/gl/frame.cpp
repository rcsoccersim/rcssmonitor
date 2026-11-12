// -*-c++-*-

/*!
  \file frame.cpp
  \brief the start and the end of a frame of the 3D view. Source File.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "frame.h"

#include "render_state.h"

namespace GLFrame {

/*-------------------------------------------------------------------*/
void
begin( QOpenGLFunctions_3_3_Core & gl,
       const GLuint fbo,
       const int width,
       const int height,
       const QColor & background )
{
    // where to draw: the framebuffer, all of it
    gl.glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    gl.glViewport( 0, 0, width, height );

    // what QPainter may leave: clipping to a rectangle or a stencil, the
    // writing of colors or depth turned off, another comparison of depth
    gl.glDisable( GL_SCISSOR_TEST );
    gl.glDisable( GL_STENCIL_TEST );
    gl.glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
    gl.glDepthMask( GL_TRUE );
    gl.glDepthFunc( GL_LESS );

    // clear: the background, and the depth to "farthest"
    gl.glClearColor( float( background.redF() ), float( background.greenF() ), float( background.blueF() ), 1.0f );
    gl.glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

/*-------------------------------------------------------------------*/
void
end( QOpenGLFunctions_3_3_Core & gl )
{
    gl.glUseProgram( 0 );
    gl.glBindVertexArray( 0 );
    gl.glActiveTexture( GL_TEXTURE0 );
    gl.glBindTexture( GL_TEXTURE_2D, 0 );
    RenderState::painter().apply( gl );
}

}
