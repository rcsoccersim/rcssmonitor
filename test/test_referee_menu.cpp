// -*-c++-*-

/*!
  \file test_referee_menu.cpp
  \brief unit test of the menu of a right click on the field
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

#include "disp_holder.h"
#include "field_canvas.h"
#include "main_window.h"
#include "options.h"
#include "play_mode_position.h"
#include "referee_menu.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMenuBar>
#include <QTimer>

/*
  Options reads and writes ~/.rcssmonitor.conf; CTest runs this test with
  HOME in the build tree (test/CMakeLists.txt).
*/

namespace {

//! the submenu that an item opens, or null (QAction::menu() is deprecated in Qt 6.0 to 6.2)
QMenu *
submenu( const QAction * act )
{
    // a submenu's item belongs to the submenu
    QMenu * menu = ( act ? qobject_cast< QMenu * >( act->parent() ) : nullptr );
    return ( menu && menu->menuAction() == act ? menu : nullptr );
}

//! the item of a menu by its text
QAction *
item( const QMenu * menu,
      const QString & text )
{
    for ( QAction * act : menu->actions() )
    {
        if ( act->text() == text )
        {
            return act;
        }
    }
    return nullptr;
}

//! the texts of the items of a menu, "-" for a separator
QStringList
texts( const QMenu * menu )
{
    QStringList list;
    for ( const QAction * act : menu->actions() )
    {
        list << ( act->isSeparator() ? QString( "-" ) : act->text() );
    }
    return list;
}

//! the play modes of Change Playmode, by name
QStringList
modeNames()
{
    const char * playmode_strings[] = PLAYMODE_STRINGS;
    QStringList list;
    for ( const rcss::rcg::PlayMode mode : changePlayModeModes() )
    {
        list << QString::fromLatin1( playmode_strings[mode] );
    }
    return list;
}

//! what the signals of a menu emitted
struct Emitted {
    QStringList calls;
    RefereeTarget target;

    void listen( RefereeMenu & menu )
      {
          QObject::connect( &menu, &RefereeMenu::dropBall,
                            [this]( double x, double y ) { calls << QString( "drop %1 %2" ).arg( x ).arg( y ); } );
          QObject::connect( &menu, &RefereeMenu::freeKickLeft,
                            [this]( double x, double y ) { calls << QString( "left %1 %2" ).arg( x ).arg( y ); } );
          QObject::connect( &menu, &RefereeMenu::freeKickRight,
                            [this]( double x, double y ) { calls << QString( "right %1 %2" ).arg( x ).arg( y ); } );
          QObject::connect( &menu, &RefereeMenu::yellowCard,
                            [this]( const RefereeTarget & t ) { calls << "yellow"; target = t; } );
          QObject::connect( &menu, &RefereeMenu::redCard,
                            [this]( const RefereeTarget & t ) { calls << "red"; target = t; } );
          QObject::connect( &menu, &RefereeMenu::playModeChanged,
                            [this]( int mode, const RefereeTarget & t ) { calls << QString( "mode %1" ).arg( mode ); target = t; } );
      }
};

}

/*!
  \brief unit test of RefereeMenu, and of the views that ask for it
*/
class TestRefereeMenu
    : public QObject {
    Q_OBJECT

private slots:

    void items()
      {
          QWidget parent;
          RefereeMenu menu( &parent );
          QCOMPARE( texts( menu.menu() ),
                    QStringList() << "Drop Ball" << "Free Kick Left" << "Free Kick Right" << "-"
                    << "Yellow Card" << "Red Card" << "-" << "Change Playmode" );

          QAction * change = item( menu.menu(), "Change Playmode" );
          QVERIFY( submenu( change ) );
          QCOMPARE( texts( submenu( change ) ), modeNames() );
          QVERIFY( modeNames().contains( "back_pass_l" ) );
          QVERIFY( ! modeNames().contains( "foul_push_l" ) );
      }

    //! the Referee menu of the menu bar offers the same play modes
    void menuBar()
      {
          MainWindow window;
          QMenu * referee = nullptr;
          for ( QAction * act : window.menuBar()->actions() )
          {
              if ( act->text() == "&Referee" )
              {
                  referee = submenu( act );
              }
          }
          QVERIFY( referee );
          QAction * change = item( referee, "Change Playmode" );
          QVERIFY( submenu( change ) );
          QCOMPARE( texts( submenu( change ) ), modeNames() );
      }

    //! the items that need a point: enabled with one, disabled with the reason without
    void pointItems()
      {
          QWidget parent;
          RefereeMenu menu( &parent );
          Emitted emitted;
          emitted.listen( menu );

          menu.setTarget( RefereeTarget::point( 10.5, -3.25 ) );
          for ( const char * text : { "Drop Ball", "Free Kick Left", "Free Kick Right" } )
          {
              QAction * act = item( menu.menu(), text );
              QVERIFY( act->isEnabled() );
              QVERIFY( act->toolTip() == act->text() ); // no reason: Qt's default
              act->trigger();
          }
          QCOMPARE( emitted.calls, QStringList() << "drop 10.5 -3.25" << "left 10.5 -3.25" << "right 10.5 -3.25" );

          emitted.calls.clear();
          menu.setTarget( RefereeTarget() );
          for ( const char * text : { "Drop Ball", "Free Kick Left", "Free Kick Right" } )
          {
              QAction * act = item( menu.menu(), text );
              QVERIFY( ! act->isEnabled() );
              QCOMPARE( act->toolTip(), QString( RefereeMenu::NO_POINT ) );
              act->trigger();
              // Qt ignores trigger() of a disabled item: even if the item
              // were triggered, nothing is emitted without a point
              emit act->triggered( false );
          }
          QVERIFY( emitted.calls.isEmpty() );
          QVERIFY( menu.menu()->toolTipsVisible() );

          // with a point again
          menu.setTarget( RefereeTarget::point( 0, 0 ) );
          QVERIFY( item( menu.menu(), "Drop Ball" )->isEnabled() );
          QVERIFY( item( menu.menu(), "Drop Ball" )->toolTip() == "Drop Ball" );
      }

    //! the cards and Change Playmode pass the target, with the player aimed at (3D) or without a point
    void targetItems()
      {
          QWidget parent;
          RefereeMenu menu( &parent );
          Emitted emitted;
          emitted.listen( menu );

          menu.setTarget( RefereeTarget::player( rcss::rcg::RIGHT, 7, 10.0, 5.0 ) );
          item( menu.menu(), "Yellow Card" )->trigger();
          QCOMPARE( emitted.calls, QStringList() << "yellow" );
          QCOMPARE( emitted.target.side_, rcss::rcg::RIGHT );
          QCOMPARE( emitted.target.unum_, 7 );

          menu.setTarget( RefereeTarget::player( rcss::rcg::LEFT, 3, -1.0, 2.0 ) );
          item( menu.menu(), "Red Card" )->trigger();
          QCOMPARE( emitted.target.side_, rcss::rcg::LEFT );
          QCOMPARE( emitted.target.unum_, 3 );

          // still with no point: from the ball (MainWindow)
          menu.setTarget( RefereeTarget() );
          QMenu * change = submenu( item( menu.menu(), "Change Playmode" ) );
          item( change, "back_pass_l" )->trigger();
          QCOMPARE( emitted.calls.last(), QString( "mode %1" ).arg( int( rcss::rcg::PM_Back_Pass_Left ) ) );
          QVERIFY( ! emitted.target.has_point_ );

          menu.setTarget( RefereeTarget::point( 30.0, -20.0 ) );
          item( change, "illegal_defense_r" )->trigger();
          QCOMPARE( emitted.calls.last(), QString( "mode %1" ).arg( int( rcss::rcg::PM_Illegal_Defense_Right ) ) );
          QVERIFY( emitted.target.has_point_ );
          QCOMPARE( emitted.target.x_, 30.0 );
          QCOMPARE( emitted.target.y_, -20.0 );
          QVERIFY( ! emitted.target.hasPlayer() );
      }

    //! a right click on the 2D view asks for the menu at the point where the button was pressed
    void fieldCanvas()
      {
          DispHolder holder;
          FieldCanvas canvas( holder );
          canvas.resize( 640, 480 );

          int count = 0;
          RefereeTarget target;
          QPoint global;
          connect( &canvas, &FieldCanvas::refereeMenuRequested,
                   [&]( const RefereeTarget & t, const QPoint & g ) { ++count; target = t; global = g; } );

          QTest::mousePress( &canvas, Qt::RightButton, Qt::NoModifier, QPoint( 120, 80 ) );
          QContextMenuEvent event( QContextMenuEvent::Mouse, QPoint( 121, 81 ), QPoint( 500, 400 ) );
          QApplication::sendEvent( &canvas, &event );
          QCOMPARE( count, 1 );
          QVERIFY( target.has_point_ );
          QVERIFY( ! target.hasPlayer() );
          QCOMPARE( target.x_, Options::instance().fieldX( 120 ) );
          QCOMPARE( target.y_, Options::instance().fieldY( 80 ) );
          QVERIFY( target.x_ != Options::instance().fieldX( 320 ) ); // not the center
          QCOMPARE( global, QPoint( 500, 400 ) );

          // not the menu key
          QContextMenuEvent key( QContextMenuEvent::Keyboard, QPoint( 10, 10 ), QPoint( 20, 20 ) );
          QApplication::sendEvent( &canvas, &key );
          QCOMPARE( count, 1 );
      }

    //! the main window shows the menu for the 2D view, for the point aimed at
    void shownByMainWindow()
      {
          MainWindow window;
          FieldCanvas * canvas = window.findChild< FieldCanvas * >();
          RefereeMenu * menu = window.findChild< RefereeMenu * >();
          QVERIFY( canvas );
          QVERIFY( menu );

          bool shown = false;
          RefereeTarget target;
          QTimer::singleShot( 0, [&]()
                                 {
                                     shown = menu->menu()->isVisible();
                                     target = menu->target();
                                     menu->menu()->close();
                                 } );
          emit canvas->refereeMenuRequested( RefereeTarget::point( 12.0, -7.0 ), QPoint( 100, 100 ) );
          QVERIFY( shown );
          QVERIFY( target.has_point_ );
          QCOMPARE( target.x_, 12.0 );
          QCOMPARE( target.y_, -7.0 );
      }
};

QTEST_MAIN( TestRefereeMenu )
#include "test_referee_menu.moc"
