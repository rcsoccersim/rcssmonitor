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

#include "figure_pack.h"
#include "figure_uniform.h"
#include "gait_table.h"
#include "gltf_model.h"
#include "humanoid.h"
#include "scene_content.h"

#include <QMatrix4x4>
#include <QString>
#include <QStringList>

#include <algorithm>
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
    bool reverse = false; //!< played backward (a forward clip, for going backward)
    float stride() const { return speed * duration; } //!< m per cycle
    /*!
      \brief the clip's time at a phase of the gait (0 to 1, the left foot
      coming down at 0); played backward, the left foot comes down where it
      leaves the ground played forward
    */
    float timeAt( const float phase ) const
      {
          return ( reverse ? contact + down - phase * duration : contact + phase * duration );
      }
};

/*!
  \struct ActionClip
  \brief a clip of an action, as read from its motion (Figures3D): where
  in it the action's moment is, and when the body is down on the ground
*/
struct ActionClip {
    int clip = -1; //!< in the motion model's clips (-1: none): a kick with the right foot, a dive to the right
    int other = -1; //!< the clip of the other foot or side (the other's mirrored, if not told)
    float duration = 0.0f; //!< s
    float moment = 0.0f; //!< s from its start: a kick's foot meeting the ball, a slide on the ground, a fall on the ground, the hands reaching for the ball
    float down = -1.0f; //!< s from its start: when the body comes down to the ground (negative: it does not)

    bool valid() const { return clip >= 0; }
    //! a time of it, kept in it (not looped back to its start)
    float at( const float time ) const
      {
          return std::min( std::max( 0.0f, time ), std::max( 0.0f, duration - std::min( 1.0e-4f, duration * 0.01f ) ) );
      }
};

/*!
  \struct ActionClips
  \brief the clips of the actions of a motion (docs/FIGURE_PACKS.md): those
  not told are not valid (the poses made by Figures3D instead)
*/
struct ActionClips {
    ActionClip kick[3]; //!< by KickKind: a touch (kick_tap), a pass (kick_pass, or else kick), a shot (kick_shot, or else kick)
    ActionClip tackle; //!< the slide; its moment on the ground (the foot meeting the ball)
    ActionClip tackle_up; //!< getting up from it (or else get_up)
    ActionClip fall; //!< falling after a foul; its moment on the ground
    ActionClip get_up;
    ActionClip catch_low;
    ActionClip catch_high;
    ActionClip dive; //!< to the right (clip) and to the left (other)
    ActionClip turn; //!< turning on the spot: to the right (clip) and to the left (other), played by how far the legs turn
    float turn_angle[2] = { 0.0f, 0.0f }; //!< how far each turns (degree): to the right, to the left
    std::vector< float > turn_times[2]; //!< each one's time (s) when it has turned each whole degree (0 to its angle)
};

/*!
  \class Figures3D
  \brief the players as human figures (docs/plan/2026-11-11_3d-view.md §5.8, §5.9):
  the models and the motion of a figure pack (FigurePack, setPack()), and
  the poses of the figures that SceneBuilder put in a SceneContent. No
  OpenGL: Scene3D draws them.

  A pack has a motion, its clips of standing, walking and running (the
  bundled ones the Mannequin's of Quaternius's Universal Animation
  Library), and the bodies drawn: the bundled ones the Mannequin itself,
  all in the figure's color but the joints, or the human bodies of the
  Universal Base Characters, a man's and a woman's (the same skeleton,
  other proportions), each figure in its own (FigureLook: its body, its
  hairstyle, the colors of its skin and its hair) and in a uniform painted
  by FigureUniform. The motion goes from one to the other through the
  humanoid layer, and the poses of the actions are made on the body drawn.
  Each pack is read once (its motion once for the packs that share it),
  and kept.

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

  With clips of walking and jogging backward and sideways (9-4b, the
  bundled ones cut from 100STYLE), the clips of each way are mixed by the
  speed as the forward ones, and those of the two ways nearest to the way
  the figure goes by GaitWays::mixAt(), the gait going on whichever way
  (gaitOf()): backward, the backward clips, then the forward ones played
  backward faster than them; sideways, the clips of the side (a side not
  told: the other's mirrored), faster than them as fast as needed. Nearly
  forward or backward (to GaitWays::TWIST_FULL), the hips turn toward the
  way gone instead (a figure running a little off its body direction runs
  forward), the clips of the two ways mixed for the rest.

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
  to the side or behind). Without clips of them, they are poses made here
  (the web version's pose.ts): each limb by its swing from hanging down,
  its turn to the right and its bend.

  With clips of the actions (9-5, ActionClips; the bundled ones the slide
  and getting up of the Universal Animation Library 2), an action plays
  its clip by the same weights, at its own speed with its moment at the
  action's: a kick (the clip of the other foot mirrored), the slide, a
  fall, the goalie's catch (turned toward the ball) and dive (by how far
  to the side the ball is). Down on the ground, a clip of getting up plays
  to end with the action, when the player can go again (upSchedule(),
  rise()), from the pose down of the clip or made here.

  Turning on the spot (9-5b): the legs follow the body direction behind it
  (GaitTable: its direction averaged over the cycles before), the hips
  turned back toward them up to TURN_TWIST_MAX, the chest keeping the body
  direction; standing, they step as they turn (turnStepOf()), by the steps
  of GaitTable (a step each TURN_STEP_ANGLE they turn, at most
  TURN_STEPS_MAX in a cycle): by a clip of turning, TURN_STEP_ANGLE of it a
  step, its own turn taken off; or else the walk's steps, smaller (its
  cycle is two steps).
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
    //! the goalie turns toward a ball to catch by at most this much (degree)
    static constexpr float CATCH_TURN = 50.0f;

    //! seconds of a cycle of the game, for the clips of the actions
    static constexpr float CYCLE_TIME = 0.1f;
    //! the hips at this part of their height or lower: the body down on the ground
    static constexpr float DOWN_HIPS = 0.4f;
    //! an action's clip whose hips go farther than this (hip heights) is pinned: done where the player is
    static constexpr float ACTION_TRAVEL = 0.3f;
    //! a foot swinging forward at least this fast (hip heights a second) kicks
    static constexpr float KICK_SPEED = 2.0f;
    //! a clip of getting up plays at most this many times as fast, to end when the player can go
    static constexpr float UP_SPEED_MAX = 2.0f;
    //! cycles over which a pose goes over to a clip of getting up
    static constexpr float UP_BLEND = 1.0f;
    //! a ball to catch this far to the side (m) starts to be caught by a dive (with its clip), from DIVE_FULL all
    static constexpr float DIVE_FROM = 0.8f;
    static constexpr float DIVE_FULL = 1.2f;

    //! the legs lag behind the body direction by at most this much (degree): the hips turned back, the chest keeping the body direction
    static constexpr float TURN_TWIST_MAX = 45.0f;
    //! the legs turning this fast (degree/s) step fully
    static constexpr float TURN_RATE_FULL = 180.0f;
    //! the walk's steps for turning on the spot mixed in at most this much (smaller steps)
    static constexpr float TURN_STEP = 0.5f;
    //! a clip of turning turns at least this much (degree)
    static constexpr float TURN_MIN = 30.0f;

    Figures3D();
    ~Figures3D();
    Figures3D( const Figures3D & ) = delete;
    Figures3D & operator=( const Figures3D & ) = delete;

    /*!
      \brief read the bundled Mannequin's pack, the one drawn first (once)
      \return false if it cannot be read; error() tells why
    */
    bool load();

    bool ready() const { return M_ready; }
    const QString & error() const { return M_error; }

    /*!
      \brief the figures of a pack: its motion and its bodies, read the
      first time it is asked for (a pack of the same name and folder)
      \return false if it cannot be read (error() tells why): the pack
      drawn stays
    */
    bool setPack( const FigurePack & pack );
    //! whether a pack is set
    bool hasPack() const { return M_pack != nullptr; }
    //! the pack drawn (an empty one if none)
    const FigurePack & pack() const;
    //! the bodies of the pack drawn, as the looks are drawn from them (SceneBuilder::addShow())
    const FigureCast & cast() const;
    //! how many bodies are drawn (the Mannequin: 1; the human bodies: 2)
    int bodyCount() const;
    //! the body of a figure (an index of the bodies drawn): the body of its looks
    int bodyOf( const FigureInstance & figure ) const;
    /*!
      \brief how long a gait cycle of a body is, of the motion's for the
      same height: the speeds of the gait clips read on it, of theirs
      (longer for longer legs), so that its feet do not slide
    */
    float bodyStride( const int body = 0 ) const;

    //! the materials of a body in each figure's color
    QStringList jerseyMaterials( const int body = 0 ) const;
    //! the materials of a body in a uniform (FigureUniform) of each figure's color
    QStringList uniformMaterials( const int body = 0 ) const;
    //! how each part (SkinPrimitive) of a body is painted
    std::vector< FigureUniform::Paint > paints( const int body = 0 ) const;
    //! where a body's uniform ends at rest
    const FigureUniform::Cuts & uniformCuts( const int body = 0 ) const;

    //! a body drawn: its model and its roles
    const GltfModel & model( const int body = 0 ) const;
    const HumanoidRig & rig( const int body = 0 ) const;
    //! the model of the motion (the clips) and its roles
    const GltfModel & motionModel() const;
    const HumanoidRig & motionRig() const;

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
      \param travel the way the clip goes in the model (forward: +z; to the
      left: +x), against which a foot on the ground goes
      \return false if the feet do not come down and go back
    */
    static bool analyzeGait( const GltfModel & model,
                             const HumanoidRig & rig,
                             const int clip,
                             GaitClip * out,
                             const GltfModel * body = nullptr,
                             const HumanoidRig * body_rig = nullptr,
                             const QVector3D & travel = QVector3D( 0.0f, 0.0f, 1.0f ) );

    //! the clips of walking and running, slowest first
    const std::vector< GaitClip > & gaitClips() const;

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

    //! how the clips are mixed at a speed (m/s), going forward
    Mix mixAt( const float speed ) const;

    //! how clips of walking and running (slowest first) are mixed at a speed (m/s; moving left 0)
    static Mix mixOf( const std::vector< GaitClip > & clips,
                      const float speed );

    /*!
      \brief the clips of walking and running of a way (GaitWays::Way),
      slowest first: forward, gaitClips(); backward, the clips of walking
      backward, then the forward ones played backward faster than them;
      sideways, the clips of the side (a side not told: the other side's
      mirrored). The ways but forward are empty without clips of other ways
      (the classic way), and the sideways ones without sideways clips.
    */
    const std::vector< GaitClip > & wayClips( const int way ) const;

    //! the strides of the clips by the way, for GaitTable
    GaitWays gaitWays() const;

    /*!
      \struct Gait
      \brief how the clips are mixed for a figure: walking or running over
      standing, the two ways mixed, and each way's clips by the speed
    */
    struct Gait {
        float moving = 0.0f; //!< walking or running (1) over standing (0)
        GaitWays::Mix ways; //!< the ways a and b, b by t
        Mix a; //!< the clips of way a (wayClips())
        Mix b; //!< the clips of way b
    };

    //! how the clips are mixed for a figure, by its velocity and its body direction
    Gait gaitOf( const FigureInstance & figure ) const;

    /*!
      \brief how far the hips turn toward the way the figure goes (degree,
      as the field's angles): most going diagonally, none going straight
      forward, backward or sideways (where the gait turns between forward
      and backward), less when slower than a walk. With sideways clips, as
      far as the way from forward or backward up to GaitWays::TWIST_FULL,
      then less to none sideways (GaitWays::twistAt()), less when slower
      than a walk.
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

    //! the clips of the actions of the motion drawn
    const ActionClips & actionClips() const;
    /*!
      \brief a kick's moment in a clip: when a foot swings forward fastest
      (its toes, or else the foot), and which foot
      \return false if no foot swings forward at KICK_SPEED
    */
    static bool kickMoment( const GltfModel & model,
                            const HumanoidRig & rig,
                            const int clip,
                            float * moment,
                            bool * right );
    //! when the body comes down to the ground in a clip: the hips to DOWN_HIPS of their height (s; negative: never)
    static float downTime( const GltfModel & model,
                           const HumanoidRig & rig,
                           const int clip );
    //! when the hands reach farthest from the chest in a clip (s): a catch's moment
    static float reachMoment( const GltfModel & model,
                              const HumanoidRig & rig,
                              const int clip );
    /*!
      \struct UpSchedule
      \brief when a clip of getting up plays: from start (cycles of the
      action's phase), at speed times its own, from offset (s) into it
    */
    struct UpSchedule {
        float start = 0.0f;
        float speed = 1.0f;
        float offset = 0.0f;
    };
    /*!
      \brief when a clip of getting up plays, to end when the action does
      (the player can go then): as late as it can at its own speed, after
      the body is down; faster if there is not the time, up to
      UP_SPEED_MAX; its start left out if that is not enough
      \param duration the clip's (s)
      \param ground when the body is down (cycles of the phase)
      \param end when the action ends (cycles of the phase)
    */
    static UpSchedule upSchedule( const float duration,
                                  const float ground,
                                  const float end );
    //! how much a goalie's catch is a dive (0 to 1): by how far to the side the ball is, with a clip of a dive
    float diveOf( const FigureActions & actions ) const;
    /*!
      \brief how far a clip turns the body about the vertical, its hips' way
      from its start to its end (degree, as the field's angles: to the
      right, positive), and its time when it has turned each whole degree
      that way (the first time it has turned so far)
    */
    static float turnAngle( const GltfModel & model,
                            const HumanoidRig & rig,
                            const int clip,
                            std::vector< float > * times );
    /*!
      \brief how much a figure steps on the spot as it turns (0 to 1): by how
      fast its legs turn, standing (less moving)
    */
    float turnStepOf( const FigureInstance & figure ) const;

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

    struct Motion;
    struct Body;
    struct Pack;
    std::vector< std::unique_ptr< Motion > > M_motions; //!< the motions read
    std::vector< std::unique_ptr< Pack > > M_packs; //!< the packs read
    const Pack * M_pack = nullptr; //!< the pack drawn

    //! the motion of the pack drawn
    const Motion & motion() const;
    //! a figure's body
    const Body & bodyFor( const FigureInstance & figure ) const;

    //! read a motion, or the one read before of the same model and clips
    const Motion * motionOf( const FigureMotionSpec & spec,
                             QString * error );
    //! read a body (a model of a skeleton of the humanoid layer) for a motion
    std::unique_ptr< Body > loadBody( const FigureBodySpec & spec,
                                      const Motion & motion,
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

    //! put a leg (right: 1) of a body as told, in the directions of the model
    static void setLeg( const Body & body,
                        HumanoidPose & pose,
                        const int right,
                        const LimbPose & limb );
    //! put an arm (right: 1) of a body as told, in the directions of the chest (or of the model)
    static void setArm( const Body & body,
                        HumanoidPose & pose,
                        const int right,
                        const LimbPose & limb,
                        const bool in_model = false );

    /*!
      \brief the body's nodes for the motion's: the bones without a role
      (the fingers) turned from the body's rest as the motion's are from
      theirs (the same skeleton: as they are)
      \param motion by the motion's node index
      \param local by the body's node index
    */
    void bodyNodes( const Body & body,
                    const std::vector< NodeTransform > & motion,
                    std::vector< NodeTransform > & local ) const;

    //! the kicking foot to the ball around the kick's moment (two-bone IK of the leg)
    void reach( const Body & body,
                const FigureInstance & figure,
                HumanoidPose & pose,
                const std::vector< NodeTransform > & nodes ) const;

    //! the neck and the actions over a pose of the clips
    void act( const Body & body,
              const FigureInstance & figure,
              HumanoidPose & pose ) const;
    /*!
      \brief a pose of an action down on the ground getting up again: the
      pose down (of the action's clip, or made here), going over to a
      clip of getting up (upSchedule()) when it plays
      \param down the pose down at the time
      \param up the clip of getting up (none if not valid: down as it is)
      \param phase the action's phase (cycles)
      \param ground when the body is down (cycles of the phase)
      \param end when the action ends (cycles of the phase)
    */
    HumanoidPose rise( const HumanoidPose & down,
                       const ActionClip & up,
                       const float phase,
                       const float ground,
                       const float end ) const;
    //! the goalie's catch made here (the arms toward the ball, bending down for a low ball, up for a high one) over a pose
    void catchPose( const Body & body,
                    const FigureActions & actions,
                    const float high,
                    HumanoidPose & target ) const;

    //! where a figure of a body's model is in the world (modelMatrix())
    QMatrix4x4 modelMatrix( const GltfModel & body,
                            const FigureInstance & figure ) const;
};

#endif
