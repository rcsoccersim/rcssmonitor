// -*-c++-*-

/*!
  \file test_player_select_keys.cpp
  \brief unit test of the keys that select a player
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

#include "main_window.h"
#include "options.h"

#include <QAction>
#include <QKeySequence>

/*
  Options reads and writes ~/.rcssmonitor.conf; CTest runs this test with
  HOME in the build tree (test/CMakeLists.txt).
*/

namespace {

/*!
  \brief the action of the main window that has the shortcut.
*/
QAction *
findAction( const MainWindow & window,
            const QKeySequence & key )
{
    QAction * found = nullptr;
    for ( QAction * act : window.findChildren< QAction * >() )
    {
        if ( act->shortcut() == key )
        {
            if ( found ) return nullptr; // not unique
            found = act;
        }
    }
    return found;
}

}

/*!
  \brief unit test of the keys that select a player
*/
class TestPlayerSelectKeys
    : public QObject {
    Q_OBJECT

private slots:

    void selectPlayer_data();
    void selectPlayer();
};

/*-------------------------------------------------------------------*/
void
TestPlayerSelectKeys::selectPlayer_data()
{
    QTest::addColumn< QString >( "key" );
    QTest::addColumn< int >( "number" ); // > 0: left, < 0: right

    QTest::newRow( "1" ) << "1" << 1;
    QTest::newRow( "5" ) << "5" << 5;
    QTest::newRow( "9" ) << "9" << 9;
    QTest::newRow( "0" ) << "0" << 10;
    QTest::newRow( "-" ) << "-" << 11;
    QTest::newRow( "Ctrl+1" ) << "Ctrl+1" << -1;
    QTest::newRow( "Ctrl+5" ) << "Ctrl+5" << -5;
    QTest::newRow( "Ctrl+9" ) << "Ctrl+9" << -9;
    QTest::newRow( "Ctrl+0" ) << "Ctrl+0" << -10;
    QTest::newRow( "Ctrl+-" ) << "Ctrl+-" << -11;
}

/*-------------------------------------------------------------------*/
/*!
  1-9, 0 and - select the left player 1-11, and with Ctrl the right one.
*/
void
TestPlayerSelectKeys::selectPlayer()
{
    QFETCH( QString, key );
    QFETCH( int, number );

    MainWindow window;
    Options::instance().setSelectedNumber( rcss::rcg::LEFT, 0 );
    Options::instance().setPlayerSelectType( Options::SELECT_AUTO_ALL );

    QAction * act = findAction( window, QKeySequence( key ) );
    QVERIFY( act );
    act->trigger();

    QCOMPARE( Options::instance().selectedNumber(), number );
    QCOMPARE( Options::instance().playerSelectType(), Options::SELECT_FIX );
}

QTEST_MAIN( TestPlayerSelectKeys )

#include "test_player_select_keys.moc"
