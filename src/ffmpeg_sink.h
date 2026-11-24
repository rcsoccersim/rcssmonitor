// -*-c++-*-

/*!
  \file ffmpeg_sink.h
  \brief the exported frames as a video, by an external ffmpeg. Header File.
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

#ifndef RCSSMONITOR_FFMPEG_SINK_H
#define RCSSMONITOR_FFMPEG_SINK_H

#include "frame_sink.h"

#include <QByteArray>
#include <QString>
#include <QStringList>

class QProcess;

/*!
  \class FfmpegSink
  \brief the exported images as a video, written by an external ffmpeg
  (docs/plan/2026-11-22-03_frame-export.md 4.5, S5, D13 to D15): the frames
  go to its standard input as raw video, and it encodes them (H.264 by
  libx264, at a quality (CRF) and a speed (preset); the format by the
  extension of the file). The arguments are given as a list, not through a
  shell.

  The pipe is not filled up: while more than two frames wait to be
  written, write() answers BUSY, and readyForFrame() tells when it can take
  one again. finish() closes the input, and finished() tells the end when
  ffmpeg has written the file. Its errors are told with the end of its
  standard error. abort() kills it and removes the file (D13).
*/
class FfmpegSink
    : public FrameSink {

    Q_OBJECT

public:

    //! what an ffmpeg can do (probe())
    struct Info {
        bool found = false; //!< it runs
        QString program; //!< the path run
        QString version; //!< the first line of -version
        QString encoder; //!< the H.264 encoder to use: libx264, or another one if it has none
        QString error; //!< why it cannot be used
    };

    //! the presets of libx264, the fastest first
    static const QStringList PRESETS;

private:

    QString M_program;
    QString M_file;
    QString M_encoder;
    int M_crf;
    QString M_preset;
    bool M_animation;

    QProcess * M_process;
    QByteArray M_stderr; //!< the end of its standard error
    qint64 M_frame_bytes;
    QSize M_size;
    bool M_busy; //!< write() answered BUSY: readyForFrame() is due
    bool M_starting; //!< in open(): a failure to start is told by its return
    bool M_finishing;
    bool M_ended; //!< finished() told

public:

    /*!
      \param program the path of ffmpeg (findProgram())
      \param file the video
      \param encoder the encoder (Info::encoder)
      \param crf the quality of libx264 (0 best to 51)
      \param preset the speed of libx264 (PRESETS)
      \param animation tuned for flat colors (the 2D view)
    */
    FfmpegSink( const QString & program,
                const QString & file,
                const QString & encoder,
                const int crf,
                const QString & preset,
                const bool animation,
                QObject * parent = nullptr );
    ~FfmpegSink() override;

    /*!
      \brief the arguments of ffmpeg for a video
      \param size the size of the frames (even numbers)
      \param fps the frames a second
    */
    static
    QStringList arguments( const QSize & size,
                           const double fps,
                           const QString & file,
                           const QString & encoder,
                           const int crf,
                           const QString & preset,
                           const bool animation );

    //! the pixel format of QImage::Format_RGB32 in the bytes of this machine
    static
    QString pixelFormat();

    /*!
      \brief the ffmpeg to run: the path given, or "ffmpeg" in the PATH
      \return the path, or an empty text if there is none
    */
    static
    QString findProgram( const QString & path );

    /*!
      \brief run an ffmpeg with -version and -encoders (a few seconds at most)
      \param path the path given (empty: in the PATH)
    */
    static
    Info probe( const QString & path );

    const QString & file() const { return M_file; }

    bool open( const QSize & pixels,
               const double fps ) override;
    WriteResult write( const QImage & image ) override;
    void finish() override;
    void abort() override;

private:

    void readStandardError();
    void handleBytesWritten();
    void handleFinished( int exit_code );
    void handleError();

    //! the reason, with the end of the standard error of ffmpeg
    QString withDetail( const QString & reason ) const;

    void end( const bool ok,
              const QString & message );
};

#endif
