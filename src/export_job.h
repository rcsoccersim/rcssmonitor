// -*-c++-*-

/*!
  \file export_job.h
  \brief the export of frames, an image at a time. Header File.
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

#ifndef RCSSMONITOR_EXPORT_JOB_H
#define RCSSMONITOR_EXPORT_JOB_H

#include <QImage>
#include <QObject>
#include <QSize>

#include <cstddef>
#include <vector>

class FieldCanvas;
class FrameSink;

/*!
  \class ExportJob
  \brief draws the frames and gives them to a sink, an image at a time in
  the event loop (docs/plan/2026-11-22-03_frame-export.md 4.5): the window
  stays responsive, and the export can be aborted. The 2D view draws the
  frames (FieldCanvas::renderFrame(): the view is not changed).
*/
class ExportJob
    : public QObject {

    Q_OBJECT

private:

    FieldCanvas & M_canvas;
    FrameSink * M_sink; //!< a child of this
    std::vector< std::size_t > M_frames; //!< the frame of each image
    QSize M_pixels;
    qreal M_dpr;
    double M_fps;

    std::size_t M_done; //!< the images the sink has taken
    QImage M_pending; //!< drawn, not taken yet (BUSY)
    bool M_started;
    bool M_ended; //!< finished() told

public:

    /*!
      \param canvas the 2D view that draws the frames
      \param sink where the images go: made a child of this
      \param frames the frame of each image (ExportTiming::frames2D())
      \param pixels the size of the images
      \param dpr the device pixel ratio of the images (FieldCanvas::renderFrame())
      \param fps the images a second (for a video)
    */
    ExportJob( FieldCanvas & canvas,
               FrameSink * sink,
               const std::vector< std::size_t > & frames,
               const QSize & pixels,
               const qreal dpr,
               const double fps,
               QObject * parent = nullptr );

    ~ExportJob() override;

    std::size_t done() const { return M_done; }
    std::size_t total() const { return M_frames.size(); }
    bool isRunning() const { return M_started && ! M_ended; }

    /*!
      \brief open the sink and start: the images are drawn after this
      returns, and finished() tells the end (an error of the sink too)
    */
    void start();

    //! stop: finished( false, ... ) at once
    void abort();

signals:

    void progressed( std::size_t done,
                     std::size_t total );

    //! the end: ok, or why not
    void finished( bool ok,
                   const QString & message );

private:

    //! draw the next image and give it to the sink
    void step();

    void end( const bool ok,
              const QString & message );
};

#endif
