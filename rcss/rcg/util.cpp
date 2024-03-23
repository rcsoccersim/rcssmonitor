// -*-c++-*-

/*!
  \file util.cpp
  \brief utilities for rcg data Source File.
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

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

#include <unordered_map>
#include <string>
#include <sstream>
#include <cstring>
#include <cmath>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#ifndef M_PI
//! pi value
#define M_PI 3.14159265358979323846
#endif

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
    to.deltax = hftonl( from.vx_ );
    to.deltay = hftonl( from.vy_ );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const pos_t & from,
         player_t & to )
{
    to.mode = from.enable;
    to.type = 0;
    to.x = nstonl( from.x );
    to.y = nstonl( from.y );
    to.deltax = 0;
    to.deltay = 0;
    to.body_angle = nstonl( from.angle );
    to.head_angle = 0;
    to.view_width = 0;
    to.stamina = 0; //hdtonl( 4000.0 );
    to.effort = 0; //hdtonl( 1.0 );
    to.recovery = 0; //hdtonl( 1.0 );
    to.kick_count = 0;
    to.dash_count = 0;
    to.turn_count = 0;
    to.say_count = 0;
    to.turn_neck_count = 0;
    to.catch_count = 0;
    to.move_count = 0;
    to.change_view_count = 0;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const Side side,
         const int unum,
         const player_t & from,
         pos_t & to )
{
    std::memset( &to, 0, sizeof( pos_t ) );

    to.enable = from.mode;
    to.side = htons( static_cast< Int16 >( side ) );
    to.unum = htons( static_cast< Int16 >( unum ) );
    to.angle = htons( static_cast< Int16 >( rint( nltohd( from.body_angle ) * 180.0 / M_PI ) ) );
    to.x = hdtons( nltohd( from.x ) );
    to.y = hdtons( nltohd( from.y ) );
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
         player_t & to )
{
    std::memset( &to, 0, sizeof( player_t ) );

    to.mode = ntohs( static_cast< Int16 >( from.state_ ) );

    if ( from.hasType() )
    {
        to.type = ntohs( from.type_ );
    }

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
    }

    to.view_quality = htons( from.highQuality() ? 1 : 0 );

    if ( from.hasStamina() )
    {
        to.stamina = hftonl( from.stamina_ );
        to.effort = hftonl( from.effort_ );
        to.recovery = hftonl( from.recovery_ );
    }

    if ( from.hasCommandCount() )
    {
        to.kick_count = htons( from.kick_count_ );
        to.dash_count = htons( from.dash_count_ );
        to.turn_count = htons( from.turn_count_ );
        to.say_count = htons( from.say_count_ );
        to.turn_neck_count = htons( from.turn_neck_count_ );
        to.catch_count = htons( from.catch_count_ );
        to.move_count = htons( from.move_count_ );
        to.change_view_count = htons( from.change_view_count_ );
    }
}


/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const Side side,
         const int unum,
         const player_t & from,
         PlayerT & to )
{
    to.side_ = ( side == LEFT ? 'l'
                 : side == RIGHT ? 'r'
                 : 'n' );
    to.unum_ = static_cast< Int16 >( unum );
    to.type_ = ntohs( from.type );

    to.view_quality_ = ntohs( from.view_quality ) ? 'h' : 'l';

    to.state_ = static_cast< Int32 >( ntohs( from.mode ) );

    to.x_ = nltohf( from.x );
    to.y_ = nltohf( from.y );
    to.vx_ = nltohf( from.deltax );
    to.vy_ = nltohf( from.deltay );
    to.body_ = nltohf( from.body_angle ) * RAD2DEGF;
    to.neck_ = nltohf( from.head_angle ) * RAD2DEGF;

    if ( from.view_width != 0 )
    {
        to.view_width_ = nltohf( from.view_width ) * RAD2DEGF;
    }

    if ( from.stamina != 0
         && from.effort != 0
         && from.recovery != 0 )
    {
        to.stamina_ = nltohf( from.stamina );
        to.effort_ = nltohf( from.effort );
        to.recovery_ = nltohf( from.recovery );
    }

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
convert( const std::string & name,
         const int score,
         team_t & to )
{
    std::memset( to.name, 0, sizeof( to.name ) );
    std::strncpy( to.name, name.c_str(), sizeof( to.name ) - 1 );
    to.score = hitons( score );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const TeamT & from,
         team_t & to )
{
    std::memset( to.name, 0, sizeof( to.name ) );
    std::strncpy( to.name, from.name_.c_str(), sizeof( to.name ) - 1 );
    to.score = htons( from.score_ );
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
    std::strncpy( buf, from.name, sizeof( buf ) - 1 );

    to.name_ = buf;
    to.score_ = ntohs( from.score );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const showinfo_t & from,
         showinfo_t2 & to )
{
    // playmode
    to.pmode = from.pmode;

    // team

    // ball
    to.ball.x = nstonl( from.pos[0].x );
    to.ball.y = nstonl( from.pos[0].y );
    to.ball.deltax = 0;
    to.ball.deltay = 0;

    // players
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        convert( from.pos[i+1], to.pos[i] );
    }

    // time
    to.time = from.time;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const showinfo_t & from,
         short_showinfo_t2 & to )
{
    // ball
    to.ball.x = nstonl( from.pos[0].x );
    to.ball.y = nstonl( from.pos[0].y );
    to.ball.deltax = 0;
    to.ball.deltay = 0;

    // players
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        convert( from.pos[i+1], to.pos[i] );
    }

    // time
    to.time = from.time;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const showinfo_t2 & from,
         showinfo_t & to )
{
    // playmode
    to.pmode = from.pmode;

    // team
    for ( int i = 0; i < 2; ++i )
    {
        std::strncpy( to.team[i].name, from.team[i].name, 16 );
        to.team[i].score = from.team[i].score;
    }

    // ball
    to.pos[0].side = htons( NEUTRAL );
    to.pos[0].x = hdtons( nltohd( from.ball.x ) );
    to.pos[0].y = hdtons( nltohd( from.ball.y ) );

    // left team
    int unum = 1;
    for ( int i = 0; i < MAX_PLAYER; ++i, ++unum )
    {
        convert( LEFT, unum, from.pos[i], to.pos[i+1] );
    }

    // right team
    unum = 1;
    for ( int i = MAX_PLAYER; i < MAX_PLAYER *2; ++i, ++unum )
    {
        convert( RIGHT, unum, from.pos[i], to.pos[i+1] );
    }

    // time
    to.time = from.time;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const char playmode,
         const TeamT & team_l,
         const TeamT & team_r,
         const short_showinfo_t2 & from,
         showinfo_t & to )
{
    // playmode
    to.pmode = playmode;

    // team
    convert( team_l, to.team[0] );
    convert( team_r, to.team[1] );

    // ball
    to.pos[0].side = htons( NEUTRAL );
    to.pos[0].x = hdtons( nltohd( from.ball.x ) );
    to.pos[0].y = hdtons( nltohd( from.ball.y ) );

    // left team
    int unum = 1;
    for ( int i = 0; i < MAX_PLAYER; ++i, ++unum )
    {
        convert( LEFT, unum, from.pos[i], to.pos[i+1] );
    }

    // right team
    unum = 1;
    for ( int i = MAX_PLAYER; i < MAX_PLAYER *2; ++i, ++unum )
    {
        convert( RIGHT, unum, from.pos[i], to.pos[i+1] );
    }

    // time
    to.time = from.time;
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
    to.pos[0].side = htons( NEUTRAL );
    to.pos[0].x = hftons( from.ball_.x_ );
    to.pos[0].y = hftons( from.ball_.y_ );

    // players
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        int idx = from.player_[i].unum_;
        if ( from.player_[i].side() == NEUTRAL ) continue;
        if ( from.player_[i].side() == RIGHT ) idx += MAX_PLAYER;
        if ( idx < 0 || MAX_PLAYER*2 + 1 <= idx ) continue;

        to.pos[idx].enable = htons( static_cast< Int16 >( from.player_[i].state_ ) );
        to.pos[idx].side = htons( from.player_[i].side() );
        to.pos[idx].unum = htons( from.player_[i].unum_ );
        to.pos[idx].angle = htons( static_cast< Int16 >( rintf( from.player_[i].body_ ) ) );
        to.pos[idx].x = hftons( from.player_[i].x_ );
        to.pos[idx].y = hftons( from.player_[i].y_ );
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
    to.ball.x = hftonl( from.ball_.x_ );
    to.ball.y = hftonl( from.ball_.y_ );
    to.ball.deltax = hftonl( from.ball_.vx_ );
    to.ball.deltay = hftonl( from.ball_.vx_ );

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
convert( const ShowInfoT & from,
         short_showinfo_t2 & to )
{
    // ball
    to.ball.x = hftonl( from.ball_.x_ );
    to.ball.y = hftonl( from.ball_.y_ );
    to.ball.deltax = hftonl( from.ball_.vx_ );
    to.ball.deltay = hftonl( from.ball_.vx_ );

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
convert( const showinfo_t2 & from,
         ShowInfoT & to )

{
    // ball
    convert( from.ball, to.ball_ );

    // players
    for ( int i = 0; i < MAX_PLAYER; ++i )
    {
        convert( LEFT, i + 1, from.pos[i], to.player_[i] );
    }

    for ( int i = MAX_PLAYER; i < MAX_PLAYER*2; ++i )
    {
        convert( RIGHT, i + 1 - MAX_PLAYER, from.pos[i], to.player_[i] );
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
    for ( int i = 0; i < MAX_PLAYER; ++i )
    {
        convert( LEFT, i + 1, from.pos[i], to.player_[i] );
    }

    for ( int i = MAX_PLAYER; i < MAX_PLAYER*2; ++i )
    {
        convert( RIGHT, i + 1 - MAX_PLAYER, from.pos[i], to.player_[i] );
    }

    // time
    to.time_ = static_cast< UInt32 >( ntohs( from.time ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
convert( const std::string & from,
         msginfo_t & to )
{
    to.board = htons( MSG_BOARD );
    std::memset( to.message, 0, sizeof( to.message ) );
    std::strncpy( to.message,
                  from.c_str(),
                  std::min( sizeof( to.message ) - 1, from.length() ) );
}

/*-------------------------------------------------------------------*/
PlayMode
to_playmode_enum( const std::string & playmode )
{
    static const std::unordered_map< std::string, PlayMode > s_map = {
        { "before_kick_off", PM_BeforeKickOff },
        { "time_over", PM_TimeOver },
        { "play_on", PM_PlayOn },
        { "kick_off_l", PM_KickOff_Left },
        { "kick_off_r", PM_KickOff_Right },
        { "kick_in_l", PM_KickIn_Left },
        { "kick_in_r", PM_KickIn_Right },
        { "free_kick_l", PM_FreeKick_Left },
        { "free_kick_r", PM_FreeKick_Right },
        { "corner_kick_l", PM_CornerKick_Left },
        { "corner_kick_r", PM_CornerKick_Right },
        { "goal_kick_l", PM_GoalKick_Left },
        { "goal_kick_r", PM_GoalKick_Right },
        { "goal_l", PM_AfterGoal_Left },
        { "goal_r", PM_AfterGoal_Right },
        { "drop_ball", PM_Drop_Ball },
        { "offside_l", PM_OffSide_Left },
        { "offside_r", PM_OffSide_Right },
        { "penalty_kick_l", PM_PK_Left },
        { "penalty_kick_r", PM_PK_Right },
        { "first_half_over", PM_FirstHalfOver },
        { "pause", PM_Pause },
        { "human_judge", PM_Human },
        { "foul_charge_l", PM_Foul_Charge_Left },
        { "foul_charge_r", PM_Foul_Charge_Right },
        { "foul_push_l", PM_Foul_Push_Left },
        { "foul_push_l", PM_Foul_Push_Right },
        { "foul_multiple_attack_l", PM_Foul_MultipleAttacker_Left },
        { "foul_multiple_attack_r", PM_Foul_MultipleAttacker_Right },
        { "foul_ballout_l", PM_Foul_BallOut_Left },
        { "foul_ballout_r", PM_Foul_BallOut_Right },
        { "back_pass_l", PM_Back_Pass_Left },
        { "back_pass_r", PM_Back_Pass_Right },
        { "free_kick_fault_l", PM_Free_Kick_Fault_Left },
        { "free_kick_fault_r", PM_Free_Kick_Fault_Right },
        { "catch_fault_l", PM_CatchFault_Left },
        { "catch_fault_r", PM_CatchFault_Right },
        { "indirect_free_kick_l", PM_IndFreeKick_Left },
        { "indirect_free_kick_r", PM_IndFreeKick_Right },
        { "penalty_setup_l", PM_PenaltySetup_Left },
        { "penalty_setup_r", PM_PenaltySetup_Right },
        { "penalty_ready_l", PM_PenaltyReady_Left },
        { "penalty_ready_r", PM_PenaltyReady_Right },
        { "penalty_taken_l", PM_PenaltyTaken_Left },
        { "penalty_taken_r", PM_PenaltyTaken_Right },
        { "penalty_miss_l", PM_PenaltyMiss_Left },
        { "penalty_miss_r", PM_PenaltyMiss_Right },
        { "penalty_score_l", PM_PenaltyScore_Left },
        { "penalty_score_r", PM_PenaltyScore_Right },
        { "illegal_defense_l", PM_Illegal_Defense_Left },
        { "illegal_defense_r", PM_Illegal_Defense_Right },
    };

    std::unordered_map< std::string, PlayMode >::const_iterator it = s_map.find( playmode );
    if ( it == s_map.end() )
    {
        return PM_Null;
    }

    return it->second;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::string
to_string( const player_type_t & from )
{
    std::ostringstream os;
    os << "(player_type ";

    os << "(id " << nstohi( from.id ) << ')'
       << "(player_speed_max " << nltohd( from.player_speed_max ) << ')'
       << "(stamina_inc_max " << nltohd( from.stamina_inc_max ) << ')'
       << "(player_decay " << nltohd( from.player_decay ) << ')'
       << "(inertia_moment " << nltohd( from.inertia_moment ) << ')'
       << "(dash_power_rate " << nltohd( from.dash_power_rate ) << ')'
       << "(player_size " << nltohd( from.player_size ) << ')'
       << "(kickable_margin " << nltohd( from.kickable_margin ) << ')'
       << "(kick_rand " << nltohd( from.kick_rand ) << ')'
       << "(extra_stamina " << nltohd( from.extra_stamina ) << ')'
       << "(effort_max " << nltohd( from.effort_max ) << ')'
       << "(effort_min " << nltohd( from.effort_min ) << ')';

    if ( from.kick_power_rate != 0 )
    {
        os << "(kick_power_rate " << nltohd( from.kick_power_rate ) << ')';
    }

    if ( from.foul_detect_probability != 0 )
    {
        os << "(foul_detect_probability " << nltohd( from.foul_detect_probability ) << ')';
    }

    if ( from.catchable_area_l_stretch != 0 )
    {
        os << "(catchable_area_l_stretch " << nltohd( from.catchable_area_l_stretch ) << ')';
    }

    os << ')';
    return os.str();
}

/*-------------------------------------------------------------------*/
/*!

 */
std::string
to_string( const server_params_t & from )
{
    double tmp = 0.0;

    std::ostringstream os;
    os << "(server_param ";

    os << "(goal_width " << nltohd( from.goal_width ) << ')'
       << "(inertia_moment " << nltohd( from.inertia_moment ) << ')'

       << "(player_size " << nltohd( from.player_size ) << ')'
       << "(player_decay " << nltohd( from.player_decay ) << ')'
       << "(player_rand " << nltohd( from.player_rand ) << ')'
       << "(player_weight " << nltohd( from.player_weight ) << ')'
       << "(player_speed_max " << nltohd( from.player_speed_max ) << ')'
       << "(player_accel_max " << nltohd( from.player_accel_max ) << ')'

       << "(stamina_max " << nltohd( from.stamina_max ) << ')'
       << "(stamina_inc_max " << nltohd( from.stamina_inc ) << ')'

       << "(recover_init " << nltohd( from.recover_init ) << ')'
       << "(recover_dec_thr " << nltohd( from.recover_dec_thr ) << ')'
       << "(recover_min " << nltohd( from.recover_min ) << ')'
       << "(recover_dec " << nltohd( from.recover_dec ) << ')'

       << "(effort_init " << nltohd( from.effort_init ) << ')'
       << "(effort_dec_thr " << nltohd( from.effort_dec_thr ) << ')'
       << "(effort_min " << nltohd( from.effort_min ) << ')'
       << "(effort_dec " << nltohd( from.effort_dec ) << ')'
       << "(effort_inc_thr " << nltohd( from.effort_inc_thr ) << ')'
       << "(effort_inc " << nltohd( from.effort_inc ) << ')'

       << "(kick_rand " << nltohd( from.kick_rand ) << ')'
       << "(team_actuator_noise " <<nstohb( from.team_actuator_noise ) << ')'
       << "(prand_factor_l " << nltohd( from.player_rand_factor_l ) << ')'
       << "(prand_factor_r " << nltohd( from.player_rand_factor_r ) << ')'
       << "(kick_rand_factor_l " << nltohd( from.kick_rand_factor_l ) << ')'
       << "(kick_rand_factor_r " << nltohd( from.kick_rand_factor_r ) << ')'

       << "(ball_size " << nltohd( from.ball_size ) << ')'
       << "(ball_decay " << nltohd( from.ball_decay ) << ')'
       << "(ball_rand " << nltohd( from.ball_rand ) << ')'
       << "(ball_weight " << nltohd( from.ball_weight ) << ')'
       << "(ball_speed_max " << nltohd( from.ball_speed_max ) << ')'
       << "(ball_accel_max " << nltohd( from.ball_accel_max ) << ')'

       << "(dash_power_rate " << nltohd( from.dash_power_rate ) << ')'
       << "(kick_power_rate " << nltohd( from.kick_power_rate ) << ')'
       << "(kickable_margin " << nltohd( from.kickable_margin ) << ')'
       << "(control_radius " << nltohd( from.control_radius ) << ')'
        //<< "(control_radius_width " << nltohd( from.control_radius_width ) << ')'

       << "(maxpower " << nltohd( from.max_power ) << ')'
       << "(minpower " << nltohd( from.min_power ) << ')'
       << "(maxmoment " << nltohd( from.max_moment ) << ')'
       << "(minmoment " << nltohd( from.min_moment ) << ')'
       << "(maxneckmoment " << nltohd( from.max_neck_moment ) << ')'
       << "(minneckmoment " << nltohd( from.min_neck_moment ) << ')'
       << "(maxneckang " << nltohd( from.max_neck_angle ) << ')'
       << "(minneckang " << nltohd( from.min_neck_angle ) << ')'

       << "(visible_angle " << nltohd( from.visible_angle ) << ')'
       << "(visible_distance " << nltohd( from.visible_distance ) << ')'

       << "(wind_dir " << nltohd( from.wind_dir ) << ')'
       << "(wind_force " << nltohd( from.wind_force ) << ')'
       << "(wind_ang " << nltohd( from.wind_ang ) << ')'
       << "(wind_rand " << nltohd( from.wind_rand ) << ')'

        //<< "(kickable_area " << nltohd( from.kickable_area ) << ')'

       << "(catchable_area_l " << nltohd( from.catch_area_l ) << ')'
       << "(catchable_area_w " << nltohd( from.catch_area_w ) << ')'
       << "(catch_probability " << nltohd( from.catch_probability ) << ')'
       << "(goalie_max_moves " << nstohi( from.goalie_max_moves ) << ')'

       << "(ckick_margin " << nltohd( from.corner_kick_margin ) << ')'
       << "(offside_active_area_size " << nltohd( from.offside_active_area ) << ')'

       << "(wind_none " << nstohb( from.wind_none ) << ')'
       << "(wind_random " << nstohb( from.use_wind_random ) << ')'

       << "(say_coach_cnt_max " << nstohi( from.coach_say_count_max ) << ')'
       << "(say_coach_msg_size " << nstohi( from.coach_say_msg_size ) << ')'

       << "(clang_win_size " << nstohi( from.clang_win_size ) << ')'
       << "(clang_define_win " << nstohi( from.clang_define_win ) << ')'
       << "(clang_meta_win " << nstohi( from.clang_meta_win ) << ')'
       << "(clang_advice_win " << nstohi( from.clang_advice_win ) << ')'
       << "(clang_info_win " << nstohi( from.clang_info_win ) << ')'
       << "(clang_mess_delay " << nstohi( from.clang_mess_delay ) << ')'
       << "(clang_mess_per_cycle " << nstohi( from.clang_mess_per_cycle ) << ')'

       << "(half_time " << nstohi( from.half_time ) << ')'
       << "(simulator_step " << nstohi( from.simulator_step ) << ')'
       << "(send_step " << nstohi( from.send_step ) << ')'
       << "(recv_step " << nstohi( from.recv_step ) << ')'
       << "(sense_body_step " << nstohi( from.sense_body_step ) << ')'
        //lcm_step = nstohi( from.lcm_step )

       << "(say_msg_size " << nstohi( from.player_say_msg_size ) << ')'
       << "(hear_max " << nstohi( from.player_hear_max ) << ')'
       << "(hear_inc " << nstohi( from.player_hear_inc ) << ')'
       << "(hear_decay " << nstohi( from.player_hear_decay ) << ')'

       << "(catch_ban_cycle " << nstohi( from.catch_ban_cycle ) << ')'

       << "(slow_down_factor " << nstohi( from.slow_down_factor ) << ')'

       << "(use_offside " << nstohb( from.use_offside) << ')'
       << "(forbid_kick_off_offside " << nstohb( from.kickoff_offside ) << ')'
       << "(offside_kick_margin " << nltohd( from.offside_kick_margin ) << ')'

       << "(audio_cut_dist " << nltohd(from.audio_cut_dist ) << ')'

       << "(quantize_step " << nltohd( from.dist_quantize_step ) << ')'
       << "(quantize_step_l " << rcg::nltohd( from.landmark_dist_quantize_step ) << ')'
        //nltohd( from.dir_quantize_step )
        //nltohd( from.dist_quantize_step_l )
        //nltohd( from.dist_quantize_step_r )
        //nltohd( from.landmark_dist_quantize_step_l )
        //nltohd( from.landmark_dist_quantize_step_r )
        //nltohd( from.dir_quantize_step_l )
        //nltohd( from.dir_quantize_step_r )

       << "(coach " << nstohb( from.coach_mode ) << ')'
       << "(coach_w_referee " << nstohb( from.coach_with_referee_mode ) << ')'
       << "(old_coach_hear " << nstohb( from.use_old_coach_hear ) << ')'

       << "(send_vi_step " << nstohi( from.online_coach_look_step ) << ')'

       << "(slowness_on_top_for_left_team " << nltohd( from.slowness_on_top_for_left_team ) << ')'
       << "(slowness_on_top_for_right_team " << nltohd( from.slowness_on_top_for_right_team ) << ')'

       << "(keepaway_length " << nltohd( from.ka_length ) << ')'
       << "(keepaway_width " << nltohd( from.ka_width ) << ')';

    // 11.0.0
    tmp = nltohd( from.ball_stuck_area );
    if ( std::fabs( tmp ) < 1000.0 )
    {
        os << "(ball_stuck_area " << tmp << ')';
    }

    // 12.0.0
    tmp = nltohd( from.max_tackle_power );
    if ( 0.0 < tmp && std::fabs( tmp ) < 200.0 )
    {
        os << "(max_tackle_power " << tmp << ')';
    }

    tmp = nltohd( from.max_back_tackle_power );
    if ( 0.0 < tmp && std::fabs( tmp ) < 200.0 )
    {
        os << "(max_back_tackle_power " << tmp << ')';
    }

    tmp = nltohd( from.tackle_dist );
    if ( 0.0 <= tmp && std::fabs( tmp ) < 3.0 )
    {
        os << "(tackle_dist " << tmp << ')';
    }

    tmp = nltohd( from.tackle_back_dist );
    if ( 0.0 <= tmp && std::fabs( tmp ) < 1.0 )
    {
        os << "(tackle_back_dist " << tmp << ')';
    }

    tmp = nltohd( from.tackle_width );
    if ( 0.0 < tmp && std::fabs( tmp ) < 2.0 )
    {
        os << "(tackle_width " << tmp << ')';
    }

    os << "(start_goal_l " << nstohi( from.start_goal_l ) << ')'
       << "(start_goal_r " << nstohi( from.start_goal_r ) << ')'

       << "(fullstate_l " << nstohb( from.fullstate_l ) << ')'
       << "(fullstate_r " << nstohb( from.fullstate_r ) << ')'

       << "(drop_ball_time " << nstohi( from.drop_ball_time ) << ')'

       << "(synch_mode " << nstohb( from.synch_mode ) << ')'
       << "(synch_offset " << nstohi( from.synch_offset ) << ')'
       << "(synch_micro_sleep " << nstohi( from.synch_micro_sleep ) << ')'

       << "(point_to_ban " << nstohi( from.point_to_ban ) << ')'
       << "(point_to_duration " << nstohi( from.point_to_duration ) << ')'
        ;

    os << ')';
    return os.str();
}

/*-------------------------------------------------------------------*/
/*!

 */
std::string
to_string( const player_params_t & from )
{
    std::ostringstream os;
    os << "(player_param ";

    os << "(player_types " << nstohi( from.player_types ) << ')'
       << "(subs_max " << nstohi( from.substitute_max ) << ')'
       << "(pt_max " << nstohi( from.pt_max ) << ')'

       << "(player_speed_max_delta_min " << nltohd( from.player_speed_max_delta_min ) << ')'
       << "(player_speed_max_delta_max " << nltohd( from.player_speed_max_delta_max ) << ')'
       << "(stamina_inc_max_delta_factor " << nltohd( from.stamina_inc_max_delta_factor ) << ')'

       << "(player_decay_delta_min " << nltohd( from.player_decay_delta_min ) << ')'
       << "(player_decay_delta_max " << nltohd( from.player_decay_delta_max ) << ')'
       << "(inertia_moment_delta_factor " << nltohd( from.inertia_moment_delta_factor ) << ')'

       << "(dash_power_rate_delta_min " << nltohd( from.dash_power_rate_delta_min ) << ')'
       << "(dash_power_rate_delta_max " << nltohd( from.dash_power_rate_delta_max ) << ')'
       << "(player_size_delta_factor " << nltohd( from.player_size_delta_factor ) << ')'

       << "(kickable_margin_delta_min " << nltohd( from.kickable_margin_delta_min ) << ')'
       << "(kickable_margin_delta_max " << nltohd( from.kickable_margin_delta_max ) << ')'
       << "(kick_rand_delta_factor " << nltohd( from.kick_rand_delta_factor ) << ')'

       << "(extra_stamina_delta_min " << nltohd( from.extra_stamina_delta_min ) << ')'
       << "(extra_stamina_delta_max " << nltohd( from.extra_stamina_delta_max ) << ')'
       << "(effort_max_delta_factor " << nltohd( from.effort_max_delta_factor ) << ')'
       << "(effort_min_delta_factor " << nltohd( from.effort_min_delta_factor ) << ')'

       << "(random_seed " << static_cast< int >( static_cast< rcg::Int32 >( ntohl( from.random_seed ) ) ) << ')'

       << "(new_dash_power_rate_delta_min " << nltohd( from.new_dash_power_rate_delta_min ) << ')'
       << "(new_dash_power_rate_delta_max " << nltohd( from.new_dash_power_rate_delta_max ) << ')'
       << "(new_stamina_inc_max_delta_factor " << nltohd( from.new_stamina_inc_max_delta_factor ) << ')'

       << "(allow_mult_default_type " << nstohb( from.allow_mult_default_type ) << ')'

       << "(kick_power_rate_delta_min " << nltohd( from.kick_power_rate_delta_min ) << ')'
       << "(kick_power_rate_delta_max " << nltohd( from.kick_power_rate_delta_max ) << ')'
       << "(foul_detect_probability_delta_factor " << nltohd( from.foul_detect_probability_delta_factor ) << ')'

       << "(catchable_area_l_stretch_min " << nltohd( from.catchable_area_l_stretch_min ) << ')'
       << "(catchable_area_l_stretch_max " << nltohd( from.catchable_area_l_stretch_max ) << ')'
        ;

    os << ')';
    return os.str();
}

} // end namespace
} // end namespace
