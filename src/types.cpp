// -*-c++-*-

/*!
  \file types.cpp
  \brief Type definitions for monitor protocol.
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

#include "types.h"

#include <iostream>
#include <cmath>

namespace {

inline
float
quantize( const double & val,
          const double & prec )
{
    return rintf( val / prec ) * prec;
}

template < typename P >
void
print_param( std::ostream & os,
             const char * name,
             const P & value )
{
    os << '(' << name << ' ' << value << ')';
}

template <>
void
print_param< std::string >( std::ostream & os,
                            const char * name,
                            const std::string & value )
{
    os << '(' << name << " \"" << value << "\")";
}

}

namespace rcss {
namespace rcg {

std::ostream &
PlayerTypeT::print( std::ostream & os ) const
{
    os << "(player_type ";
    print_param( os, "id", id_ );
    print_param( os, "player_speed_max", quantize( player_speed_max_, 0.00001 ) );
    print_param( os, "stamina_inc_max", quantize( stamina_inc_max_, 0.00001 ) );
    print_param( os, "player_decay", quantize(player_decay_, 0.000001 ) );
    print_param( os, "inertia_moment", quantize( inertia_moment_, 0.00001 ) );
    print_param( os, "dash_power_rate", quantize( dash_power_rate_, 0.00000001 ) );
    print_param( os, "player_size", quantize( player_size_, 0.00001 ) );
    print_param( os, "kickable_margin", quantize( kickable_margin_, 0.000001 ) );
    print_param( os, "kick_rand", quantize( kick_rand_, 0.000001 ) );
    print_param( os, "extra_stamina", quantize( extra_stamina_, 0.00001 ) );
    print_param( os, "effort_max", quantize( effort_max_, 0.000001 ) );
    print_param( os, "effort_min", quantize( effort_min_, 0.000001 ) );
    os << ')';

    return os;
}

std::ostream &
PlayerParamT::print( std::ostream & os ) const
{
    os << "(player_param ";
    print_param( os, "player_types", player_types_ );
    print_param( os, "subs_max", subs_max_ );
    print_param( os, "pt_max", pt_max_ );
    print_param( os, "allow_mult_default_type", allow_mult_default_type_ );
    print_param( os, "player_speed_max_delta_min", quantize( player_speed_max_delta_min_, 0.00001 ) );
    print_param( os, "player_speed_max_delta_max", quantize( player_speed_max_delta_max_, 0.00001 ) );
    print_param( os, "stamina_inc_max_delta_factor", quantize( stamina_inc_max_delta_factor_, 0.00001 ) );
    print_param( os, "player_decay_delta_min", quantize( player_decay_delta_min_, 0.00001 ) );
    print_param( os, "player_decay_delta_max", quantize( player_decay_delta_max_, 0.00001 ) );
    print_param( os, "inertia_moment_delta_factor", quantize( inertia_moment_delta_factor_, 0.00001 ) );
    print_param( os, "dash_power_rate_delta_min", quantize( dash_power_rate_delta_min_, 0.00001 ) );
    print_param( os, "dash_power_rate_delta_max", quantize( dash_power_rate_delta_max_, 0.00001 ) );
    print_param( os, "player_size_delta_factor", quantize( player_size_delta_factor_, 0.00001 ) );
    print_param( os, "kickable_margin_delta_min", quantize( kickable_margin_delta_min_, 0.00001 ) );
    print_param( os, "kickable_margin_delta_max", quantize( kickable_margin_delta_max_, 0.00001 ) );
    print_param( os, "kick_rand_delta_factor", quantize( kick_rand_delta_factor_, 0.00001 ) );
    print_param( os, "extra_stamina_delta_min", quantize( extra_stamina_delta_min_, 0.00001 ) );
    print_param( os, "extra_stamina_delta_max", quantize( extra_stamina_delta_max_, 0.00001 ) );
    print_param( os, "effort_max_delta_factor", quantize( effort_max_delta_factor_, 0.00001 ) );
    print_param( os, "effort_min_delta_factor", quantize( effort_min_delta_factor_, 0.00001 ) );
    print_param( os, "random_seed", random_seed_ );
    print_param( os, "new_dash_power_rate_delta_min", quantize( new_dash_power_rate_delta_min_, 0.00001 ) );
    print_param( os, "new_dash_power_rate_delta_max", quantize( new_dash_power_rate_delta_max_, 0.00001 ) );
    print_param( os, "new_stamina_inc_max_delta_factor", quantize( new_stamina_inc_max_delta_factor_, 0.00001 ) );
    os << ')';

    return os;
}

std::ostream &
ServerParamT::print( std::ostream & os ) const
{
    os << "(server_param ";
    print_param( os, "goal_width", quantize( goal_width_, 0.00001 ) );
    print_param( os, "inertia_moment", quantize( inertia_moment_, 0.00001 ) );
    print_param( os, "player_size", quantize( player_size_, 0.00001 ) );
    print_param( os, "player_decay", quantize( player_decay_, 0.00001 ) );
    print_param( os, "player_rand", quantize( player_rand_, 0.00001 ) );
    print_param( os, "player_weight", quantize( player_weight_, 0.00001 ) );
    print_param( os, "player_speed_max", quantize( player_speed_max_, 0.00001 ) );
    print_param( os, "player_accel_max", quantize( player_accel_max_ , 0.00001 ) );
    print_param( os, "stamina_max", quantize( stamina_max_, 0.00001 ) );
    print_param( os, "stamina_inc_max", quantize( stamina_inc_max_, 0.00001 ) );
    print_param( os, "recover_init", quantize( recover_init_, 0.00001 ) );
    print_param( os, "recover_dec_thr", quantize( recover_dec_thr_, 0.00001 ) );
    print_param( os, "recover_min", quantize( recover_min_, 0.00001 ) );
    print_param( os, "recover_dec", quantize( recover_dec_, 0.00001 ) );
    print_param( os, "effort_init", quantize( effort_init_, 0.00001 ) );
    print_param( os, "effort_dec_thr", quantize( effort_dec_thr_, 0.00001 ) );
    print_param( os, "effort_min", quantize( effort_min_, 0.00001 ) );
    print_param( os, "effort_dec", quantize( effort_dec_, 0.00001 ) );
    print_param( os, "effort_inc_thr", quantize( effort_inc_thr_, 0.00001 ) );
    print_param( os, "effort_inc", quantize( effort_inc_, 0.00001 ) );
    print_param( os, "kick_rand", quantize( kick_rand_, 0.00001 ) );
    print_param( os, "team_actuator_noise", team_actuator_noise_ );
    print_param( os, "prand_factor_l", quantize( player_rand_factor_l_, 0.00001 ) );
    print_param( os, "prand_factor_r", quantize( player_rand_factor_r_, 0.00001 ) );
    print_param( os, "kick_rand_factor_l", quantize( kick_rand_factor_l_, 0.00001 ) );
    print_param( os, "kick_rand_factor_r", quantize( kick_rand_factor_r_, 0.00001 ) );
    print_param( os, "ball_size", quantize( ball_size_, 0.00001 ) );
    print_param( os, "ball_decay", quantize( ball_decay_, 0.00001 ) );
    print_param( os, "ball_rand", quantize( ball_rand_, 0.00001 ) );
    print_param( os, "ball_weight", quantize( ball_weight_, 0.00001 ) );
    print_param( os, "ball_speed_max", quantize( ball_speed_max_, 0.00001 ) );
    print_param( os, "ball_accel_max", quantize( ball_accel_max_, 0.00001 ) );
    print_param( os, "dash_power_rate", quantize( dash_power_rate_, 0.00001 ) );
    print_param( os, "kick_power_rate", quantize( kick_power_rate_, 0.00001 ) );
    print_param( os, "kickable_margin", quantize( kickable_margin_, 0.00001 ) );
    print_param( os, "control_radius", quantize( control_radius_, 0.00001 ) );
    //( "control_radius_width", control_radius_width_ );
    //( "kickable_area", kickable_area_ );
    print_param( os, "catch_probability", quantize( catch_probability_, 0.00001 ) );
    print_param( os, "catchable_area_l", quantize( catchable_area_l_, 0.00001 ) );
    print_param( os, "catchable_area_w", quantize( catchable_area_w_, 0.00001 ) );
    print_param( os, "goalie_max_moves", goalie_max_moves_ );
    print_param( os, "maxpower", quantize( max_power_, 0.00001 ) );
    print_param( os, "minpower", quantize( min_power_, 0.00001 ) );
    print_param( os, "maxmoment", quantize( max_moment_, 0.00001 ) );
    print_param( os, "minmoment", quantize( min_moment_, 0.00001 ) );
    print_param( os, "maxneckmoment", quantize( max_neck_moment_, 0.00001 ) );
    print_param( os, "minneckmoment", quantize( min_neck_moment_, 0.00001 ) );
    print_param( os, "maxneckang", quantize( max_neck_angle_, 0.00001 ) );
    print_param( os, "minneckang", quantize( min_neck_angle_, 0.00001 ) );
    print_param( os, "visible_angle", quantize( visible_angle_, 0.00001 ) );
    print_param( os, "visible_distance", quantize( visible_distance_, 0.00001 ) );
    print_param( os, "audio_cut_dist", quantize( audio_cut_dist_, 0.00001 ) );
    print_param( os, "quantize_step", quantize( quantize_step_, 0.00001 ) );
    print_param( os, "quantize_step_l", quantize( landmark_quantize_step_, 0.00001 ) );
    //( "quantize_step_dir", dir_quantize_step_ );
    //( "quantize_step_dist_team_l", dist_quantize_step_l_ );
    //( "quantize_step_dist_team_r", dist_quantize_step_r_ );
    //( "quantize_step_dist_l_team_l", landmark_dist_quantize_step_l_ );
    //( "quantize_step_dist_l_team_r", landmark_dist_quantize_step_r_ );
    //( "quantize_step_dir_team_l", dir_quantize_step_l_ );
    //( "quantize_step_dir_team_r", dir_quantize_step_r_ );
    print_param( os, "ckick_margin", quantize( corner_kick_margin_, 0.00001 ) );
    print_param( os, "wind_dir", quantize( wind_dir_, 0.00001 ) );
    print_param( os, "wind_force", quantize( wind_force_, 0.00001 ) );
    print_param( os, "wind_ang", quantize( wind_angle_, 0.00001 ) );
    print_param( os, "wind_rand", quantize( wind_rand_, 0.00001 ) );
    print_param( os, "wind_none", wind_none_ );
    print_param( os, "wind_random", wind_random_ );
    print_param( os, "half_time", half_time_ );
    print_param( os, "drop_ball_time", drop_ball_time_ );
    print_param( os, "port", port_ );
    print_param( os, "coach_port", coach_port_ );
    print_param( os, "olcoach_port", online_coach_port_ );
    print_param( os, "say_coach_cnt_max", say_coach_count_max_ );
    print_param( os, "say_coach_msg_size", say_coach_msg_size_ );
    print_param( os, "simulator_step", simulator_step_ );
    print_param( os, "send_step", send_step_ );
    print_param( os, "recv_step", recv_step_ );
    print_param( os, "sense_body_step", sense_body_step_ );
    //( "lcm_step", lcm_step_ ); // not needed
    print_param( os, "say_msg_size", say_msg_size_ );
    print_param( os, "clang_win_size", clang_win_size_ );
    print_param( os, "clang_define_win", clang_define_win_ );
    print_param( os, "clang_meta_win", clang_meta_win_ );
    print_param( os, "clang_advice_win", clang_advice_win_ );
    print_param( os, "clang_info_win", clang_info_win_ );
    print_param( os, "clang_del_win", clang_del_win_ );
    print_param( os, "clang_rule_win", clang_rule_win_ );
    print_param( os, "clang_mess_delay", clang_mess_delay_ );
    print_param( os, "clang_mess_per_cycle", clang_mess_per_cycle_ );
    print_param( os, "hear_max", hear_max_ );
    print_param( os, "hear_inc", hear_inc_ );
    print_param( os, "hear_decay", hear_decay_ );
    print_param( os, "catch_ban_cycle", catch_ban_cycle_ );
    print_param( os, "coach", coach_mode_ );
    print_param( os, "coach_w_referee", coach_with_referee_mode_ );
    print_param( os, "old_coach_hear", old_coach_hear_ );
    print_param( os, "send_vi_step", send_vi_step_ );
    print_param( os, "use_offside", use_offside_ );
    print_param( os, "offside_kick_margin", quantize( offside_kick_margin_, 0.00001 ) );
    print_param( os, "forbid_kick_off_offside", forbid_kick_off_offside_ );
    print_param( os, "verbose", verbose_ );
    print_param( os, "offside_active_area_size", quantize( offside_active_area_size_, 0.00001 ) );
    print_param( os, "slow_down_factor", slow_down_factor_ );
    print_param( os, "synch_mode", synch_mode_ );
    print_param( os, "synch_offset", synch_offset_ );
    print_param( os, "synch_micro_sleep", synch_micro_sleep_ );
    print_param( os, "start_goal_l", start_goal_l_ );
    print_param( os, "start_goal_r", start_goal_r_ );
    print_param( os, "fullstate_l", fullstate_l_ );
    print_param( os, "fullstate_r", fullstate_r_ );
    print_param( os, "slowness_on_top_for_left_team", quantize( slowness_on_top_for_left_team_, 0.00001 ) );
    print_param( os, "slowness_on_top_for_right_team", quantize( slowness_on_top_for_right_team_, 0.00001 ) );
    print_param( os, "landmark_file", landmark_file_ );
    print_param( os, "send_comms", send_comms_ );
    print_param( os, "text_logging", text_logging_ );
    print_param( os, "game_logging", game_logging_ );
    print_param( os, "game_log_version", game_log_version_ );
    print_param( os, "text_log_dir", text_log_dir_ );
    print_param( os, "game_log_dir", game_log_dir_ );
    print_param( os, "text_log_fixed_name", text_log_fixed_name_ );
    print_param( os, "game_log_fixed_name", game_log_fixed_name_ );
    print_param( os, "text_log_fixed", text_log_fixed_ );
    print_param( os, "game_log_fixed", game_log_fixed_ );
    print_param( os, "text_log_dated", text_log_dated_ );
    print_param( os, "game_log_dated", game_log_dated_ );
    print_param( os, "log_date_format", log_date_format_ );
    print_param( os, "log_times", log_times_ );
    print_param( os, "record_messages", record_messages_ );
    print_param( os, "text_log_compression", text_log_compression_ );
    print_param( os, "game_log_compression", game_log_compression_ );
    print_param( os, "profile", profile_ );
    print_param( os, "point_to_ban", point_to_ban_ );
    print_param( os, "point_to_duration", point_to_duration_ );
    print_param( os, "tackle_dist", quantize( tackle_dist_, 0.00001 ) );
    print_param( os, "tackle_back_dist", quantize( tackle_back_dist_, 0.00001 ) );
    print_param( os, "tackle_width", quantize( tackle_width_, 0.00001 ) );
    print_param( os, "tackle_exponent", quantize( tackle_exponent_, 0.00001 ) );
    print_param( os, "tackle_cycles", tackle_cycles_ );
    print_param( os, "tackle_power_rate", quantize( tackle_power_rate_, 0.00001 ) );
    print_param( os, "freeform_wait_period", freeform_wait_period_ );
    print_param( os, "freeform_send_period", freeform_send_period_ );
    print_param( os, "free_kick_faults", free_kick_faults_ );
    print_param( os, "back_passes", back_passes_ );
    print_param( os, "proper_goal_kicks", proper_goal_kicks_ );
    print_param( os, "stopped_ball_vel", quantize( stopped_ball_vel_, 0.00001 ) );
    print_param( os, "max_goal_kicks", max_goal_kicks_ );
    print_param( os, "auto_mode", auto_mode_ );
    print_param( os, "kick_off_wait", kick_off_wait_ );
    print_param( os, "connect_wait", connect_wait_ );
    print_param( os, "game_over_wait", game_over_wait_ );
    print_param( os, "team_l_start", team_l_start_ );
    print_param( os, "team_r_start", team_r_start_ );
    print_param( os, "keepaway", keepaway_mode_ );
    print_param( os, "keepaway_length", quantize( keepaway_length_, 0.00001 ) );
    print_param( os, "keepaway_width", quantize( keepaway_width_, 0.00001 ) );
    print_param( os, "keepaway_logging", keepaway_logging_ );
    print_param( os, "keepaway_log_dir", keepaway_log_dir_ );
    print_param( os, "keepaway_log_fixed_name", keepaway_log_fixed_name_ );
    print_param( os, "keepaway_log_fixed", keepaway_log_fixed_ );
    print_param( os, "keepaway_log_dated", keepaway_log_dated_ );
    print_param( os, "keepaway_start", keepaway_start_ );
    print_param( os, "nr_normal_halfs", nr_normal_halfs_ );
    print_param( os, "nr_extra_halfs", nr_extra_halfs_ );
    print_param( os, "penalty_shoot_outs", penalty_shoot_outs_ );
    print_param( os, "pen_before_setup_wait", pen_before_setup_wait_ );
    print_param( os, "pen_setup_wait", pen_setup_wait_ );
    print_param( os, "pen_ready_wait", pen_ready_wait_ );
    print_param( os, "pen_taken_wait", pen_taken_wait_ );
    print_param( os, "pen_nr_kicks", pen_nr_kicks_ );
    print_param( os, "pen_max_extra_kicks", pen_max_extra_kicks_ );
    print_param( os, "pen_dist_x", quantize( pen_dist_x_, 0.00001 ) );
    print_param( os, "pen_random_winner", pen_random_winner_ );
    print_param( os, "pen_max_goalie_dist_x", quantize( pen_max_goalie_dist_x_, 0.00001 ) );
    print_param( os, "pen_allow_mult_kicks", pen_allow_mult_kicks_ );
    print_param( os, "pen_coach_moves_players", pen_coach_moves_players_ );
    // 11
    print_param( os, "ball_stuck_area", quantize( ball_stuck_area_, 0.00001 ) );
    print_param( os, "coach_msg_file", coach_msg_file_ );
    // 12
    print_param( os, "max_tackle_power", quantize( max_tackle_power_, 0.00001 ) );
    print_param( os, "max_back_tackle_power", quantize( max_back_tackle_power_, 0.00001 ) );
    print_param( os, "player_speed_max_min", quantize( player_speed_max_min_, 0.00001 ) );
    print_param( os, "extra_stamina", quantize( extra_stamina_, 0.00001 ) );
    print_param( os, "synch_see_offset", synch_see_offset_ );
    print_param( os, "max_monitors", max_monitors_ );
    // 12.1.3
    print_param( os, "extra_half_time", extra_half_time_ );
    // v13
    print_param( os, "stamina_capacity", quantize( stamina_capacity_, 0.00001 ) );
    print_param( os, "max_dash_angle", quantize( max_dash_angle_, 0.00001 ) );
    print_param( os, "min_dash_angle", quantize( min_dash_angle_, 0.00001 ) );
    print_param( os, "dash_angle_step", quantize( dash_angle_step_, 0.00001 ) );
    print_param( os, "side_dash_rate", quantize( side_dash_rate_, 0.00001 ) );
    print_param( os, "back_dash_rate", quantize( back_dash_rate_, 0.00001 ) );
    print_param( os, "max_dash_power", quantize( max_dash_power_, 0.00001 ) );
    print_param( os, "min_dash_power", quantize( min_dash_power_, 0.00001 ) );
    // test
    print_param( os, "min_catch_probability", quantize( min_catch_probability_, 0.00001 ) );
    print_param( os, "reliable_catch_area_l", quantize( reliable_catch_area_l_, 0.00001 ) );
    os << ')';

    return os;
}

}
}
