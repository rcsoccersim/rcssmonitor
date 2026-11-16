// -*-c++-*-

/*!
  \file test_popup_closer.cpp
  \brief unit test of PopupCloser
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

#include "popup_closer.h"

#include <QApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>
#include <QWheelEvent>
#include <QWindow>

namespace {

//! a window that counts the events given to it (as the 3D view, a window of its own)
class CountingWindow
    : public QWindow {
public:
    int presses = 0;
    int double_clicks = 0;
    int releases = 0;
    int wheels = 0;
protected:
    void mousePressEvent( QMouseEvent * ) override { ++presses; }
    void mouseDoubleClickEvent( QMouseEvent * ) override { ++double_clicks; }
    void mouseReleaseEvent( QMouseEvent * ) override { ++releases; }
    void wheelEvent( QWheelEvent * ) override { ++wheels; }
};

void
send( QWindow & window,
      const QEvent::Type type,
      const Qt::MouseButton button = Qt::LeftButton )
{
    QMouseEvent event( type, QPointF( 10.0, 10.0 ), QPointF( 110.0, 110.0 ),
                       button,
                       ( type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::MouseButtons( button ) ),
                       Qt::NoModifier );
    QCoreApplication::sendEvent( &window, &event );
}

void
sendWheel( QWindow & window )
{
    QWheelEvent event( QPointF( 10.0, 10.0 ), QPointF( 110.0, 110.0 ),
                       QPoint(), QPoint( 0, 120 ),
                       Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false );
    QCoreApplication::sendEvent( &window, &event );
}

}

class TestPopupCloser
    : public QObject {
    Q_OBJECT

private slots:

    /*!
      a click on the window closes the menu open, and goes no further; with
      no menu open, the clicks and the wheel go to the window
    */
    void closes()
      {
          CountingWindow window;
          window.installEventFilter( new PopupCloser( &window ) );

          // no menu: everything to the window
          send( window, QEvent::MouseButtonPress );
          send( window, QEvent::MouseButtonRelease );
          send( window, QEvent::MouseButtonDblClick );
          sendWheel( window );
          QCOMPARE( window.presses, 1 );
          QCOMPARE( window.releases, 1 );
          QCOMPARE( window.double_clicks, 1 );
          QCOMPARE( window.wheels, 1 );

          QMenu menu;
          menu.addAction( QStringLiteral( "Drop Ball" ) );
          menu.popup( QPoint( 200, 200 ) );
          QVERIFY( menu.isVisible() );
          QCOMPARE( QApplication::activePopupWidget(), &menu );

          // the wheel kept from the window, the menu left open
          sendWheel( window );
          QCOMPARE( window.wheels, 1 );
          QVERIFY( menu.isVisible() );

          // a click (of any button) closes it, and goes no further
          send( window, QEvent::MouseButtonPress, Qt::RightButton );
          QVERIFY( ! menu.isVisible() );
          QCOMPARE( QApplication::activePopupWidget(), static_cast< QWidget * >( nullptr ) );
          QCOMPARE( window.presses, 1 );

          // and so a double click
          menu.popup( QPoint( 200, 200 ) );
          QVERIFY( menu.isVisible() );
          send( window, QEvent::MouseButtonDblClick );
          QVERIFY( ! menu.isVisible() );
          QCOMPARE( window.double_clicks, 1 );

          // closed, the clicks go to the window again
          send( window, QEvent::MouseButtonPress );
          QCOMPARE( window.presses, 2 );
          sendWheel( window );
          QCOMPARE( window.wheels, 2 );
      }

    //! a menu and its submenu open: both closed
    void closesSubmenus()
      {
          CountingWindow window;
          window.installEventFilter( new PopupCloser( &window ) );
          QMenu menu;
          QMenu * submenu = menu.addMenu( QStringLiteral( "Change Playmode" ) );
          submenu->addAction( QStringLiteral( "play_on" ) );
          menu.popup( QPoint( 200, 200 ) );
          submenu->popup( QPoint( 300, 200 ) );
          QVERIFY( menu.isVisible() );
          QVERIFY( submenu->isVisible() );
          send( window, QEvent::MouseButtonPress );
          QVERIFY( ! submenu->isVisible() );
          QVERIFY( ! menu.isVisible() );
          QCOMPARE( window.presses, 0 );
      }

    /*!
      a menu open by exec() (as the referee's: a loop of its own until it
      closes): a click on the window ends it, with no item chosen
    */
    void endsExec()
      {
          CountingWindow window;
          window.installEventFilter( new PopupCloser( &window ) );
          QMenu menu;
          menu.addAction( QStringLiteral( "Drop Ball" ) );
          bool clicked = false;
          QTimer::singleShot( 50, &window, [&]() {
              clicked = true;
              send( window, QEvent::MouseButtonPress );
          } );
          // a guard: the menu closed after a while, if the click did not
          QTimer guard;
          guard.setSingleShot( true );
          connect( &guard, &QTimer::timeout, &menu, &QMenu::close );
          guard.start( 5000 );
          QElapsedTimer clock;
          clock.start();
          QAction * chosen = menu.exec( QPoint( 200, 200 ) );
          QVERIFY( clicked );
          QVERIFY( clock.elapsed() < 4000 ); // by the click, not the guard
          QCOMPARE( chosen, static_cast< QAction * >( nullptr ) );
          QCOMPARE( window.presses, 0 );
      }
};

QTEST_MAIN( TestPopupCloser )
#include "test_popup_closer.moc"
