// -*-c++-*-

/*!
  \file export_timing.h
  \brief the timing of the exported images and videos. Header File.
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

#ifndef RCSSMONITOR_EXPORT_TIMING_H
#define RCSSMONITOR_EXPORT_TIMING_H

#include <QString>

#include <cstddef>
#include <vector>

/*!
  \class ExportTiming
  \brief the timing model of the export (docs/plan/2026-11-22-03_frame-export.md
  4.3): pure functions. A range of frames [first, last] of the log is
  exported as images (the frames of a video) at fps per second, the game
  played speed times as fast. An image advances framesPerImage() frames
  (2D: 10 fps at 1x speed, one frame an image).
*/
class ExportTiming {
public:

    //! the margin of the rounding of the positions
    static constexpr double EPSILON = 1.0e-6;

    //! the frame rate of the 2D images by default (one frame an image)
    static constexpr double DEFAULT_FPS_2D = 10.0;

    //! the frame rate of the 3D images by default (drawn between the frames)
    static constexpr double DEFAULT_FPS_3D = 30.0;

    /*!
      \brief the frames one image advances: speed × 1000 / ( step × fps )
      \param speed how many times as fast as the game is played (> 0)
      \param step_ms the simulation step (ms; 100 by default)
      \param fps the images a second (> 0)
      \return the frames, or 0 if a value is not positive
    */
    static
    double framesPerImage( const double speed,
                           const int step_ms,
                           const double fps );

    /*!
      \brief the images of a range: floor( ( last - first ) / per_image + ε ) + 1
      \return 0 if last < first or per_image is not positive
    */
    static
    std::size_t imageCount( const std::size_t first,
                            const std::size_t last,
                            const double per_image );

    /*!
      \brief the position of the n-th image (0 first) in the frames:
      first + n × per_image (from n, not added up, not to drift)
    */
    static
    double position( const std::size_t first,
                     const std::size_t n,
                     const double per_image );

    /*!
      \brief the frame drawn by the 2D view at a position: the one at or
      before it, not after the last frame of the range
    */
    static
    std::size_t frameAt( const double position,
                         const std::size_t last );

    /*!
      \brief the frames of the 2D images of a range, one an image
      (frameAt( position( first, n, per_image ), last ))
    */
    static
    std::vector< std::size_t > frames2D( const std::size_t first,
                                         const std::size_t last,
                                         const double per_image );

    /*!
      \brief the positions of the 3D images of a range, between the frames
      (position( first, n, per_image ), not after last; within EPSILON of a
      frame: that frame)
    */
    static
    std::vector< double > positions3D( const std::size_t first,
                                       const std::size_t last,
                                       const double per_image );

    //! the length of a video of the images (s)
    static
    double duration( const std::size_t images,
                     const double fps );

    //! the digits of the numbers in the names of the images: max( 5, the digits of images - 1 )
    static
    int nameDigits( const std::size_t images );

    /*!
      \brief the name of an image: PREFIX + the number in digits + "." + ext
      \param number the number of the image (0 first)
    */
    static
    QString imageName( const QString & prefix,
                       const std::size_t number,
                       const int digits,
                       const QString & ext );
};

#endif
