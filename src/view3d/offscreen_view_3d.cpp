// -*-c++-*-

/*!
  \file offscreen_view_3d.cpp
  \brief the 3D view drawn into images, without a window. Source File.
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

#include "offscreen_view_3d.h"

#include "field_view_3d.h"

#include "disp_holder.h"
#include "frame_scope.h"
#include "options.h"

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_3_3_Core>
#include <QPainter>

#include <algorithm>
#include <cmath>

/*-------------------------------------------------------------------*/
OffscreenView3D::OffscreenView3D( DispHolder & disp_holder )
    : M_disp_holder( disp_holder ),
      M_renderer( disp_holder ),
      M_dt( 1.0 / 30.0 ),
      M_ready( false ),
      M_drawn( false )
{

}

/*-------------------------------------------------------------------*/
OffscreenView3D::~OffscreenView3D()
{
    // the GPU's resources are deleted in their context
    if ( M_context
         && M_surface
         && M_context->makeCurrent( M_surface.get() ) )
    {
        M_fbo.reset();
        M_renderer.release();
        M_context->doneCurrent();
    }
}

/*-------------------------------------------------------------------*/
QString
OffscreenView3D::unavailableReason()
{
    // Try what the drawing will need: a context of OpenGL 3.3 core with its
    // functions. Without a GPU or with an old driver, it fails here.
    QOpenGLContext context;
    context.setFormat( FieldView3D::surfaceFormat() );
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
bool
OffscreenView3D::initialize( QString * error )
{
    if ( M_ready )
    {
        return true;
    }

    // the format of the view, without samples: the smooth edges are Scene3D's own
    QSurfaceFormat format = FieldView3D::surfaceFormat();
    format.setSamples( 0 );
    std::unique_ptr< QOpenGLContext > context( new QOpenGLContext() );
    context->setFormat( format );
    if ( ! context->create()
         || context->format().version() < qMakePair( 3, 3 ) )
    {
        if ( error ) *error = QObject::tr( "The 3D view cannot be drawn: OpenGL 3.3 is not available." );
        return false;
    }
    std::unique_ptr< QOffscreenSurface > surface( new QOffscreenSurface() );
    surface->setFormat( context->format() );
    surface->create();
    if ( ! context->makeCurrent( surface.get() ) )
    {
        if ( error ) *error = QObject::tr( "The 3D view cannot be drawn: OpenGL is not available." );
        return false;
    }
    M_ready = M_renderer.initialize();
    context->doneCurrent();
    if ( ! M_ready )
    {
        if ( error ) *error = QObject::tr( "The 3D view cannot be drawn: OpenGL 3.3 core is not available." );
        return false;
    }
    M_context = std::move( context );
    M_surface = std::move( surface );
    if ( error ) error->clear();
    return true;
}

/*-------------------------------------------------------------------*/
void
OffscreenView3D::start( const CameraController & camera,
                        const double fps )
{
    M_camera = camera;
    M_dt = ( fps > 0.0 ? 1.0 / fps : 0.0 );
}

/*-------------------------------------------------------------------*/
void
OffscreenView3D::startRange()
{
    M_camera.snap();
}

/*-------------------------------------------------------------------*/
QImage
OffscreenView3D::render( const ExportPart & part,
                         const std::size_t i,
                         const QSize & pixels,
                         const qreal dpr )
{
    const double position = ( i < part.positions.size()
                              ? part.positions[i]
                              : i < part.frames.size()
                              ? static_cast< double >( part.frames[i] )
                              : -1.0 );
    if ( position < 0.0 )
    {
        return QImage();
    }
    return renderAt( position, pixels, dpr, M_dt );
}

/*-------------------------------------------------------------------*/
QImage
OffscreenView3D::renderAt( const double position,
                           const QSize & pixels,
                           const qreal dpr,
                           const double dt )
{
    return draw( position, M_camera, dt, pixels, dpr );
}

/*-------------------------------------------------------------------*/
QImage
OffscreenView3D::renderStill( const std::size_t index,
                              const CameraController & camera,
                              const QSize & pixels,
                              const qreal dpr )
{
    // the camera not moved: where the view drew it last
    CameraController still = camera;
    return draw( static_cast< double >( index ), still, 0.0, pixels, dpr );
}

/*-------------------------------------------------------------------*/
View3DState
OffscreenView3D::state() const
{
    if ( ! M_drawn )
    {
        return View3DState();
    }
    return M_renderer.state( M_drawn_camera );
}

/*-------------------------------------------------------------------*/
QImage
OffscreenView3D::draw( const double position,
                       CameraController & camera,
                       const double dt,
                       const QSize & pixels,
                       const qreal dpr )
{
    const DispCont & frames = M_disp_holder.dispCont();
    if ( ! M_ready
         || pixels.isEmpty()
         || frames.empty() )
    {
        return QImage();
    }

    const qreal ratio = ( dpr > 0.0 ? dpr : 1.0 );
    const QSize logical( std::max( 1, static_cast< int >( std::lround( pixels.width() / ratio ) ) ),
                         std::max( 1, static_cast< int >( std::lround( pixels.height() / ratio ) ) ) );

    if ( ! M_context->makeCurrent( M_surface.get() ) )
    {
        return QImage();
    }
    if ( ! M_fbo
         || M_fbo->size() != pixels )
    {
        M_fbo.reset();
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment( QOpenGLFramebufferObject::CombinedDepthStencil );
        M_fbo.reset( new QOpenGLFramebufferObject( pixels, format ) );
        if ( ! M_fbo->isValid() )
        {
            M_fbo.reset();
            M_context->doneCurrent();
            return QImage();
        }
    }

    // the frame and the state of the view set back at the end (the score
    // board and the selection of a player follow the frame drawn)
    const Interpolate::Position at = Interpolate::positionAt( frames.size(), position );
    FrameScope scope( M_disp_holder, at.index );

    // the steps of a frame of the view (FieldView3D::paintGL())
    const Options & opt = Options::instance();
    M_renderer.build( at, false, 0, camera.mode() );
    camera.setCloseup( float( opt.closeupDistance3D() ),
                       float( opt.closeupElevation3D() ) );
    camera.setPlayerCamera( opt.playerCamera() );
    const float aspect = float( logical.width() ) / float( std::max( 1, logical.height() ) );
    const CameraPose pose = camera.update( dt, aspect, M_renderer.subject() );
    M_renderer.render( M_fbo->handle(),
                       pose,
                       logical,
                       ratio,
                       opt.antialias3D() ? Options::ANTIALIAS_SAMPLES_3D : 0 );
    M_drawn_camera = pose;
    M_drawn = true;

    // The colors as the screen shows them: the framebuffer's alpha (which
    // the translucent things change, and a window does not have) is not a
    // part of the image.
    QImage raw = M_fbo->toImage();
    M_context->doneCurrent();
    if ( raw.format() == QImage::Format_RGBA8888_Premultiplied
         || raw.format() == QImage::Format_RGBA8888 )
    {
        raw.reinterpretAsFormat( QImage::Format_RGBX8888 );
    }
    else if ( raw.format() == QImage::Format_ARGB32_Premultiplied
              || raw.format() == QImage::Format_ARGB32 )
    {
        raw.reinterpretAsFormat( QImage::Format_RGB32 );
    }
    QImage image = raw.convertToFormat( QImage::Format_RGB32 );
    // QPainter takes a ratio of 1 or more (a smaller one is drawn as 1), as
    // FieldCanvas::renderFrame()
    image.setDevicePixelRatio( std::max< qreal >( 1.0, ratio ) );

    // the HUD over the scene, laid out in the logical size as on the screen
    // (below the ratio 1, made smaller onto the pixels)
    {
        QPainter painter( &image );
        painter.setWindow( 0, 0, logical.width(), logical.height() );
        if ( ratio >= 1.0 )
        {
            painter.setViewport( 0, 0, logical.width(), logical.height() );
        }
        else
        {
            painter.setViewport( 0, 0, pixels.width(), pixels.height() );
        }
        M_renderer.drawHud( painter, pose, logical, ratio, camera.mode() );
    }
    return image;
}
