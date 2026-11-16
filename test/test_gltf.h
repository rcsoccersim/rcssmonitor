// -*-c++-*-

/*!
  \file test_gltf.h
  \brief small glTF files written by the tests: a humanoid of boxes, in a
  T or an A pose, facing +z or -z, under the bone names of a preset, with
  a texture, as .glb, .gltf (with .bin and images beside it, or in data
  URIs) or VRM.
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

#ifndef RCSSMONITOR_TEST_GLTF_H
#define RCSSMONITOR_TEST_GLTF_H

#include "gltf_model.h"
#include "humanoid.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMatrix4x4>
#include <QStringList>
#include <QQuaternion>
#include <QVector2D>
#include <QVector3D>

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

namespace TestGltf {

struct Node {
    QString name;
    int parent = -1;
    QVector3D t;
    QQuaternion r;
    QVector3D s = QVector3D( 1.0f, 1.0f, 1.0f );
};

//! a clip: the keys of nodes' translations (3 floats a key), rotations (4: x, y, z, w) or scales
struct Animation {
    struct Channel {
        int node = -1;
        QString path; //!< "translation", "rotation", "scale"
        std::vector< float > times;
        std::vector< float > values;
        bool step = false;
    };
    QString name;
    std::vector< Channel > channels;
};

struct Material {
    QString name;
    QVector4D color = QVector4D( 1.0f, 1.0f, 1.0f, 1.0f );
    int image = -1; //!< of File::images
    QString alpha = QStringLiteral( "OPAQUE" );
    float cutoff = 0.5f;
    bool double_sided = false;
    bool spec_gloss = false; //!< the color and the texture as the diffuse of KHR_materials_pbrSpecularGlossiness (the base color another)
    QJsonObject transform; //!< KHR_texture_transform of the texture, if not empty
};

struct Mesh {
    int node = -1; //!< the node it is on
    int skin = -1; //!< its skin; -1: fixed to its node
    int material = -1;
    std::vector< QVector3D > positions;
    std::vector< QVector3D > normals; //!< empty: none written
    std::vector< QVector2D > uvs; //!< empty: none written
    std::vector< std::array< int, 4 > > joints;
    std::vector< QVector4D > weights;
    std::vector< std::uint32_t > indices;
};

struct Skin {
    std::vector< int > joints;
    std::vector< QMatrix4x4 > inverse_bind;
};

/*!
  \class File
  \brief a glTF file as plain data, written as .glb or .gltf
*/
class File {
public:
    std::vector< Node > nodes;
    std::vector< Material > materials;
    std::vector< QImage > images;
    std::vector< Mesh > meshes;
    std::vector< Skin > skins;
    std::vector< Animation > animations;
    QJsonObject extensions; //!< the root's (VRM, VRMC_vrm)
    QStringList required; //!< extensionsRequired

    QByteArray glb() const
      {
          QByteArray bin;
          const QByteArray json = QJsonDocument( document( bin, nullptr, QString(), false ) ).toJson( QJsonDocument::Compact );
          QByteArray j = json;
          while ( j.size() % 4 ) j.append( ' ' );
          while ( bin.size() % 4 ) bin.append( '\0' );
          QByteArray out;
          const auto u32 = [&out]( const std::uint32_t v ) { out.append( reinterpret_cast< const char * >( &v ), 4 ); };
          u32( 0x46546C67u ); // glTF
          u32( 2 );
          u32( std::uint32_t( 12 + 8 + j.size() + 8 + bin.size() ) );
          u32( std::uint32_t( j.size() ) );
          u32( 0x4E4F534Au ); // JSON
          out.append( j );
          u32( std::uint32_t( bin.size() ) );
          u32( 0x004E4942u ); // BIN
          out.append( bin );
          return out;
      }

    /*!
      write <name>.gltf with its buffer in <name>.bin and its images in
      <name>_<k>.png beside it, or all in data URIs
    */
    bool writeGltf( const QString & dir,
                    const QString & name,
                    const bool data_uris = false ) const
      {
          QByteArray bin;
          std::vector< QByteArray > pngs;
          const QJsonObject doc = document( bin, &pngs, name, data_uris );
          if ( ! data_uris )
          {
              if ( ! write( QDir( dir ).filePath( name + QStringLiteral( ".bin" ) ), bin ) ) return false;
              for ( std::size_t k = 0; k < pngs.size(); ++k )
              {
                  if ( ! write( QDir( dir ).filePath( QStringLiteral( "%1_%2.png" ).arg( name ).arg( k ) ), pngs[k] ) ) return false;
              }
          }
          return write( QDir( dir ).filePath( name + QStringLiteral( ".gltf" ) ), QJsonDocument( doc ).toJson() );
      }

    static bool write( const QString & path,
                       const QByteArray & data )
      {
          QFile f( path );
          return f.open( QIODevice::WriteOnly ) && f.write( data ) == data.size();
      }

private:
    static QByteArray png( const QImage & image )
      {
          QByteArray bytes;
          QBuffer buffer( &bytes );
          buffer.open( QIODevice::WriteOnly );
          image.save( &buffer, "PNG" );
          return bytes;
      }

    QJsonObject document( QByteArray & bin,
                          std::vector< QByteArray > * pngs,
                          const QString & name,
                          const bool data_uris ) const
      {
          QJsonArray views, accessors;
          const auto view = [&]( const QByteArray & bytes, const int target ) {
              while ( bin.size() % 4 ) bin.append( '\0' );
              QJsonObject v{ { "buffer", 0 }, { "byteOffset", bin.size() }, { "byteLength", bytes.size() } };
              if ( target ) v.insert( "target", target );
              bin.append( bytes );
              views.append( v );
              return views.size() - 1;
          };
          const auto floats = [&]( const std::vector< float > & values, const int n, const char * type, const int target,
                                   const bool bounds ) {
              QByteArray bytes( reinterpret_cast< const char * >( values.data() ), int( values.size() * sizeof( float ) ) );
              QJsonObject a{ { "bufferView", view( bytes, target ) }, { "componentType", 5126 },
                             { "count", int( values.size() / n ) }, { "type", type } };
              if ( bounds )
              {
                  QJsonArray lo, hi;
                  for ( int k = 0; k < n; ++k )
                  {
                      float mn = std::numeric_limits< float >::max(), mx = -mn;
                      for ( std::size_t i = k; i < values.size(); i += n )
                      {
                          mn = std::min( mn, values[i] );
                          mx = std::max( mx, values[i] );
                      }
                      lo.append( mn );
                      hi.append( mx );
                  }
                  a.insert( "min", lo );
                  a.insert( "max", hi );
              }
              accessors.append( a );
              return accessors.size() - 1;
          };

          QJsonArray json_nodes;
          for ( std::size_t i = 0; i < nodes.size(); ++i )
          {
              const Node & n = nodes[i];
              QJsonObject o{ { "name", n.name },
                             { "translation", QJsonArray{ n.t.x(), n.t.y(), n.t.z() } },
                             { "rotation", QJsonArray{ n.r.x(), n.r.y(), n.r.z(), n.r.scalar() } } };
              if ( n.s != QVector3D( 1.0f, 1.0f, 1.0f ) ) o.insert( "scale", QJsonArray{ n.s.x(), n.s.y(), n.s.z() } );
              QJsonArray children;
              for ( std::size_t c = 0; c < nodes.size(); ++c )
              {
                  if ( nodes[c].parent == int( i ) ) children.append( int( c ) );
              }
              if ( ! children.isEmpty() ) o.insert( "children", children );
              json_nodes.append( o );
          }

          QJsonArray json_meshes;
          for ( std::size_t m = 0; m < meshes.size(); ++m )
          {
              const Mesh & mesh = meshes[m];
              std::vector< float > p, nn, uv, w;
              for ( const QVector3D & v : mesh.positions ) p.insert( p.end(), { v.x(), v.y(), v.z() } );
              QJsonObject attributes{ { "POSITION", floats( p, 3, "VEC3", 34962, true ) } };
              if ( ! mesh.normals.empty() )
              {
                  for ( const QVector3D & v : mesh.normals ) nn.insert( nn.end(), { v.x(), v.y(), v.z() } );
                  attributes.insert( "NORMAL", floats( nn, 3, "VEC3", 34962, false ) );
              }
              if ( ! mesh.uvs.empty() )
              {
                  for ( const QVector2D & v : mesh.uvs ) uv.insert( uv.end(), { v.x(), v.y() } );
                  attributes.insert( "TEXCOORD_0", floats( uv, 2, "VEC2", 34962, false ) );
              }
              if ( mesh.skin >= 0 )
              {
                  std::vector< std::uint16_t > j;
                  for ( const std::array< int, 4 > & v : mesh.joints )
                  {
                      for ( const int k : v ) j.push_back( std::uint16_t( k ) );
                  }
                  QByteArray bytes( reinterpret_cast< const char * >( j.data() ), int( j.size() * 2 ) );
                  accessors.append( QJsonObject{ { "bufferView", view( bytes, 34962 ) }, { "componentType", 5123 },
                                                 { "count", int( mesh.joints.size() ) }, { "type", "VEC4" } } );
                  attributes.insert( "JOINTS_0", accessors.size() - 1 );
                  for ( const QVector4D & v : mesh.weights ) w.insert( w.end(), { v.x(), v.y(), v.z(), v.w() } );
                  attributes.insert( "WEIGHTS_0", floats( w, 4, "VEC4", 34962, false ) );
              }
              QByteArray ibytes( reinterpret_cast< const char * >( mesh.indices.data() ), int( mesh.indices.size() * 4 ) );
              accessors.append( QJsonObject{ { "bufferView", view( ibytes, 34963 ) }, { "componentType", 5125 },
                                             { "count", int( mesh.indices.size() ) }, { "type", "SCALAR" } } );
              QJsonObject prim{ { "attributes", attributes }, { "indices", accessors.size() - 1 } };
              if ( mesh.material >= 0 ) prim.insert( "material", mesh.material );
              json_meshes.append( QJsonObject{ { "name", QStringLiteral( "mesh%1" ).arg( m ) },
                                               { "primitives", QJsonArray{ prim } } } );
              QJsonObject node = json_nodes[mesh.node].toObject();
              node.insert( "mesh", int( m ) );
              if ( mesh.skin >= 0 ) node.insert( "skin", mesh.skin );
              json_nodes[mesh.node] = node;
          }

          QJsonArray json_skins;
          for ( const Skin & s : skins )
          {
              std::vector< float > m;
              for ( const QMatrix4x4 & ibm : s.inverse_bind ) m.insert( m.end(), ibm.constData(), ibm.constData() + 16 );
              QJsonArray joints;
              for ( const int j : s.joints ) joints.append( j );
              json_skins.append( QJsonObject{ { "joints", joints }, { "inverseBindMatrices", floats( m, 16, "MAT4", 0, false ) } } );
          }

          QJsonArray json_images, textures, json_materials;
          for ( std::size_t k = 0; k < images.size(); ++k )
          {
              const QByteArray bytes = png( images[k] );
              if ( ! pngs )
              {
                  json_images.append( QJsonObject{ { "bufferView", view( bytes, 0 ) }, { "mimeType", "image/png" } } );
              }
              else if ( data_uris )
              {
                  json_images.append( QJsonObject{ { "uri", QStringLiteral( "data:image/png;base64," ) + QString::fromLatin1( bytes.toBase64() ) } } );
              }
              else
              {
                  pngs->push_back( bytes );
                  json_images.append( QJsonObject{ { "uri", QStringLiteral( "%1_%2.png" ).arg( name ).arg( k ) } } );
              }
              textures.append( QJsonObject{ { "source", int( k ) }, { "sampler", 0 } } );
          }
          QStringList used = extensions.keys() + required;
          for ( const Material & m : materials )
          {
              const QJsonArray color{ m.color.x(), m.color.y(), m.color.z(), m.color.w() };
              QJsonObject texture{ { "index", m.image } };
              if ( ! m.transform.isEmpty() )
              {
                  texture.insert( "extensions", QJsonObject{ { "KHR_texture_transform", m.transform } } );
                  used << QStringLiteral( "KHR_texture_transform" );
              }
              QJsonObject pbr{ { "baseColorFactor", color } };
              if ( m.image >= 0 && ! m.spec_gloss ) pbr.insert( "baseColorTexture", texture );
              QJsonObject o{ { "name", m.name }, { "alphaMode", m.alpha }, { "doubleSided", m.double_sided } };
              if ( m.spec_gloss )
              {
                  // the base color another (passed over): mid gray
                  pbr.insert( "baseColorFactor", QJsonArray{ 0.5, 0.5, 0.5, 1.0 } );
                  QJsonObject diffuse{ { "diffuseFactor", color } };
                  if ( m.image >= 0 ) diffuse.insert( "diffuseTexture", texture );
                  o.insert( "extensions", QJsonObject{ { "KHR_materials_pbrSpecularGlossiness", diffuse } } );
                  used << QStringLiteral( "KHR_materials_pbrSpecularGlossiness" );
              }
              o.insert( "pbrMetallicRoughness", pbr );
              if ( m.alpha == QLatin1String( "MASK" ) ) o.insert( "alphaCutoff", m.cutoff );
              json_materials.append( o );
          }

          QJsonArray json_animations;
          for ( const Animation & a : animations )
          {
              QJsonArray samplers, channels;
              for ( const Animation::Channel & c : a.channels )
              {
                  const int n = int( c.values.size() / std::max< std::size_t >( 1, c.times.size() ) );
                  const int input = floats( c.times, 1, "SCALAR", 0, true );
                  const int output = floats( c.values, n, n == 4 ? "VEC4" : "VEC3", 0, false );
                  samplers.append( QJsonObject{ { "input", input }, { "output", output },
                                                { "interpolation", c.step ? "STEP" : "LINEAR" } } );
                  channels.append( QJsonObject{ { "sampler", samplers.size() - 1 },
                                                { "target", QJsonObject{ { "node", c.node }, { "path", c.path } } } } );
              }
              json_animations.append( QJsonObject{ { "name", a.name }, { "samplers", samplers }, { "channels", channels } } );
          }
          QJsonArray roots;
          for ( std::size_t i = 0; i < nodes.size(); ++i )
          {
              if ( nodes[i].parent < 0 ) roots.append( int( i ) );
          }
          QJsonObject buffer{ { "byteLength", 0 } };
          QJsonObject doc{ { "asset", QJsonObject{ { "version", "2.0" } } },
                           { "scene", 0 },
                           { "scenes", QJsonArray{ QJsonObject{ { "nodes", roots } } } },
                           { "nodes", json_nodes },
                           { "meshes", json_meshes },
                           { "materials", json_materials },
                           { "accessors", accessors },
                           { "bufferViews", views } };
          if ( ! json_skins.isEmpty() ) doc.insert( "skins", json_skins );
          if ( ! json_animations.isEmpty() ) doc.insert( "animations", json_animations );
          if ( ! json_images.isEmpty() )
          {
              doc.insert( "images", json_images );
              doc.insert( "textures", textures );
              doc.insert( "samplers", QJsonArray{ QJsonObject{} } );
          }
          if ( ! extensions.isEmpty() ) doc.insert( "extensions", extensions );
          used.removeDuplicates();
          if ( ! used.isEmpty() ) doc.insert( "extensionsUsed", QJsonArray::fromStringList( used ) );
          if ( ! required.isEmpty() ) doc.insert( "extensionsRequired", QJsonArray::fromStringList( required ) );
          while ( bin.size() % 4 ) bin.append( '\0' );
          buffer.insert( "byteLength", bin.size() );
          if ( pngs && ! data_uris ) buffer.insert( "uri", name + QStringLiteral( ".bin" ) );
          else if ( pngs ) buffer.insert( "uri", QStringLiteral( "data:application/octet-stream;base64," ) + QString::fromLatin1( bin.toBase64() ) );
          doc.insert( "buffers", QJsonArray{ buffer } );
          return doc;
      }
};

/*!
  \struct HumanoidOptions
  \brief how humanoid() makes a figure
*/
struct HumanoidOptions {
    QString preset = QStringLiteral( "quaternius" ); //!< the bones' names (skeletonBoneNames())
    QString prefix; //!< before each bone's name ("mixamorig:")
    float arm_down = 0.0f; //!< the upper arms turned down at rest (degree: an A pose)
    float elbow = 0.0f; //!< the lower arms turned down from the upper arms at rest (degree)
    float facing = 0.0f; //!< turned so far about +y from facing +z (degree; 180: facing -z, as a VRM 0.x model)
    bool normals = true; //!< the normals written
    int vrm = -1; //!< VRM's humanoid bones: 0 (VRM 0.x), 1 (VRM 1.0), -1: none
    QImage texture; //!< on the boxes (each face the whole image), if not null
    QString material = QStringLiteral( "Body" );
};

/*!
  \brief a humanoid of boxes: the bones of the roles at the Mannequin's
  joints (and a root), a box along each bone skinned to it, in a material
  (with the texture). The rest rotations are the identity in a T pose; in
  an A pose, the arms turned down.
  \param mannequin the Mannequin (its roles at their joints)
*/
inline
File
humanoid( const GltfModel & mannequin,
          const HumanoidOptions & o )
{
    HumanoidRig rig;
    rig.build( mannequin, quaterniusBoneNames() );
    std::vector< QMatrix4x4 > global;
    mannequin.globals( mannequin.rest, global );

    HumanBoneNames names;
    skeletonBoneNames( o.preset, &names );

    // the roles of the Mannequin, each with the nearest role above it
    std::vector< int > roles;
    std::array< int, HUMAN_BONE_COUNT > above;
    above.fill( -1 );
    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
    {
        if ( rig.node( HumanBone( b ) ) < 0 ) continue;
        roles.push_back( b );
        for ( int n = mannequin.parent[rig.node( HumanBone( b ) )]; n >= 0 && above[b] < 0; n = mannequin.parent[n] )
        {
            for ( int c = 0; c < HUMAN_BONE_COUNT; ++c )
            {
                if ( rig.node( HumanBone( c ) ) == n ) above[b] = c;
            }
        }
    }

    // where each role is, and how it is turned, in the model
    std::array< QVector3D, HUMAN_BONE_COUNT > at;
    std::array< QQuaternion, HUMAN_BONE_COUNT > turned;
    for ( const int b : roles ) at[b] = global[rig.node( HumanBone( b ) )].column( 3 ).toVector3D();
    const HumanBone arms[2][3] = {
        { HumanBone::LeftUpperArm, HumanBone::LeftLowerArm, HumanBone::LeftHand },
        { HumanBone::RightUpperArm, HumanBone::RightLowerArm, HumanBone::RightHand },
    };
    for ( int side = 0; side < 2; ++side )
    {
        const float sign = ( side == 0 ? -1.0f : 1.0f ); // down from +x and from -x
        const int u = int( arms[side][0] ), l = int( arms[side][1] ), h = int( arms[side][2] );
        const QQuaternion upper = QQuaternion::fromAxisAndAngle( 0.0f, 0.0f, 1.0f, sign * o.arm_down );
        const QQuaternion lower = QQuaternion::fromAxisAndAngle( 0.0f, 0.0f, 1.0f, sign * ( o.arm_down + o.elbow ) );
        const QVector3D to_lower = at[l] - at[u];
        const QVector3D to_hand = at[h] - at[l];
        at[l] = at[u] + upper.rotatedVector( to_lower );
        at[h] = at[l] + lower.rotatedVector( to_hand );
        turned[u] = upper;
        turned[l] = lower;
        turned[h] = lower;
    }
    const QQuaternion facing = QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, o.facing );
    for ( const int b : roles )
    {
        at[b] = facing.rotatedVector( at[b] );
        turned[b] = facing * turned[b];
    }

    File f;
    f.nodes.push_back( Node{ QStringLiteral( "Armature" ), -1, QVector3D(), QQuaternion() } );
    f.nodes.push_back( Node{ QStringLiteral( "Body" ), 0, QVector3D(), QQuaternion() } );
    std::array< int, HUMAN_BONE_COUNT > node_of;
    node_of.fill( -1 );
    Skin skin;
    for ( const int b : roles )
    {
        const int p = above[b];
        const QQuaternion parent_turn = ( p >= 0 ? turned[p] : QQuaternion() );
        const QVector3D parent_at = ( p >= 0 ? at[p] : QVector3D() );
        Node n;
        n.name = o.prefix + names[b];
        n.parent = ( p >= 0 ? node_of[p] : 0 );
        n.t = parent_turn.conjugated().rotatedVector( at[b] - parent_at );
        n.r = parent_turn.conjugated() * turned[b];
        node_of[b] = int( f.nodes.size() );
        f.nodes.push_back( n );
        QMatrix4x4 g;
        g.translate( at[b] );
        g.rotate( turned[b] );
        skin.joints.push_back( node_of[b] );
        skin.inverse_bind.push_back( g.inverted() );
    }
    f.skins.push_back( skin );

    // a box along each bone (to the first role below it, else a short one)
    Mesh mesh;
    mesh.node = 1;
    mesh.skin = 0;
    mesh.material = 0;
    for ( std::size_t j = 0; j < roles.size(); ++j )
    {
        const int b = roles[j];
        int below = -1;
        for ( const int c : roles )
        {
            if ( above[c] == b && below < 0 ) below = c;
        }
        QVector3D length = ( below >= 0 ? turned[b].conjugated().rotatedVector( at[below] - at[b] ) : QVector3D( 0.0f, 0.06f, 0.0f ) );
        if ( length.lengthSquared() < 1.0e-6f ) length = QVector3D( 0.0f, 0.06f, 0.0f );
        QMatrix4x4 g;
        g.translate( at[b] );
        g.rotate( turned[b] );
        // the box in the bone's frame: along length, 4 cm thick
        const QVector3D a = length.normalized();
        QVector3D side = QVector3D::crossProduct( a, QVector3D( 0.0f, 0.0f, 1.0f ) );
        if ( side.lengthSquared() < 1.0e-4f ) side = QVector3D::crossProduct( a, QVector3D( 1.0f, 0.0f, 0.0f ) );
        side = side.normalized() * 0.04f;
        const QVector3D depth = QVector3D::crossProduct( a, side ).normalized() * 0.04f;
        QVector3D corner[8];
        for ( int k = 0; k < 8; ++k )
        {
            corner[k] = ( k & 1 ? length : QVector3D() ) + ( k & 2 ? side : -side ) + ( k & 4 ? depth : -depth );
        }
        const int faces[6][4] = { { 0, 2, 6, 4 }, { 1, 5, 7, 3 }, { 0, 4, 5, 1 }, { 2, 3, 7, 6 }, { 0, 1, 3, 2 }, { 4, 6, 7, 5 } };
        for ( const auto & face : faces )
        {
            const std::uint32_t base = std::uint32_t( mesh.positions.size() );
            // facing out: counterclockwise seen from outside, the normal out
            const QVector3D wound = QVector3D::crossProduct( corner[face[1]] - corner[face[0]], corner[face[3]] - corner[face[0]] ).normalized();
            const QVector3D center = ( corner[0] + corner[7] ) * 0.5f;
            const QVector3D mid = ( corner[face[0]] + corner[face[2]] ) * 0.5f;
            const bool out = QVector3D::dotProduct( wound, mid - center ) > 0.0f;
            const QVector3D n = ( out ? wound : -wound );
            const QVector2D uv[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
            for ( int k = 0; k < 4; ++k )
            {
                mesh.positions.push_back( g.map( corner[face[k]] ) );
                if ( o.normals ) mesh.normals.push_back( g.mapVector( n ).normalized() );
                if ( ! o.texture.isNull() ) mesh.uvs.push_back( uv[k] );
                mesh.joints.push_back( { int( j ), 0, 0, 0 } );
                mesh.weights.push_back( QVector4D( 1.0f, 0.0f, 0.0f, 0.0f ) );
            }
            const std::uint32_t quad[6] = { 0, 1, 2, 0, 2, 3 };
            const std::uint32_t flipped[6] = { 0, 2, 1, 0, 3, 2 };
            for ( int k = 0; k < 6; ++k ) mesh.indices.push_back( base + ( out ? quad[k] : flipped[k] ) );
        }
    }
    f.meshes.push_back( mesh );

    Material m;
    m.name = o.material;
    if ( ! o.texture.isNull() )
    {
        f.images.push_back( o.texture );
        m.image = 0;
    }
    f.materials.push_back( m );

    // VRM's humanoid bones
    if ( o.vrm == 1 )
    {
        QJsonObject bones;
        for ( const int b : roles ) bones.insert( humanBoneName( HumanBone( b ) ), QJsonObject{ { "node", node_of[b] } } );
        f.extensions.insert( "VRMC_vrm", QJsonObject{ { "specVersion", "1.0" },
                                                      { "humanoid", QJsonObject{ { "humanBones", bones } } } } );
    }
    else if ( o.vrm == 0 )
    {
        QJsonArray bones;
        for ( const int b : roles ) bones.append( QJsonObject{ { "bone", humanBoneName( HumanBone( b ) ) }, { "node", node_of[b] } } );
        f.extensions.insert( "VRM", QJsonObject{ { "humanoid", QJsonObject{ { "humanBones", bones } } } } );
    }
    return f;
}

/*!
  \brief a model read, as a file again: its nodes at rest, its skin (all
  its joints), its meshes (one a part, each on a node of its own), its
  materials (their colors), its clips
*/
inline
File
fromModel( const GltfModel & m )
{
    File f;
    for ( std::size_t i = 0; i < m.rest.size(); ++i )
    {
        f.nodes.push_back( Node{ m.node_names[i], m.parent[i], m.rest[i].t, m.rest[i].r, m.rest[i].s } );
    }
    Skin skin;
    skin.joints = m.joint_nodes;
    skin.inverse_bind = m.inverse_bind;
    f.skins.push_back( skin );
    for ( const SkinPrimitive & p : m.primitives )
    {
        // (a node a mesh)
        const int body = int( f.nodes.size() );
        f.nodes.push_back( Node{ QStringLiteral( "Body_" ) + p.material, -1, QVector3D(), QQuaternion() } );
        Material mat;
        mat.name = p.material;
        mat.color = p.base_color;
        f.materials.push_back( mat );
        Mesh mesh;
        mesh.node = body;
        mesh.skin = 0;
        mesh.material = int( f.materials.size() ) - 1;
        std::vector< int > local( m.vertices.size(), -1 );
        for ( int k = p.first_index; k < p.first_index + p.index_count; ++k )
        {
            const std::uint32_t v = m.indices[k];
            if ( local[v] < 0 )
            {
                const SkinVertex & sv = m.vertices[v];
                local[v] = int( mesh.positions.size() );
                mesh.positions.push_back( QVector3D( sv.position[0], sv.position[1], sv.position[2] ) );
                mesh.normals.push_back( QVector3D( sv.normal[0], sv.normal[1], sv.normal[2] ) );
                mesh.joints.push_back( { sv.joints[0], sv.joints[1], sv.joints[2], sv.joints[3] } );
                mesh.weights.push_back( QVector4D( sv.weights[0], sv.weights[1], sv.weights[2], sv.weights[3] ) );
            }
            mesh.indices.push_back( std::uint32_t( local[v] ) );
        }
        f.meshes.push_back( mesh );
    }
    for ( const GltfModel::Clip & c : m.clips )
    {
        Animation a;
        a.name = c.name;
        for ( std::size_t i = 0; i < c.nodes.size(); ++i )
        {
            const auto add = [&]( const GltfModel::Channel & ch, const char * path ) {
                if ( ch.empty() ) return;
                a.channels.push_back( Animation::Channel{ int( i ), QString::fromLatin1( path ), ch.times, ch.values, ch.step } );
            };
            add( c.nodes[i].t, "translation" );
            add( c.nodes[i].r, "rotation" );
            add( c.nodes[i].s, "scale" );
        }
        f.animations.push_back( a );
    }
    return f;
}

/*!
  \brief a VRM Animation (.vrma) of a clip of a model: its nodes (named
  "bone<index>": the roles told by VRMC_vrm_animation only), no mesh, the
  clip
  \param m the model
  \param rig its roles
  \param clip the clip's name
*/
inline
File
animationOf( const GltfModel & m,
             const HumanoidRig & rig,
             const QString & clip )
{
    File f;
    for ( std::size_t i = 0; i < m.rest.size(); ++i )
    {
        f.nodes.push_back( Node{ QStringLiteral( "bone%1" ).arg( i ), m.parent[i], m.rest[i].t, m.rest[i].r, m.rest[i].s } );
    }
    const GltfModel::Clip & c = m.clips[m.clipIndex( clip )];
    Animation a;
    a.name = c.name;
    for ( std::size_t i = 0; i < c.nodes.size(); ++i )
    {
        if ( ! c.nodes[i].t.empty() ) a.channels.push_back( Animation::Channel{ int( i ), "translation", c.nodes[i].t.times, c.nodes[i].t.values, false } );
        if ( ! c.nodes[i].r.empty() ) a.channels.push_back( Animation::Channel{ int( i ), "rotation", c.nodes[i].r.times, c.nodes[i].r.values, false } );
    }
    f.animations.push_back( a );
    QJsonObject bones;
    for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
    {
        if ( rig.node( HumanBone( b ) ) >= 0 ) bones.insert( humanBoneName( HumanBone( b ) ), QJsonObject{ { "node", rig.node( HumanBone( b ) ) } } );
    }
    f.extensions.insert( "VRMC_vrm_animation", QJsonObject{ { "specVersion", "1.0" },
                                                            { "humanoid", QJsonObject{ { "humanBones", bones } } } } );
    return f;
}

//! the bundled Mannequin, read
inline
const GltfModel &
mannequin()
{
    static const GltfModel model = []() {
        GltfModel m;
        m.loadFile( QStringLiteral( TEST_SOURCE_DIR "/resources/figures/mannequin/Mannequin.glb" ) );
        return m;
    }();
    return model;
}

}

#endif
