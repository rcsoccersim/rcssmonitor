// -*-c++-*-

/*!
  \file billboard_mesh.cpp
  \brief the billboards of the 3D view. Source File.
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

#include "billboard_mesh.h"

#include "attributes.h"
#include "meshes_3d.h"

#include <algorithm>
#include <cstddef>

namespace {

//! a billboard in the buffer: center (3), size and offset (4), part of the texture (4), color (4)
constexpr int INSTANCE_FLOATS = 3 + 4 + 4 + 4;

//! a position in a buffer, in bytes from its start, as OpenGL takes it
const void *
offset( const std::size_t bytes )
{
    return reinterpret_cast< const void * >( bytes );
}

}

/*-------------------------------------------------------------------*/
void
BillboardMesh::create( QOpenGLFunctions_3_3_Core & gl )
{
    const std::vector< Vertex3D > vertices = Meshes3D::quad();
    M_vertex_count = GLsizei( vertices.size() );

    gl.glGenVertexArrays( 1, &M_vao );
    gl.glBindVertexArray( M_vao );

    // the square: its corners (-0.5 to 0.5) and where they are on its part of the texture (0 to 1)
    gl.glGenBuffers( 1, &M_vertex_buffer );
    gl.glBindBuffer( GL_ARRAY_BUFFER, M_vertex_buffer );
    gl.glBufferData( GL_ARRAY_BUFFER, GLsizeiptr( vertices.size() * sizeof( Vertex3D ) ), vertices.data(), GL_STATIC_DRAW );
    const GLsizei stride = sizeof( Vertex3D );
    gl.glEnableVertexAttribArray( GLAttribute::POSITION );
    gl.glVertexAttribPointer( GLAttribute::POSITION, 3, GL_FLOAT, GL_FALSE, stride, offset( offsetof( Vertex3D, position ) ) );
    gl.glEnableVertexAttribArray( GLAttribute::UV );
    gl.glVertexAttribPointer( GLAttribute::UV, 2, GL_FLOAT, GL_FALSE, stride, offset( offsetof( Vertex3D, uv ) ) );

    // the billboards, one value per instance (divisor 1), filled in draw()
    gl.glGenBuffers( 1, &M_instance_buffer );
    gl.glBindBuffer( GL_ARRAY_BUFFER, M_instance_buffer );
    const GLsizei instance_stride = INSTANCE_FLOATS * sizeof( float );
    const struct { GLuint location; GLint size; int first; } attributes[] = {
        { GLAttribute::BILLBOARD_CENTER, 3, 0 },
        { GLAttribute::BILLBOARD_SIZE, 4, 3 },
        { GLAttribute::BILLBOARD_UV, 4, 7 },
        { GLAttribute::COLOR, 4, 11 },
    };
    for ( const auto & a : attributes )
    {
        gl.glEnableVertexAttribArray( a.location );
        gl.glVertexAttribPointer( a.location, a.size, GL_FLOAT, GL_FALSE, instance_stride, offset( a.first * sizeof( float ) ) );
        gl.glVertexAttribDivisor( a.location, 1 );
    }
    M_capacity = 0;

    gl.glBindVertexArray( 0 );
    gl.glBindBuffer( GL_ARRAY_BUFFER, 0 );
}

/*-------------------------------------------------------------------*/
void
BillboardMesh::destroy( QOpenGLFunctions_3_3_Core & gl )
{
    gl.glDeleteVertexArrays( 1, &M_vao );
    gl.glDeleteBuffers( 1, &M_vertex_buffer );
    gl.glDeleteBuffers( 1, &M_instance_buffer );
    M_vao = M_vertex_buffer = M_instance_buffer = 0;
    M_vertex_count = 0;
    M_capacity = 0;
}

/*-------------------------------------------------------------------*/
void
BillboardMesh::draw( QOpenGLFunctions_3_3_Core & gl,
                     const std::vector< Billboard3D > & billboards )
{
    if ( billboards.empty() )
    {
        return;
    }

    // lay the billboards out as the buffer holds them (INSTANCE_FLOATS each)
    M_instance_data.clear();
    for ( const Billboard3D & b : billboards )
    {
        const float values[INSTANCE_FLOATS] = {
            b.center.x(), b.center.y(), b.center.z(),
            b.size.x(), b.size.y(), b.offset.x(), b.offset.y(),
            float( b.uv.left() ), float( b.uv.top() ), float( b.uv.right() ), float( b.uv.bottom() ),
            b.color.x(), b.color.y(), b.color.z(), b.color.w(),
        };
        M_instance_data.insert( M_instance_data.end(), values, values + INSTANCE_FLOATS );
    }

    // send them, making the buffer larger only when it is too small (as InstancedMesh)
    gl.glBindBuffer( GL_ARRAY_BUFFER, M_instance_buffer );
    if ( billboards.size() > M_capacity )
    {
        M_capacity = std::max< std::size_t >( 64, billboards.size() * 2 );
        gl.glBufferData( GL_ARRAY_BUFFER, GLsizeiptr( M_capacity * INSTANCE_FLOATS * sizeof( float ) ), nullptr, GL_DYNAMIC_DRAW );
    }
    gl.glBufferSubData( GL_ARRAY_BUFFER, 0, GLsizeiptr( M_instance_data.size() * sizeof( float ) ), M_instance_data.data() );
    gl.glBindBuffer( GL_ARRAY_BUFFER, 0 );

    // all of them, in their order (far to near): a later one is drawn over an earlier one
    gl.glBindVertexArray( M_vao );
    gl.glDrawArraysInstanced( GL_TRIANGLES, 0, M_vertex_count, GLsizei( billboards.size() ) );
    gl.glBindVertexArray( 0 );
}
