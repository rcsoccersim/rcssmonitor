// -*-c++-*-

/*!
  \file referee_menu.cpp
  \brief the menu of a right click on the field. Source File.
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

#include "referee_menu.h"

#include "play_mode_position.h"

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QPixmap>

const char * const RefereeMenu::NO_POINT = "Right-click on the pitch or its margin to choose where.";

namespace {

//! a square of a color, the icon of a card
QIcon
card_icon( const Qt::GlobalColor color )
{
    QPixmap icon( 16, 16 );
    icon.fill( color );
    return QIcon( icon );
}

}

/*-------------------------------------------------------------------*/
RefereeMenu::RefereeMenu( QWidget * parent )
    : QObject( parent ),
      M_menu( new QMenu( parent ) )
{
    // the tool tips tell why an item is disabled
    M_menu->setToolTipsVisible( true );

    M_drop_ball_act = addPointAction( tr( "Drop Ball" ), &RefereeMenu::dropBall );
    M_free_kick_left_act = addPointAction( tr( "Free Kick Left" ), &RefereeMenu::freeKickLeft );
    M_free_kick_right_act = addPointAction( tr( "Free Kick Right" ), &RefereeMenu::freeKickRight );

    M_menu->addSeparator();

    QAction * yellow = M_menu->addAction( card_icon( Qt::yellow ), tr( "Yellow Card" ) );
    yellow->setStatusTip( tr( "Call yellow card." ) );
    connect( yellow, &QAction::triggered,
             this, [this]() { emit yellowCard( M_target ); } );
    QAction * red = M_menu->addAction( card_icon( Qt::red ), tr( "Red Card" ) );
    red->setStatusTip( tr( "Call red card." ) );
    connect( red, &QAction::triggered,
             this, [this]() { emit redCard( M_target ); } );

    M_menu->addSeparator();

    QMenu * submenu = M_menu->addMenu( tr( "Change Playmode" ) );
    const char * playmode_strings[] = PLAYMODE_STRINGS;
    for ( const rcss::rcg::PlayMode mode : changePlayModeModes() )
    {
        QAction * act = submenu->addAction( QString::fromLatin1( playmode_strings[mode] ) );
        connect( act, &QAction::triggered,
                 this, [this, mode]() { emit playModeChanged( int( mode ), M_target ); } );
    }
}

/*-------------------------------------------------------------------*/
QAction *
RefereeMenu::addPointAction( const QString & text,
                             void ( RefereeMenu::*signal )( double, double ) )
{
    QAction * act = M_menu->addAction( text );
    connect( act, &QAction::triggered,
             this, [this, signal]()
                   {
                       if ( M_target.has_point_ )
                       {
                           emit ( this->*signal )( M_target.x_, M_target.y_ );
                       }
                   } );
    return act;
}

/*-------------------------------------------------------------------*/
void
RefereeMenu::setTarget( const RefereeTarget & target )
{
    M_target = target;

    const QString reason = ( target.has_point_ ? QString() : tr( NO_POINT ) );
    for ( QAction * act : { M_drop_ball_act, M_free_kick_left_act, M_free_kick_right_act } )
    {
        act->setEnabled( target.has_point_ );
        act->setToolTip( reason );
    }
}

/*-------------------------------------------------------------------*/
void
RefereeMenu::exec( const RefereeTarget & target,
                   const QPoint & global_pos )
{
    setTarget( target );
    M_menu->exec( global_pos );
}
