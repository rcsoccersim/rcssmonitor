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
#include <QStringList>

#include <cstddef>
#include <vector>

class FieldCanvas;
class FrameSink;

/*!
  \struct ExportPart
  \brief a part of an export: a sink and the frames of its images (e.g. a
  video of a range)
*/
struct ExportPart {
    FrameSink * sink;
    std::vector< std::size_t > frames;
};

/*!
  \class ExportJob
  \brief draws the frames and gives them to the sinks, an image at a time in
  the event loop (docs/plan/2026-11-22-03_frame-export.md 4.5): the window
  stays responsive, and the export can be aborted. The 2D view draws the
  frames (FieldCanvas::renderFrame(): the view is not changed).

  The parts are exported one after another: a part's sink is opened, given
  its images, finished, and then the next one is opened. When aborted, the
  sink of the part being exported is aborted, and those finished are kept.
*/
class ExportJob
    : public QObject {

    Q_OBJECT

private:

    FieldCanvas & M_canvas;
    std::vector< ExportPart > M_parts; //!< their sinks are children of this
    QSize M_pixels;
    qreal M_dpr;
    double M_fps;

    std::size_t M_part; //!< the part being exported
    std::size_t M_done_in_part; //!< the images its sink has taken
    std::size_t M_done; //!< the images all the sinks have taken
    std::size_t M_total;
    QStringList M_messages; //!< of the parts finished
    QImage M_pending; //!< drawn, not taken yet (BUSY)
    bool M_started;
    bool M_ended; //!< finished() told

public:

    /*!
      \param canvas the 2D view that draws the frames
      \param parts the sinks (made children of this) and their frames (ExportTiming::frames2D())
      \param pixels the size of the images
      \param dpr the device pixel ratio of the images (FieldCanvas::renderFrame())
      \param fps the images a second (for a video)
    */
    ExportJob( FieldCanvas & canvas,
               const std::vector< ExportPart > & parts,
               const QSize & pixels,
               const qreal dpr,
               const double fps,
               QObject * parent = nullptr );

    //! one sink of all the frames
    ExportJob( FieldCanvas & canvas,
               FrameSink * sink,
               const std::vector< std::size_t > & frames,
               const QSize & pixels,
               const qreal dpr,
               const double fps,
               QObject * parent = nullptr );

    ~ExportJob() override;

    std::size_t done() const { return M_done; }
    std::size_t total() const { return M_total; }
    bool isRunning() const { return M_started && ! M_ended; }

    /*!
      \brief open the first sink and start: the images are drawn after this
      returns, and finished() tells the end (an error of a sink too)
    */
    void start();

    //! stop: finished( false, ... ) at once
    void abort();

signals:

    //! the images all the sinks have taken; done == total: the sinks finishing
    void progressed( std::size_t done,
                     std::size_t total );

    //! the end: ok and the messages of the sinks (one a line), or why not
    void finished( bool ok,
                   const QString & message );

private:

    //! open the sink of the part, and go on
    void openPart();

    //! draw the next image and give it to the sink of the part
    void step();

    void partFinished( FrameSink * sink,
                       const bool ok,
                       const QString & message );

    void end( const bool ok,
              const QString & message );
};

#endif
