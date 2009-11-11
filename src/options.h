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

#include <QString>
#include <QPoint>
#include <QPointF>
#include <QFont>

#include <rcsslogplayer/types.h>

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

    static const double PITCH_LENGTH;
    static const double PITCH_WIDTH;
    static const double PITCH_HALF_LENGTH;
    static const double PITCH_HALF_WIDTH;
    static const double PITCH_MARGIN;
    static const double CENTER_CIRCLE_R;
    static const double PENALTY_AREA_LENGTH;
    static const double PENALTY_AREA_WIDTH;
    static const double PENALTY_CIRCLE_R;
    static const double PENALTY_SPOT_DIST;
    static const double GOAL_WIDTH;
    static const double GOAL_HALF_WIDTH;
    static const double GOAL_AREA_LENGTH;
    static const double GOAL_AREA_WIDTH;
    static const double GOAL_DEPTH;
    static const double CORNER_ARC_R;
    static const double GOAL_POST_RADIUS;


    //! minimum field scale
    static const double MIN_FIELD_SCALE;
    //! maximum field scale
    static const double MAX_FIELD_SCALE;
    //! zoom rate factor
    static const double ZOOM_RATIO;
    //! default logplayer timer step (ms).
    static const int DEFAULT_TIMER_INTERVAL;

private:

    //
    // monitor client options
    //
    bool M_connect;
    std::string M_server_host;
    int M_server_port;
    int M_client_version;

    //
    // monitor options
    //
    int M_max_disp_buffer;
    //std::string M_game_log_file; //!< game log file path to be opened
    //std::string M_output_file;
    bool M_auto_quit_mode;
    int M_auto_quit_wait;

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

    bool M_hide_menu_bar;
    bool M_hide_tool_bar;
    bool M_hide_status_bar;

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
    bool M_show_catch_area;
    bool M_show_tackle_area;
    bool M_show_kick_accel_area;
    bool M_show_stamina;
    bool M_show_stamina_capacity;
    bool M_show_pointto;
    bool M_show_card;

    bool M_show_offside_line;
    bool M_show_draw_info;

    double M_ball_size; //!< fixed ball radius
    double M_player_size; //!< fixed player radius

    double M_grid_step;
    bool M_show_grid_coord;

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
    void writeSettings();

public:
    ~Options();

    static
    Options & instance();

    /*!
      \biref analyze command line options
    */
    bool parseCmdLine( int argc,
                       char ** argv );


    //
    // monitor options
    //

    bool connect() const
      {
          return M_connect;
      }

    const
    std::string & serverHost() const
      {
          return M_server_host;
      }
    void setServerHost( const std::string & host )
      {
          M_server_host = host;
      }

    int serverPort() const
      {
          return M_server_port;
      }

    int clientVersion() const
      {
          return M_client_version;
      }

    int maxDispBuffer() const
      {
          return M_max_disp_buffer;
      }

    //const std::string & gameLogFile() const { return M_game_log_file; }
    //void setGameLogFile( const std::string & path ) { M_game_log_file = path; }

    //const std::string & outputFile() const { return M_output_file; }

    bool autoQuitMode() const { return M_auto_quit_mode; }
    int autoQuitWait() const { return M_auto_quit_wait; }

    //
    // window option
    //

    int windowX() const
      {
          return M_window_x;
      }

    int windowY() const
      {
          return M_window_y;
      }

    int windowWidth() const
      {
          return M_window_width;
      }

    int windowHeight() const
      {
          return M_window_height;
      }

    bool maximize() const
      {
          return M_maximize;
      }

    bool fullScreen() const
      {
          return M_full_screen;
      }

    int canvasWidth() const
      {
          return M_canvas_width;
      }

    int canvasHeight() const
      {
          return M_canvas_height;
      }

    bool hideMenuBar() const
      {
          return M_hide_menu_bar;
      }

    bool hideToolBar() const
      {
          return M_hide_tool_bar;
      }

    bool hideStatusBar() const
      {
          return M_hide_status_bar;
      }

    //
    // view options
    //

    bool antiAliasing() const
      {
          return M_anti_aliasing;
      }
    void toggleAntiAliasing()
      {
          M_anti_aliasing = ! M_anti_aliasing;
      }

    bool showScoreBoard() const
      {
          return M_show_score_board;
      }
    void toggleShowScoreBoard()
      {
          M_show_score_board = ! M_show_score_board;
      }

    bool showKeepawayArea() const
      {
          return M_show_keepaway_area;
      }
    void toggleShowKeepawayArea()
      {
          M_show_keepaway_area = ! M_show_keepaway_area;
      }

    bool showTeamGraphic() const
      {
          return M_show_team_graphic;
      }
    void toggleShowTeamGraphic()
      {
          M_show_team_graphic = ! M_show_team_graphic;
      }

    bool showFlag() const
      {
          return M_show_flag;
      }
    void toggleShowFlag()
      {
          M_show_flag = ! M_show_flag;
      }

    bool showBall() const
      {
          return M_show_ball;
      }
    void toggleShowBall()
      {
          M_show_ball = ! M_show_ball;
      }

    bool showPlayer() const
      {
          return M_show_player;
      }
    void toggleShowPlayer()
      {
          M_show_player = ! M_show_player;
      }

    bool showPlayerNumber() const
      {
          return M_show_player_number;
      }
    void toggleShowPlayerNumber()
      {
          M_show_player_number = ! M_show_player_number;
      }

    bool showPlayerType() const
      {
          return M_show_player_type;
      }
    void toggleShowPlayerType()
      {
          M_show_player_type = ! M_show_player_type;
      }

    bool showViewArea() const
      {
          return M_show_view_area;
      }
    void toggleShowViewArea()
      {
          M_show_view_area = ! M_show_view_area;
      }

    bool showCatchArea() const
      {
          return M_show_catch_area;
      }
    void toggleShowCatchArea()
      {
          M_show_catch_area = ! M_show_catch_area;
      }

    bool showTackleArea() const
      {
          return M_show_tackle_area;
      }
    void toggleShowTackleArea()
      {
          M_show_tackle_area = ! M_show_tackle_area;
      }

    bool showKickAccelArea() const
      {
          return M_show_kick_accel_area;
      }
    void toggleShowKickAccelArea()
      {
          M_show_kick_accel_area = ! M_show_kick_accel_area;
      }

    bool showStamina() const
      {
          return M_show_stamina;
      }
    void toggleShowStamina()
      {
          M_show_stamina = ! M_show_stamina;
      }

    bool showStaminaCapacity() const
      {
          return M_show_stamina_capacity;
      }
    void toggleShowStaminaCapacity()
      {
          M_show_stamina_capacity = ! M_show_stamina_capacity;
      }

    bool showPointto() const
      {
          return M_show_pointto;
      }
    void toggleShowPointto()
      {
          M_show_pointto = ! M_show_pointto;
      }

    bool showCard() const
      {
          return M_show_card;
      }
    void toggleShowCard()
      {
          M_show_card = ! M_show_card;
      }

    bool showOffsideLine() const
      {
          return M_show_offside_line;
      }
    void toggleShowOffsideLine()
      {
          M_show_offside_line = ! M_show_offside_line;
      }

    bool showDrawInfo() const
      {
          return M_show_draw_info;
      }
    void toggleShowDrawInfo()
      {
          M_show_draw_info = ! M_show_draw_info;
      }

    const
    double & ballSize() const
      {
          return M_ball_size;
      }
    void setBallSize( const double & value )
      {
          if ( value <= 0.001 ) return;
          M_ball_size = value;
      }

    const
    double & playerSize() const
      {
          return M_player_size;
      }
    void setPlayerSize( const double & value )
      {
          if ( value < 0.0 ) return;
          M_player_size = value;
      }

    const
    double & gridStep() const
      {
          return M_grid_step;
      }
    void setGridStep( const double & value )
      {
          M_grid_step = value;
      }

    bool showGridCoord() const
      {
          return M_show_grid_coord;
      }
    void toggleShowGridCoord()
      {
          M_show_grid_coord = ! M_show_grid_coord;
      }

    // field scale

    void updateFieldSize( const int canvas_width,
                          const int canvas_height );

    const
    double & fieldScale() const
      {
          return M_field_scale;
      }
    void setFieldScale( const double & value );

    bool zoomed() const
      {
          return M_zoomed;
      }
    void zoomIn();
    void zoomOut();
    void fitToScreen();

    const
    QPoint & fieldCenter() const
      {
          return M_field_center;
      }

    FocusType focusType() const
      {
          return M_focus_type;
      }
    void setFocusType( const Options::FocusType type )
      {
          M_focus_type = type;
      }

    const
    QPointF & focusPoint() const
      {
          return M_focus_point;
      }
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


    int selectedNumber() const
      {
          return M_selected_number;
      }
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

    PlayerSelectType playerSelectType() const
      {
          return M_player_select_type;
      }
    bool playerAutoSelect() const
      {
          return ( M_player_select_type != SELECT_FIX
                   && M_player_select_type != UNSELECT );
      }
    void unselectPlayer();
    void setPlayerSelectType( const Options::PlayerSelectType type );

    // ball move

    int ballVelCycle() const
      {
          return M_ball_vel_cycle;
      }
    void setBallVelCycle( const int cycle )
      {
          if ( 0 <= cycle && cycle <= 100 ) M_ball_vel_cycle = cycle;
      }


    const QFont & scoreBoardFont() const { return M_score_board_font; }
    void setScoreBoardFont( const QFont & font ) { M_score_board_font = font; }

    const QFont & playerFont() const { return M_player_font; }
    void setPlayerFont( const QFont & font ) { M_player_font = font; }

    const QFont & measureFont() const { return M_measure_font; }
    void setMeasureFont( const QFont & font ) { M_measure_font = font; }

};

#endif
