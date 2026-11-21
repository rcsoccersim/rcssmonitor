// -*-c++-*-

/*!
  \file gpu_timer.h
  \brief the time the GPU takes to draw a frame of the 3D view. Header File.
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

#ifndef RCSSMONITOR_GL_GPU_TIMER_H
#define RCSSMONITOR_GL_GPU_TIMER_H

#include <QOpenGLFunctions_3_3_Core>

/*!
  \class GpuTimer
  \brief how long the GPU takes to draw (docs/plan/2026-11-11_3d-view.md §10.3), by
  OpenGL's timer queries: begin() and end() around the drawing ask the GPU
  to note the time it spends between them. The CPU only asks for the
  drawing, which the GPU does later, so the time comes later too: it is
  read a few frames after (a ring of queries), without waiting for the GPU.

  Every function needs the OpenGL context current.
*/
class GpuTimer {
public:
    //! frames a query has to be done before it is read again
    static constexpr int RING = 4;

    GpuTimer() = default;
    GpuTimer( const GpuTimer & ) = delete;
    GpuTimer & operator=( const GpuTimer & ) = delete;

    //! make the queries
    void create( QOpenGLFunctions_3_3_Core & gl );
    //! delete them
    void destroy( QOpenGLFunctions_3_3_Core & gl );
    bool created() const { return M_queries[0] != 0; }

    /*!
      \brief start timing a frame's drawing (after reading the oldest
      query, if the GPU is done with it)
    */
    void begin( QOpenGLFunctions_3_3_Core & gl );
    //! stop timing it
    void end( QOpenGLFunctions_3_3_Core & gl );

    /*!
      \brief take the times read since the last call
      \param total the sum of them (ms)
      \return how many
    */
    int take( double * total );

private:
    GLuint M_queries[RING] = { 0, 0, 0, 0 };
    bool M_pending[RING] = { false, false, false, false }; //!< asked, not read yet
    int M_next = 0; //!< the query of the next frame
    bool M_running = false; //!< between begin() and end()
    double M_total = 0.0; //!< ms read, not taken yet
    int M_count = 0;
};

#endif
