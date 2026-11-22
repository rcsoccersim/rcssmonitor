// -*-c++-*-

/*!
  \file ui_palette.h
  \brief the color schemes and the palettes of the widgets. Header File.
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

#ifndef RCSSMONITOR_UI_PALETTE_H
#define RCSSMONITOR_UI_PALETTE_H

#include <QColor>
#include <QPalette>
#include <QString>

#include <array>
#include <utility>

class QWidget;

/*!
  \enum UIColorScheme
  \brief the color scheme of the widgets (View > UI Color Scheme,
  --ui-color-scheme): the desktop's, light or dark. The field (2D and 3D)
  has its own colors and does not change.
*/
enum class UIColorScheme {
    System, //!< the palette of the desktop (the default)
    Light, //!< the light palette (UIPalette::light())
    Dark, //!< the dark palette (UIPalette::dark())
};

//! the color schemes, with their names in the options
inline
const std::array< std::pair< UIColorScheme, const char * >, 3 > &
uiColorSchemes()
{
    static const std::array< std::pair< UIColorScheme, const char * >, 3 > list = { {
            { UIColorScheme::System, "system" },
            { UIColorScheme::Light, "light" },
            { UIColorScheme::Dark, "dark" },
        } };
    return list;
}

//! the name of a color scheme in the options
inline
QString
uiColorSchemeName( const UIColorScheme scheme )
{
    for ( const auto & s : uiColorSchemes() )
    {
        if ( s.first == scheme ) return QString::fromLatin1( s.second );
    }
    return QString();
}

/*!
  \brief the color scheme of a name in the options
  \return false if the name is unknown (scheme is left as it was)
*/
inline
bool
uiColorSchemeFromName( const QString & name,
                       UIColorScheme * scheme )
{
    for ( const auto & s : uiColorSchemes() )
    {
        if ( name == QLatin1String( s.second ) )
        {
            *scheme = s.first;
            return true;
        }
    }
    return false;
}

/*!
  \class UIPalette
  \brief the palettes of the widgets
*/
class UIPalette {
private:
    // not used
    UIPalette() = delete;

public:

    /*!
      \brief create the light palette.

      The palette has the same colors as the standard palette of the Fusion
      style in the light mode, the same with Qt5 and Qt6. Qt6 makes the
      standard palette dark when the desktop uses a dark color scheme, so
      the palette is built explicitly.
      \return the light palette
    */
    static
    QPalette light();

    /*!
      \brief create the dark palette.

      The palette has the same colors as the standard palette of the Fusion
      style in the dark mode (Qt 6.5 or later), the same with Qt5 and Qt6,
      whatever the color scheme of the desktop is.
      \return the dark palette
    */
    static
    QPalette dark();

    /*!
      \brief the palette of a color scheme
      \param scheme the color scheme
      \param system the palette of the desktop, for UIColorScheme::System
      \return the palette
    */
    static
    QPalette of( const UIColorScheme scheme,
                 const QPalette & system );

    /*!
      \brief whether a palette is dark: its window is darker than the middle
      gray. The colors drawn on the window (e.g. of an error) are chosen by
      this.
      \param palette the palette
      \return true if the palette is dark
    */
    static
    bool isDark( const QPalette & palette );

    /*!
      \brief the color of the text of an error on the window of a palette:
      a red legible on the light window and on the dark one
      \param palette the palette
      \return the color
    */
    static
    QColor errorText( const QPalette & palette );

    /*!
      \brief the base color of a field while what is typed in it is wrong:
      the base color of the palette mixed with a red, light on the light
      palette and dark on the dark one, so that the text stays legible
      \param palette the palette
      \return the color
    */
    static
    QColor errorBase( const QPalette & palette );

    /*!
      \brief mark a field (e.g. a line edit or a spin box) as wrong or not:
      its base color is errorBase() while it is wrong. Only that color is
      set, so that the field follows the application's palette (the color
      scheme) in the others, and in all again when it is not wrong.
      \param field the field
      \param wrong true while what is typed in it is wrong
    */
    static
    void markWrongField( QWidget * field,
                         const bool wrong );

    /*!
      \brief mark a text (e.g. a label) as wrong or not: its color is
      errorText() while it is wrong, as markWrongField().
      \param text the widget of the text
      \param wrong true while the text tells what is wrong
    */
    static
    void markWrongText( QWidget * text,
                        const bool wrong );

    /*!
      \brief mark again the wrong fields and texts in a widget and its
      descendants, in the colors of the application's palette now: called
      when the palette changes (the color scheme), as the marks keep the
      colors of the palette when they were made.
      \param root the widget
    */
    static
    void updateWrongMarks( QWidget * root );

private:

    /*!
      \brief create the palette of the Fusion style.
      \param dark true for the dark mode, false for the light mode
      \return the palette
    */
    static
    QPalette fusion( const bool dark );
};

#endif
