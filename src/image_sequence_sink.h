// -*-c++-*-

/*!
  \file image_sequence_sink.h
  \brief the exported frames as image files. Header File.
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

#ifndef RCSSMONITOR_IMAGE_SEQUENCE_SINK_H
#define RCSSMONITOR_IMAGE_SEQUENCE_SINK_H

#include "frame_sink.h"

#include <QByteArray>
#include <QString>
#include <QStringList>

/*!
  \class ImageSequenceSink
  \brief the exported images as files in a directory, by QImageWriter
  (docs/plan/2026-11-22-03_frame-export.md 4.5), one file a name given.
  The files written are kept when it is aborted.
*/
class ImageSequenceSink
    : public FrameSink {

    Q_OBJECT

private:

    QString M_dir;
    QStringList M_names;
    QByteArray M_format;
    int M_written;

public:

    /*!
      \param dir the directory (made if it is not there)
      \param names the file names of the images, in order
      \param format the format of QImageWriter (e.g. "png", "jpg")
    */
    ImageSequenceSink( const QString & dir,
                       const QStringList & names,
                       const QByteArray & format,
                       QObject * parent = nullptr );

    //! the image formats that can be written: png and jpg first
    static
    QList< QByteArray > formats();

    //! the number of the images written
    int written() const { return M_written; }

    const QString & directory() const { return M_dir; }

    bool open( const QSize & pixels,
               const double fps ) override;
    WriteResult write( const QImage & image ) override;
    void finish() override;
    void abort() override;
};

#endif
