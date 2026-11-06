// -*-c++-*-

/*!
  \file monitor_transport.cpp
  \brief abstract transport for the monitor client Source File.
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

#include "monitor_transport.h"

#include "tcp_monitor_transport.h"
#include "udp_monitor_transport.h"

/*-------------------------------------------------------------------*/
/*!

*/
MonitorTransport::MonitorTransport( QObject * parent )
    : QObject( parent )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
MonitorTransport::~MonitorTransport()
{

}

/*-------------------------------------------------------------------*/
/*!

*/
bool
parse_monitor_transport_type( const QString & name,
                              MonitorTransport::Type * type )
{
    const QString lower = name.trimmed().toLower();

    MonitorTransport::Type result;
    if ( lower == QLatin1String( "udp" ) )
    {
        result = MonitorTransport::UDP;
    }
    else if ( lower == QLatin1String( "tcp" ) )
    {
        result = MonitorTransport::TCP;
    }
    else if ( lower == QLatin1String( "ws" )
              || lower == QLatin1String( "websocket" ) )
    {
        result = MonitorTransport::WebSocket;
    }
    else
    {
        return false;
    }

    if ( type )
    {
        *type = result;
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
QString
monitor_transport_type_name( const MonitorTransport::Type type )
{
    switch ( type ) {
    case MonitorTransport::UDP:
        return QStringLiteral( "udp" );
    case MonitorTransport::TCP:
        return QStringLiteral( "tcp" );
    case MonitorTransport::WebSocket:
        return QStringLiteral( "ws" );
    default:
        break;
    }

    return QString();
}

/*-------------------------------------------------------------------*/
/*!

*/
MonitorTransport *
create_monitor_transport( const MonitorTransport::Type type,
                          const QString & /*ws_path*/,
                          QObject * parent,
                          QString * error )
{
    switch ( type ) {
    case MonitorTransport::UDP:
        return new UdpMonitorTransport( parent );
    case MonitorTransport::TCP:
        return new TcpMonitorTransport( parent );
    case MonitorTransport::WebSocket:
        if ( error )
        {
            *error = QObject::tr( "The WebSocket transport is not available in this build." );
        }
        return nullptr;
    default:
        break;
    }

    if ( error )
    {
        *error = QObject::tr( "Unsupported transport type." );
    }
    return nullptr;
}
