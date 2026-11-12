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
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions_3_3_Core>

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

    // paced by the display: the next frame after this one is shown
    connect( this, SIGNAL( frameSwapped() ),
             this, SLOT( drawNext() ) );
}

/*-------------------------------------------------------------------*/
FieldView3D::~FieldView3D()
{
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

    const QString key = GroundTexture::key( M_disp_holder.serverParam() );
    if ( key != M_ground_key )
    {
        M_scene.setGround( GroundTexture::draw( *M_field_painter ) );
        M_ground_key = key;
    }

    // the frames to draw, and how far between them (§5.2)
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

    M_content.clear();
    SceneBuilder::addGoals( M_content );
    if ( ! frames.empty() )
    {
        const rcss::rcg::ShowInfoT show = Interpolate::show( *frames[at.index],
                                                             ( at.next != at.index ? frames[at.next].get() : nullptr ),
                                                             at.alpha );
        SceneBuilder::addShow( M_content, show, M_disp_holder, Options::instance() );
    }
    M_moving = ( M_log_player.isPlaying() || ( live && since_newest < 1.0 ) );

    // the camera of a broadcast at the center, until the cameras come (4-4)
    CameraPose camera;
    const float elevation = 32.0f * float( M_PI ) / 180.0f;
    camera.target = QVector3D( 0.0f, 0.0f, 0.0f );
    camera.position = QVector3D( 0.0f, 72.0f * std::sin( elevation ), 72.0f * std::cos( elevation ) );

    const qreal dpr = devicePixelRatioF();
    M_scene.render( defaultFramebufferObject(),
                    static_cast< int >( std::lround( width() * dpr ) ),
                    static_cast< int >( std::lround( height() * dpr ) ),
                    camera,
                    M_content );
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
