// -*-c++-*-

/*
  A prototype of the 3D view (docs/PLAN_3D-VIEW.md, Phase 0, D1): the scene
  of SceneGL in a QOpenGLWidget, with the input, the playback and the
  measurement. Not part of the application.
*/

#ifndef PROTO3D_VIEW_GL_H
#define PROTO3D_VIEW_GL_H

#include "scene_gl.h"

#include <QElapsedTimer>
#include <QOpenGLWidget>

#include <vector>

class ViewGL
    : public QOpenGLWidget {
    Q_OBJECT

public:
    ViewGL( const ProtoLog & log,
            const GltfRig & rig,
            const ProtoOptions & options );
    ~ViewGL() override;

protected:
    void initializeGL() override;
    void paintGL() override;
    void keyPressEvent( QKeyEvent * event ) override;
    void mousePressEvent( QMouseEvent * event ) override;
    void mouseMoveEvent( QMouseEvent * event ) override;
    void wheelEvent( QWheelEvent * event ) override;

private:
    SceneGL M_scene;

    // playback
    QElapsedTimer M_clock;
    double M_play_time = 0.0; //!< cycles from the start frame
    qint64 M_last_ns = 0;
    bool M_paused = false;
    QPoint M_last_mouse;

    // measurement
    QElapsedTimer M_fps_clock;
    int M_fps_frames = 0;
    double M_fps = 0.0;
    double M_cpu_ms = 0.0, M_cpu_sum = 0.0;
    double M_pose_ms = 0.0, M_pose_sum = 0.0;
    double M_gpu_ms = 0.0, M_gpu_sum = 0.0;
    int M_gpu_samples = 0;
    QElapsedTimer M_bench_clock;
    bool M_benching = false;
    std::vector< double > M_bench_intervals;
    std::vector< double > M_bench_cpu, M_bench_gpu, M_bench_pose;
    qint64 M_last_swap_ns = -1;

    void drawOverlay();
    void finishBench();
    void onFrameSwapped();
};

/*!
  \brief draw frames into an offscreen framebuffer, without a window: to
  check the drawing where no window can be shown.
  \return 0 on success
*/
int render_offscreen( const ProtoLog & log,
                      const GltfRig & rig,
                      const ProtoOptions & options,
                      int width,
                      int height,
                      int frames,
                      const QString & image_path );

#endif
