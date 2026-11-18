// -*-c++-*-

/*!
  \file test_monitor_transport.cpp
  \brief unit test of the TCP and WebSocket transports, with local servers
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#ifdef HAVE_QT_WEBSOCKETS
#include <QWebSocket>
#include <QWebSocketServer>
#endif

#include "monitor_transport.h"

#include <memory>

namespace {

// The warnings of QObject::connect(): a signal or a slot connected by its
// name that does not exist (as QAbstractSocket::error() in Qt6) is told
// only so, when the program runs.
QStringList g_connect_warnings;
QtMessageHandler g_previous_handler = nullptr;

void
catch_connect_warnings( QtMsgType type,
                        const QMessageLogContext & context,
                        const QString & msg )
{
    if ( msg.contains( QLatin1String( "QObject::connect" ) ) )
    {
        g_connect_warnings << msg;
    }
    if ( g_previous_handler )
    {
        g_previous_handler( type, context, msg );
    }
}

//! a port of the local host where nothing listens
quint16
closed_port()
{
    QTcpServer server;
    server.listen( QHostAddress::LocalHost, 0 );
    return server.serverPort(); // closed when the server is gone
}

std::unique_ptr< MonitorTransport >
create( const MonitorTransport::Type type )
{
    QString error;
    std::unique_ptr< MonitorTransport > transport( create_monitor_transport( type,
                                                                             QStringLiteral( "/monitor" ),
                                                                             nullptr,
                                                                             &error ) );
    if ( ! transport )
    {
        qWarning( "create_monitor_transport: %s", qPrintable( error ) );
    }
    return transport;
}

}

class TestMonitorTransport
    : public QObject {

    Q_OBJECT

private slots:

    void initTestCase()
      {
          g_previous_handler = qInstallMessageHandler( catch_connect_warnings );
      }

    void cleanup()
      {
          // each test's own warnings
          const QStringList warnings = g_connect_warnings;
          g_connect_warnings.clear();
          QVERIFY2( warnings.isEmpty(),
                    qPrintable( warnings.join( QLatin1Char( '\n' ) ) ) );
      }

    /*!
      TCP: a message is sent with a NUL at its end; the messages received
      are split at their NULs, also when one comes in pieces; the server
      closing the connection is told once.
     */
    void tcp()
      {
          QTcpServer server;
          QVERIFY( server.listen( QHostAddress::LocalHost, 0 ) );

          std::unique_ptr< MonitorTransport > transport = create( MonitorTransport::TCP );
          QVERIFY( transport );
          QCOMPARE( transport->type(), MonitorTransport::TCP );
          QVERIFY( ! transport->isBinarySafe() );

          QSignalSpy connected( transport.get(), &MonitorTransport::connected );
          QSignalSpy available( transport.get(), &MonitorTransport::messagesAvailable );
          QSignalSpy disconnected( transport.get(), &MonitorTransport::disconnected );

          QString error;
          QVERIFY( transport->open( QStringLiteral( "127.0.0.1" ), server.serverPort(), &error ) );
          QCOMPARE( transport->peerName(),
                    QStringLiteral( "tcp://127.0.0.1:%1" ).arg( server.serverPort() ) );
          QTRY_VERIFY( server.hasPendingConnections() );
          QTcpSocket * peer = server.nextPendingConnection();
          QTRY_COMPARE( connected.count(), 1 );
          QVERIFY( transport->isOpen() );

          // to the server: the message and a NUL
          QVERIFY( transport->sendMessage( "(dispinit version 5)" ) );
          const QByteArray expected( "(dispinit version 5)\0", 21 );
          QByteArray got;
          QTRY_VERIFY( ( got += peer->readAll() ).size() >= expected.size() );
          QCOMPARE( got, expected );

          // from the server: split at the NULs, the empty ones dropped
          peer->write( QByteArray( "(a)\0\0(b)\0(c", 11 ) );
          QTRY_VERIFY( available.count() >= 1 );
          QByteArray msg;
          QVERIFY( transport->popMessage( msg ) );
          QCOMPARE( msg, QByteArray( "(a)" ) );
          QVERIFY( transport->popMessage( msg ) );
          QCOMPARE( msg, QByteArray( "(b)" ) );
          QVERIFY( ! transport->popMessage( msg ) );
          // the rest of a message in pieces
          available.clear();
          peer->write( QByteArray( ")\0", 2 ) );
          QTRY_VERIFY( available.count() >= 1 );
          QVERIFY( transport->popMessage( msg ) );
          QCOMPARE( msg, QByteArray( "(c)" ) );

          // the server closes the connection: told once
          peer->disconnectFromHost();
          QTRY_COMPARE( disconnected.count(), 1 );
          QVERIFY( ! disconnected.at( 0 ).at( 0 ).toString().isEmpty() );
          QVERIFY( ! transport->isOpen() );
          QTest::qWait( 100 );
          QCOMPARE( disconnected.count(), 1 );
      }

    /*!
      TCP: closing the connection here tells nothing, and the server sees
      it closed after the last message.
     */
    void tcpClose()
      {
          QTcpServer server;
          QVERIFY( server.listen( QHostAddress::LocalHost, 0 ) );

          std::unique_ptr< MonitorTransport > transport = create( MonitorTransport::TCP );
          QVERIFY( transport );
          QSignalSpy connected( transport.get(), &MonitorTransport::connected );
          QSignalSpy disconnected( transport.get(), &MonitorTransport::disconnected );
          QString error;
          QVERIFY( transport->open( QStringLiteral( "127.0.0.1" ), server.serverPort(), &error ) );
          QTRY_VERIFY( server.hasPendingConnections() );
          QTcpSocket * peer = server.nextPendingConnection();
          QTRY_COMPARE( connected.count(), 1 );

          QSignalSpy peer_closed( peer, &QTcpSocket::disconnected );
          QVERIFY( transport->sendMessage( "(dispbye)" ) );
          transport->close();
          QVERIFY( ! transport->isOpen() );
          QTRY_COMPARE( peer_closed.count(), 1 );
          QCOMPARE( peer->readAll(), QByteArray( "(dispbye)\0", 10 ) );
          QTest::qWait( 100 );
          QCOMPARE( disconnected.count(), 0 );
      }

    /*!
      TCP: a connection refused (no server) is told, with the reason. Qt6
      has no signal QAbstractSocket::error(), which the transport waited
      for: nothing was told.
     */
    void tcpRefused()
      {
          std::unique_ptr< MonitorTransport > transport = create( MonitorTransport::TCP );
          QVERIFY( transport );
          QSignalSpy connected( transport.get(), &MonitorTransport::connected );
          QSignalSpy disconnected( transport.get(), &MonitorTransport::disconnected );
          QString error;
          QVERIFY( transport->open( QStringLiteral( "127.0.0.1" ), closed_port(), &error ) );
          QTRY_COMPARE_WITH_TIMEOUT( disconnected.count(), 1, 5000 );
          QVERIFY( ! disconnected.at( 0 ).at( 0 ).toString().isEmpty() );
          QCOMPARE( connected.count(), 0 );
          QVERIFY( ! transport->isOpen() );
      }

#ifdef HAVE_QT_WEBSOCKETS
    /*!
      WebSocket: the request path, no Origin header; a message is a text
      message without a NUL; the server closing the connection is told once.
     */
    void webSocket()
      {
          QWebSocketServer server( QStringLiteral( "test" ), QWebSocketServer::NonSecureMode );
          QVERIFY( server.listen( QHostAddress::LocalHost, 0 ) );

          std::unique_ptr< MonitorTransport > transport = create( MonitorTransport::WebSocket );
          QVERIFY( transport );
          QCOMPARE( transport->type(), MonitorTransport::WebSocket );
          QVERIFY( ! transport->isBinarySafe() );

          QSignalSpy connected( transport.get(), &MonitorTransport::connected );
          QSignalSpy available( transport.get(), &MonitorTransport::messagesAvailable );
          QSignalSpy disconnected( transport.get(), &MonitorTransport::disconnected );

          QString error;
          QVERIFY( transport->open( QStringLiteral( "127.0.0.1" ), server.serverPort(), &error ) );
          QCOMPARE( transport->peerName(),
                    QStringLiteral( "ws://127.0.0.1:%1/monitor" ).arg( server.serverPort() ) );
          QTRY_VERIFY( server.hasPendingConnections() );
          std::unique_ptr< QWebSocket > peer( server.nextPendingConnection() );
          QCOMPARE( peer->requestUrl().path(), QStringLiteral( "/monitor" ) );
          QCOMPARE( peer->origin(), QString() ); // so that ws_allowed_origins lets it in
          QTRY_COMPARE( connected.count(), 1 );
          QVERIFY( transport->isOpen() );

          // to the server: a text message without a NUL
          QSignalSpy text( peer.get(), &QWebSocket::textMessageReceived );
          QVERIFY( transport->sendMessage( "(dispinit version 5)" ) );
          QTRY_COMPARE( text.count(), 1 );
          QCOMPARE( text.at( 0 ).at( 0 ).toString(), QStringLiteral( "(dispinit version 5)" ) );

          // from the server: a message each
          peer->sendTextMessage( QStringLiteral( "(a)" ) );
          peer->sendTextMessage( QStringLiteral( "(b)" ) );
          // (one notice each; QTRY_* evaluates the expression once more
          // after the wait, so it does not pop)
          QTRY_COMPARE( available.count(), 2 );
          QByteArray msg;
          QVERIFY( transport->popMessage( msg ) );
          QCOMPARE( msg, QByteArray( "(a)" ) );
          QVERIFY( transport->popMessage( msg ) );
          QCOMPARE( msg, QByteArray( "(b)" ) );
          QVERIFY( ! transport->popMessage( msg ) );

          // the server closes the connection: told once
          peer->close();
          QTRY_COMPARE( disconnected.count(), 1 );
          QVERIFY( ! disconnected.at( 0 ).at( 0 ).toString().isEmpty() );
          QVERIFY( ! transport->isOpen() );
          QTest::qWait( 100 );
          QCOMPARE( disconnected.count(), 1 );
      }

    /*!
      WebSocket: a connection refused (no server) is told, with the reason.
     */
    void webSocketRefused()
      {
          std::unique_ptr< MonitorTransport > transport = create( MonitorTransport::WebSocket );
          QVERIFY( transport );
          QSignalSpy connected( transport.get(), &MonitorTransport::connected );
          QSignalSpy disconnected( transport.get(), &MonitorTransport::disconnected );
          QString error;
          QVERIFY( transport->open( QStringLiteral( "127.0.0.1" ), closed_port(), &error ) );
          QTRY_COMPARE_WITH_TIMEOUT( disconnected.count(), 1, 5000 );
          QVERIFY( ! disconnected.at( 0 ).at( 0 ).toString().isEmpty() );
          QCOMPARE( connected.count(), 0 );
          QVERIFY( ! transport->isOpen() );
      }
#else
    //! without Qt WebSockets, the WebSocket transport is not made, with the reason
    void noWebSocket()
      {
          QString error;
          std::unique_ptr< MonitorTransport > transport( create_monitor_transport( MonitorTransport::WebSocket,
                                                                                   QStringLiteral( "/monitor" ),
                                                                                   nullptr,
                                                                                   &error ) );
          QVERIFY( ! transport );
          QVERIFY( ! error.isEmpty() );
      }
#endif
};

QTEST_GUILESS_MAIN( TestMonitorTransport )

#include "test_monitor_transport.moc"
