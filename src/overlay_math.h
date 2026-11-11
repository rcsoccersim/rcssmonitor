// -*-c++-*-

/*!
  \file overlay_math.h
  \brief calculations of the overlays, without drawing. Header File.
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

#ifndef RCSSMONITOR_OVERLAY_MATH_H
#define RCSSMONITOR_OVERLAY_MATH_H

#include <rcss/rcg/types.h>

/*!
  \namespace OverlayMath
  \brief calculations of the overlays (the offside lines, ...), kept apart
  from the painters so that they can be tested and shared by the views.
*/
namespace OverlayMath {

/*!
  \struct OffsideLines
  \brief the x coordinates of the two offside lines
*/
struct OffsideLines {
    float left_; //!< the line of the left team, in x <= 0
    float right_; //!< the line of the right team, in x >= 0
};

/*!
  \brief get the offside lines.
  \param show the show data
  \return the lines

  The line of the left team is at the second last left player in x < 0,
  or at the ball when the ball is nearer to the left goal line, and not
  beyond the left goal line. That of the right team is the same on the
  other side.
*/
OffsideLines offsideLines( const rcss::rcg::ShowInfoT & show );

}

#endif
