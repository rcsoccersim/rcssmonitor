// -*-c++-*-

/*!
  \file field_view_3d.cpp
  \brief the 3D view: a window drawn by OpenGL (QOpenGLWindow). Source File.
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

#include "field_view_3d.h"

#include "guides_3d.h"
#include "interpolate.h"
#include "offscreen_view_3d.h"

#include "disp_holder.h"
#include "log_player.h"
#include "options.h"

#include <QFontMetrics>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include <cmath>
#include <cstdio>

/*-------------------------------------------------------------------*/
FieldView3D::FieldView3D( DispHolder & disp_holder,
                          const LogPlayer & log_player,
                          QWindow * parent )
    : QOpenGLWindow( QOpenGLWindow::NoPartialUpdate, parent ),
      M_disp_holder( disp_holder ),
      M_log_player( log_player ),
      M_renderer( disp_holder ),
      M_newest( nullptr ),
      M_moving( false )
{
    setFormat( surfaceFormat() );
    // the keys are the main window's actions: the focus stays there
    setFlag( Qt::WindowDoesNotAcceptFocus );

    // While things move, a frame is drawn as soon as the one before is
    // shown: paced by the display (60 times a second on most screens),
    // without a timer of our own.
    connect( this, SIGNAL( frameSwapped() ),
             this, SLOT( drawNext() ) );

    // the times of the frames, to measure (docs/plan/2026-11-11_3d-view.md §10.3)
    M_timing.on = ! qEnvironmentVariableIsEmpty( "RCSSMONITOR_3D_TIMING" );
}

/*-------------------------------------------------------------------*/
FieldView3D::~FieldView3D()
{
    // the GPU's resources are deleted in their context
    makeCurrent();
    M_renderer.release();
    doneCurrent();
}

/*-------------------------------------------------------------------*/
QSurfaceFormat
FieldView3D::surfaceFormat()
{
    QSurfaceFormat format;
    format.setVersion( 3, 3 );
    format.setProfile( QSurfaceFormat::CoreProfile );
    format.setDepthBufferSize( 24 );
    format.setStencilBufferSize( 8 );
    return format;
}

/*-------------------------------------------------------------------*/
QString
FieldView3D::unavailableReason()
{
    // Try what the widget will need, before it is made: a context of OpenGL
    // 3.3 core with its functions. Without a GPU or with an old driver, it
    // fails here, and the 2D view stays (MainWindow::toggleView3D()).

    // these platforms show no OpenGL widgets
    const QString platform = QGuiApplication::platformName();
    if ( platform == "offscreen"
         || platform == "minimal"
         || platform == "vnc"
         || platform == "linuxfb" )
    {
        return QObject::tr( "The platform \"%1\" does not show OpenGL widgets." ).arg( platform );
    }

    return OffscreenView3D::unavailableReason();
}

/*-------------------------------------------------------------------*/
void
FieldView3D::initializeGL()
{
    // the GPU's resources, and the human figures read once; a new context
    // has no images yet: they are sent again
    M_renderer.initialize();
    M_renderer.setTiming( M_timing.on );
    M_gpu_timing = M_timing.on;
}

/*-------------------------------------------------------------------*/
void
FieldView3D::paintGL()
{
    if ( ! M_renderer.ready() )
    {
        return;
    }

    // The frames to draw, and how far between them (§5.2). Live, by a clock
    // going on with the simulation, drawn toward where the newest frame's
    // coming puts it (Interpolate::LiveClock): not jumping at every frame.
    // The delayed live view (§5.10) puts it further behind, at the frame the
    // 2D view shows (LogPlayer::liveDelayFrames()).
    const DispCont & frames = M_disp_holder.dispCont();
    const rcss::rcg::DispInfoT * newest = ( frames.empty() ? nullptr : frames.back().get() );
    if ( newest != M_newest )
    {
        M_newest = newest;
        M_newest_clock.start();
    }
    const bool live = M_log_player.isLiveMode();
    const double step = std::max( 1, M_disp_holder.serverParam().simulator_step_ );
    const double since_newest = ( M_newest_clock.isValid() ? M_newest_clock.nsecsElapsed() * 1.0e-6 / step : 1.0 );
    const double since_drawn = ( M_live_tick.isValid() ? double( M_live_tick.restart() ) / step : 0.0 );
    if ( ! M_live_tick.isValid() ) M_live_tick.start();
    Interpolate::Position at;
    const std::size_t delay_frames = M_log_player.liveDelayFrames();
    if ( live && Interpolate::waitingLive( frames.size(), delay_frames ) )
    {
        // the delayed live view, right after connecting: the first frame
        M_live_clock.reset();
        at = Interpolate::positionAt( frames.size(), 0.0 );
    }
    else if ( live && frames.size() >= 2 )
    {
        const double time = M_live_clock.advance( Interpolate::liveTime( frames.size(), since_newest,
                                                                         delay_frames ),
                                                  since_drawn,
                                                  double( frames.size() - 1 ) );
        at = Interpolate::positionAt( frames.size(), time );
    }
    else
    {
        M_live_clock.reset();
        at = Interpolate::position( frames.size(),
                                    M_disp_holder.currentIndex(),
                                    live,
                                    M_log_player.alpha(),
                                    since_newest );
    }

    // what to draw where, and what the cameras follow
    const Options & opt = Options::instance();
    M_renderer.build( at, live, delay_frames, M_camera.mode() );

    // the camera, after the ball for the time since the last frame
    const double dt = ( M_frame_clock.isValid() ? M_frame_clock.restart() * 1.0e-3 : 0.0 );
    if ( ! M_frame_clock.isValid() ) M_frame_clock.start();
    const float aspect = float( width() ) / float( std::max( 1, height() ) );
    M_camera.setCloseup( float( opt.closeupDistance3D() ),
                         float( opt.closeupElevation3D() ) );
    M_camera.setPlayerCamera( opt.playerCamera() );
    const CameraPose camera = M_camera.update( dt, aspect, M_renderer.subject() );
    M_drawn_camera = camera;
    M_drawn = true;

    // draw again after this frame (drawNext()) while playing a log, live
    // until the newest frame is reached, and while the camera glides
    M_moving = ( M_log_player.isPlaying()
                 || ( live && ( since_newest <= double( Interpolate::LIVE_DELAY )
                                || M_live_clock.time() < double( frames.size() ) - 1.0 ) )
                 || ! M_camera.settled() );

    // the GPU's times, for the timing told or the frames per second shown
    const bool gpu_timing = ( M_timing.on || opt.fps3D() );
    if ( gpu_timing != M_gpu_timing )
    {
        M_renderer.setTiming( gpu_timing );
        M_gpu_timing = gpu_timing;
    }
    // draw into the window's framebuffer, in physical pixels
    const qreal dpr = devicePixelRatio();
    M_renderer.render( defaultFramebufferObject(),
                       camera,
                       size(),
                       dpr,
                       opt.antialias3D() ? Options::ANTIALIAS_SAMPLES_3D : 0 );
    {
        double gpu = 0.0;
        const int gpu_frames = M_renderer.takeGpuTime( &gpu );
        M_timing.gpu += gpu;
        M_timing.gpu_frames += gpu_frames;
        M_fps.gpu( gpu, gpu_frames );
        if ( ! M_fps_clock.isValid() ) M_fps_clock.start();
        M_fps.frame( M_fps_clock.nsecsElapsed() * 1.0e-9 );
    }
    if ( M_timing.on )
    {
        M_timing.submit += M_renderer.submitTime();
        M_timing.build += M_renderer.buildTime();
        M_timing.players += M_renderer.poseTime();
        M_timing.figures = M_renderer.figureCount();
        if ( M_timing.since_frame.isValid() )
        {
            M_timing.longest = std::max( M_timing.longest, M_timing.since_frame.nsecsElapsed() * 1.0e-6 );
        }
        M_timing.since_frame.start();
        ++M_timing.frames;
        reportTiming();
    }

    // the HUD over the scene, as the 2D view draws it
    if ( ! frames.empty() )
    {
        QPainter painter( this );
        // on a scaled screen, a painter of an OpenGL view may have its window
        // in physical pixels while it draws in logical ones: the score board,
        // placed at the bottom of the window, fell off the view. The window
        // in logical pixels, as a painter of the 2D view has it.
        const QRect logical( QPoint( 0, 0 ), size() );
        painter.setViewport( logical );
        painter.setWindow( logical );
        M_renderer.drawHud( painter, camera, size(), dpr, M_camera.mode() );

        // the frames per second at the bottom right, over the minimap if shown;
        // "-" in the last frame before the view stops drawing (it stays)
        if ( opt.fps3D() )
        {
            const bool minimap = View3DRenderer::minimapShown( M_camera.mode() );
            const QString text = M_fps.text( M_fps_clock.nsecsElapsed() * 1.0e-9, ! M_moving );
            painter.setFont( QGuiApplication::font() ); // the application's, not the player font of the texts
            const QFontMetrics metrics( painter.font() );
            const int margin = 4;
            QRect box( 0, 0, metrics.horizontalAdvance( text ) + 2 * margin, metrics.height() + 2 * margin );
            double scale = 1.0;
            const int bottom = ( minimap
                                 ? int( std::floor( Guides3D::minimapRect( width(), height(), &scale ).top() ) ) - margin
                                 : height() - margin );
            box.moveBottomRight( QPoint( width() - 1 - margin, bottom ) );
            painter.setPen( Qt::NoPen );
            painter.setBrush( QColor( 0, 0, 0, 140 ) );
            painter.drawRect( box );
            painter.setPen( Qt::white );
            painter.drawText( box, Qt::AlignCenter, text );
        }
    }
}

/*-------------------------------------------------------------------*/
void
FieldView3D::reportTiming()
{
    if ( ! M_timing.period.isValid() )
    {
        M_timing.period.start();
        return;
    }
    const double seconds = M_timing.period.nsecsElapsed() * 1.0e-9;
    if ( seconds < TIMING_PERIOD || M_timing.frames == 0 )
    {
        return;
    }
    const double gpu = M_timing.gpu;
    const int gpu_frames = M_timing.gpu_frames;
    const double n = M_timing.frames;
    const qreal dpr = devicePixelRatio();
    std::fprintf( stderr,
                  "(3D timing) %dx%d px%s, %s, %d figures: %.1f fps (longest %.1f ms);"
                  " CPU build %.3f ms (players %.3f), submit %.3f ms; GPU %.3f ms\n",
                  int( std::lround( width() * dpr ) ), int( std::lround( height() * dpr ) ),
                  Options::instance().antialias3D() ? " smooth" : "",
                  Options::instance().playerModel3D().toUtf8().constData(),
                  M_timing.figures,
                  n / seconds, M_timing.longest,
                  M_timing.build / n, M_timing.players / n, M_timing.submit / n,
                  gpu_frames > 0 ? gpu / gpu_frames : -1.0 );
    const bool on = M_timing.on;
    M_timing = Timing();
    M_timing.on = on;
    M_timing.period.start();
    M_timing.since_frame.start();
}

/*-------------------------------------------------------------------*/
void
FieldView3D::setCameraMode( const CameraMode mode )
{
    M_camera.setMode( mode );
    update();
}

/*-------------------------------------------------------------------*/
void
FieldView3D::zoomCamera( const double factor )
{
    if ( factor <= 0.0 )
    {
        M_camera.resetZoom();
    }
    else
    {
        M_camera.zoom( float( factor ) );
    }
    // the close-up cameras' distance is a setting, which stays
    if ( isCloseup( M_camera.mode() ) )
    {
        Options::instance().setCloseupDistance3D( M_camera.closeupDistance() );
    }
    update();
}

/*-------------------------------------------------------------------*/
void
FieldView3D::mousePressEvent( QMouseEvent * event )
{
    // the right button: the referee's menu, at the player or the ground aimed at
    if ( event->button() == Qt::RightButton )
    {
        emit refereeMenuRequested( targetAt( event->pos() ), mapToGlobal( event->pos() ) );
        event->accept();
        return;
    }
    // where a drag starts
    M_last_mouse = event->pos();
    QOpenGLWindow::mousePressEvent( event );
}

/*-------------------------------------------------------------------*/
void
FieldView3D::mouseMoveEvent( QMouseEvent * event )
{
    // the drag since the last move, in logical pixels as the height
    const QPoint delta = event->pos() - M_last_mouse;
    M_last_mouse = event->pos();
    const float dx = float( delta.x() ), dy = float( delta.y() ), h = float( height() );

    bool changed = false;
    if ( event->buttons() & Qt::LeftButton )
    {
        // turn around what the camera looks at, or with a modifier move it
        if ( event->modifiers() & ( Qt::ShiftModifier | Qt::ControlModifier | Qt::MetaModifier ) )
        {
            changed = M_camera.pan( dx, dy, h );
        }
        else
        {
            changed = M_camera.orbit( dx, dy, h );
        }
    }
    else if ( event->buttons() & Qt::MiddleButton )
    {
        // down: farther, up: nearer, 5% per 100 pixels (as OrbitControls)
        changed = M_camera.dolly( float( std::pow( Cameras3D::DRAG_ZOOM, -dy * 0.01 ) ) );
    }
    else
    {
        QOpenGLWindow::mouseMoveEvent( event );
        return;
    }

    if ( changed )
    {
        emit cameraModeChanged( M_camera.mode() );
    }
    update();
}

/*-------------------------------------------------------------------*/
void
FieldView3D::viewFrom( const Viewpoint viewpoint )
{
    M_camera.viewFrom( viewpoint );
    update();
}

/*-------------------------------------------------------------------*/
RefereeTarget
FieldView3D::targetAt( const QPoint & point ) const
{
    if ( ! M_drawn )
    {
        return RefereeTarget();
    }
    return M_renderer.targetAt( M_drawn_camera, QSizeF( size() ), QPointF( point ) );
}

/*-------------------------------------------------------------------*/
View3DState
FieldView3D::state() const
{
    if ( ! M_drawn )
    {
        return View3DState();
    }
    return M_renderer.state( M_drawn_camera );
}

/*-------------------------------------------------------------------*/
void
FieldView3D::wheelEvent( QWheelEvent * event )
{
    emit wheelTurned( event->angleDelta().y() );
    event->accept();
}

/*-------------------------------------------------------------------*/
void
FieldView3D::drawNext()
{
    if ( M_moving )
    {
        update();
    }
}
