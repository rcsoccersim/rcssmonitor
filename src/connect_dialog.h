// -*-c++-*-

/*!
  \file connect_dialog.h
  \brief dialog to input the server address and the transport Header File.
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

#ifndef RCSSMONITOR_CONNECT_DIALOG_H
#define RCSSMONITOR_CONNECT_DIALOG_H

#include "monitor_transport.h"

#include <QDialog>

class QComboBox;
class QDialogButtonBox;
class QLineEdit;
class QSpinBox;

/*!
  \class ConnectDialog
  \brief dialog to input the server address and the transport.
*/
class ConnectDialog
    : public QDialog {

    Q_OBJECT

private:

    QLineEdit * M_host;
    QComboBox * M_transport;
    QSpinBox * M_port;
    QLineEdit * M_ws_path;
    QDialogButtonBox * M_buttons;

public:

    /*!
      \brief construct the dialog with the initial values.
      \param parent parent widget
      \param host host name or IP address of the server
      \param transport transport type
      \param port port number of the server
      \param ws_path request path used by the WebSocket transport
     */
    ConnectDialog( QWidget * parent,
                   const QString & host,
                   const MonitorTransport::Type transport,
                   const int port,
                   const QString & ws_path );

    ~ConnectDialog() override;

    //! get the host name without the leading and trailing spaces.
    QString host() const;

    //! get the selected transport type.
    MonitorTransport::Type transport() const;

    //! get the port number.
    int port() const;

    //! get the request path without the leading and trailing spaces.
    QString wsPath() const;

private slots:

    void updateState();

};

#endif
