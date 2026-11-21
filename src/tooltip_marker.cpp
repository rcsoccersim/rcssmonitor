// -*-c++-*-

/*!
  \file tooltip_marker.cpp
  \brief a dotted underline under the texts that have a tooltip. Source File.
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

#include "tooltip_marker.h"

#include <QCheckBox>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStyleOptionGroupBox>
#include <QTextDocument> // Qt::mightBeRichText()

#include <algorithm>

/*-------------------------------------------------------------------*/
TooltipMarker::TooltipMarker( QObject * parent )
    : QObject( parent )
{

}

/*-------------------------------------------------------------------*/
void
TooltipMarker::install( QWidget * root )
{
    TooltipMarker * marker = new TooltipMarker( root );
    for ( QWidget * w : root->findChildren< QWidget * >() )
    {
        if ( qobject_cast< QLabel * >( w )
             || qobject_cast< QCheckBox * >( w )
             || qobject_cast< QRadioButton * >( w )
             || qobject_cast< QGroupBox * >( w ) )
        {
            w->installEventFilter( marker );
        }
    }
}

/*-------------------------------------------------------------------*/
QRect
TooltipMarker::textRect( const QWidget * widget )
{
    const QStyle * style = widget->style();
    const QFontMetrics fm = widget->fontMetrics();

    if ( const QLabel * l = qobject_cast< const QLabel * >( widget ) )
    {
        // plain one-line texts only (not rich text, not wrapped, not pictures)
        if ( l->text().isEmpty() || l->wordWrap() || Qt::mightBeRichText( l->text() ) )
        {
            return QRect();
        }
        return style->itemTextRect( fm, l->contentsRect(), int( l->alignment() ), l->isEnabled(), l->text() );
    }

    const QAbstractButton * button = qobject_cast< const QCheckBox * >( widget );
    QStyle::SubElement contents = QStyle::SE_CheckBoxContents;
    if ( ! button )
    {
        button = qobject_cast< const QRadioButton * >( widget );
        contents = QStyle::SE_RadioButtonContents;
    }
    if ( button )
    {
        if ( button->text().isEmpty() )
        {
            return QRect();
        }
        QStyleOptionButton option;
        option.initFrom( button );
        option.text = button->text();
        option.icon = button->icon();
        option.iconSize = button->iconSize();
        const QRect area = style->subElementRect( contents, &option, button );
        return style->itemTextRect( fm, area, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                                    button->isEnabled(), button->text() );
    }

    if ( const QGroupBox * g = qobject_cast< const QGroupBox * >( widget ) )
    {
        if ( g->title().isEmpty() )
        {
            return QRect();
        }
        QStyleOptionGroupBox option;
        option.initFrom( g );
        option.text = g->title();
        option.lineWidth = 1;
        option.midLineWidth = 0;
        option.textAlignment = Qt::Alignment( g->alignment() );
        option.subControls = QStyle::SC_GroupBoxFrame | QStyle::SC_GroupBoxLabel;
        if ( g->isCheckable() )
        {
            option.subControls |= QStyle::SC_GroupBoxCheckBox;
        }
        if ( g->isFlat() )
        {
            option.features |= QStyleOptionFrame::Flat;
        }
        const QRect label = style->subControlRect( QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxLabel, g );
        return style->itemTextRect( fm, label, Qt::AlignLeft | Qt::AlignVCenter, g->isEnabled(), g->title() );
    }

    return QRect();
}

/*-------------------------------------------------------------------*/
bool
TooltipMarker::eventFilter( QObject * object,
                            QEvent * event )
{
    QWidget * w = qobject_cast< QWidget * >( object );
    if ( ! w
         || event->type() != QEvent::Paint
         || w->toolTip().isEmpty() )
    {
        return false;
    }

    // the widget paints itself first (its own event(), past the filters)
    object->event( event );

    const QRect text = textRect( w );
    if ( text.isEmpty() )
    {
        return true;
    }
    // a dotted line just under the baseline, in a soft text color, within
    // the widget (a label may be as high as its text only)
    const QFontMetrics fm = w->fontMetrics();
    const int y = std::min( text.top() + ( text.height() - fm.height() ) / 2 + fm.ascent() + 2, w->height() - 1 );
    QColor color = w->palette().color( w->isEnabled() ? QPalette::Active : QPalette::Disabled, QPalette::WindowText );
    color.setAlphaF( color.alphaF() * 0.7 );
    QPainter painter( w );
    QPen pen( color, 1.0, Qt::DotLine );
    pen.setCosmetic( true );
    painter.setPen( pen );
    painter.drawLine( text.left(), y, text.right(), y );
    return true;
}
