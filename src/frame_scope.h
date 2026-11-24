// -*-c++-*-

/*!
  \file frame_scope.h
  \brief the frame shown and the state of the view, for drawing another
  frame for a while. Header File.
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

#ifndef RCSSMONITOR_FRAME_SCOPE_H
#define RCSSMONITOR_FRAME_SCOPE_H

#include "options.h"

#include <cstddef>

class DispHolder;

/*!
  \class FrameScope
  \brief a guard around drawing a frame other than the one shown, e.g. into
  an image (docs/plan/2026-11-22-03_frame-export.md 4.1, S2): it keeps the
  index of DispHolder and Options::viewState(), makes the index the one
  given, and sets both back when it is destroyed. Used by the 2D view and
  the 3D view.
*/
class FrameScope {
private:

    DispHolder & M_disp_holder;
    const std::size_t M_saved_index;
    const Options::ViewState M_saved_view_state;

    // not used
    FrameScope( const FrameScope & ) = delete;
    FrameScope & operator=( const FrameScope & ) = delete;

public:

    /*!
      \param disp_holder the frames
      \param index the frame to draw (an index out of the frames is ignored:
      the frame shown is drawn)
    */
    FrameScope( DispHolder & disp_holder,
                const std::size_t index );
    ~FrameScope();
};

#endif
