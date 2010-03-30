// -*-c++-*-

/*!
  \file options.cpp
  \brief application configuration class Source File.
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

#include <QSettings>
#include <QDir>
#include <QFile>
#include <QFontMetrics>

#include "options.h"

#ifdef HAVE_BOOST_PROGRAM_OPTIONS
#include <boost/program_options.hpp>
#endif

#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdio>

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "rcssmonitor"
#endif
#ifndef VERSION
#define VERSION "x.x.x"
#endif

#ifdef Q_WS_WIN
const QString Options::CONF_FILE = QDir::currentPath() + QDir::separator() + QString( "rcssmonitor.ini" );
#else
const QString Options::CONF_FILE = QDir::homePath() + QDir::separator() + QString( ".rcssmonitor.conf" );
#endif

const double Options::PITCH_LENGTH = 105.0;
const double Options::PITCH_WIDTH = 68.0;
const double Options::PITCH_HALF_LENGTH = Options::PITCH_LENGTH * 0.5;
const double Options::PITCH_HALF_WIDTH = Options::PITCH_WIDTH * 0.5;
const double Options::PITCH_MARGIN = 5.0;
const double Options::CENTER_CIRCLE_R = 9.15;
const double Options::PENALTY_AREA_LENGTH = 16.5;
const double Options::PENALTY_AREA_WIDTH = 40.32;
const double Options::PENALTY_CIRCLE_R = 9.15;
const double Options::PENALTY_SPOT_DIST = 11.0;
const double Options::GOAL_WIDTH = 14.02;
const double Options::GOAL_HALF_WIDTH = Options::GOAL_WIDTH * 0.5;
const double Options::GOAL_AREA_LENGTH = 5.5;
const double Options::GOAL_AREA_WIDTH = 18.32;
const double Options::GOAL_DEPTH = 2.44;
const double Options::CORNER_ARC_R = 1.0;
const double Options::GOAL_POST_RADIUS = 0.06;

const double Options::MIN_FIELD_SCALE = 1.0;
const double Options::MAX_FIELD_SCALE = 400.0;
const double Options::ZOOM_RATIO = 1.5;
const int Options::DEFAULT_TIMER_INTERVAL = 100;

//const int Options::WAITING_ANIMATION_SIZE = 64;
const int Options::WAITING_ANIMATION_SIZE = 96;

const QColor Options::FIELD_COLOR( 31, 160, 31 );
const QColor Options::LINE_COLOR( 255, 255, 255 );
const QColor Options::MEASURE_LINE_COLOR( 0, 255, 255 );
const QColor Options::MEASURE_MARK_COLOR( 255, 0, 0 );
const QColor Options::MEASURE_FONT_COLOR( 255, 191, 191 );
const QColor Options::MEASURE_FONT_COLOR2( 224, 224, 192 );
const QColor Options::SCORE_BOARD_PEN_COLOR( 255, 255, 255 );
const QColor Options::SCORE_BOARD_BRUSH_COLOR( 0, 0, 0 );
const QColor Options::BALL_COLOR( 255, 255, 255 );
const QColor Options::BALL_VEL_COLOR( 255, 0, 0 );
const QColor Options::PLAYER_PEN_COLOR( 0, 0, 0 );
const QColor Options::LEFT_TEAM_COLOR( 255, 215, 0 );
const QColor Options::LEFT_GOALIE_COLOR( 39, 231, 31 );
const QColor Options::RIGHT_TEAM_COLOR( 0, 191, 255 );
const QColor Options::RIGHT_GOALIE_COLOR( 255, 153, 255 );
const QColor Options::PLAYER_NUMBER_COLOR( 255, 255, 255 );
const QColor Options::PLAYER_NUMBER_INNER_COLOR( 0, 0, 0 );
const QColor Options::NECK_COLOR( 255, 0, 0 );
const QColor Options::VIEW_AREA_COLOR( 191, 239, 191 );
const QColor Options::LARGE_VIEW_AREA_COLOR( 255, 255, 255 );
const QColor Options::BALL_COLLIDE_COLOR( 255, 0, 0 );
const QColor Options::PLAYER_COLLIDE_COLOR( 105, 155, 235 );
const QColor Options::EFFORT_DECAYED_COLOR( 255, 0, 0 );
const QColor Options::RECOVERY_DECAYED_COLOR( 255, 231, 31 );
const QColor Options::KICK_COLOR( 255, 255, 255 );
const QColor Options::KICK_FAULT_COLOR( 255, 0, 0 );
const QColor Options::KICK_ACCEL_COLOR( 0, 255, 0 );
const QColor Options::CATCH_COLOR( 10, 80, 10 );
const QColor Options::CATCH_FAULT_COLOR( 10, 80, 150 );
const QColor Options::TACKLE_COLOR( 255, 136, 127 );
const QColor Options::TACKLE_FAULT_COLOR( 79, 159, 159 );
const QColor Options::FOUL_CHARGED_COLOR( 0, 127, 0 );
const QColor Options::POINTTO_COLOR( 255, 0, 191 );


namespace {

inline
std::string
to_onoff( const bool val )
{
    return val ? std::string( "on" ) : std::string( "off" );
}

inline
std::string
to_string( const double & val,
           const double prec = 0.001 )
{
    std::ostringstream os;
    os << rint( val / prec ) * prec;
    return os.str();
    //return boost::lexical_cast< std::string >( rint( val / prec ) * prec );
}

}

/*-------------------------------------------------------------------*/
/*!
  singleton interface
*/
Options &
Options::instance()
{
    static Options s_instance;
    return s_instance;
}

/*-------------------------------------------------------------------*/
/*!

 */
Options::Options()
    :
    // monitor client options
    M_connect( true ),
    M_server_host( "127.0.0.1" ),
    M_server_port( 6000 ),
    M_client_version( 4 ),
    M_buffering_mode( false ),
    M_buffer_size( 10 ),
    M_max_disp_buffer( 65535 ),
    M_auto_quit_mode( false ),
    M_auto_quit_wait( 5 ),
    M_auto_reconnect_mode( false ),
    M_auto_reconnect_wait( 5 ),
    M_timer_interval( DEFAULT_TIMER_INTERVAL ),
    // window options
    M_window_x( -1 ),
    M_window_y( -1 ),
    M_window_width( -1 ),
    M_window_height( -1 ),
    M_maximize( false ),
    M_full_screen( false ),
    M_canvas_width( -1 ),
    M_canvas_height( -1 ),
    M_show_menu_bar( true ),
    // M_show_tool_bar( false )
    M_show_status_bar( false ),
    // view options
    M_anti_aliasing( true ),
    M_show_score_board( true ),
    M_show_keepaway_area( false ),
    M_show_team_graphic( true ),
    M_show_flag( false ),
    M_show_ball( true ),
    M_show_player( true ),
    M_show_player_number( true ),
    M_show_player_type( false ),
    M_show_view_area( false ),
    M_show_tackle_area( false ),
    M_show_stamina( false ),
    M_show_pointto( false ),
    M_show_card( true ),
    M_show_offside_line( false ),
    M_show_draw_info( true ),
    M_ball_size( 0.35 ),
    M_player_size( 0.0 ),
    M_grid_step( 0.0 ),
    M_show_grid_coord( false ),
    M_field_scale( 1.0 ),
    M_zoomed( false ),
    M_field_center( 0, 0 ),
    M_focus_type( FOCUS_POINT ),
    M_focus_point( 0.0, 0.0 ),
    M_selected_number( 0 ),
    M_player_select_type( UNSELECT ),
    M_ball_vel_cycle( 0 ),
      //
    M_buffer_recover_mode( true ),
      //
    M_field_brush( FIELD_COLOR, Qt::SolidPattern ),
    M_line_pen( LINE_COLOR, 0, Qt::SolidLine ),
    M_measure_line_pen( MEASURE_LINE_COLOR, 0, Qt::SolidLine ),
    M_measure_mark_pen( MEASURE_MARK_COLOR, 0, Qt::SolidLine ),
    M_measure_font_pen( MEASURE_FONT_COLOR, 0, Qt::SolidLine ),
    M_measure_font_pen2( MEASURE_FONT_COLOR2, 0, Qt::SolidLine ),
      //
    M_score_board_pen( SCORE_BOARD_PEN_COLOR, 0, Qt::SolidLine ),
    M_score_board_brush( SCORE_BOARD_BRUSH_COLOR, Qt::SolidPattern ),
      //
    M_ball_pen( BALL_COLOR, 0, Qt::SolidLine ),
    M_ball_brush( BALL_COLOR, Qt::SolidPattern ),
    M_ball_vel_pen( BALL_VEL_COLOR, 0, Qt::SolidLine ),
      //
    M_player_pen( PLAYER_PEN_COLOR, 0, Qt::SolidLine ),
    M_selected_player_pen( PLAYER_PEN_COLOR, 2, Qt::SolidLine ),
    M_left_team_pen( LEFT_TEAM_COLOR, 0, Qt::SolidLine ),
    M_left_team_brush( LEFT_TEAM_COLOR, Qt::SolidPattern ),
    M_left_goalie_pen( LEFT_GOALIE_COLOR, 0, Qt::SolidLine ),
    M_left_goalie_stretch_pen( LEFT_GOALIE_COLOR.darker( 200 ), 0, Qt::DotLine ),
    M_left_goalie_brush( LEFT_GOALIE_COLOR, Qt::SolidPattern ),
    M_right_team_pen( RIGHT_TEAM_COLOR, 0, Qt::SolidLine ),
    M_right_team_brush( RIGHT_TEAM_COLOR, Qt::SolidPattern ),
    M_right_goalie_pen( RIGHT_GOALIE_COLOR, 0, Qt::SolidLine ),
    M_right_goalie_stretch_pen( RIGHT_GOALIE_COLOR.darker( 200 ), 0, Qt::DotLine ),
    M_right_goalie_brush( RIGHT_GOALIE_COLOR, Qt::SolidPattern ),
    M_player_number_pen( PLAYER_NUMBER_COLOR, 0, Qt::SolidLine ),
    M_player_number_inner_pen( PLAYER_NUMBER_INNER_COLOR, 0, Qt::SolidLine ),
    M_neck_pen( NECK_COLOR, 0, Qt::SolidLine ),
    M_view_area_pen( VIEW_AREA_COLOR, 0, Qt::SolidLine ),
    M_large_view_area_pen( LARGE_VIEW_AREA_COLOR, 0, Qt::SolidLine ),
    M_ball_collide_brush( BALL_COLLIDE_COLOR, Qt::SolidPattern ),
    M_player_collide_brush( PLAYER_COLLIDE_COLOR, Qt::SolidPattern ),
    M_effort_decayed_pen( EFFORT_DECAYED_COLOR, 0, Qt::SolidLine ),
    M_recovery_decayed_pen( RECOVERY_DECAYED_COLOR, 0, Qt::SolidLine ),
    M_kick_pen( KICK_COLOR, 2, Qt::SolidLine ),
    M_kick_fault_brush( KICK_FAULT_COLOR, Qt::SolidPattern ),
    M_kick_accel_pen( KICK_ACCEL_COLOR, 0, Qt::SolidLine ),
    M_catch_brush( CATCH_COLOR, Qt::SolidPattern ),
    M_catch_fault_brush( CATCH_FAULT_COLOR, Qt::SolidPattern ),
    M_tackle_pen( TACKLE_COLOR, 2, Qt::SolidLine ),
    M_tackle_brush( TACKLE_COLOR, Qt::SolidPattern ),
    M_tackle_fault_brush( TACKLE_FAULT_COLOR, Qt::SolidPattern ),
    M_foul_charged_brush( FOUL_CHARGED_COLOR, Qt::SolidPattern ),
    M_pointto_pen( POINTTO_COLOR, 0, Qt::SolidLine ),
      //
    M_score_board_font( "Sans Serif", 16, QFont::Bold ),
    M_player_font( "Sans Serif", 9, QFont::Bold ),
    M_measure_font( "Sans Serif", 9 )
{
    //M_player_font.setStyleHint( QFont::System, QFont::PreferBitmap );
    //M_player_font.setFixedPitch( true );

    if ( ! QFile::exists( Options::CONF_FILE ) )
    {
        writeSettings( true );
    }
    else
    {
        readSettings();
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
Options::~Options()
{
    writeSettings( false );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Options::readSettings()
{
    QSettings settings( Options::CONF_FILE,
                        QSettings::IniFormat );

    QVariant val;

    //
    // main window
    //

    settings.beginGroup( "MainWindow" );

    val = settings.value( "window_width" );
    if ( val.isValid() ) M_window_width = val.toInt();

    val = settings.value( "window_height" );
    if ( val.isValid() ) M_window_height = val.toInt();

    val = settings.value( "window_x" );
    if ( val.isValid() ) M_window_x = val.toInt();

    val = settings.value( "window_y" );
    if ( val.isValid() ) M_window_y = val.toInt();

    val = settings.value( "maximize" );
    if ( val.isValid() ) M_maximize = val.toBool();

    val = settings.value( "full_screen" );
    if ( val.isValid() ) M_full_screen = val.toBool();

    val = settings.value( "show_menu_bar" );
    if ( val.isValid() ) M_show_menu_bar = val.toBool();

//     val = settings.value( "show_tool_bar" );
//     if ( val.isValid() ) M_show_tool_bar = val.toBool();

    val = settings.value( "show_status_bar" );
    if ( val.isValid() ) M_show_status_bar = val.toBool();

    settings.endGroup();

    //
    // monitor
    //

    settings.beginGroup( "Monitor" );

    val = settings.value( "connect" );
    if ( val.isValid() ) M_connect  = val.toBool();

    val = settings.value( "server_host" );
    if ( val.isValid() ) M_server_host = val.toString().toStdString();

    val = settings.value( "server_port" );
    if ( val.isValid() ) M_server_port = val.toInt();

    val = settings.value( "client_version" );
    if ( val.isValid() ) M_client_version = val.toInt();

    val = settings.value( "buffering_mode" );
    if ( val.isValid() ) M_buffering_mode = val.toBool();

    val = settings.value( "buffer_size" );
    if ( val.isValid() ) M_buffer_size = val.toInt();

    val = settings.value( "max_disp_buffer" );
    if ( val.isValid() ) M_max_disp_buffer = val.toInt();

    val = settings.value( "auto_quit_mode" );
    if ( val.isValid() ) M_auto_quit_mode = val.toBool();

    val = settings.value( "auto_quit_wait" );
    if ( val.isValid() ) M_auto_quit_wait = val.toInt();

    val = settings.value( "auto_reconnect_mode" );
    if ( val.isValid() ) M_auto_reconnect_mode = val.toBool();

    val = settings.value( "auto_reconnect_wait" );
    if ( val.isValid() ) M_auto_reconnect_wait = val.toInt();

    val = settings.value( "timer_interval" );
    if ( val.isValid() ) M_timer_interval = val.toInt();

    settings.endGroup();

    //
    // show
    //

    settings.beginGroup( "View" );

    val = settings.value( "anti_aliasing" );
    if ( val.isValid() ) M_anti_aliasing = val.toBool();

    val = settings.value( "show_score_board" );
    if ( val.isValid() ) M_show_score_board = val.toBool();

    val = settings.value( "show_keepaway_area" );
    if ( val.isValid() ) M_show_keepaway_area = val.toBool();

    val = settings.value( "show_team_graphic" );
    if ( val.isValid() ) M_show_team_graphic = val.toBool();

    val = settings.value( "show_draw_info" );
    if ( val.isValid() ) M_show_draw_info = val.toBool();

    val = settings.value( "show_ball" );
    if ( val.isValid() ) M_show_ball = val.toBool();

    val = settings.value( "show_player" );
    if ( val.isValid() ) M_show_player = val.toBool();

    val = settings.value( "show_player_number" );
    if ( val.isValid() ) M_show_player_number = val.toBool();

    val = settings.value( "show_player_type" );
    if ( val.isValid() ) M_show_player_type = val.toBool();

    val = settings.value( "show_view_area" );
    if ( val.isValid() ) M_show_view_area = val.toBool();

    val = settings.value( "show_catch_area" );
    if ( val.isValid() ) M_show_catch_area = val.toBool();

    val = settings.value( "show_tackle_area" );
    if ( val.isValid() ) M_show_tackle_area = val.toBool();

    val = settings.value( "show_kick_accel_area" );
    if ( val.isValid() ) M_show_kick_accel_area = val.toBool();

    val = settings.value( "show_stamina" );
    if ( val.isValid() ) M_show_stamina = val.toBool();

    val = settings.value( "show_pointto" );
    if ( val.isValid() ) M_show_pointto = val.toBool();

    val = settings.value( "show_card" );
    if ( val.isValid() ) M_show_card = val.toBool();

    val = settings.value( "grid_step", M_grid_step );
    if ( val.isValid() ) M_grid_step = val.toDouble();

    val = settings.value( "show_grid_coord", M_show_grid_coord );
    if ( val.isValid() ) M_show_grid_coord = val.toBool();

    val = settings.value( "show_flag", M_show_flag );
    if ( val.isValid() ) M_show_flag = val.toBool();

    val = settings.value( "show_offside_line", M_show_offside_line );
    if ( val.isValid() ) M_show_offside_line = val.toBool();

    settings.endGroup();

    //
    // size
    //

    settings.beginGroup( "Size" );

    val = settings.value( "ball_size", M_ball_size );
    if ( val.isValid() ) M_ball_size = val.toDouble();

    val = settings.value( "player_size", M_player_size );
    if ( val.isValid() ) M_player_size = val.toDouble();

    settings.endGroup();

    //
    // color
    //

    settings.beginGroup( "Color" );

    // field

    val = settings.value( "field_brush" );
    if ( val.isValid() ) M_field_brush.setColor( val.toString() );

    val = settings.value( "line_pen" );
    if ( val.isValid() ) M_line_pen.setColor( val.toString() );


    val = settings.value( "measure_line_pen" );
    if ( val.isValid() ) M_measure_line_pen.setColor( val.toString() );

    val = settings.value( "measure_mark_pen" );
    if ( val.isValid() ) M_measure_mark_pen.setColor( val.toString() );

    val = settings.value( "measure_font_pen" );
    if ( val.isValid() ) M_measure_font_pen.setColor( val.toString() );

    val = settings.value( "measure_font_pen2" );
    if ( val.isValid() ) M_measure_font_pen2.setColor( val.toString() );

    // score board

    val = settings.value( "score_board_pen" );
    if ( val.isValid() ) M_score_board_pen.setColor( val.toString() );

    val = settings.value( "score_board_brush" );
    if ( val.isValid() ) M_score_board_brush.setColor( val.toString() );

    // ball

    val = settings.value( "ball_pen" );
    if ( val.isValid() ) M_ball_pen.setColor( val.toString() );

    val = settings.value( "ball_vel_pen" );
    if ( val.isValid() ) M_ball_vel_pen.setColor( val.toString() );

    val = settings.value( "ball_brush" );
    if ( val.isValid() ) M_ball_brush.setColor( val.toString() );

    // player

    val = settings.value( "player_pen" );
    if ( val.isValid() ) M_player_pen.setColor( val.toString() );

    val = settings.value( "left_team" );
    if ( val.isValid() )
    {
        M_left_team_pen.setColor( val.toString() );
        M_left_team_brush.setColor( val.toString() );
    }

    val = settings.value( "left_goalie" );
    if ( val.isValid() )
    {
        M_left_goalie_pen.setColor( val.toString() );
        M_left_goalie_brush.setColor( val.toString() );
    }

    val = settings.value( "left_goalie_unreliable_catch" );
    if ( val.isValid() )
    {
        M_left_goalie_stretch_pen.setColor( val.toString() );
    }

    val = settings.value( "right_team" );
    if ( val.isValid() )
    {
        M_right_team_pen.setColor( val.toString() );
        M_right_team_brush.setColor( val.toString() );
    }

    val = settings.value( "right_goalie" );
    if ( val.isValid() )
    {
        M_right_goalie_pen.setColor( val.toString() );
        M_right_goalie_brush.setColor( val.toString() );
    }

    val = settings.value( "right_goalie_unreliable_catch" );
    if ( val.isValid() )
    {
        M_right_goalie_stretch_pen.setColor( val.toString() );
    }

    val = settings.value( "player_number_pen" );
    if ( val.isValid() ) M_player_number_pen.setColor( val.toString() );

    val = settings.value( "player_number_inner_pen" );
    if ( val.isValid() ) M_player_number_inner_pen.setColor( val.toString() );

    val = settings.value( "neck_pen" );
    if ( val.isValid() ) M_neck_pen.setColor( val.toString() );

    val = settings.value( "view_area_pen" );
    if ( val.isValid() ) M_view_area_pen.setColor( val.toString() );

    val = settings.value( "large_view_area_pen" );
    if ( val.isValid() ) M_large_view_area_pen.setColor( val.toString() );

    val = settings.value( "ball_collide_brush" );
    if ( val.isValid() ) M_ball_collide_brush.setColor( val.toString() );

    val = settings.value( "effort_decayed_pen" );
    if ( val.isValid() ) M_effort_decayed_pen.setColor( val.toString() );

    val = settings.value( "recovery_decayed_pen" );
    if ( val.isValid() ) M_recovery_decayed_pen.setColor( val.toString() );

    val = settings.value( "kick_pen" );
    if ( val.isValid() ) M_kick_pen.setColor( val.toString() );

    val = settings.value( "kick_fault_brush" );
    if ( val.isValid() ) M_kick_fault_brush.setColor( val.toString() );

    val = settings.value( "kick_accel_pen" );
    if ( val.isValid() ) M_kick_accel_pen.setColor( val.toString() );

    val = settings.value( "catch_brush" );
    if ( val.isValid() ) M_catch_brush.setColor( val.toString() );

    val = settings.value( "catch_fault_brush" );
    if ( val.isValid() ) M_catch_fault_brush.setColor( val.toString() );

    val = settings.value( "tackle_pen" );
    if ( val.isValid() ) M_tackle_pen.setColor( val.toString() );

    val = settings.value( "tackle_brush" );
    if ( val.isValid() ) M_tackle_brush.setColor( val.toString() );

    val = settings.value( "tackle_fault_brush" );
    if ( val.isValid() ) M_tackle_fault_brush.setColor( val.toString() );

    val = settings.value( "foul_charged_brush" );
    if ( val.isValid() ) M_foul_charged_brush.setColor( val.toString() );

    settings.endGroup();

    //
    // font
    //

    settings.beginGroup( "Font" );

    val = settings.value( "score_board_font" );
    if ( val.isValid() ) M_score_board_font.fromString( val.toString() );

    val = settings.value( "player_font" );
    if ( val.isValid() ) M_player_font.fromString( val.toString() );

    val = settings.value( "measure_font" );
    if ( val.isValid() ) M_measure_font.fromString( val.toString() );

    settings.endGroup();
}

/*-----------------------------------------------------------------*/
/*!

 */
void
Options::writeSettings( bool all )
{
    QSettings settings( Options::CONF_FILE,
                        QSettings::IniFormat );

    if ( all )
    {
        //
        // main window
        //
        settings.beginGroup( "MainWindow" );
        settings.setValue( "window_width", M_window_width );
        settings.setValue( "window_height", M_window_height );
        settings.setValue( "window_x", M_window_x );
        settings.setValue( "window_y", M_window_y );
        //     settings.setValue( "canvas_width", M_canvas_width );
        //     settings.setValue( "canvas_height", M_canvas_height );
        settings.setValue( "maximize", M_maximize );
        settings.setValue( "full_screen", M_full_screen );
        settings.setValue( "show_menu_bar", M_show_menu_bar );
        //     settings.setValue( "show_tool_bar", M_show_tool_bar );
        settings.setValue( "show_status_bar", M_show_status_bar );
        settings.endGroup();

        //
        // monitor
        //
        settings.beginGroup( "Monitor" );
        settings.setValue( "connect", M_connect );
        settings.setValue( "server_host", QString::fromStdString( M_server_host ) );
        settings.setValue( "server_port", M_server_port );
        settings.setValue( "client_version", M_client_version );
        settings.setValue( "buffering_mode", M_buffering_mode );
        settings.setValue( "buffer_size", M_buffer_size );
        settings.setValue( "max_disp_buffer", M_max_disp_buffer );
        settings.setValue( "auto_quit_mode", M_auto_quit_mode );
        settings.setValue( "auto_quit_wait", M_auto_quit_wait );
        settings.setValue( "auto_reconnect_wait", M_auto_reconnect_wait );
        settings.setValue( "timer_interval", M_timer_interval );
        settings.endGroup();
    }

    //
    // field
    //
    settings.beginGroup( "View" );
    settings.setValue( "anti_aliasing", M_anti_aliasing );
    settings.setValue( "show_score_board", M_show_score_board );
    settings.setValue( "show_keepaway_area", M_show_keepaway_area );
    settings.setValue( "show_team_graphic", M_show_team_graphic );
    settings.setValue( "show_draw_info", M_show_draw_info );
    settings.setValue( "show_ball", M_show_ball );
    settings.setValue( "show_player", M_show_player );
    settings.setValue( "show_player_number", M_show_player_number );
    settings.setValue( "show_player_type", M_show_player_type );
    settings.setValue( "show_view_area", M_show_view_area );
    settings.setValue( "show_catch_area", M_show_catch_area );
    settings.setValue( "show_tackle_area", M_show_tackle_area );
    settings.setValue( "show_kick_accel_area", M_show_kick_accel_area );
    settings.setValue( "show_stamina", M_show_stamina );
    settings.setValue( "show_pointto", M_show_pointto );
    settings.setValue( "show_card", M_show_card );
    settings.setValue( "show_grid_coord", M_show_grid_coord );
    settings.setValue( "grid_step", M_grid_step );
    settings.setValue( "show_flag", M_show_flag );
    settings.setValue( "show_offside_line", M_show_offside_line );
    settings.endGroup();

    //
    // size
    //
    settings.beginGroup( "Size" );
    settings.setValue( "ball_size", M_ball_size );
    settings.setValue( "player_size", M_player_size );
    settings.endGroup();

    //
    // color
    //
    settings.beginGroup( "Color" );
    settings.setValue( "field_brush", M_field_brush.color().name() );
    settings.setValue( "line_pen", M_line_pen.color().name() );
    settings.setValue( "measure_line_pen", M_measure_line_pen.color().name() );
    settings.setValue( "measure_mark_pen", M_measure_mark_pen.color().name() );
    settings.setValue( "measure_font_pen", M_measure_font_pen.color().name() );
    settings.setValue( "measure_font_pen2", M_measure_font_pen2.color().name() );
    //
    settings.setValue( "score_board_pen", M_score_board_pen.color().name() );
    settings.setValue( "score_board_brush", M_score_board_brush.color().name() );
    //
    settings.setValue( "ball_pen", M_ball_pen.color().name() );
    settings.setValue( "ball_vel_pen", M_ball_vel_pen.color().name() );
    settings.setValue( "ball_brush", M_ball_brush.color().name() );
    //
    settings.setValue( "player_pen", M_player_pen.color().name() );
    settings.setValue( "left_team", M_left_team_pen.color().name() );
    settings.setValue( "left_goalie", M_left_goalie_brush.color().name() );
    settings.setValue( "left_goalie_unreliable_catch", M_left_goalie_stretch_pen.color().name() );
    settings.setValue( "right_team", M_right_team_pen.color().name() );
    settings.setValue( "right_goalie", M_right_goalie_brush.color().name() );
    settings.setValue( "right_goalie_unreliable_catch", M_right_goalie_stretch_pen.color().name() );
    settings.setValue( "player_number_pen", M_player_number_pen.color().name() );
    settings.setValue( "player_number_inner_pen", M_player_number_inner_pen.color().name() );
    settings.setValue( "neck_pen", M_neck_pen.color().name() );
    settings.setValue( "view_area_pen", M_view_area_pen.color().name() );
    settings.setValue( "large_view_area_pen", M_large_view_area_pen.color().name() );
    settings.setValue( "ball_collide_brush", M_ball_collide_brush.color().name() );
    settings.setValue( "effort_decayed_pen", M_effort_decayed_pen.color().name() );
    settings.setValue( "recovery_decayed_pen", M_recovery_decayed_pen.color().name() );
    settings.setValue( "kick_pen", M_kick_pen.color().name() );
    settings.setValue( "kick_fault_brush", M_kick_fault_brush.color().name() );
    settings.setValue( "catch_brush", M_catch_brush.color().name() );
    settings.setValue( "catch_fault_brush", M_catch_fault_brush.color().name() );
    settings.setValue( "tackle_pen", M_tackle_pen.color().name() );
    settings.setValue( "tackle_brush", M_tackle_brush.color().name() );
    settings.setValue( "tackle_fault_brush", M_tackle_fault_brush.color().name() );
    settings.setValue( "foul_charged_brush", M_foul_charged_brush.color().name() );
    settings.endGroup();

    //
    // font
    //
    settings.beginGroup( "Font" );
    settings.setValue( "score_board_font", M_score_board_font.toString() );
    settings.setValue( "player_font", M_player_font.toString() );
    settings.setValue( "measure_font", M_measure_font.toString() );
    settings.endGroup();
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Options::parseCmdLine( int argc,
                       char ** argv )
{
#ifdef HAVE_BOOST_PROGRAM_OPTIONS
    namespace po = boost::program_options;

    std::string geometry;
//     std::string canvas_size;

    po::options_description visibles( "Allowed options:" );

    visibles.add_options()
        ( "help,h",
          "print this message." )
        ( "version,v",
          "print version information." )
        // monitor options
        ( "connect,c",
          po::bool_switch( &M_connect )->default_value( M_connect ),
          "connect to the soccer server." )
        ( "server-host",
          po::value< std::string >( &M_server_host )->default_value( M_server_host ),
          "set host name or ip address where the soccer server is running." )
        ( "server-port",
          po::value< int >( &M_server_port )->default_value( M_server_port ),
          "set port number to connect as the monitor client." )
        ( "client-version",
          po::value< int >( &M_client_version )->default_value( M_client_version ),
          "set a monitor client protocol version." )
        ( "buffering-mode",
          po::value< bool >( &M_buffering_mode )->default_value( M_buffering_mode, to_onoff( M_buffering_mode ) ),
          "enable buffering mode." )
        ( "buffer-size",
          po::value< int >( &M_buffer_size )->default_value( M_buffer_size ),
          "set cache size for buffering mode." )
//         ( "max-disp-buffer",
//           po::value< int >( &M_max_disp_buffer )->default_value( 65535, "65535" ),
//           "set max size of display data buffer." )
        ( "timer-interval",
          po::value< int >( &M_timer_interval )->default_value( M_timer_interval ),
          "set the desired timer interval [ms] for buffering mode." )
        ( "auto-quit-mode",
          po::value< bool >( &M_auto_quit_mode )->default_value( M_auto_quit_mode, to_onoff( M_auto_quit_mode ) ),
          "enable automatic quit mode." )
        ( "auto-quit-wait",
          po::value< int >( &M_auto_quit_wait )->default_value( M_auto_quit_wait ),
          "set an wait period for the automatic quit mode." )
        ( "auto-reconnect-mode",
          po::value< bool >( &M_auto_reconnect_mode )->default_value( M_auto_reconnect_mode, to_onoff( M_auto_reconnect_mode ) ),
          "enable automatic reconnect mode." )
        ( "auto-reconnect-wait",
          po::value< int >( &M_auto_reconnect_wait )->default_value( M_auto_reconnect_wait ),
          "set an wait period for the automatic reconnect mode." )
        // window options
        ( "geometry",
          po::value< std::string >( &geometry )->default_value( "" ),
          "specifies the window geometry ([WxH][+X+Y]). e.g. --geometry=1024x768+1+1" )
        ( "maximize",
          po::bool_switch( &M_maximize )->default_value( M_maximize ),
          "start with a maximized window." )
        ( "full-screen",
          po::bool_switch( &M_full_screen )->default_value( M_full_screen ),
          "start with a full screen window." )
        ( "show-menu-bar",
          po::value< bool >( &M_show_menu_bar )->default_value( M_show_menu_bar, to_onoff( M_show_menu_bar ) ),
          "show menu bar." )
//         ( "show-tool-bar",
//           po::value< bool >( &M_show_tool_bar )->default_value( M_show_tool_bar ),
//           "start without a tool bar." )
        ( "show-status-bar",
          po::value< bool >( &M_show_status_bar )->default_value( M_show_status_bar, to_onoff( M_show_status_bar ) ),
          "show status bar." )
        // view options
        ( "anti-aliasing",
          po::value< bool >( &M_anti_aliasing )->default_value( M_anti_aliasing, to_onoff( M_anti_aliasing ) ),
          "show anti-aliased objects." )
        ( "show-score-board",
          po::value< bool >( &M_show_score_board )->default_value( M_show_score_board, to_onoff( M_show_score_board ) ),
          "show score board." )
        ( "show-keepaway-area",
          po::value< bool >( &M_show_keepaway_area )->default_value( M_show_keepaway_area, to_onoff( M_show_keepaway_area ) ),
          "show keepaway area." )
        ( "show-team-graphic",
          po::value< bool >( &M_show_team_graphic )->default_value( M_show_team_graphic, to_onoff( M_show_team_graphic ) ),
          "show team graphic." )
        ( "show-draw-info",
          po::value< bool >( &M_show_draw_info )->default_value( M_show_draw_info, to_onoff( M_show_draw_info ) ),
          "show draw information." )
        ( "show-ball",
          po::value< bool >( &M_show_ball )->default_value( M_show_ball, to_onoff( M_show_ball ) ),
          "show ball." )
        ( "show-player",
          po::value< bool >( &M_show_player )->default_value( M_show_player, to_onoff( M_show_player ) ),
          "show players." )
        ( "show-player-number",
          po::value< bool >( &M_show_player_number )->default_value( M_show_player_number, to_onoff( M_show_player_number ) ),
          "show player\'s uniform number." )
        ( "show-player-type",
          po::value< bool >( &M_show_player_type )->default_value( M_show_player_type, to_onoff( M_show_player_type ) ),
          "show player\'s heterogeneous type ID." )
        ( "show-view-area",
          po::value< bool >( &M_show_view_area )->default_value( M_show_view_area, to_onoff( M_show_view_area ) ),
          "show player\'s view area." )
        ( "show-catch-area",
          po::value< bool >( &M_show_catch_area )->default_value( M_show_catch_area, to_onoff( M_show_catch_area ) ),
          "show player\'s catch area." )
        ( "show-tackle-area",
          po::value< bool >( &M_show_tackle_area )->default_value( M_show_tackle_area, to_onoff( M_show_tackle_area ) ),
          "show player\'s tackle area." )
        ( "show-kick-accel-area",
          po::value< bool >( &M_show_kick_accel_area )->default_value( M_show_kick_accel_area, to_onoff( M_show_kick_accel_area ) ),
          "show player\'s kick acceleration area." )
        ( "show-stamina",
          po::value< bool >( &M_show_stamina )->default_value( M_show_stamina, to_onoff( M_show_stamina ) ),
          "show player\'s stamina." )
        ( "show-stamina-capacity",
          po::value< bool >( &M_show_stamina_capacity )->default_value( M_show_stamina_capacity, to_onoff( M_show_stamina_capacity ) ),
          "show player\'s stamina capacity." )
        ( "show-pointto",
          po::value< bool >( &M_show_pointto )->default_value( M_show_pointto, to_onoff( M_show_pointto ) ),
          "show player\'s pointto status." )
        ( "show-card",
          po::value< bool >( &M_show_card )->default_value( M_show_card, to_onoff( M_show_card ) ),
          "show player\'s yellow/red cards." )
        ( "ball-size",
          po::value< double >( &M_ball_size )->default_value( M_ball_size, to_string( M_ball_size ) ),
          "set a ball radius." )
        ( "player-size",
          po::value< double >( &M_player_size )->default_value( M_player_size, to_string( M_player_size ) ),
          "set a fixed player radius." )
        ( "show-grid-coord",
          po::value< bool >( &M_show_grid_coord )->default_value( M_show_grid_coord, to_onoff( M_show_grid_coord ) ),
          "show coordinates value of grid lines." )
        ( "grid-step",
          po::value< double >( &M_grid_step )->default_value( M_grid_step, to_string( M_grid_step ) ),
          "set a grid step size." )
        ( "show-flag",
          po::value< bool >( &M_show_flag )->default_value( M_show_flag, to_onoff( M_show_flag) ),
          "show landmark flags." )
        ( "show-offside-line",
          po::value< bool >( &M_show_offside_line )->default_value( M_show_offside_line, to_onoff( M_show_offside_line ) ),
          "show offside lines." )
        ;

    po::options_description invisibles( "Invisibles" );
//     invisibles.add_options()
//         ( "game-log-file",
//           po::value< std::string >( &M_game_log_file )->default_value( "" ),
//           "set the path to Game Log file(.rcg) to be opened.")
//         ;

    po::positional_options_description pdesc;
    pdesc.add( "game-log-file", 1 );

    po::options_description all_desc( "All options:" );
    all_desc.add( visibles ).add( invisibles );


    bool help = false;
    bool version = false;
    try
    {
        po::variables_map vm;
        po::command_line_parser parser( argc, argv );
        parser.options( all_desc ).positional( pdesc );
        po::store( parser.run(), vm );
        po::notify( vm );

        help = vm.count( "help" );
        version = vm.count( "version" );
    }
    catch ( std::exception & e )
    {
        std::cerr << e.what() << std::endl;
        help = true;
    }

    if ( version )
    {
        std::cout << PACKAGE_NAME << " Version " << VERSION
                  << std::endl;
        if ( ! help )
        {
            return false;
        }
    }

    if ( help )
    {
        std::cout << "Usage: " << PACKAGE_NAME
                  << " [options ... ]\n";
        std::cout << visibles << std::endl;
        return false;
    }

    if ( M_buffer_size <= 1 )
    {
        std::cerr << "Illegal buffering size " << M_buffer_size
                  << ". The value have to be more than 1."
                  << std::endl;
        M_buffer_size = 1;
    }

    if ( M_timer_interval < 0 )
    {
        std::cerr << "Illegal timer interval " << M_timer_interval
                  << ". use default value("
                  << DEFAULT_TIMER_INTERVAL << ')'
                  << std::endl;
        M_timer_interval = DEFAULT_TIMER_INTERVAL;
    }

    if ( M_timer_interval < 5 )
    {
        std::cerr << "Too small timer interval " << M_timer_interval
                  << ".  replaced by 5." << std::endl;
        M_timer_interval = 5;
    }

    if ( M_timer_interval > 5000 )
    {
        std::cerr << "Too huge timer interval " << M_timer_interval
                  << ".  replaced by 5000." << std::endl;
        M_timer_interval = 5000;
    }

    if ( ! geometry.empty() )
    {
        int w = -1, h = -1;
        int x = -1, y = -1;

        int num = std::sscanf( geometry.c_str(),
                               " %d x %d %d %d " ,
                               &w, &h, &x, &y );
        //std::cerr << "geometry = " << geometry
        //          << "  param_num=" << num
        //          << " width=" << w << " height=" << h
        //          << " x=" << x << " y=" << y
        //          << std::endl;
        if ( num == 4 || num == 2 )
        {
            if ( w <= 0 || h <= 0 )
            {
                std::cerr << "Illegal window size [" << geometry
                          << "]" << std::endl;
            }
            else
            {
                M_window_width = w;
                M_window_height = h;
                M_window_x = x;
                M_window_y = y;
            }
        }
        else if ( std::sscanf( geometry.c_str(),
                               " %d %d " ,
                               &x, &y ) == 2 )
        {
            //std::cerr << "only pos = "
            //          << " x=" << x
            //          << " y=" << y
            //          << std::endl;
            M_window_x = x;
            M_window_y = y;
        }
        else
        {
            std::cerr << "Illegal geometry format [" << geometry
                      << "]" << std::endl;
        }
    }

//     if ( ! canvas_size.empty() )
//     {
//         int w = -1, h = -1;
//         if ( std::sscanf( canvas_size.c_str(),
//                           " %d x %d ",
//                           &w, &h ) == 2
//              && w > 1
//              && h > 1 )
//         {
//             M_canvas_width = w;
//             M_canvas_height = h;
//         }
//         else
//         {
//             std::cerr << "Illegal canvas size format [" << canvas_size
//                       << "]" << std::endl;
//         }
//     }
#endif
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Options::setDefaultColor()
{
    M_field_brush.setColor( FIELD_COLOR );
    M_line_pen.setColor( LINE_COLOR );
      //
    M_score_board_pen.setColor( SCORE_BOARD_PEN_COLOR );
    M_score_board_brush.setColor( SCORE_BOARD_BRUSH_COLOR );
      //
    M_ball_pen.setColor( BALL_COLOR );
    M_ball_brush.setColor( BALL_COLOR );
    M_ball_vel_pen.setColor( BALL_VEL_COLOR );
      //
    M_player_pen.setColor( PLAYER_PEN_COLOR );
    M_selected_player_pen.setColor( PLAYER_PEN_COLOR );
    M_left_team_pen.setColor( LEFT_TEAM_COLOR );
    M_left_team_brush.setColor( LEFT_TEAM_COLOR );
    M_left_goalie_pen.setColor( LEFT_GOALIE_COLOR );
    M_left_goalie_stretch_pen.setColor( LEFT_GOALIE_COLOR.darker( 200 ) );
    M_left_goalie_brush.setColor( LEFT_GOALIE_COLOR );
    M_right_team_pen.setColor( RIGHT_TEAM_COLOR );
    M_right_team_brush.setColor( RIGHT_TEAM_COLOR );
    M_right_goalie_pen.setColor( RIGHT_GOALIE_COLOR );
    M_right_goalie_stretch_pen.setColor( RIGHT_GOALIE_COLOR.darker( 200 ) );
    M_right_goalie_brush.setColor( RIGHT_GOALIE_COLOR );
    M_player_number_pen.setColor( PLAYER_NUMBER_COLOR );
    M_player_number_inner_pen.setColor( PLAYER_NUMBER_INNER_COLOR );
    M_neck_pen.setColor( NECK_COLOR );
    M_view_area_pen.setColor( VIEW_AREA_COLOR );
    M_large_view_area_pen.setColor( LARGE_VIEW_AREA_COLOR );
    M_ball_collide_brush.setColor( BALL_COLLIDE_COLOR );
    M_player_collide_brush.setColor( PLAYER_COLLIDE_COLOR );
    M_effort_decayed_pen.setColor( EFFORT_DECAYED_COLOR );
    M_recovery_decayed_pen.setColor( RECOVERY_DECAYED_COLOR );
    M_kick_pen.setColor( KICK_COLOR );
    M_kick_fault_brush.setColor( KICK_FAULT_COLOR );
    M_kick_accel_pen.setColor( KICK_ACCEL_COLOR );
    M_catch_brush.setColor( CATCH_COLOR );
    M_catch_fault_brush.setColor( CATCH_FAULT_COLOR );
    M_tackle_pen.setColor( TACKLE_COLOR );
    M_tackle_brush.setColor( TACKLE_COLOR );
    M_tackle_fault_brush.setColor( TACKLE_FAULT_COLOR );
    M_foul_charged_brush.setColor( FOUL_CHARGED_COLOR );
    M_pointto_pen.setColor( POINTTO_COLOR );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Options::setFieldScale( const double & value )
{
    if ( std::fabs( M_field_scale - value ) > 0.01 )
    {
        M_field_scale = std::max( MIN_FIELD_SCALE, std::min( value, MAX_FIELD_SCALE ) );
        M_zoomed = true;
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Options::zoomIn()
{
    M_zoomed = true;
    M_field_scale *= ZOOM_RATIO;
    if ( M_field_scale > MAX_FIELD_SCALE )
    {
        M_field_scale = MAX_FIELD_SCALE;
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Options::zoomOut()
{
    M_zoomed = true;
    M_field_scale /= ZOOM_RATIO;
    if ( M_field_scale < MIN_FIELD_SCALE )
    {
        M_field_scale = MIN_FIELD_SCALE;
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Options::fitToScreen()
{
    if ( M_zoomed
         || focusType() != Options::FOCUS_POINT
         || focusPoint().x() != 0.0
         || focusPoint().y() != 0.0 )
    {
        M_focus_type = Options::FOCUS_POINT;
        M_focus_point.setX( 0.0 );
        M_focus_point.setY( 0.0 );
        M_zoomed = false;

        int temp_width = M_canvas_width;
        M_canvas_width += 1;
        updateFieldSize( temp_width, M_canvas_height );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Options::updateFieldSize( const int canvas_width,
                          const int canvas_height )
{
    static bool s_team_graphic = true;
    static bool s_score_board = true;

    if ( M_canvas_width != canvas_width
         || M_canvas_height != canvas_height
         || s_team_graphic != showTeamGraphic()
         || s_score_board != showScoreBoard() )
    {
        M_canvas_width = canvas_width;
        M_canvas_height = canvas_height;

        s_team_graphic = showTeamGraphic();
        s_score_board = showScoreBoard();

        // adjust field scale to window size.
        if ( ! zoomed() )
        {
            double total_pitch_l = ( PITCH_LENGTH
                                     + PITCH_MARGIN * 2.0
                                     + 1.0 );
            double total_pitch_w = ( PITCH_WIDTH
                                     + PITCH_MARGIN * 2.0
                                     //+ 1.0
                                     );

            M_field_scale = static_cast< double >( canvas_width ) / total_pitch_l;

            int field_height = canvas_height;
            if ( showTeamGraphic() )
            {
                field_height -= 32; //48; //64;
            }
            if ( showScoreBoard() )
            {
                field_height -= QFontMetrics( scoreBoardFont() ).ascent();
            }

            // automatically adjust a field scale
            if ( total_pitch_w * fieldScale() > field_height )
            {
                M_field_scale = static_cast< double >( field_height ) / total_pitch_w;
            }

            // check the scale threshold
            if ( fieldScale() < MIN_FIELD_SCALE )
            {
                M_field_scale = MIN_FIELD_SCALE;
            }

            M_field_scale = rint( M_field_scale * 100.0 ) / 100.0;
        }
    }

    {
        int field_height = canvas_height;
        int team_graphic_height = 0;
        if ( showTeamGraphic() )
        {
            team_graphic_height = 32; //48; //64;
            field_height -= team_graphic_height;
        }
        if ( showScoreBoard() )
        {
            field_height -= QFontMetrics( scoreBoardFont() ).ascent();
        }

        M_field_center.setX( canvas_width/2 - scale( focusPoint().x() ) );
        M_field_center.setY( team_graphic_height
                             + field_height/2
                             - scale( focusPoint().y() ) );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Options::setFocusPoint( const int screen_x,
                        const int screen_y )
{
    M_focus_type = Options::FOCUS_POINT;

    M_focus_point.setX( fieldX( screen_x ) );
    M_focus_point.setY( fieldY( screen_y ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Options::unselectPlayer()
{
    M_player_select_type = Options::UNSELECT;
    setSelectedNumber( rcss::rcg::NEUTRAL, 0 );
    M_focus_type = Options::FOCUS_POINT;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Options::setPlayerSelectType( const Options::PlayerSelectType type )
{
    if ( type != Options::SELECT_FIX
         && type == M_player_select_type )
    {
        unselectPlayer();
    }
    else
    {
        M_player_select_type = type;
    }
}
