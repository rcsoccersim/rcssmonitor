// -*-c++-*-

/*
  A prototype of the 3D view (docs/PLAN_3D-VIEW.md, Phase 0, D1). Not part
  of the application.
*/

#include "view_gl.h"

#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QPainter>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>

namespace {

double
percentile( std::vector< double > v,
            double q )
{
    if ( v.empty() ) return 0.0;
    std::sort( v.begin(), v.end() );
    return v[std::min( v.size() - 1, std::size_t( q * ( v.size() - 1 ) + 0.5 ) )];
}

double
mean( const std::vector< double > & v )
{
    return v.empty() ? 0.0 : std::accumulate( v.begin(), v.end(), 0.0 ) / v.size();
}

}

/*-------------------------------------------------------------------*/
ViewGL::ViewGL( const ProtoLog & log,
                const GltfRig & rig,
                const ProtoOptions & options )
    : M_scene( log, rig, options )
{
    setFocusPolicy( Qt::StrongFocus );
    connect( this, &QOpenGLWidget::frameSwapped, this, &ViewGL::onFrameSwapped );
}

/*-------------------------------------------------------------------*/
ViewGL::~ViewGL()
{
    makeCurrent();
    M_scene.release();
    doneCurrent();
}

/*-------------------------------------------------------------------*/
void
ViewGL::initializeGL()
{
    if ( ! M_scene.initialize() )
    {
        std::exit( 1 );
    }
    M_clock.start();
    M_fps_clock.start();
    if ( M_scene.options_.bench_seconds > 0.0 )
    {
        M_bench_clock.start();
    }
}

/*-------------------------------------------------------------------*/
void
ViewGL::paintGL()
{
    const qint64 now = M_clock.nsecsElapsed();
    if ( ! M_paused )
    {
        M_play_time += ( now - M_last_ns ) * 1e-9 / PROTO_STEP;
    }
    M_last_ns = now;

    const qreal dpr = devicePixelRatioF();
    const double cpu_ms = M_scene.render( defaultFramebufferObject(), int( width() * dpr ), int( height() * dpr ), M_play_time );
    M_cpu_sum += cpu_ms;
    M_pose_sum += M_scene.poseMs();
    if ( M_benching )
    {
        M_bench_cpu.push_back( cpu_ms );
        M_bench_pose.push_back( M_scene.poseMs() );
    }
    const double gpu_ms = M_scene.takeGpuMs();
    if ( gpu_ms >= 0.0 )
    {
        M_gpu_sum += gpu_ms;
        ++M_gpu_samples;
        if ( M_benching ) M_bench_gpu.push_back( gpu_ms );
    }

    drawOverlay();
}

/*-------------------------------------------------------------------*/
void
ViewGL::drawOverlay()
{
    QPainter painter( this );
    painter.setPen( Qt::white );
    QFont font = painter.font();
    font.setPointSize( 11 );
    painter.setFont( font );
    const qreal dpr = devicePixelRatioF();
    const ProtoOptions & o = M_scene.options_;
    const QString text = QString( "%1 fps   cpu %2 ms (poses %9 ms)   gpu %3 ms   %4x%5 px   %6   shadows %7 (%8 px)   F: figures/cylinders  S: shadows  Space: pause  drag/wheel: camera" )
        .arg( M_fps, 0, 'f', 1 )
        .arg( M_cpu_ms, 0, 'f', 2 )
        .arg( M_gpu_ms, 0, 'f', 2 )
        .arg( int( width() * dpr ) )
        .arg( int( height() * dpr ) )
        .arg( o.figures ? "figures" : "cylinders" )
        .arg( o.shadows ? "on" : "off" )
        .arg( o.shadow_size )
        .arg( M_pose_ms, 0, 'f', 2 );
    painter.fillRect( QRect( 0, 0, width(), 24 ), QColor( 0, 0, 0, 140 ) );
    painter.drawText( QRect( 8, 0, width() - 16, 24 ), Qt::AlignVCenter | Qt::AlignLeft, text );
}

/*-------------------------------------------------------------------*/
void
ViewGL::onFrameSwapped()
{
    const qint64 now = M_clock.nsecsElapsed();
    if ( M_benching && M_last_swap_ns >= 0 )
    {
        M_bench_intervals.push_back( ( now - M_last_swap_ns ) * 1e-6 );
    }
    M_last_swap_ns = now;

    const ProtoOptions & o = M_scene.options_;
    ++M_fps_frames;
    if ( M_fps_clock.elapsed() >= 1000 )
    {
        const double s = M_fps_clock.nsecsElapsed() * 1e-9;
        M_fps = M_fps_frames / s;
        M_cpu_ms = M_cpu_sum / std::max( 1, M_fps_frames );
        M_pose_ms = M_pose_sum / std::max( 1, M_fps_frames );
        M_gpu_ms = ( M_gpu_samples > 0 ? M_gpu_sum / M_gpu_samples : 0.0 );
        std::printf( "%.1f fps  cpu %.2f ms (poses %.2f ms)  gpu %.2f ms  (%s, shadows %s)\n", M_fps, M_cpu_ms, M_pose_ms, M_gpu_ms,
                     o.figures ? "figures" : "cylinders", o.shadows ? "on" : "off" );
        std::fflush( stdout );
        M_fps_frames = 0;
        M_cpu_sum = M_gpu_sum = M_pose_sum = 0.0;
        M_gpu_samples = 0;
        M_fps_clock.restart();
    }

    if ( o.bench_seconds > 0.0 )
    {
        const double s = M_bench_clock.nsecsElapsed() * 1e-9;
        if ( ! M_benching && s >= 2.0 ) // warm up
        {
            M_benching = true;
        }
        else if ( M_benching && s >= 2.0 + o.bench_seconds )
        {
            finishBench();
            return;
        }
    }
    update();
}

/*-------------------------------------------------------------------*/
void
ViewGL::finishBench()
{
    const qreal dpr = devicePixelRatioF();
    const ProtoOptions & o = M_scene.options_;
    const double interval = mean( M_bench_intervals );
    std::printf( "BENCH gl %s shadows=%s(%d) size=%dx%d frames=%zu fps=%.1f "
                 "interval_ms(mean=%.2f p50=%.2f p99=%.2f max=%.2f) cpu_ms(mean=%.2f p99=%.2f) pose_ms(mean=%.2f) gpu_ms(mean=%.2f p99=%.2f)\n",
                 o.figures ? "figures" : "cylinders",
                 o.shadows ? "on" : "off", o.shadow_size,
                 int( width() * dpr ), int( height() * dpr ),
                 M_bench_intervals.size(), interval > 0 ? 1000.0 / interval : 0.0,
                 interval, percentile( M_bench_intervals, 0.5 ), percentile( M_bench_intervals, 0.99 ),
                 percentile( M_bench_intervals, 1.0 ),
                 mean( M_bench_cpu ), percentile( M_bench_cpu, 0.99 ),
                 mean( M_bench_pose ),
                 mean( M_bench_gpu ), percentile( M_bench_gpu, 0.99 ) );
    std::fflush( stdout );
    QApplication::quit();
}

/*-------------------------------------------------------------------*/
void
ViewGL::keyPressEvent( QKeyEvent * event )
{
    ProtoOptions & o = M_scene.options_;
    switch ( event->key() ) {
    case Qt::Key_F: o.figures = ! o.figures; break;
    case Qt::Key_S: o.shadows = ! o.shadows; break;
    case Qt::Key_Space: M_paused = ! M_paused; break;
    case Qt::Key_Escape: QApplication::quit(); break;
    default: QOpenGLWidget::keyPressEvent( event ); break;
    }
}

void
ViewGL::mousePressEvent( QMouseEvent * event )
{
    M_last_mouse = event->pos();
}

void
ViewGL::mouseMoveEvent( QMouseEvent * event )
{
    const QPoint d = event->pos() - M_last_mouse;
    M_last_mouse = event->pos();
    M_scene.azimuth_ += d.x() * 0.3f;
    M_scene.elevation_ = std::clamp( M_scene.elevation_ + d.y() * 0.3f, 3.0f, 89.0f );
}

void
ViewGL::wheelEvent( QWheelEvent * event )
{
    M_scene.distance_ = std::clamp( M_scene.distance_ * std::pow( 0.9f, event->angleDelta().y() / 120.0f ), 5.0f, 200.0f );
}

/*-------------------------------------------------------------------*/
int
render_offscreen( const ProtoLog & log,
                  const GltfRig & rig,
                  const ProtoOptions & options,
                  int width,
                  int height,
                  int frames,
                  const QString & image_path )
{
    QOpenGLContext context;
    context.setFormat( QSurfaceFormat::defaultFormat() );
    QOffscreenSurface surface;
    surface.setFormat( context.format() );
    surface.create();
    if ( ! context.create() || ! context.makeCurrent( &surface ) )
    {
        std::fprintf( stderr, "could not make an OpenGL context\n" );
        return 1;
    }

    int status = 0;
    {
        QOpenGLFramebufferObjectFormat fbo_format;
        fbo_format.setAttachment( QOpenGLFramebufferObject::Depth );
        fbo_format.setSamples( 4 );
        QOpenGLFramebufferObject fbo( width, height, fbo_format );

        SceneGL scene( log, rig, options );
        if ( ! scene.initialize() )
        {
            return 1;
        }
        std::vector< double > cpu, wall, pose;
        for ( int i = 0; i < frames; ++i )
        {
            QElapsedTimer t;
            t.start();
            cpu.push_back( scene.render( fbo.handle(), width, height, i * 0.5 ) );
            context.functions()->glFinish();
            wall.push_back( t.nsecsElapsed() * 1e-6 );
            pose.push_back( scene.poseMs() );
        }
        std::printf( "OFFSCREEN %s shadows=%s size=%dx%d frames=%d pose_ms(mean=%.2f) cpu_ms(mean=%.2f) frame_ms(mean=%.2f p50=%.2f)\n",
                     options.figures ? "figures" : "cylinders", options.shadows ? "on" : "off",
                     width, height, frames, mean( pose ), mean( cpu ), mean( wall ), percentile( wall, 0.5 ) );
        if ( ! image_path.isEmpty() && ! fbo.toImage().save( image_path ) )
        {
            status = 1;
        }
        scene.release();
    }
    context.doneCurrent();
    return status;
}
