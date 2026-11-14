// -*-c++-*-

/*!
  \file figure_uniform.cpp
  \brief the uniform painted on a human body. Source File.
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

#include "figure_uniform.h"

#include "gltf_model.h"
#include "humanoid.h"

#include <QMatrix4x4>

#include <vector>

namespace FigureUniform {

namespace {

//! the part of the body of a role
Part
partOf( const HumanBone bone )
{
    switch ( bone ) {
    case HumanBone::Head:
        return Part::Head;
    case HumanBone::LeftUpperArm:
    case HumanBone::LeftLowerArm:
    case HumanBone::LeftHand:
        return Part::LeftArm;
    case HumanBone::RightUpperArm:
    case HumanBone::RightLowerArm:
    case HumanBone::RightHand:
        return Part::RightArm;
    case HumanBone::LeftUpperLeg:
    case HumanBone::LeftLowerLeg:
    case HumanBone::LeftFoot:
    case HumanBone::LeftToes:
        return Part::LeftLeg;
    case HumanBone::RightUpperLeg:
    case HumanBone::RightLowerLeg:
    case HumanBone::RightFoot:
    case HumanBone::RightToes:
        return Part::RightLeg;
    default:
        // the hips, the spine, the chest, the neck and the shoulders
        return Part::Torso;
    }
}

}

/*-------------------------------------------------------------------*/
QVector4D
planeAt( const QVector3D & point,
         const QVector3D & toward )
{
    const QVector3D n = toward.normalized();
    return QVector4D( n, -QVector3D::dotProduct( n, point ) );
}

/*-------------------------------------------------------------------*/
void
paintParts( GltfModel & model,
            const HumanoidRig & rig )
{
    // the part of each node: of its role, or of the nearest role above it
    const int nodes = static_cast< int >( model.node_names.size() );
    std::vector< int > role_of( nodes, -1 );
    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
    {
        const int n = rig.node( static_cast< HumanBone >( b ) );
        if ( n >= 0 && n < nodes ) role_of[n] = b;
    }
    std::vector< Part > part_of( nodes, Part::Torso );
    for ( int i = 0; i < nodes; ++i )
    {
        int j = i;
        while ( j >= 0 && role_of[j] < 0 ) j = model.parent[j];
        part_of[i] = ( j >= 0 ? partOf( static_cast< HumanBone >( role_of[j] ) ) : Part::Torso );
    }

    // each vertex on the part whose bones move it most
    for ( SkinVertex & v : model.vertices )
    {
        float sum[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        for ( int k = 0; k < 4; ++k )
        {
            if ( v.joints[k] < model.joint_nodes.size() )
            {
                sum[static_cast< int >( part_of[model.joint_nodes[v.joints[k]]] )] += v.weights[k];
            }
        }
        int best = 0;
        for ( int p = 1; p < 6; ++p )
        {
            if ( sum[p] > sum[best] ) best = p;
        }
        v.part = static_cast< float >( best );
    }
}

/*-------------------------------------------------------------------*/
Cuts
cutsOf( const GltfModel & model,
        const HumanoidRig & rig )
{
    std::vector< QMatrix4x4 > global;
    model.globals( model.rest, global );
    auto at = [&]( const HumanBone b )
        {
            const int n = rig.node( b );
            return ( n >= 0 ? global[n].column( 3 ).toVector3D() : QVector3D() );
        };
    auto between = [&]( const HumanBone from,
                        const HumanBone to,
                        const float t )
        {
            return at( from ) + ( at( to ) - at( from ) ) * t;
        };

    Cuts c;
    const HumanBone upper_arm[2] = { HumanBone::LeftUpperArm, HumanBone::RightUpperArm };
    const HumanBone lower_arm[2] = { HumanBone::LeftLowerArm, HumanBone::RightLowerArm };
    const HumanBone upper_leg[2] = { HumanBone::LeftUpperLeg, HumanBone::RightUpperLeg };
    const HumanBone lower_leg[2] = { HumanBone::LeftLowerLeg, HumanBone::RightLowerLeg };
    const HumanBone foot[2] = { HumanBone::LeftFoot, HumanBone::RightFoot };
    for ( int s = 0; s < 2; ++s )
    {
        const QVector3D arm = at( lower_arm[s] ) - at( upper_arm[s] );
        const QVector3D thigh = at( lower_leg[s] ) - at( upper_leg[s] );
        const QVector3D shin = at( foot[s] ) - at( lower_leg[s] );
        c.planes[Cuts::SLEEVE_LEFT + s] = planeAt( between( upper_arm[s], lower_arm[s], SLEEVE ), arm );
        c.planes[Cuts::HEM_LEFT + s] = planeAt( between( upper_leg[s], lower_leg[s], HEM ), thigh );
        c.planes[Cuts::SOCK_LEFT + s] = planeAt( between( lower_leg[s], foot[s], SOCK ), shin );
        c.planes[Cuts::SHOE_LEFT + s] = planeAt( between( foot[s], lower_leg[s], SHOE ), shin );
    }
    // the collar at the neck's root, across the neck toward the head
    c.planes[Cuts::COLLAR] = planeAt( at( HumanBone::Neck ), at( HumanBone::Head ) - at( HumanBone::Neck ) );
    // the waist above the hips, the shorts below it
    c.planes[Cuts::WAIST] = planeAt( between( HumanBone::Hips, HumanBone::Spine, WAIST ),
                                     at( HumanBone::Hips ) - at( HumanBone::Spine ) );
    return c;
}

/*-------------------------------------------------------------------*/
Region
regionAt( const Cuts & cuts,
          const Part part,
          const QVector3D & rest )
{
    switch ( part ) {
    case Part::Head:
        return Region::Skin;
    case Part::LeftArm:
    case Part::RightArm:
        {
            const int s = ( part == Part::LeftArm ? 0 : 1 );
            return ( cuts.beyond( Cuts::SLEEVE_LEFT + s, rest ) > 0.0f ? Region::Skin : Region::Shirt );
        }
    case Part::LeftLeg:
    case Part::RightLeg:
        {
            const int s = ( part == Part::LeftLeg ? 0 : 1 );
            if ( cuts.beyond( Cuts::HEM_LEFT + s, rest ) <= 0.0f ) return Region::Shorts;
            if ( cuts.beyond( Cuts::SOCK_LEFT + s, rest ) <= 0.0f ) return Region::Skin;
            if ( cuts.beyond( Cuts::SHOE_LEFT + s, rest ) <= 0.0f ) return Region::Socks;
            return Region::Shoes;
        }
    case Part::Torso:
    default:
        if ( cuts.beyond( Cuts::COLLAR, rest ) > 0.0f ) return Region::Skin;
        if ( cuts.beyond( Cuts::WAIST, rest ) > 0.0f ) return Region::Shorts;
        return Region::Shirt;
    }
}

/*-------------------------------------------------------------------*/
QVector4D
colorOf( const Region region,
         const QVector4D & color )
{
    switch ( region ) {
    case Region::Skin:
        return SKIN_COLOR;
    case Region::Shorts:
        return QVector4D( color.toVector3D() * SHORTS_SHADE, color.w() );
    case Region::Shoes:
        return SHOE_COLOR;
    case Region::Shirt:
    case Region::Socks:
    default:
        return color;
    }
}

}
