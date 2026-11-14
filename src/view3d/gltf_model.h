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
#include <QMatrix4x4>
#include <QQuaternion>
#include <QString>
#include <QVector3D>
#include <QVector4D>

#include <cstdint>
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
};

/*!
  \struct SkinPrimitive
  \brief a part of the mesh drawn in one material: indices [first_index,
  first_index + index_count) of GltfModel::indices
*/
struct SkinPrimitive {
    int first_index = 0;
    int index_count = 0;
    QString material; //!< the material's name
    QVector4D base_color = QVector4D( 1.0f, 1.0f, 1.0f, 1.0f ); //!< the material's own color
};

/*!
  \class GltfModel
  \brief a skinned model of a glTF 2.0 file (.glb) and its clips, as plain
  data: the nodes (bones) with their rest transforms, the skin (which nodes
  are joints, and their inverse bind matrices), the meshes skinned with
  it, and the keyframes of the clips. No OpenGL: Figures3D poses it, and
  SkinnedMesh draws it.

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
    std::vector< std::uint32_t > indices; //!< 3 a triangle
    std::vector< SkinPrimitive > primitives;

    std::vector< Clip > clips;

    float min_y = 0.0f; //!< the lowest point of the skinned mesh at rest (m)
    float max_y = 0.0f; //!< the highest

    /*!
      \brief read a glTF binary (.glb) with its buffers inside: the first
      skin, the meshes skinned with it, and all the clips
      \param data the file
      \param error what went wrong, if not null
      \return false if it cannot be read, or has no skinned mesh
    */
    bool load( const QByteArray & data,
               QString * error = nullptr );

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
};

#endif
