// -*-c++-*-

/*!
  \file humanoid.h
  \brief the bones of a human figure by their roles, and poses that any
  skeleton can take. Header File.
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

#ifndef RCSSMONITOR_HUMANOID_H
#define RCSSMONITOR_HUMANOID_H

#include "gltf_model.h"

#include <QQuaternion>
#include <QString>
#include <QStringList>
#include <QVector3D>

#include <array>
#include <vector>

/*!
  \enum HumanBone
  \brief the roles of the bones of a human figure, as VRM's humanoid bones
  (docs/plan/2026-11-11_3d-view.md §5.8). A skeleton names its bones as it likes; a
  mapping tells which bone has which role.
*/
enum class HumanBone {
    Hips,
    Spine,
    Chest,
    UpperChest,
    Neck,
    Head,
    LeftShoulder,
    LeftUpperArm,
    LeftLowerArm,
    LeftHand,
    RightShoulder,
    RightUpperArm,
    RightLowerArm,
    RightHand,
    LeftUpperLeg,
    LeftLowerLeg,
    LeftFoot,
    LeftToes,
    RightUpperLeg,
    RightLowerLeg,
    RightFoot,
    RightToes,
};

//! how many roles there are
constexpr int HUMAN_BONE_COUNT = static_cast< int >( HumanBone::RightToes ) + 1;

//! the role's name as VRM writes it (hips, spine, leftUpperArm, ...)
const char * humanBoneName( const HumanBone bone );

//! the roles that every mapping must have
bool humanBoneRequired( const HumanBone bone );

/*!
  \brief the names of the bones of each role, by the role (empty: none)
*/
using HumanBoneNames = std::array< QString, HUMAN_BONE_COUNT >;

/*!
  \brief the bones of the Quaternius rig (the Universal Animation Library,
  the Universal Base Characters), named as Unreal's mannequin: pelvis,
  spine_01, thigh_l, ...
*/
HumanBoneNames quaterniusBoneNames();

//! the role of a name as VRM writes it ("leftUpperArm"), or -1
int humanBoneOf( const QString & name );

//! the nodes of the roles of a skeleton, by the role (-1: none)
using HumanBoneNodes = std::array< int, HUMAN_BONE_COUNT >;

/*!
  \brief the presets of the skeletons that a figure pack can name
  (FigureBodySpec::skeleton): "quaternius" (and Unreal's mannequin),
  "mixamo", "vroid" (VRoid's J_Bip_ names), "xsens" (XSens MVN's BVH, as
  100STYLE's) and "vrm" (the roles that a VRM or VRM Animation file tells)
*/
QStringList skeletonPresets();

/*!
  \brief the bones' names of a preset of names (quaternius, mixamo, vroid,
  xsens; not vrm, whose roles are in the file)
  \return false if there is no such preset (names is left as it was)
*/
bool skeletonBoneNames( const QString & preset,
                        HumanBoneNames * names );

/*!
  \brief the nodes of the roles of a model: by a pack's own map (a role's
  name, as VRM writes it, to a bone's name), else by a preset, else (both
  empty) by VRM's roles if the file has them, else by the first preset
  whose bones the model has. A preset's names match a bone of the name,
  also after a prefix ending with ':' or '_' (mixamorig:Hips, mixamorig_Hips).
  \param model the model
  \param preset a preset of skeletonPresets(), or empty
  \param map a role's name to a bone's name (empty: none)
  \param nodes the nodes found
  \param error why not, if not null: the roles required without a bone
  \return false if a required role (humanBoneRequired()) has no bone
*/
bool findHumanBones( const GltfModel & model,
                     const QString & preset,
                     const std::vector< std::pair< QString, QString > > & map,
                     HumanBoneNodes * nodes,
                     QString * error = nullptr );

/*!
  \struct HumanoidPose
  \brief a pose that any skeleton can take: for each role, how the bone is
  turned from its rest pose, relative to the role above it (a
  "normalized" rotation: in the directions of the model, the same on every
  skeleton; identity at rest), and how far the hips moved from their rest
  place, in hip heights (the height of the hips at rest).
*/
struct HumanoidPose {
    std::array< QQuaternion, HUMAN_BONE_COUNT > rotations;
    QVector3D hips; //!< the hips' move from rest, in hip heights

    QQuaternion & operator[]( const HumanBone bone ) { return rotations[static_cast< int >( bone )]; }
    const QQuaternion & operator[]( const HumanBone bone ) const { return rotations[static_cast< int >( bone )]; }

    /*!
      \brief between two poses: each role's rotation the shorter way, the
      hips' move in a line
      \param a the pose at 0
      \param b the pose at 1
      \param t how far toward b (0 to 1)
    */
    static HumanoidPose mix( const HumanoidPose & a,
                             const HumanoidPose & b,
                             const float t );

    /*!
      \brief the pose mirrored left to right (across the model's plane x = 0,
      a person at rest being its own mirror): each role takes its other
      side's rotation, mirrored, and the hips move the other way sideways
    */
    HumanoidPose mirrored() const;
};

/*!
  \class HumanoidRig
  \brief a skeleton of a GltfModel with its bones mapped to the roles: it
  reads a pose of the skeleton as a HumanoidPose (poseOf()), and puts a
  HumanoidPose on the skeleton (apply()). So a motion of one skeleton plays
  on another (retargeting), when both are at rest in the same pose (a T
  pose; a model in an A pose would need its rest turned first, which comes
  with the figure packs, Phase 9).

  How a role's rotation is read: with G the bone's rotation in the model
  in the pose and R at rest (in the T pose: turned by restTurn()), the bone
  turned D = G * R^-1 in the model; the normalized rotation is
  D(parent role)^-1 * D. Putting it on another
  skeleton is the reverse, with that skeleton's R. The bones without a
  role (the root, the fingers, ...) keep what the caller gives them.
*/
class HumanoidRig {
public:

    /*!
      \brief map the bones of a model to the roles
      \param model the model, which must live as long as this
      \param names the bones of each role
      \param error the missing bones, if not null
      \return false if a required role (humanBoneRequired()) has no bone
    */
    bool build( const GltfModel & model,
                const HumanBoneNames & names,
                QString * error = nullptr );

    /*!
      \brief map the nodes of a model to the roles (findHumanBones())
      \return false if a required role has no node
    */
    bool build( const GltfModel & model,
                const HumanBoneNodes & nodes,
                QString * error = nullptr );

    //! an arm farther than this (degree) from straight out sideways at rest is taken as if it were (an A pose)
    static constexpr float ARM_POSE_TOLERANCE = 10.0f;

    /*!
      \brief how a role's bone is taken as turned at rest in the model, from
      where the model has it, for the T pose: the upper arms of an A pose
      turned up to straight out sideways, and their lower arms straightened
      with them (and the roles below following); identity for the others
    */
    QQuaternion restTurn( const HumanBone bone ) const { return M_rest_turn[static_cast< int >( bone )]; }

    /*!
      \brief which way the model faces, from +z about the vertical (degree,
      counterclockwise from above): from its left side, the hips' joints (a
      person faces +z with its left at +x)
    */
    float facingAngle() const;

    /*!
      \brief which way is up in the model: of the six ways along its axes
      (+x, -x, +y, -y, +z, -z), the nearest to the way from the hips to the
      head at rest. +y for a model upright as glTF's; +z for a skeleton of
      a file written z up (some BVH files).
    */
    QVector3D upAxis() const;

    const GltfModel * model() const { return M_model; }

    //! the node of a role, or -1
    int node( const HumanBone bone ) const { return M_node[static_cast< int >( bone )]; }

    //! the role of a node, or -1 (a bone without a role: a finger, the root)
    int roleOf( const int node ) const
      {
          return ( node >= 0 && node < static_cast< int >( M_role_of.size() ) ? M_role_of[node] : -1 );
      }

    //! the height of the hips at rest above the lowest point of the mesh (m)
    float hipHeight() const { return M_hip_height; }

    /*!
      \brief the pose of the skeleton as a HumanoidPose
      \param local the nodes' transforms (by node index), e.g. a clip sampled
    */
    HumanoidPose poseOf( const std::vector< NodeTransform > & local ) const;

    /*!
      \brief put a pose on the skeleton: the rotations of the bones of the
      roles, and the translation of the hips
      \param pose the pose
      \param local the nodes' transforms (by node index), which the caller
      fills first: the rest, or for the bones without a role what it likes
      (the fingers of a clip of the same skeleton)
    */
    void apply( const HumanoidPose & pose,
                std::vector< NodeTransform > & local ) const;

    /*!
      \brief turn a role's bone in the model, the bones below it following
      (as the hips turned about the vertical carry the legs and the back)
      \param pose the pose to change
      \param bone the role
      \param turn the rotation in the directions of the model, about the
      bone's own joint
    */
    void turnInModel( HumanoidPose & pose,
                      const HumanBone bone,
                      const QQuaternion & turn ) const;

    /*!
      \brief how a role's bone is turned from rest in the model (D, through
      the roles above), identity if the role has no bone
    */
    QQuaternion turnedInModel( const HumanoidPose & pose,
                               const HumanBone bone ) const;

    /*!
      \brief turn a role's bone so that it is turned from rest in the model
      by turned (D), the bones below following
    */
    void setTurnedInModel( HumanoidPose & pose,
                           const HumanBone bone,
                           const QQuaternion & turned ) const;

private:
    const GltfModel * M_model = nullptr;
    std::array< int, HUMAN_BONE_COUNT > M_node; //!< the node of each role, -1 if none
    std::array< int, HUMAN_BONE_COUNT > M_parent_role; //!< the nearest role above, -1 at the hips
    std::vector< int > M_role_of; //!< by node index: its role, -1 if none
    std::array< QQuaternion, HUMAN_BONE_COUNT > M_rest_rotation; //!< R: each role's bone in the model at rest, in the T pose (restTurn())
    std::array< QQuaternion, HUMAN_BONE_COUNT > M_rest_turn; //!< restTurn()
    QVector3D M_rest_hips; //!< where the hips are in the model at rest
    float M_hip_height = 1.0f;
};

/*!
  \brief a bone's name without the prefix of its skeleton: after the last
  ':' (Mixamo's "mixamorig:Hips", "mixamorig1:Hips"), as it is otherwise.
  Bones of two files of one skeleton are matched by it.
*/
QString boneKey( const QString & name );

/*!
  \brief how far the hips go across a clip, horizontally, in hip heights:
  a clip not in place (a "root motion": a walk going forward), or about 0
*/
float clipDrift( const GltfModel & model,
                 const HumanoidRig & rig,
                 const GltfModel::Clip & clip );

/*!
  \brief how far the hips go at most across a clip, horizontally from where
  they start, in hip heights: an action going somewhere (a slide, a dive)
  goes far even if it comes back
*/
float clipTravel( const GltfModel & model,
                  const HumanoidRig & rig,
                  const GltfModel::Clip & clip );

/*!
  \enum ClipPlace
  \brief where a clip put on a skeleton goes across the floor
*/
enum class ClipPlace {
    AsItIs, //!< as the clip goes
    InPlace, //!< the hips' horizontal drift across the clip taken off in a line (a root motion: the clip then goes in place, and loops)
    Pinned, //!< the hips kept horizontally where they start (an action going somewhere: done where the player is)
};

/*!
  \brief a clip of one skeleton put on another, through the humanoid
  roles: sampled at a rate, each pose read on the one (poseOf()) and put on
  the other (apply()); the bones without a role (the fingers) of the same
  names (boneKey()) by their turns from rest. So clips of other files, and
  of other skeletons, go into a motion's model.
  \param source the skeleton of the clip, and its roles
  \param clip the clip (of source)
  \param target the skeleton to put it on, and its roles
  \param in_place the hips' horizontal drift across the clip taken off
  (a root motion: the clip then goes in place, and loops)
  \param rate samples a second
  \return the clip, for target's nodes
*/
GltfModel::Clip retargetClip( const GltfModel & source,
                              const HumanoidRig & source_rig,
                              const GltfModel::Clip & clip,
                              const GltfModel & target,
                              const HumanoidRig & target_rig,
                              const bool in_place,
                              const float rate = 30.0f );

//! retargetClip(), the clip going across the floor as told
GltfModel::Clip retargetClip( const GltfModel & source,
                              const HumanoidRig & source_rig,
                              const GltfModel::Clip & clip,
                              const GltfModel & target,
                              const HumanoidRig & target_rig,
                              const ClipPlace place,
                              const float rate = 30.0f );

/*!
  \brief a clip mirrored left to right (HumanoidPose::mirrored()): a walk
  to the right made a walk to the left. Sampled at a rate on the skeleton
  of the clip, each pose mirrored and put on it again; the bones without a
  role (the fingers) as they are.
  \param model the skeleton of the clip, and its roles
  \param clip the clip (of model)
  \param rate samples a second
  \return the clip mirrored, for model's nodes
*/
GltfModel::Clip mirrorClip( const GltfModel & model,
                            const HumanoidRig & rig,
                            const GltfModel::Clip & clip,
                            const float rate = 30.0f );

/*!
  \brief the rest of a skeleton saved in a pose (GltfModel::posed_rest: an
  FBX file of clips without a skin) turned as another skeleton's rest, bone
  by bone of the same names (boneKey()): each such bone's rotation in the
  model, and its place, made the other's.
  \param model the skeleton to change
  \param like the skeleton at rest (a T pose)
  \return how many bones were matched
*/
int restAs( GltfModel & model,
            const GltfModel & like );

/*!
  \brief a motion capture (GltfModel::capture: a BVH file) settled in the
  figures' space: its rest standing on y = 0 (the body's lowest bone at
  0, the hips above the origin), and each clip moved and turned about the
  vertical so that it starts with the hips above the origin, faces +z (its
  mean heading, from the hips' left side), and comes down to y = 0 (the
  body's lowest bone across it). The body is the hips and the bones below
  them. What is moved is the highest of the hips and the bones above them
  that a clip moves (a root above the hips, or the hips themselves), by
  its translation keys (a clip without them stays). A capture's root is
  placed in the room it was taken in (its OFFSET often at the origin with
  the legs below it, its positions far from it, its heading any way); a
  figure wants a clip in place, facing +z, on the ground. Call it with the
  model upright (+y up), and build the rig again after it.
  \param model the skeleton and its clips
  \param rig its roles
  \return false without the hips and the upper legs
*/
bool settleCapture( GltfModel & model,
                    const HumanoidRig & rig );

#endif
