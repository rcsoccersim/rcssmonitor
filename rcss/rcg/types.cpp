// -*-c++-*-

/*!
  \file types.cpp
  \brief Type definitions for rcg data format.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "types.h"

#include <iostream>
#include <iomanip>
#include <cmath>

namespace {

inline
float
quantize( const double val,
          const double prec )
{
    return rint( val / prec ) * prec;
}

template < typename P >
void
to_sexp( std::ostream & os,
             const char * name,
             const P & value )
{
    os << '(' << name << ' ' << value << ')';
}

template <>
void
to_sexp< std::string >( std::ostream & os,
                        const char * name,
                        const std::string & value )
{
    os << '(' << name << ' ' << std::quoted( value ) << ')';
}

static rcss::rcg::PlayerTypeT s_default_type;

}

namespace rcss {
namespace rcg {

PlayerTypeT::PlayerTypeT()
    : id_( 0 ),
      player_speed_max_( 1.2 ),
      stamina_inc_max_( 45.0 ),
      player_decay_( 0.4 ),
      inertia_moment_( 5.0 ),
      dash_power_rate_( 0.06 ),
      player_size_( 0.3 ),
      kickable_margin_( 0.7 ),
      kick_rand_( 0.1 ),
      extra_stamina_( 0.0 ),
      effort_max_( 1.0 ),
      effort_min_( 0.6 ),
      kick_power_rate_( 0.027 ),
      foul_detect_probability_( 0.5 ),
      catchable_area_l_stretch_( 1.0 )
{
    if ( this != &s_default_type )
    {
        player_speed_max_ = s_default_type.player_speed_max_;
        stamina_inc_max_ = s_default_type.stamina_inc_max_;
        player_decay_ = s_default_type.player_decay_;
        inertia_moment_ = s_default_type.inertia_moment_;
        dash_power_rate_ = s_default_type.dash_power_rate_;
        player_size_ = s_default_type.player_size_;
        kickable_margin_ = s_default_type.kickable_margin_;
        kick_rand_ = s_default_type.kick_rand_;
        extra_stamina_ = s_default_type.extra_stamina_;
        effort_max_ = s_default_type.effort_max_;
        effort_min_ = s_default_type.effort_min_;
        kick_power_rate_ = s_default_type.kick_power_rate_;
        foul_detect_probability_ = s_default_type.foul_detect_probability_;
        catchable_area_l_stretch_ = s_default_type.catchable_area_l_stretch_;
    }
}

std::ostream &
PlayerTypeT::toSExp( std::ostream & os ) const
{
    os << "(player_type ";
    to_sexp( os, "id", id_ );
    to_sexp( os, "player_speed_max", quantize( player_speed_max_, 0.00001 ) );
    to_sexp( os, "stamina_inc_max", quantize( stamina_inc_max_, 0.00001 ) );
    to_sexp( os, "player_decay", quantize(player_decay_, 0.000001 ) );
    to_sexp( os, "inertia_moment", quantize( inertia_moment_, 0.00001 ) );
    to_sexp( os, "dash_power_rate", quantize( dash_power_rate_, 0.00000001 ) );
    to_sexp( os, "player_size", quantize( player_size_, 0.00001 ) );
    to_sexp( os, "kickable_margin", quantize( kickable_margin_, 0.000001 ) );
    to_sexp( os, "kick_rand", quantize( kick_rand_, 0.000001 ) );
    to_sexp( os, "extra_stamina", quantize( extra_stamina_, 0.00001 ) );
    to_sexp( os, "effort_max", quantize( effort_max_, 0.000001 ) );
    to_sexp( os, "effort_min", quantize( effort_min_, 0.000001 ) );
    // 14.0
    to_sexp( os, "kick_power_rate", quantize( kick_power_rate_, 0.000001 ) );
    to_sexp( os, "foul_detect_probability", quantize( foul_detect_probability_, 0.000001 ) );
    to_sexp( os, "catchable_area_l_stretch", quantize( catchable_area_l_stretch_, 0.000001 ) );
    os << ')';

    return os;
}

void
PlayerTypeT::set_default( const ServerParamT & param )
{
    s_default_type.player_speed_max_ = param.player_speed_max_;
    s_default_type.stamina_inc_max_ = param.stamina_inc_max_;
    s_default_type.player_decay_ = param.player_decay_;
    s_default_type.inertia_moment_ = param.inertia_moment_;
    s_default_type.dash_power_rate_ = param.dash_power_rate_;
    s_default_type.player_size_ = param.player_size_;
    s_default_type.kickable_margin_ = param.kickable_margin_;
    s_default_type.kick_rand_ = param.kick_rand_;
    s_default_type.extra_stamina_ = param.extra_stamina_;
    s_default_type.effort_max_ = param.effort_init_;
    s_default_type.effort_min_ = param.effort_min_;
    s_default_type.kick_power_rate_ = param.kick_power_rate_;
    s_default_type.foul_detect_probability_ = param.foul_detect_probability_;
    s_default_type.catchable_area_l_stretch_ = 1.0;
}

PlayerParamT::PlayerParamT()
    : player_types_( 18 ),
      subs_max_( 3 ),
      pt_max_( 1 ),
      allow_mult_default_type_( false ),
      player_speed_max_delta_min_( 0.0 ),
      player_speed_max_delta_max_( 0.0 ),
      stamina_inc_max_delta_factor_( 0.0 ),
      player_decay_delta_min_( -0.05 ),
      player_decay_delta_max_( 0.1 ),
      inertia_moment_delta_factor_( 25.0 ),
      dash_power_rate_delta_min_( 0.0 ),
      dash_power_rate_delta_max_( 0.0 ),
      player_size_delta_factor_( -100.0 ),
      kickable_margin_delta_min_( -0.1 ),
      kickable_margin_delta_max_( 0.1 ),
      kick_rand_delta_factor_( 1.0 ),
      extra_stamina_delta_min_( 0.0 ),
      extra_stamina_delta_max_( 100.0 ),
      effort_max_delta_factor_( -0.002 ),
      effort_min_delta_factor_( -0.002 ),
      new_dash_power_rate_delta_min_( -0.0005 ),
      new_dash_power_rate_delta_max_( 0.0015 ),
      new_stamina_inc_max_delta_factor_( -6000.0 ),
      random_seed_( -1 ),
      kick_power_rate_delta_min_( 0.0 ),
      kick_power_rate_delta_max_( 0.0 ),
      foul_detect_probability_delta_factor_( 0.0 ),
      catchable_area_l_stretch_min_( 0.0 ),
      catchable_area_l_stretch_max_( 0.0 )
{

}

std::ostream &
PlayerParamT::toSExp( std::ostream & os ) const
{
    os << "(player_param ";
    to_sexp( os, "player_types", player_types_ );
    to_sexp( os, "subs_max", subs_max_ );
    to_sexp( os, "pt_max", pt_max_ );
    to_sexp( os, "allow_mult_default_type", allow_mult_default_type_ );
    to_sexp( os, "player_speed_max_delta_min", quantize( player_speed_max_delta_min_, 0.00001 ) );
    to_sexp( os, "player_speed_max_delta_max", quantize( player_speed_max_delta_max_, 0.00001 ) );
    to_sexp( os, "stamina_inc_max_delta_factor", quantize( stamina_inc_max_delta_factor_, 0.00001 ) );
    to_sexp( os, "player_decay_delta_min", quantize( player_decay_delta_min_, 0.00001 ) );
    to_sexp( os, "player_decay_delta_max", quantize( player_decay_delta_max_, 0.00001 ) );
    to_sexp( os, "inertia_moment_delta_factor", quantize( inertia_moment_delta_factor_, 0.00001 ) );
    to_sexp( os, "dash_power_rate_delta_min", quantize( dash_power_rate_delta_min_, 0.00001 ) );
    to_sexp( os, "dash_power_rate_delta_max", quantize( dash_power_rate_delta_max_, 0.00001 ) );
    to_sexp( os, "player_size_delta_factor", quantize( player_size_delta_factor_, 0.00001 ) );
    to_sexp( os, "kickable_margin_delta_min", quantize( kickable_margin_delta_min_, 0.00001 ) );
    to_sexp( os, "kickable_margin_delta_max", quantize( kickable_margin_delta_max_, 0.00001 ) );
    to_sexp( os, "kick_rand_delta_factor", quantize( kick_rand_delta_factor_, 0.00001 ) );
    to_sexp( os, "extra_stamina_delta_min", quantize( extra_stamina_delta_min_, 0.00001 ) );
    to_sexp( os, "extra_stamina_delta_max", quantize( extra_stamina_delta_max_, 0.00001 ) );
    to_sexp( os, "effort_max_delta_factor", quantize( effort_max_delta_factor_, 0.00001 ) );
    to_sexp( os, "effort_min_delta_factor", quantize( effort_min_delta_factor_, 0.00001 ) );
    to_sexp( os, "random_seed", random_seed_ );
    to_sexp( os, "new_dash_power_rate_delta_min", quantize( new_dash_power_rate_delta_min_, 0.00001 ) );
    to_sexp( os, "new_dash_power_rate_delta_max", quantize( new_dash_power_rate_delta_max_, 0.00001 ) );
    to_sexp( os, "new_stamina_inc_max_delta_factor", quantize( new_stamina_inc_max_delta_factor_, 0.00001 ) );
    // 14.0
    to_sexp( os, "kick_power_rate_delta_min", quantize( kick_power_rate_delta_min_, 0.000001 ) );
    to_sexp( os, "kick_power_rate_delta_max", quantize( kick_power_rate_delta_max_, 0.000001 ) );
    to_sexp( os, "foul_detect_probability_delta_factor", quantize( foul_detect_probability_delta_factor_, 0.00001 ) );
    to_sexp( os, "catchable_area_l_stretch_min", quantize( catchable_area_l_stretch_min_, 0.00001 ) );
    to_sexp( os, "catchable_area_l_stretch_max", quantize( catchable_area_l_stretch_max_, 0.00001 ) );
    os << ')';

    return os;
}


ServerParamT::ServerParamT()
    : goal_width_( 14.02 ),
      inertia_moment_( 5.0 ),
      player_size_( 0.3 ),
      player_decay_( 0.4 ),
      player_rand_( 0.1 ),
      player_weight_( 60.0 ),
      player_speed_max_( 1.2 ),
      player_accel_max_( 1.0 ),
      stamina_max_( 4000.0 ),
      stamina_inc_max_( 45.0 ),
      recover_init_( 1.0 ),
      recover_dec_thr_( 0.3 ),
      recover_min_( 0.5 ),
      recover_dec_( 0.002 ),
      effort_init_( 1.0 ),
      effort_dec_thr_( 0.3 ),
      effort_min_( 0.6 ),
      effort_dec_( 0.005 ),
      effort_inc_thr_( 0.6 ),
      effort_inc_( 0.01 ),
      kick_rand_( 0.1 ),
      team_actuator_noise_( false ),
      player_rand_factor_l_( 1.0 ),
      player_rand_factor_r_( 1.0 ),
      kick_rand_factor_l_( 1.0 ),
      kick_rand_factor_r_( 1.0 ),
      ball_size_( 0.085 ),
      ball_decay_( 0.94 ),
      ball_rand_( 0.05 ),
      ball_weight_( 0.2 ),
      ball_speed_max_( 3.0 ),
      ball_accel_max_( 2.7 ),
      dash_power_rate_( 0.006 ),
      kick_power_rate_( 0.027 ),
      kickable_margin_( 0.7 ),
      control_radius_( 2.0 ),
      //control_radius_width_( 1.7 ),
      catch_probability_( 1.0 ),
      catchable_area_l_( 1.2 ),
      catchable_area_w_( 1.0 ),
      goalie_max_moves_( 2 ),
      max_power_( 100.0 ),
      min_power_( -100.0 ),
      max_moment_( 180.0 ),
      min_moment_( -180.0 ),
      max_neck_moment_( 180.0 ),
      min_neck_moment_( -180.0 ),
      max_neck_angle_( 90.0 ),
      min_neck_angle_( -90.0 ),
      visible_angle_( 90.0 ),
      visible_distance_( 3.0 ),
      audio_cut_dist_( 50.0 ),
      dist_quantize_step_( 0.1 ),
      landmark_dist_quantize_step_( 0.01 ),
      corner_kick_margin_( 1.0 ),
      wind_dir_( 0.0 ),
      wind_force_( 0.0 ),
      wind_angle_( 0.0 ),
      wind_rand_( 0.0 ),
      wind_none_( false ),
      use_wind_random_( false ),
      half_time_( 300 ),
      drop_ball_time_( 200 ),
      port_( 6000 ),
      coach_port_( 6001 ),
      online_coach_port_( 6002 ),
      say_coach_count_max_( 128 ),
      say_coach_msg_size_( 128 ),
      simulator_step_( 100 ),
      send_step_( 150 ),
      recv_step_( 10 ),
      sense_body_step_( 100 ),
      say_msg_size_( 10 ),
      clang_win_size_( 300 ),
      clang_define_win_( 1 ),
      clang_meta_win_( 1 ),
      clang_advice_win_( 1 ),
      clang_info_win_( 1 ),
      clang_del_win_( 1 ),
      clang_rule_win_( 1 ),
      clang_mess_delay_( 50 ),
      clang_mess_per_cycle_( 1 ),
      hear_max_( 1 ),
      hear_inc_( 1 ),
      hear_decay_( 1 ),
      catch_ban_cycle_( 5 ),
      coach_mode_( false ),
      coach_with_referee_mode_( false ),
      use_old_coach_hear_( false ),
      online_coach_look_step_( 100 ),
      use_offside_( true ),
      offside_active_area_size_( 2.5 ),
      kick_off_offside_( true ),
      verbose_( false ),
      offside_kick_margin_( 9.15 ),
      slow_down_factor_( 1 ),
      synch_mode_( false ),
      synch_offset_( 60 ),
      synch_micro_sleep_( 1 ),
      start_goal_l_( 0 ),
      start_goal_r_( 0 ),
      fullstate_l_( false ),
      fullstate_r_( false ),
      slowness_on_top_for_left_team_( 1.0 ),
      slowness_on_top_for_right_team_( 1.0 ),
      landmark_file_(),
      send_comms_( false ),
      text_logging_( true ),
      game_logging_( true ),
      game_log_version_( 4 ),
      text_log_dir_( "." ),
      game_log_dir_( "." ),
      text_log_fixed_name_( "rcssserver" ),
      game_log_fixed_name_( "rcssserver" ),
      text_log_fixed_( false ),
      game_log_fixed_( false ),
      text_log_dated_( true ),
      game_log_dated_( true ),
      log_date_format_( "%Y%m%d%H%M-" ),
      log_times_( false ),
      record_messages_( false ),
      text_log_compression_( 0 ),
      game_log_compression_( 0 ),
      profile_( false ),
      point_to_ban_( 5 ),
      point_to_duration_( 20 ),
      tackle_dist_( 2.0 ),
      tackle_back_dist_( 0.5 ),
      tackle_width_( 1.0 ),
      tackle_exponent_( 6.0 ),
      tackle_cycles_( 10 ),
      tackle_power_rate_( 0.027 ),
      freeform_wait_period_( 600 ),
      freeform_send_period_( 20 ),
      free_kick_faults_( true ),
      back_passes_( true ),
      proper_goal_kicks_( false ),
      stopped_ball_vel_( 0.01 ),
      max_goal_kicks_( 3 ),
      auto_mode_( false ),
      kick_off_wait_( 100 ),
      connect_wait_( 300 ),
      game_over_wait_( 100 ),
      team_l_start_(),
      team_r_start_(),
      keepaway_mode_( false ),
      keepaway_length_( 20.0 ),
      keepaway_width_( 20.0 ),
      keepaway_logging_( true ),
      keepaway_log_dir_( "." ),
      keepaway_log_fixed_name_( "rcssserver" ),
      keepaway_log_fixed_( false ),
      keepaway_log_dated_( true ),
      keepaway_start_( -1 ),
      nr_normal_halfs_( 2 ),
      nr_extra_halfs_( 2 ),
      penalty_shoot_outs_( true ),
      pen_before_setup_wait_( 30 ),
      pen_setup_wait_( 100 ),
      pen_ready_wait_( 50 ),
      pen_taken_wait_( 200 ),
      pen_nr_kicks_( 5 ),
      pen_max_extra_kicks_( 10 ),
      pen_dist_x_( 42.5 ),
      pen_random_winner_( false ),
      pen_max_goalie_dist_x_( 14.0 ),
      pen_allow_mult_kicks_( true ),
      pen_coach_moves_players_( true ),
      ball_stuck_area_( 3.0 ),
      coach_msg_file_(),
      max_tackle_power_( 100.0 ),
      max_back_tackle_power_( 50.0 ),
      player_speed_max_min_( 0.8 ),
      extra_stamina_( 0.0 ),
      synch_see_offset_( 30 ),
      max_monitors_( -1 ),
      extra_half_time_( 300 ),
      stamina_capacity_( -1.0 ),
      max_dash_angle_( 0.0 ),
      min_dash_angle_( 0.0 ),
      dash_angle_step_( 90.0 ),
      side_dash_rate_( 0.25 ),
      back_dash_rate_( 0.5 ),
      max_dash_power_( 100.0 ),
      min_dash_power_( -100.0 ),
      tackle_rand_factor_( 1.0 ),
      foul_detect_probability_( 0.5 ),
      foul_exponent_( 10.0 ),
      foul_cycles_( 5 ),
      golden_goal_( true ),
      red_card_probability_( 0.0 ),
      illegal_defense_duration_( 20 ),
      illegal_defense_number_( 0 ),
      illegal_defense_dist_x_( 16.5 ),
      illegal_defense_width_( 40.32 ),
      fixed_teamname_l_( "" ),
      fixed_teamname_r_( "" )
{

}


std::ostream &
ServerParamT::toSExp( std::ostream & os ) const
{
    os << "(server_param ";
    to_sexp( os, "goal_width", quantize( goal_width_, 0.00001 ) );
    to_sexp( os, "inertia_moment", quantize( inertia_moment_, 0.00001 ) );
    to_sexp( os, "player_size", quantize( player_size_, 0.00001 ) );
    to_sexp( os, "player_decay", quantize( player_decay_, 0.00001 ) );
    to_sexp( os, "player_rand", quantize( player_rand_, 0.00001 ) );
    to_sexp( os, "player_weight", quantize( player_weight_, 0.00001 ) );
    to_sexp( os, "player_speed_max", quantize( player_speed_max_, 0.00001 ) );
    to_sexp( os, "player_accel_max", quantize( player_accel_max_ , 0.00001 ) );
    to_sexp( os, "stamina_max", quantize( stamina_max_, 0.00001 ) );
    to_sexp( os, "stamina_inc_max", quantize( stamina_inc_max_, 0.00001 ) );
    to_sexp( os, "recover_init", quantize( recover_init_, 0.00001 ) );
    to_sexp( os, "recover_dec_thr", quantize( recover_dec_thr_, 0.00001 ) );
    to_sexp( os, "recover_min", quantize( recover_min_, 0.00001 ) );
    to_sexp( os, "recover_dec", quantize( recover_dec_, 0.00001 ) );
    to_sexp( os, "effort_init", quantize( effort_init_, 0.00001 ) );
    to_sexp( os, "effort_dec_thr", quantize( effort_dec_thr_, 0.00001 ) );
    to_sexp( os, "effort_min", quantize( effort_min_, 0.00001 ) );
    to_sexp( os, "effort_dec", quantize( effort_dec_, 0.00001 ) );
    to_sexp( os, "effort_inc_thr", quantize( effort_inc_thr_, 0.00001 ) );
    to_sexp( os, "effort_inc", quantize( effort_inc_, 0.00001 ) );
    to_sexp( os, "kick_rand", quantize( kick_rand_, 0.00001 ) );
    to_sexp( os, "team_actuator_noise", team_actuator_noise_ );
    to_sexp( os, "prand_factor_l", quantize( player_rand_factor_l_, 0.00001 ) );
    to_sexp( os, "prand_factor_r", quantize( player_rand_factor_r_, 0.00001 ) );
    to_sexp( os, "kick_rand_factor_l", quantize( kick_rand_factor_l_, 0.00001 ) );
    to_sexp( os, "kick_rand_factor_r", quantize( kick_rand_factor_r_, 0.00001 ) );
    to_sexp( os, "ball_size", quantize( ball_size_, 0.00001 ) );
    to_sexp( os, "ball_decay", quantize( ball_decay_, 0.00001 ) );
    to_sexp( os, "ball_rand", quantize( ball_rand_, 0.00001 ) );
    to_sexp( os, "ball_weight", quantize( ball_weight_, 0.00001 ) );
    to_sexp( os, "ball_speed_max", quantize( ball_speed_max_, 0.00001 ) );
    to_sexp( os, "ball_accel_max", quantize( ball_accel_max_, 0.00001 ) );
    to_sexp( os, "dash_power_rate", quantize( dash_power_rate_, 0.00001 ) );
    to_sexp( os, "kick_power_rate", quantize( kick_power_rate_, 0.00001 ) );
    to_sexp( os, "kickable_margin", quantize( kickable_margin_, 0.00001 ) );
    to_sexp( os, "control_radius", quantize( control_radius_, 0.00001 ) );
    //( "control_radius_width", control_radius_width_ );
    //( "kickable_area", kickable_area_ );
    to_sexp( os, "catch_probability", quantize( catch_probability_, 0.00001 ) );
    to_sexp( os, "catchable_area_l", quantize( catchable_area_l_, 0.00001 ) );
    to_sexp( os, "catchable_area_w", quantize( catchable_area_w_, 0.00001 ) );
    to_sexp( os, "goalie_max_moves", goalie_max_moves_ );
    to_sexp( os, "maxpower", quantize( max_power_, 0.00001 ) );
    to_sexp( os, "minpower", quantize( min_power_, 0.00001 ) );
    to_sexp( os, "maxmoment", quantize( max_moment_, 0.00001 ) );
    to_sexp( os, "minmoment", quantize( min_moment_, 0.00001 ) );
    to_sexp( os, "maxneckmoment", quantize( max_neck_moment_, 0.00001 ) );
    to_sexp( os, "minneckmoment", quantize( min_neck_moment_, 0.00001 ) );
    to_sexp( os, "maxneckang", quantize( max_neck_angle_, 0.00001 ) );
    to_sexp( os, "minneckang", quantize( min_neck_angle_, 0.00001 ) );
    to_sexp( os, "visible_angle", quantize( visible_angle_, 0.00001 ) );
    to_sexp( os, "visible_distance", quantize( visible_distance_, 0.00001 ) );
    to_sexp( os, "audio_cut_dist", quantize( audio_cut_dist_, 0.00001 ) );
    to_sexp( os, "quantize_step", quantize( dist_quantize_step_, 0.00001 ) );
    to_sexp( os, "quantize_step_l", quantize( landmark_dist_quantize_step_, 0.00001 ) );
    //( "quantize_step_dir", dir_quantize_step_ );
    //( "quantize_step_dist_team_l", dist_quantize_step_l_ );
    //( "quantize_step_dist_team_r", dist_quantize_step_r_ );
    //( "quantize_step_dist_l_team_l", landmark_dist_quantize_step_l_ );
    //( "quantize_step_dist_l_team_r", landmark_dist_quantize_step_r_ );
    //( "quantize_step_dir_team_l", dir_quantize_step_l_ );
    //( "quantize_step_dir_team_r", dir_quantize_step_r_ );
    to_sexp( os, "ckick_margin", quantize( corner_kick_margin_, 0.00001 ) );
    to_sexp( os, "wind_dir", quantize( wind_dir_, 0.00001 ) );
    to_sexp( os, "wind_force", quantize( wind_force_, 0.00001 ) );
    to_sexp( os, "wind_ang", quantize( wind_angle_, 0.00001 ) );
    to_sexp( os, "wind_rand", quantize( wind_rand_, 0.00001 ) );
    to_sexp( os, "wind_none", wind_none_ );
    to_sexp( os, "wind_random", use_wind_random_ );
    to_sexp( os, "half_time", half_time_ );
    to_sexp( os, "drop_ball_time", drop_ball_time_ );
    to_sexp( os, "port", port_ );
    to_sexp( os, "coach_port", coach_port_ );
    to_sexp( os, "olcoach_port", online_coach_port_ );
    to_sexp( os, "say_coach_cnt_max", say_coach_count_max_ );
    to_sexp( os, "say_coach_msg_size", say_coach_msg_size_ );
    to_sexp( os, "simulator_step", simulator_step_ );
    to_sexp( os, "send_step", send_step_ );
    to_sexp( os, "recv_step", recv_step_ );
    to_sexp( os, "sense_body_step", sense_body_step_ );
    //( "lcm_step", lcm_step_ ); // not needed
    to_sexp( os, "say_msg_size", say_msg_size_ );
    to_sexp( os, "clang_win_size", clang_win_size_ );
    to_sexp( os, "clang_define_win", clang_define_win_ );
    to_sexp( os, "clang_meta_win", clang_meta_win_ );
    to_sexp( os, "clang_advice_win", clang_advice_win_ );
    to_sexp( os, "clang_info_win", clang_info_win_ );
    to_sexp( os, "clang_del_win", clang_del_win_ );
    to_sexp( os, "clang_rule_win", clang_rule_win_ );
    to_sexp( os, "clang_mess_delay", clang_mess_delay_ );
    to_sexp( os, "clang_mess_per_cycle", clang_mess_per_cycle_ );
    to_sexp( os, "hear_max", hear_max_ );
    to_sexp( os, "hear_inc", hear_inc_ );
    to_sexp( os, "hear_decay", hear_decay_ );
    to_sexp( os, "catch_ban_cycle", catch_ban_cycle_ );
    to_sexp( os, "coach", coach_mode_ );
    to_sexp( os, "coach_w_referee", coach_with_referee_mode_ );
    to_sexp( os, "old_coach_hear", use_old_coach_hear_ );
    to_sexp( os, "send_vi_step", online_coach_look_step_ );
    to_sexp( os, "use_offside", use_offside_ );
    to_sexp( os, "offside_kick_margin", quantize( offside_kick_margin_, 0.00001 ) );
    to_sexp( os, "forbid_kick_off_offside", kick_off_offside_ );
    to_sexp( os, "verbose", verbose_ );
    to_sexp( os, "offside_active_area_size", quantize( offside_active_area_size_, 0.00001 ) );
    to_sexp( os, "slow_down_factor", slow_down_factor_ );
    to_sexp( os, "synch_mode", synch_mode_ );
    to_sexp( os, "synch_offset", synch_offset_ );
    to_sexp( os, "synch_micro_sleep", synch_micro_sleep_ );
    to_sexp( os, "start_goal_l", start_goal_l_ );
    to_sexp( os, "start_goal_r", start_goal_r_ );
    to_sexp( os, "fullstate_l", fullstate_l_ );
    to_sexp( os, "fullstate_r", fullstate_r_ );
    to_sexp( os, "slowness_on_top_for_left_team", quantize( slowness_on_top_for_left_team_, 0.00001 ) );
    to_sexp( os, "slowness_on_top_for_right_team", quantize( slowness_on_top_for_right_team_, 0.00001 ) );
    to_sexp( os, "landmark_file", landmark_file_ );
    to_sexp( os, "send_comms", send_comms_ );
    to_sexp( os, "text_logging", text_logging_ );
    to_sexp( os, "game_logging", game_logging_ );
    to_sexp( os, "game_log_version", game_log_version_ );
    to_sexp( os, "text_log_dir", text_log_dir_ );
    to_sexp( os, "game_log_dir", game_log_dir_ );
    to_sexp( os, "text_log_fixed_name", text_log_fixed_name_ );
    to_sexp( os, "game_log_fixed_name", game_log_fixed_name_ );
    to_sexp( os, "text_log_fixed", text_log_fixed_ );
    to_sexp( os, "game_log_fixed", game_log_fixed_ );
    to_sexp( os, "text_log_dated", text_log_dated_ );
    to_sexp( os, "game_log_dated", game_log_dated_ );
    to_sexp( os, "log_date_format", log_date_format_ );
    to_sexp( os, "log_times", log_times_ );
    to_sexp( os, "record_messages", record_messages_ );
    to_sexp( os, "text_log_compression", text_log_compression_ );
    to_sexp( os, "game_log_compression", game_log_compression_ );
    to_sexp( os, "profile", profile_ );
    to_sexp( os, "point_to_ban", point_to_ban_ );
    to_sexp( os, "point_to_duration", point_to_duration_ );
    to_sexp( os, "tackle_dist", quantize( tackle_dist_, 0.00001 ) );
    to_sexp( os, "tackle_back_dist", quantize( tackle_back_dist_, 0.00001 ) );
    to_sexp( os, "tackle_width", quantize( tackle_width_, 0.00001 ) );
    to_sexp( os, "tackle_exponent", quantize( tackle_exponent_, 0.00001 ) );
    to_sexp( os, "tackle_cycles", tackle_cycles_ );
    to_sexp( os, "tackle_power_rate", quantize( tackle_power_rate_, 0.00001 ) );
    to_sexp( os, "freeform_wait_period", freeform_wait_period_ );
    to_sexp( os, "freeform_send_period", freeform_send_period_ );
    to_sexp( os, "free_kick_faults", free_kick_faults_ );
    to_sexp( os, "back_passes", back_passes_ );
    to_sexp( os, "proper_goal_kicks", proper_goal_kicks_ );
    to_sexp( os, "stopped_ball_vel", quantize( stopped_ball_vel_, 0.00001 ) );
    to_sexp( os, "max_goal_kicks", max_goal_kicks_ );
    to_sexp( os, "auto_mode", auto_mode_ );
    to_sexp( os, "kick_off_wait", kick_off_wait_ );
    to_sexp( os, "connect_wait", connect_wait_ );
    to_sexp( os, "game_over_wait", game_over_wait_ );
    to_sexp( os, "team_l_start", team_l_start_ );
    to_sexp( os, "team_r_start", team_r_start_ );
    to_sexp( os, "keepaway", keepaway_mode_ );
    to_sexp( os, "keepaway_length", quantize( keepaway_length_, 0.00001 ) );
    to_sexp( os, "keepaway_width", quantize( keepaway_width_, 0.00001 ) );
    to_sexp( os, "keepaway_logging", keepaway_logging_ );
    to_sexp( os, "keepaway_log_dir", keepaway_log_dir_ );
    to_sexp( os, "keepaway_log_fixed_name", keepaway_log_fixed_name_ );
    to_sexp( os, "keepaway_log_fixed", keepaway_log_fixed_ );
    to_sexp( os, "keepaway_log_dated", keepaway_log_dated_ );
    to_sexp( os, "keepaway_start", keepaway_start_ );
    to_sexp( os, "nr_normal_halfs", nr_normal_halfs_ );
    to_sexp( os, "nr_extra_halfs", nr_extra_halfs_ );
    to_sexp( os, "penalty_shoot_outs", penalty_shoot_outs_ );
    to_sexp( os, "pen_before_setup_wait", pen_before_setup_wait_ );
    to_sexp( os, "pen_setup_wait", pen_setup_wait_ );
    to_sexp( os, "pen_ready_wait", pen_ready_wait_ );
    to_sexp( os, "pen_taken_wait", pen_taken_wait_ );
    to_sexp( os, "pen_nr_kicks", pen_nr_kicks_ );
    to_sexp( os, "pen_max_extra_kicks", pen_max_extra_kicks_ );
    to_sexp( os, "pen_dist_x", quantize( pen_dist_x_, 0.00001 ) );
    to_sexp( os, "pen_random_winner", pen_random_winner_ );
    to_sexp( os, "pen_max_goalie_dist_x", quantize( pen_max_goalie_dist_x_, 0.00001 ) );
    to_sexp( os, "pen_allow_mult_kicks", pen_allow_mult_kicks_ );
    to_sexp( os, "pen_coach_moves_players", pen_coach_moves_players_ );
    // 11
    to_sexp( os, "ball_stuck_area", quantize( ball_stuck_area_, 0.00001 ) );
    to_sexp( os, "coach_msg_file", coach_msg_file_ );
    // 12
    to_sexp( os, "max_tackle_power", quantize( max_tackle_power_, 0.00001 ) );
    to_sexp( os, "max_back_tackle_power", quantize( max_back_tackle_power_, 0.00001 ) );
    to_sexp( os, "player_speed_max_min", quantize( player_speed_max_min_, 0.00001 ) );
    to_sexp( os, "extra_stamina", quantize( extra_stamina_, 0.00001 ) );
    to_sexp( os, "synch_see_offset", synch_see_offset_ );
    to_sexp( os, "max_monitors", max_monitors_ );
    // 12.1.3
    to_sexp( os, "extra_half_time", extra_half_time_ );
    // v13
    to_sexp( os, "stamina_capacity", quantize( stamina_capacity_, 0.00001 ) );
    to_sexp( os, "max_dash_angle", quantize( max_dash_angle_, 0.00001 ) );
    to_sexp( os, "min_dash_angle", quantize( min_dash_angle_, 0.00001 ) );
    to_sexp( os, "dash_angle_step", quantize( dash_angle_step_, 0.00001 ) );
    to_sexp( os, "side_dash_rate", quantize( side_dash_rate_, 0.00001 ) );
    to_sexp( os, "back_dash_rate", quantize( back_dash_rate_, 0.00001 ) );
    to_sexp( os, "max_dash_power", quantize( max_dash_power_, 0.00001 ) );
    to_sexp( os, "min_dash_power", quantize( min_dash_power_, 0.00001 ) );
    // 14.0
    to_sexp( os, "tackle_rand_factor", quantize( tackle_rand_factor_, 0.00001 ) );
    to_sexp( os, "foul_detect_probability", quantize( foul_detect_probability_, 0.00001 ) );
    to_sexp( os, "foul_exponent", quantize( foul_exponent_, 0.00001 ) );
    to_sexp( os, "foul_cycles", foul_cycles_ );
    to_sexp( os, "golden_goal", golden_goal_ );
    // 15.0
    to_sexp( os, "red_card_probability", quantize( red_card_probability_, 0.00001 ) );
    // 16.0
    to_sexp( os, "illegal_defense_duration", illegal_defense_duration_ );
    to_sexp( os, "illegal_defense_number", illegal_defense_number_ );
    to_sexp( os, "illegal_defense_dist_x", illegal_defense_dist_x_ );
    to_sexp( os, "illegal_defense_width", illegal_defense_width_ );
    to_sexp( os, "fixed_teamname_l", fixed_teamname_l_ );
    to_sexp( os, "fixed_teamname_r", fixed_teamname_r_ );
    // 17.0
    to_sexp( os, "max_catch_angle", quantize( max_catch_angle_, 0.00001 ) );
    to_sexp( os, "min_catch_angle", quantize( min_catch_angle_, 0.00001 ) );

    os << ')';

    return os;
}

}
}
