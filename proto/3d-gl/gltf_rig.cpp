// -*-c++-*-

/*
  A prototype of the 3D view (docs/PLAN_3D-VIEW.md, Phase 0, D1). Not part
  of the application.
*/

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "gltf_rig.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>

namespace {

RigTransform
node_transform( const cgltf_node & n )
{
    RigTransform t;
    if ( n.has_translation ) t.t = QVector3D( n.translation[0], n.translation[1], n.translation[2] );
    if ( n.has_rotation ) t.r = QQuaternion( n.rotation[3], n.rotation[0], n.rotation[1], n.rotation[2] );
    if ( n.has_scale ) t.s = QVector3D( n.scale[0], n.scale[1], n.scale[2] );
    return t;
}

QMatrix4x4
to_matrix( const RigTransform & t )
{
    QMatrix4x4 m;
    m.translate( t.t );
    m.rotate( t.r );
    m.scale( t.s );
    return m;
}

/*!
  \brief the value of a linear channel at a time: values with n floats per key.
*/
void
sample_channel( const GltfRig::Channel & c,
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
    const std::size_t i = std::upper_bound( ts.begin(), ts.end(), time ) - ts.begin(); // ts[i-1] <= time < ts[i]
    const float u = ( time - ts[i - 1] ) / ( ts[i] - ts[i - 1] );
    const float * a = &c.values[( i - 1 ) * n];
    const float * b = &c.values[i * n];
    if ( n == 4 )
    {
        // nlerp, the shorter way
        const float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
        const float sb = ( dot < 0.0f ? -1.0f : 1.0f );
        float len = 0.0f;
        for ( int k = 0; k < 4; ++k )
        {
            out[k] = a[k] + ( sb * b[k] - a[k] ) * u;
            len += out[k] * out[k];
        }
        len = std::sqrt( len );
        for ( int k = 0; k < 4; ++k ) out[k] /= len;
        return;
    }
    for ( int k = 0; k < n; ++k )
    {
        out[k] = a[k] + ( b[k] - a[k] ) * u;
    }
}

}

/*-------------------------------------------------------------------*/
bool
GltfRig::load( const std::string & path )
{
    cgltf_options options{};
    cgltf_data * data = nullptr;
    if ( cgltf_parse_file( &options, path.c_str(), &data ) != cgltf_result_success
         || cgltf_load_buffers( &options, data, path.c_str() ) != cgltf_result_success
         || data->skins_count < 1
         || data->meshes_count < 1 )
    {
        std::cerr << "could not read the model " << path << std::endl;
        if ( data ) cgltf_free( data );
        return false;
    }

    const auto node_index = [data]( const cgltf_node * n ) { return static_cast< int >( n - data->nodes ); };

    // nodes
    parent_.assign( data->nodes_count, -1 );
    node_names_.clear();
    rest_.resize( data->nodes_count );
    for ( cgltf_size i = 0; i < data->nodes_count; ++i )
    {
        const cgltf_node & n = data->nodes[i];
        rest_[i] = node_transform( n );
        if ( n.parent ) parent_[i] = node_index( n.parent );
        node_names_.push_back( n.name ? n.name : "" );
    }
    order_.clear();
    std::vector< bool > done( data->nodes_count, false );
    std::function< void( int ) > visit = [&]( int i )
        {
            if ( done[i] ) return;
            if ( parent_[i] >= 0 ) visit( parent_[i] );
            done[i] = true;
            order_.push_back( i );
        };
    for ( cgltf_size i = 0; i < data->nodes_count; ++i ) visit( static_cast< int >( i ) );

    // skin
    const cgltf_skin & skin = data->skins[0];
    joint_nodes_.clear();
    inverse_bind_.clear();
    for ( cgltf_size j = 0; j < skin.joints_count; ++j )
    {
        joint_nodes_.push_back( node_index( skin.joints[j] ) );
        float m[16];
        cgltf_accessor_read_float( skin.inverse_bind_matrices, j, m, 16 );
        inverse_bind_.push_back( QMatrix4x4( m ).transposed() ); // column-major to Qt's row-major constructor
    }

    // mesh
    vertices_.clear();
    indices_.clear();
    primitives_.clear();
    float min_y = 1e9f, max_y = -1e9f;
    const cgltf_mesh & mesh = data->meshes[0];
    for ( cgltf_size p = 0; p < mesh.primitives_count; ++p )
    {
        const cgltf_primitive & prim = mesh.primitives[p];
        const cgltf_accessor * pos = nullptr, * nor = nullptr, * joi = nullptr, * wei = nullptr;
        for ( cgltf_size a = 0; a < prim.attributes_count; ++a )
        {
            const cgltf_attribute & at = prim.attributes[a];
            if ( at.type == cgltf_attribute_type_position ) pos = at.data;
            else if ( at.type == cgltf_attribute_type_normal ) nor = at.data;
            else if ( at.type == cgltf_attribute_type_joints && at.index == 0 ) joi = at.data;
            else if ( at.type == cgltf_attribute_type_weights && at.index == 0 ) wei = at.data;
        }
        if ( ! pos || ! nor || ! joi || ! wei || ! prim.indices ) continue;

        const std::uint32_t base = static_cast< std::uint32_t >( vertices_.size() );
        for ( cgltf_size v = 0; v < pos->count; ++v )
        {
            RigVertex rv;
            cgltf_accessor_read_float( pos, v, rv.pos, 3 );
            cgltf_accessor_read_float( nor, v, rv.normal, 3 );
            cgltf_uint j[4];
            cgltf_accessor_read_uint( joi, v, j, 4 );
            for ( int k = 0; k < 4; ++k ) rv.joints[k] = static_cast< std::uint8_t >( j[k] );
            cgltf_accessor_read_float( wei, v, rv.weights, 4 );
            min_y = std::min( min_y, rv.pos[1] );
            max_y = std::max( max_y, rv.pos[1] );
            vertices_.push_back( rv );
        }
        RigPrimitive rp;
        rp.first_index = static_cast< int >( indices_.size() );
        rp.index_count = static_cast< int >( prim.indices->count );
        rp.material = ( prim.material && prim.material->name ? prim.material->name : "" );
        for ( cgltf_size k = 0; k < prim.indices->count; ++k )
        {
            indices_.push_back( base + static_cast< std::uint32_t >( cgltf_accessor_read_index( prim.indices, k ) ) );
        }
        primitives_.push_back( rp );
    }
    height_ = std::max( 1e-3f, max_y - min_y );

    // clips
    clips_.clear();
    for ( cgltf_size a = 0; a < data->animations_count; ++a )
    {
        const cgltf_animation & anim = data->animations[a];
        Clip clip;
        clip.name = ( anim.name ? anim.name : "" );
        clip.nodes.resize( data->nodes_count );
        for ( cgltf_size c = 0; c < anim.channels_count; ++c )
        {
            const cgltf_animation_channel & ch = anim.channels[c];
            if ( ! ch.target_node ) continue;
            Channel * dst = nullptr;
            int n = 3;
            switch ( ch.target_path ) {
            case cgltf_animation_path_type_translation: dst = &clip.nodes[node_index( ch.target_node )].t; break;
            case cgltf_animation_path_type_rotation: dst = &clip.nodes[node_index( ch.target_node )].r; n = 4; break;
            case cgltf_animation_path_type_scale: dst = &clip.nodes[node_index( ch.target_node )].s; break;
            default: break;
            }
            if ( ! dst ) continue;
            const cgltf_accessor * in = ch.sampler->input;
            const cgltf_accessor * out = ch.sampler->output;
            dst->times.resize( in->count );
            cgltf_accessor_unpack_floats( in, dst->times.data(), in->count );
            dst->values.resize( out->count * n );
            cgltf_accessor_unpack_floats( out, dst->values.data(), out->count * n );
            clip.duration = std::max( clip.duration, dst->times.back() );
        }
        clips_.push_back( std::move( clip ) );
    }

    cgltf_free( data );
    M_local.resize( rest_.size() );
    M_global.resize( rest_.size() );
    return true;
}

/*-------------------------------------------------------------------*/
int
GltfRig::clipIndex( const std::string & name ) const
{
    for ( std::size_t i = 0; i < clips_.size(); ++i )
    {
        if ( clips_[i].name == name ) return static_cast< int >( i );
    }
    return -1;
}

/*-------------------------------------------------------------------*/
void
GltfRig::sample( const Clip & clip,
                 const float time,
                 const float weight,
                 std::vector< RigTransform > & acc,
                 std::vector< float > & acc_weight ) const
{
    const float t = ( clip.duration > 0.0f ? std::fmod( std::fmod( time, clip.duration ) + clip.duration, clip.duration ) : 0.0f );
    for ( std::size_t i = 0; i < clip.nodes.size(); ++i )
    {
        const NodeChannels & nc = clip.nodes[i];
        if ( nc.t.times.empty() && nc.r.times.empty() && nc.s.times.empty() ) continue;

        float v[4];
        RigTransform & a = acc[i];
        if ( ! nc.t.times.empty() ) { sample_channel( nc.t, t, 3, v ); a.t += QVector3D( v[0], v[1], v[2] ) * weight; }
        else a.t += rest_[i].t * weight;
        if ( ! nc.s.times.empty() ) { sample_channel( nc.s, t, 3, v ); a.s += QVector3D( v[0], v[1], v[2] ) * weight; }
        else a.s += rest_[i].s * weight;
        QQuaternion q = rest_[i].r;
        if ( ! nc.r.times.empty() ) { sample_channel( nc.r, t, 4, v ); q = QQuaternion( v[3], v[0], v[1], v[2] ); }
        // sum on the same side as the first one (nlerp of the weighted sum)
        if ( acc_weight[i] > 0.0f && QQuaternion::dotProduct( a.r, q ) < 0.0f ) q = -q;
        a.r = QQuaternion( a.r.scalar() + q.scalar() * weight,
                           a.r.vector() + q.vector() * weight );
        acc_weight[i] += weight;
    }
}

/*-------------------------------------------------------------------*/
void
GltfRig::localPose( const std::vector< int > & clips,
                    const std::vector< float > & times,
                    const std::vector< float > & weights,
                    std::vector< RigTransform > & local ) const
{
    local.resize( rest_.size() );
    std::vector< float > acc_weight( rest_.size(), 0.0f );

    for ( RigTransform & t : local )
    {
        t.t = QVector3D();
        t.s = QVector3D();
        t.r = QQuaternion( 0.0f, 0.0f, 0.0f, 0.0f );
    }
    for ( std::size_t c = 0; c < clips.size(); ++c )
    {
        if ( clips[c] < 0 || weights[c] <= 0.0f ) continue;
        sample( clips_[clips[c]], times[c], weights[c], local, acc_weight );
    }
    for ( std::size_t i = 0; i < local.size(); ++i )
    {
        if ( acc_weight[i] <= 0.0f )
        {
            local[i] = rest_[i];
            continue;
        }
        local[i].t /= acc_weight[i];
        local[i].s /= acc_weight[i];
        local[i].r.normalize();
    }
}

/*-------------------------------------------------------------------*/
void
GltfRig::pose( const std::vector< int > & clips,
               const std::vector< float > & times,
               const std::vector< float > & weights,
               const QMatrix4x4 & model,
               float * out ) const
{
    std::vector< RigTransform > & local = const_cast< std::vector< RigTransform > & >( M_local );
    std::vector< QMatrix4x4 > & global = const_cast< std::vector< QMatrix4x4 > & >( M_global );
    localPose( clips, times, weights, local );

    for ( const int i : order_ )
    {
        global[i] = ( parent_[i] >= 0 ? global[parent_[i]] * to_matrix( local[i] ) : model * to_matrix( local[i] ) );
    }
    for ( std::size_t j = 0; j < joint_nodes_.size(); ++j )
    {
        const QMatrix4x4 m = global[joint_nodes_[j]] * inverse_bind_[j];
        std::copy_n( m.constData(), 16, out + j * 16 ); // column-major
    }
}
