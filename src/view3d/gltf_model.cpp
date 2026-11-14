// -*-c++-*-

/*!
  \file gltf_model.cpp
  \brief a skinned glTF model and its clips, read with cgltf. Source File.
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

// the implementation of cgltf (third_party/cgltf), here only
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "gltf_model.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>

namespace {

NodeTransform
node_transform( const cgltf_node & n )
{
    NodeTransform t;
    if ( n.has_translation ) t.t = QVector3D( n.translation[0], n.translation[1], n.translation[2] );
    // glTF writes x, y, z, w; QQuaternion takes w first
    if ( n.has_rotation ) t.r = QQuaternion( n.rotation[3], n.rotation[0], n.rotation[1], n.rotation[2] );
    if ( n.has_scale ) t.s = QVector3D( n.scale[0], n.scale[1], n.scale[2] );
    return t;
}

/*!
  \brief the value of a channel at a time, n floats a key: linear between
  the keys (the shorter way for a rotation), held for a step channel, and
  the end keys outside them
*/
void
sample_channel( const GltfModel::Channel & c,
                const float time,
                const int n,
                float * out )
{
    const std::vector< float > & ts = c.times;
    if ( ts.size() == 1 || time <= ts.front() )
    {
        std::copy_n( c.values.begin(), n, out );
        return;
    }
    if ( time >= ts.back() )
    {
        std::copy_n( c.values.end() - n, n, out );
        return;
    }
    // ts[i - 1] <= time < ts[i]
    const std::size_t i = std::upper_bound( ts.begin(), ts.end(), time ) - ts.begin();
    const float * a = &c.values[( i - 1 ) * n];
    if ( c.step )
    {
        std::copy_n( a, n, out );
        return;
    }
    const float * b = &c.values[i * n];
    const float u = ( time - ts[i - 1] ) / ( ts[i] - ts[i - 1] );
    if ( n == 4 )
    {
        // the keys close together: a normalized linear mix, the shorter way
        const float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
        const float sign = ( dot < 0.0f ? -1.0f : 1.0f );
        float length = 0.0f;
        for ( int k = 0; k < 4; ++k )
        {
            out[k] = a[k] + ( sign * b[k] - a[k] ) * u;
            length += out[k] * out[k];
        }
        length = std::sqrt( length );
        for ( int k = 0; k < 4; ++k ) out[k] /= length;
        return;
    }
    for ( int k = 0; k < n; ++k )
    {
        out[k] = a[k] + ( b[k] - a[k] ) * u;
    }
}

//! cgltf's data, freed when it goes
struct DataDeleter {
    void operator()( cgltf_data * data ) const { cgltf_free( data ); }
};

}

/*-------------------------------------------------------------------*/
bool
GltfModel::load( const QByteArray & bytes,
                 QString * error )
{
    const auto fail = [error]( const QString & what ) {
        if ( error ) *error = what;
        return false;
    };

    cgltf_options options{};
    cgltf_data * raw = nullptr;
    if ( cgltf_parse( &options, bytes.constData(), static_cast< cgltf_size >( bytes.size() ), &raw )
         != cgltf_result_success )
    {
        return fail( QStringLiteral( "not a glTF file" ) );
    }
    std::unique_ptr< cgltf_data, DataDeleter > data( raw );
    // the buffers inside the .glb (no file of their own)
    if ( cgltf_load_buffers( &options, data.get(), nullptr ) != cgltf_result_success
         || cgltf_validate( data.get() ) != cgltf_result_success )
    {
        return fail( QStringLiteral( "broken glTF data" ) );
    }
    if ( data->skins_count < 1 )
    {
        return fail( QStringLiteral( "no skin" ) );
    }

    const auto node_index = [&data]( const cgltf_node * n ) { return static_cast< int >( n - data->nodes ); };

    // the nodes, and an order with the parents first
    const int node_count = static_cast< int >( data->nodes_count );
    node_names.assign( node_count, QString() );
    parent.assign( node_count, -1 );
    rest.assign( node_count, NodeTransform() );
    for ( int i = 0; i < node_count; ++i )
    {
        const cgltf_node & n = data->nodes[i];
        node_names[i] = QString::fromUtf8( n.name ? n.name : "" );
        rest[i] = node_transform( n );
        if ( n.parent ) parent[i] = node_index( n.parent );
    }
    order.clear();
    std::vector< bool > done( node_count, false );
    const std::function< void( int ) > visit = [&]( int i ) {
        if ( done[i] ) return;
        if ( parent[i] >= 0 ) visit( parent[i] );
        done[i] = true;
        order.push_back( i );
    };
    for ( int i = 0; i < node_count; ++i ) visit( i );

    // the skin
    const cgltf_skin & skin_data = data->skins[0];
    joint_nodes.clear();
    inverse_bind.clear();
    for ( cgltf_size j = 0; j < skin_data.joints_count; ++j )
    {
        joint_nodes.push_back( node_index( skin_data.joints[j] ) );
        float m[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        if ( skin_data.inverse_bind_matrices )
        {
            cgltf_accessor_read_float( skin_data.inverse_bind_matrices, j, m, 16 );
        }
        // glTF writes column by column; Qt's constructor takes the rows
        inverse_bind.push_back( QMatrix4x4( m ).transposed() );
    }
    if ( joint_nodes.empty()
         || joint_nodes.size() > std::numeric_limits< std::uint16_t >::max() )
    {
        return fail( QStringLiteral( "no joints" ) );
    }

    // the meshes skinned with it
    vertices.clear();
    indices.clear();
    primitives.clear();
    for ( int i = 0; i < node_count; ++i )
    {
        const cgltf_node & node = data->nodes[i];
        if ( ! node.mesh || node.skin != &skin_data ) continue;

        for ( cgltf_size p = 0; p < node.mesh->primitives_count; ++p )
        {
            const cgltf_primitive & prim = node.mesh->primitives[p];
            if ( prim.type != cgltf_primitive_type_triangles ) continue;
            const cgltf_accessor * position = nullptr;
            const cgltf_accessor * normal = nullptr;
            const cgltf_accessor * joints = nullptr;
            const cgltf_accessor * weights = nullptr;
            for ( cgltf_size a = 0; a < prim.attributes_count; ++a )
            {
                const cgltf_attribute & at = prim.attributes[a];
                if ( at.type == cgltf_attribute_type_position ) position = at.data;
                else if ( at.type == cgltf_attribute_type_normal ) normal = at.data;
                else if ( at.type == cgltf_attribute_type_joints && at.index == 0 ) joints = at.data;
                else if ( at.type == cgltf_attribute_type_weights && at.index == 0 ) weights = at.data;
            }
            if ( ! position || ! normal || ! joints || ! weights ) continue;

            const std::uint32_t base = static_cast< std::uint32_t >( vertices.size() );
            for ( cgltf_size v = 0; v < position->count; ++v )
            {
                SkinVertex sv;
                cgltf_accessor_read_float( position, v, sv.position, 3 );
                cgltf_accessor_read_float( normal, v, sv.normal, 3 );
                cgltf_uint j[4] = { 0, 0, 0, 0 };
                cgltf_accessor_read_uint( joints, v, j, 4 );
                cgltf_accessor_read_float( weights, v, sv.weights, 4 );
                float sum = 0.0f;
                for ( int k = 0; k < 4; ++k )
                {
                    if ( j[k] >= joint_nodes.size() ) // not a joint: no weight
                    {
                        j[k] = 0;
                        sv.weights[k] = 0.0f;
                    }
                    sv.joints[k] = static_cast< std::uint16_t >( j[k] );
                    sum += sv.weights[k];
                }
                // the weights sum to 1 (quantized ones may not quite)
                for ( int k = 0; k < 4; ++k )
                {
                    sv.weights[k] = ( sum > 0.0f ? sv.weights[k] / sum : ( k == 0 ? 1.0f : 0.0f ) );
                }
                vertices.push_back( sv );
            }

            SkinPrimitive sp;
            sp.first_index = static_cast< int >( indices.size() );
            if ( prim.indices )
            {
                for ( cgltf_size k = 0; k < prim.indices->count; ++k )
                {
                    indices.push_back( base + static_cast< std::uint32_t >( cgltf_accessor_read_index( prim.indices, k ) ) );
                }
            }
            else
            {
                for ( cgltf_size k = 0; k < position->count; ++k )
                {
                    indices.push_back( base + static_cast< std::uint32_t >( k ) );
                }
            }
            sp.index_count = static_cast< int >( indices.size() ) - sp.first_index;
            if ( prim.material )
            {
                sp.material = QString::fromUtf8( prim.material->name ? prim.material->name : "" );
                const cgltf_float * c = prim.material->pbr_metallic_roughness.base_color_factor;
                sp.base_color = QVector4D( c[0], c[1], c[2], c[3] );
            }
            primitives.push_back( sp );
        }
    }
    if ( vertices.empty() )
    {
        return fail( QStringLiteral( "no skinned mesh" ) );
    }

    // the clips
    clips.clear();
    for ( cgltf_size a = 0; a < data->animations_count; ++a )
    {
        const cgltf_animation & anim = data->animations[a];
        Clip clip;
        clip.name = QString::fromUtf8( anim.name ? anim.name : "" );
        clip.nodes.resize( node_count );
        for ( cgltf_size c = 0; c < anim.channels_count; ++c )
        {
            const cgltf_animation_channel & ch = anim.channels[c];
            if ( ! ch.target_node ) continue;
            Channel * dst = nullptr;
            int n = 3;
            NodeChannels & nc = clip.nodes[node_index( ch.target_node )];
            switch ( ch.target_path ) {
            case cgltf_animation_path_type_translation:
                dst = &nc.t;
                break;
            case cgltf_animation_path_type_rotation:
                dst = &nc.r;
                n = 4;
                break;
            case cgltf_animation_path_type_scale:
                dst = &nc.s;
                break;
            default:
                break;
            }
            const cgltf_accessor * in = ch.sampler->input;
            const cgltf_accessor * out = ch.sampler->output;
            // a cubic spline has 3 values a key (in-tangent, value,
            // out-tangent); this reads linear and step ones only
            if ( ! dst
                 || ch.sampler->interpolation == cgltf_interpolation_type_cubic_spline
                 || in->count == 0
                 || out->count != in->count )
            {
                continue;
            }
            dst->times.resize( in->count );
            cgltf_accessor_unpack_floats( in, dst->times.data(), in->count );
            dst->values.resize( out->count * n );
            cgltf_accessor_unpack_floats( out, dst->values.data(), out->count * n );
            dst->step = ( ch.sampler->interpolation == cgltf_interpolation_type_step );
            clip.duration = std::max( clip.duration, dst->times.back() );
        }
        clips.push_back( std::move( clip ) );
    }

    // the height at rest: the skinned vertices where the rest pose puts them
    // (the vertices themselves may be in another space, e.g. quantized)
    std::vector< float > skin_rest( joint_nodes.size() * 16 );
    skin( rest, QMatrix4x4(), skin_rest.data() );
    min_y = std::numeric_limits< float >::max();
    max_y = -std::numeric_limits< float >::max();
    for ( const SkinVertex & v : vertices )
    {
        float y = 0.0f;
        for ( int k = 0; k < 4; ++k )
        {
            const float * m = &skin_rest[v.joints[k] * 16];
            // the y row of the column-major matrix times (position, 1)
            y += v.weights[k] * ( m[1] * v.position[0] + m[5] * v.position[1] + m[9] * v.position[2] + m[13] );
        }
        min_y = std::min( min_y, y );
        max_y = std::max( max_y, y );
    }
    return true;
}

/*-------------------------------------------------------------------*/
int
GltfModel::nodeIndex( const QString & name ) const
{
    const auto it = std::find( node_names.begin(), node_names.end(), name );
    return ( it == node_names.end() ? -1 : static_cast< int >( it - node_names.begin() ) );
}

/*-------------------------------------------------------------------*/
int
GltfModel::clipIndex( const QString & name ) const
{
    for ( std::size_t i = 0; i < clips.size(); ++i )
    {
        if ( clips[i].name == name ) return static_cast< int >( i );
    }
    return -1;
}

/*-------------------------------------------------------------------*/
void
GltfModel::sample( const Clip & clip,
                   const float time,
                   std::vector< NodeTransform > & local ) const
{
    local.resize( rest.size(), NodeTransform() );
    const float t = ( clip.duration > 0.0f
                      ? std::fmod( std::fmod( time, clip.duration ) + clip.duration, clip.duration )
                      : 0.0f );
    const std::size_t n = std::min( clip.nodes.size(), local.size() );
    for ( std::size_t i = 0; i < n; ++i )
    {
        const NodeChannels & nc = clip.nodes[i];
        if ( nc.empty() ) continue;
        float v[4];
        if ( ! nc.t.empty() )
        {
            sample_channel( nc.t, t, 3, v );
            local[i].t = QVector3D( v[0], v[1], v[2] );
        }
        if ( ! nc.r.empty() )
        {
            sample_channel( nc.r, t, 4, v );
            local[i].r = QQuaternion( v[3], v[0], v[1], v[2] );
        }
        if ( ! nc.s.empty() )
        {
            sample_channel( nc.s, t, 3, v );
            local[i].s = QVector3D( v[0], v[1], v[2] );
        }
    }
}

/*-------------------------------------------------------------------*/
void
GltfModel::globals( const std::vector< NodeTransform > & local,
                    std::vector< QMatrix4x4 > & global ) const
{
    global.resize( rest.size() );
    for ( const int i : order )
    {
        const QMatrix4x4 m = ( i < static_cast< int >( local.size() ) ? local[i] : rest[i] ).matrix();
        global[i] = ( parent[i] >= 0 ? global[parent[i]] * m : m );
    }
}

/*-------------------------------------------------------------------*/
void
GltfModel::skin( const std::vector< NodeTransform > & local,
                 const QMatrix4x4 & model,
                 float * out ) const
{
    std::vector< QMatrix4x4 > global;
    globals( local, global );
    for ( std::size_t j = 0; j < joint_nodes.size(); ++j )
    {
        const QMatrix4x4 m = model * global[joint_nodes[j]] * inverse_bind[j];
        std::copy_n( m.constData(), 16, out + j * 16 ); // Qt keeps the columns in a row too
    }
}
