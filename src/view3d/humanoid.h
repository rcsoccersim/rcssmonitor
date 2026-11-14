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
#include <QVector3D>

#include <array>
#include <vector>

/*!
  \enum HumanBone
  \brief the roles of the bones of a human figure, as VRM's humanoid bones
  (docs/PLAN_3D-VIEW.md §5.8). A skeleton names its bones as it likes; a
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
  in the pose and R at rest, the bone turned D = G * R^-1 in the model; the
  normalized rotation is D(parent role)^-1 * D. Putting it on another
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

    const GltfModel * model() const { return M_model; }

    //! the node of a role, or -1
    int node( const HumanBone bone ) const { return M_node[static_cast< int >( bone )]; }

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

private:
    const GltfModel * M_model = nullptr;
    std::array< int, HUMAN_BONE_COUNT > M_node; //!< the node of each role, -1 if none
    std::array< int, HUMAN_BONE_COUNT > M_parent_role; //!< the nearest role above, -1 at the hips
    std::vector< int > M_role_of; //!< by node index: its role, -1 if none
    std::array< QQuaternion, HUMAN_BONE_COUNT > M_rest_rotation; //!< R: each role's bone in the model at rest
    QVector3D M_rest_hips; //!< where the hips are in the model at rest
    float M_hip_height = 1.0f;
};

#endif
