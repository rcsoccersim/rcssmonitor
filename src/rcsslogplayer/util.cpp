// -*-c++-*-

/*!
  \file util.cpp
  \brief utilities for rcg data Source File.
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

#include "util.h"

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <rcsslogplayer/types.h>

#include <cmath>
#include <cstring>

namespace {
const float DEG2RADF = 3.14159265358979323846f / 180.0f;
const float RAD2DEGF = 180.0f / 3.14159265358979323846f;
}

namespace rcss {
namespace rcg {

/*-------------------------------------------------------------------*/
/*!

 */
int
nstohi( const Int16 val )
{
    return static_cast< int >( static_cast< Int16 >( ntohs( val ) ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
Int16
hitons( const int val )
{
    return htons( static_cast< Int16 >( val ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
nstohb( const Int16 val )
{
    return static_cast< bool >( static_cast< Int16 >( ntohs( val ) ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
Int16
hbtons( const bool val )
{
    return htons( val ? 1 : 0 );
}

/*-------------------------------------------------------------------*/
/*!

 */
double
nstohd( const Int16 val )
{
    return ( static_cast< double >( static_cast< Int16 >( ntohs( val ) ) )
             / SHOWINFO_SCALE );
}

/*-------------------------------------------------------------------*/
/*!

 */
float
nstohf( const Int16 val )
{
    return ( static_cast< float >( static_cast< Int16 >( ntohs( val ) ) )
             / SHOWINFO_SCALEF );
}

/*-------------------------------------------------------------------*/
/*!

 */
Int16
hdtons( const double & val )
{
    return htons( static_cast< Int16 >( rint( val * SHOWINFO_SCALE ) ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
Int16
hftons( const float & val )
{
    return htons( static_cast< Int16 >( rintf( val * SHOWINFO_SCALEF ) ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
double
nltohd( const Int32 & val )
{
    return ( static_cast< double >( static_cast< Int32 >( ntohl( val ) ) )
             / SHOWINFO_SCALE2 );
}

/*-------------------------------------------------------------------*/
/*!

 */
float
nltohf( const Int32 & val )
{
    return ( static_cast< float >( static_cast< Int32 >( ntohl( val ) ) )
             / SHOWINFO_SCALE2F );
}

/*-------------------------------------------------------------------*/
/*!

 */
Int32
hdtonl( const double & val )
{
    return htonl( static_cast< Int32 >( rint( val * SHOWINFO_SCALE2 ) ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
Int32
hftonl( const float & val )
{
    return htonl( static_cast< Int32 >( rintf( val * SHOWINFO_SCALE2F ) ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
Int32
nstonl( const Int16 val )
{
    return hdtonl( nstohd( val ) );
    //return ( static_cast< Int32 >
    //         ( htonl( static_cast< Int32 >( nstohd( val ) * SHOWINFO_SCALE2 ) ) ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
Int16
nltons( const Int32 & val )
{
    return hdtons( nltohd( val ) );
    //return ( static_cast< Int16 >
    //         ( htons( static_cast< Int16 >( nltohd( val ) * SHOWINFO_SCALE ) ) ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const pos_t & from,
         BallT & to )
{
    to.x_ = nstohf( from.x );
    to.y_ = nstohf( from.y );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const BallT & from,
         pos_t & to )
{
    to.side = htons( NEUTRAL );
    to.x = hftons( from.x_ );
    to.y = hftons( from.y_ );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const ball_t & from,
         BallT & to )
{
    to.x_ = nltohf( from.x );
    to.y_ = nltohf( from.y );
    to.vx_ = nltohf( from.deltax );
    to.vy_ = nltohf( from.deltay );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const BallT & from,
         ball_t & to )
{
    to.x = hftonl( from.x_ );
    to.y = hftonl( from.y_ );
    if ( from.hasVelocity() )
    {
        to.deltax = hftonl( from.vx_ );
        to.deltay = hftonl( from.vy_ );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const pos_t & from,
         PlayerT & to )
{
    Side side = static_cast< Side >( static_cast< Int16 >( ntohs( from.side ) ) );

    to.state_ = static_cast< Int32 >( ntohs( from.enable ) );
    to.side_ = ( side == LEFT ? 'l'
                 : side == RIGHT ? 'r'
                 : 'n' );
    to.unum_ = ntohs( from.unum );
    to.body_ = static_cast< float >( ntohs( from.angle ) );
    to.x_ = nstohf( from.x );
    to.y_ = nstohf( from.y );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const PlayerT & from,
         pos_t & to )
{
    to.enable = htons( static_cast< Int16 >( from.state_ ) );

    to.side = ( from.side_ == 'l' ? htons( static_cast< Int16 >( LEFT ) )
                : from.side_ == 'r' ? htons( static_cast< Int16 >( RIGHT ) )
                : htons( NEUTRAL ) );

    to.unum = ntohs( from.unum_ );
    to.angle = htons( static_cast< Int16 >( rintf( from.body_ ) ) );

    to.x = hftons( from.x_ );
    to.y = hftons( from.y_ );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const player_t & from,
         PlayerT & to )
{
    to.state_ = static_cast< Int32 >( ntohs( from.mode ) );
    to.type_ = ntohs( from.type );
    to.x_ = nltohf( from.x );
    to.y_ = nltohf( from.y );
    to.vx_ = nltohf( from.deltax );
    to.vy_ = nltohf( from.deltay );
    to.body_ = nltohf( from.body_angle ) * RAD2DEGF;
    to.neck_ = nltohf( from.head_angle ) * RAD2DEGF;
    to.view_width_ = nltohf( from.view_width ) * RAD2DEGF;
    to.view_quality_ = ntohs( from.view_quality ) ? 'h' : 'l';
    to.stamina_ = nltohf( from.stamina );
    to.effort_ = nltohf( from.effort );
    to.recovery_ = nltohf( from.recovery );
    to.kick_count_ = ntohs( from.kick_count );
    to.dash_count_ = ntohs( from.dash_count );
    to.turn_count_ = ntohs( from.turn_count );
    to.say_count_ = ntohs( from.say_count );
    to.turn_neck_count_ = ntohs( from.turn_neck_count );
    to.catch_count_ = ntohs( from.catch_count );
    to.move_count_ = ntohs( from.move_count );
    to.change_view_count_ = ntohs( from.change_view_count );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const PlayerT & from,
         player_t & to )
{
    to.mode = ntohs( static_cast< Int16 >( from.state_ ) );
    to.type = ntohs( from.type_ );
    to.x = hftonl( from.x_ );
    to.y = hftonl( from.y_ );
    if ( from.hasVelocity() )
    {
        to.deltax = hftonl( from.vx_ );
        to.deltay = hftonl( from.vy_ );
    }
    to.body_angle = hftonl( from.body_ * DEG2RADF );
    if ( from.hasNeck() )
    {
        to.head_angle = hftonl( from.neck_ * DEG2RADF );
    }
    if ( from.hasView() )
    {
        to.view_width = hftonl( from.view_width_ * DEG2RADF );
        to.view_quality = htons( from.highQuality() ? 1 : 0 );
    }
    if ( from.hasStamina() )
    {
        to.stamina = hftonl( from.stamina_ );
        to.effort = hftonl( from.effort_ );
        to.recovery = hftonl( from.recovery_ );
    }
    to.kick_count = htons( from.kick_count_ );
    to.dash_count = htons( from.dash_count_ );
    to.turn_count = htons( from.turn_count_ );
    to.say_count = htons( from.say_count_ );
    to.turn_neck_count = htons( from.turn_neck_count_ );
    to.catch_count = htons( from.catch_count_ );
    to.move_count = htons( from.move_count_ );
    to.change_view_count = htons( from.change_view_count_ );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const team_t & from,
         TeamT & to )
{
    char buf[18];
    std::memset( buf, 0, 18 );
    std::strncpy( buf, from.name, sizeof( from.name ) );

    to.name_ = buf;
    to.score_ = ntohs( from.score );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const TeamT & from,
         team_t & to )
{
    std::memset( to.name, 0, sizeof( to.name ) );
    std::strncpy( to.name,
                  from.name_.c_str(),
                  std::min( sizeof( to.name ), from.name_.length() ) );
    to.score = htons( from.score_ );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const showinfo_t & from,
         ShowInfoT & to )
{
    // ball
    convert( from.pos[0], to.ball_ );

    // players
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        convert( from.pos[i+1], to.player_[i] );
    }

    // time
    to.time_ = static_cast< UInt32 >( ntohs( from.time ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const char playmode,
         const TeamT & team_l,
         const TeamT & team_r,
         const ShowInfoT & from,
         showinfo_t & to )
{
    to.pmode = playmode;

    // team
    convert( team_l, to.team[0] );
    convert( team_r, to.team[1] );

    // ball
    convert( from.ball_, to.pos[0] );

    // players
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        int idx = from.player_[i].unum_;
        if ( from.player_[i].side() == NEUTRAL ) continue;
        if ( from.player_[i].side() == RIGHT ) idx += MAX_PLAYER;
        if ( idx < 0 || MAX_PLAYER*2 + 1 <= idx ) continue;

        convert( from.player_[i], to.pos[idx] );
    }

    // time
    to.time = htons( static_cast< Int16 >( from.time_ ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const char playmode,
         const TeamT & team_l,
         const TeamT & team_r,
         const ShowInfoT & from,
         showinfo_t2 & to )
{
    to.pmode = playmode;

    // team
    convert( team_l, to.team[0] );
    convert( team_r, to.team[1] );

    // ball
    convert( from.ball_, to.ball );

    // players
    for ( int i = 0; i < MAX_PLAYER * 2; ++i )
    {
        int idx = from.player_[i].unum_ - 1;
        if ( from.player_[i].side() == NEUTRAL ) continue;
        if ( from.player_[i].side() == RIGHT ) idx += MAX_PLAYER;
        if ( idx < 0 || MAX_PLAYER*2 <= idx ) continue;

        convert( from.player_[i], to.pos[idx] );
    }

    // time
    to.time = htons( static_cast< Int16 >( from.time_ ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const showinfo_t2 & from,
         ShowInfoT & to )
{
    // ball
    convert( from.ball, to.ball_ );

    // players
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        to.player_[i].side_ = ( i < MAX_PLAYER ? 'l' : 'r' );
        to.player_[i].unum_ = ( i < MAX_PLAYER ? i + 1 : i + 1 - MAX_PLAYER );
        convert( from.pos[i], to.player_[i] );
    }

    // time
    to.time_ = static_cast< UInt32 >( ntohs( from.time ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const short_showinfo_t2 & from,
         ShowInfoT & to )
{
    // ball
    convert( from.ball, to.ball_ );

    // players
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        to.player_[i].side_ = ( i < MAX_PLAYER ? 'l' : 'r' );
        to.player_[i].unum_ = ( i < MAX_PLAYER ? i + 1 : i + 1 - MAX_PLAYER );
        convert( from.pos[i], to.player_[i] );
    }

    // time
    to.time_ = static_cast< UInt32 >( ntohs( from.time ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const ShowInfoT & from,
         short_showinfo_t2 & to )
{
    // ball
    convert( from.ball_, to.ball );

    // players
    for ( int i = 0; i < MAX_PLAYER * 2; ++i )
    {
        int idx = from.player_[i].unum_ - 1;
        if ( from.player_[i].side() == NEUTRAL ) continue;
        if ( from.player_[i].side() == RIGHT ) idx += MAX_PLAYER;
        if ( idx < 0 || MAX_PLAYER*2 <= idx ) continue;

        convert( from.player_[i], to.pos[idx] );
    }

    // time
    to.time = htons( static_cast< Int16 >( from.time_ ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const player_type_t & from,
         PlayerTypeT & to )
{
    to.id_ = nstohi( from.id );
    to.player_speed_max_ = nltohd( from.player_speed_max );
    to.stamina_inc_max_ = nltohd( from.stamina_inc_max );
    to.player_decay_ = nltohd( from.player_decay );
    to.inertia_moment_ = nltohd( from.inertia_moment );
    to.dash_power_rate_ = nltohd( from.dash_power_rate );
    to.player_size_ = nltohd( from.player_size );
    to.kickable_margin_ = nltohd( from.kickable_margin );
    to.kick_rand_ = nltohd( from.kick_rand );
    to.extra_stamina_ = nltohd( from.extra_stamina );
    to.effort_max_ = nltohd( from.effort_max );
    to.effort_min_ = nltohd( from.effort_min );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const PlayerTypeT & from,
         player_type_t & to )
{
    to.id = hitons( from.id_ );
    to.player_speed_max = hftonl( from.player_speed_max_ );
    to.stamina_inc_max = hftonl( from.stamina_inc_max_ );
    to.player_decay = hftonl( from.player_decay_ );
    to.inertia_moment = hftonl( from.inertia_moment_ );
    to.dash_power_rate = hftonl( from.dash_power_rate_ );
    to.player_size = hftonl( from.player_size_ );
    to.kickable_margin = hftonl( from.kickable_margin_ );
    to.kick_rand = hftonl( from.kick_rand_ );
    to.extra_stamina = hftonl( from.extra_stamina_ );
    to.effort_max = hftonl( from.effort_max_ );
    to.effort_min = hftonl( from.effort_min_ );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const player_params_t & from,
         PlayerParamT & to )
{
    to.player_types_ = nstohi( from.player_types );
    to.subs_max_ = nstohi( from.substitute_max );
    to.pt_max_ = nstohi( from.pt_max );

    to.player_speed_max_delta_min_ = nltohd( from.player_speed_max_delta_min );
    to.player_speed_max_delta_max_ = nltohd( from.player_speed_max_delta_max );
    to.stamina_inc_max_delta_factor_ = nltohd( from.stamina_inc_max_delta_factor );

    to.player_decay_delta_min_ = nltohd( from.player_decay_delta_min );
    to.player_decay_delta_max_ = nltohd( from.player_decay_delta_max );
    to.inertia_moment_delta_factor_ = nltohd( from.inertia_moment_delta_factor );

    to.dash_power_rate_delta_min_ = nltohd( from.dash_power_rate_delta_min );
    to.dash_power_rate_delta_max_ = nltohd( from.dash_power_rate_delta_max );
    to.player_size_delta_factor_ = nltohd( from.player_size_delta_factor );

    to.kickable_margin_delta_min_ = nltohd( from.kickable_margin_delta_min );
    to.kickable_margin_delta_max_ = nltohd( from.kickable_margin_delta_max );
    to.kick_rand_delta_factor_ = nltohd( from.kick_rand_delta_factor );

    to.extra_stamina_delta_min_ = nltohd( from.extra_stamina_delta_min );
    to.extra_stamina_delta_max_ = nltohd( from.extra_stamina_delta_max );
    to.effort_max_delta_factor_ = nltohd( from.effort_max_delta_factor );
    to.effort_min_delta_factor_ = nltohd( from.effort_min_delta_factor );

    to.random_seed_ = static_cast< int >( static_cast< Int32 >( ntohl( from.random_seed ) ) );

    to.new_dash_power_rate_delta_min_ = nltohd( from.new_dash_power_rate_delta_min );
    to.new_dash_power_rate_delta_max_ = nltohd( from.new_dash_power_rate_delta_max );
    to.new_stamina_inc_max_delta_factor_ = nltohd( from.new_stamina_inc_max_delta_factor );

    to.allow_mult_default_type_ = static_cast< bool >( ntohs( from.allow_mult_default_type ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const PlayerParamT & from,
         player_params_t & to )
{
    to.player_types = hitons( from.player_types_ );
    to.substitute_max = hitons( from.subs_max_ );
    to.pt_max = hitons( from.pt_max_ );

    to.player_speed_max_delta_min = hdtonl( from.player_speed_max_delta_min_ );
    to.player_speed_max_delta_max = hdtonl( from.player_speed_max_delta_max_ );
    to.stamina_inc_max_delta_factor = hdtonl( from.stamina_inc_max_delta_factor_ );

    to.player_decay_delta_min = hdtonl( from.player_decay_delta_min_ );
    to.player_decay_delta_max = hdtonl( from.player_decay_delta_max_ );
    to.inertia_moment_delta_factor = hdtonl( from.inertia_moment_delta_factor_ );

    to.dash_power_rate_delta_min = hdtonl( from.dash_power_rate_delta_min_ );
    to.dash_power_rate_delta_max = hdtonl( from.dash_power_rate_delta_max_ );
    to.player_size_delta_factor = hdtonl( from.player_size_delta_factor_ );

    to.kickable_margin_delta_min = hdtonl( from.kickable_margin_delta_min_ );
    to.kickable_margin_delta_max = hdtonl( from.kickable_margin_delta_max_ );
    to.kick_rand_delta_factor = hdtonl( from.kick_rand_delta_factor_ );

    to.extra_stamina_delta_min = hdtonl( from.extra_stamina_delta_min_ );
    to.extra_stamina_delta_max = hdtonl( from.extra_stamina_delta_max_ );
    to.effort_max_delta_factor = hdtonl( from.effort_max_delta_factor_ );
    to.effort_min_delta_factor = hdtonl( from.effort_min_delta_factor_ );

    to.random_seed = htonl( static_cast< Int32 >( from.random_seed_ ) );

    to.new_dash_power_rate_delta_min = hdtonl( from.new_dash_power_rate_delta_min_ );
    to.new_dash_power_rate_delta_max = hdtonl( from.new_dash_power_rate_delta_max_ );
    to.new_stamina_inc_max_delta_factor = hdtonl( from.new_stamina_inc_max_delta_factor_ );

    to.allow_mult_default_type = hbtons( from.allow_mult_default_type_ );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const server_params_t & from,
         ServerParamT & to )
{
    double tmp = 0.0;

    to.goal_width_ = nltohd( from.goal_width );
    to.inertia_moment_ = nltohd( from.inertia_moment );

    to.player_size_ = nltohd( from.player_size );
    to.player_decay_ = nltohd( from.player_decay );
    to.player_rand_ = nltohd( from.player_rand );
    to.player_weight_ = nltohd( from.player_weight );
    to.player_speed_max_ = nltohd( from.player_speed_max );
    to.player_accel_max_ = nltohd( from.player_accel_max );

    to.stamina_max_ = nltohd( from.stamina_max );
    to.stamina_inc_max_ = nltohd( from.stamina_inc );

    to.recover_init_ = nltohd( from.recover_init );
    to.recover_dec_thr_ = nltohd( from.recover_dec_thr );
    to.recover_min_ = nltohd( from.recover_min );
    to.recover_dec_ = nltohd( from.recover_dec );

    to.effort_init_ = nltohd( from.effort_init );
    to.effort_dec_thr_ = nltohd( from.effort_dec_thr );
    to.effort_min_ = nltohd( from.effort_min );
    to.effort_dec_ = nltohd( from.effort_dec );
    to.effort_inc_thr_ = nltohd( from.effort_inc_thr );
    to.effort_inc_ = nltohd( from.effort_inc );

    to.kick_rand_ = nltohd( from.kick_rand );
    to.team_actuator_noise_ = nstohb( from.team_actuator_noise );
    to.player_rand_factor_l_ = nltohd( from.player_rand_factor_l );
    to.player_rand_factor_r_ = nltohd( from.player_rand_factor_r );
    to.kick_rand_factor_l_ = nltohd( from.kick_rand_factor_l );
    to.kick_rand_factor_r_ = nltohd( from.kick_rand_factor_r );

    to.ball_size_ = nltohd( from.ball_size );
    to.ball_decay_ = nltohd( from.ball_decay );
    to.ball_rand_ = nltohd( from.ball_rand );
    to.ball_weight_ = nltohd( from.ball_weight );
    to.ball_speed_max_ = nltohd( from.ball_speed_max );
    to.ball_accel_max_ = nltohd( from.ball_accel_max );

    to.dash_power_rate_ = nltohd( from.dash_power_rate );
    to.kick_power_rate_ = nltohd( from.kick_power_rate );
    to.kickable_margin_ = nltohd( from.kickable_margin );
    to.control_radius_ = nltohd( from.control_radius );
    to.control_radius_width_ = nltohd( from.control_radius_width );

    to.max_power_ = nltohd( from.max_power );
    to.min_power_ = nltohd( from.min_power );
    to.max_moment_ = nltohd( from.max_moment );
    to.min_moment_ = nltohd( from.min_moment );
    to.max_neck_moment_ = nltohd( from.max_neck_moment );
    to.min_neck_moment_ = nltohd( from.min_neck_moment );
    to.max_neck_angle_ = nltohd( from.max_neck_angle );
    to.min_neck_angle_ = nltohd( from.min_neck_angle );

    to.visible_angle_ = nltohd( from.visible_angle );
    to.visible_distance_ = nltohd( from.visible_distance );

    to.wind_dir_ = nltohd( from.wind_dir );
    to.wind_force_ = nltohd( from.wind_force );
    to.wind_angle_ = nltohd( from.wind_angle );
    to.wind_rand_ = nltohd( from.wind_rand );

    //to.kickable_area_ = nltohd( from.kickable_area );

    to.catchable_area_l_ = nltohd( from.catchable_area_l );
    to.catchable_area_w_ = nltohd( from.catchable_area_w );
    to.catch_probability_ = nltohd( from.catch_probability );
    to.goalie_max_moves_ = nstohi( from.goalie_max_moves );

    to.corner_kick_margin_ = nltohd( from.corner_kick_margin );
    to.offside_active_area_size_ = nltohd( from.offside_active_area_size );

    to.wind_none_ = nstohb( from.wind_none );
    to.wind_random_ = nstohb( from.wind_random );

    to.say_coach_count_max_ = nstohi( from.say_coach_count_max );
    to.say_coach_msg_size_ = nstohi( from.say_coach_msg_size );

    to.clang_win_size_ = nstohi( from.clang_win_size );
    to.clang_define_win_ = nstohi( from.clang_define_win );
    to.clang_meta_win_ = nstohi( from.clang_meta_win );
    to.clang_advice_win_ = nstohi( from.clang_advice_win );
    to.clang_info_win_ = nstohi( from.clang_info_win );
    to.clang_mess_delay_ = nstohi( from.clang_mess_delay );
    to.clang_mess_per_cycle_ = nstohi( from.clang_mess_per_cycle );

    to.half_time_ = nstohi( from.half_time );
    to.simulator_step_ = nstohi( from.simulator_step );
    to.send_step_ = nstohi( from.send_step );
    to.recv_step_ = nstohi( from.recv_step );
    to.sense_body_step_ = nstohi( from.sense_body_step );
    //to.lcm_step_ = nstohi( from.lcm_step );

    to.say_msg_size_ = nstohi( from.say_msg_size );
    to.hear_max_ = nstohi( from.hear_max );
    to.hear_inc_ = nstohi( from.hear_inc );
    to.hear_decay_ = nstohi( from.hear_decay );

    to.catch_ban_cycle_ = nstohi( from.catch_ban_cycle );

    to.slow_down_factor_ = nstohi( from.slow_down_factor );

    to.use_offside_ = nstohb( from.use_offside );
    to.forbid_kick_off_offside_ = nstohb( from.forbid_kick_off_offside );
    to.offside_kick_margin_ = nltohd( from.offside_kick_margin );

    to.audio_cut_dist_ = nltohd(from.audio_cut_dist );

    to.quantize_step_ = nltohd( from.quantize_step );
    to.landmark_quantize_step_ = nltohd( from.landmark_quantize_step );
    //to.dir_quantize_step_ = nltohd( from.dir_quantize_step );
    //to.dist_quantize_step_l_ = nltohd( from.dist_quantize_step_l );
    //to.dist_quantize_step_r_ = nltohd( from.dist_quantize_step_r );
    //to.landmark_dist_quantize_step_l_ = nltohd( from.landmark_dist_quantize_step_l );
    //to.landmark_dist_quantize_step_r_ = nltohd( from.landmark_dist_quantize_step_r );
    //to.dir_quantize_step_l_ = nltohd( from.dir_quantize_step_l );
    //to.dir_quantize_step_r_ = nltohd( from.dir_quantize_step_r );

    to.coach_mode_ = nstohb( from.coach_mode );
    to.coach_with_referee_mode_ = nstohb( from.coach_with_referee_mode );
    to.old_coach_hear_ = nstohb( from.old_coach_hear );

    to.send_vi_step_ = nstohi( from.send_vi_step );

    to.slowness_on_top_for_left_team_ = nltohd( from.slowness_on_top_for_left_team );
    to.slowness_on_top_for_right_team_ = nltohd( from.slowness_on_top_for_right_team );

    to.keepaway_length_ = nltohd( from.keepaway_length );
    to.keepaway_width_ = nltohd( from.keepaway_width );

    // 11.0.0
    to.ball_stuck_area_ = 3.0;
    tmp = nltohd( from.ball_stuck_area );
    if ( std::fabs( tmp ) < 1000.0 ) to.ball_stuck_area_ = tmp;

    // 12.0.0
    to.max_tackle_power_ = to.max_power_;
    tmp = nltohd( from.max_tackle_power );
    if ( 0.0 < tmp && std::fabs( tmp ) < 200.0 ) to.max_tackle_power_ = tmp;

    to.max_back_tackle_power_ = to.max_power_;
    tmp = nltohd( from.max_back_tackle_power );
    if ( 0.0 < tmp && std::fabs( tmp ) < 200.0 ) to.max_back_tackle_power_ = tmp;

    to.tackle_dist_ = 2.0;
    tmp = nltohd( from.tackle_dist );
    if ( 0.0 <= tmp && std::fabs( tmp ) < 3.0 ) to.tackle_dist_ = tmp;

    to.tackle_back_dist_ = 0.5;
    tmp = nltohd( from.tackle_back_dist );
    if ( 0.0 <= tmp && std::fabs( tmp ) < 1.0 ) to.tackle_back_dist_ = tmp;

    to.tackle_width_ = 1.0;
    tmp = nltohd( from.tackle_width );
    if ( 0.0 < tmp && std::fabs( tmp ) < 2.0 ) to.tackle_width_ = tmp;

    //
    to.start_goal_l_ = nstohi( from.start_goal_l );
    to.start_goal_r_ = nstohi( from.start_goal_r );

    to.fullstate_l_ = nstohb( from.fullstate_l );
    to.fullstate_r_ = nstohb( from.fullstate_r );

    to.drop_ball_time_ = nstohi( from.drop_ball_time );

    to.synch_mode_ = nstohb( from.synch_mode );
    to.synch_offset_ = nstohi( from.synch_offset );
    to.synch_micro_sleep_ = nstohi( from.synch_micro_sleep );

    to.point_to_ban_ = nstohi( from.point_to_ban );
    to.point_to_duration_ = nstohi( from.point_to_duration );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const ServerParamT & from,
         server_params_t & to )
{
    to.goal_width = hdtonl( from.goal_width_ );
    to.inertia_moment = hdtonl( from.inertia_moment_ );

    to.player_size = hdtonl( from.player_size_ );
    to.player_decay = hdtonl( from.player_decay_ );
    to.player_rand = hdtonl( from.player_rand_ );
    to.player_weight = hdtonl( from.player_weight_ );
    to.player_speed_max = hdtonl( from.player_speed_max_ );
    to.player_accel_max = hdtonl( from.player_accel_max_ );

    to.stamina_max = hdtonl( from.stamina_max_ );
    to.stamina_inc = hdtonl( from.stamina_inc_max_ );

    to.recover_init = hdtonl( from.recover_init_ );
    to.recover_dec_thr = hdtonl( from.recover_dec_thr_ );
    to.recover_min = hdtonl( from.recover_min_ );
    to.recover_dec = hdtonl( from.recover_dec_ );

    to.effort_init = hdtonl( from.effort_init_ );
    to.effort_dec_thr = hdtonl( from.effort_dec_thr_ );
    to.effort_min = hdtonl( from.effort_min_ );
    to.effort_dec = hdtonl( from.effort_dec_ );
    to.effort_inc_thr = hdtonl( from.effort_inc_thr_ );
    to.effort_inc = hdtonl( from.effort_inc_ );

    to.kick_rand = hdtonl( from.kick_rand_ );
    to.team_actuator_noise = hbtons( from.team_actuator_noise_ );
    to.player_rand_factor_l = hdtonl( from.player_rand_factor_l_ );
    to.player_rand_factor_r = hdtonl( from.player_rand_factor_r_ );
    to.kick_rand_factor_l = hdtonl( from.kick_rand_factor_l_ );
    to.kick_rand_factor_r = hdtonl( from.kick_rand_factor_r_ );

    to.ball_size = hdtonl( from.ball_size_ );
    to.ball_decay = hdtonl( from.ball_decay_ );
    to.ball_rand = hdtonl( from.ball_rand_ );
    to.ball_weight = hdtonl( from.ball_weight_ );
    to.ball_speed_max = hdtonl( from.ball_speed_max_ );
    to.ball_accel_max = hdtonl( from.ball_accel_max_ );

    to.dash_power_rate = hdtonl( from.dash_power_rate_ );
    to.kick_power_rate = hdtonl( from.kick_power_rate_ );
    to.kickable_margin = hdtonl( from.kickable_margin_ );
    to.control_radius = hdtonl( from.control_radius_ );
    to.control_radius_width = hdtonl( from.control_radius_width_ );

    to.max_power = hdtonl( from.max_power_ );
    to.min_power = hdtonl( from.min_power_ );
    to.max_moment = hdtonl( from.max_moment_ );
    to.min_moment = hdtonl( from.min_moment_ );
    to.max_neck_moment = hdtonl( from.max_neck_moment_ );
    to.min_neck_moment = hdtonl( from.min_neck_moment_ );
    to.max_neck_angle = hdtonl( from.max_neck_angle_ );
    to.min_neck_angle = hdtonl( from.min_neck_angle_ );

    to.visible_angle = hdtonl( from.visible_angle_ );
    to.visible_distance = hdtonl( from.visible_distance_ );

    to.wind_dir = hdtonl( from.wind_dir_ );
    to.wind_force = hdtonl( from.wind_force_ );
    to.wind_angle = hdtonl( from.wind_angle_ );
    to.wind_rand = hdtonl( from.wind_rand_ );

    to.kickable_area = hdtonl( from.player_size_
                               + from.kickable_margin_
                               + from.ball_size_ );

    to.catchable_area_l = hdtonl( from.catchable_area_l_ );
    to.catchable_area_w = hdtonl( from.catchable_area_w_ );
    to.catch_probability = hdtonl( from.catch_probability_ );
    to.goalie_max_moves = hitons( from.goalie_max_moves_ );

    to.corner_kick_margin = hdtonl( from.corner_kick_margin_ );
    to.offside_active_area_size = hdtonl( from.offside_active_area_size_ );

    to.wind_none = hbtons( from.wind_none_ );
    to.wind_random = hbtons( from.wind_random_ );

    to.say_coach_count_max = hitons( from.say_coach_count_max_ );
    to.say_coach_msg_size = hitons( from.say_coach_msg_size_ );

    to.clang_win_size = hitons( from.clang_win_size_ );
    to.clang_define_win = hitons( from.clang_define_win_ );
    to.clang_meta_win = hitons( from.clang_meta_win_ );
    to.clang_advice_win = hitons( from.clang_advice_win_ );
    to.clang_info_win = hitons( from.clang_info_win_ );
    to.clang_mess_delay = hitons( from.clang_mess_delay_ );
    to.clang_mess_per_cycle = hitons( from.clang_mess_per_cycle_ );

    to.half_time = hitons( from.half_time_ );
    to.simulator_step = hitons( from.simulator_step_ );
    to.send_step = hitons( from.send_step_ );
    to.recv_step = hitons( from.recv_step_ );
    to.sense_body_step = hitons( from.sense_body_step_ );

    to.lcm_step = hitons( 300 );

    to.say_msg_size = hitons( from.say_msg_size_ );
    to.hear_max = hitons( from.hear_max_ );
    to.hear_inc = hitons( from.hear_inc_ );
    to.hear_decay = hitons( from.hear_decay_ );

    to.catch_ban_cycle = hitons( from.catch_ban_cycle_ );

    to.slow_down_factor = hitons( from.slow_down_factor_ );

    to.use_offside = hbtons( from.use_offside_ );
    to.forbid_kick_off_offside = hbtons( from.forbid_kick_off_offside_ );
    to.offside_kick_margin = hdtonl( from.offside_kick_margin_ );

    to.audio_cut_dist = hdtonl(from.audio_cut_dist_ );

    to.quantize_step = hdtonl( from.quantize_step_ );
    to.landmark_quantize_step = hdtonl( from.landmark_quantize_step_ );

    to.dir_quantize_step = htonl( 0 );
    to.dist_quantize_step_l = htonl( 0 );
    to.dist_quantize_step_r = htonl( 0 );
    to.landmark_dist_quantize_step_l = htonl( 0 );
    to.landmark_dist_quantize_step_r = htonl( 0 );
    to.dir_quantize_step_l = htonl( 0 );
    to.dir_quantize_step_r = htonl( 0 );

    to.coach_mode = hbtons( from.coach_mode_ );
    to.coach_with_referee_mode = hbtons( from.coach_with_referee_mode_ );
    to.old_coach_hear = hbtons( from.old_coach_hear_ );

    to.send_vi_step = hitons( from.send_vi_step_ );

    to.slowness_on_top_for_left_team = hdtonl( from.slowness_on_top_for_left_team_ );
    to.slowness_on_top_for_right_team = hdtonl( from.slowness_on_top_for_right_team_ );

    to.keepaway_length = hdtonl( from.keepaway_length_ );
    to.keepaway_width = hdtonl( from.keepaway_width_ );

    // 11.0.0
    to.ball_stuck_area = hdtonl( from.ball_stuck_area_ );

    // 12.0.0
    to.max_tackle_power = hdtonl( from.max_tackle_power_ );
    to.max_back_tackle_power = hdtonl( from.max_back_tackle_power_ );
    to.tackle_dist = hdtonl( from.tackle_dist_ );
    to.tackle_back_dist = hdtonl( from.tackle_back_dist_ );
    to.tackle_width = hdtonl( from.tackle_width_ );

    //
    to.start_goal_l = hitons( from.start_goal_l_ );
    to.start_goal_r = hitons( from.start_goal_r_ );

    to.fullstate_l = hbtons( from.fullstate_l_ );
    to.fullstate_r = hbtons( from.fullstate_r_ );

    to.drop_ball_time = hitons( from.drop_ball_time_ );

    to.synch_mode = hbtons( from.synch_mode_ );
    to.synch_offset = hitons( from.synch_offset_ );
    to.synch_micro_sleep = hitons( from.synch_micro_sleep_ );

    to.point_to_ban = hitons( from.point_to_ban_ );
    to.point_to_duration = hitons( from.point_to_duration_ );
}

} // end namespace
} // end namespace
