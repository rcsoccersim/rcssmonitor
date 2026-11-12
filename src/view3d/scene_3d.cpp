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

#include <algorithm>
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
    if ( ! M_ground_program )
    {
        return false;
    }
    createGround();
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
    M_ground_program.reset();
    M_initialized = false;
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
                 const CameraPose & camera )
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

    // leave the state as QPainter expects it
    glBindVertexArray( 0 );
    glBindTexture( GL_TEXTURE_2D, 0 );
    M_ground_program->release();
    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
}
