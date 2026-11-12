// -*-c++-*-

/*
  A prototype of the 3D view (docs/PLAN_3D-VIEW.md, Phase 0, D1): a skinned
  glTF model and the poses of its clips, mixed by weights. The poses are
  computed from the clip times every frame (no animation player, §5.8).
  Not part of the application.
*/

#ifndef PROTO3D_GLTF_RIG_H
#define PROTO3D_GLTF_RIG_H

#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>

#include <cstdint>
#include <string>
#include <vector>

struct RigVertex {
    float pos[3];
    float normal[3];
    std::uint8_t joints[4];
    float weights[4];
};

struct RigPrimitive {
    int first_index = 0;
    int index_count = 0;
    std::string material;
};

struct RigTransform {
    QVector3D t;
    QQuaternion r;
    QVector3D s{ 1.0f, 1.0f, 1.0f };
};

class GltfRig {
public:
    struct Channel {
        std::vector< float > times;
        std::vector< float > values; //!< 3 or 4 floats per key
    };
    struct NodeChannels {
        Channel t, r, s;
    };
    struct Clip {
        std::string name;
        float duration = 0.0f;
        std::vector< NodeChannels > nodes; //!< by node index
    };

    std::vector< RigVertex > vertices_;
    std::vector< std::uint32_t > indices_;
    std::vector< RigPrimitive > primitives_;

    std::vector< int > parent_; //!< by node index, -1 at a root
    std::vector< RigTransform > rest_; //!< by node index
    std::vector< int > order_; //!< node indices, parents before children
    std::vector< int > joint_nodes_; //!< the node of each joint
    std::vector< QMatrix4x4 > inverse_bind_; //!< by joint

    std::vector< Clip > clips_;

    float height_ = 1.0f; //!< the height of the model in the bind pose

    bool load( const std::string & path );

    int clipIndex( const std::string & name ) const;
    int jointCount() const { return static_cast< int >( joint_nodes_.size() ); }

    /*!
      \brief the skin matrices of a pose: the clips mixed by weights, each at
      its time (looped). The matrices are model * global(joint) * inverse
      bind, column-major, 16 floats per joint.
      \param clips the clip indices
      \param times the times of the clips (s)
      \param weights the weights of the clips, summing to 1
      \param model the matrix of the model in the world
      \param out jointCount() * 16 floats
    */
    void pose( const std::vector< int > & clips,
               const std::vector< float > & times,
               const std::vector< float > & weights,
               const QMatrix4x4 & model,
               float * out ) const;

    /*!
      \brief the local transforms of the nodes in a pose: the clips mixed
      by weights, each at its time (looped). A node without channels keeps
      its rest transform.
      \param out by node index
    */
    void localPose( const std::vector< int > & clips,
                    const std::vector< float > & times,
                    const std::vector< float > & weights,
                    std::vector< RigTransform > & out ) const;

    //! the node names, by node index
    std::vector< std::string > node_names_;

private:
    std::vector< RigTransform > M_local; //!< work: the local transforms by node
    std::vector< QMatrix4x4 > M_global; //!< work

    void sample( const Clip & clip,
                 const float time,
                 const float weight,
                 std::vector< RigTransform > & acc,
                 std::vector< float > & acc_weight ) const;
};

#endif
