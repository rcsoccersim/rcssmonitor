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
//! and of the textures of the materials (1, 2: the shadow map, the grass's noise)
constexpr int TEXTURE_UNIT = 3;
//! the mean luminance of a texture is taken as at least this (its shades not too strong)
constexpr float MIN_LUMINANCE = 0.05f;

}

/*-------------------------------------------------------------------*/
void
SkinnedMesh::create( QOpenGLFunctions_3_3_Core & gl,
                     const GltfModel & model,
                     const std::vector< FigureUniform::Paint > & paints )
{
    M_joint_count = model.jointCount();
    M_parts.clear();
    M_textured = false;
    M_count = 0;
    // the images of the materials, once; repeated (as glTF's sampler by default)
    M_textures.clear();
    for ( const QImage & image : model.images )
    {
        M_textures.emplace_back( new Texture2D );
        if ( ! image.isNull() ) M_textures.back()->upload( gl, image, true, true );
    }
    for ( std::size_t i = 0; i < model.primitives.size(); ++i )
    {
        const SkinPrimitive & p = model.primitives[i];
        const FigureUniform::Paint paint = ( i < paints.size() ? paints[i] : FigureUniform::Paint() );
        const int texture = ( p.image >= 0 && p.image < static_cast< int >( M_textures.size() ) ? p.image : -1 );
        const float luminance = ( texture >= 0 ? model.image_luminance[texture] : 1.0f );
        M_parts.push_back( { p.first_index, p.index_count, paint.kind, int( paint.show ), p.base_color,
                             texture,
                             ( p.alpha == SkinPrimitive::Opaque ? -1.0f : p.alpha_cutoff ),
                             1.0f / std::max( MIN_LUMINANCE, luminance ),
                             p.double_sided } );
        if ( M_parts.back().textured() ) M_textured = true;
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
    gl.glEnableVertexAttribArray( GLAttribute::REST );
    gl.glVertexAttribPointer( GLAttribute::REST, 3, GL_FLOAT, GL_FALSE, stride, offset( offsetof( SkinVertex, rest ) ) );
    gl.glEnableVertexAttribArray( GLAttribute::PART );
    gl.glVertexAttribPointer( GLAttribute::PART, 2, GL_FLOAT, GL_FALSE, stride, offset( offsetof( SkinVertex, part ) ) );
    gl.glEnableVertexAttribArray( GLAttribute::SCALP );
    gl.glVertexAttribPointer( GLAttribute::SCALP, 4, GL_FLOAT, GL_FALSE, stride, offset( offsetof( SkinVertex, scalp ) ) );

    // where in the textures, in a buffer of their own: only the parts with a
    // texture read them, and the vertices stay as small (faster to draw)
    gl.glGenBuffers( 1, &M_uv_buffer );
    gl.glBindBuffer( GL_ARRAY_BUFFER, M_uv_buffer );
    std::vector< float > uvs = model.uvs;
    uvs.resize( model.vertices.size() * 2, 0.0f );
    gl.glBufferData( GL_ARRAY_BUFFER, GLsizeiptr( uvs.size() * sizeof( float ) ), uvs.data(), GL_STATIC_DRAW );
    gl.glEnableVertexAttribArray( GLAttribute::UV );
    gl.glVertexAttribPointer( GLAttribute::UV, 2, GL_FLOAT, GL_FALSE, 2 * sizeof( float ), offset( 0 ) );

    // the triangles: indices into the vertices (the VAO remembers this buffer too)
    gl.glGenBuffers( 1, &M_index_buffer );
    gl.glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, M_index_buffer );
    gl.glBufferData( GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr( model.indices.size() * sizeof( std::uint32_t ) ),
                     model.indices.data(), GL_STATIC_DRAW );

    // the colors of each figure, attributes of the instance (divisor 1)
    gl.glGenBuffers( 1, &M_instance_buffer );
    gl.glBindBuffer( GL_ARRAY_BUFFER, M_instance_buffer );
    const GLsizei instance_stride = sizeof( Instance );
    gl.glEnableVertexAttribArray( GLAttribute::COLOR );
    gl.glVertexAttribPointer( GLAttribute::COLOR, 4, GL_FLOAT, GL_FALSE, instance_stride, offset( offsetof( Instance, color ) ) );
    gl.glVertexAttribDivisor( GLAttribute::COLOR, 1 );
    gl.glEnableVertexAttribArray( GLAttribute::SKIN );
    gl.glVertexAttribPointer( GLAttribute::SKIN, 4, GL_FLOAT, GL_FALSE, instance_stride, offset( offsetof( Instance, skin ) ) );
    gl.glVertexAttribDivisor( GLAttribute::SKIN, 1 );
    gl.glEnableVertexAttribArray( GLAttribute::HAIR );
    gl.glVertexAttribPointer( GLAttribute::HAIR, 4, GL_FLOAT, GL_FALSE, instance_stride, offset( offsetof( Instance, hair ) ) );
    gl.glVertexAttribDivisor( GLAttribute::HAIR, 1 );

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
    gl.glDeleteBuffers( 1, &M_uv_buffer );
    gl.glDeleteBuffers( 1, &M_index_buffer );
    gl.glDeleteBuffers( 1, &M_instance_buffer );
    gl.glDeleteTextures( 1, &M_joint_texture );
    for ( std::unique_ptr< Texture2D > & t : M_textures ) t->destroy( gl );
    M_textures.clear();
    M_vao = M_vertex_buffer = M_uv_buffer = M_index_buffer = M_instance_buffer = M_joint_texture = 0;
    M_capacity = 0;
    M_parts.clear();
}

/*-------------------------------------------------------------------*/
void
SkinnedMesh::setFigures( QOpenGLFunctions_3_3_Core & gl,
                         const std::vector< Instance > & figures,
                         const std::vector< float > & joints )
{
    M_count = 0;
    const std::size_t floats = static_cast< std::size_t >( M_joint_count ) * 16;
    if ( ! created()
         || figures.empty()
         || joints.size() < figures.size() * floats )
    {
        return;
    }
    M_count = GLsizei( figures.size() );
    // room for the figures: made larger (twice) only when too small
    // (docs/plan/2026-11-11_3d-view.md §5.3), then overwritten
    gl.glBindBuffer( GL_ARRAY_BUFFER, M_instance_buffer );
    gl.glActiveTexture( GL_TEXTURE0 + JOINT_UNIT );
    gl.glBindTexture( GL_TEXTURE_2D, M_joint_texture );
    if ( figures.size() > M_capacity )
    {
        M_capacity = std::max< std::size_t >( 32, figures.size() * 2 );
        gl.glBufferData( GL_ARRAY_BUFFER, GLsizeiptr( M_capacity * sizeof( Instance ) ), nullptr, GL_DYNAMIC_DRAW );
        gl.glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, M_joint_count * 4, GLsizei( M_capacity ),
                         0, GL_RGBA, GL_FLOAT, nullptr );
    }
    gl.glBufferSubData( GL_ARRAY_BUFFER, 0, GLsizeiptr( figures.size() * sizeof( Instance ) ), figures.data() );
    gl.glBindBuffer( GL_ARRAY_BUFFER, 0 );
    // the rows of the figures: a joint's matrix is 4 texels of 4 floats, column by column
    gl.glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
    gl.glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, M_joint_count * 4, M_count, GL_RGBA, GL_FLOAT, joints.data() );
    gl.glBindTexture( GL_TEXTURE_2D, 0 );
}

/*-------------------------------------------------------------------*/
void
SkinnedMesh::draw( QOpenGLFunctions_3_3_Core & gl,
                   ShaderProgram & program,
                   const bool textured,
                   const bool colors )
{
    if ( ! created() || M_count == 0 ) return;
    gl.glActiveTexture( GL_TEXTURE0 + JOINT_UNIT );
    gl.glBindTexture( GL_TEXTURE_2D, M_joint_texture );
    program.set( "uJoints", JOINT_UNIT );
    if ( textured ) program.set( "uTexture", TEXTURE_UNIT );
    // each part for all the figures at once: a double-sided one without
    // culling (then the culling as the caller had it)
    const bool culling = gl.glIsEnabled( GL_CULL_FACE );
    gl.glBindVertexArray( M_vao );
    for ( const Part & p : M_parts )
    {
        if ( p.textured() != textured ) continue;
        program.set( "uPaint", p.paint );
        program.set( "uShow", p.show );
        if ( colors ) program.set( "uBaseColor", p.color );
        if ( textured )
        {
            program.set( "uTextured", p.texture >= 0 ? 1 : 0 );
            program.set( "uAlphaCutoff", p.alpha_cutoff );
            program.set( "uLumScale", p.lum_scale );
            if ( p.texture >= 0 ) M_textures[p.texture]->bind( gl, TEXTURE_UNIT );
        }
        if ( culling )
        {
            if ( p.two_sided ) gl.glDisable( GL_CULL_FACE );
            else gl.glEnable( GL_CULL_FACE );
        }
        gl.glDrawElementsInstanced( GL_TRIANGLES, p.index_count, GL_UNSIGNED_INT,
                                    offset( std::size_t( p.first_index ) * sizeof( std::uint32_t ) ), M_count );
    }
    if ( culling ) gl.glEnable( GL_CULL_FACE );
    gl.glBindVertexArray( 0 );
    gl.glActiveTexture( GL_TEXTURE0 + TEXTURE_UNIT );
    gl.glBindTexture( GL_TEXTURE_2D, 0 );
    gl.glActiveTexture( GL_TEXTURE0 + JOINT_UNIT );
    gl.glBindTexture( GL_TEXTURE_2D, 0 );
}
