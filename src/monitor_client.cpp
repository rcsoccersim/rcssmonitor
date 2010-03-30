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

#include <QtNetwork>

#include "monitor_client.h"

#include "disp_holder.h"
#include "options.h"

#include <sstream>
#include <iostream>
#include <cassert>


namespace {
const int POLL_INTERVAL_MS = 1000;
}

/*-------------------------------------------------------------------*/
/*!

*/
MonitorClient::MonitorClient( QObject * parent,
                              DispHolder & disp_holder,
                              const char * hostname,
                              const int port,
                              const int version )

    : QObject( parent )
    , M_disp_holder( disp_holder )
    , M_server_port( static_cast< quint16 >( port ) )
    , M_socket( new QUdpSocket( this ) )
    , M_timer( new QTimer( this ) )
    , M_version( version )
    , M_waited_msec( 0 )
{
    assert( parent );

    // check protocl versin range
    if ( version < 1 )
    {
        M_version = 1;
    }

    if ( 4 < version )
    {
        M_version = 4;
    }

    QHostInfo host = QHostInfo::fromName( QString::fromAscii( hostname ) );

    if ( host.error() != QHostInfo::NoError )
    {
        qDebug() << "Error " << host.errorString();
        return;
    }

    M_server_addr = host.addresses().front();

    // INADDR_ANY, bind random created port to local
    if ( ! M_socket->bind( 0 ) )
    {
        std::cerr << "MonitorClient. failed to bind the socket."
                  << std::endl;
        return;
    }

    if ( ! isConnected() )
    {
        std::cerr << "MonitorClient. failed to initialize the socket."
                  << std::endl;
        return;
    }

    // setReadBufferSize() makes no effect for QUdpSocet...
    // M_socket->setReadBufferSize( 8192 * 256 );

    connect( M_socket, SIGNAL( readyRead() ),
             this, SLOT( handleReceive() ) );

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
void
MonitorClient::disconnect()
{
    if ( M_timer )
    {
        M_timer->stop();
    }

    if ( isConnected() )
    {
        sendDispBye();
        M_socket->close();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
MonitorClient::isConnected() const
{
    return ( M_socket->socketDescriptor() != -1 );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MonitorClient::handleReceive()
{
    int receive_count = 0;

    if ( M_version >= 3 )
    {
        char buf[8192];
        while ( M_socket->hasPendingDatagrams() )
        {
            quint16 from_port;
            int n = M_socket->readDatagram( buf,
                                            8192,
                                            0, // QHostAddress*
                                            &from_port );
            if ( n > 0 )
            {
                buf[n] = '\0';
                if ( ! M_disp_holder.addDispInfoV3( buf ) )
                {
                    std::cerr << "recv: " << buf << std::endl;
                }

                if ( from_port != M_server_port )
                {
                    std::cerr << "updated server port number = "
                              << from_port
                        //<< "  localPort = "
                        //<< M_socket->localPort()
                              << std::endl;

                    M_server_port = from_port;
                }
            }
            ++receive_count;
        }
    }
    else if ( M_version == 2 )
    {
        rcss::rcg::dispinfo_t2 disp2;
        while ( M_socket->hasPendingDatagrams() )
        {
            quint16 from_port;
            int n = M_socket->readDatagram( reinterpret_cast< char * >( &disp2 ),
                                            sizeof( disp2 ),
                                            0, // QHostAddress*
                                            &from_port );
            if ( n > 0 )
            {
                if ( ! M_disp_holder.addDispInfoV2( disp2 ) )
                {
                    std::cerr << "recv: "
                              << reinterpret_cast< char * >( &disp2 )
                              << std::endl;
                }

                if ( from_port != M_server_port )
                {
                    std::cerr << "updated server port number = "
                              << from_port
                        //<< "  localPort = "
                        //<< M_socket->localPort()
                              << std::endl;

                    M_server_port = from_port;
                }
            }
            ++receive_count;
        }
    }
    else if ( M_version == 1 )
    {
        rcss::rcg::dispinfo_t disp1;
        while ( M_socket->hasPendingDatagrams() )
        {
            quint16 from_port;
            int n =  M_socket->readDatagram( reinterpret_cast< char * >( &disp1 ),
                                             sizeof( disp1 ),
                                             0, // QHostAddress*
                                             &from_port );
            if ( n > 0 )
            {
                if ( ! M_disp_holder.addDispInfoV1( disp1 ) )
                {
                    std::cerr << "recv: "
                              << reinterpret_cast< char * >( &disp1 )
                              << std::endl;
                }

                if ( from_port != M_server_port )
                {
                    std::cerr << "updated port number = "
                              << from_port << std::endl;

                    M_server_port = from_port;
                }
            }
            ++receive_count;
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
void
MonitorClient::handleTimer()
{
    M_waited_msec += POLL_INTERVAL_MS;

    //std::cerr << "handleTimer waited = " << M_waited_msec << std::endl;

    if ( Options::instance().bufferingMode() )
    {
        //std::cerr << "disp current index=" << M_disp_holder.currentIndex() << '\n'
        //          << "     container size=" << M_disp_holder.dispCont().size() << std::endl;
        DispConstPtr disp = M_disp_holder.currentDisp();
        if ( M_disp_holder.dispCont().empty()
             || ( disp && disp->pmode_ == rcss::rcg::PM_TimeOver )
             || M_disp_holder.currentIndex() >= M_disp_holder.dispCont().size() - 2 )
        {

        }
        else
        {
            //std::cerr << "now, playing buffered data" << std::endl;
            M_waited_msec -= POLL_INTERVAL_MS;
        }
    }

    if ( Options::instance().autoReconnectMode() )
    {
        if ( M_waited_msec >= Options::instance().autoReconnectWait() * 1000 )
        {
            //std::cerr << "MonitorClient::handleTimer() waited=" << M_waited_msec << "[ms]"
            //          << " emit reconnectRequested()" << std::endl;
            emit reconnectRequested();
        }
    }
    else if ( M_waited_msec >= 5 * 1000 )
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

    M_socket->writeDatagram( com.c_str(), com.length() + 1,
                             M_server_addr,
                             M_server_port );
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

    if ( M_timer )
    {
        M_timer->start( POLL_INTERVAL_MS );
    }
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
