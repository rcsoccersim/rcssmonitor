// -*-c++-*-

/*!
  \file popup_closer.h
  \brief closes the popups (menus) when the 3D view is clicked. Header File.
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

#ifndef RCSSMONITOR_POPUP_CLOSER_H
#define RCSSMONITOR_POPUP_CLOSER_H

#include <QObject>

/*!
  \class PopupCloser
  \brief an event filter on the 3D view's window (FieldView3D) that closes
  the popups open (the referee's menu, a menu of the menu bar) when the
  view is clicked, the click going no further, and keeps the wheel from
  the view while one is open.

  Qt closes a popup when a widget of the application is clicked, but not
  when a window of its own is (the 3D view is a QOpenGLWindow embedded in
  a container widget). Through X11 (XWayland too), a popup grabs the
  pointer, so the view gets no click while one is open. On Wayland it
  does: the compositor gives the clicks on the application's own surfaces
  to them, and the popup stayed open (Qt 6.4).
*/
class PopupCloser
    : public QObject {
public:
    explicit PopupCloser( QObject * parent = nullptr );

    bool eventFilter( QObject * watched,
                      QEvent * event ) override;
};

#endif
