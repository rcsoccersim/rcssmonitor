// -*-c++-*-

/*!
  \file frame_sink.h
  \brief where the exported frames go: the interface. Header File.
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

#ifndef RCSSMONITOR_FRAME_SINK_H
#define RCSSMONITOR_FRAME_SINK_H

#include <QImage>
#include <QObject>
#include <QSize>
#include <QString>

/*!
  \class FrameSink
  \brief where the exported images go (docs/plan/2026-11-22-03_frame-export.md
  4.5): image files, or a video. open(), then write() an image at a time,
  then finish() (or abort()). A sink that cannot take an image now (e.g.
  the pipe to ffmpeg is full) answers BUSY, and tells readyForFrame() when
  it can. The end is told by finished(), after finish() returns or later.
*/
class FrameSink
    : public QObject {

    Q_OBJECT

public:

    //! the answer of write()
    enum WriteResult {
        ACCEPTED, //!< taken
        BUSY, //!< not taken now: write it again after readyForFrame()
        FAILED, //!< an error (errorString())
    };

protected:

    QString M_error;

public:

    explicit
    FrameSink( QObject * parent = nullptr )
        : QObject( parent )
      { }

    ~FrameSink() override = default;

    /*!
      \brief be ready for the images
      \param pixels the size of the images
      \param fps the images a second (for a video)
      \return false on an error (errorString())
    */
    virtual
    bool open( const QSize & pixels,
               const double fps ) = 0;

    //! take an image
    virtual
    WriteResult write( const QImage & image ) = 0;

    //! no more images: finished() tells the end
    virtual
    void finish() = 0;

    //! stop: what was written so far is kept as it is (images), or removed (a video)
    virtual
    void abort() = 0;

    //! the reason of the last error
    const QString & errorString() const { return M_error; }

signals:

    //! an image can be written again after BUSY
    void readyForFrame();

    //! the end after finish(): ok, or the reason of an error
    void finished( bool ok,
                   const QString & message );
};

#endif
