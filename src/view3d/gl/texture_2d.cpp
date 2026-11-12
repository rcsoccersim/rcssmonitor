// -*-c++-*-

/*!
  \file texture_2d.cpp
  \brief a texture of the 3D view. Source File.
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

#include "texture_2d.h"

#include <QImage>

#include <algorithm>

namespace {

// anisotropic filtering is an extension in OpenGL 3.3 (standard from 4.6),
// but available everywhere: its constants are not in the 3.3 headers
constexpr GLenum TEXTURE_MAX_ANISOTROPY = 0x84FE;
constexpr GLenum MAX_TEXTURE_MAX_ANISOTROPY = 0x84FF;
constexpr float MAX_ANISOTROPY = 8.0f;

}

/*-------------------------------------------------------------------*/
void
Texture2D::upload( QOpenGLFunctions_3_3_Core & gl,
                   const QImage & image )
{
    if ( M_id == 0 )
    {
        gl.glGenTextures( 1, &M_id );
    }

    // send the pixels: 4 bytes (red, green, blue, alpha) each, row by row
    const QImage rgba = image.convertToFormat( QImage::Format_RGBA8888 );
    gl.glBindTexture( GL_TEXTURE_2D, M_id );
    gl.glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
    gl.glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, rgba.width(), rgba.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.constBits() );

    // "mipmaps": the image halved again and again. Far away, a pixel of the
    // screen covers many pixels of the image; reading a smaller copy then
    // gives their average instead of flickering lines.
    gl.glGenerateMipmap( GL_TEXTURE_2D );
    gl.glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    gl.glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    // outside 0 to 1, the color of the edge (not the image repeated)
    gl.glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    gl.glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    // anisotropic filtering: the ground seen at a low angle stays sharp
    // along the view, instead of blurring as the mipmaps alone would
    GLfloat max_anisotropy = 0.0f;
    gl.glGetFloatv( MAX_TEXTURE_MAX_ANISOTROPY, &max_anisotropy );
    if ( max_anisotropy > 1.0f )
    {
        gl.glTexParameterf( GL_TEXTURE_2D, TEXTURE_MAX_ANISOTROPY, std::min( max_anisotropy, MAX_ANISOTROPY ) );
    }

    // without the extension, the lines above leave errors: clear them, so
    // that they are not taken for errors of later calls
    while ( gl.glGetError() != GL_NO_ERROR ) { }
    gl.glBindTexture( GL_TEXTURE_2D, 0 );
}

/*-------------------------------------------------------------------*/
void
Texture2D::bind( QOpenGLFunctions_3_3_Core & gl,
                 const int unit ) const
{
    gl.glActiveTexture( GLenum( GL_TEXTURE0 + unit ) );
    gl.glBindTexture( GL_TEXTURE_2D, M_id );
}

/*-------------------------------------------------------------------*/
void
Texture2D::destroy( QOpenGLFunctions_3_3_Core & gl )
{
    gl.glDeleteTextures( 1, &M_id );
    M_id = 0;
}
