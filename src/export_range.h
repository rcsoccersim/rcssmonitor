// -*-c++-*-

/*!
  \file export_range.h
  \brief the positions and the ranges of the frames to export. Header File.
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

#ifndef RCSSMONITOR_EXPORT_RANGE_H
#define RCSSMONITOR_EXPORT_RANGE_H

#include "disp_holder.h"

#include <QString>

#include <cstddef>

/*!
  \struct ExportPosition
  \brief a position in a game log as the user writes it: a cycle (1234), or
  a cycle and a frame stopped in it (1234.3, as the status bar shows it)
*/
struct ExportPosition {
    int cycle = -1;
    int stopped = -1; //!< the stopped frame (stime), or -1 for the cycle as a whole

    bool isValid() const { return cycle >= 0; }
};

/*!
  \struct ExportRange
  \brief a range of frames to export, and the functions of the positions
  (docs/plan/2026-11-22-03_frame-export.md 4.4, S9): pure functions on the
  frames (DispCont, in the order of the cycles and the stopped frames).

  A start at a cycle is its first frame, and an end at a cycle is its last
  frame, the frames stopped in it too; a position with a stopped frame is
  that frame. Both ends are included. A position not in the log (out of it,
  or missing in a live game) is moved to the next frame for a start, and to
  the previous frame for an end ("snapped").
*/
struct ExportRange {
    std::size_t first = DispHolder::INVALID_INDEX;
    std::size_t last = DispHolder::INVALID_INDEX;

    bool isValid() const
      {
          return first != DispHolder::INVALID_INDEX
              && last != DispHolder::INVALID_INDEX
              && first <= last;
      }

    //! the number of the frames
    std::size_t size() const { return isValid() ? last - first + 1 : 0; }

    /*!
      \brief read a position: "1234" or "1234.3" (spaces around are ignored)
      \param error set to the reason if it is not one (may be null)
      \return false if it is not one
    */
    static
    bool parsePosition( const QString & text,
                        ExportPosition * pos,
                        QString * error );

    //! a position as written: "1234", or "1234.3" for a stopped frame
    static
    QString positionText( const rcss::rcg::ShowInfoT & show );

    //! the position of a frame, or an empty text if it is not one
    static
    QString positionText( const DispCont & frames,
                          const std::size_t index );

    /*!
      \brief the first frame of a range starting at a position
      \param snapped set to true if there is no frame at the position and
      the next one is taken (may be null)
      \return the index, or DispHolder::INVALID_INDEX if it is after the last frame
    */
    static
    std::size_t startIndex( const DispCont & frames,
                            const ExportPosition & pos,
                            bool * snapped );

    /*!
      \brief the last frame of a range ending at a position: the last
      frame of the cycle, the frames stopped in it too
      \param snapped set to true if there is no frame at the position and
      the previous one is taken (may be null)
      \return the index, or DispHolder::INVALID_INDEX if it is before the first frame
    */
    static
    std::size_t endIndex( const DispCont & frames,
                          const ExportPosition & pos,
                          bool * snapped );

    /*!
      \brief the time of the game at a cycle: "m:ss"
      \param step_ms the simulation step (ms)
    */
    static
    QString gameTime( const int cycle,
                      const int step_ms );
};

#endif
