// -*-c++-*-

/*!
  \file field_view_3d.h
  \brief the 3D view: a QOpenGLWidget. Header File.
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
#include "figures_3d.h"
#include "label_atlas.h"
#include "referee_target.h"
#include "scene_3d.h"
#include "scene_content.h"

#include <QElapsedTimer>
#include <QOpenGLWidget>
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
  \brief the 3D view (docs/PLAN_3D-VIEW.md §5.1): the widget, which draws
  the scene of Scene3D and handles the input. It shares the frames
  (DispHolder) and the options with the 2D view. Between two frames, it
  draws the positions on the way (docs/PLAN_3D-VIEW.md §5.2), and while they
  move, it draws again as soon as a frame is shown.

  QOpenGLWidget gives the widget an OpenGL context and a framebuffer, and
  calls, with the context current:
  - initializeGL() once, before the first paint (and again if the widget
    moves to another window): makes the GPU's resources (Scene3D::initialize()).
  - paintGL() whenever the widget is drawn (after update()): draws a frame
    into its framebuffer, which Qt then shows in the window.
  Outside them, makeCurrent() and doneCurrent() make the context current
  and not (the destructor).

  Over the scene, QPainter draws the HUD as the 2D view does (the score
  board and the team logos, with the 2D painters).

  The mouse moves the camera (CameraController): the left button turns it
  around what it looks at, with Shift, Ctrl or Meta moves it, and the
  middle button brings it nearer or farther. Doing so on a camera that
  follows the ball makes it the free camera. The top camera does not take
  the mouse. As in 2D, the wheel steps the frames (MainWindow), and the
  keys of the zoom (Z, X) zoom the camera (zoomCamera()). I and Ctrl+I put
  the free camera at a viewpoint of the whole pitch (viewFrom()).

  A right click asks for the referee's menu (refereeMenuRequested()), at
  the player or the point of the ground under the mouse, as last drawn
  (targetAt()).
*/
class FieldView3D
    : public QOpenGLWidget {

    Q_OBJECT

public:

    explicit
    FieldView3D( DispHolder & disp_holder,
                 const LogPlayer & log_player,
                 QWidget * parent = nullptr );
    ~FieldView3D() override;

    /*!
      \brief what the 3D view asks of OpenGL: version 3.3 "core" (without the
      old functions), a depth buffer, and 4x multisampling (smooth edges)
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

protected:

    //! make the GPU's resources (QOpenGLWidget calls it with the context current)
    void initializeGL() override;

    //! draw a frame (QOpenGLWidget calls it with the context current)
    void paintGL() override;

    // the mouse moves the camera
    void mousePressEvent( QMouseEvent * event ) override;
    void mouseMoveEvent( QMouseEvent * event ) override;

    //! the right button: the referee's menu
    void contextMenuEvent( QContextMenuEvent * event ) override;

private:

    DispHolder & M_disp_holder; //!< the frames, shared with the 2D view
    const LogPlayer & M_log_player; //!< whether a log is playing, and how far to the next frame
    std::unique_ptr< FieldPainter > M_field_painter; //!< the 2D field painter, which draws the ground image
    std::unique_ptr< ScoreBoardPainter > M_score_board_painter; //!< the HUD: the 2D score board
    std::unique_ptr< TeamGraphicPainter > M_team_graphic_painter; //!< the HUD: the 2D team logos
    LabelAtlas M_label_atlas; //!< the texts of the number labels
    Figures3D M_figures; //!< the players as human figures (Options::playerModel3D())
    Scene3D M_scene; //!< the OpenGL drawing
    SceneContent M_content; //!< what to draw, reused in every frame
    bool M_ready; //!< Scene3D could be initialized
    QString M_ground_key; //!< what the ground was drawn with (GroundTexture::key())
    QString M_stadium_keys[2]; //!< what each end's boards and banner were drawn with (Stadium3D::TeamLook::key())
    QImage M_minimap_field; //!< the pitch of the minimap (the HUD), drawn by the 2D field painter
    QString M_minimap_key; //!< what it was drawn with: the ground's key, its size and the screen's scale

    const rcss::rcg::DispInfoT * M_newest; //!< the newest frame when it was last drawn
    QElapsedTimer M_newest_clock; //!< since the newest frame came
    bool M_moving; //!< to be drawn again in the next frame

    CameraController M_camera; //!< which camera, and where it is
    CameraPose M_drawn_camera; //!< the camera of the last frame drawn, for a click to aim with
    bool M_drawn = false; //!< a frame has been drawn (M_drawn_camera and M_content.picks are set)
    QElapsedTimer M_frame_clock; //!< since the last frame was drawn, for the camera to glide
    QPoint M_last_mouse; //!< where the mouse was at the last press or move

private slots:
    //! after a frame is shown: draw the next one while things move
    void drawNext();
};

#endif
