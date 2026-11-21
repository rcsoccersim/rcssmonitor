// -*-c++-*-

/*!
  \file test_bvh.h
  \brief BVH files of the tests, written from a glTF model's clip: the
  bones of the humanoid roles, as motion capture writes them.
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

#ifndef RCSSMONITOR_TEST_BVH_H
#define RCSSMONITOR_TEST_BVH_H

#include "gltf_model.h"
#include "humanoid.h"

#include <QByteArray>
#include <QMatrix3x3>
#include <QQuaternion>
#include <QString>
#include <QVector3D>

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <vector>

namespace TestBvh {

/*!
  \struct Options
  \brief how bvhOf() writes a file
*/
struct Options {
    QString preset = QStringLiteral( "mixamo" ); //!< the bones' names (skeletonBoneNames())
    bool z_up = false; //!< z up (and facing -y), as some tools write; else y up, facing +z
    float facing = 0.0f; //!< turned so far about the up axis (degree)
    QQuaternion turn; //!< the axes turned so after that (a quarter turn about +z: up +x or -x)
    float scale = 1.0f; //!< the unit: 1 (m), 100 (cm)
    QString order = QStringLiteral( "ZXY" ); //!< the rotation channels: ZXY or XYZ
    bool positions = false; //!< every joint with position channels (their OFFSETs), not the root only
    bool origin_root = false; //!< the root's OFFSET at the origin (the legs below it), as motion capture writes
    QVector3D room; //!< the motion moved so (m, the model's axes): where it was taken in the room
    float heading = 0.0f; //!< the motion turned so about the model's up (degree): the way it went in the room
    bool reference = false; //!< a root "Reference" above the hips (at the origin) placing the motion in the room; the hips moved under it
    bool reference_moves_only = false; //!< the Reference with position channels only (the hips turned to the room's heading)
    float fps = 30.0f;
};

//! angles (degree) of a rotation R = Ra Rb Rc for the order abc (ZXY or XYZ)
inline
std::array< float, 3 >
eulerOf( const QQuaternion & q,
         const QString & order )
{
    const QMatrix3x3 m = q.toRotationMatrix();
    const auto deg = []( const float r ) { return r * 180.0f / 3.14159265f; };
    const auto clamp = []( const float v ) { return std::max( -1.0f, std::min( 1.0f, v ) ); };
    if ( order == QLatin1String( "XYZ" ) )
    {
        // m02 = sy; m12 = -sx cy, m22 = cx cy; m01 = -cy sz, m00 = cy cz
        return { deg( std::atan2( -m( 1, 2 ), m( 2, 2 ) ) ), deg( std::asin( clamp( m( 0, 2 ) ) ) ),
                 deg( std::atan2( -m( 0, 1 ), m( 0, 0 ) ) ) };
    }
    // ZXY: m21 = sx; m01 = -sz cx, m11 = cz cx; m20 = -cx sy, m22 = cx cy
    return { deg( std::atan2( -m( 0, 1 ), m( 1, 1 ) ) ), deg( std::asin( clamp( m( 2, 1 ) ) ) ),
             deg( std::atan2( -m( 2, 0 ), m( 2, 2 ) ) ) };
}

/*!
  \brief a BVH file of a clip of a model: the bones of its humanoid roles
  (by the Mannequin's names), at rest where the model has them, not turned
  (a T pose, as the Mannequin's); each frame turns them as the clip turns
  the model's bones, and the root (the hips) moves as the clip's hips.
  End Sites after the head, the hands and the toes.
  \param model a model with the Mannequin's bones (quaterniusBoneNames())
  \param clip the clip (null: no motion)
*/
inline
QByteArray
bvhOf( const GltfModel & model,
       const GltfModel::Clip * clip,
       const Options & o )
{
    HumanoidRig rig;
    rig.build( model, quaterniusBoneNames() );
    HumanBoneNames names;
    skeletonBoneNames( o.preset, &names );

    // the roles, each with the nearest role above it
    std::vector< int > roles;
    std::array< int, HUMAN_BONE_COUNT > above;
    above.fill( -1 );
    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
    {
        if ( rig.node( HumanBone( b ) ) < 0 ) continue;
        roles.push_back( b );
        for ( int n = model.parent[rig.node( HumanBone( b ) )]; n >= 0 && above[b] < 0; n = model.parent[n] )
        {
            for ( int c = 0; c < HUMAN_BONE_COUNT; ++c )
            {
                if ( rig.node( HumanBone( c ) ) == n ) above[b] = c;
            }
        }
    }

    // from the model's axes to the file's
    const QQuaternion axes = o.turn * QQuaternion::fromAxisAndAngle( o.z_up ? QVector3D( 0.0f, 0.0f, 1.0f ) : QVector3D( 0.0f, 1.0f, 0.0f ), o.facing )
        * ( o.z_up ? QQuaternion::fromAxisAndAngle( 1.0f, 0.0f, 0.0f, 90.0f ) : QQuaternion() );
    const auto place = [&]( const QVector3D & v ) { return axes.rotatedVector( v ) * o.scale; };
    const auto rotationOf = []( const QMatrix4x4 & g ) {
        QMatrix3x3 r;
        for ( int i = 0; i < 3; ++i )
        {
            for ( int j = 0; j < 3; ++j ) r( i, j ) = g( i, j );
        }
        // remove the scale from each column
        for ( int c = 0; c < 3; ++c )
        {
            const float len = std::sqrt( r( 0, c ) * r( 0, c ) + r( 1, c ) * r( 1, c ) + r( 2, c ) * r( 2, c ) );
            for ( int k = 0; k < 3; ++k ) r( k, c ) /= len;
        }
        return QQuaternion::fromRotationMatrix( r ).normalized();
    };

    std::vector< QMatrix4x4 > rest_global;
    model.globals( model.rest, rest_global );
    std::array< QVector3D, HUMAN_BONE_COUNT > at;
    std::array< QQuaternion, HUMAN_BONE_COUNT > rest_turn;
    for ( const int b : roles )
    {
        at[b] = rest_global[rig.node( HumanBone( b ) )].column( 3 ).toVector3D();
        rest_turn[b] = rotationOf( rest_global[rig.node( HumanBone( b ) )] );
    }

    // the hierarchy
    QByteArray out = "HIERARCHY\n";
    const QByteArray order = o.order.toLatin1();
    const auto rotations = [&order]() {
        QByteArray r;
        for ( const char c : order ) r += ' ' + QByteArray( 1, c ) + "rotation";
        return r;
    };
    const auto num = []( const float v ) { return QByteArray::number( double( v ), 'f', 6 ); };
    std::vector< int > written; // the roles in the order of their channels
    const std::function< void( int, int ) > write = [&]( const int b, const int depth ) {
        const QByteArray pad( depth * 2, ' ' );
        const QVector3D offset = place( above[b] >= 0 ? at[b] - at[above[b]] : o.origin_root ? QVector3D() : at[b] );
        out += pad + ( above[b] < 0 && ! o.reference ? "ROOT " : "JOINT " ) + names[b].toLatin1() + "\n" + pad + "{\n";
        out += pad + "  OFFSET " + num( offset.x() ) + ' ' + num( offset.y() ) + ' ' + num( offset.z() ) + "\n";
        if ( above[b] < 0 || o.positions ) out += pad + "  CHANNELS 6 Xposition Yposition Zposition" + rotations() + "\n";
        else out += pad + "  CHANNELS 3" + rotations() + "\n";
        written.push_back( b );
        bool leaf = true;
        for ( const int c : roles )
        {
            if ( above[c] == b )
            {
                write( c, depth + 1 );
                leaf = false;
            }
        }
        if ( leaf )
        {
            const QVector3D end = place( QVector3D( 0.0f, 0.05f, 0.0f ) );
            out += pad + "  End Site\n" + pad + "  {\n" + pad + "    OFFSET " + num( end.x() ) + ' ' + num( end.y() ) + ' ' + num( end.z() ) + "\n" + pad + "  }\n";
        }
        out += pad + "}\n";
    };
    if ( o.reference )
    {
        out += "ROOT Reference\n{\n  OFFSET 0.000000 0.000000 0.000000\n  CHANNELS "
            + ( o.reference_moves_only ? QByteArray( "3 Xposition Yposition Zposition" ) : "6 Xposition Yposition Zposition" + rotations() ) + "\n";
    }
    for ( const int b : roles )
    {
        if ( above[b] < 0 ) write( b, o.reference ? 1 : 0 );
    }
    if ( o.reference ) out += "}\n";

    // the motion
    const int frames = ( clip ? int( std::round( clip->duration * o.fps ) ) + 1 : 0 );
    out += "MOTION\nFrames: " + QByteArray::number( frames ) + "\nFrame Time: " + num( 1.0f / o.fps ) + "\n";
    for ( int f = 0; f < frames; ++f )
    {
        std::vector< NodeTransform > local = model.rest;
        model.sample( *clip, std::min( f / o.fps, clip->duration - 1.0e-4f ), local );
        std::vector< QMatrix4x4 > global;
        model.globals( local, global );
        // how each role turned from rest, in the file's axes (the motion turned in the room)
        const QQuaternion room_turn = QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, o.heading );
        std::array< QQuaternion, HUMAN_BONE_COUNT > turned;
        for ( const int b : roles )
        {
            turned[b] = axes * room_turn * rotationOf( global[rig.node( HumanBone( b ) )] ) * rest_turn[b].conjugated() * axes.conjugated();
        }
        QByteArray line;
        // the Reference: the room's place and heading; the hips under it
        const QQuaternion reference = ( o.reference_moves_only ? QQuaternion() : axes * room_turn * axes.conjugated() );
        if ( o.reference )
        {
            const QVector3D p = place( o.room );
            line += num( p.x() ) + ' ' + num( p.y() ) + ' ' + num( p.z() ) + ' ';
            if ( ! o.reference_moves_only )
            {
                const std::array< float, 3 > e = eulerOf( reference, o.order );
                line += num( e[0] ) + ' ' + num( e[1] ) + ' ' + num( e[2] ) + ' ';
            }
        }
        for ( const int b : written )
        {
            const QQuaternion l = ( above[b] >= 0 ? turned[above[b]].conjugated() * turned[b]
                                    : o.reference ? reference.conjugated() * turned[b]
                                    : turned[b] );
            if ( above[b] < 0 || o.positions )
            {
                const QVector3D hips_at = global[rig.node( HumanBone( b ) )].column( 3 ).toVector3D();
                const QVector3D p = ( above[b] >= 0 ? place( at[b] - at[above[b]] )
                                      : o.reference_moves_only ? place( room_turn.rotatedVector( hips_at ) )
                                      : o.reference ? place( hips_at )
                                      : place( room_turn.rotatedVector( hips_at ) + o.room ) );
                line += num( p.x() ) + ' ' + num( p.y() ) + ' ' + num( p.z() ) + ' ';
            }
            const std::array< float, 3 > e = eulerOf( l, o.order );
            line += num( e[0] ) + ' ' + num( e[1] ) + ' ' + num( e[2] ) + ' ';
        }
        out += line.trimmed() + "\n";
    }
    return out;
}

}

#endif
