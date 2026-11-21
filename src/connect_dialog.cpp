// -*-c++-*-

/*!
  \file connect_dialog.cpp
  \brief dialog to input the server address and the transport Source File.
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

#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include "connect_dialog.h"

namespace {

//! check if the transport is available in this build.
bool
is_available( const MonitorTransport::Type type )
{
#ifdef HAVE_QT_WEBSOCKETS
    Q_UNUSED( type );
    return true;
#else
    return type != MonitorTransport::WebSocket;
#endif
}

}

/*-------------------------------------------------------------------*/
/*!

*/
ConnectDialog::ConnectDialog( QWidget * parent,
                              const QString & host,
                              const MonitorTransport::Type transport,
                              const int port,
                              const QString & ws_path )
    : QDialog( parent )
    , M_host( new QLineEdit( host ) )
    , M_transport( new QComboBox() )
    , M_port( new QSpinBox() )
    , M_ws_path( new QLineEdit( ws_path ) )
    , M_buttons( new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel ) )
{
    this->setWindowTitle( tr( "Connect to the server" ) );

    M_host->setMinimumWidth( 240 );
    M_host->selectAll();

    M_transport->addItem( tr( "UDP" ), static_cast< int >( MonitorTransport::UDP ) );
    M_transport->addItem( tr( "TCP" ), static_cast< int >( MonitorTransport::TCP ) );
    if ( is_available( MonitorTransport::WebSocket ) )
    {
        M_transport->addItem( tr( "WebSocket" ), static_cast< int >( MonitorTransport::WebSocket ) );
    }
    else
    {
        // show the item to tell the user, but it cannot be selected.
        M_transport->addItem( tr( "WebSocket (not available in this build)" ),
                              static_cast< int >( MonitorTransport::WebSocket ) );
        QStandardItemModel * model = qobject_cast< QStandardItemModel * >( M_transport->model() );
        if ( model )
        {
            model->item( M_transport->count() - 1 )->setEnabled( false );
        }
    }

    const int index = M_transport->findData( static_cast< int >( transport ) );
    M_transport->setCurrentIndex( index >= 0 ? index : 0 );

    M_port->setRange( 1, 65535 );
    M_port->setValue( port );
    M_port->setToolTip( tr( "server::port for UDP, server::tcp_port for TCP,"
                            " or server::ws_port for WebSocket" ) );

    M_ws_path->setToolTip( tr( "/monitor or /player" ) );

    M_buttons->button( QDialogButtonBox::Ok )->setText( tr( "Connect" ) );

    QFormLayout * form = new QFormLayout();
    form->addRow( tr( "Host:" ), M_host );
    form->addRow( tr( "Transport:" ), M_transport );
    form->addRow( tr( "Port:" ), M_port );
    form->addRow( tr( "WebSocket path:" ), M_ws_path );

    QVBoxLayout * top_layout = new QVBoxLayout();
    top_layout->addLayout( form );
    top_layout->addWidget( M_buttons );
    this->setLayout( top_layout );

    connect( M_host, SIGNAL( textChanged( const QString & ) ),
             this, SLOT( updateState() ) );
    connect( M_transport, SIGNAL( currentIndexChanged( int ) ),
             this, SLOT( updateState() ) );
    connect( M_buttons, SIGNAL( accepted() ),
             this, SLOT( accept() ) );
    connect( M_buttons, SIGNAL( rejected() ),
             this, SLOT( reject() ) );

    updateState();
}

/*-------------------------------------------------------------------*/
/*!

*/
ConnectDialog::~ConnectDialog()
{

}

/*-------------------------------------------------------------------*/
/*!

*/
QString
ConnectDialog::host() const
{
    return M_host->text().trimmed();
}

/*-------------------------------------------------------------------*/
/*!

*/
MonitorTransport::Type
ConnectDialog::transport() const
{
    return static_cast< MonitorTransport::Type >( M_transport->currentData().toInt() );
}

/*-------------------------------------------------------------------*/
/*!

*/
int
ConnectDialog::port() const
{
    return M_port->value();
}

/*-------------------------------------------------------------------*/
/*!

*/
QString
ConnectDialog::wsPath() const
{
    return M_ws_path->text().trimmed();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConnectDialog::updateState()
{
    const MonitorTransport::Type type = transport();

    M_ws_path->setEnabled( type == MonitorTransport::WebSocket );

    M_buttons->button( QDialogButtonBox::Ok )->setEnabled( ! host().isEmpty()
                                                           && is_available( type ) );
}
