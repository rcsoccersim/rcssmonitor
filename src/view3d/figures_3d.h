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

#include "figure_uniform.h"
#include "gait_table.h"
#include "gltf_model.h"
#include "humanoid.h"
#include "scene_content.h"

#include "camera_mode.h"

#include <QMatrix4x4>
#include <QString>
#include <QStringList>

#include <array>
#include <initializer_list>
#include <memory>
#include <utility>
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
  bundled models (resources/models, CC0) and the poses of the figures that
  SceneBuilder put in a SceneContent. No OpenGL: Scene3D draws them.

  The motion is the Mannequin's of Quaternius's Universal Animation
  Library: its clips of standing, walking and running. The body drawn is
  one of two models (setModel()): the Mannequin itself, all in the
  figure's color but the joints, or a human body of the Universal Base
  Characters (the same skeleton, other proportions), in a uniform painted
  by FigureUniform. The motion goes from one to the other through the
  humanoid layer, and the poses of the actions are made on the body drawn.

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

  Actions (7-3, FigureActions from FigureActionLog): over the clips, the
  head turns by the neck (spread over the chest, the neck and the head),
  and the poses of the actions go in by their weights, which rise before
  the action's moment and fall after it: the kick (the foot on the ball's
  side meeting it at the moment; a touch, a pass with the side of the foot,
  or a shot with a long swing; around the moment the foot goes to the
  ball, by IK of the leg), the sliding tackle, sitting down after a foul
  (once the tackler has slid in), the goalie's hands toward the ball for a
  catch (crouching with the hands near the ground for a low ball, the arms
  up for a high one, leaning to the side for a ball to the side), and the
  arm on the side it points to pointing (the chest turning for a point far
  to the side or behind). The model has no clips of them, so they are
  poses made here (the web version's pose.ts): each limb by its swing from
  hanging down, its turn to the right and its bend.
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
    //! how much of the neck's turn the chest, the neck and the head take
    static constexpr float NECK_CHEST = 0.2f;
    static constexpr float NECK_NECK = 0.4f;
    static constexpr float NECK_HEAD = 0.4f;
    //! how far a goalie leans at most to catch a ball to the side (degree)
    static constexpr float CATCH_ROLL = 35.0f;
    //! how much a touch's pose counts at most (it keeps some of the running)
    static constexpr float KICK_TAP_WEIGHT = 0.8f;
    //! the kicking foot goes to the ball from this many cycles before the kick's moment to this many after it
    static constexpr float REACH_BEFORE = 0.4f;
    static constexpr float REACH_AFTER = 0.3f;
    //! pointing farther to the side than this (degree), the chest turns toward it by half the rest, up to POINT_TURN_MAX
    static constexpr float POINT_TURN_FROM = 60.0f;
    static constexpr float POINT_TURN_MAX = 60.0f;
    //! the arm points down from the shoulder's height (m) to the point, but not lower than POINT_PITCH_MIN (degree from hanging down)
    static constexpr float POINT_SHOULDER = 1.4f;
    static constexpr float POINT_PITCH_MIN = 30.0f;
    //! a ball to catch this high (m) or lower is caught bending down, this high or higher with the arms up
    static constexpr float CATCH_LOW = 0.3f;
    static constexpr float CATCH_HIGH = 1.5f;

    Figures3D();
    ~Figures3D();
    Figures3D( const Figures3D & ) = delete;
    Figures3D & operator=( const Figures3D & ) = delete;

    /*!
      \brief read the motion and the Mannequin, the body drawn first (once)
      \return false if it cannot be read; error() tells why
    */
    bool load();

    bool ready() const { return M_ready; }
    const QString & error() const { return M_error; }

    /*!
      \brief the body drawn: the Mannequin (also for Simple) or the human
      body, read the first time it is asked for
      \return false if it cannot be read (error() tells why): the body stays
    */
    bool setModel( const PlayerModel model );
    //! the body drawn
    PlayerModel bodyModel() const { return M_body_model; }
    /*!
      \brief how long a gait cycle of the body drawn is, of the motion's
      for the same height: the speeds of the gait clips read on it, of
      theirs (longer for longer legs), so that its feet do not slide
    */
    float bodyStride() const;

    //! the materials of the body drawn in each figure's color
    QStringList jerseyMaterials() const;
    //! the materials of the body drawn in a uniform (FigureUniform) of each figure's color
    QStringList uniformMaterials() const;
    //! where the body's uniform ends at rest
    const FigureUniform::Cuts & uniformCuts() const;

    //! the body drawn: its model and its roles
    const GltfModel & model() const;
    const HumanoidRig & rig() const;
    //! the model of the motion (the clips) and its roles
    const GltfModel & motionModel() const { return M_model; }
    const HumanoidRig & motionRig() const { return M_rig; }

    /*!
      \brief read a clip of walking or running: when the left foot comes
      down (where it starts to stay low), and how fast the feet go back while
      they stay low (the median, both feet), for a figure of a
      person's height. The clip goes in place, facing +z.
      \param model the model
      \param rig its roles (the feet, and the toes if any)
      \param clip the clip
      \param out the result
      \param body if not null, the clip read on this body (through the
      humanoid layer), of the speed for its proportions
      \param body_rig the body's roles
      \return false if the feet do not come down and go back
    */
    static bool analyzeGait( const GltfModel & model,
                             const HumanoidRig & rig,
                             const int clip,
                             GaitClip * out,
                             const GltfModel * body = nullptr,
                             const HumanoidRig * body_rig = nullptr );

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
      \struct Weights
      \brief how much each action's pose counts at the time drawn (0 to 1)
    */
    struct Weights {
        float kick = 0.0f;
        float tackle = 0.0f;
        float fouled = 0.0f;
        float caught = 0.0f;
    };

    //! how much each action's pose counts: by its phase, rising before its moment and falling after it
    static Weights weightsOf( const FigureActions & actions );

    /*!
      \brief through points (x ascending), eased between them (smoothstep),
      flat beyond the ends
    */
    static float curve( std::initializer_list< std::pair< float, float > > points,
                        const float x );

    //! how far a goalie leans to its right to catch a ball to the side (degree; left: negative)
    float rollOf( const FigureInstance & figure ) const;

    /*!
      \brief how much the kicking foot goes to the ball (0 to 1): most at
      the kick's moment, from REACH_BEFORE before it to REACH_AFTER after it
    */
    static float reachOf( const FigureActions & actions );

    //! where the kicking foot's toes meet the ball (world): its back, at its center's height
    static QVector3D kickTarget( const FigureKick & kick );

    /*!
      \brief where a figure is in the world: standing on the ground at the
      player, facing its body direction, as high as figure.height, leaning
      toward its acceleration about its feet (and to a ball to catch)
    */
    QMatrix4x4 modelMatrix( const FigureInstance & figure ) const;

    /*!
      \brief the transforms of the model's nodes for a figure
      \param figure the figure, with its motion
      \param time the game's time (s), with the fraction to the next frame
      \param local by the body's node index
    */
    void localPose( const FigureInstance & figure,
                    const double time,
                    std::vector< NodeTransform > & local ) const;

    /*!
      \brief where the model's nodes of a figure are in the world, posed
      (to inspect a pose: FrameBuilder3D::state())
      \param figure the figure, with its motion
      \param time the game's time (s), with the fraction to the next frame
      \param world by node index
    */
    void worldNodes( const FigureInstance & figure,
                     const double time,
                     std::vector< QMatrix4x4 > & world ) const;

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
    GltfModel M_model; //!< the motion: the Mannequin with its clips
    HumanoidRig M_rig; //!< its roles
    int M_idle = -1; //!< the clip of standing
    std::vector< GaitClip > M_gaits; //!< walking and running, slowest first

    struct Body;
    //! the bodies by PlayerModel (Mannequin, Human), read when first asked for
    std::array< std::unique_ptr< Body >, 2 > M_bodies;
    std::array< bool, 2 > M_body_tried = { { false, false } };
    const Body * M_body = nullptr; //!< the body drawn
    PlayerModel M_body_model = PlayerModel::Mannequin;

    //! read a body (a model of the motion's skeleton)
    std::unique_ptr< Body > loadBody( const char * path,
                                      const QStringList & jersey,
                                      const QStringList & uniform,
                                      QString * error ) const;

    //! a clip at a time, through the humanoid layer, and the nodes of the motion
    HumanoidPose sample( const int clip,
                         const float time,
                         std::vector< NodeTransform > & motion ) const;

    /*!
      \struct LimbPose
      \brief a leg or an arm: swung forward from hanging down (pitch; up is
      180), then turned to the right about the vertical (yaw), turned
      outward about itself first (turn, as a foot for a pass), and the knee
      or the elbow bent (degree)
    */
    struct LimbPose {
        float pitch = 0.0f;
        float yaw = 0.0f;
        float bend = 0.0f;
        float turn = 0.0f;
    };

    //! put a leg (right: 1) as told, in the directions of the model
    void setLeg( HumanoidPose & pose,
                 const int right,
                 const LimbPose & limb ) const;
    //! put an arm (right: 1) as told, in the directions of the chest (or of the model)
    void setArm( HumanoidPose & pose,
                 const int right,
                 const LimbPose & limb,
                 const bool in_model = false ) const;

    /*!
      \brief the body's nodes for the motion's: the bones without a role
      (the fingers) turned from the body's rest as the motion's are from
      theirs (the same skeleton: as they are)
      \param motion by the motion's node index
      \param local by the body's node index
    */
    void bodyNodes( const std::vector< NodeTransform > & motion,
                    std::vector< NodeTransform > & local ) const;

    //! the kicking foot to the ball around the kick's moment (two-bone IK of the leg)
    void reach( const FigureInstance & figure,
                HumanoidPose & pose,
                const std::vector< NodeTransform > & nodes ) const;

    //! the neck and the actions over a pose of the clips
    void act( const FigureInstance & figure,
              HumanoidPose & pose ) const;
};

#endif
