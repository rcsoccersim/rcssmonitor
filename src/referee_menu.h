// -*-c++-*-

/*!
  \file referee_menu.h
  \brief the menu of a right click on the field. Header File.
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


#ifndef RCSSMONITOR_REFEREE_MENU_H
#define RCSSMONITOR_REFEREE_MENU_H

#include "referee_target.h"

#include <QObject>
#include <QPoint>

class QAction;
class QMenu;
class QWidget;

/*!
  \class RefereeMenu
  \brief the menu of a right click on the field, which the 2D view and the
  3D view share (docs/PLAN_3D-VIEW.md §5.4): Drop Ball, Free Kick Left and
  Right at the point aimed at, the cards, and Change Playmode.

  A view tells where the click aims at (RefereeTarget), and exec() shows
  the menu for it. The items emit the signals, which MainWindow sends to
  the server. Without a point (3D: above the horizon), Drop Ball and the
  free kicks are disabled, with the reason in their tool tips; Change
  Playmode still works, from the ball, as in the Referee menu. The cards
  open the dialog with the player aimed at chosen (3D).
*/
class RefereeMenu
    : public QObject {

    Q_OBJECT

public:

    //! the reason of the disabled items without a point
    static const char * const NO_POINT;

    //! \param parent the widget that owns the menu
    explicit
    RefereeMenu( QWidget * parent );

    //! the menu
    QMenu * menu() const { return M_menu; }

    //! what the items act on
    const RefereeTarget & target() const { return M_target; }

    //! set what the items act on, and enable the items that need a point if it has one
    void setTarget( const RefereeTarget & target );

    //! show the menu at a point of the screen, for a target (setTarget())
    void exec( const RefereeTarget & target,
               const QPoint & global_pos );

signals:

    void dropBall( double x, double y );
    void freeKickLeft( double x, double y );
    void freeKickRight( double x, double y );
    //! a card for the target's player (none: the dialog chooses no one first)
    void yellowCard( const RefereeTarget & target );
    void redCard( const RefereeTarget & target );
    //! Change Playmode, at the target's point (none: from the ball)
    void playModeChanged( int mode,
                          const RefereeTarget & target );

private:

    QMenu * M_menu;
    QAction * M_drop_ball_act;
    QAction * M_free_kick_left_act;
    QAction * M_free_kick_right_act;
    RefereeTarget M_target;

    //! an item that needs a point, emitting the signal with it
    QAction * addPointAction( const QString & text,
                              void ( RefereeMenu::*signal )( double, double ) );
};

#endif
