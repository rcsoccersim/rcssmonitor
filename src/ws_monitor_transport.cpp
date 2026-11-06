// -*-c++-*-

/*!
  \file ws_monitor_transport.cpp
  \brief WebSocket transport for the monitor client Source File.
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

#include "ws_monitor_transport.h"

#include <QTimer>
#include <QUrl>
#include <QWebSocket>

namespace {

QString
normalize_path( const QString & path )
{
    QString result = path.trimmed();
    if ( ! result.startsWith( QLatin1Char( '/' ) ) )
    {
        result.prepend( QLatin1Char( '/' ) );
    }
    return result;
}

}

/*-------------------------------------------------------------------*/
/*!

*/
WsMonitorTransport::WsMonitorTransport( const QString & path,
                                        QObject * parent )
    : MonitorTransport( parent )
      // No Origin header is sent if the origin is empty,
      // so that the server does not check the origin.
    , M_socket( new QWebSocket( QString(), QWebSocketProtocol::VersionLatest, this ) )
    , M_path( normalize_path( path ) )
    , M_open( false )
    , M_closed( false )
    , M_disconnect_notified( false )
{
    connect( M_socket, SIGNAL( connected() ),
             this, SLOT( handleConnected() ) );
    connect( M_socket, SIGNAL( textMessageReceived( const QString & ) ),
             this, SLOT( handleTextMessage( const QString & ) ) );
    connect( M_socket, SIGNAL( binaryMessageReceived( const QByteArray & ) ),
             this, SLOT( handleBinaryMessage( const QByteArray & ) ) );
    connect( M_socket, SIGNAL( disconnected() ),
             this, SLOT( handleDisconnected() ) );
    connect( M_socket, SIGNAL( error( QAbstractSocket::SocketError ) ),
             this, SLOT( handleError( QAbstractSocket::SocketError ) ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
WsMonitorTransport::~WsMonitorTransport()
{

}

/*-------------------------------------------------------------------*/
/*!

*/
bool
WsMonitorTransport::open( const QString & host,
                          quint16 port,
                          QString * error )
{
    QUrl url;
    url.setScheme( QStringLiteral( "ws" ) );
    url.setHost( host );
    url.setPort( port );
    url.setPath( M_path );

    if ( ! url.isValid() )
    {
        if ( error )
        {
            *error = tr( "Illegal server address: %1" ).arg( url.errorString() );
        }
        return false;
    }

    M_peer_name = url.toString();

    // the result is notified by connected() or disconnected().
    M_socket->open( url );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WsMonitorTransport::close()
{
    M_closed = true;
    M_open = false;

    if ( M_socket->state() == QAbstractSocket::ConnectedState )
    {
        // write the pending data (e.g. dispbye) to the socket now,
        // because this object may be deleted soon by deleteLater().
        M_socket->flush();
        M_socket->close();
    }
    else
    {
        M_socket->abort();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
WsMonitorTransport::isOpen() const
{
    return M_open
        && M_socket->state() == QAbstractSocket::ConnectedState;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
WsMonitorTransport::sendMessage( const QByteArray & msg )
{
    if ( ! isOpen() )
    {
        return false;
    }

    // one message is sent as one text message without the terminating NUL.
    return M_socket->sendTextMessage( QString::fromUtf8( msg ) ) == msg.size();
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
WsMonitorTransport::popMessage( QByteArray & msg )
{
    if ( M_messages.empty() )
    {
        msg.clear();
        return false;
    }

    msg = M_messages.front();
    M_messages.pop_front();
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WsMonitorTransport::receiveMessage( const QByteArray & msg )
{
    if ( M_closed
         || M_disconnect_notified )
    {
        return;
    }

    // the server does not append '\0', but remove it just in case.
    QByteArray body = msg;
    while ( body.endsWith( '\0' ) )
    {
        body.chop( 1 );
    }

    if ( body.isEmpty() )
    {
        return;
    }

    M_messages.push_back( body );
    emit messagesAvailable();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WsMonitorTransport::notifyDisconnected( const QString & reason )
{
    if ( M_closed
         || M_disconnect_notified )
    {
        return;
    }

    M_disconnect_notified = true;
    M_open = false;

    // emit the signal from the event loop, not from the signal handler of QWebSocket,
    // because the receiver may close this transport.
    QTimer::singleShot( 0, this,
                        [this, reason]()
                          {
                              if ( ! M_closed )
                              {
                                  emit disconnected( reason );
                              }
                          } );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WsMonitorTransport::handleConnected()
{
    if ( M_closed
         || M_disconnect_notified )
    {
        return;
    }

    M_open = true;

    emit connected();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WsMonitorTransport::handleTextMessage( const QString & msg )
{
    receiveMessage( msg.toUtf8() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WsMonitorTransport::handleBinaryMessage( const QByteArray & msg )
{
    receiveMessage( msg );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WsMonitorTransport::handleDisconnected()
{
    // QWebSocket emits disconnected() before error() in some cases,
    // e.g. when the connection is refused. use the error if it is already set.
    if ( M_socket->error() != QAbstractSocket::UnknownSocketError )
    {
        notifyDisconnected( M_socket->errorString() );
    }
    else
    {
        notifyDisconnected( tr( "The connection was closed by the server." ) );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WsMonitorTransport::handleError( QAbstractSocket::SocketError )
{
    notifyDisconnected( M_socket->errorString() );
}
