// -*-c++-*-

/*!
  \file test_referee_commands.cpp
  \brief test of the commands that the referee menu sends to a server
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
#include "referee_menu.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <QUdpSocket>

/*
  Options reads and writes ~/.rcssmonitor.conf; CTest runs this test with
  HOME in the build tree (test/CMakeLists.txt).
*/

namespace {

/*!
  \brief a server that only records what the monitor sends (UDP on the
  local host), as rcssserver's monitor port
*/
class FakeServer {
public:
    QUdpSocket socket;
    QHostAddress client_address;
    quint16 client_port = 0;

    bool start()
      {
          return socket.bind( QHostAddress::LocalHost, 0 );
      }

    //! the next message, or an empty string if none comes in time
    QString next( const int msec = 2000 )
      {
          QElapsedTimer timer;
          timer.start();
          while ( ! socket.hasPendingDatagrams() )
          {
              if ( timer.elapsed() > msec )
              {
                  return QString();
              }
              QTest::qWait( 10 );
          }
          QByteArray data( int( socket.pendingDatagramSize() ), '\0' );
          socket.readDatagram( data.data(), data.size(), &client_address, &client_port );
          // the monitor sends the terminating 0 too
          return QString::fromLatin1( data.constData() );
      }

    //! send a message to the monitor
    void send( const QByteArray & msg )
      {
          socket.writeDatagram( msg.constData(), msg.size() + 1, client_address, client_port );
      }
};

/*!
  \brief answers the dialogs that the commands open: the card's dialog
  (QInputDialog) is accepted as it is, and what it had chosen is recorded;
  a message box is recorded and closed
*/
class DialogAnswer
    : public QObject {
public:
    QStringList chosen; //!< the item each card's dialog had chosen
    QStringList messages; //!< the texts of the message boxes

    DialogAnswer()
      {
          M_timer.setInterval( 10 );
          connect( &M_timer, &QTimer::timeout, this, &DialogAnswer::answer );
          M_timer.start();
      }

private:
    QTimer M_timer;

    void answer()
      {
          QWidget * modal = QApplication::activeModalWidget();
          if ( QInputDialog * dialog = qobject_cast< QInputDialog * >( modal ) )
          {
              // the combo box of getItem()
              const QComboBox * combo = dialog->findChild< QComboBox * >();
              chosen << ( combo ? combo->currentText() : dialog->textValue() );
              dialog->accept();
          }
          else if ( QMessageBox * box = qobject_cast< QMessageBox * >( modal ) )
          {
              messages << box->text();
              box->close();
          }
      }
};

//! the item of a menu (or of its submenus) by its text
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
        if ( act->menu() )
        {
            if ( QAction * found = item( act->menu(), text ) )
            {
                return found;
            }
        }
    }
    return nullptr;
}

}

/*!
  \brief the commands that the menu of a right click on the field sends to
  the server, through the main window (docs/PLAN_3D-VIEW.md Phase 5: "the
  referee's commands reach a fake server")
*/
class TestRefereeCommands
    : public QObject {
    Q_OBJECT

private slots:

    void commands()
      {
          FakeServer server;
          QVERIFY( server.start() );
          Options::instance().setServerHost( "127.0.0.1" );
          Options::instance().setServerPort( server.socket.localPort() );

          DialogAnswer answer;
          MainWindow window;
          RefereeMenu * menu = window.findChild< RefereeMenu * >();
          QVERIFY( menu );

          // the monitor says hello; the server tells it runs in coach mode,
          // which Change Playmode needs
          QVERIFY( QMetaObject::invokeMethod( &window, "connectMonitor" ) );
          QCOMPARE( server.next(), QString( "(dispinit version 5)" ) );
          server.send( "(server_param (coach 1))" );

          // a player aimed at in 3D: the point at its feet (16 units per meter)
          menu->setTarget( RefereeTarget::player( rcss::rcg::RIGHT, 7, 10.5, -3.25 ) );
          item( menu->menu(), "Drop Ball" )->trigger();
          QCOMPARE( server.next(), QString( "(dispfoul 168 -52 0)" ) );
          item( menu->menu(), "Free Kick Left" )->trigger();
          QCOMPARE( server.next(), QString( "(dispfoul 168 -52 1)" ) );
          item( menu->menu(), "Free Kick Right" )->trigger();
          QCOMPARE( server.next(), QString( "(dispfoul 168 -52 -1)" ) );

          // the card's dialog has the player aimed at chosen
          item( menu->menu(), "Yellow Card" )->trigger();
          QCOMPARE( answer.chosen, QStringList() << "Right 7" );
          QCOMPARE( server.next(), QString( "(dispcard -1 7 yellow)" ) );
          menu->setTarget( RefereeTarget::player( rcss::rcg::LEFT, 11, 0.0, 0.0 ) );
          item( menu->menu(), "Red Card" )->trigger();
          QCOMPARE( answer.chosen.last(), QString( "Left 11" ) );
          QCOMPARE( server.next(), QString( "(dispcard 1 11 red)" ) );

          // Change Playmode at the point: the ball moves there, then the mode changes
          menu->setTarget( RefereeTarget::point( 20.0, -10.0 ) );
          item( menu->menu(), "play_on" )->trigger();
          QCOMPARE( server.next(), QString( "(move (ball) 20 -10 0 0 0)" ) );
          QCOMPARE( server.next(), QString( "(change_mode play_on)" ) );
          // a kick in: to the touch line
          item( menu->menu(), "kick_in_l" )->trigger();
          QCOMPARE( server.next(), QString( "(move (ball) 20 -34 0 0 0)" ) );
          QCOMPARE( server.next(), QString( "(change_mode kick_in_l)" ) );

          // without a point (3D: above the horizon) nothing is placed, and a
          // card's dialog chooses no one first
          menu->setTarget( RefereeTarget() );
          item( menu->menu(), "Drop Ball" )->trigger();
          item( menu->menu(), "Yellow Card" )->trigger();
          QCOMPARE( answer.chosen.last(), QString() );
          QCOMPARE( server.next( 300 ), QString() );

          QVERIFY2( answer.messages.isEmpty(), qPrintable( answer.messages.join( ", " ) ) );
      }
};

QTEST_MAIN( TestRefereeCommands )
#include "test_referee_commands.moc"
