// -*-c++-*-

/*!
  \file figures_3d.h
  \brief the players of the 3D view as human figures. Header File.
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

#ifndef RCSSMONITOR_FIGURES_3D_H
#define RCSSMONITOR_FIGURES_3D_H

#include "gait_table.h"
#include "gltf_model.h"
#include "humanoid.h"
#include "scene_content.h"

#include <QMatrix4x4>
#include <QString>
#include <QStringList>

#include <vector>

/*!
  \struct GaitClip
  \brief a clip of walking or running, as read from its motion
  (Figures3D::analyzeGait()): when the left foot comes down, and how fast a
  foot on the ground goes back, which is how fast the figure goes without
  sliding its feet
*/
struct GaitClip {
    int clip = -1; //!< in the model's clips
    float contact = 0.0f; //!< when the left foot comes down (s from the clip's start)
    float down = 0.0f; //!< how long the left foot stays down (s)
    float duration = 0.0f; //!< one cycle of the gait (two steps, s)
    float speed = 0.0f; //!< m/s, for a figure of a person's height
    float stride() const { return speed * duration; } //!< m per cycle
    //! the clip's time at a phase of the gait (0 to 1, the left foot coming down at 0)
    float timeAt( const float phase ) const { return contact + phase * duration; }
};

/*!
  \class Figures3D
  \brief the players as human figures (docs/PLAN_3D-VIEW.md §5.8): the
  bundled model, the Mannequin of Quaternius's Universal Animation Library
  (CC0, resources/models), and the poses of the figures that SceneBuilder
  put in a SceneContent. No OpenGL: Scene3D draws them.

  A pose is decided from the log every frame, from the frame and the
  fraction to the next one only (FigureMotion, from GaitTable), so that a
  frame looks the same however it is reached (paused, stepped back, jumped
  to). The motion of a clip goes through the humanoid layer (HumanoidRig):
  read as a HumanoidPose and put on the model, so that motions and models
  of other skeletons can come later (the figure packs, §5.9).

  Walking and running (7-2): the clips of walking, jogging and sprinting
  are read at one phase of the gait, the left foot coming down at its
  start, and mixed by the speed between their own speeds; the phase is the
  gait walked (GaitTable) for the figure's height, so that a foot on the
  ground does not slide. Standing (the idle clip) is mixed in below a
  walk. Going sideways, the hips turn toward the way the figure goes, and
  the chest keeps the body direction; going backward, the clips play
  backward. The figure leans toward its acceleration, about its feet:
  forward when speeding up, back when slowing down, in when turning.
*/
class Figures3D {
public:

    //! how far apart the players' idle clips are (s), so that they do not move together
    static constexpr double IDLE_OFFSET = 0.53;
    //! the walk at this part of its speed and faster is all walking; slower, standing is mixed in
    static constexpr float MOVING_FULL = 0.8f;
    //! how far the hips turn at most toward the way the figure goes (degree)
    static constexpr float TWIST_MAX = 40.0f;
    //! how far the figure leans at most (degree)
    static constexpr float TILT_MAX = 10.0f;
    //! the acceleration (m/s^2) at which the lean is three quarters (tanh 1) of TILT_MAX
    static constexpr float TILT_ACCELERATION = 15.0f;

    /*!
      \brief read the bundled model (once)
      \return false if it cannot be read; error() tells why
    */
    bool load();

    bool ready() const { return M_ready; }
    const QString & error() const { return M_error; }

    //! the materials of the model in each figure's jersey color
    static QStringList jerseyMaterials();

    const GltfModel & model() const { return M_model; }
    const HumanoidRig & rig() const { return M_rig; }

    /*!
      \brief read a clip of walking or running: when the left foot comes
      down (where it starts to stay low), and how fast the feet go back while
      they stay low (the median, both feet), for a figure of a
      person's height. The clip goes in place, facing +z.
      \param model the model
      \param rig its roles (the feet, and the toes if any)
      \param clip the clip
      \param out the result
      \return false if the feet do not come down and go back
    */
    static bool analyzeGait( const GltfModel & model,
                             const HumanoidRig & rig,
                             const int clip,
                             GaitClip * out );

    //! the clips of walking and running, slowest first
    const std::vector< GaitClip > & gaitClips() const { return M_gaits; }

    //! the clips' strides at their speeds, for GaitTable
    std::vector< GaitStride > strides() const;

    /*!
      \struct Mix
      \brief how the clips are mixed at a speed
    */
    struct Mix {
        float moving = 0.0f; //!< walking or running (1) over standing (0)
        int from = 0; //!< between these gait clips
        int to = 0;
        float t = 0.0f; //!< how far toward to
    };

    //! how the clips are mixed at a speed (m/s)
    Mix mixAt( const float speed ) const;

    /*!
      \brief how far the hips turn toward the way the figure goes (degree,
      as the field's angles): most going diagonally, none going straight
      forward, backward or sideways (where the gait turns between forward
      and backward), less when slower than a walk
    */
    float twistOf( const FigureInstance & figure ) const;

    /*!
      \brief where a figure is in the world: standing on the ground at the
      player, facing its body direction, as high as figure.height, leaning
      toward its acceleration about its feet
    */
    QMatrix4x4 modelMatrix( const FigureInstance & figure ) const;

    /*!
      \brief the transforms of the model's nodes for a figure
      \param figure the figure, with its motion
      \param time the game's time (s), with the fraction to the next frame
      \param local by node index
    */
    void localPose( const FigureInstance & figure,
                    const double time,
                    std::vector< NodeTransform > & local ) const;

    /*!
      \brief the skin matrices of the figures of a content, into
      content.figure_joints (in the order of content.figures)
      \param content the figures (SceneBuilder::addShow(), with their motions)
      \param time the game's time (s), with the fraction to the next frame
    */
    void pose( SceneContent & content,
               const double time ) const;

private:
    bool M_loaded = false; //!< load() was tried
    bool M_ready = false;
    QString M_error;
    GltfModel M_model;
    HumanoidRig M_rig;
    int M_idle = -1; //!< the clip of standing
    std::vector< GaitClip > M_gaits; //!< walking and running, slowest first

    //! a clip at a time, through the humanoid layer, and the nodes of the motion
    HumanoidPose sample( const int clip,
                         const float time,
                         std::vector< NodeTransform > & motion ) const;
};

#endif
