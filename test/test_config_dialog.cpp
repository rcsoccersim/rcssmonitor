// -*-c++-*-

/*!
  \file test_config_dialog.cpp
  \brief unit test of ConfigDialog
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

#include "config_dialog.h"
#include "disp_holder.h"
#include "options.h"

/*
  Options reads and writes ~/.rcssmonitor.conf; CTest runs this test with
  HOME in the build tree (test/CMakeLists.txt).
*/

/*!
  \brief unit test of ConfigDialog
*/
class TestConfigDialog
    : public QObject {
    Q_OBJECT

private slots:

    void selectFix();
    void illegalDefenseColor();
};

/*-------------------------------------------------------------------*/
/*!
  The "Fix" button of the player selection sets SELECT_FIX. It used to set
  SELECT_AUTO_RIGHT.
*/
void
TestConfigDialog::selectFix()
{
    DispHolder holder;
    ConfigDialog dialog( nullptr, holder );

    Options::instance().setPlayerSelectType( Options::SELECT_AUTO_LEFT );
    QSignalSpy spy( &dialog, SIGNAL( configured() ) );

    QVERIFY( QMetaObject::invokeMethod( &dialog, "clickSelectFix" ) );
    QCOMPARE( Options::instance().playerSelectType(), Options::SELECT_FIX );
    QCOMPARE( spy.count(), 1 );

    // already Fix: nothing changes
    QVERIFY( QMetaObject::invokeMethod( &dialog, "clickSelectFix" ) );
    QCOMPARE( Options::instance().playerSelectType(), Options::SELECT_FIX );
    QCOMPARE( spy.count(), 1 );
}

/*-------------------------------------------------------------------*/
/*!
  The "Illegal Defense" item of the color list starts from the illegal
  defense color. It used to start from the pointto color.
*/
void
TestConfigDialog::illegalDefenseColor()
{
    DispHolder holder;
    ConfigDialog dialog( nullptr, holder );

    const QColor illegal_defense = Options::instance().illegalDefensePen().color();
    const QColor pointto = Options::instance().pointtoPen().color();
    QVERIFY( illegal_defense != pointto );

    const ColorItem * item = nullptr;
    for ( const QListWidget * list : dialog.findChildren< QListWidget * >() )
    {
        for ( int i = 0; i < list->count(); ++i )
        {
            if ( list->item( i )->text() == "Illegal Defense" )
            {
                item = dynamic_cast< const ColorItem * >( list->item( i ) );
            }
        }
    }

    QVERIFY( item );
    QCOMPARE( item->newColor(), illegal_defense );
}

QTEST_MAIN( TestConfigDialog )

#include "test_config_dialog.moc"
