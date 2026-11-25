// -*-c++-*-

/*!
  \file options.h
  \brief application setting variables.
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

#ifndef RCSSMONITOR_OPTIONS_H
#define RCSSMONITOR_OPTIONS_H

#include <QColor>
#include <QString>
#include <QStringList>
#include <QPoint>
#include <QPointF>
#include <QPen>
#include <QBrush>
#include <QFont>

#include "camera_mode.h"
#include "player_camera_options.h"
#include "monitor_transport.h"
#include "ui_palette.h"

#include <rcss/rcg/types.h>

#include <string>
#include <cmath>

/*!
  \class Options
  \brief application setting holder singleton.
*/
class Options {
public:

    static const QString CONF_FILE;

    enum PlayerSelectType {
        SELECT_FIX,
        SELECT_AUTO_LEFT,
        SELECT_AUTO_RIGHT,
        SELECT_AUTO_ALL,
        UNSELECT,
    };

    enum FocusType {
        FOCUS_BALL,
        FOCUS_PLAYER,
        FOCUS_POINT, // include center
    };

    static constexpr double PITCH_LENGTH = 105.0;
    static constexpr double PITCH_WIDTH = 68.0;
    static constexpr double PITCH_HALF_LENGTH = PITCH_LENGTH * 0.5;
    static constexpr double PITCH_HALF_WIDTH = PITCH_WIDTH * 0.5;
    static constexpr double PITCH_MARGIN = 5.0;
    static constexpr double CENTER_CIRCLE_R = 9.15;
    static constexpr double PENALTY_AREA_LENGTH = 16.5;
    static constexpr double PENALTY_AREA_WIDTH = 40.32;
    static constexpr double PENALTY_CIRCLE_R = 9.15;
    static constexpr double PENALTY_SPOT_DIST = 11.0;
    static constexpr double GOAL_WIDTH = 14.02;
    static constexpr double GOAL_HALF_WIDTH = GOAL_WIDTH * 0.5;
    static constexpr double GOAL_AREA_LENGTH = 5.5;
    static constexpr double GOAL_AREA_WIDTH = 18.32;
    static constexpr double GOAL_DEPTH = 2.44;
    static constexpr double CORNER_ARC_R = 1.0;
    static constexpr double GOAL_POST_RADIUS = 0.06;


    //! minimum field scale
    static constexpr double MIN_FIELD_SCALE = 1.0;
    //! maximum field scale
    static constexpr double MAX_FIELD_SCALE = 400.0;
    //! zoom rate factor
    static constexpr double ZOOM_RATIO = 1.5;

    /*!
      the delayed live view (docs/plan/2026-11-11_3d-view.md §5.10): the seconds a
      live game is drawn behind its newest frame. The largest is the time
      the ball takes from the top speed (ball_speed_max 3.0) to rolling no
      farther than the kickable distance (6.2 s with ball_decay 0.94), with
      a margin: by then, the touch that ends a kick's segment, or its end
      by the ball's decay, is known before the kick is drawn.
    */
    static constexpr double MIN_LIVE_DELAY_SECONDS = 1.0;
    static constexpr double MAX_LIVE_DELAY_SECONDS = 7.0;
    static constexpr double LIVE_DELAY_SECONDS_STEP = 0.5;
    static constexpr double DEFAULT_LIVE_DELAY_SECONDS = 3.0;

    //! the referee's whistle: the volume, as the slider's (docs/plan/2026-11-22-00_v20-features.md §C)
    static constexpr int MIN_WHISTLE_VOLUME = 0;
    static constexpr int MAX_WHISTLE_VOLUME = 100;
    static constexpr int DEFAULT_WHISTLE_VOLUME = 50;

    //! the URLs of game logs remembered (File > Open Game Log from URL)
    static constexpr int MAX_RECENT_LOG_URLS = 10;

    /*!
      the sources of File > Browse Game Logs at first (the user can change
      them): the logs of RoboCup 2026 on GitHub, and the 2D logs of
      archive.robocup.info (docs/plan/2026-11-22-02_remote-logs.md D11, D13)
    */
    static const QStringList DEFAULT_LOG_SOURCES;

    //! default logplayer timer step (ms).
    static constexpr int DEFAULT_TIMER_INTERVAL = 100;

    static constexpr int WAITING_ANIMATION_SIZE = 96;

    // default color settings
    static const QColor FIELD_COLOR;
    static const QColor LINE_COLOR;
    static const QColor MEASURE_LINE_COLOR;
    static const QColor MEASURE_MARK_COLOR;
    static const QColor MEASURE_FONT_COLOR;
    static const QColor MEASURE_FONT_COLOR2;
    static const QColor SCORE_BOARD_PEN_COLOR;
    static const QColor SCORE_BOARD_BRUSH_COLOR;
    static const QColor BALL_COLOR;
    static const QColor BALL_VEL_COLOR;
    static const QColor PLAYER_PEN_COLOR;
    static const QColor LEFT_TEAM_COLOR;
    static const QColor LEFT_GOALIE_COLOR;
    static const QColor RIGHT_TEAM_COLOR;
    static const QColor RIGHT_GOALIE_COLOR;
    static const QColor PLAYER_NUMBER_COLOR;
    static const QColor PLAYER_NUMBER_INNER_COLOR;
    static const QColor NECK_COLOR;
    static const QColor VIEW_AREA_COLOR;
    static const QColor LARGE_VIEW_AREA_COLOR;
    static const QColor FOCUS_POINT_COLOR;
    static const QColor BALL_COLLIDE_COLOR;
    static const QColor PLAYER_COLLIDE_COLOR;
    static const QColor EFFORT_DECAYED_COLOR;
    static const QColor RECOVERY_DECAYED_COLOR;
    static const QColor KICK_COLOR;
    static const QColor KICK_FAULT_COLOR;
    static const QColor KICK_ACCEL_COLOR;
    static const QColor CATCH_COLOR;
    static const QColor CATCH_FAULT_COLOR;
    static const QColor TACKLE_COLOR;
    static const QColor TACKLE_FAULT_COLOR;
    static const QColor FOUL_CHARGED_COLOR;
    static const QColor POINTTO_COLOR;
    static const QColor ILLEGAL_DEFENSE_COLOR;

private:

    //
    // monitor client options
    //
    bool M_connect;
    std::string M_server_host;
    int M_server_port;
    MonitorTransport::Type M_server_transport;
    std::string M_server_ws_path;
    int M_client_version;

    //
    // monitor/logplayer options
    //
    bool M_auto_quit_mode;
    int M_auto_quit_wait;
    bool M_auto_reconnect_mode;
    int M_auto_reconnect_wait;

    std::string M_game_log_file; //!< the file path of game log file to be opened
    bool M_auto_loop_mode;
    int M_timer_interval; //!< logplayer timer interval
    bool M_live_delay; //!< the delayed live view (§5.10)
    double M_live_delay_seconds; //!< how far behind the newest frame (s)

    //
    // window options
    //
    int M_window_x;
    int M_window_y;
    int M_window_width;
    int M_window_height;
    bool M_maximize;
    bool M_full_screen;

    int M_canvas_width;
    int M_canvas_height;
    //! showTeamGraphic() and showScoreBoard() when the scale was fit to the canvas (updateFieldSize())
    bool M_sized_with_team_graphic;
    bool M_sized_with_score_board;

    bool M_show_menu_bar;
    bool M_view_3d; //!< show the 3D view instead of the 2D view
    CameraMode M_camera_3d; //!< the camera of the 3D view
    double M_closeup_distance_3d; //!< the close-up cameras: how far from what they follow (m)
    double M_closeup_elevation_3d; //!< the close-up cameras: how high above it (degree)
    PlayerCameraOptions M_player_camera; //!< the styles of the selected player's camera
    bool M_boards_3d; //!< the boards around the pitch in the team colors
    GrassMode M_grass_3d; //!< the grass: plain, mowed in stripes, or realistic
    bool M_sky_3d; //!< the sky, and the haze over the far ground
    bool M_stands_3d; //!< the stands along the touch lines
    bool M_ball_pattern_3d; //!< the ball's pattern, which shows it rolling
    bool M_ball_flight_3d; //!< the ball flying in the air in log replay, virtually
    double M_ball_size_3d; //!< the ball's radius in the 3D view (m); below 0.01, the real size
    bool M_ball_ring_3d; //!< a ring on the ground around the ball
    double M_ball_ring_radius_3d; //!< the ring's radius (m)
    bool M_ball_trail_3d; //!< a trail behind the ball
    int M_ball_trail_cycles_3d; //!< the trail: as long as the ball went in this many cycles
    bool M_banners_3d; //!< the banners behind the goals in the team colors
    GuideMode M_compass_3d; //!< when the compass shows over the 3D view
    GuideMode M_minimap_3d; //!< when the minimap shows over the 3D view
    bool M_fps_3d; //!< the frames per second over the 3D view
    bool M_antialias_3d; //!< smooth edges of the 3D view (multisampling)
    QString M_player_model_3d; //!< how the 3D view draws the players: PLAYER_MODEL_SIMPLE, or a figure pack's name
    QString M_figure_folder_3d; //!< a folder of figure packs of the user's choice (empty: none)
    double M_figure_scale_3d; //!< the human figures: how many times a person's height
    FigureBodies M_figure_bodies_3d; //!< the human bodies: men and women, or one of them
    ShadowMode M_shadows_3d; //!< the sun's shadows of the players (and the goal frames when detailed)
    bool M_show_tool_bar;
    bool M_show_status_bar;
    UIColorScheme M_ui_color_scheme; //!< the color scheme of the widgets (View > UI Color Scheme)

    //
    // sound options
    //

    bool M_whistle; //!< the referee's whistle (View > Referee's Whistle)
    int M_whistle_volume; //!< its volume, MIN_WHISTLE_VOLUME to MAX_WHISTLE_VOLUME

    //
    // game logs
    //

    QStringList M_recent_log_urls; //!< the URLs of game logs opened recently, the newest first
    QStringList M_log_sources; //!< the sources of File > Browse Game Logs

public:

    /*!
      \struct ExportSettings
      \brief the settings of File > Export Images / Video, kept in the group
      Export of the settings (docs/plan/2026-11-22-03_frame-export.md S7)
    */
    struct ExportSettings {
        QString directory; //!< where the images are written (empty: the home)
        QString prefix = QStringLiteral( "image-" ); //!< the start of the names of the images
        QString format = QStringLiteral( "png" ); //!< the image format
        QString size = QStringLiteral( "view" ); //!< view, 1280x720, 1920x1080, 3840x2160, or custom
        int width = 1920; //!< the custom size
        int height = 1080;
        bool scale_texts = true; //!< the view enlarged: the texts, the logos and the lines as large in the image as in the view (by the device pixel ratio)
        double fps = 10.0; //!< the images a second (2D)
        double fps_3d = 30.0; //!< the images a second (3D: drawn between the frames)
        double speed = 1.0; //!< how many times as fast as the game is played
        bool name_by_cycle = false; //!< the images named by the cycles (one frame an image)
        bool separate_ranges = true; //!< the ranges each in its own directory, or in one sequence
        double goal_before = 10.0; //!< a goal scene: from these seconds before the goal (D8)
        double goal_after = 5.0; //!< to these seconds after it
        double around_before = 5.0; //!< around the position shown: from these seconds before it
        double around_after = 5.0; //!< to these seconds after it
        bool video = false; //!< a video (by ffmpeg), or images
        QString video_file; //!< the video (empty: rcssmonitor.mp4 in the home)
        int crf = 18; //!< the quality of the video (D15)
        QString preset = QStringLiteral( "medium" ); //!< the speed of the compression (D15)
        QString ffmpeg; //!< the path of ffmpeg (empty: in the PATH)
    };

    /*!
      \struct BatchExportSettings
      \brief the export from the command line (ExportBatch,
      docs/plan/2026-11-22-03_frame-export.md 4.7): the options given, not
      saved in the settings
    */
    struct BatchExportSettings {
        QString video; //!< --export-video FILE
        QString images; //!< --export-images DIR
        bool view_3d = false; //!< --export-view 3d
        QStringList ranges; //!< --export-range, given again for more (none: all)
        bool join = false; //!< --export-join: the ranges in one video (or one sequence of images)
        double fps = 0.0; //!< --export-fps (0: 10 for 2D, 30 for 3D)
        double speed = 1.0; //!< --export-speed
        int width = 1280; //!< --export-size WxH
        int height = 720;
        double scale = 0.0; //!< --export-scale: the device pixel ratio (0: the height / 720)
        QString image_format = QStringLiteral( "png" ); //!< --export-image-format
        QString image_prefix = QStringLiteral( "image-" ); //!< --export-image-prefix
        int crf = 18; //!< --export-crf
        QString preset = QStringLiteral( "medium" ); //!< --export-preset
        QString ffmpeg; //!< --ffmpeg (empty: in the PATH)

        //! an export is asked for
        bool enabled() const { return ! video.isEmpty() || ! images.isEmpty(); }
    };

private:

    ExportSettings M_export_settings;
    BatchExportSettings M_batch_export; //!< from the command line, not saved

    //
    // view options
    //

    bool M_anti_aliasing;

    bool M_show_score_board;
    bool M_show_keepaway_area;
    bool M_show_team_graphic;
    bool M_show_flag;

    bool M_show_ball;
    bool M_show_player;

    bool M_show_player_number;
    bool M_show_player_type;
    bool M_show_view_area;
    bool M_show_focus_point;
    bool M_show_all_focus_points; //!< the focus points of all players, not only of the selected one
    bool M_show_illegal_defense;
    bool M_show_catch_area;
    bool M_show_tackle_area;
    bool M_show_kick_accel_area;
    bool M_show_stamina;
    bool M_show_stamina_capacity;
    bool M_show_pointto;
    bool M_show_card;

    bool M_show_offside_line;
    bool M_show_draw_info;

    double M_ball_size; //!< the ball's radius in the 2D view; below 0.01, the real size
    double M_player_size; //!< fixed player radius
    double M_focus_point_size; //!< fixed focus point radius

    double M_grid_step;
    bool M_show_grid_coord;

    double M_team_graphic_scale;

    // zoom
    double M_field_scale;
    bool M_zoomed;
    QPoint M_field_center; //!< field center point on the screen

    // focus
    FocusType M_focus_type; //!< focus type ID
    QPointF M_focus_point; //!< real coordinates

    // player selection
    int M_selected_number; //!< selected player's uniform number.
    PlayerSelectType M_player_select_type; //!< flag for player auto selection

    // inertia movement
    int M_ball_vel_cycle; //!< specify the cycle to draw ball future point

    //
    // painter resources
    //

    // pen, brush
    QBrush M_field_brush;
    QPen M_line_pen;

    QPen M_measure_line_pen;
    QPen M_measure_mark_pen;
    QPen M_measure_font_pen;
    QPen M_measure_font_pen2;

    QPen M_score_board_pen;
    QBrush M_score_board_brush;

    QPen M_ball_pen;
    QBrush M_ball_brush;
    QPen M_ball_vel_pen;

    QPen M_player_pen;
    QPen M_selected_player_pen;
    QPen M_left_team_pen;
    QBrush M_left_team_brush;
    QPen M_left_goalie_pen;
    QPen M_left_goalie_stretch_pen;
    QBrush M_left_goalie_brush;
    QPen M_right_team_pen;
    QBrush M_right_team_brush;
    QPen M_right_goalie_pen;
    QPen M_right_goalie_stretch_pen;
    QBrush M_right_goalie_brush;
    QPen M_player_number_pen;
    QPen M_player_number_inner_pen;
    QPen M_neck_pen;
    QPen M_view_area_pen;
    QPen M_large_view_area_pen;
    QPen M_focus_point_pen;
    QBrush M_ball_collide_brush;
    QBrush M_player_collide_brush;
    QPen M_effort_decayed_pen;
    QPen M_recovery_decayed_pen;
    QPen M_kick_pen;
    QBrush M_kick_fault_brush;
    QPen M_kick_accel_pen;
    QBrush M_catch_brush;
    QBrush M_catch_fault_brush;
    QPen M_tackle_pen;
    QBrush M_tackle_brush;
    QBrush M_tackle_fault_brush;
    QBrush M_foul_charged_brush;
    QPen M_pointto_pen;
    QPen M_illegal_defense_pen;


    // font
    QFont M_score_board_font;
    QFont M_player_font;
    QFont M_measure_font;

    //! private access for singleton
    Options();

    //! noncopyable
    Options( const Options & );
    //! noncopyable
    Options & operator=( const Options & );

    void readSettings();
    void writeSettings( bool all );

    void checkConsistensy();
    void setGeometry( const std::string & val );

public:
    ~Options();

    static
    Options & instance();

    /*!
      \biref analyze command line options
    */
    bool parseCmdLine( int argc,
                       char ** argv );


    void setDefaultColor();

    //
    // monitor options
    //

    bool connect() const { return M_connect; }

    const std::string & serverHost() const { return M_server_host; }
    void setServerHost( const std::string & host ) { M_server_host = host; }

    int serverPort() const { return M_server_port; }
    void setServerPort( const int port ) { M_server_port = port; }

    MonitorTransport::Type serverTransport() const { return M_server_transport; }
    void setServerTransport( const MonitorTransport::Type type ) { M_server_transport = type; }

    const std::string & serverWSPath() const { return M_server_ws_path; }
    void setServerWSPath( const std::string & path ) { M_server_ws_path = path; }

    int clientVersion() const { return M_client_version; }

    bool autoQuitMode() const { return M_auto_quit_mode; }
    int autoQuitWait() const { return M_auto_quit_wait; }

    bool autoReconnectMode() const { return M_auto_reconnect_mode; }
    int autoReconnectWait() const { return M_auto_reconnect_wait; }

    bool autoLoopMode() const { return M_auto_loop_mode; }
    //! play the log again from the start at its end (and from the end stepping back before the start)
    void setAutoLoopMode( const bool on ) { M_auto_loop_mode = on; }

    const std::string & gameLogFile() const { return M_game_log_file; }
    //! the game log to open at the start (MainWindow::init(); the command line gives it, and the tests)
    void setGameLogFile( const std::string & path ) { M_game_log_file = path; }

    int timerInterval() const { return M_timer_interval; }

    //! the delayed live view: a live game drawn liveDelaySeconds() behind its newest frame (§5.10)
    bool liveDelay() const { return M_live_delay; }
    void setLiveDelay( const bool on ) { M_live_delay = on; }
    double liveDelaySeconds() const { return M_live_delay_seconds; }
    //! within MIN_LIVE_DELAY_SECONDS and MAX_LIVE_DELAY_SECONDS, to a step of LIVE_DELAY_SECONDS_STEP
    void setLiveDelaySeconds( const double seconds );

    //
    // window option
    //

    int windowX() const { return M_window_x; }
    int windowY() const { return M_window_y; }
    int windowWidth() const { return M_window_width; }
    int windowHeight() const { return M_window_height; }

    bool maximize() const { return M_maximize; }
    bool fullScreen() const { return M_full_screen; }

    int canvasWidth() const { return M_canvas_width; }
    int canvasHeight() const { return M_canvas_height; }

    bool showMenuBar() const { return M_show_menu_bar; }
    bool view3D() const { return M_view_3d; }
    void setView3D( const bool on ) { M_view_3d = on; }
    CameraMode camera3D() const { return M_camera_3d; }
    void setCamera3D( const CameraMode mode ) { M_camera_3d = mode; }

    // the close-up cameras of the 3D view (rcssmonitor-web cameras.ts CLOSEUP_RANGE)
    static constexpr double CLOSEUP_DISTANCE_3D = 12.0; //!< the default (m)
    static constexpr double MIN_CLOSEUP_DISTANCE_3D = 3.0;
    static constexpr double MAX_CLOSEUP_DISTANCE_3D = 60.0;
    static constexpr double CLOSEUP_ELEVATION_3D = 20.0; //!< the default (degree)
    static constexpr double MIN_CLOSEUP_ELEVATION_3D = 5.0;
    static constexpr double MAX_CLOSEUP_ELEVATION_3D = 85.0;
    double closeupDistance3D() const { return M_closeup_distance_3d; }
    //! within MIN_CLOSEUP_DISTANCE_3D and MAX_CLOSEUP_DISTANCE_3D
    void setCloseupDistance3D( const double distance );
    double closeupElevation3D() const { return M_closeup_elevation_3d; }
    //! within MIN_CLOSEUP_ELEVATION_3D and MAX_CLOSEUP_ELEVATION_3D
    void setCloseupElevation3D( const double elevation );

    //! the styles of the selected player's camera of the 3D view
    const PlayerCameraOptions & playerCamera() const { return M_player_camera; }
    //! the numbers are kept within their ranges (PlayerCameraStyle)
    void setPlayerCamera( const PlayerCameraOptions & options );

    // the defaults of the 3D tab of the settings dialog
    static constexpr bool BOARDS_3D = true;
    static constexpr bool BANNERS_3D = true;
    static constexpr GuideMode COMPASS_3D = GuideMode::Off;
    static constexpr GuideMode MINIMAP_3D = GuideMode::Off;
    /*!
      \brief the settings of the 3D tab of the settings dialog back to their
      defaults (its Default button): the compass, the minimap, the frames per
      second, the smooth edges, the shadows, and the stadium (the boards, the
      banners, the stands, the sky, the grass)
    */
    void setView3DDefaults();
    /*!
      \brief the settings of the Players & Ball tab back to their defaults:
      the players' model, the figures' bodies and scale, and the ball (its
      pattern, its flight, its ring and trail)
    */
    void setPlayersAndBall3DDefaults();
    /*!
      \brief the settings of the Camera tab back to their defaults: the
      close-up cameras and the player camera
    */
    void setCamera3DDefaults();

    //! the 3D view: the boards around the pitch, in the team colors
    bool boards3D() const { return M_boards_3d; }
    void setBoards3D( const bool on ) { M_boards_3d = on; }

    static constexpr GrassMode GRASS_3D = GrassMode::Stripes;
    GrassMode grass3D() const { return M_grass_3d; }
    void setGrass3D( const GrassMode mode ) { M_grass_3d = mode; }

    static constexpr bool SKY_3D = true;
    bool sky3D() const { return M_sky_3d; }
    void setSky3D( const bool on ) { M_sky_3d = on; }

    static constexpr bool STANDS_3D = true;
    bool stands3D() const { return M_stands_3d; }
    void setStands3D( const bool on ) { M_stands_3d = on; }

    static constexpr bool FPS_3D = false;
    //! the 3D view: the frames per second and the GPU's time of a frame over it (FpsMeter)
    bool fps3D() const { return M_fps_3d; }
    void setFps3D( const bool on ) { M_fps_3d = on; }

    static constexpr bool ANTIALIAS_3D = true;
    //! the samples of a pixel with smooth edges
    static constexpr int ANTIALIAS_SAMPLES_3D = 4;
    //! the 3D view: smooth edges (multisampling); off, a GPU has less to do (a screen of high resolution)
    bool antialias3D() const { return M_antialias_3d; }
    void setAntialias3D( const bool on ) { M_antialias_3d = on; }

    static constexpr bool BALL_PATTERN_3D = true;
    //! the 3D view: the ball's pattern, twelve pentagons, which shows it rolling
    bool ballPattern3D() const { return M_ball_pattern_3d; }
    void setBallPattern3D( const bool on ) { M_ball_pattern_3d = on; }

    /*!
      the ball's radius in the 3D view by default: below 0.01, the real size
      (the server's ball_size), as the ball aids find it where it is small.
      The 2D view has its own (ballSize()).
    */
    static constexpr double BALL_SIZE_3D = 0.0;
    static constexpr double MIN_BALL_SIZE_3D = 0.0;
    static constexpr double MAX_BALL_SIZE_3D = 1.0;
    double ballSize3D() const { return M_ball_size_3d; }
    //! within MIN_BALL_SIZE_3D and MAX_BALL_SIZE_3D
    void setBallSize3D( const double radius );

    static constexpr bool BALL_FLIGHT_3D = false;
    //! the 3D view: in log replay, the ball flies in the air, virtually (BallFlight; not live)
    bool ballFlight3D() const { return M_ball_flight_3d; }
    void setBallFlight3D( const bool on ) { M_ball_flight_3d = on; }

    static constexpr bool BALL_RING_3D = true;
    //! the 3D view: a ring on the ground around the ball, to find it where it is small
    bool ballRing3D() const { return M_ball_ring_3d; }
    void setBallRing3D( const bool on ) { M_ball_ring_3d = on; }

    //! the ring's radius by default: the default kickable_margin (m)
    static constexpr double BALL_RING_RADIUS_3D = 0.7;
    static constexpr double MIN_BALL_RING_RADIUS_3D = 0.1;
    static constexpr double MAX_BALL_RING_RADIUS_3D = 5.0;
    double ballRingRadius3D() const { return M_ball_ring_radius_3d; }
    //! within MIN_BALL_RING_RADIUS_3D and MAX_BALL_RING_RADIUS_3D
    void setBallRingRadius3D( const double radius );

    static constexpr bool BALL_TRAIL_3D = true;
    //! the 3D view: a trail behind the ball like a contrail, through its last cycles
    bool ballTrail3D() const { return M_ball_trail_3d; }
    void setBallTrail3D( const bool on ) { M_ball_trail_3d = on; }

    static constexpr int BALL_TRAIL_CYCLES_3D = 5;
    static constexpr int MIN_BALL_TRAIL_CYCLES_3D = 1;
    static constexpr int MAX_BALL_TRAIL_CYCLES_3D = 30;
    int ballTrailCycles3D() const { return M_ball_trail_cycles_3d; }
    //! within MIN_BALL_TRAIL_CYCLES_3D and MAX_BALL_TRAIL_CYCLES_3D
    void setBallTrailCycles3D( const int cycles );
    //! the 3D view: the banners high behind the goals, with the team names and logos
    bool banners3D() const { return M_banners_3d; }
    void setBanners3D( const bool on ) { M_banners_3d = on; }
    //! the 3D view: when the compass (a strip of the directions around the camera) shows
    GuideMode compass3D() const { return M_compass_3d; }
    void setCompass3D( const GuideMode mode ) { M_compass_3d = mode; }
    /*!
      the 3D view: how the players are drawn, cylinders and spheres
      (PLAYER_MODEL_SIMPLE, the default) or the human figures of a figure
      pack (its name: FigurePacks)
    */
    static constexpr const char * PLAYER_MODEL_3D = PLAYER_MODEL_SIMPLE;
    const QString & playerModel3D() const { return M_player_model_3d; }
    //! as normalizedPlayerModel(): empty for the cylinders, the old name of the Mannequin
    void setPlayerModel3D( const QString & model ) { M_player_model_3d = normalizedPlayerModel( model ); }
    //! whether the players are drawn as human figures (of a figure pack), not as cylinders
    bool figures3D() const { return M_player_model_3d != QLatin1String( PLAYER_MODEL_SIMPLE ); }
    //! the 3D view: a folder of figure packs of the user's choice, besides the bundled ones and the user's data folder (empty: none)
    const QString & figureFolder3D() const { return M_figure_folder_3d; }
    void setFigureFolder3D( const QString & folder ) { M_figure_folder_3d = folder; }
    //! the 3D view: the human figures are this many times a person's height (larger to be seen from afar)
    static constexpr double FIGURE_SCALE_3D = 1.0;
    static constexpr double MIN_FIGURE_SCALE_3D = 0.8;
    static constexpr double MAX_FIGURE_SCALE_3D = 1.5;
    double figureScale3D() const { return M_figure_scale_3d; }
    //! within MIN_FIGURE_SCALE_3D and MAX_FIGURE_SCALE_3D
    void setFigureScale3D( const double scale );
    //! the 3D view: the human bodies of men and women mixed, or of one of them (of a figure pack with both: the Human)
    static constexpr FigureBodies FIGURE_BODIES_3D = FigureBodies::Mixed;
    FigureBodies figureBodies3D() const { return M_figure_bodies_3d; }
    void setFigureBodies3D( const FigureBodies bodies ) { M_figure_bodies_3d = bodies; }

    static constexpr ShadowMode SHADOWS_3D = ShadowMode::Simple; //!< light on any GPU (the user, 2026-11-15)
    ShadowMode shadows3D() const { return M_shadows_3d; }
    void setShadows3D( const ShadowMode mode ) { M_shadows_3d = mode; }
    //! the 3D view: when the minimap (the pitch with the camera's view) shows
    GuideMode minimap3D() const { return M_minimap_3d; }
    void setMinimap3D( const GuideMode mode ) { M_minimap_3d = mode; }
    bool showToolBar() const { return M_show_tool_bar; }
    bool showStatusBar() const { return M_show_status_bar; }
    UIColorScheme uiColorScheme() const { return M_ui_color_scheme; }
    void setUIColorScheme( const UIColorScheme scheme ) { M_ui_color_scheme = scheme; }
    /*!
      \brief write the color scheme to the settings file now. Called when it
      is chosen in the menu: the settings written when the program ends do
      not have it, so that the one given by the command line
      (--ui-color-scheme) is not saved.
    */
    void writeUIColorScheme() const;

    //
    // sound options
    //

    /*!
      \brief the referee's whistle at the changes of the play mode (off by
      default: many monitors at a venue would whistle at once)
    */
    bool whistle() const { return M_whistle; }
    void setWhistle( const bool on ) { M_whistle = on; }
    //! the whistle's volume (the loudness heard), MIN_WHISTLE_VOLUME to MAX_WHISTLE_VOLUME
    int whistleVolume() const { return M_whistle_volume; }
    //! within MIN_WHISTLE_VOLUME and MAX_WHISTLE_VOLUME
    void setWhistleVolume( const int volume );
    /*!
      \brief write the whistle's on or off to the settings file now, and
      writeWhistleVolume() its volume. Called when they are chosen in the
      menu or the settings dialog: the settings written when the program
      ends do not have them, so that the ones given by the command line
      (--whistle, --whistle-volume) are not saved.
    */
    void writeWhistle() const;
    void writeWhistleVolume() const;

    //
    // game logs
    //

    //! the URLs of game logs opened recently, the newest first (at most MAX_RECENT_LOG_URLS)
    const QStringList & recentLogUrls() const { return M_recent_log_urls; }
    //! a URL opened: the newest of the recent ones (once)
    void addRecentLogUrl( const QString & url );
    //! write the recent URLs to the settings file now (called when one is opened)
    void writeRecentLogUrls() const;

    /*!
      the sources of File > Browse Game Logs, as the user enters them
      (OWNER/REPO[@REF] on GitHub, the URL of a directory listed by its
      web server);
      DEFAULT_LOG_SOURCES if not in the settings
    */
    const QStringList & logSources() const { return M_log_sources; }
    void setLogSources( const QStringList & sources ) { M_log_sources = sources; }
    //! write them to the settings file now (called when one is added or removed)
    void writeLogSources() const;

    //
    // export
    //

    const ExportSettings & exportSettings() const { return M_export_settings; }
    void setExportSettings( const ExportSettings & settings ) { M_export_settings = settings; }

    //! the export from the command line (ExportBatch)
    const BatchExportSettings & batchExport() const { return M_batch_export; }
    //! write them to the settings file now (called when an export starts)
    void writeExportSettings() const;

    //
    // view options
    //

    bool antiAliasing() const { return M_anti_aliasing; }
    void toggleAntiAliasing() { M_anti_aliasing = ! M_anti_aliasing; }

    bool showScoreBoard() const { return M_show_score_board; }
    void toggleShowScoreBoard() { M_show_score_board = ! M_show_score_board; }

    bool showKeepawayArea() const { return M_show_keepaway_area; }
    void toggleShowKeepawayArea() { M_show_keepaway_area = ! M_show_keepaway_area; }

    bool showTeamGraphic() const { return M_show_team_graphic; }
    void toggleShowTeamGraphic() { M_show_team_graphic = ! M_show_team_graphic; }

    bool showFlag() const { return M_show_flag; }
    void toggleShowFlag() { M_show_flag = ! M_show_flag; }

    bool showBall() const { return M_show_ball; }
    void toggleShowBall() { M_show_ball = ! M_show_ball; }

    bool showPlayer() const { return M_show_player; }
    void toggleShowPlayer() { M_show_player = ! M_show_player; }

    bool showPlayerNumber() const { return M_show_player_number; }
    void toggleShowPlayerNumber() { M_show_player_number = ! M_show_player_number; }

    bool showPlayerType() const { return M_show_player_type; }
    void toggleShowPlayerType() { M_show_player_type = ! M_show_player_type; }

    bool showViewArea() const { return M_show_view_area; }
    void toggleShowViewArea() { M_show_view_area = ! M_show_view_area; }

    bool showFocusPoint() const { return M_show_focus_point; }
    void toggleShowFocusPoint() { M_show_focus_point = ! M_show_focus_point; }

    //! with showFocusPoint(), the other players' focus points too (smaller, in their team colors)
    bool showAllFocusPoints() const { return M_show_all_focus_points; }
    void toggleShowAllFocusPoints() { M_show_all_focus_points = ! M_show_all_focus_points; }

    bool showIllegalDefense() const { return M_show_illegal_defense; }
    void toggleShowIllegalDefense() { M_show_illegal_defense = ! M_show_illegal_defense; }

    bool showCatchArea() const { return M_show_catch_area; }
    void toggleShowCatchArea() { M_show_catch_area = ! M_show_catch_area; }

    bool showTackleArea() const { return M_show_tackle_area; }
    void toggleShowTackleArea() { M_show_tackle_area = ! M_show_tackle_area; }

    bool showKickAccelArea() const { return M_show_kick_accel_area; }
    void toggleShowKickAccelArea() { M_show_kick_accel_area = ! M_show_kick_accel_area; }

    bool showStamina() const { return M_show_stamina; }
    void toggleShowStamina() { M_show_stamina = ! M_show_stamina; }

    bool showStaminaCapacity() const { return M_show_stamina_capacity; }
    void toggleShowStaminaCapacity() { M_show_stamina_capacity = ! M_show_stamina_capacity; }

    bool showPointto() const { return M_show_pointto; }
    void toggleShowPointto() { M_show_pointto = ! M_show_pointto; }

    bool showCard() const { return M_show_card; }
    void toggleShowCard() { M_show_card = ! M_show_card; }

    bool showOffsideLine() const { return M_show_offside_line; }
    void toggleShowOffsideLine() { M_show_offside_line = ! M_show_offside_line; }

    bool showDrawInfo() const { return M_show_draw_info; }
    void toggleShowDrawInfo() { M_show_draw_info = ! M_show_draw_info; }

    //! the ball's radius in the 2D view (the 3D view has its own: ballSize3D())
    const double & ballSize() const { return M_ball_size; }
    void setBallSize( const double & value )
      {
        if ( value <= 0.001 ) return;
          M_ball_size = value;
      }

    const double & playerSize() const { return M_player_size; }
    void setPlayerSize( const double & value )
      {
          if ( value < 0.0 ) return;
          M_player_size = value;
      }

    double focusPointSize() const { return M_focus_point_size; }
    void setFocusPointSize( const double value )
      {
          if ( value < 0.1 ) return;
          M_focus_point_size = value;
      }

    const double & gridStep() const { return M_grid_step; }
    void setGridStep( const double & value ) { M_grid_step = value; }

    bool showGridCoord() const { return M_show_grid_coord; }
    void toggleShowGridCoord() { M_show_grid_coord = ! M_show_grid_coord; }

    double teamGraphicScale() const { return M_team_graphic_scale; }
    void setTeamGraphicScale( const double value )
      {
          M_team_graphic_scale = value;
          if ( value < 0.1 ) M_team_graphic_scale = 0.1;
          if ( value > 16.0 ) M_team_graphic_scale = 16.0;
      }

    // field scale

    void updateFieldSize( const int canvas_width,
                          const int canvas_height );

    const double & fieldScale() const { return M_field_scale; }
    void setFieldScale( const double & value );

    bool zoomed() const { return M_zoomed; }
    void zoomIn();
    void zoomOut();
    void fitToScreen();

    const QPoint & fieldCenter() const { return M_field_center;}

    /*!
      \brief the scale (pixels per meter) and the center (pixels) that map
      the field to the screen
    */
    struct FieldTransform {
        double scale;
        QPoint center;
    };
    FieldTransform fieldTransform() const { return FieldTransform{ M_field_scale, M_field_center }; }
    /*!
      \brief set the mapping of the field for a while, e.g. to draw the field
      into an image (the ground of the 3D view), and set it back afterward.
      Unlike setFieldScale(), this does not zoom.
    */
    void setFieldTransform( const FieldTransform & t )
      {
          M_field_scale = t.scale;
          M_field_center = t.center;
      }

    /*!
      \brief the state of the 2D view that drawing a frame changes
      (docs/plan/2026-11-22-03_frame-export.md 4.1, S2): the mapping of the
      field and the size of the canvas, with the items shown when the scale
      was fit to it (updateFieldSize()), the focus point and the player
      selected automatically (FieldCanvas::updateFocus()).
      Whether it is zoomed and the kinds of the focus and the selection do
      not change when drawing, and are not in it.
    */
    struct ViewState {
        double field_scale;
        QPoint field_center;
        int canvas_width;
        int canvas_height;
        bool sized_with_team_graphic;
        bool sized_with_score_board;
        QPointF focus_point;
        int selected_number;
    };
    ViewState viewState() const
      {
          return ViewState{ M_field_scale, M_field_center,
                            M_canvas_width, M_canvas_height,
                            M_sized_with_team_graphic, M_sized_with_score_board,
                            M_focus_point, M_selected_number };
      }
    //! set the state back, e.g. after drawing a frame into an image (FrameScope)
    void setViewState( const ViewState & s )
      {
          M_field_scale = s.field_scale;
          M_field_center = s.field_center;
          M_canvas_width = s.canvas_width;
          M_canvas_height = s.canvas_height;
          M_sized_with_team_graphic = s.sized_with_team_graphic;
          M_sized_with_score_board = s.sized_with_score_board;
          M_focus_point = s.focus_point;
          M_selected_number = s.selected_number;
      }

    FocusType focusType() const { return M_focus_type; }
    void setFocusType( const Options::FocusType type ) { M_focus_type = type; }

    const QPointF & focusPoint() const { return M_focus_point; }
    void setFocusPoint( const int screen_x,
                        const int screen_y );
    void setFocusPointReal( const double & x,
                            const double & y )
      {
          M_focus_point.setX( x );
          M_focus_point.setY( y );
      }

    int scale( const double & len ) const
      {
          return static_cast< int >( ::rint( len * M_field_scale ) );
      }

    int screenX( const double & x ) const
      {
          return M_field_center.x() + scale( x );
      }

    int screenY( const double & y ) const
      {
          return M_field_center.y() + scale( y );
      }

    double fieldX( const int x ) const
      {
          return ( x - M_field_center.x() ) / M_field_scale;
      }

    double fieldY( const int y ) const
      {
          return ( y - M_field_center.y() ) / M_field_scale;
      }


    int selectedNumber() const { return M_selected_number; }
    void setSelectedNumber( const rcss::rcg::Side side,
                            const int unum )
      {
          M_selected_number = ( side == rcss::rcg::LEFT ? unum : -unum );
      }
    bool selectedPlayer( const rcss::rcg::Side side,
                         const int unum ) const
      {
          return ( M_selected_number
                   == ( side == rcss::rcg::LEFT ? unum : -unum ) );
      }

    PlayerSelectType playerSelectType() const { return M_player_select_type; }
    bool playerAutoSelect() const
      {
          return ( M_player_select_type != SELECT_FIX
                   && M_player_select_type != UNSELECT );
      }
    void unselectPlayer();
    void setPlayerSelectType( const Options::PlayerSelectType type );

    // ball move

    int ballVelCycle() const { return M_ball_vel_cycle; }
    void setBallVelCycle( const int cycle )
      {
          if ( 0 <= cycle && cycle <= 100 ) M_ball_vel_cycle = cycle;
      }

    //
    //
    //

    const QBrush & fieldBrush() const { return M_field_brush; }
    void setFieldColor( const QColor & col ) { M_field_brush.setColor( col ); }

    const QPen & linePen() const { return M_line_pen; }
    void setLineColor( const QColor & col ) { M_line_pen.setColor( col ); }

    const QPen & measureLinePen() const { return M_measure_line_pen; }
    void setMeasureLineColor( const QColor & col ) { M_measure_line_pen.setColor( col ); }
    const QPen & measureMarkPen() const { return M_measure_mark_pen; }
    void setMeasureMarkColor( const QColor & col ) { M_measure_mark_pen.setColor( col ); }
    const QPen & measureFontPen() const { return M_measure_font_pen; }
    void setMeasureFontColor( const QColor & col ) { M_measure_font_pen.setColor( col ); }
    const QPen & measureFontPen2() const { return M_measure_font_pen2; }
    void setMeasureFontColor2( const QColor & col ) { M_measure_font_pen2.setColor( col ); }

    const QPen & scoreBoardPen() const { return M_score_board_pen; }
    void setScoreBoardPenColor( const QColor & col ) { return M_score_board_pen.setColor( col ); }
    const QBrush & scoreBoardBrush() const { return M_score_board_brush; }
    void setScoreBoardBrushColor( const QColor & col ) { return M_score_board_brush.setColor( col ); }

    const QPen & ballPen() const { return M_ball_pen; }
    const QBrush & ballBrush() const { return M_ball_brush; }
    void setBallColor( const QColor & col )
      {
          M_ball_pen.setColor( col );
          M_ball_brush.setColor( col );
      }
    const QPen & ballVelPen() const { return M_ball_vel_pen; }
    void setBallVelColor( const QColor & col ) { M_ball_vel_pen.setColor( col ); }


    const QPen & playerPen() const { return M_player_pen; }
    const QPen & selectedPlayerPen() const { return M_selected_player_pen; }
    void setPlayerPenColor( const QColor & col )
      {
          M_player_pen.setColor( col );
          M_selected_player_pen.setColor( col );
      }
    const QPen & leftTeamPen() const { return M_left_team_pen; }
    const QBrush & leftTeamBrush() const { return M_left_team_brush; }
    void setLeftTeamColor( const QColor & col )
      {
          M_left_team_pen.setColor( col );
          M_left_team_brush.setColor( col );
      }
    const QPen & leftGoaliePen() const { return M_left_goalie_pen; }
    const QPen & leftGoalieStretchPen() const { return  M_left_goalie_stretch_pen; }
    const QBrush & leftGoalieBrush() const { return M_left_goalie_brush; }
    void setLeftGoalieColor( const QColor & col )
      {
          M_left_goalie_pen.setColor( col );
          M_left_goalie_stretch_pen.setColor( col );
          M_left_goalie_brush.setColor( col );
      }
    const QPen & rightTeamPen() const { return M_right_team_pen; }
    const QBrush & rightTeamBrush() const { return M_right_team_brush; }
    void setRightTeamColor( const QColor & col )
      {
          M_right_team_pen.setColor( col );
          M_right_team_brush.setColor( col );
      }
    const QPen & rightGoaliePen() const { return M_right_goalie_pen; }
    const QPen & rightGoalieStretchPen() const { return M_right_goalie_stretch_pen; }
    const QBrush & rightGoalieBrush() const { return  M_right_goalie_brush; }
    void setRightGoalieColor( const QColor & col )
      {
          M_right_goalie_pen.setColor( col );
          M_right_goalie_stretch_pen.setColor( col );
          M_right_goalie_brush.setColor( col );
      }
    const QPen & playerNumberPen() const { return M_player_number_pen; }
    void setPlayerNumberColor( const QColor & col ) { M_player_number_pen.setColor( col ); }
    const QPen & playerNumberInnerPen() const { return M_player_number_inner_pen; }
    void setPlayerNumberInnerColor( const QColor & col ) { M_player_number_inner_pen.setColor( col ); }
    const QPen & neckPen() const { return M_neck_pen; }
    void setNeckColor( const QColor & col ){ M_neck_pen.setColor( col ); }
    const QPen & viewAreaPen() const { return M_view_area_pen; }
    void setViewAreaColor( const QColor & col ) { M_view_area_pen.setColor( col ); }
    const QPen & largeViewAreaPen() const { return M_large_view_area_pen; }
    void setLargeViewAreaColor( const QColor & col ) { M_large_view_area_pen.setColor( col ); }
    const QPen & focusPointPen() const { return M_focus_point_pen; }
    void setFocusPointColor( const QColor & col ) { M_focus_point_pen.setColor( col ); }
    const QBrush & ballCollideBrush() const { return M_ball_collide_brush; }
    void setBallCollideColor( const QColor & col ) { M_ball_collide_brush.setColor( col ); }
    const QBrush & playerCollideBrush() const { return M_player_collide_brush; }
    void setPlayerCollideColor( const QColor & col ) { M_player_collide_brush.setColor( col ); }
    const QPen & effortDecayedPen() const { return M_effort_decayed_pen; }
    void setEffortDecayedColor( const QColor & col ) { M_effort_decayed_pen.setColor( col ); }
    const QPen & recoveryDecayedPen() const { return M_recovery_decayed_pen; }
    void setRecoveryDecayedColor( const QColor & col ) { M_recovery_decayed_pen.setColor( col ); }
    const QPen & kickPen() const { return M_kick_pen; }
    void setKickColor( const QColor & col ) { M_kick_pen.setColor( col ); }
    const QBrush & kickFaultBrush() const { return M_kick_fault_brush; }
    void setKickFaultColor( const QColor & col ) { M_kick_fault_brush.setColor( col ); }
    const QPen & kickAccelPen() const { return M_kick_accel_pen; }
    void setKickAccelColor( const QColor & col ) { M_kick_accel_pen.setColor( col ); }
    const QBrush & catchBrush() const { return M_catch_brush; }
    void setCatchColor( const QColor & col ) { M_catch_brush.setColor( col ); }
    const QBrush & catchFaultBrush() const { return M_catch_fault_brush; }
    void setCatchFaultColor( const QColor & col ) { M_catch_fault_brush.setColor( col ); }
    const QPen & tacklePen() const { return M_tackle_pen; }
    const QBrush & tackleBrush() const { return M_tackle_brush; }
    void setTackleColor( const QColor & col )
      {
          M_tackle_pen.setColor( col );
          M_tackle_brush.setColor( col );
      }
    const QBrush & tackleFaultBrush() const { return M_tackle_fault_brush; }
    void setTackleFaultColor( const QColor & col ) { M_tackle_fault_brush.setColor( col ); }
    const QBrush & foulChargedBrush() const { return M_foul_charged_brush; }
    void setFoulChargedColor( const QColor & col ) { M_foul_charged_brush.setColor( col ); }
    const QPen & pointtoPen() const { return M_pointto_pen; }
    void setPointtoColor( const QColor & col ) { M_pointto_pen.setColor( col ); }
    const QPen & illegalDefensePen() const { return M_illegal_defense_pen; }
    void setIllegalDefenseColor( const QColor & col ) { M_illegal_defense_pen.setColor( col ); }

    //

    const QFont & scoreBoardFont() const { return M_score_board_font; }
    void setScoreBoardFont( const QFont & font ) { M_score_board_font = font; }

    const QFont & playerFont() const { return M_player_font; }
    void setPlayerFont( const QFont & font ) { M_player_font = font; }

    const QFont & measureFont() const { return M_measure_font; }
    void setMeasureFont( const QFont & font ) { M_measure_font = font; }

};

#endif
