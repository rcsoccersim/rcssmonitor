// -*-c++-*-

/*!
  \file export_job.cpp
  \brief the export of frames, an image at a time. Source File.
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

#include "export_job.h"

#include "field_canvas.h"
#include "frame_sink.h"

#include <QTimer>

/*-------------------------------------------------------------------*/
ExportJob::ExportJob( FieldCanvas & canvas,
                      FrameSink * sink,
                      const std::vector< std::size_t > & frames,
                      const QSize & pixels,
                      const qreal dpr,
                      const double fps,
                      QObject * parent )
    : QObject( parent ),
      M_canvas( canvas ),
      M_sink( sink ),
      M_frames( frames ),
      M_pixels( pixels ),
      M_dpr( dpr ),
      M_fps( fps ),
      M_done( 0 ),
      M_started( false ),
      M_ended( false )
{
    M_sink->setParent( this );
    connect( M_sink, &FrameSink::readyForFrame,
             this, [this]() { if ( isRunning() ) step(); } );
    connect( M_sink, &FrameSink::finished,
             this, [this]( bool ok, const QString & message ) { end( ok, message ); } );
}

/*-------------------------------------------------------------------*/
ExportJob::~ExportJob()
{
    if ( isRunning() )
    {
        M_ended = true;
        M_sink->abort();
    }
}

/*-------------------------------------------------------------------*/
void
ExportJob::start()
{
    if ( M_started )
    {
        return;
    }
    M_started = true;

    if ( M_frames.empty() )
    {
        QTimer::singleShot( 0, this, [this]() { end( false, tr( "There is no frame to export." ) ); } );
        return;
    }
    if ( ! M_sink->open( M_pixels, M_fps ) )
    {
        const QString error = M_sink->errorString();
        QTimer::singleShot( 0, this, [this, error]() { end( false, error ); } );
        return;
    }
    QTimer::singleShot( 0, this, [this]() { step(); } );
}

/*-------------------------------------------------------------------*/
void
ExportJob::abort()
{
    if ( ! isRunning() )
    {
        return;
    }
    M_sink->abort();
    end( false, tr( "Stopped after %1 of %2 images." ).arg( M_done ).arg( M_frames.size() ) );
}

/*-------------------------------------------------------------------*/
void
ExportJob::step()
{
    if ( ! isRunning()
         || M_done >= M_frames.size() )
    {
        return;
    }

    if ( M_pending.isNull() )
    {
        M_pending = M_canvas.renderFrame( M_frames[M_done], M_pixels, M_dpr );
    }

    switch ( M_sink->write( M_pending ) ) {
    case FrameSink::ACCEPTED:
        M_pending = QImage();
        ++M_done;
        emit progressed( M_done, M_frames.size() );
        if ( ! isRunning() )
        {
            // aborted while the progress was told
            return;
        }
        if ( M_done >= M_frames.size() )
        {
            M_sink->finish(); // finished() of the sink ends this
        }
        else
        {
            QTimer::singleShot( 0, this, [this]() { step(); } );
        }
        break;
    case FrameSink::BUSY:
        // again after readyForFrame()
        break;
    case FrameSink::FAILED:
        M_sink->abort();
        end( false, M_sink->errorString() );
        break;
    }
}

/*-------------------------------------------------------------------*/
void
ExportJob::end( const bool ok,
                const QString & message )
{
    if ( M_ended )
    {
        return;
    }
    M_ended = true;
    M_pending = QImage();
    emit finished( ok, message );
}
