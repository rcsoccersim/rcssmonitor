// -*-c++-*-

/*!
  \file ui_palette.cpp
  \brief the color schemes and the palettes of the widgets. Source File.
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

#include "ui_palette.h"

#include <QApplication>
#include <QWidget>

namespace {

//! the property of a widget that markWrongField() or markWrongText() marked
const char * const WRONG_MARK = "rcssmonitor_wrong_mark";

//! the values of WRONG_MARK
enum WrongMark {
    NOT_WRONG = 0,
    WRONG_FIELD = 1,
    WRONG_TEXT = 2,
};

/*!
  \brief set the palette of a widget with only a role of it set, or none
  \param w the widget
  \param role the role set, if set
  \param color the color of the role, if invalid none is set
  \param mark the mark to keep in the widget's property

  QPalette() is the application's palette with no role set, so the widget
  follows the application's palette (the color scheme) in the roles not
  set: a widget given the whole palette of the application (as
  QApplication::palette( w )) would keep its colors when the palette
  changes later.
*/
void
set_palette_role( QWidget * w,
                  const QPalette::ColorRole role,
                  const QColor & color,
                  const WrongMark mark )
{
    QPalette palette;
    if ( color.isValid() )
    {
        palette.setColor( role, color );
    }
    w->setPalette( palette );
    w->setProperty( WRONG_MARK, int( mark ) );
}

}

/*-------------------------------------------------------------------*/
/*!

*/
QPalette
UIPalette::light()
{
    return fusion( false );
}

/*-------------------------------------------------------------------*/
/*!

*/
QPalette
UIPalette::dark()
{
    return fusion( true );
}

/*-------------------------------------------------------------------*/
/*!

*/
QPalette
UIPalette::of( const UIColorScheme scheme,
               const QPalette & system )
{
    switch ( scheme ) {
    case UIColorScheme::Light:
        return light();
    case UIColorScheme::Dark:
        return dark();
    case UIColorScheme::System:
    default:
        break;
    }
    return system;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
UIPalette::isDark( const QPalette & palette )
{
    return palette.color( QPalette::Window ).lightness() < 128;
}

/*-------------------------------------------------------------------*/
/*!

*/
QColor
UIPalette::errorText( const QPalette & palette )
{
    // the contrast to the window of light() and dark() is about 4.9:1 and 4.8:1
    return isDark( palette )
        ? QColor( 255, 112, 112 )
        : QColor( 192, 48, 48 );
}

/*-------------------------------------------------------------------*/
/*!

*/
QColor
UIPalette::errorBase( const QPalette & palette )
{
    // 40 % of the red: pink on the white base, dark red on the dark one
    const QColor base = palette.color( QPalette::Base );
    const QColor error( 255, 64, 64 );
    return QColor( ( base.red() * 6 + error.red() * 4 ) / 10,
                   ( base.green() * 6 + error.green() * 4 ) / 10,
                   ( base.blue() * 6 + error.blue() * 4 ) / 10 );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
UIPalette::markWrongField( QWidget * field,
                           const bool wrong )
{
    if ( ! field )
    {
        return;
    }

    if ( wrong )
    {
        set_palette_role( field, QPalette::Base,
                          errorBase( QApplication::palette( field ) ),
                          WRONG_FIELD );
    }
    else
    {
        set_palette_role( field, QPalette::Base, QColor(), NOT_WRONG );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
UIPalette::markWrongText( QWidget * text,
                          const bool wrong )
{
    if ( ! text )
    {
        return;
    }

    if ( wrong )
    {
        set_palette_role( text, QPalette::WindowText,
                          errorText( QApplication::palette( text ) ),
                          WRONG_TEXT );
    }
    else
    {
        set_palette_role( text, QPalette::WindowText, QColor(), NOT_WRONG );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
UIPalette::updateWrongMarks( QWidget * root )
{
    if ( ! root )
    {
        return;
    }

    QList< QWidget * > widgets = root->findChildren< QWidget * >();
    widgets.prepend( root );
    for ( QWidget * w : widgets )
    {
        switch ( w->property( WRONG_MARK ).toInt() ) {
        case WRONG_FIELD:
            markWrongField( w, true );
            break;
        case WRONG_TEXT:
            markWrongText( w, true );
            break;
        default:
            break;
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
QPalette
UIPalette::fusion( const bool dark )
{
    // The same colors as the standard palette of the Fusion style
    // (qt_fusionPalette() of Qt 6.5 or later).
    const QColor window_text = dark ? QColor( 240, 240, 240 ) : QColor( Qt::black );
    const QColor background = dark ? QColor( 50, 50, 50 ) : QColor( 239, 239, 239 );
    const QColor light = background.lighter( 150 );
    const QColor mid = background.darker( 130 );
    const QColor midlight = mid.lighter( 110 );
    const QColor base = dark ? background.darker( 140 ) : QColor( Qt::white );
    const QColor dark_color = background.darker( 150 );
    const QColor shadow = dark_color.darker( 135 );
    const QColor text = dark ? window_text : QColor( Qt::black );
    const QColor highlight( 48, 140, 198 );
    const QColor highlighted_text = dark ? window_text : QColor( Qt::white );
    const QColor disabled_text = dark ? QColor( 130, 130, 130 ) : QColor( 190, 190, 190 );
    const QColor disabled_dark = QColor( 209, 209, 209 ).darker( 110 );
    const QColor disabled_shadow = shadow.lighter( 150 );
    const QColor disabled_highlight( 145, 145, 145 );
    QColor placeholder = text;
    placeholder.setAlpha( 128 );

    QPalette pal( window_text, background, light, dark_color, mid, text, base );
    pal.setBrush( QPalette::Midlight, midlight );
    pal.setBrush( QPalette::Button, background );
    pal.setBrush( QPalette::Shadow, shadow );
    pal.setBrush( QPalette::HighlightedText, highlighted_text );

    pal.setBrush( QPalette::Disabled, QPalette::Text, disabled_text );
    pal.setBrush( QPalette::Disabled, QPalette::WindowText, disabled_text );
    pal.setBrush( QPalette::Disabled, QPalette::ButtonText, disabled_text );
    pal.setBrush( QPalette::Disabled, QPalette::Base, background );
    pal.setBrush( QPalette::Disabled, QPalette::Dark, disabled_dark );
    pal.setBrush( QPalette::Disabled, QPalette::Shadow, disabled_shadow );

    pal.setBrush( QPalette::Active, QPalette::Highlight, highlight );
    pal.setBrush( QPalette::Inactive, QPalette::Highlight, highlight );
    pal.setBrush( QPalette::Disabled, QPalette::Highlight, disabled_highlight );

    pal.setBrush( QPalette::PlaceholderText, placeholder );

    if ( dark )
    {
        // a light blue that is more legible on a dark background than Qt::blue
        pal.setBrush( QPalette::Link, highlight );
    }

    return pal;
}
