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
                      const std::vector< ExportPart > & parts,
                      const QSize & pixels,
                      const qreal dpr,
                      const double fps,
                      QObject * parent )
    : QObject( parent ),
      M_canvas( canvas ),
      M_parts( parts ),
      M_pixels( pixels ),
      M_dpr( dpr ),
      M_fps( fps ),
      M_part( 0 ),
      M_done_in_part( 0 ),
      M_done( 0 ),
      M_total( 0 ),
      M_started( false ),
      M_ended( false )
{
    for ( const ExportPart & part : M_parts )
    {
        M_total += part.frames.size();
        FrameSink * sink = part.sink;
        sink->setParent( this );
        // only the sink of the part being exported is heard
        connect( sink, &FrameSink::readyForFrame,
                 this, [this, sink]() {
                     if ( isRunning() && M_parts[M_part].sink == sink ) step();
                 } );
        connect( sink, &FrameSink::finished,
                 this, [this, sink]( bool ok, const QString & message ) { partFinished( sink, ok, message ); } );
    }
}

/*-------------------------------------------------------------------*/
ExportJob::ExportJob( FieldCanvas & canvas,
                      FrameSink * sink,
                      const std::vector< std::size_t > & frames,
                      const QSize & pixels,
                      const qreal dpr,
                      const double fps,
                      QObject * parent )
    : ExportJob( canvas, std::vector< ExportPart >{ ExportPart{ sink, frames } }, pixels, dpr, fps, parent )
{

}

/*-------------------------------------------------------------------*/
ExportJob::~ExportJob()
{
    if ( isRunning() )
    {
        M_ended = true;
        M_parts[M_part].sink->abort();
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

    if ( M_total == 0 )
    {
        QTimer::singleShot( 0, this, [this]() { end( false, tr( "There is no frame to export." ) ); } );
        return;
    }
    openPart();
}

/*-------------------------------------------------------------------*/
void
ExportJob::openPart()
{
    // the parts without frames are skipped
    while ( M_part < M_parts.size()
            && M_parts[M_part].frames.empty() )
    {
        ++M_part;
    }
    if ( M_part >= M_parts.size() )
    {
        QTimer::singleShot( 0, this, [this]() { end( true, M_messages.join( QLatin1Char( '\n' ) ) ); } );
        return;
    }

    M_done_in_part = 0;
    FrameSink * sink = M_parts[M_part].sink;
    if ( ! sink->open( M_pixels, M_fps ) )
    {
        const QString error = sink->errorString();
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
    if ( M_part < M_parts.size() )
    {
        M_parts[M_part].sink->abort();
    }
    end( false, tr( "Stopped after %1 of %2 images." ).arg( M_done ).arg( M_total ) );
}

/*-------------------------------------------------------------------*/
void
ExportJob::step()
{
    if ( ! isRunning()
         || M_part >= M_parts.size() )
    {
        return;
    }
    const ExportPart & part = M_parts[M_part];
    if ( M_done_in_part >= part.frames.size() )
    {
        return;
    }

    if ( M_pending.isNull() )
    {
        M_pending = M_canvas.renderFrame( part.frames[M_done_in_part], M_pixels, M_dpr );
    }

    switch ( part.sink->write( M_pending ) ) {
    case FrameSink::ACCEPTED:
        M_pending = QImage();
        ++M_done_in_part;
        ++M_done;
        emit progressed( M_done, M_total );
        if ( ! isRunning() )
        {
            // aborted while the progress was told
            return;
        }
        if ( M_done_in_part >= part.frames.size() )
        {
            part.sink->finish(); // finished() of the sink goes on
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
        part.sink->abort();
        end( false, part.sink->errorString() );
        break;
    }
}

/*-------------------------------------------------------------------*/
void
ExportJob::partFinished( FrameSink * sink,
                         const bool ok,
                         const QString & message )
{
    if ( ! isRunning()
         || M_part >= M_parts.size()
         || M_parts[M_part].sink != sink )
    {
        return;
    }
    if ( ! ok )
    {
        end( false, message );
        return;
    }
    M_messages << message;
    ++M_part;
    openPart();
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
