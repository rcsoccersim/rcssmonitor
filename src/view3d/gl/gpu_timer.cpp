// -*-c++-*-

/*!
  \file gpu_timer.cpp
  \brief the time the GPU takes to draw a frame of the 3D view. Source File.
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

#include "gpu_timer.h"

/*-------------------------------------------------------------------*/
void
GpuTimer::create( QOpenGLFunctions_3_3_Core & gl )
{
    if ( created() ) return;
    gl.glGenQueries( RING, M_queries );
    for ( bool & p : M_pending ) p = false;
    M_next = 0;
    M_running = false;
}

/*-------------------------------------------------------------------*/
void
GpuTimer::destroy( QOpenGLFunctions_3_3_Core & gl )
{
    if ( ! created() ) return;
    if ( M_running ) gl.glEndQuery( GL_TIME_ELAPSED );
    gl.glDeleteQueries( RING, M_queries );
    for ( int i = 0; i < RING; ++i )
    {
        M_queries[i] = 0;
        M_pending[i] = false;
    }
    M_running = false;
}

/*-------------------------------------------------------------------*/
void
GpuTimer::begin( QOpenGLFunctions_3_3_Core & gl )
{
    if ( ! created() || M_running ) return;

    // the queries done since: their times (ns), without waiting for the others
    for ( int i = 0; i < RING; ++i )
    {
        if ( ! M_pending[i] ) continue;
        GLint available = 0;
        gl.glGetQueryObjectiv( M_queries[i], GL_QUERY_RESULT_AVAILABLE, &available );
        if ( available )
        {
            GLuint64 ns = 0;
            gl.glGetQueryObjectui64v( M_queries[i], GL_QUERY_RESULT, &ns );
            M_total += double( ns ) * 1.0e-6;
            ++M_count;
            M_pending[i] = false;
        }
    }
    // a query still pending (the GPU far behind): this frame is not timed
    if ( M_pending[M_next] ) return;

    gl.glBeginQuery( GL_TIME_ELAPSED, M_queries[M_next] );
    M_running = true;
}

/*-------------------------------------------------------------------*/
void
GpuTimer::end( QOpenGLFunctions_3_3_Core & gl )
{
    if ( ! M_running ) return;
    gl.glEndQuery( GL_TIME_ELAPSED );
    M_pending[M_next] = true;
    M_next = ( M_next + 1 ) % RING;
    M_running = false;
}

/*-------------------------------------------------------------------*/
int
GpuTimer::take( double * total )
{
    const int count = M_count;
    *total = M_total;
    M_total = 0.0;
    M_count = 0;
    return count;
}
