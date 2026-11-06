// -*-c++-*-

/*!
  \file udp_monitor_transport.h
  \brief UDP transport for the monitor client Header File.
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

#ifndef RCSSMONITOR_UDP_MONITOR_TRANSPORT_H
#define RCSSMONITOR_UDP_MONITOR_TRANSPORT_H

#include "monitor_transport.h"

#include <QHostAddress>

class QUdpSocket;

/*!
  \class UdpMonitorTransport
  \brief UDP transport. one datagram is one message.

  The server replies from the port dedicated to this monitor.
  The destination port is updated to the source port of the received datagram.
*/
class UdpMonitorTransport
    : public MonitorTransport {

    Q_OBJECT

private:

    QUdpSocket * M_socket;

    QHostAddress M_server_addr;
    quint16 M_server_port;

    QString M_peer_name;

public:

    explicit
    UdpMonitorTransport( QObject * parent );

    ~UdpMonitorTransport() override;

    Type type() const override
      {
          return UDP;
      }

    bool open( const QString & host,
               quint16 port,
               QString * error ) override;

    void close() override;

    bool isOpen() const override;

    bool isBinarySafe() const override
      {
          return true;
      }

    bool sendMessage( const QByteArray & msg ) override;

    bool popMessage( QByteArray & msg ) override;

    QString peerName() const override
      {
          return M_peer_name;
      }
};

#endif
