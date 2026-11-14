// -*-c++-*-

/*!
  \file humanoid.cpp
  \brief the bones of a human figure by their roles, and poses that any
  skeleton can take. Source File.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "humanoid.h"

#include <QStringList>
#include <QVector4D>

#include <algorithm>

namespace {

//! the role of each HumanBone and the role above it (-1: none)
struct RoleInfo {
    const char * name;
    HumanBone parent; //!< the role above it (for the hips, itself: none)
    bool required;
};

const RoleInfo ROLES[HUMAN_BONE_COUNT] = {
    { "hips", HumanBone::Hips, true },
    { "spine", HumanBone::Hips, true },
    { "chest", HumanBone::Spine, false },
    { "upperChest", HumanBone::Chest, false },
    { "neck", HumanBone::UpperChest, false },
    { "head", HumanBone::Neck, true },
    { "leftShoulder", HumanBone::UpperChest, false },
    { "leftUpperArm", HumanBone::LeftShoulder, true },
    { "leftLowerArm", HumanBone::LeftUpperArm, true },
    { "leftHand", HumanBone::LeftLowerArm, true },
    { "rightShoulder", HumanBone::UpperChest, false },
    { "rightUpperArm", HumanBone::RightShoulder, true },
    { "rightLowerArm", HumanBone::RightUpperArm, true },
    { "rightHand", HumanBone::RightLowerArm, true },
    { "leftUpperLeg", HumanBone::Hips, true },
    { "leftLowerLeg", HumanBone::LeftUpperLeg, true },
    { "leftFoot", HumanBone::LeftLowerLeg, true },
    { "leftToes", HumanBone::LeftFoot, false },
    { "rightUpperLeg", HumanBone::Hips, true },
    { "rightLowerLeg", HumanBone::RightUpperLeg, true },
    { "rightFoot", HumanBone::RightLowerLeg, true },
    { "rightToes", HumanBone::RightFoot, false },
};

int
index_of( const HumanBone bone )
{
    return static_cast< int >( bone );
}

}

/*-------------------------------------------------------------------*/
const char *
humanBoneName( const HumanBone bone )
{
    return ROLES[index_of( bone )].name;
}

/*-------------------------------------------------------------------*/
bool
humanBoneRequired( const HumanBone bone )
{
    return ROLES[index_of( bone )].required;
}

/*-------------------------------------------------------------------*/
HumanBoneNames
quaterniusBoneNames()
{
    HumanBoneNames names;
    const auto set = [&names]( const HumanBone bone, const char * name ) {
        names[index_of( bone )] = QString::fromLatin1( name );
    };
    set( HumanBone::Hips, "pelvis" );
    set( HumanBone::Spine, "spine_01" );
    set( HumanBone::Chest, "spine_02" );
    set( HumanBone::UpperChest, "spine_03" );
    set( HumanBone::Neck, "neck_01" );
    set( HumanBone::Head, "Head" );
    set( HumanBone::LeftShoulder, "clavicle_l" );
    set( HumanBone::LeftUpperArm, "upperarm_l" );
    set( HumanBone::LeftLowerArm, "lowerarm_l" );
    set( HumanBone::LeftHand, "hand_l" );
    set( HumanBone::RightShoulder, "clavicle_r" );
    set( HumanBone::RightUpperArm, "upperarm_r" );
    set( HumanBone::RightLowerArm, "lowerarm_r" );
    set( HumanBone::RightHand, "hand_r" );
    set( HumanBone::LeftUpperLeg, "thigh_l" );
    set( HumanBone::LeftLowerLeg, "calf_l" );
    set( HumanBone::LeftFoot, "foot_l" );
    set( HumanBone::LeftToes, "ball_l" );
    set( HumanBone::RightUpperLeg, "thigh_r" );
    set( HumanBone::RightLowerLeg, "calf_r" );
    set( HumanBone::RightFoot, "foot_r" );
    set( HumanBone::RightToes, "ball_r" );
    return names;
}

/*-------------------------------------------------------------------*/
bool
HumanoidRig::build( const GltfModel & model,
                    const HumanBoneNames & names,
                    QString * error )
{
    M_model = &model;
    M_role_of.assign( model.node_names.size(), -1 );
    QStringList missing;
    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
    {
        M_node[b] = ( names[b].isEmpty() ? -1 : model.nodeIndex( names[b] ) );
        if ( M_node[b] >= 0 )
        {
            M_role_of[M_node[b]] = b;
        }
        else if ( ROLES[b].required )
        {
            missing << QString::fromLatin1( ROLES[b].name );
        }
    }
    if ( ! missing.isEmpty() )
    {
        if ( error ) *error = QStringLiteral( "no bone for " ) + missing.join( QStringLiteral( ", " ) );
        M_model = nullptr;
        return false;
    }

    // the nearest role above each role (skipping the roles without a bone)
    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
    {
        int p = ( b == index_of( HumanBone::Hips ) ? -1 : index_of( ROLES[b].parent ) );
        while ( p >= 0 && M_node[p] < 0 )
        {
            p = ( p == index_of( HumanBone::Hips ) ? -1 : index_of( ROLES[p].parent ) );
        }
        M_parent_role[b] = p;
    }

    // the bones at rest in the model
    std::vector< QMatrix4x4 > global;
    model.globals( model.rest, global );
    std::vector< QQuaternion > rotation( model.rest.size() );
    for ( const int i : model.order )
    {
        rotation[i] = ( model.parent[i] >= 0 ? rotation[model.parent[i]] : QQuaternion() ) * model.rest[i].r;
    }
    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
    {
        M_rest_rotation[b] = ( M_node[b] >= 0 ? rotation[M_node[b]].normalized() : QQuaternion() );
    }
    M_rest_hips = global[node( HumanBone::Hips )].column( 3 ).toVector3D();
    M_hip_height = std::max( 0.01f, M_rest_hips.y() - model.min_y );
    return true;
}

/*-------------------------------------------------------------------*/
HumanoidPose
HumanoidRig::poseOf( const std::vector< NodeTransform > & local ) const
{
    HumanoidPose pose;
    if ( ! M_model ) return pose;
    const GltfModel & model = *M_model;

    // the rotations in the model, through the parents (the scales are
    // taken as the same along every axis, as a skeleton has them)
    std::vector< QQuaternion > rotation( model.rest.size() );
    for ( const int i : model.order )
    {
        const QQuaternion & r = ( i < static_cast< int >( local.size() ) ? local[i] : model.rest[i] ).r;
        rotation[i] = ( model.parent[i] >= 0 ? rotation[model.parent[i]] : QQuaternion() ) * r;
    }

    // D: how each role's bone turned from rest, in the model
    std::array< QQuaternion, HUMAN_BONE_COUNT > turned;
    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
    {
        if ( M_node[b] < 0 ) continue;
        turned[b] = ( rotation[M_node[b]] * M_rest_rotation[b].conjugated() ).normalized();
        const int p = M_parent_role[b];
        pose.rotations[b] = ( p >= 0 ? turned[p].conjugated() * turned[b] : turned[b] ).normalized();
    }

    std::vector< QMatrix4x4 > global;
    model.globals( local, global );
    pose.hips = ( global[node( HumanBone::Hips )].column( 3 ).toVector3D() - M_rest_hips ) / M_hip_height;
    return pose;
}

/*-------------------------------------------------------------------*/
void
HumanoidRig::apply( const HumanoidPose & pose,
                    std::vector< NodeTransform > & local ) const
{
    if ( ! M_model ) return;
    const GltfModel & model = *M_model;
    local.resize( model.rest.size(), NodeTransform() );

    std::vector< QQuaternion > rotation( model.rest.size() );
    std::vector< QMatrix4x4 > global( model.rest.size() );
    std::array< QQuaternion, HUMAN_BONE_COUNT > turned;
    for ( const int i : model.order )
    {
        const int p = model.parent[i];
        const QQuaternion parent_rotation = ( p >= 0 ? rotation[p] : QQuaternion() );
        const QMatrix4x4 parent_global = ( p >= 0 ? global[p] : QMatrix4x4() );
        const int b = M_role_of[i];
        if ( b >= 0 )
        {
            const int pb = M_parent_role[b];
            turned[b] = ( pb >= 0 ? turned[pb] : QQuaternion() ) * pose.rotations[b];
            // the bone's rotation in the model, and so relative to its parent
            const QQuaternion wanted = turned[b] * M_rest_rotation[b];
            local[i].r = ( parent_rotation.conjugated() * wanted ).normalized();
            if ( b == static_cast< int >( HumanBone::Hips ) )
            {
                const QVector3D at = M_rest_hips + pose.hips * M_hip_height;
                local[i].t = ( parent_global.inverted() * QVector4D( at, 1.0f ) ).toVector3D();
            }
        }
        rotation[i] = parent_rotation * local[i].r;
        global[i] = parent_global * local[i].matrix();
    }
}

/*-------------------------------------------------------------------*/
void
HumanoidRig::turnInModel( HumanoidPose & pose,
                          const HumanBone bone,
                          const QQuaternion & turn ) const
{
    const int b = static_cast< int >( bone );
    if ( ! M_model || M_node[b] < 0 ) return;

    // D of the role above, through the roles above it
    QQuaternion above;
    for ( int p = M_parent_role[b]; p >= 0; p = M_parent_role[p] )
    {
        above = pose.rotations[p] * above;
    }
    // D' = turn * D, the roles below keeping their rotations relative to
    // it (so they turn as much in the model): N' = above^-1 * turn * above * N
    pose.rotations[b] = ( above.conjugated() * turn * above * pose.rotations[b] ).normalized();
}

/*-------------------------------------------------------------------*/
HumanoidPose
HumanoidPose::mix( const HumanoidPose & a,
                   const HumanoidPose & b,
                   const float t )
{
    if ( t <= 0.0f ) return a;
    if ( t >= 1.0f ) return b;
    HumanoidPose pose;
    for ( int k = 0; k < HUMAN_BONE_COUNT; ++k )
    {
        const QQuaternion & p = a.rotations[k];
        QQuaternion q = b.rotations[k];
        if ( QQuaternion::dotProduct( p, q ) < 0.0f ) q = -q; // the shorter way
        pose.rotations[k] = ( p * ( 1.0f - t ) + q * t ).normalized();
    }
    pose.hips = a.hips * ( 1.0f - t ) + b.hips * t;
    return pose;
}
