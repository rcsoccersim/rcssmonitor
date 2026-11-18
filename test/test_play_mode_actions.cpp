// -*-c++-*-

/*!
  \file test_play_mode_actions.cpp
  \brief unit test of the actions that change the play mode
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


#include <QtTest>

#include "field_canvas.h"
#include "main_window.h"

#include <rcss/rcg/types.h>

#include <QAction>
#include <QActionGroup>

/*
  Options reads and writes ~/.rcssmonitor.conf; CTest runs this test with
  HOME in the build tree (test/CMakeLists.txt).
*/

/*!
  \brief unit test of the actions that change the play mode
*/
class TestPlayModeActions
    : public QObject {
    Q_OBJECT

private slots:

    void fieldMenu_data();
    void fieldMenu();
};

/*-------------------------------------------------------------------*/
void
TestPlayModeActions::fieldMenu_data()
{
    QTest::addColumn< QString >( "text" );
    QTest::addColumn< int >( "mode" );

    QTest::newRow( "before_kick_off" ) << "before_kick_off" << int( rcss::rcg::PM_BeforeKickOff );
    QTest::newRow( "play_on" ) << "play_on" << int( rcss::rcg::PM_PlayOn );
    QTest::newRow( "back_pass_l" ) << "back_pass_l" << int( rcss::rcg::PM_Back_Pass_Left );
    QTest::newRow( "illegal_defense_r" ) << "illegal_defense_r" << int( rcss::rcg::PM_Illegal_Defense_Right );
}

/*-------------------------------------------------------------------*/
/*!
  An item of "Change Playmode" in the menu of the field passes its play
  mode to the field canvas, which emits it with the point of the menu.
*/
void
TestPlayModeActions::fieldMenu()
{
    QFETCH( QString, text );
    QFETCH( int, mode );

    MainWindow window;
    FieldCanvas * canvas = window.findChild< FieldCanvas * >();
    QVERIFY( canvas );
    QSignalSpy spy( canvas, SIGNAL( playModeChanged( int, const QPoint & ) ) );

    QAction * found = nullptr;
    for ( QAction * act : window.findChildren< QAction * >() )
    {
        // the items of the field menu are in an action group
        if ( act->actionGroup() && act->text() == text )
        {
            QVERIFY( ! found );
            found = act;
        }
    }
    QVERIFY( found );

    found->trigger();
    QCOMPARE( spy.count(), 1 );
    QCOMPARE( spy.at( 0 ).at( 0 ).toInt(), mode );
}

QTEST_MAIN( TestPlayModeActions )

#include "test_play_mode_actions.moc"
