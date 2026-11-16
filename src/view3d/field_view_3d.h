// -*-c++-*-

/*!
  \file field_view_3d.h
  \brief the 3D view: a window drawn by OpenGL (QOpenGLWindow). Header File.
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

#ifndef RCSSMONITOR_FIELD_VIEW_3D_H
#define RCSSMONITOR_FIELD_VIEW_3D_H

#include "cameras_3d.h"
#include "fps_meter.h"
#include "frame_builder_3d.h"
#include "interpolate.h"
#include "label_atlas.h"
#include "referee_target.h"
#include "scene_3d.h"
#include "scene_content.h"

#include <QElapsedTimer>
#include <QOpenGLWindow>
#include <QPoint>
#include <QString>
#include <QSurfaceFormat>

#include <memory>

class DispHolder;
class FieldPainter;
class LogPlayer;
class ScoreBoardPainter;
class TeamGraphicPainter;

namespace rcss {
namespace rcg {
struct DispInfoT;
}
}

/*!
  \class FieldView3D
  \brief the 3D view (docs/PLAN_3D-VIEW.md §5.1): the window, which draws
  the scene of Scene3D and handles the mouse. It shares the frames
  (DispHolder) and the options with the 2D view. Between two frames, it
  draws the positions on the way (docs/PLAN_3D-VIEW.md §5.2), and while they
  move, it draws again as soon as a frame is shown.

  It is a window of its own (QOpenGLWindow), put in the main window by a
  container widget (QWidget::createWindowContainer(), MainWindow): it draws
  straight into the window's framebuffer, which the screen shows. A
  QOpenGLWidget draws into an image that Qt then puts into the main
  window's, a few more copies of the whole view in every frame: on a 4K
  screen with a GPU in the CPU, 31 frames a second instead of 48 (§14).

  QOpenGLWindow gives the window an OpenGL context, and calls, with the
  context current:
  - initializeGL() once, before the first paint: makes the GPU's resources
    (Scene3D::initialize()).
  - paintGL() whenever the window is drawn (after update()): draws a frame
    into its framebuffer, which the screen then shows.
  Outside them, makeCurrent() and doneCurrent() make the context current
  and not (the destructor).

  It takes no keyboard focus (the keys are the main window's actions), and
  the wheel steps the frames there (wheelTurned()), as over the 2D view.

  Over the scene, QPainter draws the HUD as the 2D view does (the score
  board and the team logos, with the 2D painters).

  The mouse moves the camera (CameraController): the left button turns it
  around what it looks at, with Shift, Ctrl or Meta moves it, and the
  middle button brings it nearer or farther. Doing so on a camera that
  follows the ball makes it the free camera. The top camera does not take
  the mouse. As in 2D, the wheel steps the frames (MainWindow), and the
  keys of the zoom (Z, X) zoom the camera (zoomCamera()). I and Ctrl+I put
  the free camera at a viewpoint of the whole pitch (viewFrom()).

  The right button asks for the referee's menu (refereeMenuRequested()),
  at the player or the point of the ground under the mouse, as last drawn
  (targetAt()).

  Smooth edges (the option antialias_3d) are drawn by Scene3D into a
  framebuffer of samples, and averaged into the window's, which has none
  (surfaceFormat()): turned on and off as the frames go.
*/
class FieldView3D
    : public QOpenGLWindow {

    Q_OBJECT

public:

    explicit
    FieldView3D( DispHolder & disp_holder,
                 const LogPlayer & log_player,
                 QWindow * parent = nullptr );
    ~FieldView3D() override;

    /*!
      \brief what the 3D view asks of OpenGL: version 3.3 "core" (without the
      old functions), a depth buffer and a stencil (for QPainter); no
      multisampling: the smooth edges are Scene3D's own (RenderTarget)
    */
    static QSurfaceFormat surfaceFormat();

    /*!
      \brief check if the 3D view can be shown here.
      \return an empty string if it can, or why it cannot
    */
    static QString unavailableReason();

    //! the camera
    CameraMode cameraMode() const { return M_camera.mode(); }

    //! change the camera
    void setCameraMode( const CameraMode mode );

    /*!
      \brief zoom the camera, as the 2D view's keys (CameraController::zoom())
      \param factor less than 1: nearer; more than 1: farther; 0: back to the camera's own distance
    */
    void zoomCamera( const double factor );

    //! put the free camera at a viewpoint of the whole pitch (the camera becomes the free camera)
    void viewFrom( const Viewpoint viewpoint );

    /*!
      \brief what a right click at a point of the view aims at, as last
      drawn (Picking::targetAt()): nothing before the first frame is drawn
      \param point the point in the view (logical pixels)
    */
    RefereeTarget targetAt( const QPoint & point ) const;

    /*!
      \brief what the last frame drew, for the tests to check
      (docs/PLAN_3D-VIEW.md §10.2): not drawn before the first frame
    */
    View3DState state() const;

signals:

    //! the mouse made the camera the free camera
    void cameraModeChanged( CameraMode mode );

    /*!
      \brief a right click asks for the referee's menu (RefereeMenu)
      \param target the player or the point aimed at (targetAt())
      \param global_pos where to show the menu on the screen
    */
    void refereeMenuRequested( const RefereeTarget & target,
                               const QPoint & global_pos );

    /*!
      \brief the wheel turned over the view, to step the frames as over the
      2D view (a window of its own: its events do not go up to the main window)
      \param delta QWheelEvent::angleDelta().y()
    */
    void wheelTurned( int delta );

protected:

    //! make the GPU's resources (QOpenGLWindow calls it with the context current)
    void initializeGL() override;

    //! draw a frame (QOpenGLWindow calls it with the context current)
    void paintGL() override;

    //! the mouse moves the camera; the right button: the referee's menu
    void mousePressEvent( QMouseEvent * event ) override;
    void mouseMoveEvent( QMouseEvent * event ) override;

    //! the wheel: the frames (wheelTurned())
    void wheelEvent( QWheelEvent * event ) override;

private:

    DispHolder & M_disp_holder; //!< the frames, shared with the 2D view
    const LogPlayer & M_log_player; //!< whether a log is playing, and how far to the next frame
    std::unique_ptr< FieldPainter > M_field_painter; //!< the 2D field painter, which draws the ground image
    std::unique_ptr< ScoreBoardPainter > M_score_board_painter; //!< the HUD: the 2D score board
    std::unique_ptr< TeamGraphicPainter > M_team_graphic_painter; //!< the HUD: the 2D team logos
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

    const rcss::rcg::DispInfoT * M_newest; //!< the newest frame when it was last drawn
    QElapsedTimer M_newest_clock; //!< since the newest frame came
    QElapsedTimer M_live_tick; //!< since the last drawing, for the live clock
    Interpolate::LiveClock M_live_clock; //!< the time a live game is drawn at
    bool M_moving; //!< to be drawn again in the next frame

    CameraController M_camera; //!< which camera, and where it is
    CameraPose M_drawn_camera; //!< the camera of the last frame drawn, for a click to aim with
    bool M_drawn = false; //!< a frame has been drawn (M_drawn_camera and M_content.picks are set)
    QElapsedTimer M_frame_clock; //!< since the last frame was drawn, for the camera to glide
    QPoint M_last_mouse; //!< where the mouse was at the last press or move

    /*!
      \struct Timing
      \brief the times of the frames, told every TIMING_PERIOD seconds when
      the environment variable RCSSMONITOR_3D_TIMING is set
      (docs/PLAN_3D-VIEW.md §10.3)
    */
    struct Timing {
        bool on = false;
        QElapsedTimer period; //!< since the times were last told
        QElapsedTimer since_frame; //!< since the last frame
        int frames = 0;
        double build = 0.0; //!< ms of the CPU: what to draw (FrameBuilder3D, the labels)
        double players = 0.0; //!< of it, the players with the figures
        double submit = 0.0; //!< ms of the CPU: asking the GPU to draw (Scene3D::render())
        double longest = 0.0; //!< the longest time between two frames (ms)
        int figures = 0; //!< in the last frame
        double gpu = 0.0; //!< ms of the GPU, of gpu_frames frames (Scene3D::takeGpuTime())
        int gpu_frames = 0;
    };
    static constexpr double TIMING_PERIOD = 2.0;
    Timing M_timing;
    bool M_gpu_timing = false; //!< the GPU's times are taken (M_timing.on, or the option fps_3d)
    FpsMeter M_fps; //!< the frames per second shown over the view (the option fps_3d)
    QElapsedTimer M_fps_clock; //!< the steady clock of M_fps
    //! tell the times, every TIMING_PERIOD seconds
    void reportTiming();

private slots:
    //! after a frame is shown: draw the next one while things move
    void drawNext();
};

#endif
