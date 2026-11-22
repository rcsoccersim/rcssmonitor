// -*-c++-*-

/*!
  \file ui_icon.h
  \brief the icons that follow the color scheme. Header File.
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

#ifndef RCSSMONITOR_UI_ICON_H
#define RCSSMONITOR_UI_ICON_H

#include <QIcon>
#include <QString>

/*!
  \class UIIcon
  \brief the icons of the tool bars, which follow the color scheme

  fromName() creates an icon from the PNG files embedded with the Qt
  resource system (icons/icons.qrc). Each icon has the images for the light
  and the dark color schemes (16, 24, 32 and 48 pixels each), made from an
  SVG file by icons/generate.sh, and the images are chosen by the palette
  of the application whenever the icon is painted (UIPalette::isDark()).

  The same as soccerwindow2's UIIcon (docs/plan/2026-11-22-01_icon-redesign.md).
*/
class UIIcon {
private:
    // not used
    UIIcon() = delete;

public:

    //! the sizes of the images of an icon (pixels, in ascending order)
    static const int SIZES[4];

    /*!
      \brief register the icons embedded with the Qt resource system.
      fromName() calls it. Call it before using the paths of the resources
      (":/icons/...") directly. It can be called more than once.
    */
    static
    void initResources();

    /*!
      \brief create an icon from the embedded PNG files
      :/icons/light/SIZE/NAME.png and :/icons/dark/SIZE/NAME.png.

      The icon is drawn in the largest size of the images that fits in the
      size asked (in device independent pixels), from the image of that size
      times the device pixel ratio (e.g. the 32 pixel image for the 16 pixel
      icon on a display whose device pixel ratio is 2), or else from the
      nearest larger image, scaled. The images of the light or dark color
      scheme are chosen whenever the icon is painted, so that the icon
      follows a change of the palette.
      \param name the name of the icon (e.g. "play")
      \return the icon. A null icon, with a warning, if there are no files
      of the name.
    */
    static
    QIcon fromName( const QString & name );
};

#endif
