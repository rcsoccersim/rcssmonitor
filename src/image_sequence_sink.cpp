// -*-c++-*-

/*!
  \file image_sequence_sink.cpp
  \brief the exported frames as image files. Source File.
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

#include "image_sequence_sink.h"

#include <QDir>
#include <QImageWriter>

/*-------------------------------------------------------------------*/
ImageSequenceSink::ImageSequenceSink( const QString & dir,
                                      const QStringList & names,
                                      const QByteArray & format,
                                      QObject * parent )
    : FrameSink( parent ),
      M_dir( dir ),
      M_names( names ),
      M_format( format ),
      M_written( 0 )
{

}

/*-------------------------------------------------------------------*/
QList< QByteArray >
ImageSequenceSink::formats()
{
    QList< QByteArray > list;
    const QList< QByteArray > supported = QImageWriter::supportedImageFormats();
    for ( const char * f : { "png", "jpg" } )
    {
        if ( supported.contains( f ) ) list.push_back( f );
    }
    return list;
}

/*-------------------------------------------------------------------*/
bool
ImageSequenceSink::open( const QSize &,
                         const double )
{
    M_written = 0;
    M_error.clear();
    if ( ! QDir().mkpath( M_dir ) )
    {
        M_error = tr( "Could not make the directory %1." ).arg( QDir::toNativeSeparators( M_dir ) );
        return false;
    }
    return true;
}

/*-------------------------------------------------------------------*/
FrameSink::WriteResult
ImageSequenceSink::write( const QImage & image )
{
    if ( M_written >= M_names.size() )
    {
        M_error = tr( "More images than their names." );
        return FAILED;
    }

    const QString path = QDir( M_dir ).filePath( M_names[M_written] );
    QImageWriter writer( path, M_format );
    if ( ! writer.write( image ) )
    {
        M_error = tr( "Could not write %1:\n%2" ).arg( QDir::toNativeSeparators( path ), writer.errorString() );
        return FAILED;
    }
    ++M_written;
    return ACCEPTED;
}

/*-------------------------------------------------------------------*/
void
ImageSequenceSink::finish()
{
    // the plural by hand: without a translation, "%n image(s)" is shown as it is
    emit finished( true, ( M_written == 1
                           ? tr( "1 image in %1" ).arg( QDir::toNativeSeparators( M_dir ) )
                           : tr( "%1 images in %2" ).arg( M_written ).arg( QDir::toNativeSeparators( M_dir ) ) ) );
}

/*-------------------------------------------------------------------*/
void
ImageSequenceSink::abort()
{
    // the images written are kept
}
