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
#include "scene_content.h"
#include "gl/billboard_mesh.h"
#include "gl/instanced_mesh.h"
#include "gl/shader_program.h"
#include "gl/texture_2d.h"
#include "gl/vertex_batch.h"

#include <QImage>
#include <QOpenGLFunctions_3_3_Core>

#include <vector>

/*!
  \class Scene3D
  \brief the scene of the 3D view, drawn with OpenGL 3.3 core and our own
  shaders (D1) into a framebuffer: that of a QOpenGLWidget, or an offscreen
  one for the tests. It knows nothing of the widget.

  How a frame is drawn:
  - SceneBuilder decides what to draw where (SceneContent: lists of
    instances by shape). It knows nothing of OpenGL.
  - render() draws in "passes". A pass applies a RenderState (how to draw:
    depth, culling, translucency), binds a ShaderProgram (what the GPU
    computes for each vertex and pixel), sets its uniforms (the camera, the
    sun), and draws the lists of instances, one InstancedMesh per shape.

  The OpenGL calls are in the parts in src/view3d/gl; this class only
  combines them. To draw something new: add its shape (Meshes3D) and an
  InstancedMesh here, its list to SceneContent, where it goes to
  SceneBuilder, and its draw() to the pass of the right shader and render
  state in render() (or a new pass with a new shader in src/view3d/shaders).

  OpenGL works on the "current context", which belongs to one window or
  offscreen surface: initialize(), release(), setGround() and render() must
  be called with it current (QOpenGLWidget makes it current around
  initializeGL() and paintGL()).

  It inherits QOpenGLFunctions_3_3_Core, Qt's table of the OpenGL 3.3
  functions, and passes itself to the parts as "gl".
*/
class Scene3D
    : protected QOpenGLFunctions_3_3_Core {
public:

    Scene3D();
    ~Scene3D();

    Scene3D( const Scene3D & ) = delete;
    Scene3D & operator=( const Scene3D & ) = delete;

    /*!
      \brief compile the shaders and send the shapes to the GPU, in the
      current context.
      \return false if OpenGL 3.3 core is not available or a shader fails
    */
    bool initialize();

    //! delete what initialize() made, in the current context
    void release();

    //! send the image of the ground (GroundTexture::draw()) to the GPU
    void setGround( const QImage & image );

    //! send the image of the labels (LabelAtlas::image()) to the GPU
    void setLabels( const QImage & image );

    /*!
      \brief draw the scene.
      \param fbo the framebuffer to draw into: OpenGL's number of the
      widget's (QOpenGLWidget::defaultFramebufferObject()) or of an offscreen one
      \param width, height its size in device pixels (physical pixels of the screen)
      \param camera the camera
      \param content what to draw where (SceneBuilder)
    */
    void render( GLuint fbo,
                 int width,
                 int height,
                 const CameraPose & camera,
                 const SceneContent & content );

private:
    bool M_initialized = false;

    // the shader programs (src/view3d/shaders/<name>.vert and .frag)
    ShaderProgram M_ground_program; //!< "ground": the image of the ground, unlit
    ShaderProgram M_lit_program; //!< "lit": the solids, lit by the sun
    ShaderProgram M_flat_program; //!< "flat": the flat colors, translucent
    ShaderProgram M_billboard_program; //!< "billboard": the labels, facing the camera
    ShaderProgram M_colored_program; //!< "colored": the lines, dots and view areas, each vertex its color

    // the shapes (Meshes3D), each drawn in instances
    InstancedMesh M_ground; //!< the area of the ground image
    InstancedMesh M_cylinder; //!< the bodies, the goal posts and crossbars
    InstancedMesh M_sphere; //!< the heads, the ball
    InstancedMesh M_cone; //!< the body and face directions
    InstancedMesh M_disc; //!< the circles under the players, the ball's shadow
    InstancedMesh M_ring; //!< the outline of a highlighted player
    InstancedMesh M_decay_ring; //!< the decay of effort or recovery
    InstancedMesh M_quad; //!< the nets, the floors of the goals
    BillboardMesh M_billboards; //!< the numbers and the cards

    // made again in every frame (OverlayBuilder)
    VertexBatch M_sector_batch; //!< the view areas
    VertexBatch M_line_batch; //!< the lines on the ground
    VertexBatch M_dotted_batch; //!< the dotted lines on the ground
    VertexBatch M_point_batch; //!< the dots on the ground

    Texture2D M_ground_texture; //!< the image of the ground
    Texture2D M_label_texture; //!< the images of the labels
    const std::vector< Instance3D > M_ground_instance; //!< one instance of the ground, where it is (no move)
};

#endif
