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
#include "gl/gpu_timer.h"
#include "gl/shadow_map.h"
#include "gl/skinned_mesh.h"
#include "figure_uniform.h"
#include "stands_3d.h"
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

    /*!
      \brief time the GPU's drawing of the frames (GpuTimer), or not
      (docs/PLAN_3D-VIEW.md §10.3)
    */
    void setTiming( const bool on );
    /*!
      \brief the GPU's times of the frames drawn since the last call (setTiming())
      \param total their sum (ms)
      \return how many frames
    */
    int takeGpuTime( double * total ) { return M_gpu_timer.take( total ); }

    //! send the image of the ground (GroundTexture::draw()) to the GPU
    void setGround( const QImage & image );

    //! send the image of the labels (LabelAtlas::image()) to the GPU
    void setLabels( const QImage & image );

    /*!
      \brief send the images of an end's boards and banner (Stadium3D) to the GPU
      \param end 0: left, 1: right
    */
    /*!
      \brief set the image of a stand (Stands3D::image())
      \param index the stand (Stands3D::stand())
    */
    void setStands( const int index,
                    const QImage & image );

    void setStadium( const int end,
                     const QImage & board,
                     const QImage & banner );

    //! the bodies of the human figures at most (Figures3D: the man's and the woman's)
    static constexpr int FIGURE_BODIES = 2;

    /*!
      \brief send a body of the human figures (Figures3D) to the GPU, for
      the figures of a SceneContent of that body (SceneContent::figure_bodies)
      \param body its index (0 to FIGURE_BODIES - 1); the bodies after it are
      deleted (send them in order)
      \param model the model
      \param paints how its parts are painted (Figures3D::paints())
      \param cuts where its uniform ends
    */
    void setFigureBody( const int body,
                        const GltfModel & model,
                        const std::vector< FigureUniform::Paint > & paints,
                        const FigureUniform::Cuts & cuts = FigureUniform::Cuts() );

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
    ShaderProgram M_panel_program; //!< "panel": the boards and the banners, an image on the front, unlit
    ShaderProgram M_skinned_program; //!< "skinned" and "lit": the human figures, lit by the sun
    ShaderProgram M_depth_program; //!< "depth": the solids as the sun sees them, into the shadow map
    ShaderProgram M_skinned_depth_program; //!< "skinned" and "depth": the figures into the shadow map
    ShaderProgram M_shadow_program; //!< "shadow": the plane of the shadows over the ground
    ShaderProgram M_blob_program; //!< "blob": the simple shadows, soft ellipses on the ground
    ShaderProgram M_sky_program; //!< "sky": the sky dome around the camera
    ShaderProgram M_outside_program; //!< "outside": the ground beyond the ground image, in the haze far away
    ShaderProgram M_stand_program; //!< "stand": the stands, their images lit by the sun

    // the shapes (Meshes3D), each drawn in instances
    InstancedMesh M_ground; //!< the area of the ground image
    InstancedMesh M_cylinder; //!< the bodies, the goal posts and crossbars
    InstancedMesh M_sphere; //!< the heads, the ball
    InstancedMesh M_cone; //!< the body and face directions
    InstancedMesh M_disc; //!< the circles under the players, the ball's shadow
    InstancedMesh M_ring; //!< the outline of a highlighted player
    InstancedMesh M_decay_ring; //!< the decay of effort or recovery
    InstancedMesh M_quad; //!< the nets, the floors of the goals
    InstancedMesh M_panel; //!< the boards and the banners
    InstancedMesh M_shadow_plane; //!< the area of the shadows, over the ground
    InstancedMesh M_far_ground; //!< the ground beyond the ground image (Scenery3D::FAR_GROUND around the center)
    InstancedMesh M_stands[Stands3D::STAND_COUNT]; //!< the stands (Stands3D)
    Texture2D M_stand_textures[Stands3D::STAND_COUNT]; //!< and their images
    std::vector< Instance3D > M_background; //!< the instance of the sky or of the ground beyond in a frame
    ShadowMap M_shadow_map; //!< the depths as the sun sees the solids (none if the GPU cannot)
    BillboardMesh M_billboards; //!< the numbers and the cards
    std::array< SkinnedMesh, FIGURE_BODIES > M_figures; //!< the bodies of the human figures of the players
    std::array< FigureUniform::Cuts, FIGURE_BODIES > M_figure_cuts; //!< where their uniforms end
    std::array< std::vector< SkinnedMesh::Instance >, FIGURE_BODIES > M_figure_instances; //!< the figures of each body in a frame
    std::array< std::vector< float >, FIGURE_BODIES > M_figure_joints; //!< and their skin matrices
    GpuTimer M_gpu_timer; //!< the GPU's time of a frame, when timed

    // made again in every frame (OverlayBuilder)
    VertexBatch M_sector_batch; //!< the view areas
    VertexBatch M_line_batch; //!< the lines on the ground
    VertexBatch M_dotted_batch; //!< the dotted lines on the ground
    VertexBatch M_point_batch; //!< the dots on the ground

    Texture2D M_ground_texture; //!< the image of the ground
    Texture2D M_grass_noise; //!< the noise of the realistic grass (Grass3D::noiseImage()), tiling both ways
    Texture2D M_label_texture; //!< the images of the labels
    Texture2D M_board_textures[2]; //!< a tile of the boards of each end (repeated)
    Texture2D M_banner_textures[2]; //!< the banner of each end
    const std::vector< Instance3D > M_ground_instance; //!< one instance of the ground, where it is (no move)
    const std::vector< Instance3D > M_shadow_instance; //!< one instance of the plane of the shadows, just over the ground

    //! sort the figures of a frame by their bodies (M_figure_instances, M_figure_joints)
    void sortFigures( const SceneContent & content );
    /*!
      \brief draw the figures sorted, with a skinned program bound
      \param program the main one, or the depth one for the shadow map
      \param uniform for the main one: the planes of each body's uniform,
      and the colors of its parts
    */
    void drawFigures( ShaderProgram & program,
                      const bool uniform );
};

#endif
