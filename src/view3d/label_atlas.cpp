// -*-c++-*-

/*!
  \file label_atlas.cpp
  \brief the images of the labels of the 3D view. Source File.
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

#include "label_atlas.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>

namespace {

//! the room left at each side of a slot (pixels), so that a text does not touch its neighbor
constexpr int MARGIN = 4;
//! where the middle of the text is from the top of a slot (pixels)
constexpr double TEXT_MIDDLE = 34.0;
//! the black outline around the white text (pixels)
constexpr double OUTLINE = 6.0;

}

/*-------------------------------------------------------------------*/
LabelAtlas::LabelAtlas()
    : M_image( COLUMNS * LABEL_WIDTH, ROWS * LABEL_HEIGHT, QImage::Format_ARGB32_Premultiplied ),
      M_changed( true )
{
    M_image.fill( Qt::transparent );
    M_widths.fill( 0.0 );
    // the slot of the cards: white, tinted by their color (SceneBuilder)
    QPainter painter( &M_image );
    painter.fillRect( slotRect( WHITE_SLOT ), Qt::white );
}

/*-------------------------------------------------------------------*/
QRect
LabelAtlas::slotRect( const int slot ) const
{
    return QRect( ( slot % COLUMNS ) * LABEL_WIDTH, ( slot / COLUMNS ) * LABEL_HEIGHT,
                  LABEL_WIDTH, LABEL_HEIGHT );
}

/*-------------------------------------------------------------------*/
bool
LabelAtlas::setText( const int slot,
                     const QString & text,
                     const QFont & font )
{
    if ( slot < 0 || PLAYER_SLOTS <= slot )
    {
        return false;
    }

    QFont label_font( font.family() );
    label_font.setPixelSize( LABEL_FONT );
    label_font.setBold( font.bold() );
    const QString key = label_font.toString() + '|' + text;
    if ( key == M_keys[slot] )
    {
        return false;
    }
    M_keys[slot] = key;

    const QRect rect = slotRect( slot );
    QPainter painter( &M_image );
    painter.setCompositionMode( QPainter::CompositionMode_Source );
    painter.fillRect( rect, Qt::transparent );
    painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
    painter.setRenderHint( QPainter::Antialiasing );
    painter.setClipRect( rect );

    // the text as a path: centered, its middle at TEXT_MIDDLE, squeezed to
    // the slot if it is wider (as the web version's fillText with maxWidth)
    QPainterPath path;
    path.addText( 0.0, 0.0, label_font, text );
    const QFontMetricsF metrics( label_font );
    const double width = metrics.horizontalAdvance( text );
    const double room = LABEL_WIDTH - 2 * MARGIN;
    const double squeeze = ( width > room ? room / width : 1.0 );
    M_widths[slot] = std::min( width, room );

    painter.translate( rect.left() + LABEL_WIDTH / 2.0 - width * squeeze / 2.0,
                       rect.top() + TEXT_MIDDLE + ( metrics.ascent() - metrics.descent() ) / 2.0 );
    painter.scale( squeeze, 1.0 );
    painter.strokePath( path, QPen( Qt::black, OUTLINE / squeeze, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
    painter.fillPath( path, Qt::white );

    M_changed = true;
    return true;
}

/*-------------------------------------------------------------------*/
QRectF
LabelAtlas::uv( const int slot ) const
{
    const QRect r = slotRect( slot );
    return QRectF( double( r.left() ) / M_image.width(),
                   double( r.top() ) / M_image.height(),
                   double( r.width() ) / M_image.width(),
                   double( r.height() ) / M_image.height() );
}

/*-------------------------------------------------------------------*/
double
LabelAtlas::textWidth( const int slot ) const
{
    if ( slot < 0 || PLAYER_SLOTS <= slot )
    {
        return 0.0;
    }
    return M_widths[slot] * LABEL_SCALE / LABEL_HEIGHT;
}

/*-------------------------------------------------------------------*/
bool
LabelAtlas::takeChanged()
{
    const bool changed = M_changed;
    M_changed = false;
    return changed;
}
