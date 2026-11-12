// -*-c++-*-

/*!
  \file test_view_switch.cpp
  \brief test of switching between the 2D view and the 3D view
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
#include "field_view_3d.h"
#include "main_window.h"
#include "options.h"

#include <QAction>
#include <QKeySequence>
#include <QStackedWidget>
#include <QStatusBar>

/*
  Options reads and writes ~/.rcssmonitor.conf; CTest runs this test with
  HOME in the build tree and QT_QPA_PLATFORM=offscreen (test/CMakeLists.txt).
  That platform shows no OpenGL widgets, so the 3D view is not available
  here: this test checks how the main window falls back to the 2D view. The
  3D view itself is drawn offscreen in test_scene_3d.
*/

namespace {

QAction *
view3DAction( const MainWindow & window )
{
    for ( QAction * act : window.findChildren< QAction * >() )
    {
        if ( act->shortcut() == QKeySequence( Qt::Key_D ) ) return act;
    }
    return nullptr;
}

}

/*!
  \brief test of switching between the 2D view and the 3D view
*/
class TestViewSwitch
    : public QObject {
    Q_OBJECT

private slots:

    void unavailableHere();
    void fallBackTo2D();
};

/*-------------------------------------------------------------------*/
void
TestViewSwitch::unavailableHere()
{
    QVERIFY( FieldView3D::unavailableReason().contains( "offscreen" ) );
}

/*-------------------------------------------------------------------*/
/*!
  D tries the 3D view; where it is not available, the action is disabled
  with the reason, and the 2D view stays with the selected player.
*/
void
TestViewSwitch::fallBackTo2D()
{
    Options::instance().setView3D( false );
    Options::instance().setSelectedNumber( rcss::rcg::RIGHT, 7 );

    MainWindow window;
    QAction * act = view3DAction( window );
    QVERIFY( act );
    QVERIFY( act->isEnabled() );
    QVERIFY( act->isCheckable() );

    QStackedWidget * stack = window.findChild< QStackedWidget * >();
    QVERIFY( stack );
    QVERIFY( qobject_cast< FieldCanvas * >( stack->currentWidget() ) );

    act->trigger();

    QVERIFY( ! act->isChecked() );
    QVERIFY( ! act->isEnabled() );
    QVERIFY( act->statusTip().contains( "offscreen" ) );
    QVERIFY( window.statusBar()->currentMessage().contains( "offscreen" ) );
    QVERIFY( qobject_cast< FieldCanvas * >( stack->currentWidget() ) );
    QCOMPARE( stack->count(), 1 );
    QVERIFY( ! Options::instance().view3D() );
    QCOMPARE( Options::instance().selectedNumber(), -7 );
}

QTEST_MAIN( TestViewSwitch )

#include "test_view_switch.moc"
