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

#include "bvh_model.h"
#include "fbx_model.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
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

//! cgltf reads the files that a .gltf names through Qt (a file, or a resource)
cgltf_result
read_file( const cgltf_memory_options *,
           const cgltf_file_options *,
           const char * path,
           cgltf_size * size,
           void ** data )
{
    QFile file( QString::fromUtf8( path ) );
    if ( ! file.open( QIODevice::ReadOnly ) ) return cgltf_result_file_not_found;
    const QByteArray bytes = file.readAll();
    void * mem = std::malloc( std::max< std::size_t >( 1, std::size_t( bytes.size() ) ) );
    if ( ! mem ) return cgltf_result_out_of_memory;
    std::memcpy( mem, bytes.constData(), std::size_t( bytes.size() ) );
    *size = cgltf_size( bytes.size() );
    *data = mem;
    return cgltf_result_success;
}

void
release_file( const cgltf_memory_options *,
              const cgltf_file_options *,
              void * data )
{
    std::free( data );
}

/*!
  \brief whether an extension that a file is not read without is read, or
  can be passed over: KHR_mesh_quantization (read as floats),
  KHR_texture_transform (applied to the texture coordinates), the
  materials' (the base color read; the rest of the shading passed over),
  the lights', VRM's. Not: a compression of the meshes or of the textures
  (KHR_draco_mesh_compression, EXT_meshopt_compression, KHR_texture_basisu,
  EXT_texture_webp), which would be drawn wrong.
*/
bool
reads_extension( const QString & name )
{
    return name == QLatin1String( "KHR_mesh_quantization" )
        || name == QLatin1String( "KHR_texture_transform" )
        || name.startsWith( QLatin1String( "KHR_materials_" ) )
        || name == QLatin1String( "KHR_lights_punctual" )
        || name == QLatin1String( "VRM" )
        || name.startsWith( QLatin1String( "VRMC_" ) );
}

//! KHR_texture_transform: texture coordinates scaled, turned and moved
void
transform_uv( const cgltf_texture_transform & t,
              float * uv )
{
    const float c = std::cos( t.rotation );
    const float s = std::sin( t.rotation );
    const float x = uv[0] * t.scale[0];
    const float y = uv[1] * t.scale[1];
    uv[0] = c * x + s * y + t.offset[0];
    uv[1] = -s * x + c * y + t.offset[1];
}

//! two matrices the same, to a tolerance
bool
same_matrix( const QMatrix4x4 & a,
             const QMatrix4x4 & b )
{
    for ( int k = 0; k < 16; ++k )
    {
        if ( std::fabs( a.constData()[k] - b.constData()[k] ) > 1.0e-5f ) return false;
    }
    return true;
}

/*!
  \brief the bytes of an image of the file: in a buffer, in a data URI, or
  in a file beside it
*/
QByteArray
image_bytes( const cgltf_image & image,
             const QString & dir )
{
    if ( image.buffer_view )
    {
        const cgltf_buffer_view & view = *image.buffer_view;
        const char * base = static_cast< const char * >( view.buffer->data );
        if ( ! base ) return QByteArray();
        return QByteArray( base + view.offset, int( view.size ) );
    }
    if ( ! image.uri ) return QByteArray();
    const QByteArray uri( image.uri );
    if ( uri.startsWith( "data:" ) )
    {
        const int comma = uri.indexOf( ',' );
        return ( comma < 0 ? QByteArray() : QByteArray::fromBase64( uri.mid( comma + 1 ) ) );
    }
    if ( dir.isEmpty() ) return QByteArray();
    QFile file( dir + QLatin1Char( '/' ) + QUrl::fromPercentEncoding( uri ) );
    return ( file.open( QIODevice::ReadOnly ) ? file.readAll() : QByteArray() );
}

//! the mean luminance of an image where it is not transparent (0 to 1)
float
mean_luminance( const QImage & image )
{
    const QImage small = image.scaled( 64, 64, Qt::IgnoreAspectRatio, Qt::SmoothTransformation )
        .convertToFormat( QImage::Format_RGBA8888 );
    double sum = 0.0;
    double weight = 0.0;
    for ( int y = 0; y < small.height(); ++y )
    {
        const uchar * p = small.constScanLine( y );
        for ( int x = 0; x < small.width(); ++x, p += 4 )
        {
            const double a = p[3] / 255.0;
            sum += a * ( 0.299 * p[0] + 0.587 * p[1] + 0.114 * p[2] ) / 255.0;
            weight += a;
        }
    }
    return ( weight > 0.0 ? float( sum / weight ) : 1.0f );
}

//! the smooth normals of triangles, where a primitive has none
void
compute_normals( std::vector< SkinVertex > & vertices,
                 const std::size_t first_vertex,
                 const std::vector< std::uint32_t > & indices,
                 const std::size_t first_index )
{
    std::vector< QVector3D > sum( vertices.size() - first_vertex );
    for ( std::size_t k = first_index; k + 2 < indices.size(); k += 3 )
    {
        const SkinVertex & a = vertices[indices[k]];
        const SkinVertex & b = vertices[indices[k + 1]];
        const SkinVertex & c = vertices[indices[k + 2]];
        const QVector3D pa( a.position[0], a.position[1], a.position[2] );
        const QVector3D pb( b.position[0], b.position[1], b.position[2] );
        const QVector3D pc( c.position[0], c.position[1], c.position[2] );
        const QVector3D n = QVector3D::crossProduct( pb - pa, pc - pa ); // as long as the triangle is large
        for ( int i = 0; i < 3; ++i ) sum[indices[k + i] - first_vertex] += n;
    }
    for ( std::size_t v = 0; v < sum.size(); ++v )
    {
        const QVector3D n = ( sum[v].lengthSquared() > 0.0f ? sum[v].normalized() : QVector3D( 0.0f, 1.0f, 0.0f ) );
        vertices[first_vertex + v].normal[0] = n.x();
        vertices[first_vertex + v].normal[1] = n.y();
        vertices[first_vertex + v].normal[2] = n.z();
    }
}

}

/*-------------------------------------------------------------------*/
bool
GltfModel::loadFile( const QString & path,
                     QString * error,
                     const bool mesh_required )
{
    QFile file( path );
    if ( ! file.open( QIODevice::ReadOnly ) )
    {
        if ( error ) *error = QStringLiteral( "cannot open " ) + QFileInfo( path ).fileName();
        return false;
    }
    return load( file.readAll(), error, path, mesh_required );
}

/*-------------------------------------------------------------------*/
void
GltfModel::clear()
{
    node_names.clear();
    parent.clear();
    rest.clear();
    order.clear();
    joint_nodes.clear();
    inverse_bind.clear();
    vertices.clear();
    uvs.clear();
    indices.clear();
    primitives.clear();
    images.clear();
    image_luminance.clear();
    clips.clear();
    humanoid_bones.clear();
    vrm_version = -1;
    posed_rest = false;
    capture = false;
    min_y = max_y = 0.0f;
}

/*-------------------------------------------------------------------*/
int
GltfModel::addImage( QImage image )
{
    if ( image.width() > MAX_IMAGE_SIZE || image.height() > MAX_IMAGE_SIZE )
    {
        image = image.scaled( std::min( image.width(), MAX_IMAGE_SIZE ), std::min( image.height(), MAX_IMAGE_SIZE ),
                              Qt::KeepAspectRatio, Qt::SmoothTransformation );
    }
    image_luminance.push_back( image.isNull() ? 1.0f : mean_luminance( image ) );
    images.push_back( image );
    return static_cast< int >( images.size() ) - 1;
}

/*-------------------------------------------------------------------*/
bool
GltfModel::load( const QByteArray & bytes,
                 QString * error,
                 const QString & path,
                 const bool mesh_required )
{
    const auto fail = [error]( const QString & what ) {
        if ( error ) *error = what;
        return false;
    };

    // an FBX file: by ufbx, into the same data
    if ( FbxModel::isFbx( bytes, path ) )
    {
        if ( ! FbxModel::read( bytes, path, *this, error ) ) return false;
        if ( mesh_required && vertices.empty() ) return fail( QStringLiteral( "no mesh" ) );
        orderParts();
        computeRest();
        return true;
    }
    // a BVH file: a skeleton and its motion, no mesh
    if ( BvhModel::isBvh( bytes, path ) )
    {
        if ( ! BvhModel::read( bytes, path, *this, error ) ) return false;
        if ( mesh_required ) return fail( QStringLiteral( "no mesh (a BVH file has a motion only)" ) );
        computeRest();
        return true;
    }

    cgltf_options options{};
    options.file.read = &read_file;
    options.file.release = &release_file;
    cgltf_data * raw = nullptr;
    if ( cgltf_parse( &options, bytes.constData(), static_cast< cgltf_size >( bytes.size() ), &raw )
         != cgltf_result_success )
    {
        return fail( QStringLiteral( "not a glTF file" ) );
    }
    std::unique_ptr< cgltf_data, DataDeleter > data( raw );
    // an extension that the file is not read without, and that is not read
    // here: not read, rather than drawn wrong
    for ( cgltf_size e = 0; e < data->extensions_required_count; ++e )
    {
        const QString name = QString::fromUtf8( data->extensions_required[e] );
        if ( ! reads_extension( name ) )
        {
            return fail( QStringLiteral( "it needs the extension %1, which is not read" ).arg( name ) );
        }
    }
    // the buffers: inside a .glb, or in files beside a .gltf
    const QByteArray encoded_path = path.toUtf8();
    const cgltf_result buffers = cgltf_load_buffers( &options, data.get(),
                                                     path.isEmpty() ? nullptr : encoded_path.constData() );
    if ( buffers == cgltf_result_file_not_found
         || buffers == cgltf_result_io_error )
    {
        return fail( QStringLiteral( "a buffer's file is not found" ) );
    }
    if ( buffers != cgltf_result_success
         || cgltf_validate( data.get() ) != cgltf_result_success )
    {
        return fail( QStringLiteral( "broken glTF data" ) );
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

    // the joints: a node with its inverse bind matrix, once (the skins of
    // the file joined; a node with another matrix in another skin is
    // another joint)
    joint_nodes.clear();
    inverse_bind.clear();
    std::map< int, std::vector< int > > joints_of_node;
    const auto joint_of = [&]( const int node, const QMatrix4x4 & ibm ) {
        std::vector< int > & list = joints_of_node[node];
        for ( const int j : list )
        {
            if ( same_matrix( inverse_bind[j], ibm ) ) return j;
        }
        joint_nodes.push_back( node );
        inverse_bind.push_back( ibm );
        list.push_back( static_cast< int >( joint_nodes.size() ) - 1 );
        return list.back();
    };
    // by skin, its joints' indices in the list
    std::vector< std::vector< int > > skin_joints( data->skins_count );
    for ( cgltf_size s = 0; s < data->skins_count; ++s )
    {
        const cgltf_skin & skin_data = data->skins[s];
        for ( cgltf_size j = 0; j < skin_data.joints_count; ++j )
        {
            float m[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
            if ( skin_data.inverse_bind_matrices )
            {
                cgltf_accessor_read_float( skin_data.inverse_bind_matrices, j, m, 16 );
            }
            // glTF writes column by column; Qt's constructor takes the rows
            skin_joints[s].push_back( joint_of( node_index( skin_data.joints[j] ), QMatrix4x4( m ).transposed() ) );
        }
    }

    // the images, made smaller if large, with their mean luminance
    images.clear();
    image_luminance.clear();
    const QString dir = ( path.isEmpty() ? QString() : QFileInfo( path ).path() );
    for ( cgltf_size k = 0; k < data->images_count; ++k )
    {
        addImage( QImage::fromData( image_bytes( data->images[k], dir ) ) );
    }
    const auto image_of = [&data]( const cgltf_texture_view & view ) {
        if ( ! view.texture || ! view.texture->image ) return -1;
        return static_cast< int >( view.texture->image - data->images );
    };

    // the meshes: skinned, or fixed to their node (a joint of its own)
    vertices.clear();
    uvs.clear();
    indices.clear();
    primitives.clear();
    for ( int i = 0; i < node_count; ++i )
    {
        const cgltf_node & node = data->nodes[i];
        if ( ! node.mesh ) continue;
        const std::vector< int > * joint_map = ( node.skin
                                                 ? &skin_joints[node.skin - data->skins]
                                                 : nullptr );
        const int fixed_joint = ( node.skin ? -1 : joint_of( i, QMatrix4x4() ) );

        for ( cgltf_size p = 0; p < node.mesh->primitives_count; ++p )
        {
            const cgltf_primitive & prim = node.mesh->primitives[p];
            if ( prim.type != cgltf_primitive_type_triangles ) continue;
            // the base color texture: which texture coordinates, and how
            // they are moved (KHR_texture_transform)
            const cgltf_texture_view * base_view = nullptr;
            if ( prim.material )
            {
                const cgltf_material & mat = *prim.material;
                base_view = ( mat.has_pbr_specular_glossiness && ! mat.pbr_metallic_roughness.base_color_texture.texture
                              ? &mat.pbr_specular_glossiness.diffuse_texture
                              : &mat.pbr_metallic_roughness.base_color_texture );
            }
            const cgltf_texture_transform * uv_transform = ( base_view && base_view->has_transform
                                                             ? &base_view->transform
                                                             : nullptr );
            const int uv_set = ( ! base_view ? 0
                                 : uv_transform && uv_transform->has_texcoord ? uv_transform->texcoord
                                 : base_view->texcoord );
            const cgltf_accessor * position = nullptr;
            const cgltf_accessor * normal = nullptr;
            const cgltf_accessor * texcoord = nullptr;
            const cgltf_accessor * joints = nullptr;
            const cgltf_accessor * weights = nullptr;
            for ( cgltf_size a = 0; a < prim.attributes_count; ++a )
            {
                const cgltf_attribute & at = prim.attributes[a];
                if ( at.type == cgltf_attribute_type_position ) position = at.data;
                else if ( at.type == cgltf_attribute_type_normal ) normal = at.data;
                else if ( at.type == cgltf_attribute_type_texcoord && at.index == uv_set ) texcoord = at.data;
                else if ( at.type == cgltf_attribute_type_joints && at.index == 0 ) joints = at.data;
                else if ( at.type == cgltf_attribute_type_weights && at.index == 0 ) weights = at.data;
            }
            if ( ! position ) continue;
            if ( joint_map && ( ! joints || ! weights ) ) continue;

            const std::uint32_t base = static_cast< std::uint32_t >( vertices.size() );
            for ( cgltf_size v = 0; v < position->count; ++v )
            {
                SkinVertex sv;
                cgltf_accessor_read_float( position, v, sv.position, 3 );
                if ( normal ) cgltf_accessor_read_float( normal, v, sv.normal, 3 );
                float uv[2] = { 0.0f, 0.0f };
                if ( texcoord ) cgltf_accessor_read_float( texcoord, v, uv, 2 );
                if ( uv_transform ) transform_uv( *uv_transform, uv );
                uvs.push_back( uv[0] );
                uvs.push_back( uv[1] );
                if ( ! joint_map )
                {
                    sv.joints[0] = static_cast< std::uint16_t >( fixed_joint );
                    sv.joints[1] = sv.joints[2] = sv.joints[3] = 0;
                    sv.weights[0] = 1.0f;
                    sv.weights[1] = sv.weights[2] = sv.weights[3] = 0.0f;
                    vertices.push_back( sv );
                    continue;
                }
                cgltf_uint j[4] = { 0, 0, 0, 0 };
                cgltf_accessor_read_uint( joints, v, j, 4 );
                cgltf_accessor_read_float( weights, v, sv.weights, 4 );
                float sum = 0.0f;
                for ( int k = 0; k < 4; ++k )
                {
                    if ( j[k] >= joint_map->size() ) // not a joint: no weight
                    {
                        sv.joints[k] = 0;
                        sv.weights[k] = 0.0f;
                    }
                    else
                    {
                        sv.joints[k] = static_cast< std::uint16_t >( ( *joint_map )[j[k]] );
                    }
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
            if ( ! normal ) compute_normals( vertices, base, indices, std::size_t( sp.first_index ) );
            if ( prim.material )
            {
                const cgltf_material & mat = *prim.material;
                sp.material = QString::fromUtf8( mat.name ? mat.name : "" );
                // the metallic-roughness model, or the older specular-glossiness one's diffuse
                if ( mat.has_pbr_specular_glossiness && ! mat.pbr_metallic_roughness.base_color_texture.texture )
                {
                    const cgltf_float * c = mat.pbr_specular_glossiness.diffuse_factor;
                    sp.base_color = QVector4D( c[0], c[1], c[2], c[3] );
                    sp.image = image_of( mat.pbr_specular_glossiness.diffuse_texture );
                }
                else
                {
                    const cgltf_float * c = mat.pbr_metallic_roughness.base_color_factor;
                    sp.base_color = QVector4D( c[0], c[1], c[2], c[3] );
                    sp.image = image_of( mat.pbr_metallic_roughness.base_color_texture );
                }
                if ( sp.image >= 0 && images[sp.image].isNull() ) sp.image = -1; // not decoded
                sp.alpha = ( mat.alpha_mode == cgltf_alpha_mode_mask ? SkinPrimitive::Mask
                             : mat.alpha_mode == cgltf_alpha_mode_blend ? SkinPrimitive::Blend
                             : SkinPrimitive::Opaque );
                sp.alpha_cutoff = ( sp.alpha == SkinPrimitive::Mask ? mat.alpha_cutoff : 0.5f );
                sp.double_sided = mat.double_sided;
            }
            primitives.push_back( sp );
        }
    }
    if ( vertices.empty() && mesh_required )
    {
        return fail( QStringLiteral( "no mesh" ) );
    }
    if ( joint_nodes.size() > std::numeric_limits< std::uint16_t >::max() )
    {
        return fail( QStringLiteral( "too many joints" ) );
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

    // VRM's humanoid bones: VRM 1.0 (VRMC_vrm) before VRM 0.x (VRM); a VRM
    // Animation (VRMC_vrm_animation, .vrma) names them as VRM 1.0 does (its
    // role nodes at rest in the VRM T-pose), but is not a VRM
    humanoid_bones.clear();
    vrm_version = -1;
    posed_rest = false; // glTF: the nodes' transforms are the rest
    capture = false;
    bool found = false;
    for ( const int version : { 1, 0, 2 } )
    {
        static const char * const extension[] = { "VRM", "VRMC_vrm", "VRMC_vrm_animation" };
        for ( cgltf_size e = 0; e < data->data_extensions_count && ! found; ++e )
        {
            const cgltf_extension & ext = data->data_extensions[e];
            if ( ! ext.name || ! ext.data
                 || std::strcmp( ext.name, extension[version] ) != 0 )
            {
                continue;
            }
            const QJsonObject humanoid = QJsonDocument::fromJson( QByteArray( ext.data ) ).object()
                .value( QStringLiteral( "humanoid" ) ).toObject();
            const QJsonValue bones = humanoid.value( QStringLiteral( "humanBones" ) );
            const auto add = [&]( const QString & role, const int node ) {
                if ( ! role.isEmpty() && node >= 0 && node < node_count ) humanoid_bones.emplace_back( role, node );
            };
            if ( version != 0 ) // an object: the role, {"node": index}
            {
                const QJsonObject o = bones.toObject();
                for ( auto it = o.begin(); it != o.end(); ++it )
                {
                    add( it.key(), it.value().toObject().value( QStringLiteral( "node" ) ).toInt( -1 ) );
                }
            }
            else // a list of {"bone": role, "node": index}
            {
                for ( const QJsonValue & b : bones.toArray() )
                {
                    const QJsonObject o = b.toObject();
                    add( o.value( QStringLiteral( "bone" ) ).toString(), o.value( QStringLiteral( "node" ) ).toInt( -1 ) );
                }
            }
            if ( version != 2 ) vrm_version = version;
            found = true;
        }
    }

    orderParts();
    computeRest();
    return true;
}

/*-------------------------------------------------------------------*/
void
GltfModel::orderParts()
{
    for ( const SkinPrimitive & p : primitives )
    {
        orderTriangles( indices.data() + p.first_index, static_cast< std::size_t >( p.index_count ), vertices.size() );
    }
}

/*-------------------------------------------------------------------*/
void
GltfModel::orderTriangles( std::uint32_t * indices,
                           const std::size_t count,
                           const std::size_t vertex_count )
{
    const std::size_t triangles = count / 3;
    if ( triangles < 2 ) return;

    // each vertex's triangles, and how many of them are left to put out
    std::vector< int > live( vertex_count, 0 );
    for ( std::size_t i = 0; i < triangles * 3; ++i ) ++live[indices[i]];
    std::vector< int > start( vertex_count + 1, 0 );
    for ( std::size_t v = 0; v < vertex_count; ++v ) start[v + 1] = start[v] + live[v];
    std::vector< int > around( triangles * 3 );
    {
        std::vector< int > fill( start.begin(), start.end() - 1 );
        for ( std::size_t t = 0; t < triangles; ++t )
        {
            for ( int c = 0; c < 3; ++c ) around[fill[indices[t * 3 + c]]++] = static_cast< int >( t );
        }
    }

    // when each vertex last came into the cache (a time; in the cache while
    // within VERTEX_CACHE of the time now)
    std::vector< int > stamp( vertex_count, 0 );
    int time = VERTEX_CACHE + 1;
    std::vector< char > emitted( triangles, 0 );
    std::vector< std::uint32_t > out;
    out.reserve( triangles * 3 );
    std::vector< std::uint32_t > dead_ends; // the vertices put out, the latest last
    std::vector< std::uint32_t > candidates;
    std::size_t cursor = 0; // the vertices in their order, for a new start
    long fan = indices[0];
    while ( fan >= 0 )
    {
        // all the triangles left around the fan's vertex
        candidates.clear();
        for ( int k = start[fan]; k < start[fan + 1]; ++k )
        {
            const int t = around[k];
            if ( emitted[t] ) continue;
            for ( int c = 0; c < 3; ++c )
            {
                const std::uint32_t v = indices[t * 3 + c];
                out.push_back( v );
                dead_ends.push_back( v );
                candidates.push_back( v );
                --live[v];
                if ( time - stamp[v] > VERTEX_CACHE ) stamp[v] = time++;
            }
            emitted[t] = 1;
        }
        // the next fan: a vertex put out with triangles left, still in the
        // cache after them, the earliest in it
        fan = -1;
        int best = -1;
        for ( const std::uint32_t v : candidates )
        {
            if ( live[v] <= 0 ) continue;
            const int priority = ( time - stamp[v] + 2 * live[v] <= VERTEX_CACHE ? time - stamp[v] : 0 );
            if ( priority > best )
            {
                best = priority;
                fan = v;
            }
        }
        // or else, a vertex put out lately with triangles left, or the next with any
        while ( fan < 0 && ! dead_ends.empty() )
        {
            const std::uint32_t d = dead_ends.back();
            dead_ends.pop_back();
            if ( live[d] > 0 ) fan = d;
        }
        while ( fan < 0 && cursor < vertex_count )
        {
            if ( live[cursor] > 0 ) fan = static_cast< long >( cursor );
            ++cursor;
        }
    }
    std::copy( out.begin(), out.end(), indices );
}

/*-------------------------------------------------------------------*/
void
GltfModel::computeRest()
{
    // the height at rest: the skinned vertices where the rest pose puts them
    // (the vertices themselves may be in another space, e.g. quantized)
    std::vector< float > skin_rest( joint_nodes.size() * 16 );
    skin( rest, QMatrix4x4(), skin_rest.data() );
    min_y = std::numeric_limits< float >::max();
    max_y = -std::numeric_limits< float >::max();
    for ( SkinVertex & v : vertices )
    {
        for ( int r = 0; r < 3; ++r )
        {
            v.rest[r] = 0.0f;
            for ( int k = 0; k < 4; ++k )
            {
                const float * m = &skin_rest[v.joints[k] * 16];
                // the row r of the column-major matrix times (position, 1)
                v.rest[r] += v.weights[k] * ( m[r] * v.position[0] + m[4 + r] * v.position[1] + m[8 + r] * v.position[2] + m[12 + r] );
            }
        }
        min_y = std::min( min_y, v.rest[1] );
        max_y = std::max( max_y, v.rest[1] );
    }
    if ( vertices.empty() ) min_y = max_y = 0.0f; // a skeleton of clips
}

/*-------------------------------------------------------------------*/
void
GltfModel::turnAbout( const QQuaternion & turn )
{
    // a new root above the roots: the joints' inverse bind matrices stay
    // (the skin matrices turn with the joints, and so the vertices)
    const int root = static_cast< int >( rest.size() );
    for ( int & p : parent )
    {
        if ( p < 0 ) p = root;
    }
    parent.push_back( -1 );
    node_names.push_back( QStringLiteral( "(turned to the front)" ) );
    NodeTransform t;
    t.r = turn.normalized();
    rest.push_back( t );
    order.insert( order.begin(), root );
    for ( Clip & c : clips ) c.nodes.resize( rest.size() );
    computeRest();
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
