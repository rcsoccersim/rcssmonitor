// -*-c++-*-

/*!
  \file render_state.cpp
  \brief how the 3D view draws: depth, culling and blending. Source File.
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

#include "render_state.h"

#include <QOpenGLFunctions_3_3_Core>

/*-------------------------------------------------------------------*/
void
RenderState::apply( QOpenGLFunctions_3_3_Core & gl ) const
{
    // the depth buffer (see the header)
    if ( depth_test ) gl.glEnable( GL_DEPTH_TEST ); else gl.glDisable( GL_DEPTH_TEST );
    gl.glDepthMask( depth_write ? GL_TRUE : GL_FALSE );

    // skip the triangles seen from behind
    if ( cull_back )
    {
        gl.glEnable( GL_CULL_FACE );
        gl.glCullFace( GL_BACK );
    }
    else
    {
        gl.glDisable( GL_CULL_FACE );
    }

    // mix with what is there by the alpha
    if ( blend )
    {
        gl.glEnable( GL_BLEND );
        // the color: a * new + (1 - a) * old. The alpha of the framebuffer
        // (not the color) stays opaque, so that the window behind does not
        // show through.
        gl.glBlendFuncSeparate( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
    }
    else
    {
        gl.glDisable( GL_BLEND );
    }
}
