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
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include <vector>

namespace FigureUniform {

namespace {

//! the part of the body of a role
Part
partOf( const HumanBone bone )
{
    switch ( bone ) {
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
        // the hips, the spine, the chest, the neck, the head and the shoulders
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

    // how much the arms' and the legs' bones move each vertex
    for ( SkinVertex & v : model.vertices )
    {
        v.part[0] = v.part[1] = 0.0f;
        for ( int k = 0; k < 4; ++k )
        {
            if ( v.joints[k] >= model.joint_nodes.size() ) continue;
            const Part p = part_of[model.joint_nodes[v.joints[k]]];
            if ( p == Part::LeftArm || p == Part::RightArm ) v.part[0] += v.weights[k];
            if ( p == Part::LeftLeg || p == Part::RightLeg ) v.part[1] += v.weights[k];
        }
    }
}

/*-------------------------------------------------------------------*/
Cuts
cutsOf( const GltfModel & model,
        const HumanoidRig & rig,
        const QStringList & skin )
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
    // The collar round the neck: the skin within its width above the neck's
    // root (a little below it at most), and all above the chin. The neck's
    // middle line through the middle of the neck's skin (the bones are
    // toward its back), along the bones; its width the median distance of
    // the skin from it (the trapezius may reach the neck's middle at the
    // back: measured higher, and not by the farthest).
    const QVector3D neck = at( HumanBone::Neck );
    const QVector3D up = at( HumanBone::Head ) - neck;
    const float length = up.length();
    const QVector3D axis = up.normalized();
    c.planes[Cuts::COLLAR] = planeAt( neck - axis * COLLAR_DROP, axis );
    c.planes[Cuts::CHIN] = planeAt( neck + axis * ( length * CHIN ), axis );
    QVector3D middle = neck + axis * ( length * NECK_MEASURED );
    float radius = 0.0f;
    {
        // the skin's points across the neck's middle, but the arms' and the legs'
        std::vector< QVector3D > ring;
        for ( const SkinPrimitive & p : model.primitives )
        {
            if ( ! skin.contains( p.material ) ) continue;
            for ( int i = p.first_index; i < p.first_index + p.index_count; ++i )
            {
                const SkinVertex & v = model.vertices[model.indices[i]];
                const QVector3D x( v.rest[0], v.rest[1], v.rest[2] );
                if ( v.part[0] < 0.5f && v.part[1] < 0.5f
                     && std::fabs( QVector3D::dotProduct( x - middle, axis ) ) < 0.01f * length / 0.08f )
                {
                    ring.push_back( x );
                }
            }
        }
        if ( ! ring.empty() )
        {
            QVector3D sum;
            for ( const QVector3D & x : ring ) sum += x;
            middle = sum / float( ring.size() );
            std::vector< float > distances;
            for ( const QVector3D & x : ring )
            {
                const QVector3D d = x - middle;
                distances.push_back( ( d - axis * QVector3D::dotProduct( d, axis ) ).length() );
            }
            std::nth_element( distances.begin(), distances.begin() + distances.size() / 2, distances.end() );
            radius = distances[distances.size() / 2];
        }
    }
    c.planes[Cuts::NECK] = QVector4D( middle, radius * COLLAR_WIDTH );
    // the waist above the hips, the shorts below it
    c.planes[Cuts::WAIST] = planeAt( between( HumanBone::Hips, HumanBone::Spine, WAIST ),
                                     at( HumanBone::Hips ) - at( HumanBone::Spine ) );
    // the left side, beyond the plane halfway between the shoulders
    c.planes[Cuts::LEFT] = planeAt( between( HumanBone::LeftUpperArm, HumanBone::RightUpperArm, 0.5f ),
                                    at( HumanBone::LeftUpperArm ) - at( HumanBone::RightUpperArm ) );
    return c;
}

/*-------------------------------------------------------------------*/
namespace {

//! the nearest point of a triangle to a point (Ericson, Real-Time Collision Detection 5.1.5)
QVector3D
nearestOnTriangle( const QVector3D & p,
                   const QVector3D & a,
                   const QVector3D & b,
                   const QVector3D & c )
{
    const QVector3D ab = b - a, ac = c - a, ap = p - a;
    const float d1 = QVector3D::dotProduct( ab, ap ), d2 = QVector3D::dotProduct( ac, ap );
    if ( d1 <= 0.0f && d2 <= 0.0f ) return a;
    const QVector3D bp = p - b;
    const float d3 = QVector3D::dotProduct( ab, bp ), d4 = QVector3D::dotProduct( ac, bp );
    if ( d3 >= 0.0f && d4 <= d3 ) return b;
    const float vc = d1 * d4 - d3 * d2;
    if ( vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f ) return a + ab * ( d1 / ( d1 - d3 ) );
    const QVector3D cp = p - c;
    const float d5 = QVector3D::dotProduct( ab, cp ), d6 = QVector3D::dotProduct( ac, cp );
    if ( d6 >= 0.0f && d5 <= d6 ) return c;
    const float vb = d5 * d2 - d1 * d6;
    if ( vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f ) return a + ac * ( d2 / ( d2 - d6 ) );
    const float va = d3 * d6 - d5 * d4;
    if ( va <= 0.0f && ( d4 - d3 ) >= 0.0f && ( d5 - d6 ) >= 0.0f )
    {
        return b + ( c - b ) * ( ( d4 - d3 ) / ( ( d4 - d3 ) + ( d5 - d6 ) ) );
    }
    const float denom = 1.0f / ( va + vb + vc );
    return a + ab * ( vb * denom ) + ac * ( vc * denom );
}

QVector3D
restOf( const SkinVertex & v )
{
    return QVector3D( v.rest[0], v.rest[1], v.rest[2] );
}

}

/*-------------------------------------------------------------------*/
void
paintScalp( GltfModel & model,
            const QStringList & skin,
            const std::vector< std::pair< QString, std::uint32_t > > & hairs,
            Cuts * cuts )
{
    for ( SkinVertex & v : model.vertices )
    {
        for ( float & s : v.scalp ) s = 0.0f;
    }
    // the vertices of the skin
    std::vector< std::uint32_t > skin_vertices;
    {
        std::vector< bool > seen( model.vertices.size(), false );
        for ( const SkinPrimitive & p : model.primitives )
        {
            if ( ! skin.contains( p.material ) ) continue;
            for ( int i = p.first_index; i < p.first_index + p.index_count; ++i )
            {
                const std::uint32_t k = model.indices[i];
                if ( ! seen[k] )
                {
                    seen[k] = true;
                    skin_vertices.push_back( k );
                }
            }
        }
    }

    const int count = std::min( SCALP_HAIRS, static_cast< int >( hairs.size() ) );
    for ( int slot = 0; slot < count; ++slot )
    {
        if ( cuts ) cuts->scalp[slot] = hairs[slot].second;
        // the hairstyle's triangles, in the cells of a grid (SCALP wide)
        // that their boxes, SCALP larger, touch: a point is looked for in
        // its own cell only
        std::vector< QVector3D > tri;
        for ( const SkinPrimitive & p : model.primitives )
        {
            if ( p.material != hairs[slot].first ) continue;
            for ( int i = p.first_index; i < p.first_index + p.index_count; ++i )
            {
                tri.push_back( restOf( model.vertices[model.indices[i]] ) );
            }
        }
        if ( tri.size() < 3 ) continue;
        const auto cell = []( const float x ) { return static_cast< long long >( std::floor( x / SCALP ) ); };
        const auto key = []( const long long x, const long long y, const long long z ) {
            return ( ( x & 0xfffff ) << 40 ) | ( ( y & 0xfffff ) << 20 ) | ( z & 0xfffff );
        };
        std::unordered_map< long long, std::vector< std::size_t > > grid;
        for ( std::size_t t = 0; t + 2 < tri.size(); t += 3 )
        {
            long long low[3], high[3];
            for ( int a = 0; a < 3; ++a )
            {
                const float lo = std::min( { tri[t][a], tri[t + 1][a], tri[t + 2][a] } ) - SCALP;
                const float hi = std::max( { tri[t][a], tri[t + 1][a], tri[t + 2][a] } ) + SCALP;
                low[a] = cell( lo );
                high[a] = cell( hi );
            }
            for ( long long x = low[0]; x <= high[0]; ++x )
                for ( long long y = low[1]; y <= high[1]; ++y )
                    for ( long long z = low[2]; z <= high[2]; ++z )
                        grid[key( x, y, z )].push_back( t );
        }
        for ( const std::uint32_t k : skin_vertices )
        {
            SkinVertex & v = model.vertices[k];
            const QVector3D p = restOf( v );
            const auto it = grid.find( key( cell( p.x() ), cell( p.y() ), cell( p.z() ) ) );
            if ( it == grid.end() ) continue;
            // the skin's normal (the vertex is at rest where it is skinned
            // with the rest pose; a bundled body is bound at rest)
            const QVector3D n = QVector3D( v.normal[0], v.normal[1], v.normal[2] ).normalized();
            for ( const std::size_t t : it->second )
            {
                const QVector3D to = nearestOnTriangle( p, tri[t], tri[t + 1], tri[t + 2] ) - p;
                const float d = to.length();
                if ( d < SCALP
                     && ( d < SCALP_TOUCH || QVector3D::dotProduct( to, n ) > 0.5f * d ) )
                {
                    v.scalp[slot] = 1.0f;
                    break;
                }
            }
        }
    }
}

/*-------------------------------------------------------------------*/
Part
partAt( const Cuts & cuts,
        const float arm,
        const float leg,
        const QVector3D & rest )
{
    const bool left = ( cuts.beyond( Cuts::LEFT, rest ) >= 0.0f );
    if ( arm > 0.5f ) return ( left ? Part::LeftArm : Part::RightArm );
    if ( leg > 0.5f ) return ( left ? Part::LeftLeg : Part::RightLeg );
    return Part::Torso;
}

/*-------------------------------------------------------------------*/
Region
regionAt( const Cuts & cuts,
          const Part part,
          const QVector3D & rest )
{
    switch ( part ) {
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
        if ( cuts.inCollar( rest ) ) return Region::Skin;
        if ( cuts.beyond( Cuts::WAIST, rest ) > 0.0f ) return Region::Shorts;
        return Region::Shirt;
    }
}

/*-------------------------------------------------------------------*/
QVector4D
colorOf( const Region region,
         const QVector4D & color,
         const QVector4D & skin )
{
    switch ( region ) {
    case Region::Skin:
        return skin;
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
