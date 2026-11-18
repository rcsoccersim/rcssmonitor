// -*-c++-*-

/*!
  \file monitor_client.cpp
  \brief Monitor Client class Source File.
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

#include <QByteArray>
#include <QTimer>

#include "monitor_client.h"

#include "monitor_transport.h"
#include "disp_holder.h"
#include "options.h"

#include <algorithm>
#include <sstream>
#include <iostream>
#include <cassert>
#include <cmath>
#include <cstring>


namespace {
const int POLL_INTERVAL_MS = 1000;
}

/*-------------------------------------------------------------------*/
/*!

*/
MonitorClient::MonitorClient( QObject * parent,
                              DispHolder & disp_holder,
                              MonitorTransport * transport,
                              const int version )

    : QObject( parent )
    , M_disp_holder( disp_holder )
    , M_transport( transport )
    , M_timer( new QTimer( this ) )
    , M_version( version )
    , M_waited_msec( 0 )
    , M_transport_connected( false )
    , M_first_message_received( false )
{
    assert( parent );
    assert( transport );

    M_transport->setParent( this );

    // check protocol versin range
    if ( version == -1 )
    {
        M_version = -1;
    }
    else if ( version < 1 )
    {
        M_version = 1;
    }
    else if ( 5 < version )
    {
        M_version = 5;
    }

    connect( M_transport, SIGNAL( connected() ),
             this, SLOT( handleConnected() ) );
    connect( M_transport, SIGNAL( messagesAvailable() ),
             this, SLOT( handleMessages() ) );
    connect( M_transport, SIGNAL( disconnected( const QString & ) ),
             this, SLOT( handleDisconnected( const QString & ) ) );

    connect( M_timer, SIGNAL( timeout() ),
             this, SLOT( handleTimer() ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
MonitorClient::~MonitorClient()
{
    disconnect();

    //std::cerr << "delete MonitorClient" << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
MonitorClient::start( const QString & host,
                      quint16 port,
                      QString * error )
{
    // the server replies text messages to v1/v2 over TCP/WebSocket.
    // they must not be read as the binary structures.
    if ( ( M_version == 1 || M_version == 2 )
         && ! M_transport->isBinarySafe() )
    {
        if ( error )
        {
            *error = tr( "The protocol version %1 is not available over %2."
                         " Use version 3 or later, or -1 (JSON)." )
                .arg( M_version )
                .arg( monitor_transport_type_name( M_transport->type() ) );
        }
        return false;
    }

    if ( ! M_transport->open( host, port, error ) )
    {
        return false;
    }

    // the timer also limits the wait time for the connection setup.
    M_waited_msec = 0;
    M_timer->start( POLL_INTERVAL_MS );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::disconnect()
{
    M_timer->stop();

    // no more notification from the transport
    M_transport->disconnect( this );

    if ( isConnected() )
    {
        sendDispBye();
    }

    M_transport->close();
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
MonitorClient::isConnected() const
{
    return M_transport->isOpen();
}

/*-------------------------------------------------------------------*/
/*!

*/
QString
MonitorClient::peerName() const
{
    return M_transport->peerName();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::handleConnected()
{
    M_transport_connected = true;
    sendDispInit();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::handleMessages()
{
    int receive_count = 0;

    QByteArray msg;
    while ( M_transport->popMessage( msg ) )
    {
        ++receive_count;

        if ( ! handleMessage( msg ) )
        {
            // disconnection has been requested.
            return;
        }
    }

    if ( receive_count > 0 )
    {
        M_waited_msec = 0;
        M_timer->start( POLL_INTERVAL_MS );

        emit received();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
MonitorClient::handleMessage( const QByteArray & msg )
{
    if ( ! M_first_message_received )
    {
        M_first_message_received = true;

        // the server replies an error message instead of the initial data.
        // e.g. (error no_more_monitor)
        if ( msg.startsWith( "(error" ) )
        {
            requestDisconnect( tr( "Connection refused by the server: %1" )
                               .arg( QString::fromLatin1( msg.constData() ) ) );
            return false;
        }
    }

    if ( M_version == -1 ) // JSON
    {
        if ( ! M_disp_holder.addJSON( msg.constData() ) )
        {
            std::cerr << "recv: " << msg.constData() << std::endl;
        }
    }
    else if ( M_version >= 3 ) // S-Expression
    {
        if ( ! M_disp_holder.addDispInfoV3( msg.constData() ) )
        {
            std::cerr << "recv: " << msg.constData() << std::endl;
        }
    }
    else if ( M_version == 2 ) // binary format v2
    {
        rcss::rcg::dispinfo_t2 disp2;
        std::memset( &disp2, 0, sizeof( disp2 ) );
        std::memcpy( &disp2, msg.constData(),
                     std::min( static_cast< size_t >( msg.size() ), sizeof( disp2 ) ) );

        if ( ! M_disp_holder.addDispInfoV2( disp2 ) )
        {
            std::cerr << "recv: " << msg.constData() << std::endl;
        }
    }
    else if ( M_version == 1 ) // binary format v1
    {
        rcss::rcg::dispinfo_t disp1;
        std::memset( &disp1, 0, sizeof( disp1 ) );
        std::memcpy( &disp1, msg.constData(),
                     std::min( static_cast< size_t >( msg.size() ), sizeof( disp1 ) ) );

        if ( ! M_disp_holder.addDispInfoV1( disp1 ) )
        {
            std::cerr << "recv: " << msg.constData() << std::endl;
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::handleDisconnected( const QString & reason )
{
    if ( M_transport_connected )
    {
        requestDisconnect( tr( "Disconnected from the server: %1" ).arg( reason ) );
    }
    else
    {
        requestDisconnect( tr( "Failed to connect to the server: %1" ).arg( reason ) );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::handleTimer()
{
    M_waited_msec += POLL_INTERVAL_MS;

    //std::cerr << "handleTimer waited = " << M_waited_msec << std::endl;

    if ( Options::instance().autoReconnectMode() )
    {
        if ( M_waited_msec >= Options::instance().autoReconnectWait() * 1000 )
        {
            //std::cerr << "MonitorClient::handleTimer() waited=" << M_waited_msec << "[ms]"
            //          << " emit reconnectRequested()" << std::endl;
            requestDisconnect( tr( "No response from the server." ) );
        }
    }
    else if ( M_waited_msec >= 5 * 1000 )
    {
        requestDisconnect( tr( "No response from the server." ) );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::requestDisconnect( const QString & reason )
{
    M_timer->stop();

    std::cerr << reason.toStdString() << std::endl;
    emit errorMessage( reason );

    if ( Options::instance().autoReconnectMode() )
    {
        emit reconnectRequested();
    }
    else
    {
        emit disconnectRequested();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendCommand( const std::string & com )
{
    if ( ! isConnected() )
    {
        return;
    }

    if ( ! M_transport->sendMessage( QByteArray::fromStdString( com ) ) )
    {
        std::cerr << "failed to send: " << com << std::endl;
        return;
    }

    std::cerr << "send: " << com << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendDispInit()
{
    std::ostringstream ostr;
    if ( M_version )
    {
        ostr << "(dispinit version " << M_version << ")";
    }
    else
    {
        ostr << "(dispinit)";
    }

    sendCommand( ostr.str() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendDispBye()
{
    sendCommand( "(dispbye)" );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendKickOff()
{
    sendCommand( "(dispstart)" );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendFreeKick( const double & x,
                             const double & y,
                             const rcss::rcg::Side side )
{
    std::ostringstream ostr;
    ostr << "(dispfoul "
         << static_cast< int >( rint( x * rcss::rcg::SHOWINFO_SCALE ) ) << " "
         << static_cast< int >( rint( y * rcss::rcg::SHOWINFO_SCALE ) ) << " "
         << side << ")";

    sendCommand( ostr.str() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendMove( const rcss::rcg::Side side,
                         const int unum,
                         const double & x,
                         const double & y,
                         const double & angle )
{
    if ( side == rcss::rcg::NEUTRAL )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " dispplayer invalid side" << std::endl;
        return;
    }

    if ( unum < 1 || 11 < unum )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " dispplayer invalid unum " << unum << std::endl;
        return;
    }

    std::ostringstream ostr;
    ostr << "(dispplayer "
         << side << " "
         << unum << " "
         << static_cast< int >( rint( x * rcss::rcg::SHOWINFO_SCALE ) ) << " "
         << static_cast< int >( rint( y * rcss::rcg::SHOWINFO_SCALE ) ) << " "
         << static_cast< int >( rint( angle * rcss::rcg::SHOWINFO_SCALE ) ) << ")";

    sendCommand( ostr.str() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendDiscard( const rcss::rcg::Side side,
                            const int unum )
{
    if ( side == rcss::rcg::NEUTRAL )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " dispdiscard invalid side" << std::endl;
        return;
    }

    if ( unum < 1 || 11 < unum )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " dispdiscard invalid unum " << unum << std::endl;
        return;
    }

    std::ostringstream ostr;
    ostr << "(dispdiscard "
         << side << " "
         << unum << ")";

    sendCommand( ostr.str() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendYellowCard( const rcss::rcg::Side side,
                               const int unum )
{
    if ( side == rcss::rcg::NEUTRAL )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " dispcard invalid side" << std::endl;
        return;
    }

    if ( unum < 1 || 11 < unum )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " dispscard invalid unum " << unum << std::endl;
        return;
    }

    std::ostringstream ostr;
    ostr << "(dispcard " << side << ' ' << unum  << " yellow)";

    sendCommand( ostr.str() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendRedCard( const rcss::rcg::Side side,
                            const int unum )
{
    if ( side == rcss::rcg::NEUTRAL )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " dispcard invalid side" << std::endl;
        return;
    }

    if ( unum < 1 || 11 < unum )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " dispscard invalid unum " << unum << std::endl;
        return;
    }

    std::ostringstream ostr;
    ostr << "(dispcard " << side << ' ' << unum  << " red)";

    sendCommand( ostr.str() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendChangeMode( const rcss::rcg::PlayMode pmode )
{
    static const char * pmodes[] = PLAYMODE_STRINGS;

    std::ostringstream ostr;
    ostr << "(change_mode " << pmodes[pmode] << ")";

    sendCommand( ostr.str() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendTrainerMoveBall( const double & x,
                                    const double & y )
{
    std::ostringstream ostr;

    ostr << "(move (ball) " << x << " " << y << ")";

    sendCommand( ostr.str() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendTrainerMoveBall( const double & x,
                                    const double & y,
                                    const double & vx,
                                    const double & vy )
{
    std::ostringstream ostr;

    ostr << "(move (ball) "
         << x << " " << y
         << " 0 "
         << vx << " " << vy
         << ")";

    sendCommand( ostr.str() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendTrainerMovePlayer( const std::string & team_name,
                                      const int unum,
                                      const double & x,
                                      const double & y )
{
    std::ostringstream ostr;

    ostr << "(move (player " << team_name << " " << unum << ") "
         << x << " " << y
         << ")";

    sendCommand( ostr.str() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendTrainerMovePlayer( const std::string & team_name,
                                      const int unum,
                                      const double & x,
                                      const double & y,
                                      const double & angle )
{
    std::ostringstream ostr;

    ostr << "(move (player " << team_name << " " << unum << ") "
         << x << " " << y
         << " " << angle
         << ")";

    sendCommand( ostr.str() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendTrainerMovePlayer( const std::string & team_name,
                                      const int unum,
                                      const double & x,
                                      const double & y,
                                      const double & angle,
                                      const double & vx,
                                      const double & vy )
{
    std::ostringstream ostr;

    ostr << "(move (player " << team_name << " " << unum << ") "
         << x << " " << y
         << " " << angle
         << " " << vx << " " << vy
         << ")";

    sendCommand( ostr.str() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendRecover()
{
    sendCommand( "(recover)" );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sencChangePlayerType( const std::string & team_name,
                                     const int unum,
                                     const int type )
{
    std::ostringstream ostr;

    ostr << "(change_player_type "
         << team_name << " "
         << unum << " "
         << type << ")";

    sendCommand( ostr.str() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::sendCheckBal()
{
    sendCommand( "(check_ball)" );
}
