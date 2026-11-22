// -*-c++-*-

/*!
  \file ui_style.cpp
  \brief the style of the widgets. Source File.
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

#include "ui_style.h"

#include <QPainter>
#include <QStyleOption>
#include <QToolBar>

namespace {

/*!
  \brief whether a widget is in a tool bar: its parent, or an ancestor in
  the same window (the main window's buttons are in a frame in the tool bar)
*/
bool
in_tool_bar( const QWidget * w )
{
    for ( const QWidget * p = w ? w->parentWidget() : nullptr;
          p;
          p = p->parentWidget() )
    {
        if ( qobject_cast< const QToolBar * >( p ) )
        {
            return true;
        }
        if ( p->isWindow() )
        {
            break;
        }
    }
    return false;
}

}

/*-------------------------------------------------------------------*/
/*!

*/
UIStyle::UIStyle( const QString & key )
    : QProxyStyle( key )
{
    // The style menu identifies the current style by its object name.
    if ( baseStyle() )
    {
        setObjectName( baseStyle()->objectName() );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
UIStyle::drawPrimitive( PrimitiveElement element,
                        const QStyleOption * option,
                        QPainter * painter,
                        const QWidget * widget ) const
{
    if ( element == PE_PanelButtonTool
         && ( option->state & State_On )
         && in_tool_bar( widget ) )
    {
        const QColor highlight = option->palette.color( QPalette::Highlight );
        QColor fill = highlight;
        fill.setAlpha( option->state & ( State_MouseOver | State_Sunken ) ? 140 : 90 );

        painter->save();
        painter->setRenderHint( QPainter::Antialiasing, true );
        painter->setPen( highlight );
        painter->setBrush( fill );
        painter->drawRoundedRect( QRectF( option->rect ).adjusted( 0.5, 0.5, -0.5, -0.5 ), 3.0, 3.0 );
        painter->restore();
        return;
    }

    QProxyStyle::drawPrimitive( element, option, painter, widget );
}
