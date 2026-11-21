// -*-c++-*-

/*!
  \file ws_monitor_transport.h
  \brief WebSocket transport for the monitor client Header File.
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

#ifndef RCSSMONITOR_WS_MONITOR_TRANSPORT_H
#define RCSSMONITOR_WS_MONITOR_TRANSPORT_H

#include "monitor_transport.h"

#include <QAbstractSocket>

#include <deque>

class QWebSocket;

/*!
  \class WsMonitorTransport
  \brief WebSocket transport. each message is carried by one text message.
*/
class WsMonitorTransport
    : public MonitorTransport {

    Q_OBJECT

private:

    QWebSocket * M_socket;
    const QString M_path; //!< request path, e.g. "/monitor"

    std::deque< QByteArray > M_messages; //!< received messages

    QString M_peer_name;

    bool M_open; //!< true after connected() until the connection is lost or closed
    bool M_closed; //!< true after close() is called
    bool M_disconnect_notified; //!< true after the disconnection is notified

public:

    /*!
      \brief constructor
      \param path request path. '/' is prepended if it is missing.
      \param parent parent object
     */
    WsMonitorTransport( const QString & path,
                        QObject * parent );

    ~WsMonitorTransport() override;

    Type type() const override
      {
          return WebSocket;
      }

    bool open( const QString & host,
               quint16 port,
               QString * error ) override;

    void close() override;

    bool isOpen() const override;

    bool isBinarySafe() const override
      {
          return false;
      }

    bool sendMessage( const QByteArray & msg ) override;

    bool popMessage( QByteArray & msg ) override;

    QString peerName() const override
      {
          return M_peer_name;
      }

private:

    void receiveMessage( const QByteArray & msg );

    void notifyDisconnected( const QString & reason );

private slots:

    void handleConnected();
    void handleTextMessage( const QString & msg );
    void handleBinaryMessage( const QByteArray & msg );
    void handleDisconnected();
    void handleError( QAbstractSocket::SocketError socket_error );

};

#endif
