// -*-c++-*-

/*!
  \file field_view_3d.cpp
  \brief the 3D view: a QOpenGLWidget. Source File.
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

#include "ground_texture.h"
#include "interpolate.h"

#include "disp_holder.h"
#include "field_painter.h"
#include "log_player.h"
#include "options.h"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions_3_3_Core>
#include <QWheelEvent>

#include <cmath>
#include <iostream>

/*-------------------------------------------------------------------*/
FieldView3D::FieldView3D( DispHolder & disp_holder,
                          const LogPlayer & log_player,
                          QWidget * parent )
    : QOpenGLWidget( parent ),
      M_disp_holder( disp_holder ),
      M_log_player( log_player ),
      M_field_painter( new FieldPainter( disp_holder ) ),
      M_ready( false ),
      M_newest( nullptr ),
      M_moving( false )
{
    setFormat( surfaceFormat() );
    setFocusPolicy( Qt::WheelFocus );

    // While things move, a frame is drawn as soon as the one before is
    // shown: paced by the display (60 times a second on most screens),
    // without a timer of our own.
    connect( this, SIGNAL( frameSwapped() ),
             this, SLOT( drawNext() ) );
}

/*-------------------------------------------------------------------*/
FieldView3D::~FieldView3D()
{
    // the GPU's resources are deleted in their context
    makeCurrent();
    M_scene.release();
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
    format.setSamples( 4 );
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

    QOpenGLContext context;
    context.setFormat( surfaceFormat() );
    if ( ! context.create() )
    {
        return QObject::tr( "OpenGL is not available." );
    }
    const QPair< int, int > version = context.format().version();
    if ( version < qMakePair( 3, 3 ) )
    {
        return QObject::tr( "OpenGL 3.3 is not available (%1.%2)." ).arg( version.first ).arg( version.second );
    }

    QOffscreenSurface surface;
    surface.setFormat( context.format() );
    surface.create();
    if ( ! context.makeCurrent( &surface ) )
    {
        return QObject::tr( "OpenGL is not available." );
    }
    QOpenGLFunctions_3_3_Core functions;
    const bool ok = functions.initializeOpenGLFunctions();
    context.doneCurrent();
    return ok ? QString() : QObject::tr( "OpenGL 3.3 core is not available." );
}

/*-------------------------------------------------------------------*/
void
FieldView3D::initializeGL()
{
    M_ready = M_scene.initialize();
    if ( ! M_ready )
    {
        std::cerr << "(FieldView3D) OpenGL 3.3 core is not available" << std::endl;
    }
    M_ground_key.clear();
}

/*-------------------------------------------------------------------*/
void
FieldView3D::paintGL()
{
    if ( ! M_ready )
    {
        return;
    }

    // the ground image: drawn again only when its colors or options changed
    const QString key = GroundTexture::key( M_disp_holder.serverParam() );
    if ( key != M_ground_key )
    {
        M_scene.setGround( GroundTexture::draw( *M_field_painter ) );
        M_ground_key = key;
    }

    // The frames to draw, and how far between them (§5.2). Live, the
    // fraction is the time since the newest frame came, in simulation steps.
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
    const Interpolate::Position at = Interpolate::position( frames.size(),
                                                            M_disp_holder.currentIndex(),
                                                            live,
                                                            M_log_player.alpha(),
                                                            since_newest );

    // what to draw where: the goals, and the players and the ball on the way
    // between the two frames
    M_content.clear();
    SceneBuilder::addGoals( M_content );
    float ball_x = 0.0f, ball_y = 0.0f;
    if ( ! frames.empty() )
    {
        const rcss::rcg::ShowInfoT show = Interpolate::show( *frames[at.index],
                                                             ( at.next != at.index ? frames[at.next].get() : nullptr ),
                                                             at.alpha );
        SceneBuilder::addShow( M_content, show, M_disp_holder, Options::instance() );
        ball_x = show.ball_.x_;
        ball_y = show.ball_.y_;
    }

    // the camera, after the ball for the time since the last frame
    const double dt = ( M_frame_clock.isValid() ? M_frame_clock.restart() * 1.0e-3 : 0.0 );
    if ( ! M_frame_clock.isValid() ) M_frame_clock.start();
    const float aspect = float( width() ) / float( std::max( 1, height() ) );
    const CameraPose camera = M_camera.update( dt, aspect, ball_x, ball_y );

    // draw again after this frame (drawNext()) while playing a log, live
    // until the newest frame is reached, and while the camera glides
    M_moving = ( M_log_player.isPlaying()
                 || ( live && since_newest < 1.0 )
                 || ! M_camera.settled() );

    // draw into the widget's framebuffer, in physical pixels: on a screen
    // scaled by 1.5, a widget of 100 logical pixels has 150 of them
    const qreal dpr = devicePixelRatioF();
    M_scene.render( defaultFramebufferObject(),
                    static_cast< int >( std::lround( width() * dpr ) ),
                    static_cast< int >( std::lround( height() * dpr ) ),
                    camera,
                    M_content );
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
FieldView3D::mousePressEvent( QMouseEvent * event )
{
    // where a drag starts (the right button is left for the menu, Phase 5)
    M_last_mouse = event->pos();
    QOpenGLWidget::mousePressEvent( event );
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
        changed = M_camera.dolly( float( std::pow( Cameras3D::WHEEL_ZOOM, -dy * 0.01 ) ) );
    }
    else
    {
        QOpenGLWidget::mouseMoveEvent( event );
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
FieldView3D::wheelEvent( QWheelEvent * event )
{
    // a notch (120) away from the user: 5% nearer. The top camera does not
    // take it: as in 2D, it steps the frames (MainWindow::wheelEvent()).
    const double notches = event->angleDelta().y() / 120.0;
    if ( notches == 0.0
         || ! M_camera.takesMouse() )
    {
        QOpenGLWidget::wheelEvent( event );
        return;
    }
    if ( M_camera.dolly( float( std::pow( Cameras3D::WHEEL_ZOOM, notches ) ) ) )
    {
        emit cameraModeChanged( M_camera.mode() );
    }
    event->accept();
    update();
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
