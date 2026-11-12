// -*-c++-*-

/*!
  \file player_selection.cpp
  \brief the automatic selection of a player, in 2D and 3D. Source File.
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

#include "player_selection.h"

#include "options.h"

#include <rcss/rcg/types.h>

#include <cmath>

/*-------------------------------------------------------------------*/
bool
selectPlayerNearestBall( const rcss::rcg::ShowInfoT & show,
                         Options & opt )
{
    if ( ! opt.playerAutoSelect() )
    {
        return false;
    }

    const Options::PlayerSelectType type = opt.playerSelectType();

    float min_dist2 = 40000.0f;

    rcss::rcg::Side side = rcss::rcg::NEUTRAL;
    int unum = 0;

    // the left team, the right team, or both
    const int first = ( type == Options::SELECT_AUTO_RIGHT
                        ? 11
                        : 0 );
    const int last = ( type == Options::SELECT_AUTO_LEFT
                       ? 11
                       : 22 );
    for ( int i = first; i < last; ++i )
    {
        if ( show.player_[i].state_ != 0 )
        {
            float d2
                = std::pow( show.ball_.x_ - show.player_[i].x_, 2 )
                + std::pow( show.ball_.y_ - show.player_[i].y_, 2 );
            if ( d2 < min_dist2 )
            {
                min_dist2 = d2;
                side = show.player_[i].side();
                unum = show.player_[i].unum_;
            }
        }
    }

    if ( unum == 0 )
    {
        return false;
    }

    opt.setSelectedNumber( side, unum );
    return true;
}
