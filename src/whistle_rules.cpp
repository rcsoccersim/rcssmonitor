// -*-c++-*-

/*!
  \file whistle_rules.cpp
  \brief the referee's whistle at the changes of the play mode. Source File.
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

#include "whistle_rules.h"

using namespace rcss::rcg;

/*-------------------------------------------------------------------*/
bool
isWhistleStopMode( const PlayMode mode )
{
    switch ( mode ) {
    case PM_Foul_Charge_Left:
    case PM_Foul_Charge_Right:
    case PM_Foul_Push_Left:
    case PM_Foul_Push_Right:
    case PM_Foul_MultipleAttacker_Left:
    case PM_Foul_MultipleAttacker_Right:
    case PM_Foul_BallOut_Left:
    case PM_Foul_BallOut_Right:
    case PM_OffSide_Left:
    case PM_OffSide_Right:
    case PM_Back_Pass_Left:
    case PM_Back_Pass_Right:
    case PM_Free_Kick_Fault_Left:
    case PM_Free_Kick_Fault_Right:
    case PM_CatchFault_Left:
    case PM_CatchFault_Right:
    case PM_Illegal_Defense_Left:
    case PM_Illegal_Defense_Right:
        return true;
    default:
        break;
    }
    return false;
}

/*-------------------------------------------------------------------*/
WhistleKind
whistleFor( const PlayMode prev,
            const PlayMode next,
            const int next_time )
{
    if ( prev == next
         || prev == PM_Null )
    {
        return WhistleKind::None;
    }

    switch ( next ) {
    case PM_KickOff_Left:
    case PM_KickOff_Right:
        if ( prev == PM_AfterGoal_Left
             || prev == PM_AfterGoal_Right
             || prev == PM_BeforeKickOff
             || prev == PM_FirstHalfOver
             || prev == PM_Pause
             || prev == PM_Human )
        {
            return WhistleKind::Short;
        }
        // a new half from the play: the logs have no before_kick_off at
        // the end of a half
        return WhistleKind::LongTwice;
    case PM_BeforeKickOff:
        return next_time > 0 ? WhistleKind::LongTwice : WhistleKind::None;
    case PM_FirstHalfOver:
        return WhistleKind::LongTwice;
    case PM_TimeOver:
        return WhistleKind::LongThrice;
    case PM_AfterGoal_Left:
    case PM_AfterGoal_Right:
        return WhistleKind::Long;
    case PM_FreeKick_Left:
    case PM_FreeKick_Right:
    case PM_IndFreeKick_Left:
    case PM_IndFreeKick_Right:
        // the restart after a foul (not a free kick after a catch)
        return isWhistleStopMode( prev ) ? WhistleKind::Short : WhistleKind::None;
    case PM_PenaltyReady_Left:
    case PM_PenaltyReady_Right:
        return WhistleKind::Short;
    default:
        break;
    }

    if ( isWhistleStopMode( next )
         && ! isWhistleStopMode( prev ) )
    {
        return WhistleKind::Short;
    }

    return WhistleKind::None;
}

/*-------------------------------------------------------------------*/
WhistleKind
strongerWhistle( const WhistleKind a,
                 const WhistleKind b )
{
    return static_cast< int >( a ) >= static_cast< int >( b ) ? a : b;
}
