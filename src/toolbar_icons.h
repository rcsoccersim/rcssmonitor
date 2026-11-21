// -*-c++-*-

/*!
  \file toolbar_icons.h
  \brief the toolbar icons drawn in code, as the web version's. Header File.
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

#ifndef RCSSMONITOR_TOOLBAR_ICONS_H
#define RCSSMONITOR_TOOLBAR_ICONS_H

#include <QColor>
#include <QIcon>
#include <QImage>
#include <QPalette>

/*!
  \enum DrawnIcon
  \brief the toolbar icons drawn in code (drawnIcon()), with the shapes of
  the web version's icons (rcssmonitor-web src/ui/icons.ts), for the
  buttons that have no XPM
*/
enum class DrawnIcon {
    Open, //!< a folder
    Connect, //!< a plug in its socket
    Disconnect, //!< the plug pulled out
    KickOff, //!< a ball
    Loop, //!< two arrows round
    LiveDelay, //!< a clock (the delayed live view; not in the web version)
};

/*!
  \brief an icon drawn on a square of size pixels, from the web version's
  16 x 16 shapes, in a color on transparent
*/
QImage drawnIconImage( const DrawnIcon icon,
                       const QColor & color,
                       const int size );

/*!
  \brief an icon in the text color of the palette (as toolIcon()), sharp
  on a scaled screen too (drawn at 16, 24, 32 and 48 pixels)
*/
QIcon drawnIcon( const DrawnIcon icon,
                 const QPalette & palette );

#endif
