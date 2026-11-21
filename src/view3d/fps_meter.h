// -*-c++-*-

/*!
  \file fps_meter.h
  \brief the frames per second of the 3D view, shown over it. Header File.
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

#ifndef RCSSMONITOR_FPS_METER_H
#define RCSSMONITOR_FPS_METER_H

#include <QString>

#include <deque>

/*!
  \class FpsMeter
  \brief the frames per second of the 3D view, and the GPU's time of a
  frame, for the option fps_3d (shown over the view).

  The frames are counted over the last WINDOW seconds. The 3D view draws
  only while something moves (FieldView3D::drawNext()), and Qt 6 shows at
  most the screen's rate (60 fps): so the rate alone tells neither how much
  room there is nor anything while still. The GPU's time of a frame (by the
  timer queries, a few frames late) tells the room; the rate is "-" while
  the view is still (fewer than two frames in the window).
*/
class FpsMeter {
public:

    //! the frames are counted over this many seconds
    static constexpr double WINDOW = 1.0;

    /*!
      \brief a frame was drawn
      \param now the time (s, of a steady clock)
    */
    void frame( const double now );

    /*!
      \brief the GPU's times of frames came (GpuTimer::take())
      \param total the sum of them (ms)
      \param count how many
    */
    void gpu( const double total,
              const int count );

    /*!
      \brief the frames per second, at a time
      \param now the time (s)
      \return negative while still: fewer than two frames in the window
    */
    double fps( const double now ) const;

    //! the GPU's time of a frame (ms), of the last times that came; negative before any
    double gpuTime() const { return M_gpu; }

    /*!
      \brief "60 fps · GPU 5.6 ms"; "- fps · GPU 5.6 ms" while still (also
      when told so: the last frame before the view stops drawing, which
      stays on the screen); without the GPU's time before any came
      \param now the time (s)
      \param still the view draws no more frames after this one
    */
    QString text( const double now,
                  const bool still = false ) const;

    //! start again
    void clear();

private:

    std::deque< double > M_frames; //!< the times of the frames in the window
    double M_gpu = -1.0;
};

#endif
