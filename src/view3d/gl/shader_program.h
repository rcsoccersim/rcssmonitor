// -*-c++-*-

/*!
  \file shader_program.h
  \brief a shader program of the 3D view. Header File.
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

#ifndef RCSSMONITOR_GL_SHADER_PROGRAM_H
#define RCSSMONITOR_GL_SHADER_PROGRAM_H

#include <QOpenGLShaderProgram>
#include <QString>

#include <memory>
#include <string>
#include <unordered_map>

/*!
  \class ShaderProgram
  \brief a pair of shaders, the small programs that the GPU runs to draw.

  - The vertex shader (<name>.vert) runs for every vertex: it moves the
    vertex from the shape's own space to the screen (with the model matrix
    of the instance and the camera's matrix), and passes values (normal,
    color, ...) on to the fragment shader.
  - The fragment shader (<name>.frag) runs for every pixel that a triangle
    covers, with those values interpolated across the triangle, and decides
    the pixel's color.

  The sources are files in the resources (src/view3d/shaders, listed in
  view3d_shaders.qrc) without the "#version" line: load() adds it, and the
  locations of the attributes (GLAttribute) as A_POSITION, ..., A_COLOR,
  A_BILLBOARD_CENTER, ...
  The two shaders are compiled and "linked" into one program, which is
  chosen with bind() before drawing.

  A "uniform" is an input of a shader that is the same for everything in
  one draw call, e.g. the camera's matrix or the direction of the sun. It
  is set by its name with set() after bind(). OpenGL finds a uniform by a
  number ("location"), looked up by name; set() looks it up once and
  remembers it.

  Every function needs the OpenGL context (QOpenGLContext) current, as in
  Scene3D::initialize() and render().
*/
class ShaderProgram {
public:

    ShaderProgram();
    ~ShaderProgram();

    ShaderProgram( const ShaderProgram & ) = delete;
    ShaderProgram & operator=( const ShaderProgram & ) = delete;

    /*!
      \brief compile and link :/shaders/<name>.vert and :/shaders/<name>.frag
      in the current context.
      \return false on an error, which is printed with the line of the shader
    */
    bool load( const QString & name );

    /*!
      \brief compile and link :/shaders/<vertex>.vert and
      :/shaders/<fragment>.frag (two programs may share a shader)
      \return false on an error, which is printed with the line of the shader
    */
    bool load( const QString & vertex,
               const QString & fragment );

    //! delete the program, in the current context
    void destroy();

    //! use this program for the draw calls that follow
    void bind();

    //! use no program
    void release();

    /*!
      \brief set a uniform of the bound program.
      \param name its name in the shader, e.g. "uViewProj"
      \param value a value that QOpenGLShaderProgram::setUniformValue() takes:
      QMatrix4x4 for mat4, QVector3D for vec3, int for a sampler2D (the
      texture unit), float, ...
    */
    template < typename T >
    void set( const char * name,
              const T & value )
      {
          M_program->setUniformValue( location( name ), value );
      }

private:
    std::unique_ptr< QOpenGLShaderProgram > M_program; //!< Qt compiles and links the shaders
    std::unordered_map< std::string, int > M_locations; //!< the locations of the uniforms by name

    //! the location of a uniform, looked up at the first time (-1 if the shader has none)
    int location( const char * name );
};

#endif
