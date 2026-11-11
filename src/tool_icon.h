// -*-c++-*-

/*!
  \file tool_icon.h
  \brief toolbar icons in the colors of the palette. Header File.
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

#ifndef RCSSMONITOR_TOOL_ICON_H
#define RCSSMONITOR_TOOL_ICON_H

#include <QColor>
#include <QIcon>
#include <QImage>
#include <QPalette>

/*!
  \brief draw a monochrome image in a color: a black pixel becomes the
  color, a white one transparent, and a gray one the color partly
  transparent. A transparent pixel stays transparent.
  \param image the image, black on white or on transparent
  \param color the color
  \return the image in the color, on transparent
*/
QImage tintIcon( const QImage & image,
                 const QColor & color );

/*!
  \brief make a toolbar icon from an XPM in black, drawn in the text color
  of the palette, so that it is seen with a dark palette too.
  \param xpm the XPM data
  \param palette the palette
  \return the icon
*/
QIcon toolIcon( const char * const xpm[],
                const QPalette & palette );

#endif
