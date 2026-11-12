// -*-c++-*-

/*!
  \file player_style.cpp
  \brief how a player is colored by its state, in 2D and 3D. Source File.
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

#include "player_style.h"

#include "options.h"

#include <rcss/rcg/types.h>

#include <algorithm>
#include <cmath>

/*-------------------------------------------------------------------*/
PlayerStyle
playerStyle( const rcss::rcg::PlayerT & player,
             const rcss::rcg::PlayerTypeT & type,
             const rcss::rcg::ServerParamT & param,
             const Options & opt )
{
    PlayerStyle style;

    style.pen = ( opt.selectedPlayer( player.side(), player.unum_ )
                  ? opt.selectedPlayerPen()
                  : opt.playerPen() );

    switch ( player.side() ) {
    case rcss::rcg::LEFT:
        style.brush = ( player.isGoalie() ? opt.leftGoalieBrush() : opt.leftTeamBrush() );
        break;
    case rcss::rcg::RIGHT:
        style.brush = ( player.isGoalie() ? opt.rightGoalieBrush() : opt.rightTeamBrush() );
        break;
    default:
        style.brush = QBrush( Qt::black );
        break;
    }

    // the colors of the states
    if ( ! player.isAlive() )
    {
        style.brush = QBrush( Qt::black );
    }
    if ( player.isIllegalDefenseState()
         && opt.showIllegalDefense() )
    {
        style.pen = opt.illegalDefensePen();
    }
    if ( player.isKicking() )
    {
        style.pen = opt.kickPen();
    }
    if ( player.isKickingFault() )
    {
        style.brush = opt.kickFaultBrush();
    }
    if ( player.isCatching() )
    {
        style.brush = opt.catchBrush();
    }
    if ( player.isCatchingFault() )
    {
        style.brush = opt.catchFaultBrush();
    }
    if ( player.isTackling() )
    {
        style.pen = opt.tacklePen();
        style.brush = opt.tackleBrush();
    }
    if ( player.isTacklingFault() )
    {
        style.pen = opt.tacklePen();
        style.brush = opt.tackleFaultBrush();
    }
    if ( player.isFoulCharged() )
    {
        style.brush = opt.foulChargedBrush();
    }
    if ( player.isCollidedBall() )
    {
        style.brush = opt.ballCollideBrush();
    }
    if ( player.isCollidedPlayer() )
    {
        style.brush = opt.playerCollideBrush();
    }

    // darker by steps of 1/8 of the stamina, and the ring of the decay
    style.body = style.brush;
    style.decay = QPen( Qt::NoPen );
    if ( player.hasStamina() )
    {
        const double stamina_rate = player.stamina_ / param.stamina_max_;
        int dark_rate = 200 - static_cast< int >( std::rint( 200 * std::rint( stamina_rate / 0.125 ) * 0.125 ) );
        dark_rate = std::max( 0, dark_rate - 50 );
        if ( dark_rate != 0 )
        {
            style.body = QBrush( style.brush.color().darker( 100 + dark_rate ) );
        }

        if ( ! player.hasFullEffort( type.effort_max_ ) )
        {
            style.decay = opt.effortDecayedPen();
        }
        else if ( ! player.hasFullRecovery() )
        {
            style.decay = opt.recoveryDecayedPen();
        }
    }

    return style;
}
