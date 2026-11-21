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

#include <utility>
#include <vector>

#include "camera_mode.h"
#include "disp_holder.h"
#include "referee_target.h"
#include "toolbar_icons.h"

class QActionGroup;
class QComboBox;
class QToolButton;
class QLabel;
class QLineEdit;

class ConfigDialog;
class FieldCanvas;
class FieldView3D;
class QStackedWidget;
class LogPlayer;
class LogPlayerSlider;
class MonitorClient;
class PlayerTypeDialog;
class RefereeMenu;

class MainWindow
    : public QMainWindow {

    Q_OBJECT

private:

    QString M_window_style;

    DispHolder M_disp_holder;

    QToolBar * M_log_player_tool_bar; //!< the buttons
    QToolBar * M_position_tool_bar = nullptr; //!< the slider, the frame label, the cycle field, the time per frame: in a second row when the window is narrow

    PlayerTypeDialog * M_player_type_dialog;
    ConfigDialog * M_config_dialog;
    FieldCanvas * M_field_canvas;
    QStackedWidget * M_view_stack; //!< the 2D view and the 3D view
    QWidget * M_field_view_3d; //!< the container of the 3D view's window, made when it is first shown
    FieldView3D * M_view_3d = nullptr; //!< the 3D view: a window in M_field_view_3d
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
    RefereeMenu * M_referee_menu = nullptr; //!< the menu of a right click on the field, in 2D and 3D

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
    QAction * M_loop_act = nullptr; //!< loop playback (the option auto_loop_mode)
    QAction * M_live_delay_act = nullptr; //!< the delayed live view (the option live_delay, §5.10)

    // the tool bar's fields after the slider (as the web version's)
    QLabel * M_frame_label = nullptr; //!< "cycle (frame / frames)"
    QLineEdit * M_cycle_edit = nullptr; //!< a cycle to go to, with Enter
    QLabel * M_interval_label = nullptr; //!< the time per frame while playing

    // view actions
    QAction * M_toggle_menu_bar_act;
    QAction * M_toggle_tool_bar_act;
    QAction * M_toggle_status_bar_act;
    QAction * M_toggle_view_3d_act;
    QActionGroup * M_camera_3d_act_group = nullptr; //!< the cameras of the 3D view (View > 3D Camera)
    QAction * M_next_camera_3d_act = nullptr; //!< E
    QComboBox * M_camera_3d_combo = nullptr; //!< the cameras in the tool bar, shown with the 3D view
    QActionGroup * M_viewpoint_act_group = nullptr; //!< the viewpoints of the whole pitch (View > 3D Camera)
    QAction * M_next_viewpoint_act = nullptr; //!< Ctrl+I
    QToolButton * M_viewpoint_button = nullptr; //!< the viewpoints in the tool bar, shown with the 3D view
    bool M_viewpoint_shown = false; //!< a viewpoint has been chosen (M_viewpoint), for Ctrl+I
    Viewpoint M_viewpoint = Viewpoint::Side;
    QAction * M_full_screen_act;
    QAction * M_show_player_type_dialog_act;
    QActionGroup * M_style_act_group;
    QAction * M_show_config_dialog_act;

    //! the toolbar actions and their icons (XPM in black), drawn in the
    //! text color of the palette
    std::vector< std::pair< QAction *, const char * const * > > M_tool_icons;
    std::vector< std::pair< QAction *, DrawnIcon > > M_drawn_icons; //!< the icons drawn in code (toolbar_icons.h)

    // not used
    MainWindow( const MainWindow & ) = delete;
    const MainWindow & operator=( const MainWindow & ) = delete;

public:

    MainWindow();
    ~MainWindow();

    void init();

    /*!
      \brief the margins of the views' stack: a pixel at the bottom when the
      3D view is shown and the stack covers all of a full screen window, or
      none. Covering all of the screen, the 3D view's window stopped being
      drawn (GNOME, Qt 6.4; on Wayland, and through XWayland at times): Qt
      waited forever for the compositor's word to draw the next frame, as
      if the compositor put that window straight on the screen. With a
      pixel of the main window shown, the compositor puts them together as
      usual.
      \param view_3d the 3D view shown
      \param full_screen the window full screen
      \param views the stack's geometry in the window
      \param window the window's rectangle
    */
    static QMargins viewMargins( const bool view_3d,
                                 const bool full_screen,
                                 const QRect & views,
                                 const QRect & window );

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

    void setToolIcon( QAction * act,
                      const char * const xpm[] );
    //! the icon of a toolbar action drawn in code, drawn again when the palette changes
    //! the tips of the delayed live view's action, with its seconds
    void updateLiveDelayTips();

    void setDrawnIcon( QAction * act,
                       const DrawnIcon icon );
    void updateToolIcons();
    /*!
      \brief the position tool bar after the buttons in one row, or in a
      second row below them when the window is too narrow for both (as the
      web version's tool bar wraps)
    */
    void updateToolBarRows();

protected:

    // overrided methods
    void changeEvent( QEvent * event ) override;
    //! Esc in the tool bar's cycle field gives the keys back to the field
    bool eventFilter( QObject * watched,
                      QEvent * event ) override;
    void closeEvent( QCloseEvent * event );
    //! the tool bars in one row or two, as the window's width allows
    void resizeEvent( QResizeEvent * event ) override;
    void wheelEvent( QWheelEvent * event );

//     void dragEnterEvent( QDragEnterEvent * event );
//     void dropEvent( QDropEvent * event );

private:

    void openGameLogFile( const QString & filepath );
    bool openGameLogFileImpl( const QString & filepath );

    //! use the camera of the 3D view: in the options, the menu, the tool bar and the view
    void setCamera3D( const CameraMode mode );

    //! enable or disable the camera actions with the 3D view
    void enableCamera3D( const bool on );

    //! the 3D view if it is shown, or null
    FieldView3D * shownView3D() const;

    //! the keyboard focus to the view shown: the 2D view, or the views' stack (the 3D view's window takes none)
    void focusView();

    //! a pixel of the window below the 3D view when it would cover all of a full screen window (viewMargins())
    void updateViewMargins();

    //! the wheel steps the frames, over the 2D view or the 3D view
    void stepByWheel( const int delta );

    //! put the free camera of the 3D view at a viewpoint of the whole pitch (only says it is for 3D in 2D)
    void viewFrom( const Viewpoint viewpoint );

    void connectMonitorTo( const char * hostname );

    /*!
      \brief ask which player gets a card
      \param title the title of the dialog
      \param initial the player chosen first, if the target has one
      \param side set to 'l' or 'r'
      \param unum set to the uniform number
      \return false if canceled or none was chosen
    */
    bool choosePlayer( const QString & title,
                       const RefereeTarget & initial,
                       char * side,
                       int * unum );

    //! the menu of a right click on the field
    void createRefereeMenu();

private slots:

    //! the tool bar's frame label and time per frame
    void updatePlayerLabels();
    //! go to the cycle typed in the tool bar (the field turns red if the log has none)
    void goToTypedCycle();
    //! loop playback on or off
    void toggleLoop( bool on );
    //! the delayed live view on or off (the tool bar and the menu)
    void toggleLiveDelay( bool on );
    //! the options of the delayed live view changed in the settings dialog
    void applyLiveDelay();

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
    void showMonitorError( const QString & msg );

    // view menu actions slots
    void toggleMenuBar( bool checked );
    void toggleToolBar( bool checked );
    void toggleStatusBar( bool checked );
    void toggleView3D( bool on );
    void selectCamera3D( QAction * action );
    void nextCamera3D();
    void nextViewpoint3D();

    // the zoom of the 2D field, or of the 3D camera when it is shown (Z, X, I)
    void zoomIn();
    void zoomOut();
    void fitToScreen();
    void toggleFullScreen();
    void showPlayerTypeDialog();
    void changeStyle( bool checked );
    void showConfigDialog();
    void setFocusPoint( const QPoint & point );

    // the referee's actions, at points on the field (m)
    void dropBall( const double x,
                   const double y );
    void freeKickLeft( const double x,
                       const double y );
    void freeKickRight( const double x,
                        const double y );
    void yellowCard( const char side,
                     const int unum );
    void redCard( const char side,
                  const int unum );
    // the cards from the Referee menu: the dialog asks which player
    void yellowCard();
    void redCard();
    // the cards from the menu of the field: the dialog has the player aimed at chosen first
    void yellowCard( const RefereeTarget & target );
    void redCard( const RefereeTarget & target );
    // Change Playmode: from the ball (the Referee menu), or at a point
    void changePlayMode( const int mode );
    void changePlayMode( const int mode,
                         const double x,
                         const double y );

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
