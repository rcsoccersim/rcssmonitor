// -*-c++-*-

/*!
  \file udp_monitor_transport.cpp
  \brief UDP transport for the monitor client Source File.
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

#include "udp_monitor_transport.h"

#include <QHostInfo>
#include <QTimer>
#include <QUdpSocket>
#include <QUrl>

#include <iostream>

/*-------------------------------------------------------------------*/
/*!

*/
UdpMonitorTransport::UdpMonitorTransport( QObject * parent )
    : MonitorTransport( parent )
    , M_socket( new QUdpSocket( this ) )
    , M_server_port( 0 )
{
    // setReadBufferSize() makes no effect for QUdpSocet...
    // M_socket->setReadBufferSize( 8192 * 256 );

    connect( M_socket, SIGNAL( readyRead() ),
             this, SIGNAL( messagesAvailable() ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
UdpMonitorTransport::~UdpMonitorTransport()
{

}

/*-------------------------------------------------------------------*/
/*!

*/
bool
UdpMonitorTransport::open( const QString & host,
                           quint16 port,
                           QString * error )
{
    QHostInfo info = QHostInfo::fromName( host );

    if ( info.error() != QHostInfo::NoError
         || info.addresses().isEmpty() )
    {
        if ( error )
        {
            *error = info.errorString();
        }
        return false;
    }

    M_server_addr = info.addresses().front();
    M_server_port = port;

    // INADDR_ANY, bind random created port to local
    if ( ! M_socket->bind( 0 ) )
    {
        if ( error )
        {
            *error = M_socket->errorString();
        }
        return false;
    }

    QUrl url;
    url.setScheme( QStringLiteral( "udp" ) );
    url.setHost( host );
    url.setPort( port );
    M_peer_name = url.toString();

    // UDP has no connection setup.
    // connected() is emitted from the event loop as with the other transports.
    QTimer::singleShot( 0, this,
                        [this]()
                          {
                              if ( isOpen() )
                              {
                                  emit connected();
                              }
                          } );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
UdpMonitorTransport::close()
{
    M_socket->close();
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
UdpMonitorTransport::isOpen() const
{
    return M_socket->state() == QAbstractSocket::BoundState;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
UdpMonitorTransport::sendMessage( const QByteArray & msg )
{
    if ( ! isOpen() )
    {
        return false;
    }

    // send the message including the terminating NUL.
    // QByteArray guarantees that constData() is NUL terminated.
    const qint64 len = msg.size() + 1;
    return M_socket->writeDatagram( msg.constData(), len,
                                    M_server_addr,
                                    M_server_port ) == len;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
UdpMonitorTransport::popMessage( QByteArray & msg )
{
    while ( M_socket->hasPendingDatagrams() )
    {
        const qint64 size = M_socket->pendingDatagramSize();
        msg.resize( size > 0 ? static_cast< int >( size ) : 0 );

        quint16 from_port = 0;
        const qint64 n = M_socket->readDatagram( msg.data(),
                                                 msg.size(),
                                                 nullptr, // QHostAddress*
                                                 &from_port );
        if ( n < 0 )
        {
            break;
        }

        if ( n == 0 )
        {
            // empty datagram
            continue;
        }

        msg.resize( static_cast< int >( n ) );

        if ( from_port != M_server_port )
        {
            std::cerr << "updated server port number = "
                      << from_port << std::endl;

            M_server_port = from_port;
        }

        return true;
    }

    msg.clear();
    return false;
}
