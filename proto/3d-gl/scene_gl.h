// -*-c++-*-

/*
  A prototype of the 3D view (docs/PLAN_3D-VIEW.md, Phase 0, D1): the scene
  drawn with OpenGL 3.3 core and our own shaders, into any framebuffer (a
  QOpenGLWidget, or an offscreen one). Not part of the application.
*/

#ifndef PROTO3D_SCENE_GL_H
#define PROTO3D_SCENE_GL_H

#include "gltf_rig.h"
#include "proto_log.h"

#include <QOpenGLFunctions_3_3_Core>
#include <QVector3D>

#include <memory>
#include <vector>

class QOpenGLShaderProgram;

struct ProtoOptions {
    bool figures = true; //!< human figures, or cylinders
    bool shadows = true;
    int shadow_size = 4096;
    double bench_seconds = 0.0; //!< > 0: measure and quit
    int start_frame = 1000;
};

class SceneGL
    : protected QOpenGLFunctions_3_3_Core {
public:
    ProtoOptions options_;

    // camera: orbit around the ball
    float azimuth_ = 90.0f; //!< degrees, from +X toward +Z: from the side of +y, as a broadcast
    float elevation_ = 32.0f;
    float distance_ = 72.0f;

    SceneGL( const ProtoLog & log,
             const GltfRig & rig,
             const ProtoOptions & options );
    ~SceneGL();

    //! create the resources in the current context
    bool initialize();
    //! delete the resources, in the current context
    void release();

    /*!
      \brief draw a frame.
      \param fbo the framebuffer to draw into
      \param fb_w, fb_h its size in device pixels
      \param play_time cycles from the start frame
      \return the CPU time of the frame (ms): the poses and the commands
    */
    double render( GLuint fbo,
                   int fb_w,
                   int fb_h,
                   double play_time );

    //! the GPU time of a recent frame (ms), or < 0 if none is ready yet
    double takeGpuMs();

    //! the CPU time of the poses of the last frame (ms)
    double poseMs() const { return M_pose_ms; }

private:
    const ProtoLog & M_log;
    const GltfRig & M_rig;

    std::unique_ptr< QOpenGLShaderProgram > M_skinned;
    std::unique_ptr< QOpenGLShaderProgram > M_skinned_depth;
    std::unique_ptr< QOpenGLShaderProgram > M_instanced;
    std::unique_ptr< QOpenGLShaderProgram > M_instanced_depth;
    std::unique_ptr< QOpenGLShaderProgram > M_ground;

    GLuint M_rig_vao = 0, M_rig_vbo = 0, M_rig_ibo = 0;
    GLuint M_cyl_vao = 0, M_cyl_vbo = 0, M_cyl_ibo = 0, M_cyl_inst = 0;
    GLsizei M_cyl_count = 0;
    GLuint M_sphere_vao = 0, M_sphere_vbo = 0, M_sphere_ibo = 0, M_sphere_inst = 0;
    GLsizei M_sphere_count = 0;
    GLuint M_ground_vao = 0, M_ground_vbo = 0;
    GLuint M_ground_tex = 0;
    GLuint M_joint_tex = 0;
    GLuint M_shadow_fbo = 0, M_shadow_tex = 0;
    GLuint M_queries[2] = { 0, 0 };
    int M_query_index = 0;
    bool M_query_pending[2] = { false, false };
    double M_gpu_ms = -1.0;
    double M_pose_ms = 0.0;

    std::vector< float > M_joint_data; //!< players * joints * 16
    std::vector< float > M_cyl_data; //!< per instance: mat4 + color
    std::vector< float > M_sphere_data;
    float M_colors[PROTO_PLAYERS * 3];

    void createRig();
    void createCylinder();
    void createSphere();
    void createGround();
    void createShadowMap();
    void updateInstances( double play_time,
                          QVector3D * ball );
    void drawScene( bool depth_only,
                    const QMatrix4x4 & view_proj,
                    const QMatrix4x4 & light_view_proj );
};

#endif
