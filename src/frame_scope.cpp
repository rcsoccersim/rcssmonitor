// -*-c++-*-

/*!
  \file frame_scope.cpp
  \brief the frame shown and the state of the view, for drawing another
  frame for a while. Source File.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "frame_scope.h"

#include "disp_holder.h"

/*-------------------------------------------------------------------*/
FrameScope::FrameScope( DispHolder & disp_holder,
                        const std::size_t index )
    : M_disp_holder( disp_holder ),
      M_saved_index( disp_holder.currentIndex() ),
      M_saved_view_state( Options::instance().viewState() )
{
    M_disp_holder.setIndex( index );
}

/*-------------------------------------------------------------------*/
FrameScope::~FrameScope()
{
    M_disp_holder.restoreIndex( M_saved_index );
    Options::instance().setViewState( M_saved_view_state );
}
