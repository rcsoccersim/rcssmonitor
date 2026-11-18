// -*-c++-*-

/*!
  \file test_player_type_sort.cpp
  \brief unit test of PlayerTypeSortProxy
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

#include <QtTest>
#include <QStandardItemModel>

#include "player_type_sort_proxy.h"

namespace {

/*!
  \brief sort one column of texts and return the texts in the sorted order.
*/
QStringList
sorted( const QStringList & texts )
{
    QStandardItemModel model( texts.size(), 1 );
    for ( int i = 0; i < texts.size(); ++i )
    {
        model.setData( model.index( i, 0 ), texts[i] );
    }

    PlayerTypeSortProxy proxy( nullptr );
    proxy.setSourceModel( &model );
    proxy.sort( 0, Qt::AscendingOrder );

    QStringList result;
    for ( int i = 0; i < proxy.rowCount(); ++i )
    {
        result << proxy.data( proxy.index( i, 0 ) ).toString();
    }
    return result;
}

}

/*!
  \brief unit test of PlayerTypeSortProxy
*/
class TestPlayerTypeSort
    : public QObject {
    Q_OBJECT

private slots:

    void twoNumbers();
    void oneNumber();
    void sortKey();
};

/*-------------------------------------------------------------------*/
/*!
  A column with two numbers in a cell sorts by the first one. It did not
  sort, as the whole text read as one number was 0.
*/
void
TestPlayerTypeSort::twoNumbers()
{
    QCOMPARE( sorted( { "1.050 / 1.234", "0.950 / 1.300", "1.000 / 0.900" } ),
              QStringList( { "0.950 / 1.300", "1.000 / 0.900", "1.050 / 1.234" } ) );
}

/*-------------------------------------------------------------------*/
/*!
  A column with one number sorts by value, not by text.
*/
void
TestPlayerTypeSort::oneNumber()
{
    QCOMPARE( sorted( { "10", "9", "-1", "0.5" } ),
              QStringList( { "-1", "0.5", "9", "10" } ) );
}

/*-------------------------------------------------------------------*/
/*!
  The sort key is the first number, or 0 without one.
*/
void
TestPlayerTypeSort::sortKey()
{
    QCOMPARE( PlayerTypeSortProxy::sortKey( "1.050 / 1.234" ), 1.05 );
    QCOMPARE( PlayerTypeSortProxy::sortKey( " 2.5e1 / 3" ), 25.0 );
    QCOMPARE( PlayerTypeSortProxy::sortKey( "-.5" ), -0.5 );
    QCOMPARE( PlayerTypeSortProxy::sortKey( "abc" ), 0.0 );
    QCOMPARE( PlayerTypeSortProxy::sortKey( "" ), 0.0 );
}

QTEST_APPLESS_MAIN( TestPlayerTypeSort )

#include "test_player_type_sort.moc"
