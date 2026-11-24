// -*-c++-*-

/*!
  \file view_3d_renderer.h
  \brief what the 3D view draws in a frame, apart from its window. Header File.
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

#ifndef RCSSMONITOR_VIEW_3D_RENDERER_H
#define RCSSMONITOR_VIEW_3D_RENDERER_H

#include "cameras_3d.h"
#include "frame_builder_3d.h"
#include "interpolate.h"
#include "label_atlas.h"
#include "referee_target.h"
#include "scene_3d.h"
#include "scene_content.h"

#include <QImage>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <QString>

#include <cstddef>
#include <memory>

class DispHolder;
class FieldPainter;
class QPainter;
class ScoreBoardPainter;
class TeamGraphicPainter;

/*!
  \class View3DRenderer
  \brief what the 3D view draws in a frame (the scene and the HUD), apart
  from the window, its clocks, its camera and its mouse, which FieldView3D
  keeps (docs/plan/2026-11-22-03_frame-export.md 4.2). An export draws the
  frames with one of its own, in a context of its own.

  It keeps what is made once and reused: the GPU's resources (Scene3D),
  the human figures and their motions (FrameBuilder3D), the texts of the
  labels (LabelAtlas), the images of the ground, the ends and the stands,
  and the 2D painters of the HUD. A frame is drawn in four steps:
  - build(): what to draw at a position between two frames, and what the
    cameras follow (subject()).
  - the caller moves its camera (CameraController::update()).
  - render(): the labels and the aids of the ball sized for the camera,
    then the scene into a framebuffer.
  - drawHud(): the score board, the team logos, the texts of the overlays
    and the guides, with a QPainter over the scene.

  initialize(), release(), build() and render() must be called with the
  OpenGL context current (Scene3D).
*/
class View3DRenderer {
public:

    explicit
    View3DRenderer( DispHolder & disp_holder );
    ~View3DRenderer();

    View3DRenderer( const View3DRenderer & ) = delete;
    View3DRenderer & operator=( const View3DRenderer & ) = delete;

    /*!
      \brief make the GPU's resources in the current context (a new context
      has none: the images are sent again), and read the human figures once
      \return false if OpenGL 3.3 core is not available (nothing is drawn)
    */
    bool initialize();

    //! delete the GPU's resources (the context current)
    void release();

    //! initialize() succeeded
    bool ready() const { return M_ready; }

    //! take the GPU's times of the frames (Scene3D::setTiming())
    void setTiming( const bool on ) { M_scene.setTiming( on ); }

    //! the GPU's times since the last call (Scene3D::takeGpuTime())
    int takeGpuTime( double * total ) { return M_scene.takeGpuTime( total ); }

    /*!
      \brief what to draw at a position between two frames: the images of
      the ground, the ends and the stands when they changed, the automatic
      selection of a player, the goals, the players (posed as figures) and
      the ball on the way, the overlays.
      \param at where between the frames (Interpolate)
      \param live a live game
      \param delay_frames the delayed live view's frames behind the newest
      \param mode the camera, for what is drawn of the players
    */
    void build( const Interpolate::Position & at,
                const bool live,
                const std::size_t delay_frames,
                const CameraMode mode );

    //! what the cameras follow, as the last build() found it
    const CameraSubject & subject() const { return M_builder.subject(); }

    /*!
      \brief draw the scene of the last build() into a framebuffer: first
      the number labels, the cards and the aids of the ball, sized for the
      camera and the view
      \param fbo the framebuffer
      \param camera where the camera is
      \param logical the size of the view (logical pixels)
      \param dpr the device pixel ratio: the framebuffer is logical × dpr
      \param samples of the smooth edges (0: none)
    */
    void render( const GLuint fbo,
                 const CameraPose & camera,
                 const QSize & logical,
                 const qreal dpr,
                 const int samples );

    /*!
      \brief draw the HUD over the scene, as the 2D view draws it: the
      score board, the team logos, the texts of the overlays and the guides
      (the compass and the minimap)
      \param painter whose window and viewport are the logical size
      \param camera where the camera was in render()
      \param logical the size of the view (logical pixels)
      \param dpr the device pixel ratio, for the minimap's image
      \param mode the camera, for the guides it shows
    */
    void drawHud( QPainter & painter,
                  const CameraPose & camera,
                  const QSize & logical,
                  const qreal dpr,
                  const CameraMode mode );

    //! whether the minimap is drawn at the bottom right for a camera (the option minimap_3d)
    static bool minimapShown( const CameraMode mode );

    //! ms of the CPU of the last frame: build() and render() up to the GPU
    double buildTime() const { return M_build_time; }

    //! ms of the CPU of the last frame: asking the GPU to draw (Scene3D::render())
    double submitTime() const { return M_submit_time; }

    //! of the build time, the players with the figures (FrameBuilder3D::poseTime())
    double poseTime() const { return M_builder.poseTime(); }

    //! the human figures drawn in the last frame
    int figureCount() const { return static_cast< int >( M_content.figures.size() ); }

    //! what the last frame drew, seen by a camera (FrameBuilder3D::state())
    View3DState state( const CameraPose & camera ) const;

    /*!
      \brief what a point of the view aims at, in the last frame (Picking::targetAt())
      \param camera where the camera was
      \param size the size of the view (logical pixels)
      \param point the point in the view (logical pixels)
    */
    RefereeTarget targetAt( const CameraPose & camera,
                            const QSizeF & size,
                            const QPointF & point ) const;

private:

    DispHolder & M_disp_holder; //!< the frames
    std::unique_ptr< FieldPainter > M_field_painter; //!< the 2D field painter, which draws the ground image
    std::unique_ptr< ScoreBoardPainter > M_score_board_painter; //!< the HUD: the 2D score board
    std::unique_ptr< TeamGraphicPainter > M_team_graphic_painter; //!< the HUD and the ends: the team logos
    LabelAtlas M_label_atlas; //!< the texts of the number labels
    FrameBuilder3D M_builder; //!< what to draw in a frame, the human figures (Options::playerModel3D()) posed
    Scene3D M_scene; //!< the OpenGL drawing
    SceneContent M_content; //!< what to draw, reused in every frame
    bool M_ready; //!< Scene3D could be initialized
    QString M_figure_pack; //!< the folder of the figure pack whose bodies were sent to the GPU (empty: none)
    QString M_ground_key; //!< what the ground was drawn with (GroundTexture::key())
    QString M_stadium_keys[2]; //!< what each end's boards and banner were drawn with (Stadium3D::TeamLook::key())
    QString M_stands_key; //!< what the stands' images were drawn with (the team colors and names)
    QImage M_minimap_field; //!< the pitch of the minimap (the HUD), drawn by the 2D field painter
    QString M_minimap_key; //!< what it was drawn with: the ground's key, its size and the screen's scale
    double M_build_time; //!< ms of the CPU (buildTime())
    double M_submit_time; //!< ms of the CPU (submitTime())

    //! the images of the ground, the ends and the stands, drawn again when they change
    void updateImages();
};

#endif
