// -*-c++-*-

/*!
  \file scene_3d.h
  \brief the scene of the 3D view, drawn with OpenGL 3.3 core. Header File.
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

#ifndef RCSSMONITOR_SCENE_3D_H
#define RCSSMONITOR_SCENE_3D_H

#include "camera_3d.h"
#include "scene_builder.h"

#include <QImage>
#include <QOpenGLFunctions_3_3_Core>

#include <memory>
#include <vector>

class QOpenGLShaderProgram;

/*!
  \class Scene3D
  \brief the scene of the 3D view, drawn with OpenGL 3.3 core and our own
  shaders (D1) into a framebuffer: that of a QOpenGLWidget, or an offscreen
  one for the tests. It knows nothing of the widget.
*/
class Scene3D
    : protected QOpenGLFunctions_3_3_Core {
public:

    Scene3D();
    ~Scene3D();

    Scene3D( const Scene3D & ) = delete;
    Scene3D & operator=( const Scene3D & ) = delete;

    /*!
      \brief create the resources in the current context.
      \return false if OpenGL 3.3 core is not available
    */
    bool initialize();

    //! delete the resources, in the current context
    void release();

    //! the image of the ground (GroundTexture::draw())
    void setGround( const QImage & image );

    /*!
      \brief draw the scene.
      \param fbo the framebuffer to draw into
      \param width, height its size in device pixels
      \param camera the camera
      \param content the players, the ball and the goals
    */
    void render( GLuint fbo,
                 int width,
                 int height,
                 const CameraPose & camera,
                 const SceneContent & content );

private:

    /*!
      \struct Mesh
      \brief a shape of unit size, drawn in instances
    */
    struct Mesh {
        GLuint vao = 0;
        GLuint vbo = 0; //!< the vertices: position and normal
        GLuint instance_vbo = 0; //!< the instances: the model matrix and the color
        GLsizei vertex_count = 0;
        std::size_t capacity = 0; //!< the instances that instance_vbo holds
    };

    bool M_initialized = false;
    std::unique_ptr< QOpenGLShaderProgram > M_lit_program;
    std::unique_ptr< QOpenGLShaderProgram > M_flat_program;
    Mesh M_cylinder;
    Mesh M_sphere;
    Mesh M_cone;
    Mesh M_disc;
    Mesh M_ring;
    Mesh M_decay_ring;
    Mesh M_quad;
    std::vector< float > M_instance_data; //!< reused to upload the instances

    std::unique_ptr< QOpenGLShaderProgram > M_ground_program;
    GLuint M_ground_vao = 0;
    GLuint M_ground_vbo = 0;
    GLuint M_ground_texture = 0;

    void createGround();
    void createMesh( Mesh & mesh,
                     const std::vector< float > & vertices );
    void releaseMesh( Mesh & mesh );
    void drawInstances( Mesh & mesh,
                        const std::vector< Instance3D > & instances );
};

#endif
