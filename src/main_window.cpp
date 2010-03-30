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

#include "config_dialog.h"
#include "field_canvas.h"
#include "log_player.h"
#include "monitor_client.h"
#include "player_type_dialog.h"
#include "options.h"

#include <string>
#include <iostream>
#include <cstring>
#include <cstdio>

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
      M_player_type_dialog( static_cast< PlayerTypeDialog * >( 0 ) ),
      M_config_dialog( static_cast< ConfigDialog * >( 0 ) ),
      M_field_canvas( static_cast< FieldCanvas * >( 0 ) ),
      M_monitor_client( static_cast< MonitorClient * >( 0 ) ),
      M_log_player( new LogPlayer( M_disp_holder, this ) )
{
    this->setWindowIcon( QIcon( QPixmap( rcss_xpm ) ) );
    this->setWindowTitle( tr( PACKAGE_NAME" "VERSION ) );
    this->setMinimumSize( 280, 220 );

    readSettings();

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createFieldCanvas();
    createConfigDialog();

    connect( M_field_canvas, SIGNAL( focusChanged( const QPoint & ) ),
             this, SLOT( setFocusPoint( const QPoint & ) ) );
    connect( M_log_player, SIGNAL( updated() ),
             this, SIGNAL( viewUpdated() ) );
    connect( M_log_player, SIGNAL( updated() ),
             this, SLOT( updateBufferingLabel() ) );
    connect( M_log_player, SIGNAL( recoverTimerHandled() ),
             this, SLOT( updateBufferingLabel() ) );
    connect( M_log_player, SIGNAL( recoverTimerHandled() ),
             this, SLOT( showRecoveringState() ) );
    connect( M_log_player, SIGNAL( quitRequested() ),
             this, SLOT( setQuitTimer() ) );

    this->resize( Options::instance().windowWidth() > 0
                  ? Options::instance().windowWidth()
                  : 640,
                  Options::instance().windowHeight() > 0
                  ? Options::instance().windowHeight()
                  : 480 );
    this->move( Options::instance().windowX() >= 0
                ? Options::instance().windowX()
                : this->x(),
                Options::instance().windowY() >= 0
                ? Options::instance().windowY()
                : this->y() );
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
    if ( Options::instance().fullScreen() )
    {
        this->showFullScreen();
    }
    else if ( Options::instance().maximize() )
    {
        this->showMaximized();
    }

    if ( QApplication::setStyle( M_window_style ) )
    {
        Q_FOREACH( QAction * action, M_style_act_group->actions() )
        {
            if ( action->data().toString().toLower() == QApplication::style()->objectName().toLower() )
            {
                M_window_style = QApplication::style()->objectName().toLower();
                action->setChecked( true );
                break;
            }
        }
    }

    if ( ! Options::instance().bufferingMode() )
    {
        M_buffering_label->hide();
    }

    toggleMenuBar( Options::instance().showMenuBar() );
    toggleStatusBar( Options::instance().showStatusBar() );

    if ( Options::instance().connect() )
    {
        connectMonitor();
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

    settings.beginGroup( "MainWindow" );

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

    settings.beginGroup( "MainWindow" );

    settings.setValue( "window_style", M_window_style );

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
    // kick off
    M_kick_off_act = new QAction( tr( "&KickOff" ), this );
#ifdef Q_WS_MAC
    M_kick_off_act->setShortcut( Qt::META + Qt::Key_K );
#else
    M_kick_off_act->setShortcut( Qt::CTRL + Qt::Key_K );
#endif
    M_kick_off_act->setStatusTip( tr( "Send kick-off command." ) );
    connect( M_kick_off_act, SIGNAL( triggered() ),
             this, SLOT( kickOff() ) );
    this->addAction( M_kick_off_act );

    // yellow card
    M_yellow_card_act = new QAction( tr( "Yellow Card" ), this );
    {
        QPixmap icon( 16, 16 );
        icon.fill( Qt::yellow );
        M_yellow_card_act->setIcon( QIcon( icon ) );
    }
    M_yellow_card_act->setStatusTip( tr( "Call yellow card." ) );
    connect( M_yellow_card_act, SIGNAL( triggered() ),
             this, SLOT( yellowCard() ) );
    this->addAction( M_yellow_card_act );
    // red card
    M_red_card_act = new QAction( tr( "Red Card" ), this );
    {
        QPixmap icon( 16, 16 );
        icon.fill( Qt::red );
        M_red_card_act->setIcon( QIcon( icon ) );
    }
    M_red_card_act->setStatusTip( tr( "Call red card." ) );
    connect( M_red_card_act, SIGNAL( triggered() ),
             this, SLOT( redCard() ) );
    this->addAction( M_red_card_act );

    // playmode change
    M_playmode_change_act_group = new QActionGroup( this );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createActionsView()
{
    // menu bar
    M_toggle_menu_bar_act = new QAction( tr( "Menu Bar" ), this );
#ifdef Q_WS_MAC
    M_toggle_menu_bar_act->setShortcut( Qt::META + Qt::Key_M );
#else
    M_toggle_menu_bar_act->setShortcut( Qt::CTRL + Qt::Key_M );
#endif
    M_toggle_menu_bar_act->setCheckable( true );
    M_toggle_menu_bar_act->setChecked( Options::instance().showMenuBar() );
    M_toggle_menu_bar_act->setStatusTip( tr( "Show/Hide menu bar." ) );
    connect( M_toggle_menu_bar_act, SIGNAL( toggled( bool ) ),
             this, SLOT( toggleMenuBar( bool ) ) );
    this->addAction( M_toggle_menu_bar_act );

    // status bar
    M_toggle_status_bar_act = new QAction( tr( "Status Bar" ), this );
    M_toggle_status_bar_act->setCheckable( true );
    M_toggle_status_bar_act->setChecked( Options::instance().showStatusBar() );
    M_toggle_status_bar_act->setStatusTip( tr( "Show/Hide status bar." ) );
    connect( M_toggle_status_bar_act, SIGNAL( toggled( bool ) ),
             this, SLOT( toggleStatusBar( bool ) ) );
    this->addAction( M_toggle_status_bar_act );

    // full screen
    M_full_screen_act = new QAction( tr( "&Full Screen" ), this );
    M_full_screen_act->setShortcut( tr( "F11" ) );
    //M_full_screen_act->setShortcut( tr( "Alt+Enter" ) );
    //M_full_screen_act->setShortcut( Qt::ALT + Qt::Key_Return );
    //M_full_screen_act->setShortcut( Qt::ALT + Qt::Key_Enter );
    M_full_screen_act->setStatusTip( tr( "Toggle full screen." ) );
    connect( M_full_screen_act, SIGNAL( triggered() ),
             this, SLOT( toggleFullScreen() ) );
    this->addAction( M_full_screen_act );
    //(void) new QShortcut( Qt::ALT + Qt::Key_Enter, this, SLOT( toggleFullScreen() ) );
    (void) new QShortcut( Qt::ALT + Qt::Key_Return,
                          this, SLOT( toggleFullScreen() ) );
    (void) new QShortcut( Qt::ALT + Qt::Key_Enter,
                          this, SLOT( toggleFullScreen() ) );

    // qt style menu group
    M_style_act_group = new QActionGroup( this );
    Q_FOREACH ( QString style_name, QStyleFactory::keys() )
    {
        QAction * subaction = new QAction( M_style_act_group );
        subaction->setText( style_name );
        subaction->setData( style_name );
        subaction->setCheckable( true );
        if ( style_name.toLower() == QApplication::style()->objectName().toLower() )
        {
            subaction->setChecked( true );
        }
        connect( subaction, SIGNAL( triggered( bool ) ),
                 this, SLOT( changeStyle( bool ) ) );
    }

    // show/hide player type dialog
    M_show_player_type_dialog_act = new QAction( tr( "Player Type List" ), this );
#ifdef Q_WS_MAC
    M_show_player_type_dialog_act->setShortcut( tr( "Meta+H" ) );
#else
    M_show_player_type_dialog_act->setShortcut( tr( "Ctrl+H" ) );
#endif
    M_show_player_type_dialog_act->setStatusTip( tr( "Show player type list dialog." ) );
    connect( M_show_player_type_dialog_act, SIGNAL( triggered() ),
             this, SLOT( showPlayerTypeDialog() ) );
    this->addAction( M_show_player_type_dialog_act );

    // show/hide config dialog
    M_show_config_dialog_act = new QAction( tr( "Preference" ), this );
#ifdef Q_WS_MAC
    M_show_config_dialog_act->setShortcut( tr( "Meta+V" ) );
#else
    M_show_config_dialog_act->setShortcut( tr( "Ctrl+V" ) );
#endif
    M_show_config_dialog_act->setStatusTip( tr( "Show view config dialog." ) );
    connect( M_show_config_dialog_act, SIGNAL( triggered() ),
             this, SLOT( showConfigDialog() ) );
    this->addAction( M_show_config_dialog_act );
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

//     menu->addAction( M_set_live_mode_act );

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

    menu->addSeparator();

    menu->addAction( M_yellow_card_act );
    menu->addAction( M_red_card_act );

    menu->addSeparator();

    {
        QMenu * submenu = menu->addMenu( tr( "Change Playmode" ) );

        const char * playmode_strings[] = PLAYMODE_STRINGS;

        QSignalMapper * mapper = new QSignalMapper( this );
        connect( mapper, SIGNAL( mapped( int ) ),
                 this, SLOT( changePlayMode( int ) ) );
        for ( int mode = 0; mode < rcss::rcg::PM_MAX; ++mode )
        {
            if ( mode == rcss::rcg::PM_BeforeKickOff
                 || mode == rcss::rcg::PM_PlayOn
                 || mode == rcss::rcg::PM_KickIn_Left
                 || mode == rcss::rcg::PM_KickIn_Right
                 || mode == rcss::rcg::PM_CornerKick_Left
                 || mode == rcss::rcg::PM_CornerKick_Right
                 || mode == rcss::rcg::PM_GoalKick_Left
                 || mode == rcss::rcg::PM_GoalKick_Right
                 || mode == rcss::rcg::PM_Foul_Charge_Left
                 || mode == rcss::rcg::PM_Foul_Charge_Right
                 //|| mode == rcss::rcg::PM_Foul_Push_Left
                 //|| mode == rcss::rcg::PM_Foul_Push_Right
                 || mode == rcss::rcg::PM_Back_Pass_Left
                 || mode == rcss::rcg::PM_Back_Pass_Right
                 || mode == rcss::rcg::PM_GoalKick_Right
                 || mode == rcss::rcg::PM_IndFreeKick_Left
                 || mode == rcss::rcg::PM_IndFreeKick_Right )
            {
                QAction * act = submenu->addAction( QString::fromAscii( playmode_strings[mode] ) );
                connect( act, SIGNAL( triggered() ), mapper, SLOT( map() ) );
                mapper->setMapping( act, mode );
            }
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createMenuView()
{
    QMenu * menu = menuBar()->addMenu( tr( "&View" ) );

    menu->addAction( M_toggle_menu_bar_act );
    menu->addAction( M_toggle_status_bar_act );

    menu->addSeparator();

    menu->addAction( M_show_player_type_dialog_act );

    menu->addSeparator();

    menu->addAction( M_full_screen_act );

    {
        QMenu * submenu = menu->addMenu( tr( "Window &Style" ) );
        Q_FOREACH ( QAction * action, M_style_act_group->actions() )
        {
            submenu->addAction( action );
        }
    }
    menu->addAction( M_show_config_dialog_act );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createMenuHelp()
{
    QMenu * menu = menuBar()->addMenu( tr( "&Help" ) );
    //menu->addAction( M_about_act );
    {
        QAction * act = menu->addAction( tr( "About" ), this, SLOT( about() ) );
        act->setStatusTip( tr( "Show the about dialog." ) );
    }
    {
        QAction * act = menu->addAction( tr( "Shortcut Keys" ), this, SLOT( printShortcutKeys() ) );
        act->setStatusTip( tr( "Print available shortcut keys." ) );
    }

    menu->addSeparator();

    menu->addAction( tr( "About Qt" ), qApp, SLOT( aboutQt() ) );
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

    int min_width = 0;

    M_buffering_label = new QLabel( tr( "Buffering   0" ) );
    min_width
        = M_buffering_label->fontMetrics().width(  tr( "Buffering 000" ) )
        + 16;
    M_buffering_label->setMinimumWidth( min_width );
    M_buffering_label->setAlignment( Qt::AlignLeft );
    this->statusBar()->addPermanentWidget( M_buffering_label );

    //

    M_position_label = new QLabel( tr( "(0.0, 0.0)" ) );

    min_width
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
    M_field_canvas = new FieldCanvas( M_disp_holder );

    QMenu * popup_menu = M_field_canvas->createPopupMenu();
    popup_menu->addSeparator();
    popup_menu->addAction( M_yellow_card_act );
    popup_menu->addAction( M_red_card_act );


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

    connect( M_field_canvas, SIGNAL( playModeChanged( int, const QPoint & ) ),
             this, SLOT( changePlayMode( int, const QPoint & ) ) );

    {
        const char * playmode_strings[] = PLAYMODE_STRINGS;

        QSignalMapper * mapper = new QSignalMapper( this );
        connect( mapper, SIGNAL( mapped( int ) ),
                 M_field_canvas, SLOT( changePlayMode( int ) ) );
        for ( int mode = 0; mode < rcss::rcg::PM_MAX; ++mode )
        {
            if ( mode == rcss::rcg::PM_BeforeKickOff
                 || mode == rcss::rcg::PM_PlayOn
                 || mode == rcss::rcg::PM_KickIn_Left
                 || mode == rcss::rcg::PM_KickIn_Right
                 || mode == rcss::rcg::PM_CornerKick_Left
                 || mode == rcss::rcg::PM_CornerKick_Right
                 || mode == rcss::rcg::PM_GoalKick_Left
                 || mode == rcss::rcg::PM_GoalKick_Right
                 || mode == rcss::rcg::PM_Foul_Charge_Left
                 || mode == rcss::rcg::PM_Foul_Charge_Right
                 //|| mode == rcss::rcg::PM_Foul_Push_Left
                 //|| mode == rcss::rcg::PM_Foul_Push_Right
                 || mode == rcss::rcg::PM_Back_Pass_Left
                 || mode == rcss::rcg::PM_Back_Pass_Right
                 || mode == rcss::rcg::PM_GoalKick_Right
                 || mode == rcss::rcg::PM_IndFreeKick_Left
                 || mode == rcss::rcg::PM_IndFreeKick_Right )
            {
                QAction * act = new QAction( M_playmode_change_act_group );
                act->setText( QString::fromAscii( playmode_strings[mode] ) );
                connect( act, SIGNAL( triggered() ), mapper, SLOT( map() ) );
                mapper->setMapping( act, mode );
            }
        }

        popup_menu->addSeparator();
        QMenu * submenu = popup_menu->addMenu( tr( "Change Playmode" ) );
        Q_FOREACH ( QAction * act, M_playmode_change_act_group->actions() )
        {
            submenu->addAction( act );
        }
    }

}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::createConfigDialog()
{
    if ( M_config_dialog )
    {
        return;
    }

    M_config_dialog = new ConfigDialog( this, M_disp_holder );

    M_config_dialog->hide();

    connect( M_config_dialog, SIGNAL( configured() ),
             this, SIGNAL( viewUpdated() ) );

    connect( M_config_dialog, SIGNAL( canvasResized( const QSize & ) ),
             this, SLOT( resizeCanvas( const QSize & ) ) );

    // register short cut keys
    {
        // z
        QAction * act = new QAction( tr( "ZoomIn" ), this );
        act->setShortcut( Qt::Key_Z );
        act->setStatusTip( tr( "Zoom in." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( zoomIn() ) );
    }
    {
        // x
        QAction * act = new QAction( tr( "ZoomOut" ), this );
        act->setShortcut( Qt::Key_X );
        act->setStatusTip( tr( "Zoom out." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( zoomOut() ) );
    }
    {
        // Ctrl + z
        QAction * act = new QAction( tr( "ZoomOut" ), this );
#ifdef Q_WS_MAC
        act->setShortcut( Qt::META + Qt::Key_Z );
#else
        act->setShortcut( Qt::CTRL + Qt::Key_Z );
#endif
        act->setStatusTip( tr( "Zoom out." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( zoomOut() ) );
    }
    {
        // i
        QAction * act = new QAction( tr( "Fit Field Size" ), this );
        act->setShortcut( Qt::Key_I );
        act->setStatusTip( tr( "Fit field size to the screen." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( fitToScreen() ) );
    }

    // field style
//     {
//         QAction * act = new QAction( tr( "Show Keepaway Area" ), this );
//         act->setStatusTip( tr( "Show keepaway area." ) );
//         this->addAction( act );
//         connect( act, SIGNAL( triggered() ),
//                  M_config_dialog, SLOT( toggleShowKeepawayArea() ) );
//     }

    // player detail
    {
        // n
        QAction * act = new QAction( tr( "Show Player Number" ), this );
        act->setShortcut( Qt::Key_N );
        act->setStatusTip( tr( "Show/Hide player number." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleShowPlayerNumber() ) );
    }
    {
        // h
        QAction * act = new QAction( tr( "Show Player Type Id" ), this );
        act->setShortcut( Qt::Key_H );
        act->setStatusTip( tr( "Show/Hide player type id." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleShowPlayerType() ) );
    }
    {
        // s
        QAction * act = new QAction( tr( "Show Stamina" ), this );
        act->setShortcut( Qt::Key_S );
        act->setStatusTip( tr( "Show/Hide player's stamina." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleShowStamina() ) );
    }
    {
        // v
        QAction * act = new QAction( tr( "Show View Area" ), this );
        act->setShortcut( Qt::Key_V );
        act->setStatusTip( tr( "Show/Hide player's view area." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleShowViewArea() ) );
    }
    {
        // c
        QAction * act = new QAction( tr( "Show Catch Area" ), this );
        act->setShortcut( Qt::Key_C );
        act->setStatusTip( tr( "Show/Hide goalie's catchable area." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleShowCatchArea() ) );
    }
    {
        // k
        QAction * act = new QAction( tr( "Show Tackle Area" ), this );
        act->setShortcut( Qt::Key_T );
        act->setStatusTip( tr( "Show/Hide player's tackle area if player can tackle the ball." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleShowTackleArea() ) );
    }
    {
        // k
        QAction * act = new QAction( tr( "Show Kick Accel Area" ), this );
        act->setShortcut( Qt::Key_K );
        act->setStatusTip( tr( "Show/Hide player's kick accel area if player can kick the ball." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleShowKickAccelArea() ) );
    }
    {
        // Ctrl + a
        QAction * act = new QAction( tr( "Show Pointto" ), this );
#ifdef Q_WS_MAC
        act->setShortcut( Qt::META + Qt::Key_A );
#else
        act->setShortcut( Qt::CTRL + Qt::Key_A );
#endif
        act->setStatusTip( tr( "Show/Hide player's pointing position." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleShowPointto() ) );
    }

    // show/hide
    {
        // Ctrl + s
        QAction * act = new QAction( tr( "Show Score Board" ), this );
 #ifdef Q_WS_MAC
        act->setShortcut( Qt::META + Qt::Key_S );
#else
        act->setShortcut( Qt::CTRL + Qt::Key_S );
#endif
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleShowScoreBoard() ) );
    }
    {
        // Ctrl + b
        QAction * act = new QAction( tr( "Show Ball" ), this );
#ifdef Q_WS_MAC
        act->setShortcut( Qt::META + Qt::Key_B );
#else
        act->setShortcut( Qt::CTRL + Qt::Key_B );
#endif
        act->setStatusTip( tr( "Show/Hide ball." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleShowBall() ) );
    }
    {
        // Ctrl + p
        QAction * act = new QAction( tr( "Show Players" ), this );
#ifdef Q_WS_MAC
        act->setShortcut( Qt::META + Qt::Key_P );
#else
        act->setShortcut( Qt::CTRL + Qt::Key_P );
#endif
        act->setStatusTip( tr( "Show/Hide players." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleShowPlayer() ) );
    }
    {
        // Ctrl + f
        QAction * act = new QAction( tr( "Show Flags" ), this );
#ifdef Q_WS_MAC
        act->setShortcut( Qt::META + Qt::Key_F );
#else
        act->setShortcut( Qt::CTRL + Qt::Key_F );
#endif
        act->setStatusTip( tr( "Show/Hide flags." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleShowFlag() ) );
    }
    {
        // o
        QAction * act = new QAction( tr( "Show Offside Line" ), this );
        act->setShortcut( Qt::Key_O );
        act->setStatusTip( tr( "Show/Hide offside lines." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleShowOffsideLine() ) );
    }

    // number 1-10
    for ( int i = 0; i < 10; ++i )
    {
        {
            QAction * act = new QAction( QString( "Select Left %1" ).arg( i ), this );
            act->setShortcut( Qt::Key_0 + i );
            act->setStatusTip( QString( "Select left player %1" ).arg( i ) );
            this->addAction( act );
            connect( act, SIGNAL( triggered() ),
                     M_config_dialog, SLOT( selectPlayerWithKey() ) );
        }
        {
            QAction * act = new QAction( QString( "Selct Right %1" ).arg( i ), this );
#ifdef Q_WS_MAC
            act->setShortcut( Qt::META + Qt::Key_0 + i );
#else
            act->setShortcut( Qt::CTRL + Qt::Key_0 + i );
#endif
            act->setStatusTip( QString( "Select right player %1" ).arg( i ) );
            this->addAction( act );
            connect( act, SIGNAL( triggered() ),
                     M_config_dialog, SLOT( selectPlayerWithKey() ) );
        }
    }
    // number 11
    {
        {
            QAction * act = new QAction( tr( "Select Left 11" ), this );
            act->setShortcut( Qt::Key_Minus );
            act->setStatusTip( tr( "Select left player 11" ) );
            this->addAction( act );
            connect( act, SIGNAL( triggered() ),
                     M_config_dialog, SLOT( selectPlayerWithKey() ) );
        }
        {
            QAction * act = new QAction( tr( "Select Right 11" ), this );
#ifdef Q_WS_MAC
            act->setShortcut( Qt::META + Qt::Key_Minus );
#else
            act->setShortcut( Qt::CTRL + Qt::Key_Minus );
#endif
            act->setStatusTip( tr( "Select right player 11" ) );
            this->addAction( act );
            connect( act, SIGNAL( triggered() ),
                     M_config_dialog, SLOT( selectPlayerWithKey() ) );
        }
    }
    // b
    {
        QAction * act = new QAction( tr( "Focus Ball" ), this );
        act->setShortcut( Qt::Key_B );
        act->setStatusTip( tr( "Toggle automatic ball focus mode." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleFocusBall() ) );
    }
    // p
    {
        QAction * act = new QAction( tr( "Focus selected player" ), this );
        act->setShortcut( Qt::Key_P );
        act->setStatusTip( tr( "Toggle automatic player focus mode." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleFocusPlayer() ) );
    }
    // a
    {
        QAction * act = new QAction( tr( "Select the ball nearest player automatically" ), this );
        act->setShortcut( Qt::Key_A );
        act->setStatusTip( tr( "Toggle automatic player selection from all players." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleSelectAutoAll() ) );
    }
    // l
    {
        QAction * act = new QAction( tr( "Select the ball nearest left player automatically" ), this );
        act->setShortcut( Qt::Key_L );
        act->setStatusTip( tr( "Toggle automatic player selection from left team." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleSelectAutoLeft() ) );
    }
    // r
    {
        QAction * act = new QAction( tr( "Select the ball nearest right player automatically" ), this );
        act->setShortcut( Qt::Key_R );
        act->setStatusTip( tr( "Toggle automatic player selection from right team." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( toggleSelectAutoRight() ) );
    }
    // u
    {
        QAction * act = new QAction( tr( "Unselect player" ), this );
        act->setShortcut( Qt::Key_U );
        act->setStatusTip( tr( "Unselect the player." ) );
        this->addAction( act );
        connect( act, SIGNAL( triggered() ),
                 M_config_dialog, SLOT( setUnselect() ) );
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
        std::cerr << "Empty host name. Connection failed." << std::endl;
        this->statusBar()->showMessage( tr( "Empty host name." ), 5000 );
        return;
    }

    disconnectMonitor();

    std::cerr << "Connect to [" << hostname << "] ..." << std::endl;
    this->statusBar()->showMessage( tr( "Connect to %1 ..." ).arg( QString::fromAscii( hostname ) ),
                                    5000 );

    M_monitor_client = new MonitorClient( this,
                                          M_disp_holder,
                                          hostname,
                                          Options::instance().serverPort(),
                                          Options::instance().clientVersion() );

    if ( ! M_monitor_client->isConnected() )
    {
        std::cerr << "Conenction failed." << std::endl;
        this->statusBar()->showMessage( tr( "Connection failed." ), 5000 );
        delete M_monitor_client;
        M_monitor_client = static_cast< MonitorClient * >( 0 );
        return;
    }

    // reset all data
    M_disp_holder.clear();
    M_log_player->clear();

    if ( M_player_type_dialog )
    {
        M_player_type_dialog->hide();
    }

    if ( M_config_dialog )
    {
        M_config_dialog->fitToScreen();
    }

    Options::instance().setServerHost( hostname );
    Options::instance().setMonitorClientMode( true );
    Options::instance().setBufferRecoverMode( true );

//     M_save_image_act->setEnabled( false );
//     M_open_output_act->setEnabled( true );

//     M_set_live_mode_act->setEnabled( true );
    M_connect_monitor_act->setEnabled( false );
    M_connect_monitor_to_act->setEnabled( false );
    M_disconnect_monitor_act->setEnabled( true );

    connect( M_monitor_client, SIGNAL( received() ),
             this, SLOT( receiveMonitorPacket() ) );
    connect( M_monitor_client, SIGNAL( disconnectRequested() ),
             this, SLOT( disconnectMonitor() ) );
    connect( M_monitor_client, SIGNAL( reconnectRequested() ),
             this, SLOT( reconnectMonitor() ) );

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
MainWindow::about()
{
    QString msg( tr( PACKAGE_NAME"-"VERSION"\n\n" ) );
    msg += tr( "The RoboCup Soccer Simulator Monitor (rcssmonitor) is used to view\n"
               "the simulation as it takes place by connecting to the rcssserver or\n"
               "to view the playback of a simulation by connecting to the rcsslogplayer.\n"
               "\n"
               "The RoboCup Soccer Simulator Official Web Page:\n"
               "  http://sserver.sourceforge.net/\n"
               "Author:\n"
               "  The RoboCup Soccer Simulator Maintenance Committee.\n"
               "  <sserver-admin@lists.sourceforgenet>" );

    QMessageBox::about( this,
                        tr( "About "PACKAGE_NAME ),
                        msg );

    // from Qt 4.1 documents
    /*
      about() looks for a suitable icon in four locations:

      1. It prefers parent->icon() if that exists.
      2. If not, it tries the top-level widget containing parent.
      3. If that fails, it tries the active window.
      4. As a last resort it uses the Information icon.

      The about box has a single button labelled "OK".
    */
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::printShortcutKeys()
{
    static QDialog * dialog = static_cast< QDialog * >( 0 );

    if ( ! dialog )
    {
        dialog = new QDialog( this );
        dialog->setWindowTitle( tr( "Shortcut Keys" ) );

        QVBoxLayout * layout = new QVBoxLayout();

        QTableWidget * table_widget = new QTableWidget( dialog );
        table_widget->insertColumn( 0 );
        table_widget->insertColumn( 1 );

        QStringList header;
        header.push_back( tr( "key" ) );
        header.push_back( tr( "action" ) );
        table_widget->setHorizontalHeaderLabels( header );

        table_widget->horizontalHeader()->setStretchLastSection( true );
        table_widget->horizontalHeader()->setResizeMode( QHeaderView::ResizeToContents );
        table_widget->verticalHeader()->hide();

        int row = 0;

        Q_FOREACH( QAction * act, this->actions() )
        {
            if ( ! act->shortcut().isEmpty() )
            {
                table_widget->insertRow( row );
                table_widget->setItem ( row, 0, new QTableWidgetItem( act->shortcut().toString() ) );
                table_widget->setItem ( row, 1, new QTableWidgetItem( QString( act->statusTip() ).remove( QChar( '&' ) ) ) );
                ++row;
            }
        }

        table_widget->setSortingEnabled( true );
        //     std::cout <<  "table row_count = " << table_widget->rowCount()
        //               <<  "table col_count = " << table_widget->columnCount()
        //               << std::endl;

        layout->addWidget( table_widget );
        dialog->setLayout( layout );
    }

    if ( dialog )
    {
        dialog->show();
    }

#if 0
    Q_FOREACH( QAction * act, this->actions() )
    {
        if ( ! act->shortcut().isEmpty() )
        {
            std::cout << act->shortcut().toString().toStdString() << ", "
                      << QString( act->text() ).remove( QChar( '&' ) ).toStdString()
                      << '\n';
        }
    }
    std::cout << std::flush;
#endif
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::setQuitTimer()
{
    int wait_msec = ( Options::instance().autoQuitWait() > 0
                      ? Options::instance().autoQuitWait() * 1000
                      : 100 );
    QTimer::singleShot( wait_msec,
                        qApp, SLOT( quit() ) );
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
        this->statusBar()->showMessage( tr( "KickOff" ), 3000 );
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
    if ( M_monitor_client )
    {
        M_monitor_client->disconnect();

        disconnect( M_monitor_client, SIGNAL( received() ),
                    this, SLOT( receiveMonitorPacket() ) );
        disconnect( M_monitor_client, SIGNAL( disconnectRequested() ),
                    this, SLOT( disconnectMonitor() ) );
        disconnect( M_monitor_client, SIGNAL( reconnectRequested() ),
                    this, SLOT( reconnectMonitor() ) );

        delete M_monitor_client;
        M_monitor_client = static_cast< MonitorClient * >( 0 );

        //
        // quit application if auto_quit_mode is on
        //
        if ( ! Options::instance().autoReconnectMode()
             && ! Options::instance().bufferingMode()
             && Options::instance().autoQuitMode() )
        {
            setQuitTimer();
        }
    }

    Options::instance().setMonitorClientMode( false );
    Options::instance().setBufferRecoverMode( false );

//     M_set_live_mode_act->setEnabled( false );
    M_connect_monitor_act->setEnabled( true );
    M_connect_monitor_to_act->setEnabled( true );
    M_disconnect_monitor_act->setEnabled( false );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::reconnectMonitor()
{
    //std::cerr << "MainWindow::reconnectMonitor()" << std::endl;
    if ( M_monitor_client )
    {
        disconnectMonitor();
        std::cerr << "Trying to reconnect ..." << std::endl;
        QTimer::singleShot( 1 * 1000,
                            this, SLOT( connectMonitor() ) );
    }
    else
    {
        connectMonitor();
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::toggleMenuBar( bool checked )
{
    this->menuBar()->setVisible( checked );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::toggleStatusBar( bool checked )
{
    this->statusBar()->setVisible( checked );
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
MainWindow::toggleFullScreen()
{
    if ( this->isFullScreen() )
    {
        this->showNormal();
    }
    else
    {
        this->showFullScreen();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MainWindow::showPlayerTypeDialog()
{
    if ( M_player_type_dialog )
    {
        M_player_type_dialog->setVisible( ! M_player_type_dialog->isVisible() );
    }
    else
    {
        M_player_type_dialog = new PlayerTypeDialog( M_disp_holder, this );
        M_player_type_dialog->show();
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::showConfigDialog()
{
    M_config_dialog->setVisible( ! M_config_dialog->isVisible() );

    if ( M_config_dialog->isVisible() )
    {
        M_config_dialog->setFixedSize( M_config_dialog->size() );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::setFocusPoint( const QPoint & point )
{
    Options::instance().setFocusPoint( point.x(), point.y() );

    emit viewUpdated();
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::dropBall( const QPoint & point )
{
    {
        double x = Options::instance().fieldX( point.x() );
        double y = Options::instance().fieldY( point.y() );

        std::cerr << "drop ball to ("
                  << x << ", " << y << ")"
                  << std::endl;
    }

    if ( M_monitor_client
         && M_monitor_client->isConnected() )
    {
        double x = Options::instance().fieldX( point.x() );
        double y = Options::instance().fieldY( point.y() );

        std::cerr << "drop ball to ("
                  << x << ", " << y << ")"
                  << std::endl;
        this->statusBar()->showMessage( tr( "Drop ball at (%1, %2)" ).arg( x ).arg( y ),
                                        1000 );
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
        this->statusBar()->showMessage( tr( "Free kick left at (%1, %2)" ).arg( x ).arg( y ),
                                        1000 );
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
        this->statusBar()->showMessage( tr( "Free kick right at (%1, %2)" ).arg( x ).arg( y ),
                                        1000 );
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
        this->statusBar()->showMessage( tr( "Yellow card (%1 %2)" ).arg( side ).arg( unum ),
                                        5000 );

        rcss::rcg::Side s = ( side == 'l'
                              ? rcss::rcg::LEFT
                              : side == 'r'
                              ? rcss::rcg::RIGHT
                              : rcss::rcg::NEUTRAL );
        M_monitor_client->sendYellowCard( s, unum );
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
        this->statusBar()->showMessage( tr( "Red card (%1 %2)" ).arg( side ).arg( unum ),
                                        5000 );

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
MainWindow::yellowCard()
{
//     int selected_unum = Options::instance().selectedNumber();
//     if ( selected_unum != 0 )
//     {
//         yellowCard( ( selected_unum > 0 ? 'l' : 'r' ),
//                     qAbs( selected_unum ) );
//         return;
//     }

    QStringList players;
    players << tr( "" );
    for ( int i = 1; i <= 11; ++i )
    {
        players << tr( "Left %1" ).arg( i );
    }
    for ( int i = 1; i <= 11; ++i )
    {
        players << tr( "Right %1" ).arg( i );
    }

    bool ok = false;
    QString str = QInputDialog::getItem( this,
                                         tr( "Yellow Card" ),
                                         tr( "Select Player" ),
                                         players,
                                         0, // current
                                         false, // editable
                                         &ok );
    if ( ! ok )
    {
        return;
    }

    char side_str[8];
    int unum = 0;
    if ( std::sscanf( str.toAscii().data(), " %s %d ", side_str, &unum ) != 2 )
    {
        return;
    }

    yellowCard( ( side_str[0] == 'L'
                  ?  'l'
                  : side_str[0] == 'R'
                  ? 'r'
                  : '?' ),
                unum );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::redCard()
{
//     int selected_unum = Options::instance().selectedNumber();
//     if ( selected_unum != 0 )
//     {
//         redCard( ( selected_unum > 0 ? 'l' : 'r' ),
//                  qAbs( selected_unum ) );
//         return;
//     }

    QStringList players;
    players << tr( "" );
    for ( int i = 1; i <= 11; ++i )
    {
        players << tr( "Left %1" ).arg( i );
    }
    for ( int i = 1; i <= 11; ++i )
    {
        players << tr( "Right %1" ).arg( i );
    }

    bool ok = false;
    QString str = QInputDialog::getItem( this,
                                         tr( "Red Card" ),
                                         tr( "Select Player" ),
                                         players,
                                         0, // current
                                         false, // editable
                                         &ok );
    if ( ! ok )
    {
        return;
    }

    char side_str[8];
    int unum = 0;
    if ( std::sscanf( str.toAscii().data(), " %s %d ", side_str, &unum ) != 2 )
    {
        return;
    }

    redCard( ( side_str[0] == 'L'
               ?  'l'
               : side_str[0] == 'R'
               ? 'r'
               : '?' ),
             unum );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::changePlayMode( int mode )
{
    DispConstPtr disp = M_disp_holder.currentDisp();
    if ( ! disp )
    {
        return;
    }

    QPoint point;
    point.setX( Options::instance().screenX( disp->show_.ball_.x_ ) );
    point.setY( Options::instance().screenY( disp->show_.ball_.y_ ) );

    changePlayMode( mode, point );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::changePlayMode( int mode,
                            const QPoint & point )
{
    if ( ! M_monitor_client
         || ! M_monitor_client->isConnected() )
    {
        return;
    }

    const rcss::rcg::ServerParamT & SP = M_disp_holder.serverParam();

    if ( ! SP.coach_mode_
         && ! SP.coach_with_referee_mode_ )
    {
        QMessageBox::warning( this,
                              tr( "PlayMode Change" ),
                              tr( "Server is running without coach mode." ) );
        return;
    }

    rcss::rcg::PlayMode pmode = static_cast< rcss::rcg::PlayMode >( mode );

    double x = Options::instance().fieldX( point.x() );
    double y = Options::instance().fieldY( point.y() );

    x = std::min( x, + Options::PITCH_LENGTH*0.5 );
    x = std::max( x, - Options::PITCH_LENGTH*0.5 );
    y = std::min( y, + Options::PITCH_WIDTH*0.5 );
    y = std::max( y, - Options::PITCH_WIDTH*0.5 );

    switch ( pmode ) {
    case rcss::rcg::PM_BeforeKickOff:
        M_monitor_client->sendTrainerMoveBall( 0.0, 0.0, 0.0, 0.0 );
        M_monitor_client->sendChangeMode( pmode );
        break;
    case rcss::rcg::PM_PlayOn:
        M_monitor_client->sendTrainerMoveBall( x, y, 0.0, 0.0 );
        M_monitor_client->sendChangeMode( pmode );
        break;
    case rcss::rcg::PM_KickIn_Left:
    case rcss::rcg::PM_KickIn_Right:
        y = ( y > 0.0
              ? + Options::PITCH_WIDTH*0.5
              : - Options::PITCH_WIDTH*0.5 );

        M_monitor_client->sendTrainerMoveBall( x, y, 0.0, 0.0 );
        M_monitor_client->sendChangeMode( pmode );
        break;
    case rcss::rcg::PM_CornerKick_Left:
        x = + Options::PITCH_LENGTH*0.5 - Options::CORNER_ARC_R;
        y = ( y > 0.0
              ? ( + Options::PITCH_WIDTH*0.5 - Options::CORNER_ARC_R )
              : ( - Options::PITCH_WIDTH*0.5 + Options::CORNER_ARC_R ) );

        M_monitor_client->sendTrainerMoveBall( x, y, 0.0, 0.0 );
        M_monitor_client->sendChangeMode( pmode );
        break;
    case rcss::rcg::PM_CornerKick_Right:
        x = - Options::PITCH_LENGTH*0.5 + Options::CORNER_ARC_R;
        y = ( y > 0.0
              ? ( + Options::PITCH_WIDTH*0.5 - Options::CORNER_ARC_R )
              : ( - Options::PITCH_WIDTH*0.5 + Options::CORNER_ARC_R ) );

        M_monitor_client->sendTrainerMoveBall( x, y, 0.0, 0.0 );
        M_monitor_client->sendChangeMode( pmode );
        break;
    case rcss::rcg::PM_GoalKick_Left:
        x = - Options::PITCH_LENGTH*0.5 + Options::GOAL_AREA_LENGTH;
        y = ( y > 0.0
              ? + Options::GOAL_AREA_WIDTH*0.5
              : - Options::GOAL_AREA_WIDTH*0.5 );

        M_monitor_client->sendTrainerMoveBall( x, y, 0.0, 0.0 );
        M_monitor_client->sendChangeMode( pmode );
        break;
    case rcss::rcg::PM_GoalKick_Right:
        x = + Options::PITCH_LENGTH*0.5 - Options::GOAL_AREA_LENGTH;
        y = ( y > 0.0
              ? + Options::GOAL_AREA_WIDTH*0.5
              : - Options::GOAL_AREA_WIDTH*0.5 );

        M_monitor_client->sendTrainerMoveBall( x, y, 0.0, 0.0 );
        M_monitor_client->sendChangeMode( pmode );
        break;
    case rcss::rcg::PM_Foul_Charge_Left:
        M_monitor_client->sendTrainerMoveBall( x, y, 0.0, 0.0 );
        M_monitor_client->sendChangeMode( pmode );
        break;
    case rcss::rcg::PM_Foul_Charge_Right:
        M_monitor_client->sendTrainerMoveBall( x, y, 0.0, 0.0 );
        M_monitor_client->sendChangeMode( pmode );
        break;
    case rcss::rcg::PM_Foul_Push_Left:
        M_monitor_client->sendTrainerMoveBall( x, y, 0.0, 0.0 );
        M_monitor_client->sendChangeMode( pmode );
        break;
    case rcss::rcg::PM_Foul_Push_Right:
        M_monitor_client->sendTrainerMoveBall( x, y, 0.0, 0.0 );
        M_monitor_client->sendChangeMode( pmode );
        break;
    case rcss::rcg::PM_Back_Pass_Right:
        if ( x < ( + Options::PITCH_LENGTH*0.5 - Options::PENALTY_AREA_LENGTH ) )
        {
            x = + Options::PITCH_LENGTH*0.5 - Options::PENALTY_AREA_LENGTH;
        }

        if ( std::fabs( y ) > Options::PENALTY_AREA_WIDTH*0.5 )
        {
            y = ( y > 0.0
                  ? + Options::PENALTY_AREA_WIDTH*0.5
                  : - Options::PENALTY_AREA_WIDTH*0.5 );
        }
    case rcss::rcg::PM_IndFreeKick_Left:
        if ( x >= ( + Options::PITCH_LENGTH*0.5 - Options::GOAL_AREA_LENGTH )
             && std::fabs( y ) <= Options::GOAL_AREA_WIDTH*0.5 )
        {
            x = + Options::PITCH_LENGTH*0.5 - Options::GOAL_AREA_LENGTH;
        }

        M_monitor_client->sendTrainerMoveBall( x, y, 0.0, 0.0 );
        M_monitor_client->sendChangeMode( pmode );
        break;
    case rcss::rcg::PM_Back_Pass_Left:
        if ( x > ( - Options::PITCH_LENGTH*0.5 + Options::PENALTY_AREA_LENGTH ) )
        {
            x = - Options::PITCH_LENGTH*0.5 + Options::PENALTY_AREA_LENGTH;
        }

        if ( std::fabs( y ) > Options::PENALTY_AREA_WIDTH*0.5 )
        {
            y = ( y > 0.0
                  ? + Options::PENALTY_AREA_WIDTH*0.5
                  : - Options::PENALTY_AREA_WIDTH*0.5 );
        }
    case rcss::rcg::PM_IndFreeKick_Right:
        if ( x <= ( - Options::PITCH_LENGTH*0.5 + Options::GOAL_AREA_LENGTH )
             && std::fabs( y ) <= Options::GOAL_AREA_WIDTH*0.5 )
        {
            x = - Options::PITCH_LENGTH*0.5 + Options::GOAL_AREA_LENGTH;
        }

        M_monitor_client->sendTrainerMoveBall( x, y, 0.0, 0.0 );
        M_monitor_client->sendChangeMode( pmode );
        break;
    default:
        QMessageBox::warning( this,
                              tr( "PlayMode Change" ),
                              tr( "Unsupported playmode " ) );
        break;
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::receiveMonitorPacket()
{
    if ( Options::instance().bufferingMode() )
    {
        M_log_player->startTimer();
    }
    else
    {
        M_log_player->showLive();
    }
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

        int width_diff = rect.width() - this->centralWidget()->width();
        int height_diff = rect.height() - this->centralWidget()->height();

        if ( width_diff + size.width() < this->minimumWidth() )
        {
            std::cerr << "Too small canvas width "
                      << size.width() << " "
                      << " minimum=" << this->minimumWidth()
                      << std::endl;
            QMessageBox::warning( this,
                                  tr( "Canvas Resize" ),
                                  tr( "Too small canvas width: %1. minimum=%2" )
                                  .arg( size.width() )
                                  .arg( this->minimumWidth() ) );
            return;
        }

        if ( height_diff + size.height() < this->minimumHeight() )
        {
            std::cerr << "Too small canvas height "
                      << size.height() << " "
                      << " minimum=" << this->minimumHeight()
                      << std::endl;
            QMessageBox::warning( this,
                                  tr( "Canvas Resize" ),
                                  tr( "Too small canvas height: %1. minimum=%2" )
                                  .arg( size.height() )
                                  .arg( this->minimumHeight() ) );
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
    if ( this->statusBar()->isVisible() )
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

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::updateBufferingLabel()
{
    static int s_last_value = -1;

    if ( this->statusBar()->isVisible()
         && M_buffering_label->isVisible() )
    {
        int current_index = M_disp_holder.currentIndex() == DispHolder::INVALID_INDEX
            ? 0
            : M_disp_holder.currentIndex();
        //M_buffering_label->setText( tr( "Buffering %1/%2" )
        //                            .arg( cur )
        //                            .arg( M_disp_holder.dispCont().size() ) );
        int current_cache = M_disp_holder.dispCont().size() - current_index;
        current_cache = std::max( 0, current_cache - 1 );
        if ( s_last_value != current_cache )
        {
            M_buffering_label->setText( tr( "Buffering %1" ).arg( current_cache ) );
            s_last_value = current_cache;
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
MainWindow::showRecoveringState()
{
    M_field_canvas->update( M_field_canvas->width()/2 - Options::WAITING_ANIMATION_SIZE/2,
                            M_field_canvas->height()/2 - Options::WAITING_ANIMATION_SIZE/2,
                            Options::WAITING_ANIMATION_SIZE,
                            Options::WAITING_ANIMATION_SIZE );
}
