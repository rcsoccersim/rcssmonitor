// -*-c++-*-

/*!
  \file play_mode_position.cpp
  \brief the ball position for Change Playmode. Source File.
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

#include "play_mode_position.h"

#include "options.h"

#include <algorithm>
#include <cmath>

/*-------------------------------------------------------------------*/
/*!

 */
bool
playModeBallPosition( const rcss::rcg::PlayMode mode,
                      double * x_ptr,
                      double * y_ptr )
{
    double x = *x_ptr;
    double y = *y_ptr;

    x = std::min( x, + Options::PITCH_LENGTH*0.5 );
    x = std::max( x, - Options::PITCH_LENGTH*0.5 );
    y = std::min( y, + Options::PITCH_WIDTH*0.5 );
    y = std::max( y, - Options::PITCH_WIDTH*0.5 );

    switch ( mode ) {
    case rcss::rcg::PM_BeforeKickOff:
        x = 0.0;
        y = 0.0;
        break;
    case rcss::rcg::PM_PlayOn:
        break;
    case rcss::rcg::PM_KickIn_Left:
    case rcss::rcg::PM_KickIn_Right:
        y = ( y > 0.0
              ? + Options::PITCH_WIDTH*0.5
              : - Options::PITCH_WIDTH*0.5 );
        break;
    case rcss::rcg::PM_CornerKick_Left:
        x = + Options::PITCH_LENGTH*0.5 - Options::CORNER_ARC_R;
        y = ( y > 0.0
              ? ( + Options::PITCH_WIDTH*0.5 - Options::CORNER_ARC_R )
              : ( - Options::PITCH_WIDTH*0.5 + Options::CORNER_ARC_R ) );
        break;
    case rcss::rcg::PM_CornerKick_Right:
        x = - Options::PITCH_LENGTH*0.5 + Options::CORNER_ARC_R;
        y = ( y > 0.0
              ? ( + Options::PITCH_WIDTH*0.5 - Options::CORNER_ARC_R )
              : ( - Options::PITCH_WIDTH*0.5 + Options::CORNER_ARC_R ) );
        break;
    case rcss::rcg::PM_GoalKick_Left:
        x = - Options::PITCH_LENGTH*0.5 + Options::GOAL_AREA_LENGTH;
        y = ( y > 0.0
              ? + Options::GOAL_AREA_WIDTH*0.5
              : - Options::GOAL_AREA_WIDTH*0.5 );
        break;
    case rcss::rcg::PM_GoalKick_Right:
        x = + Options::PITCH_LENGTH*0.5 - Options::GOAL_AREA_LENGTH;
        y = ( y > 0.0
              ? + Options::GOAL_AREA_WIDTH*0.5
              : - Options::GOAL_AREA_WIDTH*0.5 );
        break;
    case rcss::rcg::PM_Foul_Charge_Left:
    case rcss::rcg::PM_Foul_Charge_Right:
    case rcss::rcg::PM_Foul_Push_Left:
    case rcss::rcg::PM_Foul_Push_Right:
        break;
    case rcss::rcg::PM_Back_Pass_Left:
        // into the left penalty area
        if ( x > ( - Options::PITCH_LENGTH*0.5 + Options::PENALTY_AREA_LENGTH ) )
        {
            x = - Options::PITCH_LENGTH*0.5 + Options::PENALTY_AREA_LENGTH;
        }

        if ( std::fabs( y ) > Options::PENALTY_AREA_WIDTH*0.5 )
        {
            y = ( y > 0.0
                  ? + Options::PENALTY_AREA_WIDTH*0.5
                  : - Options::PENALTY_AREA_WIDTH*0.5 );
        }
        break;
    case rcss::rcg::PM_Back_Pass_Right:
        // into the right penalty area
        if ( x < ( + Options::PITCH_LENGTH*0.5 - Options::PENALTY_AREA_LENGTH ) )
        {
            x = + Options::PITCH_LENGTH*0.5 - Options::PENALTY_AREA_LENGTH;
        }

        if ( std::fabs( y ) > Options::PENALTY_AREA_WIDTH*0.5 )
        {
            y = ( y > 0.0
                  ? + Options::PENALTY_AREA_WIDTH*0.5
                  : - Options::PENALTY_AREA_WIDTH*0.5 );
        }
        break;
    case rcss::rcg::PM_IndFreeKick_Left:
        // out of the right goal area
        if ( x >= ( + Options::PITCH_LENGTH*0.5 - Options::GOAL_AREA_LENGTH )
             && std::fabs( y ) <= Options::GOAL_AREA_WIDTH*0.5 )
        {
            x = + Options::PITCH_LENGTH*0.5 - Options::GOAL_AREA_LENGTH;
        }
        break;
    case rcss::rcg::PM_IndFreeKick_Right:
        // out of the left goal area
        if ( x <= ( - Options::PITCH_LENGTH*0.5 + Options::GOAL_AREA_LENGTH )
             && std::fabs( y ) <= Options::GOAL_AREA_WIDTH*0.5 )
        {
            x = - Options::PITCH_LENGTH*0.5 + Options::GOAL_AREA_LENGTH;
        }
        break;
    case rcss::rcg::PM_Illegal_Defense_Left:
        x = -41.5;
        y = 0.0;
        break;
    case rcss::rcg::PM_Illegal_Defense_Right:
        x = +41.5;
        y = 0.0;
        break;
    default:
        return false;
    }

    *x_ptr = x;
    *y_ptr = y;
    return true;
}
