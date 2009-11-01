// -*-c++-*-

/*!
  \file main_window.cpp
  \brief main application window class Source File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
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

#include <QtGui>

#include "main_window.h"

#include "field_canvas.h"
#include "monitor_client.h"
#include "options.h"

#include <string>
#include <iostream>
#include <cstring>

#include "icons/rcss.xpm"

// #ifndef PACKAGE_STRING
// #define PACKAGE_STRING "rcssmonitor x.x.x"
// #endif

/*-------------------------------------------------------------------*/
/*!

 */
MainWindow::MainWindow()
    : QMainWindow( /* parent, flags */ ),
      M_window_style( "plastique" ),
      M_monitor_client( static_cast< MonitorClient * >( 0 ) )
{
    this->setWindowIcon( QIcon( QPixmap( rcss_xpm ) ) );
    this->setWindowTitle( tr( PACKAGE_STRING ) );
    this->setMinimumSize( 280, 220 );

    readSettings();

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createFieldCanvas();

}

/*-------------------------------------------------------------------*/
/*!

 */
MainWindow::~MainWindow()
{
    writeSettings();
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::init()
{
    if ( Options::instance().connect() )
    {
        connectMonitor();
    }

    if ( Options::instance().fullScreen() )
    {
        this->showFullScreen();
    }
    else if ( Options::instance().maximize() )
    {
        this->showMaximized();
    }
    else if ( Options::instance().canvasWidth() > 0
              && Options::instance().canvasHeight() > 0 )
    {
        resizeCanvas( QSize( Options::instance().canvasWidth(),
                             Options::instance().canvasHeight() ) );
    }
    else
    {
        this->resize( 640, 480 );
    }

    if ( QApplication::setStyle( M_window_style ) ) // no style found
    {
        Q_FOREACH( QAction * action, M_style_act_group->actions() )
        {
            if ( action->data().toString().toLower()
                 == QApplication::style()->objectName().toLower() )
            {
                M_window_style = QApplication::style()->objectName().toLower();
                action->setChecked( true );
                break;
            }
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::readSettings()
{
    QSettings settings( Options::CONF_FILE,
                        QSettings::IniFormat );

    settings.beginGroup( "Global" );

    QVariant val;

    M_window_style = settings.value( "window_style", "plastique" ).toString();

    settings.endGroup();
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::writeSettings()
{
    QSettings settings( Options::CONF_FILE,
                        QSettings::IniFormat );

    settings.beginGroup( "Global" );

    settings.setValue( "window_style", M_window_style );

//     if ( ! Options::instance().minimumMode() )
//     {
//         settings.setValue( "window_width", this->width() );
//         settings.setValue( "window_height", this->height() );
//         settings.setValue( "window_x", this->pos().x() );
//         settings.setValue( "window_y", this->pos().y() );
//         settings.setValue( "maximize", this->isMaximized() );
//         settings.setValue( "full_screen", this->isFullScreen() );
//         settings.setValue( "hide_menu_bar", this->menuBar()->isHidden() );
//         settings.setValue( "hide_tool_bar", ( M_log_player_tool_bar->isHidden()
//                                               && M_log_slider_tool_bar->isHidden() ) );
//         settings.setValue( "hide_status_bar", this->statusBar()->isHidden() );
//     }

    settings.endGroup();
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createActions()
{
    createActionsFile();
    createActionsMonitor();
    createActionsReferee();
    createActionsView();
    createActionsHelp();

}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createActionsFile()
{
    //
    M_exit_act = new QAction( tr( "&Quit" ), this );
#ifdef Q_WS_MAC
    M_exit_act->setShortcut( Qt::META + Qt::Key_Q );
#else
    M_exit_act->setShortcut( Qt::CTRL + Qt::Key_Q );
#endif
    M_exit_act->setStatusTip( tr( "Exit the application." ) );
    connect( M_exit_act, SIGNAL( triggered() ),
             this, SLOT( close() ) );
    this->addAction( M_exit_act );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createActionsMonitor()
{
    //
    M_connect_monitor_act = new QAction( tr( "&Connect" ), this );
#ifdef Q_WS_MAC
    M_connect_monitor_act->setShortcut( Qt::META + Qt::Key_C );
#else
    M_connect_monitor_act->setShortcut( Qt::CTRL + Qt::Key_C );
#endif
    M_connect_monitor_act->setStatusTip( "Connect to the rcssserver on localhost" );
    M_connect_monitor_act->setEnabled( true );
    connect( M_connect_monitor_act, SIGNAL( triggered() ),
             this, SLOT( connectMonitor() ) );
    this->addAction( M_connect_monitor_act );
    //
    M_connect_monitor_to_act = new QAction( tr( "Connect &to ..." ), this );
    M_connect_monitor_to_act->setStatusTip( tr( "Connect to the rcssserver on the remote host." ) );
    M_connect_monitor_to_act->setEnabled( true );
    connect( M_connect_monitor_to_act, SIGNAL( triggered() ),
             this, SLOT( connectMonitorTo() ) );
    this->addAction( M_connect_monitor_to_act );
    //
    M_disconnect_monitor_act = new QAction( tr( "&Disconnect" ), this );
    M_disconnect_monitor_act->setStatusTip( tr( "Disonnect from rcssserver." ) );
    M_disconnect_monitor_act->setEnabled( false );
    connect( M_disconnect_monitor_act, SIGNAL( triggered() ),
             this, SLOT( disconnectMonitor() ) );
    this->addAction( M_disconnect_monitor_act );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createActionsReferee()
{
    //
    M_kick_off_act = new QAction( tr( "&KickOff" ), this );
#ifdef Q_WS_MAC
    M_kick_off_act->setShortcut( Qt::META + Qt::Key_K );
#else
    M_kick_off_act->setShortcut( Qt::CTRL + Qt::Key_K );
#endif
    M_kick_off_act->setStatusTip( tr( "Send kick-off command to the connected rcssserver." ) );
    M_kick_off_act->setEnabled( false );
    connect( M_kick_off_act, SIGNAL( triggered() ),
             this, SLOT( kickOff() ) );
    this->addAction( M_kick_off_act );
    //

}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createActionsView()
{
    // qt style menu group
    M_style_act_group = new QActionGroup( this );
    Q_FOREACH ( QString style_name, QStyleFactory::keys() )
    {
        QAction * subaction = new QAction( M_style_act_group );
        subaction->setText( style_name );
        subaction->setData( style_name );
        subaction->setCheckable( true );
        if ( style_name.toLower()
             == QApplication::style()->objectName().toLower() )
        {
            subaction->setChecked( true );
        }
        connect( subaction, SIGNAL( triggered( bool ) ),
                 this, SLOT( changeStyle( bool ) ) );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createActionsHelp()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createMenus()
{
    createMenuFile();
    createMenuMonitor();
    createMenuReferee();
    createMenuView();
    createMenuHelp();
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createMenuFile()
{
    //QMenu * menu = menuBar()->addMenu( tr( "&File" ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createMenuMonitor()
{
    QMenu * menu = menuBar()->addMenu( tr( "&Monitor" ) );

    menu->addAction( M_connect_monitor_act );
    menu->addAction( M_connect_monitor_to_act );
    menu->addAction( M_disconnect_monitor_act );

    menu->addSeparator();

    menu->addAction( M_exit_act );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createMenuReferee()
{
    QMenu * menu = menuBar()->addMenu( tr( "&Referee" ) );

    menu->addAction( M_kick_off_act );
//     menu->addAction( M_set_live_mode_act );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createMenuView()
{
    QMenu * menu = menuBar()->addMenu( tr( "&View" ) );

    {
        QMenu * submenu = menu->addMenu( tr( "Qt &Style" ) );
        Q_FOREACH ( QAction * action, M_style_act_group->actions() )
        {
            submenu->addAction( action );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createMenuHelp()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createToolBars()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createStatusBar()
{
    this->statusBar()->showMessage( tr( "Ready" ) );

    M_position_label = new QLabel( tr( "(0.0, 0.0)" ) );

    int min_width
        = M_position_label->fontMetrics().width(  tr( "(-60.0, -30.0)" ) )
        + 16;
    M_position_label->setMinimumWidth( min_width );
    M_position_label->setAlignment( Qt::AlignRight );

    this->statusBar()->addPermanentWidget( M_position_label );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createFieldCanvas()
{
    M_field_canvas = new FieldCanvas();
    this->setCentralWidget( M_field_canvas );

    M_field_canvas->setFocus();

    connect( this, SIGNAL( viewUpdated() ),
             M_field_canvas, SLOT( update() ) );

    connect( M_field_canvas, SIGNAL( mouseMoved( const QPoint & ) ),
             this, SLOT( updatePositionLabel( const QPoint & ) ) );

    connect( M_field_canvas, SIGNAL( dropBall( const QPoint & ) ),
             this, SLOT( dropBall( const QPoint & ) ) );
    connect( M_field_canvas, SIGNAL( freeKickLeft( const QPoint & ) ),
             this, SLOT( freeKickLeft( const QPoint & ) ) );
    connect( M_field_canvas, SIGNAL( freeKickRight( const QPoint & ) ),
             this, SLOT( freeKickRight( const QPoint & ) ) );

    // create & set context menus
    {
        QMenu * menu = new QMenu( M_field_canvas );
        menu->addAction( M_kick_off_act );
        menu->addSeparator();
        menu->addAction( tr( "Drop Ball" ),
                         M_field_canvas, SLOT( dropBall() ) );
        menu->addAction( tr( "Free Kick Left" ),
                         M_field_canvas, SLOT( freeKickLeft() ) );
        menu->addAction( tr( "Free Kick Right" ),
                         M_field_canvas, SLOT( freeKickRight() ) );

        M_field_canvas->setMonitorMenu( menu );
    }

}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::closeEvent( QCloseEvent * event )
{
    event->ignore();

    qApp->quit();
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::connectMonitorTo( const char * hostname )
{
    if ( std::strlen( hostname ) == 0 )
    {
        std::cerr << "Empty host name! Connection failed!" << std::endl;
        return;
    }

    std::cerr << "Connect to [" << hostname << "] ..." << std::endl;

    M_monitor_client = new MonitorClient( this,
                                          //M_main_data.dispHolder(),
                                          hostname,
                                          Options::instance().serverPort(),
                                          Options::instance().clientVersion() );

    if ( ! M_monitor_client->isConnected() )
    {
        std::cerr << "Conenction failed." << std::endl;
        delete M_monitor_client;
        M_monitor_client = static_cast< MonitorClient * >( 0 );
        return;
    }

    // reset all data
//     M_main_data.clear();

//     if ( M_player_type_dialog )
//     {
//         M_player_type_dialog->hide();
//     }

//     if ( M_config_dialog )
//     {
//         M_config_dialog->unzoom();
//     }

    Options::instance().setMonitorClientMode( true );
    Options::instance().setServerHost( hostname );

//     M_save_image_act->setEnabled( false );
//     M_open_output_act->setEnabled( true );

//     M_kick_off_act->setEnabled( true );
//     M_set_live_mode_act->setEnabled( true );
//     M_connect_monitor_act->setEnabled( false );
//     M_connect_monitor_to_act->setEnabled( false );
//     M_disconnect_monitor_act->setEnabled( true );

    connect( M_monitor_client, SIGNAL( received() ),
             this, SLOT( receiveMonitorPacket() ) );
//     connect( M_monitor_client, SIGNAL( timeout() ),
//              this, SLOT( disconnectMonitor() ) );

//     M_log_player->setLiveMode();

    M_monitor_client->sendDispInit();

    if ( QApplication::overrideCursor() )
    {
        QApplication::restoreOverrideCursor();
    }

}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::kickOff()
{
    if ( M_monitor_client
         && M_monitor_client->isConnected() )
    {
        M_monitor_client->sendKickOff();
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::connectMonitor()
{
    std::string host = Options::instance().serverHost();
    if ( host.empty() )
    {
        host = "127.0.0.1";
    }

    connectMonitorTo( host.c_str() );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::connectMonitorTo()
{
    std::string host = Options::instance().serverHost();
    if ( host.empty() )
    {
        host = "127.0.0.1";
    }

    bool ok = true;
    QString text = QInputDialog::getText( this,
                                          tr( "Input sserver host name" ),
                                          tr( "Host name: "),
                                          QLineEdit::Normal,
                                          QString::fromStdString( host ),
                                          & ok );
    if ( ok
         && ! text.isEmpty() )
    {
        connectMonitorTo( text.toStdString().c_str() );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::disconnectMonitor()
{
    //std::cerr << "MainWindow::disconnectMonitor()" << std::endl;
    if ( M_monitor_client )
    {
        M_monitor_client->disconnect();

        disconnect( M_monitor_client, SIGNAL( received() ),
                    this, SLOT( receiveMonitorPacket() ) );

        disconnect( M_monitor_client, SIGNAL( timeout() ),
                    this, SLOT( disconnectMonitor() ) );

        delete M_monitor_client;
        M_monitor_client = static_cast< MonitorClient * >( 0 );
    }

    Options::instance().setMonitorClientMode( false );

    M_kick_off_act->setEnabled( false );
//     M_set_live_mode_act->setEnabled( false );
    M_connect_monitor_act->setEnabled( true );
    M_connect_monitor_to_act->setEnabled( true );
    M_disconnect_monitor_act->setEnabled( false );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::changeStyle( bool checked )
{
    if ( ! checked )
    {
        return;
    }

    QAction * action = qobject_cast< QAction * >( sender() );
    QStyle * style = QStyleFactory::create( action->data().toString() );
    Q_ASSERT( style );

    QApplication::setStyle( style );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::dropBall( const QPoint & point )
{
    if ( M_monitor_client
         && M_monitor_client->isConnected() )
    {
        double x = Options::instance().fieldX( point.x() );
        double y = Options::instance().fieldY( point.y() );

        std::cerr << "drop ball to ("
                  << x << ", " << y << ")"
                  << std::endl;
        M_monitor_client->sendDropBall( x, y );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::freeKickLeft( const QPoint & point )
{
    if ( M_monitor_client
         && M_monitor_client->isConnected() )
    {
        double x = Options::instance().fieldX( point.x() );
        double y = Options::instance().fieldY( point.y() );

        std::cerr << "free kick left at ("
                  << x << ", " << y << ")"
                  << std::endl;
        M_monitor_client->sendFreeKickLeft( x, y );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::freeKickRight( const QPoint & point )
{
    if ( M_monitor_client
         && M_monitor_client->isConnected() )
    {
        double x = Options::instance().fieldX( point.x() );
        double y = Options::instance().fieldY( point.y() );

        std::cerr << "free kick right at ("
                  << x << ", " << y << ")"
                  << std::endl;
        M_monitor_client->sendFreeKickRight( x, y );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::yellowCard( const char side,
                        const int unum )
{
    if ( M_monitor_client
         && M_monitor_client->isConnected() )
    {
        std::cerr << "yellow_card : "
                  << side << ' ' << unum
                  << std::endl;

        rcss::rcg::Side s = ( side == 'l'
                              ? rcss::rcg::LEFT
                              : side == 'r'
                              ? rcss::rcg::RIGHT
                              : rcss::rcg::NEUTRAL );
        M_monitor_client->sendFreeKickRight( s, unum );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::redCard( const char side,
                     const int unum )
{
    if ( M_monitor_client
         && M_monitor_client->isConnected() )
    {
        std::cerr << "red_card : "
                  << side << ' ' << unum
                  << std::endl;
        rcss::rcg::Side s = ( side == 'l'
                              ? rcss::rcg::LEFT
                              : side == 'r'
                              ? rcss::rcg::RIGHT
                              : rcss::rcg::NEUTRAL );
        M_monitor_client->sendRedCard( s, unum );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::receiveMonitorPacket()
{
//     if ( M_log_player->isLiveMode() )
//     {
//         M_log_player->showLive();
//     }
//     else
//     {
//         M_log_slider_tool_bar->updateSlider();
//     }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::resizeCanvas( const QSize & size )
{
    if ( centralWidget() )
    {
        if ( this->isMaximized()
             || this->isFullScreen() )
        {
            this->showNormal();
        }

        QRect rect = this->geometry();

        int width_diff = rect.width() - centralWidget()->width();
        int height_diff = rect.height() - centralWidget()->height();

        if ( width_diff + size.width() < this->minimumWidth() )
        {
            std::cerr << "Too small canvas width "
                      << size.width() << " "
                      << " minimum=" << this->minimumWidth()
                      << std::endl;
            return;
        }

        if ( height_diff + size.height() < this->minimumHeight() )
        {
            std::cerr << "Too small canvas height "
                      << size.height() << " "
                      << " minimum=" << this->minimumHeight()
                      << std::endl;
            return;
        }

        rect.setWidth( size.width() + width_diff );
        rect.setHeight( size.height() + height_diff );

        this->setGeometry( rect );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::updatePositionLabel( const QPoint & point )
{
    if ( M_position_label
         && M_field_canvas
         && statusBar()
         && statusBar()->isVisible() )
    {
        double x = Options::instance().fieldX( point.x() );
        double y = Options::instance().fieldY( point.y() );

        char buf[32];
        snprintf( buf, 32,
                  "(%.2f, %.2f)",
                  x, y );

        M_position_label->setText( QString::fromAscii( buf ) );
    }
}
