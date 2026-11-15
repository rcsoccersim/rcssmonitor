// -*-c++-*-

/*!
  \file fbx_model.cpp
  \brief an FBX file read into a GltfModel, with ufbx. Source File.
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

#include "fbx_model.h"

#include "gltf_model.h"

#include "ufbx.h"

#include <QFileInfo>
#include <QHash>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>

namespace {

struct SceneDeleter {
    void operator()( ufbx_scene * s ) const { ufbx_free_scene( s ); }
};

struct BakedDeleter {
    void operator()( ufbx_baked_anim * b ) const { ufbx_free_baked_anim( b ); }
};

QString
text( const ufbx_string & s )
{
    return QString::fromUtf8( s.data, static_cast< int >( s.length ) );
}

//! ufbx's affine matrix (column by column, the translation last)
QMatrix4x4
matrixOf( const ufbx_matrix & m )
{
    // (QMatrix4x4 takes the rows)
    return QMatrix4x4( float( m.m00 ), float( m.m01 ), float( m.m02 ), float( m.m03 ),
                       float( m.m10 ), float( m.m11 ), float( m.m12 ), float( m.m13 ),
                       float( m.m20 ), float( m.m21 ), float( m.m22 ), float( m.m23 ),
                       0.0f, 0.0f, 0.0f, 1.0f );
}

NodeTransform
transformOf( const ufbx_transform & t )
{
    NodeTransform n;
    n.t = QVector3D( float( t.translation.x ), float( t.translation.y ), float( t.translation.z ) );
    n.r = QQuaternion( float( t.rotation.w ), float( t.rotation.x ), float( t.rotation.y ), float( t.rotation.z ) );
    n.s = QVector3D( float( t.scale.x ), float( t.scale.y ), float( t.scale.z ) );
    return n;
}

//! an affine matrix as a move, a turn and a scale (no shear)
NodeTransform
decompose( const QMatrix4x4 & m )
{
    NodeTransform n;
    n.t = m.column( 3 ).toVector3D();
    QVector3D axis[3] = { m.column( 0 ).toVector3D(), m.column( 1 ).toVector3D(), m.column( 2 ).toVector3D() };
    n.s = QVector3D( axis[0].length(), axis[1].length(), axis[2].length() );
    if ( QVector3D::dotProduct( QVector3D::crossProduct( axis[0], axis[1] ), axis[2] ) < 0.0f )
    {
        n.s.setX( -n.s.x() ); // mirrored
    }
    float r[9];
    for ( int c = 0; c < 3; ++c )
    {
        const float s = ( c == 0 ? n.s.x() : c == 1 ? n.s.y() : n.s.z() );
        const QVector3D a = ( s != 0.0f ? axis[c] / s : axis[c] );
        r[0 * 3 + c] = a.x();
        r[1 * 3 + c] = a.y();
        r[2 * 3 + c] = a.z();
    }
    n.r = QQuaternion::fromRotationMatrix( QMatrix3x3( r ) ).normalized();
    return n;
}

//! two affine matrices the same, to a tolerance relative to their size
bool
near( const QMatrix4x4 & a,
      const QMatrix4x4 & b )
{
    float size = 1.0f;
    for ( int k = 0; k < 16; ++k ) size = std::max( size, std::fabs( a.constData()[k] ) );
    for ( int k = 0; k < 16; ++k )
    {
        if ( std::fabs( a.constData()[k] - b.constData()[k] ) > 1.0e-4f * size ) return false;
    }
    return true;
}

//! whether an image has pixels that are not quite opaque
bool
transparent( const QImage & image )
{
    if ( image.isNull() || ! image.hasAlphaChannel() ) return false;
    const QImage rgba = image.convertToFormat( QImage::Format_RGBA8888 );
    for ( int y = 0; y < rgba.height(); ++y )
    {
        const uchar * p = rgba.constScanLine( y );
        for ( int x = 0; x < rgba.width(); ++x, p += 4 )
        {
            if ( p[3] < 250 ) return true;
        }
    }
    return false;
}

/*!
  the bytes of a texture: embedded in the file, or a file beside it (as it
  names it relative to the FBX file, as it is, or its name alone, also in
  "<name>.fbm", where exporters put the textures)
*/
QByteArray
textureBytes( const ufbx_texture & texture,
              const QString & path )
{
    const ufbx_texture * t = &texture;
    if ( t->type != UFBX_TEXTURE_FILE && t->file_textures.count > 0 ) t = t->file_textures.data[0];
    if ( t->content.size > 0 )
    {
        return QByteArray( static_cast< const char * >( t->content.data ), static_cast< int >( t->content.size ) );
    }
    if ( path.isEmpty() ) return QByteArray();
    const QFileInfo fbx( path );
    const QString dir = fbx.path();
    const QString name = QFileInfo( text( t->filename ).replace( QLatin1Char( '\\' ), QLatin1Char( '/' ) ) ).fileName();
    QStringList candidates;
    if ( t->relative_filename.length > 0 )
    {
        candidates << dir + QLatin1Char( '/' ) + text( t->relative_filename ).replace( QLatin1Char( '\\' ), QLatin1Char( '/' ) );
    }
    if ( t->filename.length > 0 ) candidates << text( t->filename );
    if ( ! name.isEmpty() )
    {
        candidates << dir + QLatin1Char( '/' ) + name
                   << dir + QLatin1Char( '/' ) + fbx.completeBaseName() + QStringLiteral( ".fbm/" ) + name;
    }
    for ( const QString & c : candidates )
    {
        QFile file( c );
        if ( QFileInfo( c ).isFile() && file.open( QIODevice::ReadOnly ) ) return file.readAll();
    }
    return QByteArray();
}

}

namespace FbxModel {

/*-------------------------------------------------------------------*/
bool
isFbx( const QByteArray & data,
       const QString & path )
{
    return data.startsWith( "Kaydara FBX Binary" )
        || data.startsWith( "; FBX" )
        || path.endsWith( QLatin1String( ".fbx" ), Qt::CaseInsensitive );
}

/*-------------------------------------------------------------------*/
bool
read( const QByteArray & data,
      const QString & path,
      GltfModel & m,
      QString * error )
{
    const auto fail = [error]( const QString & what ) {
        if ( error ) *error = what;
        return false;
    };
    m.clear();

    // glTF's space: y up, a person facing +z, meters; the geometry and the
    // transforms converted, the nodes of a plain inheritance
    ufbx_load_opts opts = {};
    opts.target_axes = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters = 1.0;
    opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
    opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY;
    opts.inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_COMPENSATE;
    opts.generate_missing_normals = true;
    opts.clean_skin_weights = true;
    ufbx_error err;
    std::unique_ptr< ufbx_scene, SceneDeleter > scene( ufbx_load_memory( data.constData(), static_cast< size_t >( data.size() ),
                                                                         &opts, &err ) );
    if ( ! scene )
    {
        return fail( QStringLiteral( "not an FBX file (" ) + text( err.description ) + QLatin1Char( ')' ) );
    }
    const ufbx_scene & s = *scene;

    // the nodes (not the root of the scene, which is ufbx's own)
    std::vector< int > index_of( s.nodes.count, -1 );
    std::vector< const ufbx_node * > node_of;
    for ( size_t i = 0; i < s.nodes.count; ++i )
    {
        const ufbx_node * n = s.nodes.data[i];
        if ( n->is_root ) continue;
        index_of[i] = static_cast< int >( node_of.size() );
        node_of.push_back( n );
        m.node_names.push_back( text( n->name ) );
        m.rest.push_back( transformOf( n->local_transform ) );
    }
    const int count = static_cast< int >( node_of.size() );
    m.parent.assign( count, -1 );
    std::vector< std::vector< int > > children( count );
    std::vector< int > roots;
    for ( int i = 0; i < count; ++i )
    {
        const ufbx_node * p = node_of[i]->parent;
        m.parent[i] = ( p ? index_of[p->typed_id] : -1 );
        if ( m.parent[i] >= 0 ) children[m.parent[i]].push_back( i );
        else roots.push_back( i );
    }
    // the order: each node after its parent
    std::vector< int > stack( roots.rbegin(), roots.rend() );
    while ( ! stack.empty() )
    {
        const int i = stack.back();
        stack.pop_back();
        m.order.push_back( i );
        for ( auto c = children[i].rbegin(); c != children[i].rend(); ++c ) stack.push_back( *c );
    }

    // the rest: the bind pose of the skin where a bone has one (a file may
    // be saved in another pose); the nodes' own transforms otherwise
    std::map< int, QMatrix4x4 > bind;
    for ( size_t d = 0; d < s.skin_deformers.count; ++d )
    {
        const ufbx_skin_deformer * skin = s.skin_deformers.data[d];
        for ( size_t c = 0; c < skin->clusters.count; ++c )
        {
            const ufbx_skin_cluster * cluster = skin->clusters.data[c];
            if ( cluster->bone_node && ! cluster->bone_node->is_root )
            {
                bind.emplace( index_of[cluster->bone_node->typed_id], matrixOf( cluster->bind_to_world ) );
            }
        }
    }
    {
        std::vector< QMatrix4x4 > world( count );
        for ( const int i : m.order )
        {
            const int p = m.parent[i];
            const QMatrix4x4 above = ( p >= 0 ? world[p] : QMatrix4x4() );
            const auto b = bind.find( i );
            if ( b == bind.end() )
            {
                world[i] = above * m.rest[i].matrix();
                continue;
            }
            world[i] = b->second;
            const QMatrix4x4 local = above.inverted() * b->second;
            if ( ! near( local, m.rest[i].matrix() ) ) m.rest[i] = decompose( local );
        }
    }

    // the joints: a node with its inverse bind matrix, once
    std::map< int, std::vector< int > > joints_of_node;
    const auto joint_of = [&]( const int node, const QMatrix4x4 & ibm ) {
        std::vector< int > & list = joints_of_node[node];
        for ( const int j : list )
        {
            if ( near( m.inverse_bind[j], ibm ) ) return j;
        }
        m.joint_nodes.push_back( node );
        m.inverse_bind.push_back( ibm );
        list.push_back( static_cast< int >( m.joint_nodes.size() ) - 1 );
        return list.back();
    };

    // the images of the textures, each once
    std::map< const ufbx_texture *, int > image_of;
    const auto imageOf = [&]( const ufbx_texture * t ) {
        if ( ! t ) return -1;
        const auto it = image_of.find( t );
        if ( it != image_of.end() ) return it->second;
        const QImage image = QImage::fromData( textureBytes( *t, path ) );
        const int index = ( image.isNull() ? -1 : m.addImage( image ) );
        image_of.emplace( t, index );
        return index;
    };

    // a material: its color and texture, its transparency
    const auto primitiveOf = [&]( const ufbx_material * mat ) {
        SkinPrimitive sp;
        if ( ! mat ) return sp;
        sp.material = text( mat->name );
        const ufbx_texture * texture = ( mat->pbr.base_color.texture ? mat->pbr.base_color.texture
                                         : mat->fbx.diffuse_color.texture );
        sp.image = imageOf( texture );
        if ( sp.image < 0 && mat->pbr.base_color.has_value )
        {
            // the color alone (with a texture, the texture's own colors: a
            // diffuse color written with it is often only the exporter's default)
            const float f = ( mat->pbr.base_factor.has_value ? float( mat->pbr.base_factor.value_real ) : 1.0f );
            const ufbx_vec4 & c = mat->pbr.base_color.value_vec4;
            sp.base_color = QVector4D( float( c.x ) * f, float( c.y ) * f, float( c.z ) * f, 1.0f );
        }
        // a see-through material (an opacity of 0 is taken as not written)
        const float opacity = ( mat->pbr.opacity.has_value ? float( mat->pbr.opacity.value_real ) : 1.0f );
        if ( opacity > 0.01f && opacity < 0.99f )
        {
            sp.base_color.setW( opacity );
            sp.alpha = SkinPrimitive::Blend;
            sp.double_sided = true;
        }
        if ( sp.image >= 0 && transparent( m.images[sp.image] ) )
        {
            // a mask (hair cards, eyelashes), seen from both sides
            sp.alpha = SkinPrimitive::Mask;
            sp.double_sided = true;
        }
        return sp;
    };

    // the meshes of the nodes, by their materials, in triangles
    for ( int i = 0; i < count; ++i )
    {
        const ufbx_node * n = node_of[i];
        if ( ! n->mesh ) continue;
        const ufbx_mesh & mesh = *n->mesh;
        if ( mesh.num_faces == 0 || ! mesh.vertex_position.exists ) continue;
        const ufbx_skin_deformer * skin = ( mesh.skin_deformers.count > 0 ? mesh.skin_deformers.data[0] : nullptr );
        std::vector< int > cluster_joint;
        if ( skin )
        {
            for ( size_t c = 0; c < skin->clusters.count; ++c )
            {
                const ufbx_skin_cluster * cluster = skin->clusters.data[c];
                cluster_joint.push_back( cluster->bone_node && ! cluster->bone_node->is_root
                                         ? joint_of( index_of[cluster->bone_node->typed_id], matrixOf( cluster->geometry_to_bone ) )
                                         : -1 );
            }
        }
        // a vertex of a mesh without a skin, or not weighted, moves with the
        // node (the geometry is in the node's space: no geometry transform is
        // left after ufbx's conversion)
        int fixed = -1;
        const auto fixedJoint = [&]() {
            if ( fixed < 0 ) fixed = joint_of( i, QMatrix4x4() );
            return fixed;
        };

        // the parts by material (one part of all the faces if none)
        std::vector< std::pair< int, std::vector< uint32_t > > > parts;
        if ( mesh.material_parts.count > 0 )
        {
            for ( size_t p = 0; p < mesh.material_parts.count; ++p )
            {
                const ufbx_mesh_part & part = mesh.material_parts.data[p];
                if ( part.num_triangles == 0 ) continue;
                parts.emplace_back( static_cast< int >( part.index ),
                                    std::vector< uint32_t >( part.face_indices.data, part.face_indices.data + part.face_indices.count ) );
            }
        }
        else
        {
            std::vector< uint32_t > all( mesh.num_faces );
            for ( size_t f = 0; f < mesh.num_faces; ++f ) all[f] = static_cast< uint32_t >( f );
            parts.emplace_back( -1, all );
        }

        std::vector< uint32_t > corners( mesh.max_face_triangles * 3 );
        for ( const auto & part : parts )
        {
            const ufbx_material * mat = nullptr;
            if ( part.first >= 0 )
            {
                const std::size_t k = static_cast< std::size_t >( part.first );
                mat = ( k < n->materials.count ? n->materials.data[k]
                        : k < mesh.materials.count ? mesh.materials.data[k] : nullptr );
            }
            SkinPrimitive sp = primitiveOf( mat );
            sp.first_index = static_cast< int >( m.indices.size() );
            // the corners of the triangles, each the same once
            QHash< QByteArray, std::uint32_t > seen;
            for ( const uint32_t f : part.second )
            {
                const uint32_t triangles = ufbx_triangulate_face( corners.data(), corners.size(), &mesh, mesh.faces.data[f] );
                for ( uint32_t k = 0; k < triangles * 3; ++k )
                {
                    const uint32_t ix = corners[k];
                    SkinVertex v = {};
                    const ufbx_vec3 p = ufbx_get_vertex_vec3( &mesh.vertex_position, ix );
                    v.position[0] = float( p.x );
                    v.position[1] = float( p.y );
                    v.position[2] = float( p.z );
                    if ( mesh.vertex_normal.exists )
                    {
                        const ufbx_vec3 nv = ufbx_get_vertex_vec3( &mesh.vertex_normal, ix );
                        v.normal[0] = float( nv.x );
                        v.normal[1] = float( nv.y );
                        v.normal[2] = float( nv.z );
                    }
                    float uv[2] = { 0.0f, 0.0f };
                    if ( mesh.vertex_uv.exists )
                    {
                        const ufbx_vec2 t = ufbx_get_vertex_vec2( &mesh.vertex_uv, ix );
                        uv[0] = float( t.x );
                        uv[1] = 1.0f - float( t.y ); // FBX: from the image's bottom left
                    }
                    // the four heaviest weights (ufbx keeps them heaviest first), summing to 1
                    if ( ! skin )
                    {
                        v.joints[0] = static_cast< std::uint16_t >( fixedJoint() );
                        v.weights[0] = 1.0f;
                    }
                    else
                    {
                        const ufbx_skin_vertex & sv = skin->vertices.data[mesh.vertex_indices.data[ix]];
                        int used = 0;
                        float sum = 0.0f;
                        for ( uint32_t w = 0; w < sv.num_weights && used < 4; ++w )
                        {
                            const ufbx_skin_weight & sw = skin->weights.data[sv.weight_begin + w];
                            if ( sw.cluster_index >= cluster_joint.size() || cluster_joint[sw.cluster_index] < 0 || sw.weight <= 0.0 ) continue;
                            v.joints[used] = static_cast< std::uint16_t >( cluster_joint[sw.cluster_index] );
                            v.weights[used] = float( sw.weight );
                            sum += v.weights[used];
                            ++used;
                        }
                        if ( used == 0 )
                        {
                            v.joints[0] = static_cast< std::uint16_t >( fixedJoint() );
                            v.weights[0] = 1.0f;
                            used = 1;
                            sum = 1.0f;
                        }
                        for ( int w = 0; w < 4; ++w )
                        {
                            if ( w < used ) v.weights[w] /= sum;
                            else { v.joints[w] = 0; v.weights[w] = 0.0f; }
                        }
                    }
                    QByteArray key( reinterpret_cast< const char * >( &v ), static_cast< int >( offsetof( SkinVertex, rest ) ) );
                    key.append( reinterpret_cast< const char * >( uv ), static_cast< int >( sizeof( uv ) ) );
                    auto it = seen.find( key );
                    if ( it == seen.end() )
                    {
                        it = seen.insert( key, static_cast< std::uint32_t >( m.vertices.size() ) );
                        m.vertices.push_back( v );
                        m.uvs.push_back( uv[0] );
                        m.uvs.push_back( uv[1] );
                    }
                    m.indices.push_back( it.value() );
                }
            }
            sp.index_count = static_cast< int >( m.indices.size() ) - sp.first_index;
            if ( sp.index_count > 0 ) m.primitives.push_back( sp );
        }
    }
    if ( m.joint_nodes.size() > std::numeric_limits< std::uint16_t >::max() )
    {
        return fail( QStringLiteral( "too many joints" ) );
    }

    // without a skin, nothing tells the rest: the nodes are as saved (a
    // file of clips: often its first frame)
    m.posed_rest = ( s.skin_deformers.count == 0 );

    // the takes, baked: keys to be read in lines (the rotations the shorter way), from 0
    ufbx_bake_opts bake = {};
    bake.trim_start_time = true;
    bake.resample_rate = 30.0;
    for ( size_t a = 0; a < s.anim_stacks.count; ++a )
    {
        const ufbx_anim_stack * stack = s.anim_stacks.data[a];
        std::unique_ptr< ufbx_baked_anim, BakedDeleter > baked( ufbx_bake_anim( &s, stack->anim, &bake, &err ) );
        if ( ! baked ) continue;
        GltfModel::Clip clip;
        clip.name = text( stack->name );
        clip.duration = float( baked->playback_duration );
        clip.nodes.resize( m.rest.size() );
        for ( size_t b = 0; b < baked->nodes.count; ++b )
        {
            const ufbx_baked_node & bn = baked->nodes.data[b];
            if ( bn.typed_id >= index_of.size() || index_of[bn.typed_id] < 0 ) continue;
            GltfModel::NodeChannels & nc = clip.nodes[index_of[bn.typed_id]];
            const auto vectors = [&clip]( const ufbx_baked_vec3_list & keys, GltfModel::Channel & ch ) {
                for ( size_t k = 0; k < keys.count; ++k )
                {
                    ch.times.push_back( float( keys.data[k].time ) );
                    ch.values.push_back( float( keys.data[k].value.x ) );
                    ch.values.push_back( float( keys.data[k].value.y ) );
                    ch.values.push_back( float( keys.data[k].value.z ) );
                    clip.duration = std::max( clip.duration, ch.times.back() );
                }
            };
            vectors( bn.translation_keys, nc.t );
            vectors( bn.scale_keys, nc.s );
            for ( size_t k = 0; k < bn.rotation_keys.count; ++k )
            {
                const ufbx_baked_quat & q = bn.rotation_keys.data[k];
                nc.r.times.push_back( float( q.time ) );
                nc.r.values.push_back( float( q.value.x ) );
                nc.r.values.push_back( float( q.value.y ) );
                nc.r.values.push_back( float( q.value.z ) );
                nc.r.values.push_back( float( q.value.w ) );
                clip.duration = std::max( clip.duration, nc.r.times.back() );
            }
        }
        m.clips.push_back( std::move( clip ) );
    }
    return true;
}

}
