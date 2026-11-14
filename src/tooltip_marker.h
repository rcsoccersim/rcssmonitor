// -*-c++-*-

/*!
  \file tooltip_marker.h
  \brief a dotted underline under the texts that have a tooltip. Header File.
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

#ifndef RCSSMONITOR_TOOLTIP_MARKER_H
#define RCSSMONITOR_TOOLTIP_MARKER_H

#include <QObject>
#include <QRect>

class QWidget;

/*!
  \class TooltipMarker
  \brief marks the texts that have a tooltip with a dotted underline, as
  the web version does: the labels, the check boxes, the radio buttons and
  the titles of the group boxes (not the push buttons, whose frame would be
  crossed). The details of an item are in its tooltip (the settings dialog
  keeps them out of the way), and the line tells which items have some.

  An event filter: on a paint event of a widget, the widget paints itself
  first, then the line is drawn over it, if the widget has a tooltip then
  (so a tooltip set later shows too).
*/
class TooltipMarker
    : public QObject {
public:

    explicit TooltipMarker( QObject * parent );

    /*!
      \brief mark the widgets under a widget (made already), with one marker
      owned by it
      \param root the widget (the settings dialog)
    */
    static void install( QWidget * root );

    /*!
      \brief where the text of a widget is drawn, or an empty rectangle for a
      widget that is not marked
    */
    static QRect textRect( const QWidget * widget );

protected:

    bool eventFilter( QObject * object,
                      QEvent * event ) override;
};

#endif
