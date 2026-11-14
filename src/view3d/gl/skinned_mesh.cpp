// -*-c++-*-

/*!
  \file skinned_mesh.cpp
  \brief a skinned mesh (a human figure), drawn in instances. Source File.
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

#include "skinned_mesh.h"

#include "attributes.h"

#include <QStringList>

#include <algorithm>
#include <cstddef>

namespace {

//! a position in a buffer, in bytes from its start, as OpenGL takes it
const void *
offset( const std::size_t bytes )
{
    return reinterpret_cast< const void * >( bytes );
}

//! the texture unit of the skin matrices while drawing
constexpr int JOINT_UNIT = 0;

}

/*-------------------------------------------------------------------*/
void
SkinnedMesh::create( QOpenGLFunctions_3_3_Core & gl,
                     const GltfModel & model,
                     const QStringList & jersey )
{
    M_joint_count = model.jointCount();
    M_parts.clear();
    for ( const SkinPrimitive & p : model.primitives )
    {
        M_parts.push_back( { p.first_index, p.index_count, jersey.contains( p.material ), p.base_color } );
    }

    gl.glGenVertexArrays( 1, &M_vao );
    gl.glBindVertexArray( M_vao );

    // the vertices, once. The joints are integers: glVertexAttribIPointer
    // passes them as they are (glVertexAttribPointer would make floats).
    gl.glGenBuffers( 1, &M_vertex_buffer );
    gl.glBindBuffer( GL_ARRAY_BUFFER, M_vertex_buffer );
    gl.glBufferData( GL_ARRAY_BUFFER, GLsizeiptr( model.vertices.size() * sizeof( SkinVertex ) ),
                     model.vertices.data(), GL_STATIC_DRAW );
    const GLsizei stride = sizeof( SkinVertex );
    gl.glEnableVertexAttribArray( GLAttribute::POSITION );
    gl.glVertexAttribPointer( GLAttribute::POSITION, 3, GL_FLOAT, GL_FALSE, stride, offset( offsetof( SkinVertex, position ) ) );
    gl.glEnableVertexAttribArray( GLAttribute::NORMAL );
    gl.glVertexAttribPointer( GLAttribute::NORMAL, 3, GL_FLOAT, GL_FALSE, stride, offset( offsetof( SkinVertex, normal ) ) );
    gl.glEnableVertexAttribArray( GLAttribute::JOINTS );
    gl.glVertexAttribIPointer( GLAttribute::JOINTS, 4, GL_UNSIGNED_SHORT, stride, offset( offsetof( SkinVertex, joints ) ) );
    gl.glEnableVertexAttribArray( GLAttribute::WEIGHTS );
    gl.glVertexAttribPointer( GLAttribute::WEIGHTS, 4, GL_FLOAT, GL_FALSE, stride, offset( offsetof( SkinVertex, weights ) ) );

    // the triangles: indices into the vertices (the VAO remembers this buffer too)
    gl.glGenBuffers( 1, &M_index_buffer );
    gl.glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, M_index_buffer );
    gl.glBufferData( GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr( model.indices.size() * sizeof( std::uint32_t ) ),
                     model.indices.data(), GL_STATIC_DRAW );

    // the color of each figure, an attribute of the instance (divisor 1)
    gl.glGenBuffers( 1, &M_instance_buffer );
    gl.glBindBuffer( GL_ARRAY_BUFFER, M_instance_buffer );
    gl.glEnableVertexAttribArray( GLAttribute::COLOR );
    gl.glVertexAttribPointer( GLAttribute::COLOR, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), offset( 0 ) );
    gl.glVertexAttribDivisor( GLAttribute::COLOR, 1 );

    gl.glBindVertexArray( 0 );
    gl.glBindBuffer( GL_ARRAY_BUFFER, 0 );
    gl.glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    // the texture of the skin matrices: made with its room in draw()
    gl.glGenTextures( 1, &M_joint_texture );
    gl.glBindTexture( GL_TEXTURE_2D, M_joint_texture );
    gl.glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    gl.glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    gl.glBindTexture( GL_TEXTURE_2D, 0 );
    M_capacity = 0;
}

/*-------------------------------------------------------------------*/
void
SkinnedMesh::destroy( QOpenGLFunctions_3_3_Core & gl )
{
    gl.glDeleteVertexArrays( 1, &M_vao );
    gl.glDeleteBuffers( 1, &M_vertex_buffer );
    gl.glDeleteBuffers( 1, &M_index_buffer );
    gl.glDeleteBuffers( 1, &M_instance_buffer );
    gl.glDeleteTextures( 1, &M_joint_texture );
    M_vao = M_vertex_buffer = M_index_buffer = M_instance_buffer = M_joint_texture = 0;
    M_capacity = 0;
    M_parts.clear();
}

/*-------------------------------------------------------------------*/
void
SkinnedMesh::draw( QOpenGLFunctions_3_3_Core & gl,
                   ShaderProgram & program,
                   const std::vector< QVector4D > & colors,
                   const std::vector< float > & joints )
{
    const std::size_t floats = static_cast< std::size_t >( M_joint_count ) * 16;
    if ( ! created()
         || colors.empty()
         || joints.size() < colors.size() * floats )
    {
        return;
    }
    const GLsizei count = GLsizei( colors.size() );

    // room for the figures: made larger (twice) only when too small
    // (docs/PLAN_3D-VIEW.md §5.3), then overwritten
    gl.glBindBuffer( GL_ARRAY_BUFFER, M_instance_buffer );
    gl.glBindTexture( GL_TEXTURE_2D, M_joint_texture );
    if ( colors.size() > M_capacity )
    {
        M_capacity = std::max< std::size_t >( 32, colors.size() * 2 );
        gl.glBufferData( GL_ARRAY_BUFFER, GLsizeiptr( M_capacity * 4 * sizeof( float ) ), nullptr, GL_DYNAMIC_DRAW );
        gl.glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, M_joint_count * 4, GLsizei( M_capacity ),
                         0, GL_RGBA, GL_FLOAT, nullptr );
    }
    gl.glBufferSubData( GL_ARRAY_BUFFER, 0, GLsizeiptr( colors.size() * 4 * sizeof( float ) ), colors.data() );
    gl.glBindBuffer( GL_ARRAY_BUFFER, 0 );
    // the rows of the figures: a joint's matrix is 4 texels of 4 floats, column by column
    gl.glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
    gl.glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, M_joint_count * 4, count, GL_RGBA, GL_FLOAT, joints.data() );

    gl.glActiveTexture( GL_TEXTURE0 + JOINT_UNIT );
    gl.glBindTexture( GL_TEXTURE_2D, M_joint_texture );
    program.set( "uJoints", JOINT_UNIT );

    // each part for all the figures at once
    gl.glBindVertexArray( M_vao );
    for ( const Part & p : M_parts )
    {
        program.set( "uInstanceColor", p.jersey ? 1 : 0 );
        program.set( "uBaseColor", p.color );
        gl.glDrawElementsInstanced( GL_TRIANGLES, p.index_count, GL_UNSIGNED_INT,
                                    offset( std::size_t( p.first_index ) * sizeof( std::uint32_t ) ), count );
    }
    gl.glBindVertexArray( 0 );
    gl.glBindTexture( GL_TEXTURE_2D, 0 );
}
