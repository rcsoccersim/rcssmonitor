// -*-c++-*-

/*!
  \file player_type_sort_proxy.h
  \brief sort proxy of the player type list. Header File.
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

#ifndef RCSSMONITOR_PLAYER_TYPE_SORT_PROXY_H
#define RCSSMONITOR_PLAYER_TYPE_SORT_PROXY_H

#include <QSortFilterProxyModel>
#include <QRegularExpression>
#include <QString>

/*!
  \class PlayerTypeSortProxy
  \brief sorts the rows of the player type list by the number in a cell.
*/
class PlayerTypeSortProxy
    : public QSortFilterProxyModel {
public:

    explicit
    PlayerTypeSortProxy( QObject * parent )
        : QSortFilterProxyModel( parent )
      { }

    /*!
      \brief get the sort key of a cell: the first number in the text, or 0
      if the text does not start with a number.
      \param text the cell text
      \return the sort key

      Some cells hold two numbers, such as "1.050 / 1.234". Reading the whole
      text as one number gave 0 for them, and such columns did not sort.
    */
    static
    double sortKey( const QString & text )
      {
          static const QRegularExpression s_number( "^\\s*[-+]?(\\d+\\.?\\d*|\\.\\d+)([eE][-+]?\\d+)?" );
          const QRegularExpressionMatch match = s_number.match( text );
          return match.hasMatch()
              ? match.captured( 0 ).trimmed().toDouble()
              : 0.0;
      }

protected:

    bool lessThan( const QModelIndex & left,
                   const QModelIndex & right ) const override
      {
          const QAbstractItemModel * src = sourceModel();
          return sortKey( src->data( left ).toString() )
              < sortKey( src->data( right ).toString() );
      }
};

#endif
