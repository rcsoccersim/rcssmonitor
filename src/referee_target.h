// -*-c++-*-

/*!
  \file referee_target.h
  \brief what a right click on the field aims at. Header File.
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


#ifndef RCSSMONITOR_REFEREE_TARGET_H
#define RCSSMONITOR_REFEREE_TARGET_H

#include <rcss/rcg/types.h>

/*!
  \struct RefereeTarget
  \brief what a right click on the field aims at, for the referee's menu
  (RefereeMenu): a point on the field, and in the 3D view a player.

  In the 2D view, the point under the mouse. In the 3D view, a player whose
  body or head is under the mouse, at its feet; else the point where the
  camera's ray meets the ground, on the pitch or its margin; or nothing
  (above the horizon, or the ground beyond the margin, too near the horizon
  to aim at).
*/
struct RefereeTarget {
    bool has_point_ = false; //!< x_ and y_ are set
    double x_ = 0.0; //!< the point on the field (m)
    double y_ = 0.0;
    rcss::rcg::Side side_ = rcss::rcg::NEUTRAL; //!< the player aimed at, NEUTRAL if none
    int unum_ = 0; //!< its uniform number, if side_ is not NEUTRAL

    //! a point without a player
    static RefereeTarget point( const double x,
                                const double y )
      {
          RefereeTarget t;
          t.has_point_ = true;
          t.x_ = x;
          t.y_ = y;
          return t;
      }

    //! a player aimed at, with the point at its feet
    static RefereeTarget player( const rcss::rcg::Side side,
                                 const int unum,
                                 const double x,
                                 const double y )
      {
          RefereeTarget t = point( x, y );
          t.side_ = side;
          t.unum_ = unum;
          return t;
      }

    //! a player is aimed at
    bool hasPlayer() const { return side_ != rcss::rcg::NEUTRAL; }
};

#endif
