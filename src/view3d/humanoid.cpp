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

#include <QMatrix3x3>
#include <QStringList>
#include <QVector4D>

#include <algorithm>
#include <cmath>
#include <limits>

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

namespace {

/*!
  \brief the bones' names of each named preset, by the role (empty: none).
  Mixamo's: its own bones (Spine, Spine1, Spine2 for the spine, the chest
  and the upper chest). VRoid's: J_Bip_ and C (center), L or R. XSens
  MVN's (motion capture, 100STYLE's BVH files): the Collar a shoulder, the
  Shoulder an upper arm.
*/
HumanBoneNames
presetNames( const QString & preset )
{
    HumanBoneNames names;
    if ( preset == QLatin1String( "quaternius" ) )
    {
        names = quaterniusBoneNames();
    }
    else if ( preset == QLatin1String( "mixamo" ) )
    {
        const char * mixamo[HUMAN_BONE_COUNT] = {
            "Hips", "Spine", "Spine1", "Spine2", "Neck", "Head",
            "LeftShoulder", "LeftArm", "LeftForeArm", "LeftHand",
            "RightShoulder", "RightArm", "RightForeArm", "RightHand",
            "LeftUpLeg", "LeftLeg", "LeftFoot", "LeftToeBase",
            "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase",
        };
        for ( int b = 0; b < HUMAN_BONE_COUNT; ++b ) names[b] = QString::fromLatin1( mixamo[b] );
    }
    else if ( preset == QLatin1String( "vroid" ) )
    {
        const char * vroid[HUMAN_BONE_COUNT] = {
            "J_Bip_C_Hips", "J_Bip_C_Spine", "J_Bip_C_Chest", "J_Bip_C_UpperChest", "J_Bip_C_Neck", "J_Bip_C_Head",
            "J_Bip_L_Shoulder", "J_Bip_L_UpperArm", "J_Bip_L_LowerArm", "J_Bip_L_Hand",
            "J_Bip_R_Shoulder", "J_Bip_R_UpperArm", "J_Bip_R_LowerArm", "J_Bip_R_Hand",
            "J_Bip_L_UpperLeg", "J_Bip_L_LowerLeg", "J_Bip_L_Foot", "J_Bip_L_ToeBase",
            "J_Bip_R_UpperLeg", "J_Bip_R_LowerLeg", "J_Bip_R_Foot", "J_Bip_R_ToeBase",
        };
        for ( int b = 0; b < HUMAN_BONE_COUNT; ++b ) names[b] = QString::fromLatin1( vroid[b] );
    }
    else if ( preset == QLatin1String( "xsens" ) )
    {
        // XSens MVN's BVH (100STYLE): four bones of the spine, the collars
        // on the last; Chest3 has no role
        const char * xsens[HUMAN_BONE_COUNT] = {
            "Hips", "Chest", "Chest2", "Chest4", "Neck", "Head",
            "LeftCollar", "LeftShoulder", "LeftElbow", "LeftWrist",
            "RightCollar", "RightShoulder", "RightElbow", "RightWrist",
            "LeftHip", "LeftKnee", "LeftAnkle", "LeftToe",
            "RightHip", "RightKnee", "RightAnkle", "RightToe",
        };
        for ( int b = 0; b < HUMAN_BONE_COUNT; ++b ) names[b] = QString::fromLatin1( xsens[b] );
    }
    return names;
}

/*!
  \brief the node of a preset's bone: of the name, or of the name after a
  prefix ending with ':' or '_' (mixamorig:Hips); -1 if none
*/
int
presetNode( const GltfModel & model,
            const QString & name )
{
    if ( name.isEmpty() ) return -1;
    const int exact = model.nodeIndex( name );
    if ( exact >= 0 ) return exact;
    for ( std::size_t i = 0; i < model.node_names.size(); ++i )
    {
        const QString & n = model.node_names[i];
        if ( n.size() > name.size()
             && n.endsWith( name )
             && ( n[n.size() - name.size() - 1] == QLatin1Char( ':' )
                  || n[n.size() - name.size() - 1] == QLatin1Char( '_' ) ) )
        {
            return static_cast< int >( i );
        }
    }
    return -1;
}

//! the required roles without a node
QStringList
missingRoles( const HumanBoneNodes & nodes )
{
    QStringList missing;
    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
    {
        if ( nodes[b] < 0 && ROLES[b].required ) missing << QString::fromLatin1( ROLES[b].name );
    }
    return missing;
}

//! the nodes of a preset (of skeletonPresets()) in a model
HumanBoneNodes
presetNodes( const GltfModel & model,
             const QString & preset )
{
    HumanBoneNodes nodes;
    nodes.fill( -1 );
    if ( preset == QLatin1String( "vrm" ) )
    {
        for ( const std::pair< QString, int > & bone : model.humanoid_bones )
        {
            const int b = humanBoneOf( bone.first );
            if ( b >= 0 ) nodes[b] = bone.second;
        }
        return nodes;
    }
    const HumanBoneNames names = presetNames( preset );
    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b ) nodes[b] = presetNode( model, names[b] );
    return nodes;
}

}

/*-------------------------------------------------------------------*/
int
humanBoneOf( const QString & name )
{
    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
    {
        if ( name == QLatin1String( ROLES[b].name ) ) return b;
    }
    return -1;
}

/*-------------------------------------------------------------------*/
bool
skeletonBoneNames( const QString & preset,
                   HumanBoneNames * names )
{
    if ( preset == QLatin1String( "vrm" ) || ! skeletonPresets().contains( preset ) ) return false;
    *names = presetNames( preset );
    return true;
}

/*-------------------------------------------------------------------*/
QStringList
skeletonPresets()
{
    return QStringList( { QStringLiteral( "quaternius" ), QStringLiteral( "mixamo" ),
                          QStringLiteral( "vroid" ), QStringLiteral( "xsens" ), QStringLiteral( "vrm" ) } );
}

/*-------------------------------------------------------------------*/
bool
findHumanBones( const GltfModel & model,
                const QString & preset,
                const std::vector< std::pair< QString, QString > > & map,
                HumanBoneNodes * nodes,
                QString * error )
{
    const auto fail = [error]( const QString & why ) {
        if ( error ) *error = why;
        return false;
    };

    // the pack's own map
    if ( ! map.empty() )
    {
        nodes->fill( -1 );
        for ( const std::pair< QString, QString > & m : map )
        {
            const int b = humanBoneOf( m.first );
            if ( b < 0 ) return fail( QStringLiteral( "no role \"" ) + m.first + QLatin1Char( '"' ) );
            ( *nodes )[b] = model.nodeIndex( m.second );
            if ( ( *nodes )[b] < 0 )
            {
                return fail( QStringLiteral( "no bone \"" ) + m.second + QStringLiteral( "\" for " ) + m.first );
            }
        }
        const QStringList missing = missingRoles( *nodes );
        return missing.isEmpty() || fail( QStringLiteral( "no bone for " ) + missing.join( QStringLiteral( ", " ) ) );
    }

    // a preset
    if ( ! preset.isEmpty() )
    {
        if ( ! skeletonPresets().contains( preset ) ) return fail( QStringLiteral( "no skeleton \"" ) + preset + QLatin1Char( '"' ) );
        if ( preset == QLatin1String( "vrm" ) && model.humanoid_bones.empty() )
        {
            return fail( QStringLiteral( "not a VRM model (no humanoid bones)" ) );
        }
        *nodes = presetNodes( model, preset );
        const QStringList missing = missingRoles( *nodes );
        return missing.isEmpty()
            || fail( QStringLiteral( "no bone for " ) + missing.join( QStringLiteral( ", " ) )
                     + QStringLiteral( " (skeleton " ) + preset + QLatin1Char( ')' ) );
    }

    // found: VRM's roles, else the first preset that fits
    if ( ! model.humanoid_bones.empty() )
    {
        return findHumanBones( model, QStringLiteral( "vrm" ), map, nodes, error );
    }
    QString first;
    for ( const QString & p : skeletonPresets() )
    {
        if ( p == QLatin1String( "vrm" ) ) continue;
        const HumanBoneNodes found = presetNodes( model, p );
        const QStringList missing = missingRoles( found );
        if ( missing.isEmpty() )
        {
            *nodes = found;
            return true;
        }
        if ( first.isEmpty() ) first = missing.join( QStringLiteral( ", " ) );
    }
    return fail( QStringLiteral( "the bones are of no skeleton known (" ) + skeletonPresets().join( QStringLiteral( ", " ) )
                 + QStringLiteral( "): no bone for " ) + first );
}

/*-------------------------------------------------------------------*/
bool
HumanoidRig::build( const GltfModel & model,
                    const HumanBoneNames & names,
                    QString * error )
{
    HumanBoneNodes nodes;
    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
    {
        nodes[b] = ( names[b].isEmpty() ? -1 : model.nodeIndex( names[b] ) );
    }
    return build( model, nodes, error );
}

/*-------------------------------------------------------------------*/
bool
HumanoidRig::build( const GltfModel & model,
                    const HumanBoneNodes & nodes,
                    QString * error )
{
    M_model = &model;
    M_role_of.assign( model.node_names.size(), -1 );
    QStringList missing;
    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
    {
        M_node[b] = ( nodes[b] >= 0 && nodes[b] < static_cast< int >( model.node_names.size() ) ? nodes[b] : -1 );
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
    const auto at = [&]( const HumanBone bone ) {
        return global[node( bone )].column( 3 ).toVector3D();
    };

    // the T pose: an arm far from straight out sideways (an A pose) taken
    // as if it were, and its lower arm straightened with it; the roles
    // below each turn following those above
    M_rest_turn.fill( QQuaternion() );
    const HumanBone arms[2][3] = {
        { HumanBone::LeftUpperArm, HumanBone::LeftLowerArm, HumanBone::LeftHand },
        { HumanBone::RightUpperArm, HumanBone::RightLowerArm, HumanBone::RightHand },
    };
    std::array< QQuaternion, HUMAN_BONE_COUNT > own; // each role's own turn
    for ( int side = 0; side < 2; ++side )
    {
        const QVector3D out( side == 0 ? 1.0f : -1.0f, 0.0f, 0.0f ); // a person's left at +x
        const auto off = []( const QVector3D & a, const QVector3D & b ) {
            return std::acos( std::max( -1.0f, std::min( 1.0f, QVector3D::dotProduct( a, b ) ) ) ) * 180.0f / 3.14159265f;
        };
        const QVector3D upper = ( at( arms[side][1] ) - at( arms[side][0] ) ).normalized();
        QQuaternion turn;
        if ( off( upper, out ) > ARM_POSE_TOLERANCE )
        {
            turn = QQuaternion::rotationTo( upper, out );
            own[index_of( arms[side][0] )] = turn;
        }
        const QVector3D lower = turn.rotatedVector( at( arms[side][2] ) - at( arms[side][1] ) ).normalized();
        if ( ! turn.isIdentity() && off( lower, out ) > ARM_POSE_TOLERANCE )
        {
            own[index_of( arms[side][1] )] = QQuaternion::rotationTo( lower, out );
        }
    }
    // (Qt's product of quaternions is not exact even by the identity: a
    // model without a turn keeps its rest as it is, to the last bit)
    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b ) // the roles are listed after those above them
    {
        const int p = M_parent_role[b];
        const QQuaternion & above = ( p >= 0 ? M_rest_turn[p] : M_rest_turn[b] );
        M_rest_turn[b] = ( own[b].isIdentity() ? above
                           : above.isIdentity() ? own[b]
                           : ( own[b] * above ).normalized() );
    }

    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
    {
        if ( M_node[b] < 0 ) M_rest_rotation[b] = QQuaternion();
        else if ( M_rest_turn[b].isIdentity() ) M_rest_rotation[b] = rotation[M_node[b]].normalized();
        else M_rest_rotation[b] = ( M_rest_turn[b] * rotation[M_node[b]] ).normalized();
    }
    M_rest_hips = global[node( HumanBone::Hips )].column( 3 ).toVector3D();
    M_hip_height = std::max( 0.01f, M_rest_hips.y() - model.min_y );
    return true;
}

/*-------------------------------------------------------------------*/
float
HumanoidRig::facingAngle() const
{
    if ( ! M_model ) return 0.0f;
    std::vector< QMatrix4x4 > global;
    M_model->globals( M_model->rest, global );
    // the left side: from the right hip's joint to the left's (or the arms')
    QVector3D left = global[node( HumanBone::LeftUpperLeg )].column( 3 ).toVector3D()
        - global[node( HumanBone::RightUpperLeg )].column( 3 ).toVector3D();
    left.setY( 0.0f );
    if ( left.lengthSquared() < 1.0e-8f )
    {
        left = global[node( HumanBone::LeftUpperArm )].column( 3 ).toVector3D()
            - global[node( HumanBone::RightUpperArm )].column( 3 ).toVector3D();
        left.setY( 0.0f );
    }
    if ( left.lengthSquared() < 1.0e-8f ) return 0.0f;
    // facing = left x up: (-z, 0, x) of the left
    return std::atan2( -left.z(), left.x() ) * 180.0f / 3.14159265f;
}

/*-------------------------------------------------------------------*/
QVector3D
HumanoidRig::upAxis() const
{
    if ( ! M_model ) return QVector3D( 0.0f, 1.0f, 0.0f );
    std::vector< QMatrix4x4 > global;
    M_model->globals( M_model->rest, global );
    const QVector3D up = global[node( HumanBone::Head )].column( 3 ).toVector3D()
        - global[node( HumanBone::Hips )].column( 3 ).toVector3D();
    QVector3D axis( 0.0f, 1.0f, 0.0f );
    float best = 0.0f;
    for ( int a = 0; a < 3; ++a )
    {
        if ( std::fabs( up[a] ) > best )
        {
            best = std::fabs( up[a] );
            axis = QVector3D();
            axis[a] = ( up[a] < 0.0f ? -1.0f : 1.0f );
        }
    }
    return axis;
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

/*-------------------------------------------------------------------*/
QQuaternion
HumanoidRig::turnedInModel( const HumanoidPose & pose,
                            const HumanBone bone ) const
{
    const int b = static_cast< int >( bone );
    if ( ! M_model || M_node[b] < 0 ) return QQuaternion();
    QQuaternion turned;
    for ( int r = b; r >= 0; r = M_parent_role[r] )
    {
        turned = pose.rotations[r] * turned;
    }
    return turned;
}

/*-------------------------------------------------------------------*/
void
HumanoidRig::setTurnedInModel( HumanoidPose & pose,
                               const HumanBone bone,
                               const QQuaternion & turned ) const
{
    const int b = static_cast< int >( bone );
    if ( ! M_model || M_node[b] < 0 ) return;
    const int p = M_parent_role[b];
    const QQuaternion above = ( p >= 0 ? turnedInModel( pose, HumanBone( p ) ) : QQuaternion() );
    pose.rotations[b] = ( above.conjugated() * turned ).normalized();
}

/*-------------------------------------------------------------------*/
QString
boneKey( const QString & name )
{
    const int colon = name.lastIndexOf( QLatin1Char( ':' ) );
    return ( colon >= 0 ? name.mid( colon + 1 ) : name );
}

namespace {

//! a clip's time near its end, not looped back to its start
float
endOf( const GltfModel::Clip & clip )
{
    return std::max( 0.0f, clip.duration - std::min( 1.0e-4f, clip.duration * 0.01f ) );
}

}

/*-------------------------------------------------------------------*/
float
clipDrift( const GltfModel & model,
           const HumanoidRig & rig,
           const GltfModel::Clip & clip )
{
    std::vector< NodeTransform > local = model.rest;
    model.sample( clip, 0.0f, local );
    const QVector3D start = rig.poseOf( local ).hips;
    local = model.rest;
    model.sample( clip, endOf( clip ), local );
    const QVector3D drift = rig.poseOf( local ).hips - start;
    return std::hypot( drift.x(), drift.z() );
}

/*-------------------------------------------------------------------*/
GltfModel::Clip
retargetClip( const GltfModel & source,
              const HumanoidRig & source_rig,
              const GltfModel::Clip & clip,
              const GltfModel & target,
              const HumanoidRig & target_rig,
              const bool in_place,
              const float rate )
{
    GltfModel::Clip out;
    out.name = clip.name;
    out.duration = clip.duration;
    out.nodes.resize( target.rest.size() );
    const int steps = std::max( 1, static_cast< int >( std::ceil( clip.duration * rate - 1.0e-3f ) ) );

    // the bones without a role of the same names in both
    std::vector< int > same( target.rest.size(), -1 );
    for ( std::size_t j = 0; j < target.rest.size(); ++j )
    {
        if ( target_rig.roleOf( static_cast< int >( j ) ) >= 0 ) continue;
        const QString key = boneKey( target.node_names[j] );
        if ( key.isEmpty() ) continue;
        for ( std::size_t i = 0; i < source.rest.size(); ++i )
        {
            if ( source_rig.roleOf( static_cast< int >( i ) ) < 0 && boneKey( source.node_names[i] ) == key )
            {
                same[j] = static_cast< int >( i );
                break;
            }
        }
    }

    // the poses at the samples (the last one at the clip's end)
    std::vector< HumanoidPose > poses( steps + 1 );
    std::vector< std::vector< NodeTransform > > locals( steps + 1 );
    for ( int k = 0; k <= steps; ++k )
    {
        locals[k] = source.rest;
        source.sample( clip, ( k == steps ? endOf( clip ) : clip.duration * k / steps ), locals[k] );
        poses[k] = source_rig.poseOf( locals[k] );
    }
    // in place: the hips' horizontal drift taken off in a line (so the end meets the start)
    if ( in_place )
    {
        QVector3D drift = poses[steps].hips - poses[0].hips;
        drift.setY( 0.0f );
        for ( int k = 0; k <= steps; ++k ) poses[k].hips -= drift * ( float( k ) / steps );
    }

    // on the target: the roles' bones, the hips' place, the bones of the same names
    for ( int k = 0; k <= steps; ++k )
    {
        std::vector< NodeTransform > local = target.rest;
        for ( std::size_t j = 0; j < same.size(); ++j )
        {
            if ( same[j] < 0 ) continue;
            const QQuaternion turn = source.rest[same[j]].r.conjugated() * locals[k][same[j]].r;
            local[j].r = ( target.rest[j].r * turn ).normalized();
        }
        target_rig.apply( poses[k], local );
        const float time = clip.duration * k / steps;
        for ( std::size_t j = 0; j < local.size(); ++j )
        {
            const int role = target_rig.roleOf( static_cast< int >( j ) );
            if ( role < 0 && same[j] < 0 ) continue;
            GltfModel::Channel & r = out.nodes[j].r;
            r.times.push_back( time );
            r.values.insert( r.values.end(), { local[j].r.x(), local[j].r.y(), local[j].r.z(), local[j].r.scalar() } );
            if ( role == static_cast< int >( HumanBone::Hips ) )
            {
                GltfModel::Channel & t = out.nodes[j].t;
                t.times.push_back( time );
                t.values.insert( t.values.end(), { local[j].t.x(), local[j].t.y(), local[j].t.z() } );
            }
        }
    }
    return out;
}

/*-------------------------------------------------------------------*/
int
restAs( GltfModel & model,
        const GltfModel & like )
{
    // each bone of the other at rest in the model: its rotation (through
    // its parents) and its place
    std::vector< QQuaternion > like_rotation( like.rest.size() );
    for ( const int i : like.order )
    {
        const int p = like.parent[i];
        like_rotation[i] = ( p >= 0 ? like_rotation[p] : QQuaternion() ) * like.rest[i].r;
    }
    std::vector< QMatrix4x4 > like_global;
    like.globals( like.rest, like_global );

    std::vector< QQuaternion > rotation( model.rest.size() );
    std::vector< QMatrix4x4 > global( model.rest.size() );
    int matched = 0;
    for ( const int i : model.order )
    {
        const int p = model.parent[i];
        const QQuaternion above = ( p >= 0 ? rotation[p] : QQuaternion() );
        const QMatrix4x4 above_global = ( p >= 0 ? global[p] : QMatrix4x4() );
        const QString key = boneKey( model.node_names[i] );
        for ( std::size_t j = 0; j < like.rest.size() && ! key.isEmpty(); ++j )
        {
            if ( boneKey( like.node_names[j] ) == key )
            {
                model.rest[i].r = ( above.conjugated() * like_rotation[j] ).normalized();
                model.rest[i].t = ( above_global.inverted() * like_global[j].column( 3 ) ).toVector3D();
                ++matched;
                break;
            }
        }
        rotation[i] = above * model.rest[i].r;
        global[i] = above_global * model.rest[i].matrix();
    }
    return matched;
}

/*-------------------------------------------------------------------*/
bool
settleCapture( GltfModel & model,
               const HumanoidRig & rig )
{
    const int hips = rig.node( HumanBone::Hips );
    const int left = rig.node( HumanBone::LeftUpperLeg );
    const int right = rig.node( HumanBone::RightUpperLeg );
    if ( hips < 0 || left < 0 || right < 0 ) return false;

    // the bone that places the skeleton: the highest of the hips and the
    // bones above them that a clip moves (a root above the hips, as a
    // reference node or Blender's root, or the hips themselves)
    int mover = hips;
    for ( int n = hips; n >= 0; n = model.parent[n] )
    {
        for ( const GltfModel::Clip & c : model.clips )
        {
            if ( n < static_cast< int >( c.nodes.size() ) && ! c.nodes[n].empty() ) mover = n;
        }
    }
    // the body: the hips and the bones below them (a root may lie anywhere)
    std::vector< bool > body( model.rest.size(), false );
    for ( const int i : model.order )
    {
        body[i] = ( i == hips || ( model.parent[i] >= 0 && body[model.parent[i]] ) );
    }
    const auto lowestOf = [&body]( const std::vector< QMatrix4x4 > & g ) {
        float y = std::numeric_limits< float >::max();
        for ( std::size_t i = 0; i < g.size(); ++i )
        {
            if ( body[i] ) y = std::min( y, g[i]( 1, 3 ) );
        }
        return y;
    };

    std::vector< QMatrix4x4 > global;
    model.globals( model.rest, global );
    const QMatrix4x4 above = ( model.parent[mover] >= 0 ? global[model.parent[mover]] : QMatrix4x4() );
    const QMatrix4x4 into = above.inverted();

    // the rest: the hips above the origin, the body's lowest bone at y = 0
    const QVector3D rest_hips = global[hips].column( 3 ).toVector3D();
    const QVector3D shift( -rest_hips.x(), -lowestOf( global ), -rest_hips.z() );
    model.rest[mover].t += into.mapVector( shift );

    // each clip moving the mover: where the hips start, how low the body
    // comes, which way the hips face; the mover moved and turned so
    QMatrix3x3 r3;
    for ( int col = 0; col < 3; ++col )
    {
        const QVector3D axis = above.column( col ).toVector3D().normalized(); // (the scale off)
        for ( int row = 0; row < 3; ++row ) r3( row, col ) = axis[row];
    }
    const QQuaternion above_turn = QQuaternion::fromRotationMatrix( r3 ).normalized();
    for ( GltfModel::Clip & c : model.clips )
    {
        if ( mover >= static_cast< int >( c.nodes.size() ) || c.nodes[mover].t.empty() ) continue;
        GltfModel::Channel & t = c.nodes[mover].t;
        float floor = std::numeric_limits< float >::max();
        QVector3D start;
        QVector3D side; // the left side, summed across the clip
        std::vector< NodeTransform > local;
        std::vector< QMatrix4x4 > g;
        for ( std::size_t k = 0; k < t.times.size(); ++k )
        {
            local = model.rest;
            model.sample( c, std::min( t.times[k], std::max( 0.0f, c.duration - 1.0e-4f ) ), local );
            model.globals( local, g );
            floor = std::min( floor, lowestOf( g ) );
            if ( k == 0 ) start = g[hips].column( 3 ).toVector3D();
            QVector3D l = ( g[left].column( 3 ) - g[right].column( 3 ) ).toVector3D();
            l.setY( 0.0f );
            if ( l.lengthSquared() > 1.0e-12f ) side += l.normalized();
        }
        // facing = left x up: (-z, 0, x) of the left; turned back to +z
        const float facing = ( side.lengthSquared() > 1.0e-12f
                               ? std::atan2( -side.z(), side.x() ) * 180.0f / 3.14159265f
                               : 0.0f );
        const QQuaternion turn = QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, -facing );
        const QVector3D from( start.x(), floor, start.z() );
        for ( std::size_t k = 0; k < t.times.size(); ++k )
        {
            const QVector3D at = above.map( QVector3D( t.values[k * 3], t.values[k * 3 + 1], t.values[k * 3 + 2] ) );
            const QVector3D back = into.map( turn.rotatedVector( at - from ) );
            t.values[k * 3] = back.x();
            t.values[k * 3 + 1] = back.y();
            t.values[k * 3 + 2] = back.z();
        }
        // the mover's rotation in the model turned too: above * r -> turn * above * r
        GltfModel::Channel & r = c.nodes[mover].r;
        const QQuaternion change = ( above_turn.conjugated() * turn * above_turn ).normalized();
        if ( r.empty() && ! turn.isIdentity() )
        {
            // keys of the rest's rotation, turned (at the translation's times)
            r.times = t.times;
            r.values.clear();
            const QQuaternion q = ( change * model.rest[mover].r ).normalized();
            for ( std::size_t k = 0; k < t.times.size(); ++k ) r.values.insert( r.values.end(), { q.x(), q.y(), q.z(), q.scalar() } );
        }
        else
        {
            for ( std::size_t k = 0; k < r.times.size(); ++k )
            {
                const QQuaternion q = ( change * QQuaternion( r.values[k * 4 + 3], r.values[k * 4], r.values[k * 4 + 1], r.values[k * 4 + 2] ) ).normalized();
                r.values[k * 4] = q.x();
                r.values[k * 4 + 1] = q.y();
                r.values[k * 4 + 2] = q.z();
                r.values[k * 4 + 3] = q.scalar();
            }
        }
    }
    return true;
}
