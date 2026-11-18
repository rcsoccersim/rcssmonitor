// -*-c++-*-

/*!
  \file monitor_transport.h
  \brief abstract transport for the monitor client Header File.
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

#ifndef RCSSMONITOR_MONITOR_TRANSPORT_H
#define RCSSMONITOR_MONITOR_TRANSPORT_H

#include <QObject>
#include <QByteArray>
#include <QString>

/*!
  \class MonitorTransport
  \brief abstract interface that sends and receives one message at a time.

  The received message is stored in QByteArray, so its constData() can be used
  as a NUL terminated string.
*/
class MonitorTransport
    : public QObject {

    Q_OBJECT

public:

    enum Type {
        UDP,
        TCP,
        WebSocket,
    };

private:

    //! not used
    MonitorTransport() = delete;
    MonitorTransport( const MonitorTransport & ) = delete;
    MonitorTransport & operator=( const MonitorTransport & ) = delete;

public:

    explicit
    MonitorTransport( QObject * parent );

    ~MonitorTransport() override;

    virtual
    Type type() const = 0;

    /*!
      \brief start the connection.
      \param host host name or IP address of the server
      \param port port number of the server
      \param error the reason of the failure is stored, if not null
      \return false only if the failure is detected immediately (e.g. DNS lookup error)

      connected() is emitted when the connection is established.
     */
    virtual
    bool open( const QString & host,
               quint16 port,
               QString * error ) = 0;

    /*!
      \brief close the connection. the pending data are sent out as far as possible.
      disconnected() is not emitted by this method.
     */
    virtual
    void close() = 0;

    /*!
      \brief check if the connection is available.
      \return true after connected() until the connection is closed.
     */
    virtual
    bool isOpen() const = 0;

    /*!
      \brief check if the binary format (protocol v1/v2) can be carried.
     */
    virtual
    bool isBinarySafe() const = 0;

    /*!
      \brief send one message. the delimiter is appended by the implementation.
      \param msg message without delimiter
      \return true if the message is accepted.
     */
    virtual
    bool sendMessage( const QByteArray & msg ) = 0;

    /*!
      \brief get one received message.
      \param msg variable to store the message
      \return false if no message is available.
     */
    virtual
    bool popMessage( QByteArray & msg ) = 0;

    /*!
      \brief get the peer description for logs and status bar, e.g. "udp://127.0.0.1:6000"
     */
    virtual
    QString peerName() const = 0;

signals:

    //! emitted only once when the connection is established.
    void connected();

    //! emitted when some messages are ready to be got by popMessage().
    void messagesAvailable();

    //! emitted only once when the connection is lost. never emitted by close().
    void disconnected( const QString & reason );
};

/*!
  \brief get the transport type from its name.
  \param name "udp", "tcp", "ws" or "websocket". case insensitive.
  \param type variable to store the result
  \return false if the name is unknown.
 */
bool parse_monitor_transport_type( const QString & name,
                                   MonitorTransport::Type * type );

/*!
  \brief get the name of the transport type.
  \return "udp", "tcp" or "ws"
 */
QString monitor_transport_type_name( const MonitorTransport::Type type );

/*!
  \brief create the transport instance.
  \param type transport type
  \param ws_path request path used by the WebSocket transport
  \param parent parent object
  \param error the reason of the failure is stored, if not null
  \return created instance, or nullptr if the type is not available.
 */
MonitorTransport * create_monitor_transport( const MonitorTransport::Type type,
                                             const QString & ws_path,
                                             QObject * parent,
                                             QString * error );

#endif
