// -*-c++-*-

/*!
  \file main_window.h
  \brief main application window class Header File.
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

#ifndef RCSSMONITOR_MAIN_WINDOW_H
#define RCSSMONITOR_MAIN_WINDOW_H

#include <QMainWindow>
#include <QString>

#include "disp_holder.h"

class QActionGroup;
class QLabel;

class ConfigDialog;
class FieldCanvas;
class LogPlayer;
class LogPlayerSlider;
class MonitorClient;
class PlayerTypeDialog;

class MainWindow
    : public QMainWindow {

    Q_OBJECT

private:

    QString M_window_style;

    DispHolder M_disp_holder;

    QToolBar * M_log_player_tool_bar;

    PlayerTypeDialog * M_player_type_dialog;
    ConfigDialog * M_config_dialog;
    FieldCanvas * M_field_canvas;
    MonitorClient * M_monitor_client;
    LogPlayer * M_log_player;
    //LogPlayerSlider * M_log_player_slider;

    QLabel * M_position_label;

    // file actions
    QAction * M_open_act;
    QAction * M_exit_act;

    // monitor actions
    QAction * M_connect_monitor_act;
    QAction * M_connect_monitor_to_act;
    QAction * M_disconnect_monitor_act;

    // referee actions
    QAction * M_kick_off_act;
    QAction * M_yellow_card_act;
    QAction * M_red_card_act;
    QActionGroup * M_playmode_change_act_group;

    // logplayer actions
    QAction * M_live_mode_act;
    QAction * M_log_player_play_or_stop_act;
    QAction * M_log_player_step_backward_act;
    QAction * M_log_player_step_forward_act;
    QAction * M_log_player_decelerate_act;
    QAction * M_log_player_accelerate_act;
    QAction * M_log_player_previous_score_act;
    QAction * M_log_player_next_score_act;
    QAction * M_log_player_go_to_first_act;
    QAction * M_log_player_go_to_last_act;

    // view actions
    QAction * M_toggle_menu_bar_act;
    QAction * M_toggle_tool_bar_act;
    QAction * M_toggle_status_bar_act;
    QAction * M_full_screen_act;
    QAction * M_show_player_type_dialog_act;
    QActionGroup * M_style_act_group;
    QAction * M_show_config_dialog_act;

    // not used
    MainWindow( const MainWindow & ) = delete;
    const MainWindow & operator=( const MainWindow & ) = delete;

public:

    MainWindow();
    ~MainWindow();

    void init();

private:

    void readSettings();
    void writeSettings();

    void createActions();
    void createActionsFile();
    void createActionsMonitor();
    void createActionsReferee();
    void createActionsLogPlayer();
    void createActionsView();
    void createActionsHelp();

    void createMenus();
    void createMenuFile();
    void createMenuMonitor();
    void createMenuReferee();
    void createMenuView();
    void createMenuHelp();

    void createToolBars();
    void createStatusBar();

    void createFieldCanvas();
    void createConfigDialog();

protected:

    // overrided methods
    void closeEvent( QCloseEvent * event );
//     void resizeEvent( QResizeEvent * event );
    void wheelEvent( QWheelEvent * event );

//     void dragEnterEvent( QDragEnterEvent * event );
//     void dropEvent( QDropEvent * event );

private:

    void openGameLogFile( const QString & filepath );
    bool openGameLogFileImpl( const QString & filepath );

    void connectMonitorTo( const char * hostname );

private slots:

    void about();
    void printShortcutKeys();

    void setQuitTimer();

    // file menu action slots
    void openGameLogFile();

    // monitor menu action slots
    void kickOff();
    void connectMonitor(); // connect to the host given by command lien or localhost
    void connectMonitorTo(); // open host input dialog
    void disconnectMonitor();
    void reconnectMonitor();

    // view menu actions slots
    void toggleMenuBar( bool checked );
    void toggleToolBar( bool checked );
    void toggleStatusBar( bool checked );
    void toggleFullScreen();
    void showPlayerTypeDialog();
    void changeStyle( bool checked );
    void showConfigDialog();
    void setFocusPoint( const QPoint & point );

    // context menu action slots
    void dropBall( const QPoint & pos );
    void freeKickLeft( const QPoint & pos );
    void freeKickRight( const QPoint & pos );
    void yellowCard( const char side,
                     const int unum );
    void redCard( const char side,
                  const int unum );
    void yellowCard();
    void redCard();
    void changePlayMode( const int mode );
    void changePlayMode( const int mode,
                         const QPoint & point );

    //
    void receiveMonitorPacket();
    void resizeCanvas( const QSize & size );
    void updatePositionLabel( const QPoint & point );

    //
    void setLiveMode();
    void togglePlayOrStopIcon( bool checked );

signals:

    void viewUpdated();

};

#endif
