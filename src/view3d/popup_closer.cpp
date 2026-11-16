// -*-c++-*-

/*!
  \file popup_closer.cpp
  \brief closes the popups (menus) when the 3D view is clicked. Source File.
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

#include "popup_closer.h"

#include <QApplication>
#include <QEvent>
#include <QWidget>

/*-------------------------------------------------------------------*/
PopupCloser::PopupCloser( QObject * parent )
    : QObject( parent )
{

}

/*-------------------------------------------------------------------*/
bool
PopupCloser::eventFilter( QObject *,
                          QEvent * event )
{
    switch ( event->type() ) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
        if ( QApplication::activePopupWidget() )
        {
            // all of them (a menu and its submenus), the newest first; at
            // most as many as there can be, if one would not close
            for ( int i = 0; i < 16; ++i )
            {
                QWidget * popup = QApplication::activePopupWidget();
                if ( ! popup ) break;
                popup->close();
            }
            return true;
        }
        break;
    case QEvent::Wheel:
        return ( QApplication::activePopupWidget() != nullptr );
    default:
        break;
    }
    return false;
}
