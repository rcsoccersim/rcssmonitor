// -*-c++-*-

/*!
  \file monitor_client.h
  \brief Monitor Client class Header File.
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

#ifndef RCSSMONITOR_MONITOR_CLIENT_H
#define RCSSMONITOR_MONITOR_CLIENT_H

#include <QObject>
#include <QString>

#include <rcss/rcg/types.h>

class QByteArray;
class QTimer;
class DispHolder;
class MonitorTransport;

class MonitorClient
    : public QObject {

    Q_OBJECT

private:

    DispHolder & M_disp_holder;

    MonitorTransport * M_transport;
    QTimer * M_timer;

    int M_version; //!< protocol version

    int M_waited_msec;

    bool M_transport_connected; //!< true after the transport is connected
    bool M_first_message_received; //!< true after the first message from the server


    //! not used
    MonitorClient() = delete;
    MonitorClient( const MonitorClient & ) = delete;
    MonitorClient & operator=( const MonitorClient & ) = delete;
public:
    /*!
      \brief constructor
      \param parent parent object
      \param disp_holder data holder to store the received data
      \param transport transport instance. the ownership is moved to this object.
      \param version protocol version
     */
    MonitorClient( QObject * parent,
                   DispHolder & disp_holder,
                   MonitorTransport * transport,
                   const int version );

    ~MonitorClient() override;

    /*!
      \brief start the connection. dispinit is sent after the connection is established.
      \param host host name or IP address of the server
      \param port port number of the server
      \param error the reason of the failure is stored, if not null
      \return false only if the failure is detected immediately
     */
    bool start( const QString & host,
                quint16 port,
                QString * error );

    void disconnect();

    bool isConnected() const;

    //! peer description, e.g. "udp://127.0.0.1:6000"
    QString peerName() const;

private:

    void sendCommand( const std::string & com );

    void sendDispInit();
    void sendDispBye();

    bool handleMessage( const QByteArray & msg );
    void requestDisconnect( const QString & reason );

public:

    // monitor command

    void sendKickOff();
    void sendFreeKick( const double & x,
                       const double & y,
                       const rcss::rcg::Side side );
    void sendDropBall( const double & x,
                       const double & y )
      {
          sendFreeKick( x, y, rcss::rcg::NEUTRAL );
      }
    void sendFreeKickLeft( const double & x,
                           const double & y )
      {
          sendFreeKick( x, y, rcss::rcg::LEFT );
      }
    void sendFreeKickRight( const double & x,
                            const double & y )
      {
          sendFreeKick( x, y, rcss::rcg::RIGHT );
      }

    void sendMove( const rcss::rcg::Side side,
                   const int unum,
                   const double & x,
                   const double & y,
                   const double & angle );

    void sendDiscard( const rcss::rcg::Side side,
                      const int unum );

    void sendYellowCard( const rcss::rcg::Side side,
                         const int unum );
    void sendRedCard( const rcss::rcg::Side side,
                      const int unum );


    // trainer command

    void sendChangeMode( const rcss::rcg::PlayMode pmode );

    void sendTrainerMoveBall( const double & x,
                              const double & y );

    void sendTrainerMoveBall( const double & x,
                              const double & y,
                              const double & vx,
                              const double & vy );

    void sendTrainerMovePlayer( const std::string & team_name,
                                const int unum,
                                const double & x,
                                const double & y );
    void sendTrainerMovePlayer( const std::string & team_name,
                                const int unum,
                                const double & x,
                                const double & y,
                                const double & angle );
    void sendTrainerMovePlayer( const std::string & team_name,
                                const int unum,
                                const double & x,
                                const double & y,
                                const double & angle,
                                const double & vx,
                                const double & vy );


    void sendRecover();

    void sencChangePlayerType( const std::string & team_name,
                               const int unum,
                               const int type );

    void sendCheckBal();

private slots:

    void handleConnected();
    void handleMessages();
    void handleDisconnected( const QString & reason );
    void handleTimer();

signals:

    void received();
    void disconnectRequested();
    void reconnectRequested();

    //! error message to be shown in the status bar
    void errorMessage( const QString & msg );

};

#endif
