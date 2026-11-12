// -*-c++-*-

/*!
  \file instanced_mesh.cpp
  \brief a shape of the 3D view, drawn in instances. Source File.
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

#include "instanced_mesh.h"

#include "attributes.h"

#include <algorithm>
#include <cstddef>

namespace {

//! an instance in the buffer: the model matrix (16 floats, column by column) and the color (4 floats)
constexpr int INSTANCE_FLOATS = 16 + 4;

/*!
  \brief a position in a buffer, in bytes from its start, as OpenGL takes it
  (a pointer, for historical reasons)
*/
const void *
offset( const std::size_t bytes )
{
    return reinterpret_cast< const void * >( bytes );
}

}

/*-------------------------------------------------------------------*/
void
InstancedMesh::create( QOpenGLFunctions_3_3_Core & gl,
                       const std::vector< Vertex3D > & vertices )
{
    M_vertex_count = GLsizei( vertices.size() );

    // the VAO records the settings of the attributes below, until it is unbound
    gl.glGenVertexArrays( 1, &M_vao );
    gl.glBindVertexArray( M_vao );

    //
    // the vertices: send them once (GL_STATIC_DRAW: they do not change), and
    // tell where each attribute is in a Vertex3D. The "stride" is the size of
    // one vertex: the attribute of the next vertex is that many bytes later.
    //
    gl.glGenBuffers( 1, &M_vertex_buffer );
    gl.glBindBuffer( GL_ARRAY_BUFFER, M_vertex_buffer );
    gl.glBufferData( GL_ARRAY_BUFFER, GLsizeiptr( vertices.size() * sizeof( Vertex3D ) ), vertices.data(), GL_STATIC_DRAW );
    const GLsizei stride = sizeof( Vertex3D );
    gl.glEnableVertexAttribArray( GLAttribute::POSITION );
    gl.glVertexAttribPointer( GLAttribute::POSITION, 3, GL_FLOAT, GL_FALSE, stride, offset( offsetof( Vertex3D, position ) ) );
    gl.glEnableVertexAttribArray( GLAttribute::NORMAL );
    gl.glVertexAttribPointer( GLAttribute::NORMAL, 3, GL_FLOAT, GL_FALSE, stride, offset( offsetof( Vertex3D, normal ) ) );
    gl.glEnableVertexAttribArray( GLAttribute::UV );
    gl.glVertexAttribPointer( GLAttribute::UV, 2, GL_FLOAT, GL_FALSE, stride, offset( offsetof( Vertex3D, uv ) ) );

    //
    // the instances: the buffer is filled in draw(). A mat4 attribute takes
    // 4 locations, one per column of 4 floats. The divisor 1 makes these
    // attributes step to the next instance once per instance (not per
    // vertex): every vertex of a copy of the shape reads the same matrix
    // and color.
    //
    gl.glGenBuffers( 1, &M_instance_buffer );
    gl.glBindBuffer( GL_ARRAY_BUFFER, M_instance_buffer );
    const GLsizei instance_stride = INSTANCE_FLOATS * sizeof( float );
    for ( GLuint column = 0; column < 4; ++column )
    {
        const GLuint location = GLAttribute::MODEL + column;
        gl.glEnableVertexAttribArray( location );
        gl.glVertexAttribPointer( location, 4, GL_FLOAT, GL_FALSE, instance_stride, offset( column * 4 * sizeof( float ) ) );
        gl.glVertexAttribDivisor( location, 1 );
    }
    gl.glEnableVertexAttribArray( GLAttribute::COLOR );
    gl.glVertexAttribPointer( GLAttribute::COLOR, 4, GL_FLOAT, GL_FALSE, instance_stride, offset( 16 * sizeof( float ) ) );
    gl.glVertexAttribDivisor( GLAttribute::COLOR, 1 );
    M_capacity = 0; // no room yet: draw() makes it

    // unbind, so that later calls do not change this VAO by mistake
    gl.glBindVertexArray( 0 );
    gl.glBindBuffer( GL_ARRAY_BUFFER, 0 );
}

/*-------------------------------------------------------------------*/
void
InstancedMesh::destroy( QOpenGLFunctions_3_3_Core & gl )
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
InstancedMesh::draw( QOpenGLFunctions_3_3_Core & gl,
                     const std::vector< Instance3D > & instances )
{
    if ( instances.empty() )
    {
        return;
    }

    //
    // lay the instances out as the buffer holds them (INSTANCE_FLOATS each).
    // QMatrix4x4 keeps its numbers column by column, as GLSL reads a mat4.
    //
    M_instance_data.clear();
    for ( const Instance3D & i : instances )
    {
        const float * m = i.model.constData();
        M_instance_data.insert( M_instance_data.end(), m, m + 16 );
        M_instance_data.push_back( i.color.x() );
        M_instance_data.push_back( i.color.y() );
        M_instance_data.push_back( i.color.z() );
        M_instance_data.push_back( i.color.w() );
    }

    //
    // send them to the instance buffer. Making a buffer is slow, and making
    // one in every frame once leaked GPU memory in the web version, so the
    // buffer is made larger (twice what is needed) only when it is too small,
    // and is overwritten (glBufferSubData) otherwise (docs/PLAN_3D-VIEW.md §5.3).
    //
    gl.glBindBuffer( GL_ARRAY_BUFFER, M_instance_buffer );
    if ( instances.size() > M_capacity )
    {
        M_capacity = std::max< std::size_t >( 64, instances.size() * 2 );
        gl.glBufferData( GL_ARRAY_BUFFER, GLsizeiptr( M_capacity * INSTANCE_FLOATS * sizeof( float ) ), nullptr, GL_DYNAMIC_DRAW );
    }
    gl.glBufferSubData( GL_ARRAY_BUFFER, 0, GLsizeiptr( M_instance_data.size() * sizeof( float ) ), M_instance_data.data() );
    gl.glBindBuffer( GL_ARRAY_BUFFER, 0 );

    // draw all the copies: the triangles of the shape, once per instance
    gl.glBindVertexArray( M_vao );
    gl.glDrawArraysInstanced( GL_TRIANGLES, 0, M_vertex_count, GLsizei( instances.size() ) );
    gl.glBindVertexArray( 0 );
}
