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
    case MonitorTransport::WebSocket:
    default:
        break;
    }

    if ( error )
    {
        *error = QObject::tr( "Unsupported transport type." );
    }
    return nullptr;
}
