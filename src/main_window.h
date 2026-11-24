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

#include <QImage>
#include <QMainWindow>
#include <QPalette>
#include <QString>

#include "camera_mode.h"
#include "disp_holder.h"
#include "log_cache.h"
#include "referee_target.h"

class QActionGroup;
class QComboBox;
class QMimeData;
class QProgressDialog;
class QToolButton;
class QLabel;
class QLineEdit;

class ConfigDialog;
class FieldCanvas;
class FieldView3D;
class QStackedWidget;
class ExportDialog;
class ExportRangeList;
class LogBrowserDialog;
class LogDownloader;
class LogPlayer;
class LogPlayerSlider;
class MonitorClient;
class PlayerTypeDialog;
class RefereeMenu;
class Whistle;

class MainWindow
    : public QMainWindow {

    Q_OBJECT

private:

    QString M_window_style; //!< the key of the style chosen in View > Window Style (empty: the platform's)
    QPalette M_system_palette; //!< the palette of the desktop when the window was made (UIColorScheme::System)

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
    LogPlayerSlider * M_log_player_slider = nullptr; //!< the frames, with the ranges to export and the goals
    Whistle * M_whistle; //!< the referee's whistle at the changes of the play mode of the frame shown
    ExportRangeList * M_export_ranges; //!< the ranges to export: marked here, edited in the export dialog

    LogCache M_log_cache; //!< the game logs downloaded
    LogDownloader * M_log_downloader = nullptr; //!< a game log from a URL (File > Open Game Log from URL)
    QProgressDialog * M_download_dialog = nullptr; //!< while it downloads
    LogBrowserDialog * M_log_browser = nullptr; //!< File > Browse Game Logs, made when it is first shown
    ExportDialog * M_export_dialog = nullptr; //!< File > Export Images / Video, made when it is first shown
    QString M_game_log_path; //!< the file of the game log open (Save a Copy), or empty (none, or live)

    QLabel * M_position_label;

    // file actions
    QAction * M_open_act;
    QAction * M_open_url_act = nullptr; //!< File > Open Game Log from URL
    QAction * M_browse_logs_act = nullptr; //!< File > Browse Game Logs
    QAction * M_save_copy_act = nullptr; //!< File > Save a Copy of Game Log: enabled while a game log is open
    QAction * M_save_image_act = nullptr; //!< File > Save Image of This Frame: the 2D view (disabled while the 3D view is shown)
    QAction * M_export_act = nullptr; //!< File > Export Images / Video: enabled with frames, not connected
    QAction * M_mark_start_act = nullptr; //!< File > Export Range > Mark the Start ([): as the export
    QAction * M_mark_end_act = nullptr; //!< File > Export Range > Mark the End (])
    QAction * M_clear_marks_act = nullptr; //!< File > Export Range > Clear the Marks
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
    QAction * M_toggle_whistle_act = nullptr; //!< View > Referee's Whistle (the option whistle)
    QAction * M_show_player_type_dialog_act;
    QActionGroup * M_style_act_group;
    QActionGroup * M_ui_color_scheme_act_group = nullptr; //!< View > UI Color Scheme
    QAction * M_show_config_dialog_act;

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

    //! the tips of the delayed live view's action, with its seconds
    void updateLiveDelayTips();

    /*!
      \brief set the palette of the color scheme in the options to the
      application, and check it in the menu. Called when the window starts,
      when the scheme is chosen, and when the style changes (which may set
      the style's palette).
    */
    void applyUIColorScheme();
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

    //! a game log file or URL dragged over the window
    void dragEnterEvent( QDragEnterEvent * event ) override;
    void dragMoveEvent( QDragMoveEvent * event ) override;
    //! a game log file or URL dropped: opened
    void dropEvent( QDropEvent * event ) override;

private:

    bool openGameLogFileImpl( const QString & filepath );

public:

    //! open a game log file (File > Open Game Log, the command line, a drop, a download)
    void openGameLogFile( const QString & filepath );

    /*!
      \brief the game log in dropped data: the first URL, a local file's path
      or an http:// or https:// URL; or a text that is such a URL. Empty if
      none.
    */
    static QString droppedGameLog( const QMimeData * data );

    /*!
      \brief open a game log from a URL (docs/plan/2026-11-22-02_remote-logs.md
      4.1): from the cache if it was downloaded before, or downloaded into the
      cache with a progress dialog (Cancel), and then opened. A URL that cannot
      be opened is told in a message box.
      \param text the URL, as given (LogUrl::parse())
    */
    void openGameLogUrl( const QString & text );

    /*!
      \brief copy the file of the game log open to a path (File > Save a Copy
      of Game Log). The file is kept as it is (e.g. compressed).
      \return false if no game log is open or it cannot be copied
    */
    bool saveGameLogCopy( const QString & path );

    /*!
      \brief draw the frame shown in the 2D view into an image, as large as
      the view on the screen, with its device pixel ratio
      (docs/plan/2026-11-22-03_frame-export.md 4.6)
      \return the image, or a null image if there is no frame
    */
    QImage renderImage() const;

    //! the file name of the image of the frame shown: the game log's name and the cycle
    QString defaultImageName() const;

    /*!
      \brief save the frame shown in the 2D view to an image file (File >
      Save Image of This Frame). The format is by the extension (PNG if there is none).
      \param error set to the reason if it fails (may be null)
      \return false if there is no frame, or the file cannot be written
    */
    bool saveImage( const QString & path,
                    QString * error = nullptr ) const;

    //! the dialog of File > Export Images / Video, or null before it is first shown
    ExportDialog * exportDialog() const { return M_export_dialog; }

    //! the ranges to export
    ExportRangeList * exportRanges() const { return M_export_ranges; }

    //! the slider of the frames
    LogPlayerSlider * logPlayerSlider() const { return M_log_player_slider; }

    //! the cache of the downloaded game logs
    const LogCache & gameLogCache() const { return M_log_cache; }

private:

    //! stop a download going on, and close its dialog
    void cancelDownload();

    //! use the camera of the 3D view: in the options, the menu, the tool bar and the view
    void setCamera3D( const CameraMode mode );

    //! enable or disable the camera actions with the 3D view
    void enableCamera3D( const bool on );

    //! the 3D view if it is shown, or null
    FieldView3D * shownView3D() const;

    //! the keyboard focus to the view shown: the 2D view, or the views' stack (the 3D view's window takes none)
    void focusView();

    //! File > Save Image of This Frame: enabled while the 2D view is shown (D5 of docs/plan/2026-11-22-03_frame-export.md)
    void updateSaveImageAction();

    //! File > Export Images / Video: enabled with frames, while not connected to a server (S1)
    void updateExportAction();

    //! the bands of the ranges to export and the ticks of the goals on the slider (D9)
    void updateRangeMarks();

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
    //! the referee's whistle on or off (View > Referee's Whistle)
    void toggleWhistle( bool on );
    //! the whistle's options changed in the settings dialog
    void applyWhistle();
    //! blow the short whistle at the volume of the options (the settings dialog's Test)
    void tryWhistle();

    void about();
    void printShortcutKeys();

    void setQuitTimer();

    // file menu action slots
    void openGameLogFile();
    //! File > Open Game Log from URL: the dialog
    void openGameLogUrl();
    //! File > Browse Game Logs: the dialog (modeless), made when it is first shown
    void showLogBrowser();
    //! File > Export Images / Video: the dialog (modeless), made when it is first shown
    void showExportDialog();
    //! File > Save a Copy of Game Log: the file dialog
    void saveGameLogCopy();
    //! File > Save Image of This Frame: the frame shown when chosen, then the file dialog
    void saveImage();
    //! the download of a game log: its progress, its end
    void showDownloadProgress( qint64 received,
                               qint64 total );
    void openDownloadedLog( const QString & path );
    void showDownloadError( const QString & message );

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
    //! View > UI Color Scheme: use the scheme chosen, and save it
    void changeUIColorScheme( QAction * action );
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
