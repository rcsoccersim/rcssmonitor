// -*-c++-*-

/*!
  \file render_target.cpp
  \brief the framebuffer of smooth edges (multisampling) of the 3D view. Source File.
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

#include "render_target.h"

#include <algorithm>

/*-------------------------------------------------------------------*/
bool
RenderTarget::prepare( QOpenGLFunctions_3_3_Core & gl,
                       const int width,
                       const int height,
                       const int samples )
{
    GLint most = 0;
    gl.glGetIntegerv( GL_MAX_SAMPLES, &most );
    const int count = std::max( 1, std::min( samples, int( most ) ) );
    if ( created()
         && width == M_width
         && height == M_height
         && count == M_samples )
    {
        return true;
    }
    destroy( gl );
    if ( width <= 0 || height <= 0 ) return false;

    gl.glGenRenderbuffers( 1, &M_color );
    gl.glBindRenderbuffer( GL_RENDERBUFFER, M_color );
    gl.glRenderbufferStorageMultisample( GL_RENDERBUFFER, count, GL_RGBA8, width, height );
    gl.glGenRenderbuffers( 1, &M_depth );
    gl.glBindRenderbuffer( GL_RENDERBUFFER, M_depth );
    gl.glRenderbufferStorageMultisample( GL_RENDERBUFFER, count, GL_DEPTH24_STENCIL8, width, height );
    gl.glBindRenderbuffer( GL_RENDERBUFFER, 0 );

    gl.glGenFramebuffers( 1, &M_fbo );
    gl.glBindFramebuffer( GL_FRAMEBUFFER, M_fbo );
    gl.glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, M_color );
    gl.glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, M_depth );
    const bool complete = ( gl.glCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE );
    gl.glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    if ( ! complete )
    {
        destroy( gl );
        return false;
    }
    M_width = width;
    M_height = height;
    M_samples = count;
    return true;
}

/*-------------------------------------------------------------------*/
void
RenderTarget::destroy( QOpenGLFunctions_3_3_Core & gl )
{
    if ( M_fbo ) gl.glDeleteFramebuffers( 1, &M_fbo );
    if ( M_color ) gl.glDeleteRenderbuffers( 1, &M_color );
    if ( M_depth ) gl.glDeleteRenderbuffers( 1, &M_depth );
    M_fbo = M_color = M_depth = 0;
    M_width = M_height = M_samples = 0;
}

/*-------------------------------------------------------------------*/
void
RenderTarget::resolve( QOpenGLFunctions_3_3_Core & gl,
                       const GLuint to ) const
{
    if ( ! created() ) return;
    // the colors only (QPainter draws the HUD over them, without depth)
    gl.glBindFramebuffer( GL_READ_FRAMEBUFFER, M_fbo );
    gl.glBindFramebuffer( GL_DRAW_FRAMEBUFFER, to );
    gl.glBlitFramebuffer( 0, 0, M_width, M_height, 0, 0, M_width, M_height, GL_COLOR_BUFFER_BIT, GL_NEAREST );
    gl.glBindFramebuffer( GL_FRAMEBUFFER, to );
}
