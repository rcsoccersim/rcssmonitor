// -*-c++-*-

/*!
  \file shadow_map.cpp
  \brief the shadow map of the sun. Source File.
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

#include "shadow_map.h"

/*-------------------------------------------------------------------*/
bool
ShadowMap::create( QOpenGLFunctions_3_3_Core & gl )
{
    destroy( gl );

    GLint max_size = 0;
    gl.glGetIntegerv( GL_MAX_TEXTURE_SIZE, &max_size );
    M_size = ( max_size >= 8192 ? 4096 : 2048 );

    // the depths: 24 bits a pixel, none at first
    gl.glGenTextures( 1, &M_texture );
    gl.glBindTexture( GL_TEXTURE_2D, M_texture );
    gl.glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, M_size, M_size, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr );
    // read by comparing (sampler2DShadow): lit where the depth given is not
    // beyond the depth in the map, the 4 pixels around mixed
    gl.glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    gl.glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    gl.glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
    gl.glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
    // outside the map, the depth of its border: "farthest", as cleared (lit)
    gl.glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
    gl.glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
    const GLfloat farthest[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    gl.glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, farthest );
    gl.glBindTexture( GL_TEXTURE_2D, 0 );

    // the framebuffer: the depths only, no colors to draw or read
    gl.glGenFramebuffers( 1, &M_fbo );
    gl.glBindFramebuffer( GL_FRAMEBUFFER, M_fbo );
    gl.glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, M_texture, 0 );
    gl.glDrawBuffer( GL_NONE );
    gl.glReadBuffer( GL_NONE );
    const bool complete = ( gl.glCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE );
    gl.glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    if ( ! complete )
    {
        destroy( gl );
        return false;
    }
    return true;
}

/*-------------------------------------------------------------------*/
void
ShadowMap::destroy( QOpenGLFunctions_3_3_Core & gl )
{
    if ( M_fbo != 0 )
    {
        gl.glDeleteFramebuffers( 1, &M_fbo );
        M_fbo = 0;
    }
    if ( M_texture != 0 )
    {
        gl.glDeleteTextures( 1, &M_texture );
        M_texture = 0;
    }
    M_size = 0;
}

/*-------------------------------------------------------------------*/
void
ShadowMap::begin( QOpenGLFunctions_3_3_Core & gl ) const
{
    gl.glBindFramebuffer( GL_FRAMEBUFFER, M_fbo );
    gl.glViewport( 0, 0, M_size, M_size );
    gl.glDepthMask( GL_TRUE ); // clearing obeys it
    gl.glClearDepth( 1.0 );
    gl.glClear( GL_DEPTH_BUFFER_BIT );
}

/*-------------------------------------------------------------------*/
void
ShadowMap::end( QOpenGLFunctions_3_3_Core & gl,
                const GLuint fbo,
                const int width,
                const int height ) const
{
    gl.glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    gl.glViewport( 0, 0, width, height );
}

/*-------------------------------------------------------------------*/
void
ShadowMap::bind( QOpenGLFunctions_3_3_Core & gl,
                 const int unit ) const
{
    gl.glActiveTexture( GLenum( GL_TEXTURE0 + unit ) );
    gl.glBindTexture( GL_TEXTURE_2D, M_texture );
}
