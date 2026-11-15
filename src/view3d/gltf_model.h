// -*-c++-*-

/*!
  \file gltf_model.h
  \brief a skinned glTF model and its clips, read with cgltf. Header File.
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

#ifndef RCSSMONITOR_GLTF_MODEL_H
#define RCSSMONITOR_GLTF_MODEL_H

#include <QByteArray>
#include <QImage>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QString>
#include <QVector3D>
#include <QVector4D>

#include <cstdint>
#include <utility>
#include <vector>

/*!
  \struct NodeTransform
  \brief where a node (a bone) is relative to its parent: moved by t,
  turned by r, scaled by s (applied in the order scale, turn, move)
*/
struct NodeTransform {
    QVector3D t;
    QQuaternion r;
    QVector3D s = QVector3D( 1.0f, 1.0f, 1.0f );

    QMatrix4x4 matrix() const
      {
          QMatrix4x4 m;
          m.translate( t );
          m.rotate( r );
          m.scale( s );
          return m;
      }
};

/*!
  \struct SkinVertex
  \brief a vertex of a skinned mesh: it moves with up to 4 joints (bones),
  by their weights (summing to 1)
*/
struct SkinVertex {
    float position[3];
    float normal[3];
    std::uint16_t joints[4]; //!< indices into the joints of the skin
    float weights[4];
    float rest[3]; //!< where the rest pose puts it in the model (m), skinned
    float part[2] = { 0.0f, 0.0f }; //!< how much the arms' and the legs' bones move it, for the uniform (FigureUniform::paintParts())
    float scalp[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; //!< under each hairstyle of the model (1) or not (0), painted with the hair (FigureUniform::paintScalp())
};

/*!
  \struct SkinPrimitive
  \brief a part of the mesh drawn in one material: indices [first_index,
  first_index + index_count) of GltfModel::indices
*/
struct SkinPrimitive {
    //! how the material's alpha is taken (glTF's alphaMode)
    enum Alpha {
        Opaque = 0,
        Mask = 1, //!< transparent below alpha_cutoff (hair cards, eyelashes)
        Blend = 2, //!< see-through: drawn as a mask at 0.5 (the figures are not sorted)
    };
    int first_index = 0;
    int index_count = 0;
    QString material; //!< the material's name
    QVector4D base_color = QVector4D( 1.0f, 1.0f, 1.0f, 1.0f ); //!< the material's own color
    int image = -1; //!< its base color texture: an index of GltfModel::images (-1: none)
    int alpha = Opaque;
    float alpha_cutoff = 0.5f; //!< Mask: transparent below this
    bool double_sided = false; //!< seen from behind too (no culling)
};

/*!
  \class GltfModel
  \brief a skinned model of a glTF 2.0 file (.glb, .gltf, .vrm), or of an
  FBX file (FbxModel), and its clips, as plain data: the nodes (bones) with their rest transforms, the
  joints (which nodes, and their inverse bind matrices: the skins of the
  file joined, and the nodes that meshes are fixed to), the meshes, the
  materials and their textures, and the keyframes of the clips. No
  OpenGL: Figures3D poses it, and SkinnedMesh draws it.

  glTF's space: y up, and a person faces +z. A skinned vertex is drawn at
  sum of weight * global(joint) * inverse_bind(joint) * position, where
  global() is the joint's transform in the model, through its parents.
*/
class GltfModel {
public:

    //! the keyframes of a node's translation (3 floats a key) or rotation (4: x, y, z, w)
    struct Channel {
        std::vector< float > times; //!< s, ascending
        std::vector< float > values;
        bool step = false; //!< held until the next key, not interpolated

        bool empty() const { return times.empty(); }
    };

    struct NodeChannels {
        Channel t, r, s;

        bool empty() const { return t.empty() && r.empty() && s.empty(); }
    };

    struct Clip {
        QString name;
        float duration = 0.0f; //!< s
        std::vector< NodeChannels > nodes; //!< by node index
    };

    std::vector< QString > node_names; //!< by node index
    std::vector< int > parent; //!< by node index, -1 at a root
    std::vector< NodeTransform > rest; //!< by node index: the transforms in the file
    std::vector< int > order; //!< node indices, parents before their children

    std::vector< int > joint_nodes; //!< the node of each joint of the skin
    std::vector< QMatrix4x4 > inverse_bind; //!< by joint

    std::vector< SkinVertex > vertices;
    //! by vertex, 2 floats: where in its material's texture (0 to 1 across, from the image's top left).
    //! Apart from the vertices: only a part with a texture reads them (a larger vertex is slower to draw)
    std::vector< float > uvs;
    std::vector< std::uint32_t > indices; //!< 3 a triangle
    std::vector< SkinPrimitive > primitives;
    std::vector< QImage > images; //!< the textures of the materials (at most MAX_IMAGE_SIZE)
    std::vector< float > image_luminance; //!< by image: its mean luminance (0 to 1) where it is not transparent

    std::vector< Clip > clips;

    //! VRM's humanoid bones (a role's name as VRM writes it, and its node), if the file is a VRM
    std::vector< std::pair< QString, int > > humanoid_bones;
    int vrm_version = -1; //!< 0 for VRM 0.x, 1 for VRM 1.0, -1 if not a VRM
    //! the rest may be a pose, not the bind pose: an FBX file without a skin
    //! (its nodes as saved, often a clip's first frame; restAs() turns it)
    bool posed_rest = false;

    float min_y = 0.0f; //!< the lowest point of the skinned mesh at rest (m)
    float max_y = 0.0f; //!< the highest

    //! a texture larger than this (pixels) is made smaller: a figure is small on the screen
    static constexpr int MAX_IMAGE_SIZE = 1024;

    /*!
      \brief read a glTF file: a binary (.glb, .vrm) or JSON (.gltf), with
      its buffers and images inside or in files beside it. All the meshes
      are read: those skinned (the skins joined into one list of joints)
      and those fixed to a node, which move with it; the materials with
      their base color textures; the clips; VRM's humanoid bones.
      \param data the file
      \param error what went wrong, if not null
      \param path where the file is (a file or a resource), to find the
      files it names; empty: none. An FBX file (FbxModel::isFbx()) is read
      by FbxModel::read().
      \param mesh_required false for a file of clips (a skeleton without a mesh)
      \return false if it cannot be read, or has no mesh (if one is required)
    */
    bool load( const QByteArray & data,
               QString * error = nullptr,
               const QString & path = QString(),
               const bool mesh_required = true );

    /*!
      \brief read a glTF or FBX file by its path (a file or a resource): load()
      \return false if it cannot be opened or read
    */
    bool loadFile( const QString & path,
                   QString * error = nullptr,
                   const bool mesh_required = true );

    //! empty: no nodes, no mesh, no clips
    void clear();

    /*!
      \brief add a texture's image: made smaller if larger than
      MAX_IMAGE_SIZE, with its mean luminance (image_luminance)
      \return its index in images
    */
    int addImage( QImage image );

    /*!
      \brief turn the whole model (its skeleton, so its mesh and its clips
      too) about its origin: a new root node above the old roots, turned so.
      To face +z (a VRM 0.x model faces -z).
    */
    void turnAbout( const QQuaternion & turn );

    //! the index of the node of the name, or -1
    int nodeIndex( const QString & name ) const;

    //! the index of the clip of the name, or -1
    int clipIndex( const QString & name ) const;

    int jointCount() const { return static_cast< int >( joint_nodes.size() ); }

    //! the height of the skinned mesh at rest (m)
    float height() const { return max_y - min_y; }

    /*!
      \brief the transforms of the nodes in a clip at a time (looped by its
      duration). A node without keys in the clip keeps what local has, so
      start it from rest.
      \param clip the clip
      \param time s, any (looped)
      \param local by node index, resized to the nodes
    */
    void sample( const Clip & clip,
                 const float time,
                 std::vector< NodeTransform > & local ) const;

    /*!
      \brief the transforms of the nodes in the model, through their parents
      \param local by node index
      \param global by node index, resized to the nodes
    */
    void globals( const std::vector< NodeTransform > & local,
                  std::vector< QMatrix4x4 > & global ) const;

    /*!
      \brief the skin matrices of a pose: model * global(joint) *
      inverse_bind(joint), 16 floats a joint, column by column (as OpenGL
      reads a matrix)
      \param local the nodes' transforms (by node index)
      \param model where the model is in the world
      \param out jointCount() * 16 floats
    */
    void skin( const std::vector< NodeTransform > & local,
               const QMatrix4x4 & model,
               float * out ) const;

private:
    //! where the rest pose puts the vertices (SkinVertex::rest), and the lowest and highest of them
    void computeRest();
};

#endif
