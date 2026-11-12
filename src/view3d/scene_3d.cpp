// -*-c++-*-

/*!
  \file scene_3d.cpp
  \brief the scene of the 3D view, drawn with OpenGL 3.3 core. Source File.
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

#include "scene_3d.h"

#include "ground_texture.h"

#include <QOpenGLShaderProgram>

#include <QVector3D>

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

// the sky, until it is drawn (Phase 8)
const float SKY[] = { 0.55f, 0.72f, 0.88f };

const char * GROUND_VS = R"(#version 330 core
layout( location = 0 ) in vec3 aPos;
layout( location = 1 ) in vec2 aUV;
uniform mat4 uViewProj;
out vec2 vUV;
void main()
{
    vUV = aUV;
    gl_Position = uViewProj * vec4( aPos, 1.0 );
}
)";

const char * GROUND_FS = R"(#version 330 core
in vec2 vUV;
uniform sampler2D uTexture;
out vec4 fragColor;
void main()
{
    fragColor = vec4( texture( uTexture, vUV ).rgb, 1.0 );
}
)";

// the sun: from the far left, 44 degrees up (rcssmonitor-web, §4.3)
const QVector3D SUN = QVector3D( -60.0f, 65.0f, -30.0f ).normalized();

// the shapes in instances: the vertices in the model space, and the model
// matrix (4 columns) and the color of each instance
const char * LIT_VS = R"(#version 330 core
layout( location = 0 ) in vec3 aPos;
layout( location = 1 ) in vec3 aNormal;
layout( location = 2 ) in mat4 iModel;
layout( location = 6 ) in vec4 iColor;
uniform mat4 uViewProj;
out vec3 vNormal;
out vec4 vColor;
void main()
{
    vNormal = transpose( inverse( mat3( iModel ) ) ) * aNormal;
    vColor = iColor;
    gl_Position = uViewProj * iModel * vec4( aPos, 1.0 );
}
)";

// Lambert with the sun and a light from the sky
const char * LIT_FS = R"(#version 330 core
in vec3 vNormal;
in vec4 vColor;
uniform vec3 uSun;
out vec4 fragColor;
void main()
{
    vec3 n = normalize( vNormal );
    float light = 0.42 + 0.22 * ( 0.5 + 0.5 * n.y ) + 0.5 * max( dot( n, uSun ), 0.0 );
    fragColor = vec4( min( vColor.rgb * light, vec3( 1.0 ) ), vColor.a );
}
)";

const char * FLAT_VS = R"(#version 330 core
layout( location = 0 ) in vec3 aPos;
layout( location = 2 ) in mat4 iModel;
layout( location = 6 ) in vec4 iColor;
uniform mat4 uViewProj;
out vec4 vColor;
void main()
{
    vColor = iColor;
    gl_Position = uViewProj * iModel * vec4( aPos, 1.0 );
}
)";

const char * FLAT_FS = R"(#version 330 core
in vec4 vColor;
out vec4 fragColor;
void main()
{
    fragColor = vColor;
}
)";

//
// the shapes of unit size: triangles of (position, normal)
//

/*!
  \brief add a triangle, turned counterclockwise as seen from the side
  that its normals face (the front, which culling keeps)
*/
void
add_triangle( std::vector< float > & v,
              QVector3D a, QVector3D na,
              QVector3D b, QVector3D nb,
              QVector3D c, QVector3D nc )
{
    if ( QVector3D::dotProduct( QVector3D::crossProduct( b - a, c - a ), na + nb + nc ) < 0.0f )
    {
        std::swap( b, c );
        std::swap( nb, nc );
    }
    for ( const QVector3D & p : { a, na, b, nb, c, nc } )
    {
        v.push_back( p.x() );
        v.push_back( p.y() );
        v.push_back( p.z() );
    }
}

QVector3D
around_y( const double angle,
          const float r,
          const float y )
{
    return QVector3D( r * float( std::cos( angle ) ), y, r * float( std::sin( angle ) ) );
}

//! radius 1, height 1 along y, centered, with its ends
std::vector< float >
cylinder_vertices( const int segments )
{
    std::vector< float > v;
    const QVector3D up( 0.0f, 1.0f, 0.0f ), down( 0.0f, -1.0f, 0.0f );
    for ( int i = 0; i < segments; ++i )
    {
        const double a0 = 2.0 * M_PI * i / segments, a1 = 2.0 * M_PI * ( i + 1 ) / segments;
        const QVector3D b0 = around_y( a0, 1.0f, -0.5f ), b1 = around_y( a1, 1.0f, -0.5f );
        const QVector3D t0 = around_y( a0, 1.0f, 0.5f ), t1 = around_y( a1, 1.0f, 0.5f );
        const QVector3D n0 = around_y( a0, 1.0f, 0.0f ), n1 = around_y( a1, 1.0f, 0.0f );
        add_triangle( v, b0, n0, b1, n1, t1, n1 );
        add_triangle( v, b0, n0, t1, n1, t0, n0 );
        add_triangle( v, QVector3D( 0.0f, 0.5f, 0.0f ), up, t0, up, t1, up );
        add_triangle( v, QVector3D( 0.0f, -0.5f, 0.0f ), down, b0, down, b1, down );
    }
    return v;
}

//! radius 1
std::vector< float >
sphere_vertices( const int segments,
                 const int rings )
{
    std::vector< float > v;
    auto at = [&]( const int i, const int j ) {
        const double a = 2.0 * M_PI * i / segments;
        const double b = M_PI * j / rings - M_PI / 2.0;
        return around_y( a, float( std::cos( b ) ), float( std::sin( b ) ) );
    };
    for ( int j = 0; j < rings; ++j )
    {
        for ( int i = 0; i < segments; ++i )
        {
            const QVector3D p00 = at( i, j ), p10 = at( i + 1, j ), p01 = at( i, j + 1 ), p11 = at( i + 1, j + 1 );
            if ( j > 0 ) add_triangle( v, p00, p00, p10, p10, p11, p11 );
            if ( j < rings - 1 ) add_triangle( v, p00, p00, p11, p11, p01, p01 );
        }
    }
    return v;
}

//! the base of radius 1 on the yz plane at the origin, the tip at x = 1
std::vector< float >
cone_vertices( const int segments )
{
    std::vector< float > v;
    const QVector3D tip( 1.0f, 0.0f, 0.0f ), back( -1.0f, 0.0f, 0.0f );
    auto rim = [&]( const int i ) {
        const double a = 2.0 * M_PI * i / segments;
        return QVector3D( 0.0f, float( std::cos( a ) ), float( std::sin( a ) ) );
    };
    for ( int i = 0; i < segments; ++i )
    {
        const QVector3D r0 = rim( i ), r1 = rim( i + 1 );
        // the side leans 45 degrees
        const QVector3D n0 = ( r0 + tip ).normalized(), n1 = ( r1 + tip ).normalized();
        add_triangle( v, r0, n0, r1, n1, tip, ( n0 + n1 ).normalized() );
        add_triangle( v, QVector3D(), back, r0, back, r1, back );
    }
    return v;
}

//! from inner to outer on the xz plane, facing up (a disc if inner is 0)
std::vector< float >
ring_vertices( const float inner,
               const float outer,
               const int segments )
{
    std::vector< float > v;
    const QVector3D up( 0.0f, 1.0f, 0.0f );
    for ( int i = 0; i < segments; ++i )
    {
        const double a0 = 2.0 * M_PI * i / segments, a1 = 2.0 * M_PI * ( i + 1 ) / segments;
        const QVector3D o0 = around_y( a0, outer, 0.0f ), o1 = around_y( a1, outer, 0.0f );
        const QVector3D i0 = around_y( a0, inner, 0.0f ), i1 = around_y( a1, inner, 0.0f );
        add_triangle( v, i0, up, o0, up, o1, up );
        if ( inner > 0.0f )
        {
            add_triangle( v, i0, up, o1, up, i1, up );
        }
    }
    return v;
}

//! 1 x 1 on the xy plane, centered, facing +z
std::vector< float >
quad_vertices()
{
    std::vector< float > v;
    const QVector3D n( 0.0f, 0.0f, 1.0f );
    const QVector3D a( -0.5f, -0.5f, 0.0f ), b( 0.5f, -0.5f, 0.0f ), c( 0.5f, 0.5f, 0.0f ), d( -0.5f, 0.5f, 0.0f );
    add_triangle( v, a, n, b, n, c, n );
    add_triangle( v, a, n, c, n, d, n );
    return v;
}

std::unique_ptr< QOpenGLShaderProgram >
make_program( const char * vs,
              const char * fs )
{
    std::unique_ptr< QOpenGLShaderProgram > p( new QOpenGLShaderProgram );
    if ( ! p->addShaderFromSourceCode( QOpenGLShader::Vertex, vs )
         || ! p->addShaderFromSourceCode( QOpenGLShader::Fragment, fs )
         || ! p->link() )
    {
        std::cerr << "(Scene3D) shader error: " << p->log().toStdString() << std::endl;
        return nullptr;
    }
    return p;
}

}

/*-------------------------------------------------------------------*/
Scene3D::Scene3D() = default;

/*-------------------------------------------------------------------*/
Scene3D::~Scene3D() = default;

/*-------------------------------------------------------------------*/
bool
Scene3D::initialize()
{
    if ( M_initialized )
    {
        return true;
    }
    if ( ! initializeOpenGLFunctions() )
    {
        return false;
    }

    M_ground_program = make_program( GROUND_VS, GROUND_FS );
    M_lit_program = make_program( LIT_VS, LIT_FS );
    M_flat_program = make_program( FLAT_VS, FLAT_FS );
    if ( ! M_ground_program
         || ! M_lit_program
         || ! M_flat_program )
    {
        M_ground_program.reset();
        M_lit_program.reset();
        M_flat_program.reset();
        return false;
    }
    createGround();
    createMesh( M_cylinder, cylinder_vertices( 20 ) );
    createMesh( M_sphere, sphere_vertices( 24, 16 ) );
    createMesh( M_cone, cone_vertices( 12 ) );
    createMesh( M_disc, ring_vertices( 0.0f, 1.0f, 32 ) );
    createMesh( M_ring, ring_vertices( 0.92f, 1.08f, 40 ) );
    createMesh( M_decay_ring, ring_vertices( 1.1f, 1.22f, 40 ) );
    createMesh( M_quad, quad_vertices() );
    M_initialized = true;
    return true;
}

/*-------------------------------------------------------------------*/
void
Scene3D::release()
{
    if ( ! M_initialized )
    {
        return;
    }
    glDeleteVertexArrays( 1, &M_ground_vao );
    glDeleteBuffers( 1, &M_ground_vbo );
    glDeleteTextures( 1, &M_ground_texture );
    M_ground_vao = M_ground_vbo = M_ground_texture = 0;
    for ( Mesh * m : { &M_cylinder, &M_sphere, &M_cone, &M_disc, &M_ring, &M_decay_ring, &M_quad } )
    {
        releaseMesh( *m );
    }
    M_ground_program.reset();
    M_lit_program.reset();
    M_flat_program.reset();
    M_initialized = false;
}

/*-------------------------------------------------------------------*/
void
Scene3D::createMesh( Mesh & mesh,
                     const std::vector< float > & vertices )
{
    mesh.vertex_count = GLsizei( vertices.size() / 6 );
    glGenVertexArrays( 1, &mesh.vao );
    glBindVertexArray( mesh.vao );

    glGenBuffers( 1, &mesh.vbo );
    glBindBuffer( GL_ARRAY_BUFFER, mesh.vbo );
    glBufferData( GL_ARRAY_BUFFER, GLsizeiptr( vertices.size() * sizeof( float ) ), vertices.data(), GL_STATIC_DRAW );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( float ), nullptr );
    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( float ), reinterpret_cast< void * >( 3 * sizeof( float ) ) );

    // the instances: 4 columns of the model matrix and the color
    glGenBuffers( 1, &mesh.instance_vbo );
    glBindBuffer( GL_ARRAY_BUFFER, mesh.instance_vbo );
    const GLsizei stride = 20 * sizeof( float );
    for ( int i = 0; i < 5; ++i )
    {
        const GLuint location = GLuint( 2 + i );
        glEnableVertexAttribArray( location );
        glVertexAttribPointer( location, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast< void * >( i * 4 * sizeof( float ) ) );
        glVertexAttribDivisor( location, 1 );
    }
    mesh.capacity = 0;

    glBindVertexArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
}

/*-------------------------------------------------------------------*/
void
Scene3D::releaseMesh( Mesh & mesh )
{
    glDeleteVertexArrays( 1, &mesh.vao );
    glDeleteBuffers( 1, &mesh.vbo );
    glDeleteBuffers( 1, &mesh.instance_vbo );
    mesh = Mesh();
}

/*-------------------------------------------------------------------*/
void
Scene3D::drawInstances( Mesh & mesh,
                        const std::vector< Instance3D > & instances )
{
    if ( instances.empty() )
    {
        return;
    }

    M_instance_data.clear();
    for ( const Instance3D & i : instances )
    {
        const float * m = i.model.constData(); // column major
        M_instance_data.insert( M_instance_data.end(), m, m + 16 );
        M_instance_data.push_back( i.color.x() );
        M_instance_data.push_back( i.color.y() );
        M_instance_data.push_back( i.color.z() );
        M_instance_data.push_back( i.color.w() );
    }

    // the buffer grows, and is reused while it is large enough (§5.3)
    glBindBuffer( GL_ARRAY_BUFFER, mesh.instance_vbo );
    const GLsizeiptr size = GLsizeiptr( M_instance_data.size() * sizeof( float ) );
    if ( instances.size() > mesh.capacity )
    {
        mesh.capacity = std::max< std::size_t >( 64, instances.size() * 2 );
        glBufferData( GL_ARRAY_BUFFER, GLsizeiptr( mesh.capacity * 20 * sizeof( float ) ), nullptr, GL_DYNAMIC_DRAW );
    }
    glBufferSubData( GL_ARRAY_BUFFER, 0, size, M_instance_data.data() );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    glBindVertexArray( mesh.vao );
    glDrawArraysInstanced( GL_TRIANGLES, 0, mesh.vertex_count, GLsizei( instances.size() ) );
}

/*-------------------------------------------------------------------*/
void
Scene3D::createGround()
{
    // the area of the image, at y = 0. The image rows go along +z (the
    // field's +y), and the triangles turn counterclockwise seen from above.
    const float x0 = float( -GroundTexture::length() / 2 ), x1 = -x0;
    const float z0 = float( -GroundTexture::width() / 2 ), z1 = -z0;
    const float v[] = { x0, 0, z0, 0, 0,  x1, 0, z1, 1, 1,  x1, 0, z0, 1, 0,
                        x0, 0, z0, 0, 0,  x0, 0, z1, 0, 1,  x1, 0, z1, 1, 1 };
    glGenVertexArrays( 1, &M_ground_vao );
    glBindVertexArray( M_ground_vao );
    glGenBuffers( 1, &M_ground_vbo );
    glBindBuffer( GL_ARRAY_BUFFER, M_ground_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( v ), v, GL_STATIC_DRAW );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof( float ), nullptr );
    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof( float ), reinterpret_cast< void * >( 3 * sizeof( float ) ) );
    glBindVertexArray( 0 );

    glGenTextures( 1, &M_ground_texture );
}

/*-------------------------------------------------------------------*/
void
Scene3D::setGround( const QImage & image )
{
    const QImage rgba = image.convertToFormat( QImage::Format_RGBA8888 );
    glBindTexture( GL_TEXTURE_2D, M_ground_texture );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, rgba.width(), rgba.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.constBits() );
    glGenerateMipmap( GL_TEXTURE_2D );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    // anisotropic filtering is an extension in OpenGL 3.3, but everywhere
    GLfloat max_aniso = 0.0f;
    glGetFloatv( 0x84FF /* GL_MAX_TEXTURE_MAX_ANISOTROPY */, &max_aniso );
    if ( max_aniso > 1.0f )
    {
        glTexParameterf( GL_TEXTURE_2D, 0x84FE /* GL_TEXTURE_MAX_ANISOTROPY */, std::min( max_aniso, 8.0f ) );
    }
    while ( glGetError() != GL_NO_ERROR ) { }
    glBindTexture( GL_TEXTURE_2D, 0 );
}

/*-------------------------------------------------------------------*/
void
Scene3D::render( GLuint fbo,
                 int width,
                 int height,
                 const CameraPose & camera,
                 const SceneContent & content )
{
    if ( ! M_initialized )
    {
        return;
    }

    const QMatrix4x4 view_proj = camera.projection( float( width ) / std::max( 1, height ) ) * camera.view();

    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glViewport( 0, 0, width, height );
    glClearColor( SKY[0], SKY[1], SKY[2], 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    M_ground_program->bind();
    M_ground_program->setUniformValue( "uViewProj", view_proj );
    M_ground_program->setUniformValue( "uTexture", 0 );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, M_ground_texture );
    glBindVertexArray( M_ground_vao );
    glDrawArrays( GL_TRIANGLES, 0, 6 );
    glBindTexture( GL_TEXTURE_2D, 0 );
    M_ground_program->release();

    // the solids, lit by the sun
    M_lit_program->bind();
    M_lit_program->setUniformValue( "uViewProj", view_proj );
    M_lit_program->setUniformValue( "uSun", SUN );
    drawInstances( M_cylinder, content.cylinders );
    drawInstances( M_sphere, content.spheres );
    drawInstances( M_cone, content.cones );
    M_lit_program->release();

    // the translucent ones: the layers on the ground from low to high, and
    // the nets, seen from both sides. They do not hide what comes after.
    glEnable( GL_BLEND );
    glBlendFuncSeparate( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
    glDepthMask( GL_FALSE );
    glDisable( GL_CULL_FACE );
    M_flat_program->bind();
    M_flat_program->setUniformValue( "uViewProj", view_proj );
    drawInstances( M_disc, content.discs );
    drawInstances( M_ring, content.rings );
    drawInstances( M_decay_ring, content.decay_rings );
    drawInstances( M_quad, content.quads );
    M_flat_program->release();

    // leave the state as QPainter expects it
    glBindVertexArray( 0 );
    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );
    glDisable( GL_DEPTH_TEST );
}
