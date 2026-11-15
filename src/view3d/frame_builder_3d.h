// -*-c++-*-

/*!
  \file frame_builder_3d.h
  \brief what the 3D view draws in a frame, apart from the widget. Header File.
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

#ifndef RCSSMONITOR_FRAME_BUILDER_3D_H
#define RCSSMONITOR_FRAME_BUILDER_3D_H

#include "ball_3d.h"
#include "camera_3d.h"
#include "cameras_3d.h"
#include "figures_3d.h"
#include "gait_table.h"
#include "interpolate.h"
#include "scene_content.h"
#include "sight.h"

#include "camera_mode.h"

#include <rcss/rcg/types.h>

#include <QQuaternion>
#include <QVector3D>

#include <cstddef>
#include <vector>

class DispHolder;
class Options;

/*!
  \struct FigureProbe
  \brief points of a figure's pose, to compare poses (View3DState): where
  its left knee is, and how its head is turned, in the world
*/
struct FigureProbe {
    int player = -1; //!< its index in the show
    QVector3D left_knee;
    QQuaternion head;
};

/*!
  \struct View3DState
  \brief what the 3D view drew in its last frame, for the tests to check
  (docs/PLAN_3D-VIEW.md §10.2, the web version's Render3DState):
  FieldView3D::state(), FrameBuilder3D::state()
*/
struct View3DState {
    bool drawn = false; //!< a frame was drawn
    Interpolate::Position at; //!< the frames drawn, and how far between them
    CameraMode camera = CameraMode::Broadcast;
    CameraPose camera_pose; //!< where the camera was

    int players = 0; //!< the players drawn
    int cylinders = 0; //!< of them, as cylinders (the others as figures)
    int labels = 0; //!< the number labels, the cards and the stamina bars drawn

    PlayerModel model = PlayerModel::Simple; //!< how the players were drawn (Simple when not as figures)
    bool figures_ready = false; //!< the model of the figures could be read
    int figures = 0; //!< the players drawn as figures
    //! the figures doing each action (its pose counting)
    int kicks = 0;
    int tackles = 0;
    int fouled = 0;
    int catches = 0;
    std::vector< FigureProbe > probes; //!< of each figure, in the order drawn
    std::vector< float > figure_joints; //!< the skin matrices of the figures (SceneContent::figure_joints)
    std::vector< int > figure_bodies; //!< the body of each figure (SceneContent::figure_bodies)
    bool shadows = false; //!< the sun cast the detailed shadows (SceneContent::shadows)
    bool sky = false; //!< the sky was drawn (SceneContent::sky)
    bool realistic_grass = false; //!< the realistic grass was drawn (SceneContent::realistic_grass)
    bool stands = false; //!< the stands were drawn (SceneContent::stands)
    bool ball_pattern = false; //!< the ball's pattern was drawn (SceneContent::ball_pattern)
    QQuaternion ball_turn; //!< how the ball was turned, as it rolled (BallSpin)
    int blob_shadows = 0; //!< the simple shadows drawn (SceneContent::blob_shadows)
};

/*!
  \class FrameBuilder3D
  \brief what the 3D view draws in a frame (docs/PLAN_3D-VIEW.md §5.1),
  without the widget and without OpenGL, so that the tests can build the
  frames of a log as FieldView3D does: the show between the two frames
  (Interpolate), the goals and the boards, the players (as cylinders or
  as human figures, posed from the log: GaitTable, FigureActionLog,
  Figures3D) and the ball, the overlays, as the player camera's player
  perceives them, and what the cameras follow. FieldView3D then moves the
  camera after it, adds the labels (which are sized for the camera) and
  draws it.

  The poses are decided from the frames and the fraction only
  (docs/PLAN_3D-VIEW.md §5.2): a frame looks the same however it is
  reached. The table of the gaits keeps what it read of the log, which
  does not change it.
*/
class FrameBuilder3D {
public:

    //! the human figures (the GL side sends their model to the GPU)
    Figures3D & figures() { return M_figures; }
    const Figures3D & figures() const { return M_figures; }

    /*!
      \brief what to draw for a frame, but the labels (SceneBuilder::addLabels()
      with the camera, after it)
      \param holder the frames and the parameters
      \param opt the options
      \param at the frames to draw, and how far between them (Interpolate)
      \param camera the camera: no figures from the top camera, and the
      player camera draws what its player perceives
      \param content cleared, then filled
    */
    void build( const DispHolder & holder,
                const Options & opt,
                const Interpolate::Position & at,
                const CameraMode camera,
                SceneContent & content );

    //! the show drawn (between the two frames)
    const rcss::rcg::ShowInfoT & show() const { return M_show; }
    //! whether there were frames to draw
    bool hasShow() const { return M_has_show; }
    //! what the cameras follow in the show
    const CameraSubject & subject() const { return M_subject; }
    //! what the player camera's player perceives, or null (everything is drawn)
    const Perceptions * seen() const { return M_seen ? &M_perceptions : nullptr; }
    //! the game's time drawn (s), with the fraction to the next frame
    double time() const { return M_time; }
    //! how long the players took in the last build(), with the figures' motions, actions and poses (ms of the CPU)
    double poseTime() const { return M_pose_time; }

    /*!
      \brief what was drawn: the counts of a content built by build() (and
      the labels added after it), with the figures' poses
      \param content the content
      \param camera the camera that drew it
    */
    View3DState state( const SceneContent & content,
                       const CameraPose & camera ) const;

private:
    Figures3D M_figures;
    GaitTable M_gaits; //!< how far the players have walked, for the figures
    BallSpin M_spin; //!< how the ball has rolled
    QQuaternion M_ball_turn; //!< how the ball is turned in the frame built
    rcss::rcg::ShowInfoT M_show;
    bool M_has_show = false;
    Interpolate::Position M_at;
    CameraMode M_camera = CameraMode::Broadcast;
    CameraSubject M_subject;
    Perceptions M_perceptions;
    bool M_seen = false;
    double M_time = 0.0;
    double M_pose_time = 0.0;
    PlayerModel M_model = PlayerModel::Simple; //!< the figures' model, Simple when not drawn as figures
    bool M_model_failed = false; //!< a model could not be read (told once)
};

#endif
