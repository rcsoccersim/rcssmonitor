/*
 * Copyright (c) 1999 - 2001, Artur Merke <amerke@ira.uka.de>
 *
 * This file is part of FrameView2d.
 *
 * FrameView2d is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * FrameView2d is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FrameView2d; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "smonitor_dev.h"
#include "global_defs.h"

//#include "object2d.h"
#include "rgb_db.h"
#include "ascii_processor.h"
#include "str2val.h"
//#include "tools.h"

#include <boost/cstdint.hpp>

#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif

#include <map>
#include <iostream>
#include <iomanip>
#include <cstdio>      /* Standard I/O definitions. */
#include <cstdlib>     /* Standard C Library definitions. */
#include <cstring>     /* C Library string functions. */
#include <cctype>      /* isprint(...) */
#include <cmath>

#include <netinet/in.h> /* wegen htons(...)*/

//#define DEBUG(XXX) cout << "\ndebug(" << __LINE__ << "):" XXX << flush
#define DEBUG(XXX)

typedef boost::int16_t Int16;
typedef boost::int32_t Int32;

//#define WEBFIELD 1 //just a macro which doesn't show the menu and the players (for the NetPlayer)
/*****************************************************************************
 \short soccerserver parameters (version 1 and 2)

 This part contains server specific data types, which are taken from the files
   - sserver-6.XX/server/param.h
   - sserver-6.XX/server/utility
 and for the extension for protocol version 2
   - sserver-7.00/server/param.h
   - sserver-7.00/server/utility
*/
namespace SSrv {
// from  utility.h
#define PI M_PI

// from param.h

#define SHOWINFO_SCALE          16.0
#define SHOWINFO_SCALE2         65536.0 //ver. 2
#define SHOWINFO_SCALE2_int     65536   //art!

enum ProtocolMode {
    NO_INFO = 0,  //ver. 2
    SHOW_MODE,
    MSG_MODE,
    DRAW_MODE,
    BLANK_MODE,
    PM_MODE, //ver. 2
    TEAM_MODE, //ver. 2
    PT_MODE, //ver. 2
    PARAM_MODE, //ver. 2
    PPARAM_MODE, //ver. 2
    FRAMEVIEW_MODE, //art!!!
    //ver. 3
    V3_SHOW_MODE,
    V3_MSG_MODE,
    V3_DRAW_MODE,
    V3_BLANK_MODE,
    V3_PM_MODE,
    V3_TEAM_MODE,
    V3_PT_MODE,
    V3_PARAM_MODE,
    V3_PPARAM_MODE,
};

#define DrawClear       0
#define DrawPoint       1
#define DrawCircle      2
#define DrawLine        3
#define MSG_BOARD       1
#define LOG_BOARD       2

//#define MAX_PLAYER 11

#define	DISABLE			0x0000
#define	STAND			0x0001
#define	KICK			0x0002
#define KICK_FAULT		0x0004
#define	GOALIE			0x0008
#define CATCH			0x0010
#define CATCH_FAULT		0x0020
#define BALL_TO_PLAYER          0x0040
#define PLAYER_TO_BAL           0x0080
#define DISCARD                 0x0100
#define BALL_COLLIDE            0x0400 // player collided with the ball
#define PLAYER_COLLIDE          0x0800 // player collided with another player
#define TACKLE       0x1000
#define TACKLE_FAULT 0x2000
#define BACK_PASS 0x4000
#define FREE_KICK_FAULT 0x8000

#define PLAYMODE_STRINGS {"",                   \
            "before_kick_off",                  \
            "time_over",                        \
            "play_on",                          \
            "kick_off_l",                       \
            "kick_off_r",                       \
            "kick_in_l",                        \
            "kick_in_r",                        \
            "free_kick_l",                      \
            "free_kick_r",                      \
            "corner_kick_l",                    \
            "corner_kick_r",                    \
            "goal_kick_l",                      \
            "goal_kick_r",                      \
            "goal_l",                           \
            "goal_r",                           \
            "drop_ball",                        \
            "offside_l",                        \
            "offside_r",                        \
            "penalty_kick_l",                   \
            "penalty_kick_r",                   \
            "first_half_over",                  \
            "pause",                            \
            "human_judge",                      \
            "foul_charge_l",                    \
            "foul_charge_r",                    \
            "foul_push_l",                      \
            "foul_push_r",                      \
            "foul_multiple_attack_l",           \
            "foul_multiple_attack_r",           \
            "foul_ballout_l",                   \
            "foul_ballout_r",                   \
            "back_pass_l",                      \
            "back_pass_r",                      \
            "free_kick_fault_l",                \
            "free_kick_fault_r",                \
            "catch_fault_l",                    \
            "catch_fault_r",                    \
            "indirect_free_kick_l",             \
            "indirect_free_kick_r",             \
            "penalty_setup_l",                  \
            "penalty_setup_r",                  \
            "penalty_ready_l",                  \
            "penalty_ready_r",                  \
            "penalty_taken_l",                  \
            "penalty_taken_r",                  \
            "penalty_miss_l",                   \
            "penalty_miss_r",                   \
            "penalty_score_l",                  \
            "penalty_score_r"                   \
            }

const int PLAYMODE_STRINGS_SIZE = 36 + 14;

#define	COLOR_NAME_MAX	64

typedef struct {
    Int16	enable;
    Int16	side;
    Int16	unum;
    Int16	angle;
    Int16	x;
    Int16	y;
} pos_t;

typedef struct {
    char	name[16];
    Int16	score;
} team_t;

typedef struct {
    char	pmode;
    team_t	team[2];
    pos_t	pos[MAX_PLAYER * 2 + 1];
    Int16	time;
} showinfo_t;

typedef	struct {
    Int16	board;
    char	message[2048];
} msginfo_t;

typedef struct {
    Int16	x;
    Int16	y;
    char	color[COLOR_NAME_MAX];
} pointinfo_t;

typedef struct {
    Int16	x;
    Int16	y;
    Int16	r;
    char	color[COLOR_NAME_MAX];
} circleinfo_t;

typedef struct {
    Int16	x1;
    Int16	y1;
    Int16	x2;
    Int16	y2;
    char	color[COLOR_NAME_MAX];
} lineinfo_t;

typedef struct {
    Int16 mode;
    union {
        pointinfo_t		pinfo;
        circleinfo_t	cinfo;
        lineinfo_t		linfo;
    } object;
} drawinfo_t;

typedef struct {
    Int16	mode;
    union {
        showinfo_t	show;
        msginfo_t	msg;
        drawinfo_t	draw;
    } body;
} dispinfo_t;


typedef struct {
    Int32 x;
    Int32 y;
    Int32 deltax;
    Int32 deltay;
} ball_t;

typedef struct {
    Int16 mode;
    Int16 type;
    Int32 x;
    Int32 y;
    Int32 deltax;
    Int32 deltay;
    Int32 body_angle;
    Int32 head_angle;
    Int32 view_width;
    Int16 view_quality;
    Int32 stamina;
    Int32 effort;
    Int32 recovery;
    Int16 kick_count;
    Int16 dash_count;
    Int16 turn_count;
    Int16 say_count;
    Int16 tneck_count;
    Int16 catch_count;
    Int16 move_count;
    Int16 chg_view_count;
} player_t;

typedef struct {
    char          pmode;
    team_t        team[2];
    ball_t        ball;
    player_t      pos[MAX_PLAYER * 2];
    Int16         time;
} showinfo_t2;

typedef struct {
    Int16 id;
    Int32 player_speed_max;
    Int32 stamina_inc_max;
    Int32 player_decay;
    Int32 inertia_moment;
    Int32 dash_power_rate;
    Int32 player_size;
    Int32 kickable_margin;
    Int32 kick_rand;
    Int32 extra_stamina;
    Int32 effort_max;
    Int32 effort_min;

    // spare variables which are to be used for paramenter added in the future
    Int32 sparelong1;
    Int32 sparelong2;
    Int32 sparelong3;
    Int32 sparelong4;
    Int32 sparelong5;
    Int32 sparelong6;
    Int32 sparelong7;
    Int32 sparelong8;
    Int32 sparelong9;
    Int32 sparelong10;
} player_type_t;

typedef struct
{
    Int32 gwidth;					/* goal width */
    Int32 inertia_moment;			/* intertia moment for turn */
    Int32 psize;					/* player size */
    Int32 pdecay;					/* player decay */
    Int32 prand;					/* player rand */
    Int32 pweight;					/* player weight */
    Int32 pspeed_max;				/* player speed max */
    // th 6.3.00
    Int32 paccel_max;				/* player acceleration max */
    //
    Int32 stamina_max;				/* player stamina max */
    Int32 stamina_inc;				/* player stamina inc */
    Int32 recover_init;			/* player recovery init */
    Int32 recover_dthr;			/* player recovery decriment threshold */
    Int32 recover_min;				/* player recovery min */
    Int32 recover_dec;				/* player recovery decriment */
    Int32 effort_init;				/* player dash effort init */
    Int32 effort_dthr;				/* player dash effort decriment threshold */
    Int32 effort_min;				/* player dash effrot min */
    Int32 effort_dec;				/* player dash effort decriment */
    Int32 effort_ithr;				/* player dash effort incriment threshold */
    Int32 effort_inc;				/* player dash effort incriment */
    // pfr 8/14/00: for RC2000 evaluation
    Int32 kick_rand;                                /* noise added directly to kicks */
    Int16 team_actuator_noise;                        /* flag whether to use team specific actuator noise */
    Int32 prand_factor_l;                           /* factor to multiple prand for left team */
    Int32 prand_factor_r;                           /* factor to multiple prand for right team */
    Int32 kick_rand_factor_l;                       /* factor to multiple kick_rand for left team */
    Int32 kick_rand_factor_r;                       /* factor to multiple kick_rand for right team */

    Int32 bsize;					/* ball size */
    Int32 bdecay;					/* ball decay */
    Int32 brand;					/* ball rand */
    Int32 bweight;					/* ball weight */
    Int32 bspeed_max;				/* ball speed max */
    // th 6.3.00
    Int32 baccel_max;				/* ball acceleration max */
    //
    Int32 dprate;					/* dash power rate */
    Int32 kprate;					/* kick power rate */
    Int32 kmargin;					/* kickable margin */
    Int32 ctlradius;				/* control radius */
    Int32 ctlradius_width;			/* (control radius ) - (plyaer size ) */
    Int32 maxp;					/* max power */
    Int32 minp;					/* min power */
    Int32 maxm;					/* max moment */
    Int32 minm;					/* min moment */
    Int32 maxnm;					/* max neck moment */
    Int32 minnm;					/* min neck moment */
    Int32 maxn;					/* max neck angle */
    Int32 minn;					/* min neck angle */
    Int32 visangle;				/* visible angle */
    Int32 visdist;					/* visible distance */
    Int32 windir;					/* wind direction */
    Int32 winforce;				/* wind force */
    Int32 winang;					/* wind angle for rand */
    Int32 winrand;					/* wind force for force */
    Int32 kickable_area;			/* kickable_area */
    Int32 catch_area_l;			/* goalie catchable area length */
    Int32 catch_area_w;			/* goalie catchable area width */
    Int32 catch_prob;				/* goalie catchable possibility */
    Int16   goalie_max_moves;                 /* goalie max moves after a catch */
    Int32 ckmargin;				/* corner kick margin */
    Int32 offside_area;			/* offside active area size */
    Int16 win_no;					/* wind factor is none */
    Int16 win_random;				/* wind factor is random */
    Int16 say_cnt_max;				/* max count of coach SAY */
    Int16 SayCoachMsgSize;				/* max length of coach SAY */
    Int16 clang_win_size;
    Int16 clang_define_win;
    Int16 clang_meta_win;
    Int16 clang_advice_win;
    Int16 clang_info_win;
    Int16 clang_mess_delay;
    Int16 clang_mess_per_cycle;
    Int16 half_time;					/* half time */
    Int16 sim_st;					/* simulator step interval msec */
    Int16 send_st;					/* udp send step interval msec */
    Int16 recv_st;					/* udp recv step interval msec */
    Int16 sb_step;					/* sense_body interval step msec */
    Int16 lcm_st;		                        /* lcm of all the above steps msec */
    Int16 SayMsgSize;				/* string size of say message */
    Int16 hear_max;					/* player hear_capacity_max */
    Int16 hear_inc;					/* player hear_capacity_inc */
    Int16 hear_decay;				/* player hear_capacity_decay */
    Int16 cban_cycle;				/* goalie catch ban cycle */
    Int16 slow_down_factor;                          /* factor to slow down simulator and send intervals */
    Int16 useoffside;				/* flag for using off side rule */
    Int16 kickoffoffside;			/* flag for permit kick off offside */
    Int32 offside_kick_margin;		/* offside kick margin */
    Int32 audio_dist;				/* audio cut off distance */
    Int32 dist_qstep;				/* quantize step of distance */
    Int32 land_qstep;				/* quantize step of distance for landmark */
    Int32 dir_qstep;				/* quantize step of direction */
    Int32 dist_qstep_l;			/* team right quantize step of distance */
    Int32 dist_qstep_r;			/* team left quantize step of distance */
    Int32 land_qstep_l;			/* team right quantize step of distance for landmark */
    Int32 land_qstep_r;			/* team left quantize step of distance for landmark */
    Int32 dir_qstep_l;				/* team left quantize step of direction */
    Int32 dir_qstep_r;				/* team right quantize step of direction */
    Int16 CoachMode;				/* coach mode */
    Int16 CwRMode;					/* coach with referee mode */
    Int16 old_hear;					/* old format for hear command (coach ) */
    Int16 sv_st;					/* online coach's look interval step */


    // spare variables which are to be used for paramenter added in the future
    Int32 slowness_on_top_for_left_team;
    Int32 slowness_on_top_for_right_team;
    Int32 ka_length;                    /* keepaway region length */
    Int32 ka_width;                     /* keepaway region width */
    // 11.0.0
    Int32 ball_stuck_area; // automatic drop ball threshold while play_on
    // 12.0.0
    Int32 min_tackle_power;
    Int32 max_tackle_power;
    Int32 player_speed_max_min;
    Int32 extra_stamina;

    Int32 sparelong10;

    Int16 start_goal_l;
    Int16 start_goal_r;
    Int16 fullstate_l;
    Int16 fullstate_r;
    Int16 drop_time;
    Int16 synch_mode;
    Int16 synch_offset;
    Int16 synch_micro_sleep;
    Int16 point_to_ban;
    Int16 point_to_duration;

} server_params_t;

typedef struct {
    Int16 player_types;
    Int16 subs_max;
    Int16 pt_max;

    Int32 player_speed_max_delta_min;
    Int32 player_speed_max_delta_max;
    Int32 stamina_inc_max_delta_factor;

    Int32 player_decay_delta_min;
    Int32 player_decay_delta_max;
    Int32 inertia_moment_delta_factor;

    Int32 dash_power_rate_delta_min;
    Int32 dash_power_rate_delta_max;
    Int32 player_size_delta_factor;

    Int32 kickable_margin_delta_min;
    Int32 kickable_margin_delta_max;
    Int32 kick_rand_delta_factor;

    Int32 extra_stamina_delta_min;
    Int32 extra_stamina_delta_max;
    Int32 effort_max_delta_factor;
    Int32 effort_min_delta_factor;

    Int32 sparelong1;
    Int32 sparelong2;
    Int32 sparelong3;
    Int32 sparelong4;
    Int32 sparelong5;
    Int32 sparelong6;
    Int32 sparelong7;
    Int32 sparelong8;
    Int32 sparelong9;
    Int32 sparelong10;

    Int16 spareshort1;
    Int16 spareshort2;
    Int16 spareshort3;
    Int16 spareshort4;
    Int16 spareshort5;
    Int16 spareshort6;
    Int16 spareshort7;
    Int16 spareshort8;
    Int16 spareshort9;
    Int16 spareshort10;

} player_params_t;

typedef struct {
    Int16 mode;
    union {
        showinfo_t2 		show;
        msginfo_t 		msg;
        player_type_t		ptinfo;
        server_params_t		sparams;
        player_params_t		pparams;
    } body;
} dispinfo_t2;

}

/******************************************************************************/

class PlayerTypes {
    struct _player_type {
        bool valid;
        double player_radius;
        double kickable_margin;

        _player_type()
            : valid( false )
            , player_radius( 0.3 )
            , kickable_margin( 0.7 )
          {  }
    };

    static double ball_radius;
    static _player_type std_type;
    static std::map< int, _player_type > types;
public:
    static bool use_std_type;

    static
    void init( const double & std_ball_radius,
               const double & std_player_radius,
               const double & std_kickable_margin )
      {
          ball_radius = std_ball_radius;
          std_type.valid = true;
          std_type.player_radius = std_player_radius;
          std_type.kickable_margin = std_kickable_margin;
          use_std_type = true;
      }

    static
    void set_type( const int type,
                   const double & player_radius,
                   const double & kickable_margin )
      {
          //if (type < 0 || type >= NUM_TYPES ) {
          //WARNING_OUT << "\nwrong player type number" << type << " (ignoring )";
          //return;
          //}
          types[type].valid = true;
          types[type].player_radius = player_radius;
          types[type].kickable_margin = kickable_margin;
      }

    static
    double get_player_radius( const int type )
      {
          if ( type == -1 || use_std_type ) // default type
          {
              return std_type.player_radius;
          }

          std::map< int, _player_type >::iterator it = types.find( type );
          if ( it == types.end() )
          {
              WARNING_OUT << "\nwrong player type number" << type
                          << " (using std type )";
              return std_type.player_radius;
          }

          if ( ! it->second.valid )
          {
              WARNING_OUT << "\nplayer type " << type
                          << " not valid (using std type )";
              return std_type.player_radius;
          }

          return it->second.player_radius;
      }

    static
    double get_kick_radius( int type )
      {
          double def_result = std_type.player_radius
              + std_type.kickable_margin
              + ball_radius;

          if ( type == -1 || use_std_type ) // default type
          {
              return def_result;
          }

          std::map< int, _player_type >::iterator it = types.find( type );
          if ( it == types.end() )
          {
              WARNING_OUT << "\nwrong player type number" << type << " (using std type )";
              return def_result;
          }

          if ( ! it->second.valid )
          {
              WARNING_OUT << "\nplayer type " << type << " not valid (using std type )";
              return def_result;
          }

          return it->second.player_radius + it->second.kickable_margin + ball_radius;
      }
};

//definition of the static internal data of PlayerTypes
double PlayerTypes::ball_radius = 0.3;
PlayerTypes::_player_type PlayerTypes::std_type;
std::map< int, PlayerTypes::_player_type > PlayerTypes::types;
bool PlayerTypes::use_std_type = false;

/******************************************************************************/

const int VisualPlayersViewArea::LOW = 0;
const int VisualPlayersViewArea::HIGH = 1;
const int VisualPlayersViewArea::FEEL_RANGE= 2;


VisualPlayersViewArea::VisualPlayersViewArea()
{

}

void
VisualPlayersViewArea::init( const int my_key,
                             const int my_layer,
                             const RGBcolor & my_exact,
                             const RGBcolor & my_fuzzy )
{
    key = my_key;
    layer = my_layer;
    c_fuzzy = my_fuzzy;
    c_exact = my_exact;
    circlearc1.set_color( c_fuzzy );
    circlearc1.filled = true;

    circle.set_color( c_fuzzy );
    circle.filled = true;
    circle.rel.radius = 3.0; //feel range

    circlearc2.set_color( c_exact );
    circlearc2.filled = true;

    view_quality = -1;
    view_width = -1.0;
    set_view_mode( FEEL_RANGE, 0.5*M_PI );
}

void
VisualPlayersViewArea::draw( DisplayBase * disp,
                             const Area2d & area,
                             const Frame2d & p_frame,
                             const bool chg )
{
    circlearc1.draw( disp, area, p_frame, chg );
    circle.draw( disp, area, p_frame, chg );
    circlearc2.draw( disp, area, p_frame, chg );
}

void
VisualPlayersViewArea::actualize( const Frame2d & f,
                                  const bool chg )
{
    circlearc1.actualize( f, chg );
    circle.actualize( f, chg );
    circlearc2.actualize( f, chg );
}

void
VisualPlayersViewArea::set_view_mode( const int quality,
                                      const double & width )
{
    if ( quality != view_quality )
    {
        view_quality = quality;
        switch ( view_quality ) {
        case LOW:
            circlearc1.rel.radius = 40;
            circlearc1.changed = true;
            circlearc1.visible = true;

            circlearc2.rel.radius = 20;
            circlearc2.changed = true;
            circlearc2.visible = false;
            break;
        case HIGH:
            circlearc1.rel.radius = 60;
            circlearc1.changed = true;
            circlearc1.visible = true;

            circlearc2.rel.radius = 40;
            circlearc2.changed = true;
            circlearc2.visible = true;
            break;
        case FEEL_RANGE:
            circlearc1.visible = false;
            circlearc2.visible = false;
            break;
        default:
            ERROR_OUT << "wrong view_quality " << view_width;
        }
    }

    if ( std::fabs( width - view_width ) > 0.01 )
    {
        view_width = width;
        Angle a = 0.5 * view_width;
        circlearc1.rel.ang1 = -a;
        circlearc1.rel.ang2 =  a;
        circlearc1.changed = true;

        circlearc2.rel.ang1 = -a;
        circlearc2.rel.ang2 =  a;
        circlearc2.changed = true;
    }
}

/******************************************************************************/

const int VisualBall::MAX_LINE_MARKERS_NUM = 50;
const int VisualBall::MAX_LABEL_LEN = 20;

VisualBall::VisualBall()
{

}

void
VisualBall::init( const int my_key,
                  const int my_layer,
                  const double & my_small_r,
                  const double & my_large_r,
                  const RGBcolor & my_c_ball,
                  const RGBcolor & my_c_line_until_ball_stops,
                  const RGBcolor & my_c_line_markers,
                  const RGBcolor & my_c_font )
{
    ball_decay = 0.94;
    key = my_key;
    layer = my_layer;

    c_ball = my_c_ball;
    c_line_until_ball_stops = my_c_line_until_ball_stops;
    c_line_markers = my_c_line_markers;
    c_font = my_c_font;

    show_vel = false;
    show_vel_string = false;
    changed = true;

    double small_r = my_small_r;
    double large_r = my_large_r;

    ball_point.use_intersects_area = false;
    ball_point.set_color( c_ball );

    ball_circle.rel.radius = small_r;
    ball_circle.use_intersects_area = false;
    ball_circle.set_color( c_ball );
    ball_circle.filled = true;

    ball_margin.rel.radius = large_r;
    ball_margin.use_intersects_area = false;
    ball_margin.set_color( c_ball );
    ball_margin.filled = false;

    line_until_ball_stops.use_intersects_area = false;
    line_until_ball_stops.set_color( c_line_until_ball_stops );

    line_markers.set_max_size( MAX_LINE_MARKERS_NUM );
    line_markers.set_cur_size( 0 );
    line_markers.use_intersects_area = false;
    line_markers.set_color( c_line_markers );

    label.set_color( c_font );
}

void
VisualBall::draw( DisplayBase * disp,
                  const Area2d & area,
                  const Frame2d & p_frame,
                  const bool chg )
{
    changed = ( changed || chg );

    ball_point.actualize( p_frame, changed );

    if ( ! ball_point.intersects_area( area ) )
    {
        return;
    }

    if ( show_vel )
    {
        line_until_ball_stops.draw( disp, area, p_frame, changed );
        line_markers.draw( disp,area,p_frame * vel_frame, changed );
    }

    ball_point.draw( disp, area, p_frame, false ); //already actualized!

    ball_circle.draw( disp, area, p_frame, changed );
    ball_margin.draw( disp, area, p_frame, changed );

    if ( show_vel_string )
    {
        label.draw( disp, area,p_frame, changed );
    }

    changed = false;
}

void
VisualBall::actualize( const Frame2d & f,
                       const bool chg )
{
    changed = ( changed || chg );

    ball_point.actualize( f, changed );
    ball_circle.actualize( f, changed );
    ball_margin.actualize( f, changed );

    line_until_ball_stops.actualize( f, changed );
    line_markers.actualize( f, changed );

    label.actualize( f, changed );

    changed = false;
}

void
VisualBall::set_show_vel( const Point2d & vel_as_point )
{
    show_vel = true;

    Vector2d vel = Vector2d( vel_as_point.x, vel_as_point.y );
    double norm = vel.norm();

    line_until_ball_stops.rel.p2.x =  vel.x / ( 1.0 - ball_decay );
    line_until_ball_stops.rel.p2.y =  vel.y / ( 1.0 - ball_decay );
    line_until_ball_stops.changed = true;

    vel_frame.set_angle( vel );

    line_markers.changed = true;
    if ( norm < 0.5 )
    {
        line_markers.set_cur_size( 0 );
        return;
    }

    double distance = 0.0;
    line_markers.set_cur_size( MAX_LINE_MARKERS_NUM );

    for ( int i = 0; i < MAX_LINE_MARKERS_NUM; ++i )
    {
        distance += norm;
        line_markers.rel.tab[i].p1.x = distance;
        line_markers.rel.tab[i].p1.y = -0.5*norm;
        line_markers.rel.tab[i].p2.x = distance;
        line_markers.rel.tab[i].p2.y =  0.5*norm;

        norm *= ball_decay;
        if ( norm < 0.5 )
        {
            line_markers.set_cur_size( i + 1 );
            break;
        }
    }
}

void
VisualBall::set_show_vel_string( const Point2d & vel )
{
    char buf[32];
    std::snprintf( buf, 32,
                   "(%.2f,%.2f)",
                   vel.x, vel.y );
    label.content = buf;

    show_vel_string = true;
    label.changed = true;
    //cout << "\nlabel = " << label.content.tab << flush;
}

/******************************************************************************/

const int VisualPlayer::MAX_LABEL_LEN = 20;

VisualPlayer::VisualPlayer()
    : c_black( 0, 0, 0 ),
      c_white( 255, 255, 255 ),
      c_red( 255, 0, 0 ),
      c_blue( 0, 0, 255 ),
      c_orange( 255, 165, 0 ),
      c_pink( 255, 192, 203 ),
      c_olive( 128, 128, 0 )
{

}

void
VisualPlayer::init( const int my_key,
                    const int my_layer,
                    const int p_number,
                    const RGBcolor & my_c_invalid,
                    const RGBcolor & my_c_player,
                    const RGBcolor & my_c_goalie,
                    const RGBcolor & my_c_font )
{
    key = my_key;
    layer = my_layer;

    c_invalid = my_c_invalid;
    c_player = my_c_player;
    c_goalie = my_c_goalie;
    c_font = my_c_font;

    type = -1;
    use_number = true;
    show_kick = false;
    show_kick_fault = false;
    show_tackle = false;
    show_tackle_fault = false;
    show_catch_fault = false;
    ball_collision = false;
    is_active = false;
    is_goalie = false;
    changed = true;

    double small_r = PlayerTypes::get_player_radius( type );
    double large_r = PlayerTypes::get_kick_radius( type );

    body_bg.rel.radius = large_r;
    body_bg.use_intersects_area = false;
    body_bg.set_color( c_invalid );
    body_bg.filled = true;

    body.rel.radius = small_r;
    body.use_intersects_area = false;
    body.set_color( c_black );

    body_margin.rel.radius = large_r;
    body_margin.use_intersects_area = false;
    body_margin.set_color( c_black );

    body_margin_kick.rel.radius = large_r;
    body_margin_kick.use_intersects_area = false;
    body_margin_kick.set_color( c_white );

    body_margin_kick_fault.rel.radius = large_r;
    body_margin_kick_fault.use_intersects_area = false;
    body_margin_kick_fault.filled = true;
    body_margin_kick_fault.set_color( c_blue );

    body_margin_tackle.rel.radius = large_r;
    body_margin_tackle.use_intersects_area = false;
    body_margin_tackle.filled = true;
    body_margin_tackle.set_color( c_pink );

    body_margin_tackle_fault.rel.radius = large_r;
    body_margin_tackle_fault.use_intersects_area = false;
    body_margin_tackle_fault.filled = true;
    body_margin_tackle_fault.set_color( c_olive );

    body_margin_catch_fault.rel.radius = large_r;
    body_margin_catch_fault.use_intersects_area = false;
    body_margin_catch_fault.filled = true;
    body_margin_catch_fault.set_color( c_blue );

    body_margin_ball_collision.rel.radius = large_r;
    body_margin_ball_collision.use_intersects_area = false;
    body_margin_ball_collision.filled = true;
    body_margin_ball_collision.set_color( c_red );

    body_dir.rel.p2.x = large_r;
    body_dir.use_intersects_area = false;
    body_dir.set_color( c_black );

    head_dir.rel.p2.x = large_r;
    head_dir.use_intersects_area = false;
    head_dir.set_color( c_red );

    char unum_str[4];
    std::snprintf( unum_str, 4, "%d", p_number );
    label.content = unum_str;
    label.set_color( c_font );

    //body_frame_chg = false;
    head_frame_chg = false;
}

void
VisualPlayer::draw( DisplayBase * disp,
                    const Area2d & area,
                    const Frame2d & p_frame,
                    const bool chg )
{
    changed = ( changed || chg );

    body_bg.actualize( p_frame, changed );

    if ( ! body_bg.intersects_area( area ) )
    {
        return;
    }

    body_bg.draw( disp, area, p_frame, false ); //already actualized!

    if ( ball_collision )
    {
        body_margin_ball_collision.draw( disp, area,p_frame, changed );
    }

    if ( show_kick_fault )
    {
        body_margin_kick_fault.draw( disp, area, p_frame, changed );
    }
    else if ( show_kick )
    {
        body_margin_kick.draw( disp, area, p_frame, changed );
    }
    else if ( show_catch_fault )
    {
        body_margin_catch_fault.draw( disp, area, p_frame, changed );
    }
    else if ( show_tackle_fault )
    {
        body_margin_tackle_fault.draw( disp, area, p_frame, changed );
    }
    else if ( show_tackle )
    {
        body_margin_tackle.draw( disp, area, p_frame, changed );
    }
    else
    {
        body_margin.draw( disp, area, p_frame, changed );
    }

    body.draw( disp, area, p_frame, changed );

    body_dir.draw( disp, area, p_frame, changed );

    head_dir.draw( disp, area, p_frame * head_frame, changed || head_frame_chg );
    head_frame_chg = false;

    if ( use_number )
    {
        //label.draw( disp,area,p_frame * body_frame,changed || body_frame_chg );
        label.draw( disp, area, p_frame * reverse_body_frame, changed );
        //body_frame_chg = false;
    }

    changed = false;
}

void
VisualPlayer::actualize( const Frame2d & f,
                         const bool chg )
{
    changed = ( changed || chg );

    body_bg.actualize( f, changed );
    body.actualize( f, changed );
    body_margin.actualize( f, changed );

    body_dir.actualize( f, changed );

    head_dir.actualize( f * head_frame, changed || head_frame_chg ); //must be changed
    head_frame_chg = false;

    label.actualize( f * reverse_body_frame, changed );
    //label.actualize( f * body_frame,changed || body_frame_chg );
    //body_frame_chg = false;

    changed = false;
}

void
VisualPlayer::set_type( const int t )
{
    if ( type == t )
    {
        return;
    }

    //cout << "\n type " << type << "->" << t;

    type = t;

    double small_r = PlayerTypes::get_player_radius( type );
    double large_r = PlayerTypes::get_kick_radius( type );

    body_bg.rel.radius = large_r;
    body_bg.changed = true;

    body.rel.radius = small_r;
    body.changed = true;

    body_margin.rel.radius = large_r;
    body_margin.changed = true;

    body_margin_kick.rel.radius = large_r;
    body_margin_kick.changed = true;

    body_margin_kick_fault.rel.radius = large_r;
    body_margin_kick_fault.changed = true;

    body_margin_tackle.rel.radius = large_r;
    body_margin_tackle.changed = true;

    body_margin_tackle_fault.rel.radius = large_r;
    body_margin_tackle_fault.changed = true;

    body_margin_catch_fault.rel.radius = large_r;
    body_margin_catch_fault.changed = true;

    body_dir.rel.p2.x = large_r;
    body_dir.changed = true;

    head_dir.rel.p2.x = large_r;
    head_dir.changed = true;
}

void
VisualPlayer::set_body_angle( const Angle & a )
{
    //body_frame_chg = true;
    reverse_body_frame.set_angle( -a );
}

void
VisualPlayer::set_head_angle( const Angle & a )
{
    head_frame_chg = true;
    head_frame.set_angle( a );
}

void
VisualPlayer::set_label( const char * lab )
{

    label.content.erase();

    if ( ! lab || *lab == '\0' )
    {
        return;
    }

    label.content.assign( lab,
                          std::min( std::strlen( lab ),
                                    size_t( MAX_LABEL_LEN ) ) );
}

void
VisualPlayer::set_active( bool flag )
{
    if ( flag == is_active )
    {
        return;
    }

    is_active = flag;

    if ( !is_active )
    {
        body_bg.set_color( c_invalid );
        return;
    }

    is_goalie = false;

    if ( is_goalie )
    {
        body_bg.set_color( c_goalie );
    }
    else
    {
        body_bg.set_color( c_player );
    }
}

void
VisualPlayer::set_goalie( bool flag )
{
    if ( flag == is_goalie )
    {
        return;
    }

    is_goalie = flag;

    if ( ! is_active )
    {
        return;
    }

    if ( is_goalie )
    {
        body_bg.set_color( c_goalie );
    }
    else
    {
        body_bg.set_color( c_player );
    }
}

/******************************************************************************/

const double VisualField::FIELD_LENGTH                = 105.0;
const double VisualField::FIELD_WIDTH                 = 68.0;
//const double VisualField::FIELD_MARGIN                = 5.0;
const double VisualField::FIELD_CENTER_CIRCLE_R       = 9.15;
const double VisualField::FIELD_PENALTY_AREA_LENGTH   = 16.5;
const double VisualField::FIELD_PENALTY_AREA_WIDTH    = 40.32;
const double VisualField::FIELD_GOAL_AREA_LENGTH      = 5.5;
const double VisualField::FIELD_GOAL_AREA_WIDTH       = 18.32;
const double VisualField::FIELD_GOAL_WIDTH            = 14.02;
const double VisualField::FIELD_GOAL_DEPTH            = 2.44;
const double VisualField::FIELD_PENALTY_SPOT_DIST     = 11.0;
const double VisualField::FIELD_CORNER_ARC_R          = 1.0;
//const double FIELD_CORNER_KICK_MARGIN    = 1.0;

const double VisualField::HLEN = VisualField::FIELD_LENGTH / 2.0;
const double VisualField::HWID = VisualField::FIELD_WIDTH / 2.0;

const double VisualField::PEN_X = HLEN - FIELD_PENALTY_AREA_LENGTH;
const double VisualField::PEN_Y = FIELD_PENALTY_AREA_WIDTH / 2.0;
const double VisualField::GOAL_X = HLEN - FIELD_GOAL_AREA_LENGTH;
const double VisualField::GOAL_Y = FIELD_GOAL_AREA_WIDTH / 2.0;

const double VisualField::GPOST_RAD = 0.06;


VisualField::VisualField()
{

}

void VisualField::init( const int my_key,
                        const int my_layer,
                        const RGBcolor & my_c_line,
                        const RGBcolor & my_c_goal,
                        const bool my_keepaway,
                        const double & my_keepaway_length,
                        const double & my_keepaway_width )
{
    key = my_key;
    layer = my_layer;

    c_line = my_c_line;
    c_goal = my_c_goal;
    c_black = RGBcolor( 0,0,0 );
    c_white = RGBcolor( 255,255,255 );
    c_red =   RGBcolor( 255,0,0 );

    keepaway = my_keepaway;
    keepaway_length = my_keepaway_length;
    keepaway_width = my_keepaway_width;

    changed = true;

    //penalty points
    Point2d pp[4];
    pp[0] = Point2d( -HLEN+FIELD_PENALTY_SPOT_DIST,0 );
    pp[1] = Point2d( HLEN-FIELD_PENALTY_SPOT_DIST,0 );
    points.set_points( 2,pp );
    points.set_color( c_line );
    points.use_intersects_area = false;

    //lines
    Line2d ll[20];
    //field lines
    ll[0] = Line2d( Point2d( -HLEN,-HWID ),Point2d( HLEN,-HWID ) );
    ll[1] = Line2d( Point2d( HLEN,-HWID ),Point2d( HLEN,+HWID ) );
    ll[2] = Line2d( Point2d( HLEN,+HWID ),Point2d( -HLEN,+HWID ) );
    ll[3] = Line2d( Point2d( -HLEN,+HWID ),Point2d( -HLEN,-HWID ) );
    //middle line
    if ( !keepaway )
        ll[4] = Line2d( Point2d(    0,-HWID ),Point2d(    0,+HWID ) );
    //left penalty area
    ll[5] = Line2d( Point2d( -HLEN,-PEN_Y ),Point2d( -PEN_X,-PEN_Y ) );
    ll[6] = Line2d( Point2d( -PEN_X,-PEN_Y ),Point2d( -PEN_X, PEN_Y ) );
    ll[7] = Line2d( Point2d( -PEN_X, PEN_Y ),Point2d( -HLEN, PEN_Y ) );
    //left goal area
    ll[8] = Line2d( Point2d( -HLEN,-GOAL_Y ),Point2d( -GOAL_X,-GOAL_Y ) );
    ll[9] = Line2d( Point2d( -GOAL_X,-GOAL_Y ),Point2d( -GOAL_X, GOAL_Y ) );
    ll[10] = Line2d( Point2d( -GOAL_X, GOAL_Y ),Point2d( -HLEN, GOAL_Y ) );
    //right penalty area
    ll[11] = Line2d( Point2d( HLEN,-PEN_Y ),Point2d( PEN_X,-PEN_Y ) );
    ll[12] = Line2d( Point2d( PEN_X,-PEN_Y ),Point2d( PEN_X, PEN_Y ) );
    ll[13] = Line2d( Point2d( PEN_X, PEN_Y ),Point2d( HLEN, PEN_Y ) );
    //right goal area
    ll[14] = Line2d( Point2d( HLEN,-GOAL_Y ),Point2d( GOAL_X,-GOAL_Y ) );
    ll[15] = Line2d( Point2d( GOAL_X,-GOAL_Y ),Point2d( GOAL_X, GOAL_Y ) );
    ll[16] = Line2d( Point2d( GOAL_X, GOAL_Y ),Point2d( HLEN, GOAL_Y ) );

    lines.set_lines( 17,ll );
    lines.set_color( c_line );
    lines.use_intersects_area = false;

    //cirlces
    CircleArc2d ca[5];
    //center circle
    if ( !keepaway )
        ca[0] = CircleArc2d( Point2d( 0.0,0.0 ),FIELD_CENTER_CIRCLE_R,0.0,2.0*M_PI );
    //edge circlearcs
    ca[1] = CircleArc2d( Point2d( -HLEN,-HWID ),FIELD_CORNER_ARC_R,0.0,0.5*M_PI );
    ca[2] = CircleArc2d( Point2d( HLEN,-HWID ),FIELD_CORNER_ARC_R,0.5*M_PI,M_PI );
    ca[3] = CircleArc2d( Point2d( HLEN, HWID ),FIELD_CORNER_ARC_R,M_PI,1.5*M_PI );
    ca[4] = CircleArc2d( Point2d( -HLEN, HWID ),FIELD_CORNER_ARC_R,1.5*M_PI,2.0*M_PI );
    circlearcs.set_circlearcs( 5,ca );
    circlearcs.set_color( c_line );
    circlearcs.use_intersects_area = false;

    //keepaway region
    pp[0] = Point2d( keepaway_length*0.5,-keepaway_width*0.5 );
    pp[1] = Point2d( -keepaway_length*0.5,-keepaway_width*0.5 );
    pp[2] = Point2d( -keepaway_length*0.5,keepaway_width*0.5 );
    pp[3] = Point2d( keepaway_length*0.5,keepaway_width*0.5 );
    keep_area.set_points( 4,pp );
    keep_area.set_color( c_line );
    keep_area.filled = false;
    keep_area.use_intersects_area = false;

    //left goal
    pp[0] = Point2d( -HLEN,-FIELD_GOAL_WIDTH*0.5 - GPOST_RAD );
    pp[1] = Point2d( -( HLEN+ FIELD_GOAL_DEPTH ),-FIELD_GOAL_WIDTH*0.5 - GPOST_RAD );
    pp[2] = Point2d( -( HLEN+ FIELD_GOAL_DEPTH ),+FIELD_GOAL_WIDTH*0.5 + GPOST_RAD );
    pp[3] = Point2d( -HLEN,FIELD_GOAL_WIDTH*0.5 + GPOST_RAD );
    goal_l.set_points( 4,pp );
    goal_l.set_color( c_goal );
    goal_l.filled = true;
    goal_l.use_intersects_area = false;

    //right goal
    pp[0] = Point2d( HLEN,-FIELD_GOAL_WIDTH*0.5 - GPOST_RAD );
    pp[1] = Point2d( HLEN+ FIELD_GOAL_DEPTH,-FIELD_GOAL_WIDTH*0.5 - GPOST_RAD );
    pp[2] = Point2d( HLEN+ FIELD_GOAL_DEPTH,+FIELD_GOAL_WIDTH*0.5 + GPOST_RAD );
    pp[3] = Point2d( HLEN,FIELD_GOAL_WIDTH*0.5 + GPOST_RAD );
    goal_r.set_points( 4,pp );
    goal_r.set_color( c_goal );
    goal_r.filled = true;
    goal_r.use_intersects_area = false;

    //vertical stripe
    pp[0] = Point2d( 0,-FIELD_WIDTH*0.5 );
    pp[1] = Point2d( 20, -FIELD_WIDTH*0.5 );
    pp[2] = Point2d( 20,  FIELD_WIDTH*0.5 );
    pp[3] = Point2d( 0,  FIELD_WIDTH*0.5 );
    vertical_stripe.set_points( 4,pp );
    vertical_stripe.set_color( c_red );
    vertical_stripe.filled = true;
    vertical_stripe.use_intersects_area = false;
    use_vertical_stripe( false );

    Circle2d gpost_circles[ 4 ];
    gpost_circles[ 0 ] = Circle2d( Point2d( HLEN-GPOST_RAD,
                                            FIELD_GOAL_WIDTH*0.5 + GPOST_RAD ),
                                   GPOST_RAD,
                                   2 );
    gpost_circles[ 1 ] = Circle2d( Point2d( HLEN-GPOST_RAD,
                                            -FIELD_GOAL_WIDTH*0.5 - GPOST_RAD ),
                                   GPOST_RAD,
                                   2 );
    gpost_circles[ 2 ] = Circle2d( Point2d( -HLEN+GPOST_RAD,
                                            FIELD_GOAL_WIDTH*0.5 + GPOST_RAD ),
                                   GPOST_RAD,
                                   2 );
    gpost_circles[ 3 ] = Circle2d( Point2d( -HLEN+GPOST_RAD,
                                            -FIELD_GOAL_WIDTH*0.5 - GPOST_RAD ),
                                   GPOST_RAD,
                                   2 );
    goal_posts.set_circles( 4, gpost_circles );
    goal_posts.set_color( c_goal );
    goal_posts.filled = true;
    goal_posts.use_intersects_area = false;
}

void VisualField::draw( DisplayBase * disp,
                        const Area2d & area,
                        const Frame2d & p_frame,
                        const bool chg )
{
    changed = changed || chg;

    vertical_stripe.draw( disp,area,p_frame,changed );

    points.draw( disp,area,p_frame,changed );
    goal_l.draw( disp,area,p_frame,changed );
    goal_r.draw( disp,area,p_frame,changed );
    if ( keepaway )
        keep_area.draw( disp,area,p_frame,changed );

    lines.draw( disp,area,p_frame,changed );
    circlearcs.draw( disp,area,p_frame,changed );

    goal_posts.draw( disp,area,p_frame,changed );

    changed = false;
}

void
VisualField::actualize( const Frame2d & f,
                        const bool chg )
{
    changed = ( changed || chg );

    goal_l.actualize( f,changed );
    goal_r.actualize( f,changed );
    lines.actualize( f,changed );
    circlearcs.actualize( f,changed );
    goal_posts.actualize( f,changed );

    changed = false;
}

void
VisualField::set_goal_width( const double & width )
{
    //cerr << "\nchanging goal width to " << width << flush;

    goal_l.rel.tab[0].y = -width*0.5 - GPOST_RAD;
    goal_l.rel.tab[1].y = -width*0.5 - GPOST_RAD;
    goal_l.rel.tab[2].y = +width*0.5 + GPOST_RAD;
    goal_l.rel.tab[3].y = +width*0.5 + GPOST_RAD;
    goal_l.changed = true;

    goal_r.rel.tab[0].y = -width*0.5 - GPOST_RAD;
    goal_r.rel.tab[1].y = -width*0.5 - GPOST_RAD;
    goal_r.rel.tab[2].y = +width*0.5 + GPOST_RAD;
    goal_r.rel.tab[3].y = +width*0.5 + GPOST_RAD;
    goal_r.changed = true;

    goal_posts.rel.tab[0].center.y = +width*0.5 + GPOST_RAD;
    goal_posts.rel.tab[1].center.y = -width*0.5 - GPOST_RAD;
    goal_posts.rel.tab[2].center.y = +width*0.5 + GPOST_RAD;
    goal_posts.rel.tab[3].center.y = -width*0.5 - GPOST_RAD;
    goal_posts.changed = true;
}

/******************************************************************************/

const int SMonitorDevice::frame_canvas_left = 94;
const int SMonitorDevice::frame_canvas_right = 95;
const int SMonitorDevice::frame_ball = 96;
const int SMonitorDevice::frame_varea = 97;
const int SMonitorDevice::frame_shadow = 98;

const int SMonitorDevice::BUTTON_START = 0;
const int SMonitorDevice::BUTTON_RECONNECT = 1;
const int SMonitorDevice::BUTTON_SCALE_LEVEL = 2;
const int SMonitorDevice::BUTTON_INFO_LEVEL = 3;
const int SMonitorDevice::BUTTON_MODE = 4;
const int SMonitorDevice::BUTTON_UNZOOM = 5;
const int SMonitorDevice::BUTTON_QUIT = 6;
const int SMonitorDevice::BUTTON_NUMBER = 7;

const int SMonitorDevice::POPUP_BUTTON_DROP_BALL = 0;
const int SMonitorDevice::POPUP_BUTTON_FREE_KICK_LEFT = 1;
const int SMonitorDevice::POPUP_BUTTON_FREE_KICK_RIGHT = 2;
const int SMonitorDevice::POPUP_BUTTON_NUMBER = 3;


const int SMonitorDevice::Options::info_level_min = 0;
const int SMonitorDevice::Options::info_level_def = 1;
const int SMonitorDevice::Options::info_level_max = 3;

const int SMonitorDevice::Options::scale_level_min = 0;
const int SMonitorDevice::Options::scale_level_def = 0;
const int SMonitorDevice::Options::scale_level_max = 5;
const double SMonitorDevice::Options::scale_level_inc = 0.2;
const double SMonitorDevice::Options::scale_level_beg = 1.0;

const int SMonitorDevice::Options::MODE_STANDARD = 0;
const int SMonitorDevice::Options::MODE_MOVE = 1;
const int SMonitorDevice::Options::MODE_SHOW_VIEW_AREA = 2;


const int SMonitorDevice::id_ball = -1;
const int SMonitorDevice::id_invalid = -2;


SMonitorDevice::Options::Options()
{
    server_port = 6000;
    std::strcpy( server_host, "127.0.0.1" );
    coach_port = server_port+1;
    protocol_version = 2;
    connect_on_start = true;

    pen_taken_wait = 200;

    keepaway = false;
    keepaway_length = 20.0;
    keepaway_width = 20.0;

    list_player_types = false;
    show_ball_collisions = false;
    just_edit = false;
    track_ball = false;
    show_ball_vel = false;

    player_radius = 0.3;
    player_skin = 1;
    player_num_pos.x = 1.2;
    player_num_pos.y = 0.0;
    kick_radius = 1.085;
    ball_radius = 0.3;//0.085;
    ball_skin = 1;


    c_team_l = RGBcolor( 255,255,0 );
    c_goalie_l = RGBcolor( 0,255,0 );
    c_font_l = RGBcolor( 255,255,255 );//( 255,0,0 );
    c_invalid_l = RGBcolor( 0,0,0 );

    c_team_r = RGBcolor( 0,255,255 );
    c_goalie_r = RGBcolor( 255,153,255 );
    c_font_r = RGBcolor( 255,255,255 ); //( 0,0,139 );
    c_invalid_r = RGBcolor( 0,0,0 );


    c_ball = RGBcolor( 255,255,255 );
    c_field = RGBcolor( 0,153,0 );//( 153,204,51 ); //( 204,204,204 );
    c_marigin = RGBcolor( 204,51,51 );
    c_line = RGBcolor( 255,255,255 );
    c_goal = RGBcolor( 0,0,0 );

    c_varea_exact = RGBcolor( 0,180,0 ); // 0, 0xB4, 0
    c_varea_fuzzy = RGBcolor( 0,170,0 ); // 0, 0xAA, 0

    info_level = info_level_def;
    scale_level = scale_level_def;

    mode = MODE_STANDARD;
    //mode = MODE_SHOW_VIEW_AREA;
    active_in_mode = id_invalid;

    lt_show_spare_space_to_offside_line = false;
    rt_show_spare_space_to_offside_line = false;
}

double
SMonitorDevice::Options::scale_factor() const
{
    return scale_level_beg + scale_level * scale_level_inc;
}

bool
SMonitorDevice::Options::set_scale_level_from_double( const double & s )
{
    for ( int i = scale_level_max; i >= 0; --i )
    {
        if ( s >= scale_level_beg + i * scale_level_inc - 0.0001 )
        {
            scale_level = i;
            return true;
        }
    }

    scale_level = scale_level_min;
    return true;
}

const char *
SMonitorDevice::Options::get_mode_string() const
{
    if ( mode == MODE_STANDARD )
        return "mode std ";
    if ( mode == MODE_MOVE )
        return "mode move";
    if ( mode == MODE_SHOW_VIEW_AREA )
        return "mode view";
    return "mode ????";
}

const char *
SMonitorDevice::Options::get_mode_value_string() const
{
    if ( mode == MODE_STANDARD )
        return "std";
    if ( mode == MODE_MOVE )
        return "move";
    if ( mode == MODE_SHOW_VIEW_AREA )
        return "view";
    return "????";
}

bool
SMonitorDevice::Options::set_mode_from_string( const char * s )
{
    if ( ! std::strcmp( "std", s ) )
        mode = MODE_STANDARD;
    else if ( ! std::strcmp( "move", s ) )
        mode = MODE_MOVE;
    else if ( ! std::strcmp( "view", s ) )
        mode = MODE_SHOW_VIEW_AREA;
    else
        return false;
    return true;
}


SMonitorDevice::ServerState::ServerState()
{
    reset();
}

void
SMonitorDevice::ServerState::reset()
{
    current_time_ = 0;
    pen_taken_timer_ = 0;
    playmode_ = 0;
    playmode_string_.erase();
    left_teamname_.erase();
    right_teamname_.erase();
    reconnected_ = true;
}

void
SMonitorDevice::Positions::set_player( const int i,
                                       const Point2d & p,
                                       const Angle & a )
{
    player[i].pos = p;
    player[i].body_angle = a;
}

bool
SMonitorDevice::Positions::max_x_pos( const int p1,
                                      const int p2,
                                      double & x1 ) const
{
    bool res = false;
    for ( int i =p1; i < p2; ++i )
    {
        const Player & p = player[i];
        if ( ! p.alive )
            continue;

        if ( ! res )
        {
            x1 = p.pos.x;
            res = true;
        }
        else if ( p.pos.x > x1 )
        {
            x1 = p.pos.x;
        }
    }
    return res;
}

//copy of max_x_pos with > replaced by <
bool
SMonitorDevice::Positions::min_x_pos( const int p1,
                                      const int p2,
                                      double & x1 ) const
{
    bool res = false;
    for ( int i =p1; i < p2; ++i )
    {
        const Player & p = player[i];
        if ( ! p.alive )
            continue;

        if ( ! res )
        {
            x1 = p.pos.x;
            res = true;
        }
        else if ( p.pos.x < x1 )
        {
            x1 = p.pos.x;
        }
    }
    return res;
}

bool
SMonitorDevice::Positions::max_x_pos( const int p1,
                                      const int p2,
                                      double & x1,
                                      double & x2 ) const
{
    int res = 0;
    for ( int i =p1; i < p2; ++i )
    {
        const Player & p = player[i];

        if ( !p.alive )
            continue;

        if ( res == 0 )
        {
            x1 = p.pos.x;
            res = 1;
        }
        else if ( res == 1 )
        {
            res = 2;
            if ( p.pos.x > x1 )
            {
                x2 = x1;
                x1 = p.pos.x;
            }
            else
            {
                x2 = p.pos.x;
            }
        }
        else if ( p.pos.x > x2 )
        {
            if ( p.pos.x > x1 )
            {
                x2 = x1;
                x1 = p.pos.x;
            }
            else
            {
                x2 = p.pos.x;
            }
        }
    }
    return ( res ==2 );
}

//copy of max_x_pos with > replaced by <
bool
SMonitorDevice::Positions::min_x_pos( const int p1,
                                      const int p2,
                                      double & x1,
                                      double & x2 ) const
{
    int res = 0;
    for ( int i =p1; i < p2; i++ ) {
        const Player & p = player[i];

        if ( !p.alive )
            continue;

        if ( res == 0 ) {
            x1 = p.pos.x;
            res = 1;
        }
        else if ( res == 1 ) {
            res = 2;
            if ( p.pos.x < x1 ) {
                x2 = x1;
                x1 = p.pos.x;
            }
            else
                x2 = p.pos.x;
        }
        else if ( p.pos.x < x2 ) {
            if ( p.pos.x < x1 ) {
                x2 = x1;
                x1 = p.pos.x;
            }
            else
                x2 = p.pos.x;
        }
    }
    return ( res ==2 );
}

std::ostream &
SMonitorDevice::Positions::print_inline( std::ostream & out ) const
{
    int prec = 4;
    out << std::setprecision( prec ) << ball.pos.x
        << " " << std::setprecision( prec ) << ball.pos.y
        << " " << std::setprecision( prec ) << ball.vel.x
        << " " << std::setprecision( prec ) << ball.vel.y;
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        out << "    " << std::setprecision( prec ) << player[i].pos.x
            << " " << std::setprecision( prec ) << player[i].pos.y
            << " " << std::setprecision( prec ) << player[i].body_angle.get_value()
            << " 0 0"; // the velocity is 0
    }
    return out;
}

std::ostream &
SMonitorDevice::Positions::print_formation( std::ostream & out,
                                            bool left_half,
                                            bool left_team ) const
{
    int start_idx = 0;
    double dum = 0.0;
    if ( left_half )
    {
        out << "\n[left half";
        dum = 52.5;
    }
    else
    {
        out << "\n[right half";
    }

    if ( left_team )
    {
        out << ", left team]";
    }
    else
    {
        out << ", right team]";
        start_idx = 11;
    }

    char buffer[200];
    for ( int i = 1+start_idx; i< MAX_PLAYER+start_idx; ++i )
    {
        float x = ( player[i].pos.x + dum )/52.5;
        if ( x<0.0 ) x = 0.0;
        if ( x>1.0 ) x = 1.0;
        float y = ( player[i].pos.y + 34.0 )/68.0;
        if ( y<0.0 ) y = 0.0;
        if ( y>1.0 ) y = 1.0;
        std::snprintf( buffer, 200,
                       "\npos_%-2d = %.2f %.2f *",
                       i - start_idx, x, y  );
        out << buffer;
    }

    return out << std::flush;
}
/*****************************************************************************/
/*****************************************************************************/
inline
double
x_SERVER_2_LP( const double & x )
{
    return x;
}

inline
double
y_SERVER_2_LP( const double & y )
{
    return -y;
}

double ang_SERVER_2_LP( double a ) {
    a = ( a*PI )/180.0;
    if ( a < 0.0 ) return -a;
    return -a+2*PI;
}
/*****************************************************************************/
/*****************************************************************************/

SMonitorDevice::SMonitorDevice()
    : M_timeover( false )
{
    server_state.reset();
    //init_connection(); //vorlaeufig
}

void SMonitorDevice::generic_description_of_options( std::ostream & o, int mode ) const {
    print_option_entry( o,mode,"port",options.server_port,"defines the socket port for communication" );
    print_option_entry( o,mode,"host",options.server_host,"defines the host on which the soccer server is running" );
    print_option_entry( o,mode,"version",options.protocol_version,"version of monitor <-> server protocol" );
    print_option_entry( o,mode,"connect_on_start",options.connect_on_start,"if set then a connection to the server is established during the start" );
    print_option_entry( o,mode, "pen_taken_wait", options.pen_taken_wait, "set the default penalty taken wait cycles" );
    print_option_entry( o,mode,"keepaway",options.keepaway,"if set then the field will be laid out for a keepaway match" );
    print_option_entry( o,mode,"keepaway_length",options.keepaway_length,"length of the keepaway region" );
    print_option_entry( o,mode,"keepaway_width",options.keepaway_width,"width of the keepaway region" );
    print_option_entry( o,mode,"list_player_types",options.list_player_types,"print all player types after a connection" );
    print_option_entry( o,mode,"show_ball_collisions",options.show_ball_collisions,"show if a player collides with the  ball" );
    print_option_entry( o,mode,"just_edit",options.just_edit,"don't connect to the soccer server" );
    print_option_entry( o,mode,"scale",options.scale_factor(),"factor all player's and the ball sizes are multiplied by" );
    print_option_entry( o,mode,"detail",options.info_level,"level of player information such as his number, stamina and type" );
    print_option_entry( o,mode,"mode",options.get_mode_value_string(),"initial mode to use" );
    print_option_entry( o,mode,"player_radius",options.player_radius,"default inner player radius ( if server doesn't provide type info )" );
    print_option_entry( o,mode,"kick_radius",options.kick_radius,"default outer player radius ( if server doesn't provide type info )" );
    print_option_entry( o,mode,"player_num_pos_x",options.player_num_pos.x,"x position of player number relative to player's center" );
    print_option_entry( o,mode,"player_num_pos_y",options.player_num_pos.y,"y position of player number relative to player's center" );

    print_option_entry( o,mode,"ball_radius",options.ball_radius,"define how large the inner circle of the ball should be" );
    o << "\n";
    print_option_entry( o,mode,"c_team_l",options.c_team_l,"color of left team players" );
    print_option_entry( o,mode,"c_goalie_l",options.c_goalie_l,"color of left goalie" );
    print_option_entry( o,mode,"c_font_l",options.c_font_l,"color of left team number strings" );
    print_option_entry( o,mode,"c_team_r",options.c_team_r,"color of right team players" );
    print_option_entry( o,mode,"c_goalie_r",options.c_goalie_r,"color of right goalie" );
    print_option_entry( o,mode,"c_font_r",options.c_font_r,"color of right team number strings" );
    print_option_entry( o,mode,"c_ball",options.c_ball,"color of the ball" );
    print_option_entry( o,mode,"c_field",options.c_field,"color of the soccer field" );
    print_option_entry( o,mode,"c_line",options.c_line,"color of the soccer field lines" );
    print_option_entry( o,mode,"c_goal",options.c_goal,"color of the soccer field goals" );
    print_option_entry( o,mode,"c_varea_exact",options.c_varea_exact,"color of the exact view area" );
    print_option_entry( o,mode,"c_varea_fuzzy",options.c_varea_fuzzy,"color of the fuzzy view area" );
    o << "\n";
}

bool SMonitorDevice::process_options( const ValueParser & vp ) {
    double tmp_double;
    int res;

    vp.get( "port", options.server_port );
    vp.get( "host", options.server_host,STRING_MAX_SIZE );

#if USE_COACH
    vp.get( "coach_port", options.coach_port );
#endif
    vp.get( "version", options.protocol_version );
    vp.get( "connect_on_start",options.connect_on_start );

    vp.get( "pen_taken_wait", options.pen_taken_wait );

    vp.get( "keepaway",options.keepaway );
    vp.get( "keepaway_length",options.keepaway_length );
    vp.get( "keepaway_width",options.keepaway_width );

    vp.get( "list_player_types",options.list_player_types );
    vp.get( "show_ball_collisions",options.show_ball_collisions );
    vp.get( "just_edit",options.just_edit );

    res = vp.get( "scale",tmp_double );
    if ( res == 1 )
        options.set_scale_level_from_double( tmp_double );

    vp.get( "detail", options.info_level );
    if ( options.info_level < options.info_level_min )
        options.info_level = options.info_level_min;
    else if ( options.info_level > options.info_level_max )
        options.info_level = options.info_level_max;

    char tmp_str[MAX_NAME_LEN];
    res = vp.get( "mode",tmp_str,MAX_NAME_LEN );
    if ( res > 0 ) {
        std::cout << "\nsetting mode [" << tmp_str << "]";
        options.set_mode_from_string( tmp_str );
        std::cout << "\nnew mode: " << options.get_mode_string() << std::flush;
    }

    vp.get( "player_radius",options.player_radius );
    vp.get( "player_skin",options.player_skin );
    vp.get( "kick_radius",options.kick_radius );
    vp.get( "player_num_pos_x",options.player_num_pos.x );
    vp.get( "player_num_pos_y",options.player_num_pos.y );
    vp.get( "ball_radius",options.ball_radius );
    vp.get( "ball_skin",options.ball_skin );

    char rgb_str[20];
    vp.get( "c_team_l",rgb_str,20,'\0' );   AsciiProcessor::get_col( rgb_str,options.c_team_l );
    vp.get( "c_goalie_l",rgb_str,20,'\0' ); AsciiProcessor::get_col( rgb_str,options.c_goalie_l );
    vp.get( "c_font_l",rgb_str,20,'\0' );   AsciiProcessor::get_col( rgb_str,options.c_font_l );

    vp.get( "c_team_r",rgb_str,20,'\0' );   AsciiProcessor::get_col( rgb_str,options.c_team_r );
    vp.get( "c_goalie_r",rgb_str,20,'\0' ); AsciiProcessor::get_col( rgb_str,options.c_goalie_r );
    vp.get( "c_font_r",rgb_str,20,'\0' );   AsciiProcessor::get_col( rgb_str,options.c_font_r );

    vp.get( "c_ball",rgb_str,20,'\0' );     AsciiProcessor::get_col( rgb_str,options.c_ball );
    vp.get( "c_field",rgb_str,20,'\0' );    AsciiProcessor::get_col( rgb_str,options.c_field );
    vp.get( "c_marigin",rgb_str,20,'\0' );  AsciiProcessor::get_col( rgb_str,options.c_marigin );
    vp.get( "c_line",rgb_str,20,'\0' );     AsciiProcessor::get_col( rgb_str,options.c_line );
    vp.get( "c_goal",rgb_str,20,'\0' );     AsciiProcessor::get_col( rgb_str,options.c_goal );

    vp.get( "c_varea_exact",rgb_str,20,'\0' ); AsciiProcessor::get_col( rgb_str,options.c_varea_exact );
    vp.get( "c_varea_fuzzy",rgb_str,20,'\0' ); AsciiProcessor::get_col( rgb_str,options.c_varea_fuzzy );

    if ( vp.num_of_not_accessed_entries() ) {
        ERROR_OUT << "\nSMonitorDevice: not recognized options:";
        vp.show_not_accessed_entries( ERROR_STREAM );
        return false;
    }

    return true;
}

bool SMonitorDevice::process_options( const char * fname ) {
    ValueParser vp( fname,"SMonitorDevice" );
    return process_options( vp );
}

bool SMonitorDevice::process_options( int argc, char const* const* argv ) {
    ValueParser vp( argc,argv,"m_" );
    return process_options( vp );
}

void SMonitorDevice:: help_char_command( std::ostream & o ) const {
    o << "\n"
      << "server commands:\n"
      << "b = drop ball at current mouse pointer position\n"
      << "c = connect to server\n"
      << "l = free kick for left team\n"
      << "r = free kick for right team\n"
      << "s = start a match\n"
      << "\n"
      << "monitor commands:\n"
      << "m = toggle move/view mode\n"
      << "p = print current positions on cout\n"
      << "P = print current formation ( for left or right )\n"
      << "v = toggle displaying future ball position and velocity"
        //      << "T = toggle tracking of the ball\n"
      << "\n"
      << "coach commands:\n"
      << "t = toggle current team ( starts with left team )\n"
      << "i = position player ( of the current team ) with number i at mouse pointer position\n"
      << "    i can take following values: 1,2,3,4,5,6,7,8,9\n"
      << "                                 A or 0 which means 10\n"
      << "		                 B which means 11\n";
}

void
SMonitorDevice::help_options( std::ostream & o ) const
{
    o << "\nsoccer monitor options:"
      << "\n"
      << "\n( all colors are define by their rrggbb values )";

    generic_description_of_options( o,1 );
}

void
SMonitorDevice::generate_file_options( std::ostream & o ) const
{
    o << "\n[SMonitorDevice]"
      << "\n"
      << "\n### all options can be used on the command line if prefixed with '-m_'"
      << "\n### for example '-m_host localhost' will specify the host of the soccer server"
      << "\n"
      << "\n#color options: all colors are define by their rrggbb values"
      << "\n";
    generic_description_of_options( o,0 );
}

bool
SMonitorDevice::process_char_command( BuilderBase * build,
                                      MenuBase * menu,
                                      const InputEvent & event )
{
    bool redraw = false;
    //  static char buf[100];
    //  ostrstream dum_str( buf,100 );
    switch ( event.key ) {
    case 'c':
        if ( options.just_edit )
        {
            std::cout << "\nthis key is not set in edit mode";
            break;
        }
        server.init_serv_addr( options.server_host,options.server_port );
        send_dispinit();
        server_state.reset();
        M_score_board_string.erase();
        break;
    case 'l':
        if ( options.just_edit )
        {
            std::cout << "\nthis key is not set in edit mode";
            break;
        }
        send_dispfoul_left( event.pos );
        break;
#if 1
    case 'm':
        options.active_in_mode = id_invalid;
        if ( options.mode == Options::MODE_STANDARD )
        {
            options.mode = Options::MODE_MOVE;
        }
        else if ( options.mode == Options::MODE_MOVE )
        {
            build->set_cmd_set_frame_visible( frame_shadow,0 );
            options.mode = Options::MODE_SHOW_VIEW_AREA;
        }
        else if ( options.mode == Options::MODE_SHOW_VIEW_AREA )
        {
            build->set_cmd_set_frame_visible( frame_varea,0 );
            options.mode = Options::MODE_STANDARD;
        }
        menu->set_button_label( BUTTON_MODE,options.get_mode_string() );
        return true;
        break;
#endif
    case 'p':
        std::cout << "\n";
        server_pos.print_inline( std::cout );
        std::cout << std::flush;
        break;
    case 'P':
        std::cout << "\n";
        server_pos.print_formation( std::cout,coach_state.left_team, coach_state.left_team );
        std::cout << std::flush;
        break;
    case 'T':
        //options.toggle_track_ball();
        break;
    case 'r':
        if ( options.just_edit )
        {
            std::cout << "\nthis key is not set in edit mode";
            break;
        }
        send_dispfoul_right( event.pos );
        break;
    case 's':
        if ( options.just_edit )
        {
            std::cout << "\nthis key is not set in edit mode";
            break;
        }
        //server.init_serv_addr( options.server_host,options.server_port );
        send_dispstart();
        break;
    case 'v':
        options.toggle_show_ball_vel();
        if ( options.show_ball_vel )
        {
            vis_ball.set_show_vel( server_pos.ball.vel );
        }
        else
        {
            vis_ball.unset_show_vel();
        }
        return true;
        break;
    case '>':
        break; // just a debugging feature
        if ( options.lt_show_spare_space_to_offside_line )
        {
            options.lt_show_spare_space_to_offside_line = false;
        }
        else
        {
            options.lt_show_spare_space_to_offside_line = true;
            options.rt_show_spare_space_to_offside_line = false;
        }
        break;
    case '<':
        break; //just a debugging feature
        if ( options.rt_show_spare_space_to_offside_line )
        {
            options.rt_show_spare_space_to_offside_line = false;
        }
        else
        {
            options.rt_show_spare_space_to_offside_line = true;
            options.lt_show_spare_space_to_offside_line = false;
        }
        break;
    }

    int o_id = id_invalid;
    if ( 'b' == event.key ) o_id = id_ball;
    else if ( 'A' == event.key || '0' == event.key ) o_id = 9;
    else if ( 'B' == event.key ) o_id = 10;
    else o_id = int( event.key )-49;
    if ( p_valid( o_id ) && !coach_state.left_team ) o_id += 11;

    if ( options.mode == Options::MODE_MOVE && o_valid( o_id )
         || id_ball == o_id )
    {
        if ( options.just_edit )
        {
            set_object_pos( build,o_id,event.pos );
            return true;
        }
        else
        {
#if 1 // if there is no connection to the server, this is needed
            send_object_pos( o_id,event.pos );
            set_object_pos( build,o_id,event.pos );
            return true;
#else
            send_object_pos( o_id,event.pos );
            return false; //don't redraw
#endif
        }
    }

    if ( options.mode == Options::MODE_SHOW_VIEW_AREA
         && p_valid( o_id ) )
    {
        if ( options.active_in_mode == o_id )
        {
            options.active_in_mode = id_invalid;
            build->set_cmd_set_frame_visible( frame_varea,0 );
        }
        else
        {
            options.active_in_mode = o_id;
            Positions::Player & p = server_pos.ref_player( options.active_in_mode );
            build->set_cmd_set_frame_pos_ang( frame_varea,p.pos,p.body_angle+p.head_angle_rel );
            vis_view_area.set_view_mode( p.view_quality, p.view_width );
            build->set_cmd_set_frame_visible( frame_varea,1 );
        }
        return true; //redraw
    }

    switch ( event.key ) {
    case 't': coach_state.toggle_team(); break;
    case 'e':
#if 0
        if ( options.just_edit )
        {
            std::cout << "\nthis key is not set in edit mode";
            break;
        }
        coach.send_msg( "(ear on)",9 );
#else
        std::cout << "\nthis key is not set anymore";
#endif
        break;
        //case 'm': COACH::sock.send_msg( "(change_mode play_on)",22 );
    }
    return redraw;
}

bool
SMonitorDevice::process_mouse_button_event( BuilderBase * build,
                                            const InputEvent & event )
{
    //options.active_in_mode = id_invalid;
    static unsigned long last_time = 0;
    //if ( event.mouse_button ! = 2 )
    //  return false;

    if ( event.mouse_button == 2
         && event.mouse_button_state == InputEvent::MOUSE_BUTTON_PRESSED )
    {
        Point2d rel;
        double min = 100.0;
        int p_min = id_invalid;
        for ( int i = 0; i < MAX_PLAYER*2; ++i )
        {
            rel.x = event.pos.x - server_pos.player[i].pos.x;
            rel.y = event.pos.y - server_pos.player[i].pos.y;

            //take the greater absolute value
            double dum = ( std::fabs( rel.x ) > fabs( rel.y )
                           ? std::fabs( rel.x )
                           : std::fabs( rel.y ) );

            if ( dum < min )
            {
                min = dum;
                p_min = i;
            }
        }
        rel.x = event.pos.x - server_pos.ball.pos.x;
        rel.y = event.pos.y - server_pos.ball.pos.y;

        double dum = ( std::fabs( rel.x ) > std::fabs( rel.y )
                       ? std::fabs( rel.x )
                       : std::fabs( rel.y ) );
        if ( dum < min )
        {
            min = dum;
            p_min = id_ball;
        }

        if ( min <= options.kick_radius * 1.5 * options.scale_factor() )
        {
            options.active_in_mode = p_min;
        }
        else
        {
            options.active_in_mode = id_invalid;
        }

        //std::cout << "\noptions.active_in_mode = " << options.active_in_mode << flush;
        if ( options.mode == Options::MODE_MOVE )
        {
            if ( o_valid( options.active_in_mode ) )
            {
                build->set_cmd_set_frame_pos( frame_shadow, event.pos );
                build->set_cmd_set_frame_visible( frame_shadow, 1 );
            }
        }
        else if ( options.mode == Options::MODE_SHOW_VIEW_AREA )
        {
            if ( !p_valid( options.active_in_mode ) )
            {
                build->set_cmd_set_frame_visible( frame_varea,0 );
            }
            else
            {
                Positions::Player & p = server_pos.ref_player( options.active_in_mode );
                build->set_cmd_set_frame_pos_ang( frame_varea,p.pos,p.body_angle+p.head_angle_rel );
                vis_view_area.set_view_mode( p.view_quality, p.view_width );
                build->set_cmd_set_frame_visible( frame_varea,1 );
            }
            return true; //redraw
        }
        //std::cout << "\n>>>>>>>>>>>>>>>>>>>> min = " << min << " " << options.kick_radius * 1.5;
        return false; //no need for redraw
    }

    if ( event.mouse_button_state == InputEvent::MOUSE_BUTTON_MOTION
         && options.mode == Options::MODE_MOVE )
        if ( p_valid( options.active_in_mode )
             && event.time > 75 + last_time )
        {
            build->set_cmd_set_frame_pos( p_frame( options.active_in_mode ),
                                          event.pos );
            last_time = event.time;
            return true;
        }
        else if ( o_ball( options.active_in_mode )
                  && event.time > 75 + last_time )
        {
            build->set_cmd_set_frame_pos( frame_ball, event.pos );
            last_time = event.time;
            return true;
        }
        else
            return false;

    if ( event.mouse_button == 2
         && event.mouse_button_state == InputEvent::MOUSE_BUTTON_RELEASED
         && options.mode == Options::MODE_MOVE )
    {
        if ( ! o_valid( options.active_in_mode ) )
        {
            return false;
        }

        bool res = false;
        if ( options.just_edit )
        {
            set_object_pos( build,options.active_in_mode, event.pos );
            res = true;
        }
        else
        {
#if 1
            set_object_pos( build,options.active_in_mode, event.pos ); //is needed
            res = true;                       // when no connection to a server is established
#endif
            send_object_pos( options.active_in_mode, event.pos ); //send new pos to server!
        }

        build->set_cmd_set_frame_visible( frame_shadow,0 );
        last_time = 0;
        options.active_in_mode = id_invalid;
        return res;
    }

    return false;
}

bool
SMonitorDevice::process_menu_button( BuilderBase * build,
                                     MenuBase * menu,
                                     const InputEvent & event )
{
    char buf[20];
    bool chg = false;
    //static unsigned long last_time =0;
    switch ( event.menu_button ) {
    case BUTTON_START:
        send_dispstart();
        return false;

    case BUTTON_RECONNECT:
        if ( options.just_edit )
        {
            std::cout << "\nthis key is not set in edit mode";
            break;
        }
        server.init_serv_addr( options.server_host,options.server_port );
        send_dispinit();
        server_state.reset();
        M_score_board_string.erase();
        return false;

    case BUTTON_SCALE_LEVEL:
        if ( event.mouse_button == 1 )
        {
            chg = false;
            if ( options.scale_level > options.scale_level_min )
            {
                options.scale_level--;
                chg = true;
            }
        }
        else
        {
            if ( options.scale_level < options.scale_level_max )
            {
                options.scale_level++;
                chg = true;
            }
        }

        if ( chg )
        {
            double scale = options.scale_factor();
            std::snprintf( buf, sizeof( buf ), "scale %.1f", scale );
            menu->set_button_label( BUTTON_SCALE_LEVEL,buf );
            set_all_objects_scale( build, scale );
        }
        return chg;

    case BUTTON_INFO_LEVEL:
        if ( event.mouse_button == 1 )
        {
            chg = false;
            if ( options.info_level > options.info_level_min )
            {
                options.info_level--;
                chg = true;
            }
        }
        else
        {
            if ( options.info_level < options.info_level_max )
            {
                options.info_level++;
                chg = true;
            }
        }

        if ( chg )
        {
            std::snprintf( buf, sizeof( buf ), "detail %d", options.info_level );
            vis_player_set_info_level( options.info_level );
            vis_ball_set_info_level( options.info_level );
            menu->set_button_label( BUTTON_INFO_LEVEL,buf );
        }
        return chg;

    case BUTTON_MODE:
        options.active_in_mode = id_invalid;

        chg = false;
        if ( event.mouse_button ==1 )
        {
            if ( options.mode == Options::MODE_MOVE )
            {
                build->set_cmd_set_frame_visible( frame_shadow, 0 );
                options.mode = Options::MODE_STANDARD;
                chg = true;
            }
            else if ( options.mode == Options::MODE_SHOW_VIEW_AREA )
            {
                build->set_cmd_set_frame_visible( frame_varea, 0 );
                options.mode = Options::MODE_MOVE;
                chg = true;
            }
        }
        else
        {
            if ( options.mode == Options::MODE_STANDARD )
            {
                options.mode = Options::MODE_MOVE;
                chg = true;
            }
            else if ( options.mode == Options::MODE_MOVE )
            {
                build->set_cmd_set_frame_visible( frame_shadow, 0 );
                options.mode = Options::MODE_SHOW_VIEW_AREA;
                chg = true;
            }
        }

        if ( chg )
        {
            menu->set_button_label( BUTTON_MODE,options.get_mode_string() );
        }

        return chg;
    case BUTTON_UNZOOM:
        build->set_cmd_set_view_area( initial_area );
        return true;
    case BUTTON_QUIT:
        menu->set_exit_program();
        return false;
    default:
        break;
    }

    return false;
}

bool
SMonitorDevice::process_popup_button( BuilderBase * build,
                                      MenuBase * popup,
                                      const InputEvent & event )
{
    static Point2d pos;

    //std::cout << "\npos = " << pos.x << "," << pos.y << flush;
    //std::cout << "\ninput_pos = " << event.pos.x << "," << event.pos.y << flush;

    if ( event.mouse_button_state == InputEvent::MOUSE_BUTTON_PRESSED ) {
        pos = event.pos;
        return false; //don't redraw
    }

    switch ( event.menu_button ) {
    case POPUP_BUTTON_DROP_BALL:
        send_dispball( pos );
        //set_object_pos( build,id_ball, pos );
        break;
    case POPUP_BUTTON_FREE_KICK_LEFT:
        send_dispfoul_left( pos );
        //set_object_pos( build,id_ball, pos );
        break;
    case POPUP_BUTTON_FREE_KICK_RIGHT:
        send_dispfoul_right( pos );
        //set_object_pos( build,id_ball, pos );
        break;
    }
    //popup->set_menu_extended( false );
    return true;
};

bool
SMonitorDevice::init_menu( MenuBase * menu )
{
#ifdef WEBFIELD
    return false; //TEST
#endif
    char buf[20];
    menu->set_number_of_buttons( BUTTON_NUMBER );
    menu->set_button_label( BUTTON_START,"kick off" );
    menu->set_button_label( BUTTON_RECONNECT,"connect" );
    double scale = options.scale_factor();
    std::snprintf( buf, sizeof( buf ), "scale %.1f", scale );
    menu->set_button_label( BUTTON_SCALE_LEVEL,buf );
    std::snprintf( buf, sizeof( buf ), "detail %d", options.info_level );
    menu->set_button_label( BUTTON_INFO_LEVEL,buf );
    menu->set_button_label( BUTTON_MODE,options.get_mode_string() );
    menu->set_button_label( BUTTON_UNZOOM,"unzoom" );
    menu->set_button_label( BUTTON_QUIT,"quit" );

    //menu->set_menu_extended( false );
    menu->set_menu_extended( true );
    return true;
};

bool
SMonitorDevice::init_popup( MenuBase * popup )
{
    popup->set_number_of_buttons( POPUP_BUTTON_NUMBER );
    popup->set_button_label( POPUP_BUTTON_DROP_BALL, "drop ball" );
    popup->set_button_label( POPUP_BUTTON_FREE_KICK_LEFT, "free kick left" );
    popup->set_button_label( POPUP_BUTTON_FREE_KICK_RIGHT, "free kick right" );
    return false; //don't redraw!!!
};

bool
SMonitorDevice::init_frames( BuilderBase * build )
{
    int layer = -1;

    RGBcolor c_black( 0,0,0 );
    RGBcolor c_red( 255,0,0 );

    //background
    build->set_cmd_set_background_color( options.c_field );

    //view area frame
    build->set_cmd_insert_frame( 0,frame_varea,Point2d( 2.0,0.0 ),0.0,layer-1 );
    vis_view_area.init( 0,0, options.c_varea_exact, options.c_varea_fuzzy );
    build->set_cmd_insert_visobject( frame_varea, & vis_view_area );
    build->set_cmd_set_frame_visible( frame_varea,0 );

    //soccer field
    vis_field.init( 0,layer,options.c_line,options.c_goal,
                    options.keepaway, options.keepaway_length,
                    options.keepaway_width );
    build->set_cmd_insert_visobject( 0, & vis_field );

    //PlayerTypes::init( options.ball_radius, options.player_radius,
    PlayerTypes::init( 0.085, options.player_radius,
                       options.kick_radius - 0.085 - options.player_radius );

    //in this frame the logger drawings are displayed
    build->set_cmd_insert_frame( 0, frame_canvas_left, Point2d( 0.0,0.0 ),0.0, layer+4 );
    build->set_cmd_insert_frame( 0, frame_canvas_right, Point2d( 0.0,0.0 ),M_PI, layer+4 );

    double b_rad = options.ball_radius;
    double k_rad = options.kick_radius;
    //ball
    vis_ball.init( 0,0, b_rad, k_rad,
                   options.c_ball,
                   c_red, c_red, c_black );

    server_pos.set_ball_pos( Point2d( 2.0, 0.0 ) );
#ifdef WEBFIELD
    server_pos.set_ball_pos( Point2d( 2.0, 60.0 ) );
#endif
    build->set_cmd_insert_frame( 0,frame_ball, server_pos.ball.pos,0.0,layer+3 );

    build->set_cmd_insert_visobject( frame_ball, & vis_ball );


#if 0
    if ( options.ball_skin == 2 ) {
        build->set_cmd_insert_f_circle( frame_ball,0,Circle2d( Point2d( 0.0,0.0 ),b_rad ),0,options.c_ball );
        build->set_cmd_insert_circle( frame_ball,1,Circle2d( Point2d( 0.0,0.0 ),b_rad ),0,options.c_ball );
    }
    else {
        build->set_cmd_insert_f_circle( frame_ball,0,Circle2d( Point2d( 0.0,0.0 ),b_rad ),0,options.c_ball );
        build->set_cmd_insert_point( frame_ball,1,Point2d( 0.0,0.0 ),0,options.c_ball );
        //build->set_cmd_insert_circle( frame_ball,2,Circle2d( Point2d( 0.0,0.0 ),b_rad*10 ),0,options.c_ball );
        build->set_cmd_insert_circle( frame_ball,2,Circle2d( Point2d( 0.0,0.0 ), k_rad*0.8 ),0,options.c_ball );
    }
#endif
    //shadow
    build->set_cmd_insert_frame( 0,frame_shadow,Point2d( 2.0,0.0 ),0.0,layer+3 );
    build->set_cmd_insert_circle( frame_shadow,0,Circle2d( Point2d( 0.0,0.0 ), k_rad ),0,c_black );
    build->set_cmd_insert_line( frame_shadow,0,Line2d( Point2d( -k_rad,0.0 ),Point2d( k_rad,0.0 ) ),0,c_black );
    build->set_cmd_insert_line( frame_shadow,0,Line2d( Point2d( 0.0,-k_rad ),Point2d( 0.0,k_rad ) ),0,c_black );
    build->set_cmd_set_frame_visible( frame_shadow,0 );

    //players
    //double init_y = 35.2;
    double init_y = 68.0/2.0 + 3.0;
#ifdef WEBFIELD
    init_y = 60;
#endif
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        RGBcolor c_invalid = options.c_invalid_l;
        RGBcolor c_player = options.c_team_l;
        RGBcolor c_goalie = options.c_goalie_l;
        RGBcolor c_font = options.c_font_l;
        //Point2d pos_player = Point2d( -8.0 -i*3.0*k_rad,init_y );
        Point2d pos_player = Point2d( -( ( i+1 )*3.0 ), init_y );
        if ( p_right( i ) )
        {
            c_invalid = options.c_invalid_r;
            c_player = options.c_team_r;
            c_goalie = options.c_goalie_r;
            c_font = options.c_font_r;
            //pos_player = Point2d( 8.0 + ( i-MAX_PLAYER )*3.0*k_rad,init_y );
            //pos_player = Point2d( 8.0 + ( i-MAX_PLAYER )*3.0*k_rad,init_y );
            pos_player = Point2d( ( i+1-MAX_PLAYER )*3.0, init_y );
        }
        server_pos.set_player( i,pos_player,0.0 );
        build->set_cmd_insert_frame( 0,p_frame( i ),pos_player,0.0,layer+1 );
        vis_player[i].init( 0, 0, p_number( i ), c_invalid, c_player, c_goalie, c_font );
        build->set_cmd_insert_visobject( p_frame( i ),vis_player+i );
        vis_player[i].set_label_pos( options.player_num_pos );
    }

    vis_player_set_info_level( options.info_level );
    vis_ball_set_info_level( options.info_level );

    double scale = options.scale_factor();
    if ( scale != 1.0 )
    {
        set_all_objects_scale( build, scale );
    }

    return true;
}

bool
SMonitorDevice::init_connection()
{
    if ( options.just_edit )
    {
        return true;
    }

    server.init_socket_fd();
    //server.init_socket_fd( 6010 ); //test to use client2dview with smonitor_dev.C
    server.set_fd_nonblock();
    server.init_serv_addr( options.server_host,options.server_port );

#if USE_COACH
    coach.init_socket_fd();
    coach.set_fd_nonblock();
    coach.init_serv_addr( options.server_host,options.coach_port );
#endif

    if ( options.connect_on_start )
    {
        send_dispinit();
    }
    return true;
}

bool
SMonitorDevice::uses_mouse() const
{
    if ( options.mode == Options::MODE_MOVE
         || options.mode == Options::MODE_SHOW_VIEW_AREA )
    {
        return true;
    }
    return false;
}

bool
SMonitorDevice::uses_popup() const
{
    if ( options.mode == Options::MODE_STANDARD )
    {
        return true;
    }
    return false;
}

bool
SMonitorDevice::destruct()
{
    send_dispbye();
    //some stuff should be still done here
    return true;
}

bool
SMonitorDevice::process_input( fd_set * set,
                               BuilderBase * build )
{
    if ( options.just_edit )
    {
        return true;
    }

    bool redraw = false;
    //process server commands
    if ( is_fd_in_set( server.socket_fd, set ) )
    {
        char * buf;

        int num_bytes = 0;
        int msg_type;
        int num_show_msg = 0;

        buf = buffer1;
        while ( server.recv_msg( buf, num_bytes, true ) )
        {
            //if ( RUN::freeze ) continue;
            msg_type = server_msg_type( buf );

            if ( SSrv::V3_SHOW_MODE == msg_type
                 || SSrv::SHOW_MODE == msg_type )
            {
                if ( num_bytes > 0 )
                {
                    ++num_show_msg;
                    //rotate the buf to keep the last show message in the other buffer!
                    if ( buf == buffer1 )
                    {
                        buf = buffer2;
                    }
                    else
                    {
                        buf = buffer1;
                    }
                }
            }
            else if ( SSrv::FRAMEVIEW_MODE == msg_type )
            {
                bool res = server_interpret_frameview_msg( build,buf );
                redraw = redraw || res;
            }
            else if ( SSrv::V3_MSG_MODE == msg_type )
            {
                bool res = server_interpret_msginfo_v3( build, buf );
                redraw = redraw || res;
            }
            else if ( SSrv::MSG_MODE == msg_type )
            {
                bool res = server_interpret_msginfo_t( build, ( void* )( buf ) );
                redraw = redraw || res;
            }
            else if ( SSrv::V3_DRAW_MODE == msg_type )
            {
                std::cerr << "\nmode = V3_DRAW_MODE not yet implemented";
            }
            else if ( SSrv::DRAW_MODE == msg_type )
            {
                bool res = server_interpret_drawinfo_t( build, ( void* )( buf ) );
                redraw = redraw || res;
            }
            else if ( SSrv::NO_INFO == msg_type )
            {
                std::cerr << "\nmode = NO_INFO not yet implemented";
            }
            else if ( SSrv::V3_BLANK_MODE == msg_type
                      || SSrv::BLANK_MODE == msg_type )
            {
                std::cerr << "\nmode = BLANK_MODE not yet implemented";
            }
            else if ( SSrv::V3_PM_MODE == msg_type )
            {
                bool res = server_interpret_playmode_v3( build, buf );
                redraw = redraw || res;
            }
            else if ( SSrv::PM_MODE == msg_type )
            {
                std::cerr << "\nmode = PM_MODE not yet implemented";
            }
            else if ( SSrv::V3_TEAM_MODE == msg_type )
            {
                bool res = server_interpret_team_v3( build, buf );
                redraw = redraw || res;
            }
            else if ( SSrv::TEAM_MODE == msg_type )
            {
                std::cerr << "\nmode = TEAM_MODE not yet implemented";
            }
            else if ( SSrv::V3_PT_MODE == msg_type )
            {
                // player_type
                bool res = server_interpret_player_type_v3( build, buf );
                redraw = redraw || res;
            }
            else if ( SSrv::PT_MODE == msg_type )
            {
                bool res = server_interpret_player_type_t( build, ( void* )( buf ) );
                redraw = redraw || res;
            }
            else if ( SSrv::V3_PARAM_MODE == msg_type )
            {
                // server_param
                bool res = server_interpret_server_param_v3( build, buf );
                redraw = redraw || res;
            }
            else if ( SSrv::PARAM_MODE == msg_type )
            {	 //server parameters
                bool res = server_interpret_server_params_t( build, ( void* )( buf ) );
                redraw = redraw || res;
            }
            else if ( SSrv::V3_PPARAM_MODE == msg_type
                      || SSrv::PPARAM_MODE == msg_type )
            {
                // player_param
                //std::cerr << "\nmode = PPARAM_MODE not yet implemented";
            }
            else
            {
                ERROR_OUT << "\nunknown mode " << msg_type << " :";
                int i = 0;
                while ( i < 50 )
                {
                    if ( std::isprint( buf[i] ) )
                    {
                        std::cout << buf[i];
                    }
                    else
                    {
                        break;
                    }
                    ++i;
                }

                if ( i > 0 )
                {
                    std::cout << "\nHINT: if you use server version < 7.02 repositioning of players"
                              << "\nmay couse such messages" << std::flush;
                }
            }
        }

        if ( num_show_msg > 0 )
        {
            //switch to the cached buffer
            if ( buf == buffer1 )
            {
                buf = buffer2;
            }
            else
            {
                buf = buffer1;
            }

            bool res = false;
            if ( options.protocol_version >= 3 ) // msg_type == V3_SHOT_MODE
            {
                res = server_interpret_showinfo_v3( build, buf );
            }
            else if ( options.protocol_version >= 2 )
            {
                res = server_interpret_showinfo_t2( build, ( void* )( buf ) );
            }
            else
            {
                res = server_interpret_showinfo_t( build, ( void* )( buf ) );
            }

            redraw = redraw || res;
            if ( num_show_msg > 1 )
            {
                std::cerr << "\nskipped " << num_show_msg-1 << " SHOW messages";
            }
        }
    }

#if USE_COACH
    //process coach commands
    if ( is_fd_in_set( coach.socket_fd, set ) ) {
        int num_bytes;
        while ( coach.recv_msg( buffer[0] , num_bytes, true ) ) {
            buffer[0][num_bytes] = '\0';
            std::cerr << "\ncoach received: " << buffer[0] << endl;
        }
    }
#endif

    return redraw;
}

int  SMonitorDevice::set_fds( fd_set * set ) {
    if ( options.just_edit )
        return -1;
    add_fd_to_set( server.socket_fd,set );
#if USE_COACH
    add_fd_to_set( coach.socket_fd,set );
    return server.socket_fd>coach.socket_fd?server.socket_fd:coach.socket_fd;
#else
    return server.socket_fd;
#endif

}

bool SMonitorDevice::got_fds( fd_set * set ) {
    if ( options.just_edit )
        return false;
    if ( is_fd_in_set( server.socket_fd,set ) ) return true;
#if USE_COACH
    return is_fd_in_set( coach.socket_fd,set );
#else
    return false;
#endif
}


/* private methods */
void SMonitorDevice::vis_ball_set_info_level( int lev ) {
    if ( lev >= 2 )
        vis_ball.set_show_vel_string( server_pos.ball.vel );
    else
        vis_ball.unset_show_vel_string();
}

void SMonitorDevice::vis_player_set_info_level( int lev, VisualPlayer & vis_p,
                                                const Positions::Player & p,
                                                const int unum ) {
    char dum[20];
    vis_p.set_use_number( true );
    switch ( options.info_level ) {
    case 0:
        vis_p.set_use_number( false );
        break;
    case 1:
        std::snprintf( dum, 19, "%d", unum );
        vis_p.set_label( dum );
        break;
    case 2:
        std::snprintf( dum, 19, "%d,%d", unum, ( int )p.stamina );
        vis_p.set_label( dum );
        break;
    case 3:
        std::snprintf( dum, 19, "%d,%d,t%d", unum, ( int )p.stamina, p.type );
        vis_p.set_label( dum );
        break;
    default:
        vis_p.set_label( 0 );
    }
}

void SMonitorDevice::vis_player_set_info_level( int lev ) {
    for ( int i =0; i < MAX_PLAYER*2; i++ ) {
        Positions::Player & p = server_pos.ref_player( i );
        VisualPlayer & vis_p = vis_player[i];
        vis_player_set_info_level( lev, vis_p, p, p_number( i ) );
    }

}

int
SMonitorDevice::server_msg_type( void * ptr )
{
    const char * buf = reinterpret_cast< const char * >( ptr );
    //if ( *( ( char* )ptr ) == '_' )
    if ( *buf == '_' )
    {
        return SSrv::FRAMEVIEW_MODE;
    }

    if ( options.protocol_version == 3 )
    {
        if ( ! std::strncmp( buf, "(show ", 6 ) )
        {
            return SSrv::V3_SHOW_MODE;
        }
        else if ( ! std::strncmp( buf, "(playmode ", 10 ) )
        {
            return SSrv::V3_PM_MODE;
        }
        else if ( ! std::strncmp( buf, "(team ", 6 ) )
        {
            return SSrv::V3_TEAM_MODE;
        }
        else if ( ! std::strncmp( buf, "(msg ", 5 ) )
        {
            return SSrv::V3_MSG_MODE;
        }
        else if ( ! std::strncmp( buf, "(player_type ", 13 ) )
        {
            return SSrv::V3_PT_MODE;
        }
        else if ( ! std::strncmp( buf, "(player_param ", 14 ) )
        {
            return SSrv::V3_PPARAM_MODE;
        }
        else if ( ! std::strncmp( buf, "(server_param ", 14 ) )
        {
            return SSrv::V3_PARAM_MODE;
        }
        else
        {
            return SSrv::NO_INFO;
        }
    }

    SSrv::dispinfo_t * dispinfo_dum = reinterpret_cast< SSrv::dispinfo_t * >( ptr );
    return ntohs( dispinfo_dum->mode );
    //SSrv::dispinfo_t & dispinfo = *dispinfo_dum;
    //return  ntohs( dispinfo.mode );
}


bool SMonitorDevice::ins_simple_obj( const char * buf, int fref, BuilderBase * build,const char* & next ) {
    const int idx_zero = 0;
    RGBcolor col;
    int num_entries;

    next = buf;
    strspace( next,next );
    if ( *next == 'l' )
        num_entries = 4;
    else if ( *next == 'c' )
        num_entries = 3;
    else if ( *next == 'p' )
        num_entries = 2;
    else
        return false;

    buf = next;
    next++;

    double dd[4];
    if ( num_entries != str2val( next, num_entries,dd,next ) )
        return false;

    strspace( next,next );
    if ( next[0] == '#' )
        next++;

    const char * dum;
    if ( next[0] != ';' )
        if ( AsciiProcessor::get_col( next,col,dum ) )
            next = dum;
        else {
            //we need a copy of the color string
            char dum[100];
            dum[99] = '\0';
            for ( int i =0; i<99; i++ ) {
                if ( *next == ';' || *next == ' ' || *next == '\0' ) {
                    dum[i] = '\0';
                    break;
                }
                dum[i] = *next;
                next++;
            }
            col = RGB_DB::XNamedColor_to_RGBcolor( dum );
        }

    switch ( *buf ) {
    case 'l':
        build->set_cmd_insert_line( fref,idx_zero, Line2d( Point2d( dd[0],dd[1] ),Point2d( dd[2],dd[3] ) ),0,col );
        break;
    case 'c':
        build->set_cmd_insert_circle( fref,idx_zero, Circle2d( Point2d( dd[0],dd[1] ),dd[2] ),0,col );
        break;
    case 'p':
        build->set_cmd_insert_point( fref,idx_zero, Point2d( dd[0],dd[1] ),0,col );
        break;
    }

    if ( ! strskip( next,';',next ) )
        return false;
    return true;
}

bool
SMonitorDevice::server_interpret_showinfo_t( BuilderBase * build,
                                             void * ptr )
{
    SSrv::dispinfo_t * dispinfo = reinterpret_cast< SSrv::dispinfo_t * >( ptr );

    if ( ntohs( dispinfo->mode ) != SSrv::SHOW_MODE )
    {
        ERROR_OUT << "\nmode != SHOW_MODE";
        return false;
    }

    const SSrv::showinfo_t & showinfo = dispinfo->body.show; //shortcut
    server_state.current_time_ = ntohs( showinfo.time );
    server_state.left_teamname_.assign( showinfo.team[0].name,
                                        std::min( std::strlen( showinfo.team[0].name ),
                                                  size_t( 16 ) ) );
    server_state.right_teamname_.assign( showinfo.team[1].name,
                                         std::min( std::strlen( showinfo.team[1].name ),
                                                   size_t( 16 ) ) );
    //match info
    Int16 s_l = ntohs( showinfo.team[0].score );
    Int16 s_r = ntohs( showinfo.team[1].score );
    if ( server_state.left_teamname_.empty() ) s_l = 0;
    if ( server_state.right_teamname_.empty() ) s_r = 0;

    updatePlayMode( static_cast< int >( showinfo.pmode ) );
    updateScores( server_state.current_time_,
                  server_state.playmode_,
                  s_l, s_r );

    updateScoreBoard( server_state.current_time_ );

    if ( server_state.reconnected_ )
    {
        //clear the drawing plane
        DEBUG( << " refreshing drawarea" );
        build->set_cmd_empty_frame( frame_canvas_left );
        build->set_cmd_empty_frame( frame_canvas_right );
    }

    { //set the ball
        /* Zwischenspeicherung als short notwendig, da ansonsten ntohs nicht richtig funkioniert */
        Int16 s_x      = ntohs( showinfo.pos[0].x );
        Int16 s_y      = ntohs( showinfo.pos[0].y );
        Point2d pos;
        pos.x     = x_SERVER_2_LP ( s_x / SHOWINFO_SCALE );
        pos.y     = y_SERVER_2_LP ( s_y / SHOWINFO_SCALE );
        server_pos.set_ball_pos( pos ); //just to collect informations

        if ( options.active_in_mode != id_ball
             || options.mode != Options::MODE_MOVE )
        {
            build->set_cmd_set_frame_pos( frame_ball, pos );
        }
        else
        {
            build->set_cmd_set_frame_pos_ang( frame_shadow, pos, Angle() );
        }

        if ( options.track_ball )
        {
            build->set_cmd_set_frame_pos( 0, Point2d( -pos.x, -pos.y ) );
        }
    }

    for ( int i = 0; i < 2*MAX_PLAYER; ++i ) //players [0 ... MAX_PLAYER*2-1]
    {
        /* Zwischenspeicherung als short notwendig, da ansonsten ntohs nicht richtig funkioniert */
        Int16 s_x      = ntohs( showinfo.pos[i+1].x );
        Int16 s_y      = ntohs( showinfo.pos[i+1].y );
        Int16 s_mode   = ntohs( showinfo.pos[i+1].enable );
        //Int16 s_side   = ntohs( showinfo.pos[i+1].side );
        //Int16 s_unum   = ntohs( showinfo.pos[i+1].unum );
        Int16 s_angle  = ntohs( showinfo.pos[i+1].angle );
        Point2d pos;
        pos.x     = x_SERVER_2_LP ( s_x / SHOWINFO_SCALE );
        pos.y     = y_SERVER_2_LP ( s_y / SHOWINFO_SCALE );
        Angle angle = Angle( ang_SERVER_2_LP ( double( s_angle ) ) );

        server_pos.set_player( i,pos,angle ); //just to collect informations

        if ( options.active_in_mode != i
             || options.mode != Options::MODE_MOVE )
        {
            build->set_cmd_set_frame_pos_ang( p_frame( i ),pos,angle );
        }
        else
        {
            build->set_cmd_set_frame_pos_ang( frame_shadow,pos,angle );
        }

        vis_player[i].set_active( s_mode != DISABLE );
        vis_player[i].set_goalie( s_mode & GOALIE );
        vis_player[i].set_kicking( s_mode & KICK );

        vis_player[i].set_kicking_fault( s_mode & KICK_FAULT );
        vis_player[i].set_catching_fault( s_mode & CATCH_FAULT );
        vis_player[i].set_tackling( s_mode & TACKLE );
        vis_player[i].set_tackling_fault( s_mode & TACKLE_FAULT );

        vis_player[i].set_ball_collision( s_mode & BALL_COLLIDE );
    }

    server_state.reconnected_ = false;
    return true;
}


bool
SMonitorDevice::server_interpret_drawinfo_t( BuilderBase * build,
                                             void * ptr )
{
    SSrv::dispinfo_t * dispinfo = reinterpret_cast< SSrv::dispinfo_t * >( ptr );

    Int16 s_x, s_y, s_x2, s_y2, s_r;
    Point2d pos, pos2;
    double r;
    bool redraw = false;
    const int idx_zero = 0;

    if ( ntohs ( dispinfo->mode ) != SSrv::DRAW_MODE )
    {
        ERROR_OUT << "\nmode != DRAW_MODE";
        return false;
    }

    const SSrv::drawinfo_t & drawinfo = dispinfo->body.draw; //shortcut
    switch ( ntohs( drawinfo.mode ) ) {
    case DrawPoint:
        redraw = true;
        s_x = ntohs( drawinfo.object.pinfo.x );
        s_y = ntohs( drawinfo.object.pinfo.y );
        pos.x = x_SERVER_2_LP ( s_x );
        pos.y = y_SERVER_2_LP ( s_y );
        //std::cout << "\nDrawPoint " << draw_obj_idx << " x,y = " << x << "," << y;
        build->set_cmd_insert_point( frame_canvas_left,idx_zero,pos,0,
                                     RGB_DB::XNamedColor_to_RGBcolor( drawinfo.object.pinfo.color ) );

        break;
    case DrawCircle:
        redraw = true;
        s_x = ntohs( drawinfo.object.cinfo.x );
        s_y = ntohs( drawinfo.object.cinfo.y );
        s_r = ntohs( drawinfo.object.cinfo.r );
        r = s_r;
        pos.x = x_SERVER_2_LP ( s_x ) + r;
        pos.y = y_SERVER_2_LP ( s_y ) - r;

        //std::cout << "\nDrawCircle " << draw_obj_idx << " ( x,y ),r = ( " << x << "," << y << " ), "<< r;
        build->set_cmd_insert_circle( frame_canvas_left,idx_zero,Circle2d( pos,r ),0,
                                      RGB_DB::XNamedColor_to_RGBcolor( drawinfo.object.cinfo.color ) );
        break;
    case DrawLine:
        redraw = true;
        s_x = ntohs( drawinfo.object.linfo.x1 );
        s_y = ntohs( drawinfo.object.linfo.y1 );
        s_x2 = ntohs( drawinfo.object.linfo.x2 );
        s_y2 = ntohs( drawinfo.object.linfo.y2 );
        pos.x = x_SERVER_2_LP ( s_x  );
        pos.y = y_SERVER_2_LP ( s_y  );
        pos2.x = x_SERVER_2_LP ( s_x2 );
        pos2.y = y_SERVER_2_LP ( s_y2 );
        build->set_cmd_insert_line( frame_canvas_left, idx_zero,
                                    Line2d( pos,pos2 ), 0,
                                    RGB_DB::XNamedColor_to_RGBcolor( drawinfo.object.linfo.color ) );
        break;
    case DrawClear:
        redraw = true;
        DEBUG( << " refreshing drawarea" );
        build->set_cmd_empty_frame( frame_canvas_left );
        build->set_cmd_empty_frame( frame_canvas_right );
        //field.drawclear();
        break;
    default:
        ERROR_OUT << "\nIllegal value in DRAW_MODE: " << ntohs( drawinfo.mode );
    }
    return redraw;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
SMonitorDevice::server_interpret_msginfo_t( BuilderBase * build,
                                            void * ptr )
{
    if ( options.protocol_version >= 3 )
    {
        return false;
    }

    const SSrv::dispinfo_t * dispinfo = NULL;
    const SSrv::dispinfo_t2 * dispinfo2 = NULL;
    const SSrv::msginfo_t * msg = NULL;

    if ( options.protocol_version == 2 )
    {
        dispinfo2 = reinterpret_cast< SSrv::dispinfo_t2 * >( ptr );
        if ( ntohs( dispinfo2->mode ) != SSrv::MSG_MODE )
        {
            ERROR_OUT << "\nmode != MSG_MODE";
            return false;
        }
        msg = &( dispinfo2->body.msg );
    }
    else
    {
        dispinfo = reinterpret_cast< SSrv::dispinfo_t * >( ptr );
        if ( ntohs( dispinfo->mode ) != SSrv::MSG_MODE )
        {
            ERROR_OUT << "\nmode != MSG_MODE";
            return false;
        }
        msg = &( dispinfo->body.msg );
    }

    if ( ! msg )
    {
        return false;
    }

    if ( server_interpret_command_msg( build, msg->message )  )
    {
        return true;
    }

    //std::cout <<"\nMSG_MODE ";

    //left player is sending on MSG_BOARD, right player is sending on LOG_BOARD
    //coordinates of right player have to be converted
    int canvas = ( ntohs( msg->board ) == LOG_BOARD
                   ? frame_canvas_right
                   : frame_canvas_left );

#if 0
    std::cout <<"\n got message, canvas = " << canvas << " msg = ";
    std::cout << "\n>>>";// << msg;
    for ( int i =0; i< 300; i++ ) {
        if ( msg[i] == '\0' )
            break;
        else if ( isprint( msg[i] ) )
            std::cout << msg[i];
        else
            std::cout << '@';
    }
    std::cout << flush;
#endif

    return server_interpret_frameview_msg( build, msg->message, true, canvas );
}

void
SMonitorDevice::show_parser_error_point( std::ostream & out,
                                         const char * origin,
                                         const char * parse_error_point )
{
    if ( parse_error_point )
    {
        for ( ; origin<parse_error_point && *origin!= '\0'; ++origin )
        {
            out << *origin;
        }
    }
    else
    {
        out << origin
            << "\n[no parse error point provided]";
    }

    if ( origin != parse_error_point )
    {
        out << "\n[something wrong with parse error point]";
        return;
    }

    out << "   <***parse error***>   ";

    int i =0;
    while ( i < 25 && *origin != '\0' )
    {
        out << *origin;
        ++origin;
        ++i;
    }

    if ( *origin != '\0' )
    {
        out << " .......";
    }
}


bool
SMonitorDevice::server_interpret_frameview_msg( BuilderBase * build,
                                                const char * msg,
                                                bool enforce_frame,
                                                int frame )
{
    int canvas;
    if ( ! strskip( msg,"_2D_",msg ) )
    {
        ERROR_OUT << "\nNOT recognized as 2d object" << std::flush;
        return false;
    }

    if ( enforce_frame )
    {
        canvas = frame;
    }
    else
    {
        if ( msg[0] == 'R' )
        {
            canvas = frame_canvas_right;
            msg++;
        }
        else if ( msg[0] == 'L' )
        {
            canvas = frame_canvas_left;
            msg++;
        }
        else
        {
            canvas = frame_canvas_left;
        }
    }
    //std::cout << "\ngot 2D message " << msg << flush;

    while ( 1 )
    {
        strspace( msg, msg );

        if ( msg[0] == '\0' )
        {
            break;
        }

        const char * beg_msg = msg;
        int res = false;

        if ( ! std::strncmp ( msg, "CLEAR", 5 ) )
        {
            //clear drawing plane
            msg += 5;
            DEBUG( << " refreshing drawarea" );
            build->set_cmd_empty_frame( frame_canvas_left );
            build->set_cmd_empty_frame( frame_canvas_right );
            if ( strskip( msg,';',msg ) )
                res = true;
        }
        else if ( msg[0] == 'l' || msg[0] == 'c' || msg[0] == 'p' )
        {
            res = ins_simple_obj( msg,canvas,build,msg );
        }
        else
        {
            res = AsciiProcessor::ins_obj( msg,canvas,build,msg );
        }

        if ( ! res )
        {
            ERROR_OUT << "\nDrawing input parse error:\n";
            //ERROR_STREAM << "\noriginal message: " << str;
            //ERROR_STREAM << "\n\n>>>";
            show_parser_error_point( ERROR_STREAM,beg_msg,msg );
            return false;
        }
    }
    return true;
    //std::cout << dispinfo.body.msg.message << flush;
}

bool
SMonitorDevice::server_interpret_command_msg( BuilderBase * build,
                                              const char * msg )
{
    if ( ! std::strncmp( msg, "(change_player_type",
                         std::strlen( "(change_player_type" ) ) )
    {
        // nothing to do.
        return true;
    }

    if ( ! std::strncmp( msg, "(team_graphic",
                         std::strlen( "(team_graphic" ) ) )
    {
        // TODO: added xpm holder
        return true;
    }

    if ( *msg == '(' )
    {
        ERROR_OUT << "\nUnsupported message: " << msg << std::flush;
    }

    return false;
}


//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
bool
SMonitorDevice::server_interpret_showinfo_t2( BuilderBase * build,
                                              void * ptr )
{
    SSrv::dispinfo_t2 * dispinfo = reinterpret_cast< SSrv::dispinfo_t2 * >( ptr );

    if ( ntohs( dispinfo->mode ) != SSrv::SHOW_MODE )
    {
        ERROR_OUT << "\nmode != SHOW_MODE";
        return false;
    }

    const SSrv::showinfo_t2 & showinfo = dispinfo->body.show; //shortcut
    server_state.current_time_ = ntohs( showinfo.time );
    server_state.left_teamname_.assign( showinfo.team[0].name,
                                        std::min( std::strlen( showinfo.team[0].name ),
                                                  size_t( 16 ) ) );
    server_state.right_teamname_.assign( showinfo.team[1].name,
                                        std::min( std::strlen( showinfo.team[1].name ),
                                                  size_t( 16 ) ) );
    //match info
    Int16 s_l = ntohs( showinfo.team[0].score );
    Int16 s_r = ntohs( showinfo.team[1].score );
    if ( server_state.left_teamname_.empty() ) s_l = 0;
    if ( server_state.right_teamname_.empty() ) s_r = 0;

    updatePlayMode( static_cast< int >( showinfo.pmode ) );
    updateScores( server_state.current_time_,
                  server_state.playmode_,
                  s_l, s_r );

    updateScoreBoard( server_state.current_time_ );

    if ( server_state.reconnected_ )
    {
        //clear the drawing plane
        DEBUG( << " refreshing drawarea" );
        build->set_cmd_empty_frame( frame_canvas_left );
        build->set_cmd_empty_frame( frame_canvas_right );
    }

    {
        //set the ball
        Int32 l_x      = ntohl ( showinfo.ball.x );
        Int32 l_y      = ntohl( showinfo.ball.y );
        //the velocity is
        Int32 l_vx     = ntohl( showinfo.ball.deltax );
        Int32 l_vy     = ntohl( showinfo.ball.deltay );

        Positions::Ball & b = server_pos.ball;
        b.pos.x = x_SERVER_2_LP ( double( l_x ) / SHOWINFO_SCALE2 );
        b.pos.y = y_SERVER_2_LP ( double( l_y ) / SHOWINFO_SCALE2 );
        b.vel.x = x_SERVER_2_LP ( double( l_vx ) / SHOWINFO_SCALE2 );
        b.vel.y = y_SERVER_2_LP ( double( l_vy ) / SHOWINFO_SCALE2 );

        if ( options.active_in_mode != id_ball
             || options.mode != Options::MODE_MOVE )
        {
            build->set_cmd_set_frame_pos( frame_ball, b.pos );
            if ( options.show_ball_vel )
            {
                vis_ball.set_show_vel( b.vel );
            }
            vis_ball_set_info_level( options.info_level );
        }
        else
        {
            build->set_cmd_set_frame_pos_ang( frame_shadow,b.pos,Angle() );
        }

        if ( options.track_ball )
        {
            build->set_cmd_set_frame_pos( 0, Point2d( -b.pos.x, -b.pos.y ) );
        }
    }

    //set players
    for ( int i = 0; i< 2*MAX_PLAYER; ++i )
    {
        const SSrv::player_t & info_pos = showinfo.pos[i];
        /* Zwischenspeicherung als short notwendig, da ansonsten ntohs nicht richtig funkioniert */
        Int32 l_x      = ntohl( info_pos.x );
        Int32 l_y      = ntohl( info_pos.y );
        Int16 s_mode  = ntohs( info_pos.mode );
        Int16 s_type  = ntohs( info_pos.type );
        Int32 l_body_angle  = ntohl( info_pos.body_angle );
        Int32 l_head_angle  = ntohl( info_pos.head_angle );

        Int32 l_stamina     = ntohl( info_pos.stamina );
        Int32 l_view_width = ntohl( info_pos.view_width );
        Int16 s_view_quality = ntohs( info_pos.view_quality );

        Positions::Player & p = server_pos.ref_player( i );
        VisualPlayer & vis_p = vis_player[i];

        p.pos.x     = x_SERVER_2_LP ( double( l_x ) / SHOWINFO_SCALE2 );
        p.pos.y     = y_SERVER_2_LP ( double( l_y ) / SHOWINFO_SCALE2 );

        p.type = s_type;
        //angles are now radians ( but still in wrong direction ) ( ver. 7.00 ),
        p.body_angle =  - double( l_body_angle ) / SHOWINFO_SCALE2;
        p.head_angle_rel =  - double( l_head_angle ) / SHOWINFO_SCALE2;
        p.view_width = double( l_view_width )/ SHOWINFO_SCALE2;
        p.view_quality = s_view_quality;
        p.stamina = double( l_stamina ) / SHOWINFO_SCALE2;
        p.alive = s_mode != DISABLE;

        vis_p.set_body_angle( p.body_angle );

        if ( options.active_in_mode != i || options.mode != Options::MODE_MOVE ) {
            build->set_cmd_set_frame_pos_ang( p_frame( i ),p.pos,p.body_angle );
            vis_p.set_head_angle( p.head_angle_rel );
        }
        else
            build->set_cmd_set_frame_pos_ang( frame_shadow,p.pos,p.body_angle );

        if ( options.active_in_mode == i && options.mode == Options::MODE_SHOW_VIEW_AREA ) {
            vis_view_area.set_view_mode( p.view_quality,p.view_width );
            build->set_cmd_set_frame_pos_ang( frame_varea, p.pos,p.body_angle + p.head_angle_rel );
        }

        if ( guess.use_type ) vis_p.set_type( p.type );
        vis_p.set_active( p.alive );
        vis_p.set_goalie( s_mode & GOALIE );
        vis_p.set_kicking( s_mode & KICK );

        vis_p.set_kicking_fault( s_mode & KICK_FAULT );
        vis_p.set_catching_fault( s_mode & CATCH_FAULT );
        vis_p.set_tackling( s_mode & TACKLE );
        vis_p.set_tackling_fault( s_mode & TACKLE_FAULT );

        if ( options.show_ball_collisions )
        {
            vis_p.set_ball_collision( s_mode & BALL_COLLIDE );
        }

        if ( guess.use_stamina )
        {
            vis_p.set_low_stamina_indicator( p.stamina < 1500.0 );
        }

        vis_player_set_info_level( options.info_level, vis_p, p, p_number( i ) );
    }

#if 1
    if ( options.rt_show_spare_space_to_offside_line )
    {
        double x1, x2, x3;
        bool res = server_pos.lt_min_x_pos( x1,x2 ) && server_pos.rt_min_x_pos( x3 );
        //std::cout << "\nres = " << res << " x1 = " << x1 << " x2 = " << x2 << "; x3 = " << x3 << flush;
        if ( x2>0.0 )
            x2 = 0.0;
        if ( res && x2 < x3 ) {
            if ( server_pos.ball.pos.x < x2 )
                x2 = server_pos.ball.pos.x;
            //std::cout << "<<<<<<<<<<< use it" << flush;
            vis_field.use_vertical_stripe( true );
            vis_field.set_vertical_stripe( x2,x3 );
        }
        else
            vis_field.use_vertical_stripe( false );
    }
    else if ( options.lt_show_spare_space_to_offside_line ) {
        double x1, x2, x3;
        bool res = server_pos.rt_max_x_pos( x1,x2 ) && server_pos.lt_max_x_pos( x3 );
        //std::cout << "\nres = " << res << " x1 = " << x1 << " x2 = " << x2 << "; x3 = " << x3 << flush;
        if ( x2<0.0 )
            x2 = 0.0;
        if ( res && x2 > x3 ) {
            if ( server_pos.ball.pos.x > x2 )
                x2 = server_pos.ball.pos.x;
            //std::cout << "<<<<<<<<<<< use it" << flush;
            vis_field.use_vertical_stripe( true );
            vis_field.set_vertical_stripe( x2,x3 );
        }
        else
            vis_field.use_vertical_stripe( false );
    }
    else
        vis_field.use_vertical_stripe( false );

#endif

    server_state.reconnected_ = false;

    return true;
}

bool
SMonitorDevice::server_interpret_player_type_t( BuilderBase * build,
                                                void * ptr )
{
    SSrv::dispinfo_t2 * dispinfo = reinterpret_cast< SSrv::dispinfo_t2 * >( ptr );

    if ( ntohs( dispinfo->mode ) != SSrv::PT_MODE )
    {
        ERROR_OUT << "\nmode != PT_MODE";
        return false;
    }

    const SSrv::player_type_t &player_type = dispinfo->body.ptinfo; //shortcut
    if ( options.list_player_types ) {
        std::cout << ""
                  << "\ntype = " << ntohs( player_type.id )
                  << "\nplayer_speed_max = " << double( ntohl( player_type.player_speed_max ) ) / SHOWINFO_SCALE2
                  << "\nstamina_inc_max = " << double( ntohl( player_type.stamina_inc_max ) ) / SHOWINFO_SCALE2
                  << "\nplayer_decay    = " << double( ntohl( player_type.player_decay ) ) / SHOWINFO_SCALE2
                  << "\ninertia_moment  = " << double( ntohl( player_type.inertia_moment ) ) / SHOWINFO_SCALE2
                  << "\ndash_power_rate = " << double( ntohl( player_type.dash_power_rate ) ) / SHOWINFO_SCALE2
                  << "\nplayer_size     = " << double( ntohl( player_type.player_size ) ) / SHOWINFO_SCALE2
                  << "\nkickable_margin = " << double( ntohl( player_type.kickable_margin ) ) / SHOWINFO_SCALE2
                  << "\nkick_rand       = " << double( ntohl( player_type.kick_rand ) ) / SHOWINFO_SCALE2
            // << "\nextra_stamina   = " << double( ntohl( player_type.extra_stamina ) ) / SHOWINFO_SCALE2
            // << "\neffort_max      = " << double( ntohl( player_type.effort_max ) ) / SHOWINFO_SCALE2
            // << "\neffort_min      = " << double( ntohl( player_type.effort_min ) ) / SHOWINFO_SCALE2
                  << "";
    }

    Int16 s_type = ntohs( player_type.id );
    double player_size = double( ntohl( player_type.player_size ) )/SHOWINFO_SCALE2;
    double kickable_margin = double( ntohl( player_type.kickable_margin ) )/SHOWINFO_SCALE2;

    static bool put_warning = false;

    if ( std::fabs( player_size ) < 0.01 )
    {
        if ( ! put_warning )
        {
            WARNING_OUT << "\nlogplayer seems not to send player types info ( check for version >= 7.08 )";
            PlayerTypes::use_std_type = true;
            put_warning = true;
        }
        guess.use_type = false;
        return false;
    }
    guess.use_type = true;
    guess.use_stamina = true;

    PlayerTypes::use_std_type = false;

    PlayerTypes::set_type( s_type, player_size, kickable_margin );
    return false;
}

bool
SMonitorDevice::server_interpret_server_params_t( BuilderBase * build,
                                                  void * ptr )
{
    SSrv::dispinfo_t2 * dispinfo = reinterpret_cast< SSrv::dispinfo_t2 * >( ptr );

    if ( ntohs( dispinfo->mode ) != SSrv::PARAM_MODE )
    {
        ERROR_OUT << "\nmode != PARAM_MODE";
        return false;
    }

    const SSrv::server_params_t &server_params = dispinfo->body.sparams; //shortcut

    double goal_width = double( ntohl( server_params.gwidth ) ) / SHOWINFO_SCALE2;
    if ( std::fabs( goal_width ) < 0.01 )
    {
        WARNING_OUT << "\nlogplayer seems not to send correct server parameters ( check for version >= 7.08 )";
        return false;
    }

    vis_field.set_goal_width( double( ntohl( server_params.gwidth ) ) / SHOWINFO_SCALE2 );
    vis_ball.set_ball_decay( double( ntohl( server_params.bdecay ) ) / SHOWINFO_SCALE2 );

    PlayerTypes::init( double( ntohl( server_params.bsize ) ) / SHOWINFO_SCALE2,
                       double( ntohl( server_params.psize ) ) / SHOWINFO_SCALE2,
                       double( ntohl( server_params.kmargin ) ) / SHOWINFO_SCALE2 );

#if 0
    Int32 stamina_inc;                          /* player stamina inc */
    Int32 visdist;                                      /* visible distance */
    Int32 kickable_area;                        /* kickable_area */
    Int32 catch_area_l;                 /* goalie catchable area length */
    Int32 catch_area_w;                 /* goalie catchable area width */
    Int32 catch_prob;                           /* goalie catchable possibility */
    Int16   goalie_max_moves;                 /* goalie max moves after a catch */
    Int32 ckmargin;                             /* corner kick margin */
#endif

    return true;
}

bool
SMonitorDevice::server_interpret_showinfo_v3( BuilderBase * build,
                                              const char * buf )
{
    int n_read = 0;

    int time = 0;

    if ( std::sscanf( buf, " ( show %d %n ", &time, &n_read ) != 1 )
    {
        ERROR_OUT << "\nIllegal time value in show info.";
        return false;
    }
    buf += n_read;

    //std::cerr << "\nshow " << time << " len =" << std::strlen( buf );

    int playmode = 0;

    if ( std::sscanf( buf, " ( pm %d ) %n ",
                      &playmode, &n_read ) != 1 )
    {
        ERROR_OUT << "\nIllegal playmode in show info.";
        return false;
    }
    buf += n_read;

    int score_l = 0, score_r = 0;
    int pen_score_l = 0, pen_miss_l = 0, pen_score_r = 0, pen_miss_r = 0;

    if ( std::sscanf( buf, " ( score %d %d %n ",
                      &score_l, &score_r,
                      &n_read) != 2 )
    {
        ERROR_OUT << "\nIllegal score in show info.";
        return false;
    }
    buf += n_read;

    if ( *buf != ')' )
    {
        if ( std::sscanf( buf, " %d %d %d %d ) %n ",
                          &pen_score_l, &pen_miss_l,
                          &pen_score_r, &pen_miss_r,
                          &n_read ) != 4 )
        {
            ERROR_OUT << "\nIllegal penalty scores in show info.";
            return false;
        }
        buf += n_read;
    }
    else
    {
        ++buf;
    }

    server_state.current_time_ = time;

    updatePlayMode( playmode );
    updateScores( time, playmode,
                  score_l, score_r,
                  pen_score_l, pen_miss_l,
                  pen_score_r, pen_miss_r,
                  true ); // has penalty score info

    updateScoreBoard( time );

    if ( server_state.reconnected_ )
    {
        //clear the drawing plane
        DEBUG( << " refreshing drawarea" );
        build->set_cmd_empty_frame( frame_canvas_left );
        build->set_cmd_empty_frame( frame_canvas_right );
    }

    {
        double x, y, vx, vy;

        if ( std::sscanf( buf, " ( ( b ) %lf %lf %lf %lf ) %n ",
                          &x, &y, &vx, &vy, &n_read ) != 4 )
        {
            ERROR_OUT << "\nIllegal ball info in show info.";
            return false;
        }
        buf += n_read;

        Positions::Ball & b = server_pos.ball;
        b.pos.x = x_SERVER_2_LP( x );
        b.pos.y = y_SERVER_2_LP( y );
        b.vel.x = x_SERVER_2_LP( vx );
        b.vel.y = y_SERVER_2_LP( vy );

        if ( options.active_in_mode != id_ball
             || options.mode != Options::MODE_MOVE )
        {
            build->set_cmd_set_frame_pos( frame_ball, b.pos );
            if ( options.show_ball_vel )
            {
                vis_ball.set_show_vel( b.vel );
            }
            vis_ball_set_info_level( options.info_level );
        }
        else
        {
            build->set_cmd_set_frame_pos_ang( frame_shadow,b.pos,Angle() );
        }

        if ( options.track_ball )
        {
            build->set_cmd_set_frame_pos( 0, Point2d( -b.pos.x, -b.pos.y ) );
        }
    }

    for ( int i = 0; i < 2*MAX_PLAYER; ++i )
    {
        if ( *buf == ')' ) break;

        char side;
        int unum, type;
        long state;
        double x, y, vx, vy, body, neck;

        if ( std::sscanf( buf,
                          " ( ( %c %d ) %d %lx %lf %lf %lf %lf %lf %lf %n ",
                          &side, &unum, &type, &state,
                          &x, &y, &vx, &vy, &body, &neck,
                          &n_read ) != 10 )
        {
            WARNING_OUT << "\nIllegal player " << i << " info in show info ";
            //<< std::string( buf, 16 );
            break;
        }
        buf += n_read;

        double arm_dist = -1.0, arm_head = 0.0;
        if ( std::sscanf( buf,
                          " %lf %lf %n ",
                          &arm_dist, &arm_head,
                          &n_read ) == 2 )
        {
            buf += n_read;
        }

        char view_quality = 'h';
        double view_width = 0.0;
        if ( std::sscanf( buf,
                          " ( v %c %lf ) %n ",
                          &view_quality, &view_width,
                          &n_read ) != 2 )
        {
            WARNING_OUT << "\nIllegal player " << i << " view info in show info";
            break;
        }
        buf += n_read;

        double stamina, effort, recovery;
        if ( std::sscanf( buf,
                          " ( s %lf %lf %lf ) %n ",
                          &stamina, &effort, &recovery,
                          &n_read ) != 3 )
        {
            WARNING_OUT << "\nIllegal player " << i << " stamina info in show info";
            break;
        }
        buf += n_read;

        char focus_side = 'n';
        int focus_unum = 0;
        if ( std::sscanf( buf,
                          " ( f %c %d ) %n ",
                          &focus_side, &focus_unum,
                          &n_read ) == 2 )
        {
            buf += n_read;
        }

        int n_kick, n_dash, n_turn, n_catch, n_move,
            n_turn_neck, n_change_view, n_say, n_tackle,
            n_pointto, n_attentionto;
        if ( std::sscanf( buf,
                          " ( c %d %d %d %d %d %d %d %d %d %d %d ) ) %n ",
                          &n_kick, &n_dash, &n_turn, &n_catch, &n_move,
                          &n_turn_neck, &n_change_view, &n_say, &n_tackle,
                          &n_pointto, &n_attentionto,
                          &n_read ) != 11 )
        {
            WARNING_OUT << "\nIllegal player " << i << " count info in show info";
            break;
        }
        buf += n_read;

        const int idx = ( side == 'l'
                          ? unum - 1
                          : MAX_PLAYER + unum - 1 );
        if ( idx < 0 || 2*MAX_PLAYER <= idx )
        {
            WARNING_OUT << "\nIllegal player " << i << " unum in show info";
            break;
        }

        Positions::Player & p = server_pos.ref_player( idx );
        VisualPlayer & vis_p = vis_player[idx];

        p.pos.x = x_SERVER_2_LP( x );
        p.pos.y = y_SERVER_2_LP( y );
        p.type = type;
        p.body_angle = - body * M_PI / 180.0;
        p.head_angle_rel = - neck * M_PI / 180.0;
        p.view_quality = ( view_quality == 'h'
                           ? VisualPlayersViewArea::HIGH
                           : VisualPlayersViewArea::LOW );
        p.view_width = view_width * M_PI / 180.0;
        p.stamina = stamina;
        p.alive = ( state != DISABLE );

        vis_p.set_body_angle( p.body_angle );

        if ( options.active_in_mode != i
             || options.mode != Options::MODE_MOVE )
        {
            build->set_cmd_set_frame_pos_ang( p_frame( idx ), p.pos, p.body_angle );
            vis_p.set_head_angle( p.head_angle_rel );
        }
        else
        {
            build->set_cmd_set_frame_pos_ang( frame_shadow, p.pos, p.body_angle );
        }

        if ( options.active_in_mode == i
             && options.mode == Options::MODE_SHOW_VIEW_AREA )
        {
            vis_view_area.set_view_mode( p.view_quality, p.view_width );
            build->set_cmd_set_frame_pos_ang( frame_varea,
                                              p.pos,
                                              p.body_angle + p.head_angle_rel );
        }

        if ( guess.use_type )
        {
            vis_p.set_type( p.type );
        }

        vis_p.set_active( p.alive );
        vis_p.set_goalie( state & GOALIE );
        vis_p.set_kicking( state & KICK );

        vis_p.set_kicking_fault( state & KICK_FAULT );
        vis_p.set_catching_fault( state & CATCH_FAULT );
        vis_p.set_tackling( state & TACKLE );
        vis_p.set_tackling_fault( state & TACKLE_FAULT );

        if ( options.show_ball_collisions )
        {
            vis_p.set_ball_collision( state & BALL_COLLIDE );
        }

        if ( guess.use_stamina )
        {
            vis_p.set_low_stamina_indicator( p.stamina < 1500.0 );
        }

        vis_player_set_info_level( options.info_level, vis_p, p, p_number( idx ) );
    }

    // TODO: offside line

    server_state.reconnected_ = false;

    return true;
}

bool
SMonitorDevice::server_interpret_msginfo_v3( BuilderBase * build,
                                             const char * buf )
{
    // ( msg <board_type> "<message_string>" )

    int time = 0;
    int board = 0;
    char msg[8192];

    if ( std::sscanf( buf,
                      " ( msg %d %d \"%8191[^\"]\" ) ",
                      &time, &board, msg ) != 3 )
    {
        ERROR_OUT << "\nIllegal msg info.";
        return false;
    }

    if ( server_interpret_command_msg( build, msg ) )
    {
        return true;
    }

    int canvas = ( board == LOG_BOARD
                   ? frame_canvas_right
                   : frame_canvas_left );

    return server_interpret_frameview_msg( build, msg, true, canvas );;
}

bool
SMonitorDevice::server_interpret_playmode_v3( BuilderBase * build,
                                              const char * buf )
{
    int time = 0;
    char pmode[256];

    if ( std::sscanf( buf,
                      " ( playmode %d %255[^ )] ) ",
                      &time, pmode ) != 2
         || std::strlen( pmode ) == 0 )
    {
        ERROR_OUT << "\nIllegal playmode info.";
        return false;
    }

    server_state.current_time_ = time;
    server_state.playmode_string_ = pmode;

    if ( server_state.playmode_string_ == "time_over" )
    {
        M_timeover = true;
    }

    return true;
}

bool
SMonitorDevice::server_interpret_team_v3( BuilderBase * build,
                                          const char * buf )
{
    int time = 0;
    char name_l[256];
    char name_r[256];
    int score_l = 0, score_r = 0;
    int pen_score_l = 0, pen_miss_l = 0, pen_score_r = 0, pen_miss_r = 0;

    int n = std::sscanf( buf,
                         " ( team %d %255s %255s %d %d %d %d %d %d ",
                         &time,
                         name_l, name_r,
                         &score_l, &score_r,
                         &pen_score_l, &pen_miss_l,
                         &pen_score_r, &pen_miss_r );

    if ( n != 5 && n != 9 )
    {
        ERROR_OUT << "\nIllegal team info. n = ";
        return false;
    }


    if ( std::strlen( name_l ) != 4
         || std::strncmp( name_l, "null", 4 ) != 0 )
    {
        server_state.left_teamname_.assign( name_l,
                                            std::min( std::strlen( name_l ),
                                                      size_t( 16 ) ) );
    }

    if ( std::strlen( name_r ) != 4
         || std::strncmp( name_r, "null", 4 ) != 0 )
    {
        server_state.right_teamname_.assign( name_r,
                                            std::min( std::strlen( name_r ),
                                                      size_t( 16 ) ) );
    }

    // update match status

    server_state.current_time_ = time;

    //     updateScores( time,
    //                   server_state.playmode_,
    //                   score_l, score_r,
    //                   pen_score_l, pen_miss_l,
    //                   pen_score_r, pen_miss_r,
    //                   true );

    return true;
}

bool
SMonitorDevice::server_interpret_player_type_v3( BuilderBase * build,
                                                 const char * buf )
{
    int n_read = 0;

    int id = -1;
    double player_size = 0.3;
    double kickable_margin = 0.7;

    if ( std::sscanf( buf, " ( player_type ( id %d ) %n ",
                      &id, &n_read ) != 1
         || id < 0 )
    {
        ERROR_OUT << "\nIllegal player type id. ";
        return false;
    }
    buf += n_read;

    while ( *buf != '\0' )
    {
        if ( *buf == ')' ) break;

        char name[32];
        double value = 0.0;

        if ( std::sscanf( buf, " ( %31s %lf ) %n ",
                          name, &value,
                          &n_read ) != 2
             || n_read == 0 )
        {
            ERROR_OUT << "\nIllegal parameter in playr_type message.";
            break;
        }
        buf += n_read;

        if ( ! std::strncmp( name, "player_size",
                             std::strlen( "player_size" ) ) )
        {
            player_size = value;
        }
        else if ( ! std::strncmp( name, "kickable_margin",
                                  std::strlen( "kickable_margin" ) ) )
        {
            kickable_margin = value;
        }
    }

    static bool put_warning = false;

    if ( std::fabs( player_size ) < 0.01 )
    {
        if ( ! put_warning )
        {
            WARNING_OUT << "\nlogplayer seems not to send player types info ( check for version >= 7.08 )";
            PlayerTypes::use_std_type = true;
            put_warning = true;
        }
        guess.use_type = false;
        return false;
    }
    guess.use_type = true;
    guess.use_stamina = true;

    PlayerTypes::use_std_type = false;

    PlayerTypes::set_type( id, player_size, kickable_margin );

    return true;
}

bool
SMonitorDevice::server_interpret_server_param_v3( BuilderBase * build,
                                                  const char * buf )
{
    double goal_width = 14.02;
    double ball_size = 0.085;
    double ball_decay = 0.96;
    double player_size = 0.3;
    double kickable_margin = 0.7;

    std::map< std::string, double * > param_map;
    param_map[ "goal_width" ] = &goal_width;
    param_map[ "ball_size" ] = &ball_size;
    param_map[ "ball_decay" ] = &ball_decay;
    param_map[ "player_size" ] = &player_size;
    param_map[ "kickable_margin" ] = &kickable_margin;

    {
        char message_type[32];
        int n_read = 0;
        if ( std::sscanf( buf, " ( %31s %n ", message_type ) != 1
             || std::strncmp( message_type, "server_param",
                              std::strlen( "server_param" ) ) != 0 )
        {
            ERROR_OUT << "\nIllegal message type.";
            return false;
        }
        buf += n_read;
    }

    while ( *buf != '\0' )
    {
        char name[32];
        double value = 0.0;
        int n_read = 0;

        if ( std::sscanf( buf, " ( %31s %lf ) %n ",
                          name, &value, &n_read ) == 2 )
        {
            std::map< std::string, double * >::iterator
                it = param_map.find( name );
            if ( it != param_map.end() )
            {
                *( it->second ) = value;
            }

            buf += n_read;
        }
        else
        {
            // skip to the next parameter
            if ( *buf == '(' ) ++buf;
            while ( *buf != '\0' && *buf != '(' )
            {
                // skip quated value
                if ( *buf == '\"' )
                {
                    ++buf;
                    while ( *buf != '\0' && *buf != '\"' ) ++buf;
                    if ( *buf == '\"' ) ++buf;
                }
                else
                {
                    ++buf;
                }
            }
        }
    }

    if ( std::fabs( goal_width ) < 0.01 )
    {
        WARNING_OUT << "\nlogplayer seems not to send correct server parameters ( check for version >= 7.08 )";
        return false;
    }

    vis_field.set_goal_width( goal_width );
    vis_ball.set_ball_decay( ball_decay );

    PlayerTypes::init( ball_size, player_size, kickable_margin );

    return true;
}

void
SMonitorDevice::updatePlayMode( const int pmode )
{
    static const char * const PlayModeString[] = PLAYMODE_STRINGS;

    if ( server_state.playmode_ == pmode )
    {
        return;
    }

    server_state.playmode_ = pmode;

    if ( pmode >= 0 && pmode < SSrv::PLAYMODE_STRINGS_SIZE )
    {
        server_state.playmode_string_ = PlayModeString[pmode];
        if ( pmode == 2 )
        {
            M_timeover = true;
        }

        if ( pmode == 44 || pmode == 45 ) // pen_taken_[lr]
        {
            server_state.pen_taken_timer_ = std::max( 0, options.pen_taken_wait );
        }
    }
    else
    {
        char dum[30];
        std::snprintf( dum, 30, "sim_mode %d", pmode );
        server_state.playmode_string_ = dum;
    }
}

void
SMonitorDevice::updateScores( const int time,
                              const int pmode,
                              const int score_l,
                              const int score_r,
                              const int pen_score_l,
                              const int pen_miss_l,
                              const int pen_score_r,
                              const int pen_miss_r,
                              const bool has_pen_score )
{
    std::map< int, Score >::iterator it = M_scores.lower_bound( time );

    if ( it == M_scores.end()
         || it->second.left_score_ != score_l
         || it->second.right_score_ != score_r
         || ( has_pen_score
              && ( it->second.left_pen_score_ != pen_score_l
                   || it->second.right_pen_score_ != pen_score_r
                   || it->second.left_pen_miss_ != pen_miss_l
                   || it->second.right_pen_miss_ != pen_miss_r ) )
         )
    {
        M_scores.insert( std::pair< int, Score >( time,
                                                  Score( score_l, score_r,
                                                         pen_score_l, pen_miss_l,
                                                         pen_score_r, pen_miss_r ) ) );
        //std::cerr << '\n' << time << " added new score info " << std::endl;
        return;
    }

    static int s_last_pmode = 0;

    if ( s_last_pmode == pmode )
    {
        return;
    }
    s_last_pmode = pmode;

    // penalty score or miss, but no score information
    if ( 46 <= pmode && pmode <= 49
         && pen_score_l == 0
         && pen_score_r == 0
         && pen_miss_l == 0
         && pen_miss_r == 0 )
    {
        Score new_score( 0, 0, 0, 0, 0, 0 );

        if ( it != M_scores.end() )
        {
            new_score = it->second;
        }

        switch ( pmode ) {
        case 46: // penalty_miss_l
            new_score.left_pen_miss_ += 1;
            break;
        case 47: // penalty_miss_r
            new_score.right_pen_miss_ += 1;
            break;
        case 48: // penalty_score_l
            new_score.left_pen_score_ += 1;
            break;
        case 49: // penalty_score_r
            new_score.right_pen_score_ += 1;
            break;
        default:
            // no new score info
            return;
        }

        M_scores.insert( std::pair< int, Score >( time, new_score ) );
        //std::cerr << time << " added new penalty score info " << std::endl;
    }

}

void
SMonitorDevice::updateScoreBoard( const int time )
{
    char score_board_msg[512];

    std::map< int, Score >::iterator score = M_scores.lower_bound( time );

    if ( score == M_scores.end() )
    {
        std::snprintf( score_board_msg, 512,
                       " %10s %d:%d %-10s %16s %6d    ",
                       "", 0, 0, "", "", time );
    }
    else
    {
        //if ( ! score->second.hasPenaltyScore() )
        if ( ( 40 <= server_state.playmode_     // penalty_setup_l
               && server_state.playmode_ <= 49 ) // penalty_score_r
             || score->second.hasPenaltyScore() )
        {
            std::string left_penalty;
            std::string right_penalty;
            int l_s = 0, l_m = 0, r_s = 0, r_m = 0;
            for ( std::map< int, Score >::reverse_iterator it = M_scores.rbegin();
                  it != M_scores.rend();
                  ++it )
            {
                if ( it->first > time ) break;

                //                 std::cerr << it->first << ": "
                //                           << it->second.left_pen_score_ << ','
                //                           << it->second.left_pen_miss_ << '-'
                //                           << it->second.right_pen_score_ << ','
                //                           << it->second.right_pen_miss_ << std::endl;

                if ( it->second.left_pen_score_ != l_s )
                {
                    left_penalty += 'o';
                    l_s = it->second.left_pen_score_;
                }
                else if ( it->second.left_pen_miss_ != l_m )
                {
                    left_penalty += 'x';
                    l_m = it->second.left_pen_miss_;
                }
                else if ( it->second.right_pen_score_ != r_s )
                {
                    right_penalty += 'o';
                    r_s = it->second.right_pen_score_;
                }
                else if ( it->second.right_pen_miss_ != r_m )
                {
                    right_penalty += 'x';
                    r_m = it->second.right_pen_miss_;
                }
            }

            char pen_timer[32];
            std::memset( pen_timer, 0, 32 );
            if ( server_state.playmode_ == 44
                 || server_state.playmode_ == 45 ) // pen_taken_[lr]
            {
                std::snprintf( pen_timer, 32, "%4d", server_state.pen_taken_timer_ );
                server_state.pen_taken_timer_ = std::max( 0, server_state.pen_taken_timer_ - 1 );
            }

            std::snprintf( score_board_msg, 512,
                           " %10s %d:%d |%-5s:%-5s| %-10s %16s%s %6d ",
                           server_state.left_teamname_.c_str(),
                           score->second.left_score_,
                           score->second.right_score_,
                           left_penalty.c_str(),
                           right_penalty.c_str(),
                           server_state.right_teamname_.c_str(),
                           server_state.playmode_string_.c_str(),
                           pen_timer,
                           time );

            //             std::snprintf( score_board_msg, 512,
            //                            " %10s %d(%d/%d):%d(%d/%d) %-10s %16s %6d",
            //                            server_state.left_teamname_.c_str(),
            //                            score->second.left_score_,
            //                            score->second.left_pen_score_,
            //                            score->second.left_pen_score_ + score->second.left_pen_miss_,
            //                            score->second.right_score_,
            //                            score->second.right_pen_score_,
            //                            score->second.right_pen_score_ + score->second.right_pen_miss_,
            //                            server_state.right_teamname_.c_str(),
            //                            server_state.playmode__string.c_str(),
            //                            time );
        }
        else
        {
            std::snprintf( score_board_msg, 512,
                           " %10s %d:%d %-10s %16s %6d    ",
                           server_state.left_teamname_.c_str(),
                           score->second.left_score_,
                           score->second.right_score_,
                           server_state.right_teamname_.c_str(),
                           server_state.playmode_string_.c_str(),
                           time );
        }
    }

    M_score_board_string = score_board_msg;
}

void
SMonitorDevice::set_object_pos( BuilderBase * build,
                                int o_id,
                                const Point2d & pos )
{
    if ( o_ball( o_id ) )
    {
        server_pos.set_ball_pos( pos ); //just to collect informations
        build->set_cmd_set_frame_pos( frame_ball,pos );
        return;
    }

    server_pos.set_player_pos( o_id,pos ); //just to collect informations
    build->set_cmd_set_frame_pos( p_frame( o_id ),pos );

    if ( !server_pos.player_alive( o_id ) ) {
        server_pos.set_player_alive( o_id );
        vis_player[o_id].set_active();
        vis_player[o_id].set_goalie( p_number( o_id ) == 1 ); //assume goalie has number 1 in his team
    }
}

void
SMonitorDevice::set_all_objects_scale( BuilderBase * build,
                                       double scale )
{
    build->set_cmd_set_frame_scale( frame_ball, scale );
    build->set_cmd_set_frame_scale( frame_shadow, scale );
    for ( int i = 0; i < 2*MAX_PLAYER; ++i )
    {
        build->set_cmd_set_frame_scale( p_frame( i ), scale );
    }
}

void
SMonitorDevice::send_object_pos( int o_id,
                                 const Point2d & pos )
{
    if ( o_ball( o_id ) )
    {
        send_dispball( pos );
        return;
    }

#ifdef HAVE_SSTREAM
    std::ostringstream dum_str;
#else
    std::ostrstream dum_str;
#endif

#if 0 // #if USE_COACH for server version < 7.02
    dum_str << "(move ( player ";
    if ( p_left( o_id ) )
    {
        dum_str << server_state.left_teamname_;
    }
    else
    {
        dum_str << server_state.right_teamname_;
    }
    dum_str << " " << p_number( o_id ) << " ) " << int( pos.x )
            << " " <<  int( -pos.y ) << " )";

#ifdef HAVE_SSTREAM
    coach.send_msg( dum_str.str().c_str(), dum_str.str().length() );
#else
    dum_str << std::ends;
    coach.send_msg( dum_str.str(), std::strlen( dum_str.str() ) );
    dum_str.freeze( false );
#endif

#else
    /* don't use the coach to position players but the
       ( dispplayer ... ) command ( works in server 7.02 upwards ) */
    dum_str << "(dispplayer";
    if ( p_left( o_id ) )
    {
        dum_str << " 1";
    }
    else
    {
        dum_str << " -1";
    }

    //preserve the angle of the player
    int tmp = int( -( server_pos.player[o_id].body_angle.get_value()- 2*M_PI ) * 180.0/M_PI );
    if ( tmp > 180 )
        tmp -= 360;
    //std::cout << "\n body agnle = " << int ( tmp ) << std::flush; //don't change angle

    dum_str << " " << p_number( o_id ) << " "
            << int( pos.x * SHOWINFO_SCALE ) << " " <<  int( -pos.y * SHOWINFO_SCALE )
            << " 0 0 "
            << tmp << " )";
#ifdef HAVE_SSTREAM
    server.send_msg( dum_str.str().c_str(), dum_str.str().length() );
#else
    dum_str << std::ends;
    server.send_msg( dum_str.str(), std::strlen( dum_str.str() ) );
    dum_str.freeze( false );
#endif
#endif
}

void
SMonitorDevice::send_dispinit()
{
    if ( options.protocol_version >= 2 )
    {
        //const char msg[] = "(dispinit version 2)";
        char msg[32];
        std::snprintf( msg, 32,
                       "(dispinit version %d)",
                       options.protocol_version );
        server.send_msg( msg, std::strlen( msg ) + 1 );
    }
    else
    {
        const char msg[] = "(dispinit)";
        server.send_msg( msg, std::strlen( msg ) + 1 );
        //std::cout << "\ntime =" << TOOLS::get_current_ms_time() << ", sending msg =:" << msg << flush;
    }

    M_scores.clear();
}

void
SMonitorDevice::send_dispstart()
{
    const char msg[] = "(dispstart)";
    server.send_msg( msg, std::strlen( msg ) + 1 );
}

void
SMonitorDevice::send_dispball( const Point2d & pos )
{
    char buf[100];
    std::snprintf( buf, 100,
                   "(dispfoul %d %d 0)",
                   int( pos.x * SHOWINFO_SCALE ),
                   int( - pos.y * SHOWINFO_SCALE ) );
    server.send_msg( buf, std::strlen( buf ) + 1 );
}

void
SMonitorDevice::send_dispfoul_left( const Point2d & pos )
{
    char buf[100];
    std::snprintf( buf, 100,
                   "(dispfoul %d %d 1)",
                   int( pos.x * SHOWINFO_SCALE ),
                   int( - pos.y * SHOWINFO_SCALE ) );
    server.send_msg( buf, std::strlen( buf ) + 1 );
}

void
SMonitorDevice::send_dispfoul_right( const Point2d & pos )
{
    static char buf[100];
    std::snprintf( buf, 100,
                   "(dispfoul %d %d -1)",
                   int( pos.x * SHOWINFO_SCALE ),
                   int( - pos.y * SHOWINFO_SCALE ) );
    server.send_msg( buf, std::strlen( buf ) + 1 );
}

void
SMonitorDevice::send_dispbye()
{
    const char msg[] = "(dispbye)";
    server.send_msg( msg, std::strlen( msg ) + 1 );
}

/*
  if ( false && i == 0 )
  std::cout << "\nl_body_angle = " << l_body_angle
  << ", res = " << double( l_body_angle ) / SHOWINFO_SCALE2
  << ", res2 = " << ang_SERVER_2_LP ( double( l_body_angle ) / SHOWINFO_SCALE2 )
  << "\nl_head_angle = " << l_head_angle
  << ", res = " << double( l_head_angle ) / SHOWINFO_SCALE2
  << ", res2 = " << ang_SERVER_2_LP ( double( l_head_angle ) / SHOWINFO_SCALE2 ) << flush;

  if ( false && i == 0 ) {
  std::cout << "\n-- time ( " << ntohs( showinfo.time ) << " )"
  << "\ntype = " << ntohs( showinfo.pos[i].type )
  << "\nstamina = " << double( ntohl( showinfo.pos[i].stamina ) ) /SHOWINFO_SCALE2
  << "\neffort = " << double( ntohl( showinfo.pos[i].effort ) ) / SHOWINFO_SCALE2
  << "\nrecovery = " << double( ntohl( showinfo.pos[i].recovery ) ) / SHOWINFO_SCALE2
  << "\nk = " << ntohs( showinfo.pos[i].kick_count )
  << " , d = " << ntohs( showinfo.pos[i].dash_count )
  << " , t = " << ntohs( showinfo.pos[i].turn_count )
  << " , m = " << ntohs( showinfo.pos[i].move_count )
  << " ; c = " << ntohs( showinfo.pos[i].catch_count )
  << " | n = " << ntohs( showinfo.pos[i].tneck_count )
  << " | s = " << ntohs( showinfo.pos[i].say_count )
  << " | v = " << ntohs( showinfo.pos[i].chg_view_count );
  }
*/
