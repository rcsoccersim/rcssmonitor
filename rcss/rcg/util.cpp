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

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <rcss/rcg/types.h>

#include <sstream>
#include <cstring>
#include <cmath>

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

namespace {

inline
Int16
my_htons( const Int16 v )
{
    return htons( v );
}

inline
Int16
my_ntohs( const Int16 v )
{
    return ntohs( v );
}

inline
Int32
my_ntohl( const Int32 v )
{
    return ntohl( v );
}

inline
Int32
my_htonl( const Int32 v )
{
    return htonl( v );
}

}


/*-------------------------------------------------------------------*/
Int16
nstohi( const Int16 val )
{
    return static_cast< int >( static_cast< Int16 >( ntohs( val ) ) );
}

/*-------------------------------------------------------------------*/
Int16
hitons( const int val )
{
    return htons( static_cast< Int16 >( val ) );
}

/*-------------------------------------------------------------------*/
bool
nstohb( const Int16 val )
{
    return static_cast< bool >( static_cast< Int16 >( ntohs( val ) ) );
}

/*-------------------------------------------------------------------*/
Int16
hbtons( const bool val )
{
    return htons( val ? 1 : 0 );
}

/*-------------------------------------------------------------------*/
double
nstohd( const Int16 val )
{
    return ( static_cast< double >( static_cast< Int16 >( ntohs( val ) ) )
             / SHOWINFO_SCALE );
}

/*-------------------------------------------------------------------*/
float
nstohf( const Int16 val )
{
    return ( static_cast< float >( static_cast< Int16 >( ntohs( val ) ) )
             / SHOWINFO_SCALEF );
}

/*-------------------------------------------------------------------*/
Int16
hdtons( const double & val )
{
    return htons( static_cast< Int16 >( rint( val * SHOWINFO_SCALE ) ) );
}

/*-------------------------------------------------------------------*/
Int16
hftons( const float & val )
{
    return htons( static_cast< Int16 >( rintf( val * SHOWINFO_SCALEF ) ) );
}

/*-------------------------------------------------------------------*/
double
nltohd( const Int32 & val )
{
    return ( static_cast< double >( static_cast< Int32 >( ntohl( val ) ) )
             / SHOWINFO_SCALE2 );
}

/*-------------------------------------------------------------------*/
float
nltohf( const Int32 & val )
{
    return ( static_cast< float >( static_cast< Int32 >( ntohl( val ) ) )
             / SHOWINFO_SCALE2F );
}

/*-------------------------------------------------------------------*/
Int32
hdtonl( const double & val )
{
    return htonl( static_cast< Int32 >( rint( val * SHOWINFO_SCALE2 ) ) );
}

/*-------------------------------------------------------------------*/
Int32
hftonl( const float & val )
{
    return htonl( static_cast< Int32 >( rintf( val * SHOWINFO_SCALE2F ) ) );
}

/*-------------------------------------------------------------------*/
Int32
nstonl( const Int16 val )
{
    return hdtonl( nstohd( val ) );
}

/*-------------------------------------------------------------------*/
Int16
nltons( const Int32 & val )
{
    return hdtons( nltohd( val ) );
}

/*-------------------------------------------------------------------*/
void
convert( const pos_t & from,
         BallT & to )
{
    to.x_ = nstohf( from.x );
    to.y_ = nstohf( from.y );
}

/*-------------------------------------------------------------------*/
void
convert( const BallT & from,
         pos_t & to )
{
    to.x = hftons( from.x_ );
    to.y = hftons( from.y_ );
}

/*-------------------------------------------------------------------*/
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
void
convert( const pos_t & from,
         PlayerT & to )
{
    Side side = static_cast< Side >( static_cast< Int16 >( ntohs( from.side ) ) );

    to.state_ = static_cast< Int32 >( ntohs( from.enable ) );
    to.side_ = side;
    to.unum_ = ntohs( from.unum );
    to.body_ = static_cast< float >( ntohs( from.angle ) );
    to.x_ = nstohf( from.x );
    to.y_ = nstohf( from.y );
}

/*-------------------------------------------------------------------*/
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
void
convert( const Side side,
         const int unum,
         const player_t & from,
         PlayerT & to )
{
    to.side_ = side;
    to.unum_ = static_cast< Int16 >( unum );
    to.type_ = ntohs( from.type );

    to.high_quality_ = ntohs( from.view_quality ) ? true : false;

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

    to.random_seed_ = static_cast< int >( my_ntohl( from.random_seed ) );

    to.new_dash_power_rate_delta_min_ = nltohd( from.new_dash_power_rate_delta_min );
    to.new_dash_power_rate_delta_max_ = nltohd( from.new_dash_power_rate_delta_max );
    to.new_stamina_inc_max_delta_factor_ = nltohd( from.new_stamina_inc_max_delta_factor );

    to.allow_mult_default_type_ = static_cast< bool >( my_ntohs( from.allow_mult_default_type ) );
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
    to.random_seed = my_htonl( static_cast< Int32 >( from.random_seed_ ) );

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
    //to.control_radius_width_ = nltohd( from.control_radius_width );

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
    to.use_wind_random_ = nstohb( from.wind_rand );

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
    to.kick_off_offside_ = nstohb( from.kick_off_offside );
    to.offside_kick_margin_ = nltohd( from.offside_kick_margin );

    to.audio_cut_dist_ = nltohd(from.audio_cut_dist );

    to.dist_quantize_step_ = nltohd( from.dist_quantize_step );
    to.landmark_dist_quantize_step_ = nltohd( from.landmark_dist_quantize_step );
    //to.dir_quantize_step_ = nltohd( from.dir_quantize_step );
    //to.dist_quantize_step_l_ = nltohd( from.dist_quantize_step_l );
    //to.dist_quantize_step_r_ = nltohd( from.dist_quantize_step_r );
    //to.landmark_dist_quantize_step_l_ = nltohd( from.landmark_dist_quantize_step_l );
    //to.landmark_dist_quantize_step_r_ = nltohd( from.landmark_dist_quantize_step_r );
    //to.dir_quantize_step_l_ = nltohd( from.dir_quantize_step_l );
    //to.dir_quantize_step_r_ = nltohd( from.dir_quantize_step_r );

    to.coach_mode_ = nstohb( from.coach_mode );
    to.coach_with_referee_mode_ = nstohb( from.coach_with_referee_mode );
    to.use_old_coach_hear_ = nstohb( from.use_old_coach_hear );

    to.online_coach_look_step_ = nstohi( from.online_coach_look_step );

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
    //to.control_radius_width = hdtonl( from.control_radius_width_ );

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
    to.use_wind_random = hbtons( from.use_wind_random_ );

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
    to.kick_off_offside = hbtons( from.kick_off_offside_ );
    to.offside_kick_margin = hdtonl( from.offside_kick_margin_ );

    to.audio_cut_dist = hdtonl(from.audio_cut_dist_ );

    to.dist_quantize_step = hdtonl( from.dist_quantize_step_ );
    to.landmark_dist_quantize_step = hdtonl( from.landmark_dist_quantize_step_ );

    to.dir_quantize_step = my_htonl( 0 );
    to.dist_quantize_step_l = my_htonl( 0 );
    to.dist_quantize_step_r = my_htonl( 0 );
    to.landmark_dist_quantize_step_l = my_htonl( 0 );
    to.landmark_dist_quantize_step_r = my_htonl( 0 );
    to.dir_quantize_step_l = my_htonl( 0 );
    to.dir_quantize_step_r = my_htonl( 0 );

    to.coach_mode = hbtons( from.coach_mode_ );
    to.coach_with_referee_mode = hbtons( from.coach_with_referee_mode_ );
    to.use_old_coach_hear = hbtons( from.use_old_coach_hear_ );

    to.online_coach_look_step = hitons( from.online_coach_look_step_ );

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
/*!

 */
std::string
to_sexp( const player_type_t & from )
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
to_sexp( const server_params_t & from )
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
       << "(wind_ang " << nltohd( from.wind_angle ) << ')'
       << "(wind_rand " << nltohd( from.wind_rand ) << ')'

        //<< "(kickable_area " << nltohd( from.kickable_area ) << ')'

       << "(catchable_area_l " << nltohd( from.catchable_area_l ) << ')'
       << "(catchable_area_w " << nltohd( from.catchable_area_w ) << ')'
       << "(catch_probability " << nltohd( from.catch_probability ) << ')'
       << "(goalie_max_moves " << nstohi( from.goalie_max_moves ) << ')'

       << "(ckick_margin " << nltohd( from.corner_kick_margin ) << ')'
       << "(offside_active_area_size " << nltohd( from.offside_active_area_size ) << ')'

       << "(wind_none " << nstohb( from.wind_none ) << ')'
       << "(wind_random " << nstohb( from.use_wind_random ) << ')'

       << "(say_coach_cnt_max " << nstohi( from.say_coach_count_max ) << ')'
       << "(say_coach_msg_size " << nstohi( from.say_coach_msg_size ) << ')'

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

       << "(say_msg_size " << nstohi( from.say_msg_size ) << ')'
       << "(hear_max " << nstohi( from.hear_max ) << ')'
       << "(hear_inc " << nstohi( from.hear_inc ) << ')'
       << "(hear_decay " << nstohi( from.hear_decay ) << ')'

       << "(catch_ban_cycle " << nstohi( from.catch_ban_cycle ) << ')'

       << "(slow_down_factor " << nstohi( from.slow_down_factor ) << ')'

       << "(use_offside " << nstohb( from.use_offside) << ')'
       << "(forbid_kick_off_offside " << nstohb( from.kick_off_offside ) << ')'
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

       << "(keepaway_length " << nltohd( from.keepaway_length ) << ')'
       << "(keepaway_width " << nltohd( from.keepaway_width ) << ')';

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
to_sexp( const player_params_t & from )
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
