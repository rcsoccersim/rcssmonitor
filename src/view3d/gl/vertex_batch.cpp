// -*-c++-*-

/*!
  \file vertex_batch.cpp
  \brief lines, points and triangles of the 3D view, made in every frame. Source File.
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

#include "vertex_batch.h"

#include "attributes.h"

#include <algorithm>
#include <cstddef>

namespace {

//! a vertex in the buffer: position (3), color (4), distance (1)
constexpr int VERTEX_FLOATS = 3 + 4 + 1;

//! a position in a buffer, in bytes from its start, as OpenGL takes it
const void *
offset( const std::size_t bytes )
{
    return reinterpret_cast< const void * >( bytes );
}

}

/*-------------------------------------------------------------------*/
void
VertexBatch::create( QOpenGLFunctions_3_3_Core & gl )
{
    gl.glGenVertexArrays( 1, &M_vao );
    gl.glBindVertexArray( M_vao );

    // one buffer, filled in draw(): each vertex its position, color and distance
    gl.glGenBuffers( 1, &M_buffer );
    gl.glBindBuffer( GL_ARRAY_BUFFER, M_buffer );
    const GLsizei stride = VERTEX_FLOATS * sizeof( float );
    gl.glEnableVertexAttribArray( GLAttribute::POSITION );
    gl.glVertexAttribPointer( GLAttribute::POSITION, 3, GL_FLOAT, GL_FALSE, stride, offset( 0 ) );
    gl.glEnableVertexAttribArray( GLAttribute::COLOR );
    gl.glVertexAttribPointer( GLAttribute::COLOR, 4, GL_FLOAT, GL_FALSE, stride, offset( 3 * sizeof( float ) ) );
    gl.glEnableVertexAttribArray( GLAttribute::DISTANCE );
    gl.glVertexAttribPointer( GLAttribute::DISTANCE, 1, GL_FLOAT, GL_FALSE, stride, offset( 7 * sizeof( float ) ) );
    M_capacity = 0;

    gl.glBindVertexArray( 0 );
    gl.glBindBuffer( GL_ARRAY_BUFFER, 0 );
}

/*-------------------------------------------------------------------*/
void
VertexBatch::destroy( QOpenGLFunctions_3_3_Core & gl )
{
    gl.glDeleteVertexArrays( 1, &M_vao );
    gl.glDeleteBuffers( 1, &M_buffer );
    M_vao = M_buffer = 0;
    M_capacity = 0;
}

/*-------------------------------------------------------------------*/
void
VertexBatch::draw( QOpenGLFunctions_3_3_Core & gl,
                   const std::vector< ColorVertex3D > & vertices,
                   const Primitive primitive )
{
    if ( vertices.empty() )
    {
        return;
    }

    // lay the vertices out as the buffer holds them (VERTEX_FLOATS each)
    M_data.clear();
    for ( const ColorVertex3D & v : vertices )
    {
        const float values[VERTEX_FLOATS] = {
            v.position.x(), v.position.y(), v.position.z(),
            v.color.x(), v.color.y(), v.color.z(), v.color.w(),
            v.distance,
        };
        M_data.insert( M_data.end(), values, values + VERTEX_FLOATS );
    }

    // send them, making the buffer larger only when it is too small (as InstancedMesh)
    gl.glBindBuffer( GL_ARRAY_BUFFER, M_buffer );
    if ( vertices.size() > M_capacity )
    {
        M_capacity = std::max< std::size_t >( 1024, vertices.size() * 2 );
        gl.glBufferData( GL_ARRAY_BUFFER, GLsizeiptr( M_capacity * VERTEX_FLOATS * sizeof( float ) ), nullptr, GL_DYNAMIC_DRAW );
    }
    gl.glBufferSubData( GL_ARRAY_BUFFER, 0, GLsizeiptr( M_data.size() * sizeof( float ) ), M_data.data() );
    gl.glBindBuffer( GL_ARRAY_BUFFER, 0 );

    // draw them as the shapes asked for
    GLenum mode = GL_TRIANGLES;
    switch ( primitive ) {
    case Primitive::Lines:
        mode = GL_LINES;
        break;
    case Primitive::Points:
        // the size of the dots comes from the vertex shader (gl_PointSize)
        gl.glEnable( GL_PROGRAM_POINT_SIZE );
        mode = GL_POINTS;
        break;
    case Primitive::Triangles:
        break;
    }
    gl.glBindVertexArray( M_vao );
    gl.glDrawArrays( mode, 0, GLsizei( vertices.size() ) );
    gl.glBindVertexArray( 0 );
    if ( primitive == Primitive::Points )
    {
        gl.glDisable( GL_PROGRAM_POINT_SIZE );
    }
}
