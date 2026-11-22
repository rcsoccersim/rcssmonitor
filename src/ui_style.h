// -*-c++-*-

/*!
  \file ui_style.h
  \brief the style of the widgets. Header File.
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

#ifndef RCSSMONITOR_UI_STYLE_H
#define RCSSMONITOR_UI_STYLE_H

#include <QProxyStyle>

/*!
  \class UIStyle
  \brief the style that emphasizes the checked tool buttons in the tool bars.

  The checked state of the tool buttons is hard to see with some styles and
  palettes (especially with a dark palette). This style draws the panel of
  the checked tool buttons in the tool bars (also in a widget in a tool bar,
  as the main window's) with a translucent background
  and a border of the highlight color of the palette. The other elements are
  drawn by the base style, so the sizes of the buttons do not change.
*/
class UIStyle
    : public QProxyStyle {
public:

    /*!
      \brief create the style with the base style given by the key.
      \param key the key of the base style (e.g. "Fusion"). If the key is
      invalid, the default style is used as the base style.
    */
    explicit
    UIStyle( const QString & key );

    /*!
      \brief draw the primitive element.
      The panel of the checked tool buttons in the tool bars is drawn by this
      style. The other elements are drawn by the base style.
    */
    void drawPrimitive( PrimitiveElement element,
                        const QStyleOption * option,
                        QPainter * painter,
                        const QWidget * widget = nullptr ) const override;
};

#endif
