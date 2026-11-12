// -*-c++-*-

/*!
  \file shader_program.cpp
  \brief a shader program of the 3D view. Source File.
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

#include "shader_program.h"

#include "attributes.h"

#include <QFile>

#include <iostream>

/*!
  \brief register the resources of the shaders (view3d_shaders.qrc).

  Qt's resources are compiled into the program. They are in the static
  library rcssmonitor_core, and the linker leaves out what nothing refers
  to: Q_INIT_RESOURCE refers to them, so that they are linked and
  registered. It must be called outside any namespace.
*/
static void
init_shader_resources()
{
    Q_INIT_RESOURCE( view3d_shaders );
}

namespace {

/*!
  \brief the first lines of every shader: the version of GLSL (OpenGL 3.3
  core), and the locations of the attributes (gl/attributes.h)
*/
QByteArray
shader_header()
{
    return QByteArray( "#version 330 core\n" )
        + "#define A_POSITION " + QByteArray::number( GLAttribute::POSITION ) + "\n"
        + "#define A_NORMAL " + QByteArray::number( GLAttribute::NORMAL ) + "\n"
        + "#define A_UV " + QByteArray::number( GLAttribute::UV ) + "\n"
        + "#define A_MODEL " + QByteArray::number( GLAttribute::MODEL ) + "\n"
        + "#define A_COLOR " + QByteArray::number( GLAttribute::COLOR ) + "\n"
        + "#define A_BILLBOARD_CENTER " + QByteArray::number( GLAttribute::BILLBOARD_CENTER ) + "\n"
        + "#define A_BILLBOARD_SIZE " + QByteArray::number( GLAttribute::BILLBOARD_SIZE ) + "\n"
        + "#define A_BILLBOARD_UV " + QByteArray::number( GLAttribute::BILLBOARD_UV ) + "\n";
}

/*!
  \brief read a shader from the resources and compile it into the program.
  \return false on an error, which is printed (the compiler's message has
  the line numbers, counted with the lines of shader_header())
*/
bool
add_shader( QOpenGLShaderProgram & program,
            const QOpenGLShader::ShaderType type,
            const QString & path )
{
    QFile file( path );
    if ( ! file.open( QIODevice::ReadOnly ) )
    {
        std::cerr << "(ShaderProgram) cannot read " << path.toStdString() << std::endl;
        return false;
    }
    if ( ! program.addShaderFromSourceCode( type, shader_header() + file.readAll() ) )
    {
        std::cerr << "(ShaderProgram) " << path.toStdString() << ": "
                  << program.log().toStdString() << std::endl;
        return false;
    }
    return true;
}

}

/*-------------------------------------------------------------------*/
ShaderProgram::ShaderProgram() = default;

/*-------------------------------------------------------------------*/
ShaderProgram::~ShaderProgram() = default;

/*-------------------------------------------------------------------*/
bool
ShaderProgram::load( const QString & name )
{
    static bool resources = false;
    if ( ! resources )
    {
        init_shader_resources();
        resources = true;
    }

    // compile the two shaders, and link them into one program: the outputs
    // of the vertex shader ("out") meet the inputs of the fragment shader
    // ("in") of the same names
    std::unique_ptr< QOpenGLShaderProgram > program( new QOpenGLShaderProgram );
    if ( ! add_shader( *program, QOpenGLShader::Vertex, ":/shaders/" + name + ".vert" )
         || ! add_shader( *program, QOpenGLShader::Fragment, ":/shaders/" + name + ".frag" ) )
    {
        return false;
    }
    if ( ! program->link() )
    {
        std::cerr << "(ShaderProgram) " << name.toStdString() << ": "
                  << program->log().toStdString() << std::endl;
        return false;
    }
    M_program = std::move( program );
    M_locations.clear();
    return true;
}

/*-------------------------------------------------------------------*/
void
ShaderProgram::destroy()
{
    M_program.reset();
    M_locations.clear();
}

/*-------------------------------------------------------------------*/
void
ShaderProgram::bind()
{
    M_program->bind();
}

/*-------------------------------------------------------------------*/
void
ShaderProgram::release()
{
    M_program->release();
}

/*-------------------------------------------------------------------*/
int
ShaderProgram::location( const char * name )
{
    const auto it = M_locations.find( name );
    if ( it != M_locations.end() )
    {
        return it->second;
    }

    // the first time: ask OpenGL. A uniform that the shader does not use
    // may be removed by the compiler, and has no location (-1): setting it
    // does nothing.
    const int loc = M_program->uniformLocation( name );
    if ( loc < 0 )
    {
        std::cerr << "(ShaderProgram) no uniform " << name << std::endl;
    }
    M_locations.emplace( name, loc );
    return loc;
}
