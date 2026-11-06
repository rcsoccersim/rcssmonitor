// -*-c++-*-

/*!
  \file tcp_monitor_transport.cpp
  \brief TCP transport for the monitor client Source File.
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

#include "tcp_monitor_transport.h"

#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

namespace {
//! the maximum length of an incomplete message
const int MAX_PARTIAL_SIZE = 1024 * 1024;
}

/*-------------------------------------------------------------------*/
/*!

*/
TcpMonitorTransport::TcpMonitorTransport( QObject * parent )
    : MonitorTransport( parent )
    , M_socket( new QTcpSocket( this ) )
    , M_splitter( MAX_PARTIAL_SIZE )
    , M_open( false )
    , M_closed( false )
    , M_disconnect_notified( false )
{
    connect( M_socket, SIGNAL( connected() ),
             this, SLOT( handleConnected() ) );
    connect( M_socket, SIGNAL( readyRead() ),
             this, SLOT( handleReadyRead() ) );
    connect( M_socket, SIGNAL( disconnected() ),
             this, SLOT( handleDisconnected() ) );
    // errorOccurred() is not available before Qt 5.15
    connect( M_socket, SIGNAL( error( QAbstractSocket::SocketError ) ),
             this, SLOT( handleError( QAbstractSocket::SocketError ) ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
TcpMonitorTransport::~TcpMonitorTransport()
{

}

/*-------------------------------------------------------------------*/
/*!

*/
bool
TcpMonitorTransport::open( const QString & host,
                           quint16 port,
                           QString * /*error*/ )
{
    QUrl url;
    url.setScheme( QStringLiteral( "tcp" ) );
    url.setHost( host );
    url.setPort( port );
    M_peer_name = url.toString();

    // the host name is resolved asynchronously.
    // the failure is notified by disconnected().
    M_socket->connectToHost( host, port );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
TcpMonitorTransport::close()
{
    M_closed = true;
    M_open = false;

    if ( M_socket->state() == QAbstractSocket::ConnectedState )
    {
        // write the pending data (e.g. dispbye) to the socket now,
        // because this object may be deleted soon by deleteLater().
        M_socket->flush();
        M_socket->disconnectFromHost();
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
TcpMonitorTransport::isOpen() const
{
    return M_open
        && M_socket->state() == QAbstractSocket::ConnectedState;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
TcpMonitorTransport::sendMessage( const QByteArray & msg )
{
    if ( ! isOpen() )
    {
        return false;
    }

    // send the message including the terminating NUL.
    // QByteArray guarantees that constData() is NUL terminated.
    const qint64 len = msg.size() + 1;
    return M_socket->write( msg.constData(), len ) == len;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
TcpMonitorTransport::popMessage( QByteArray & msg )
{
    return M_splitter.pop( msg );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
TcpMonitorTransport::notifyDisconnected( const QString & reason )
{
    if ( M_closed
         || M_disconnect_notified )
    {
        return;
    }

    M_disconnect_notified = true;
    M_open = false;

    // emit the signal from the event loop, not from the signal handler of QTcpSocket,
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
TcpMonitorTransport::handleConnected()
{
    if ( M_closed
         || M_disconnect_notified )
    {
        return;
    }

    M_socket->setSocketOption( QAbstractSocket::LowDelayOption, 1 );
    M_open = true;

    emit connected();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
TcpMonitorTransport::handleReadyRead()
{
    if ( M_closed
         || M_disconnect_notified )
    {
        return;
    }

    if ( ! M_splitter.append( M_socket->readAll() ) )
    {
        M_splitter.clear();
        notifyDisconnected( tr( "too long message" ) );
        M_socket->abort();
        return;
    }

    if ( M_splitter.hasMessage() )
    {
        emit messagesAvailable();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
TcpMonitorTransport::handleDisconnected()
{
    notifyDisconnected( tr( "The connection was closed by the server." ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
TcpMonitorTransport::handleError( QAbstractSocket::SocketError )
{
    notifyDisconnected( M_socket->errorString() );
}
