// -*-c++-*-

/*!
  \file parser_v4.cpp
  \brief rcg v4 parser Source File.
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

#include "parser_v4.h"

#include "handler.h"
#include "types.h"

#include <map>
#include <iomanip>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <climits>
#include <cstdio>

namespace rcss {
namespace rcg {

typedef std::map< std::string, int * > IntMap;
typedef std::map< std::string, double * > DoubleMap;
typedef std::map< std::string, bool * > BoolMap;
typedef std::map< std::string, std::string * > StringMap;

namespace {

/*-------------------------------------------------------------------*/
// remove quatation characters
std::string
clean_string( std::string str )
{
    if( str.empty() )
    {
        return str;
    }

    if ( *str.begin() == '\'' )
    {
        if( *str.rbegin() == '\''  )
        {
            str = str.substr( 1, str.length() - 2 );
        }
        else
        {
            return str;
        }

        // replace escape character
        for ( std::string::size_type escape_pos = str.find( "\\'" );
              escape_pos != std::string::npos;
              escape_pos = str.find( "\\'" ) )
        {
            // replace "\'" with "'"
            str.replace( escape_pos, 2, "'" );
        }
    }
    else if ( *str.begin() == '"' )
    {
        if ( *str.rbegin() == '"'  )
        {
            str = str.substr( 1, str.length() - 2 );
        }
        else
        {
            return str;
        }

        // replace escape character
        for( std::string::size_type escape_pos = str.find( "\\\"" );
             escape_pos != std::string::npos;
             escape_pos = str.find( "\\\"" ) )
        {
            // replace "\"" with """
            str.replace( escape_pos, 2, "\"" );
        }
    }

    return str;
}

/*-------------------------------------------------------------------*/
bool
parse_param( const int n_line,
             const std::string & line,
             IntMap & int_map,
             DoubleMap & double_map,
             BoolMap & bool_map,
             StringMap & string_map )
{
    int n_read = 0;

    char message_name[32];
    if ( std::sscanf( line.c_str(), " ( %31s %n ", message_name, &n_read ) != 1 )
    {
        std::cerr << ":error: failed to parse the message type." << std::endl;
        return false;
    }

    for ( std::string::size_type pos = line.find_first_of( '(', n_read );
          pos != std::string::npos;
          pos = line.find_first_of( '(', pos ) )
    {
        std::string::size_type end_pos = line.find_first_of( ' ', pos );
        if ( end_pos == std::string::npos )
        {
            std::cerr << n_line << ": Failed to find the seprator space."
                      << std::endl;
            return false;
        }
        pos += 1;

        const std::string name_str( line, pos, end_pos - pos );

        pos = end_pos; // pos indcates the position of the white space after the param name

        // search end paren or double quatation
        end_pos = line.find_first_of( ")\"", end_pos ); //"
        if ( end_pos == std::string::npos )
        {
            std::cerr << n_line << ": Failed to find the parameter value for [" << name_str << ']'
                      << std::endl;
            return false;
        }

        // found quated value
        if ( line[end_pos] == '\"' )
        {
            pos = end_pos;
            end_pos = line.find_first_of( '\"', end_pos + 1 ); //"
            if ( end_pos == std::string::npos )
            {
                std::cerr << n_line << ": Failed to parse the quated value for [" << name_str << ']'
                          << std::endl;
                return false;
            }
            end_pos += 1; // skip double quatation
        }
        else
        {
            pos += 1; // skip the white space after the param name
        }

        // pos indicates the first position of the value string
        // end_pos indicates the position of the end of paren

        std::string value_str( line, pos, end_pos - pos );
        pos = end_pos;

        // pos indicates the position of the end of paren

        //
        // check parameter maps
        //
        try
        {
            IntMap::iterator int_it = int_map.find( name_str );
            if ( int_it != int_map.end() )
            {
                *(int_it->second) = std::stoi( value_str );
                continue;
            }

            DoubleMap::iterator double_it = double_map.find( name_str );
            if ( double_it != double_map.end() )
            {
                *(double_it->second) = std::stod( value_str );
                continue;
            }

            BoolMap::iterator bool_it = bool_map.find( name_str );
            if ( bool_it != bool_map.end() )
            {
                if ( value_str == "0" || value_str == "false" || value_str == "off" )
                {
                    *(bool_it->second) = false;
                }
                else if ( value_str == "1" || value_str == "true" || value_str == "on" )
                {
                    *(bool_it->second) = true;
                }
                else
                {
                    std::cerr << n_line << ": Unknown bool value. name=" << name_str << " value=" << value_str
                              << std::endl;
                }
                continue;
            }

            StringMap::iterator string_it = string_map.find( name_str );
            if ( string_it != string_map.end() )
            {
                *(string_it->second) = clean_string( value_str );
                continue;
            }

            std::cerr << n_line << ": Unsupported parameter in " << message_name
                      << " name=" << name_str << " value=" << value_str
                      << std::endl;
        }
        catch ( std::exception & e )
        {
            std::cerr << e.what() << '\n'
                      << n_line << ":  name=" << name_str << " value=" << value_str
                      << std::endl;
        }
    }

    return true;
}

}


/*-------------------------------------------------------------------*/
bool
ParserV4::parse( std::istream & is,
                 Handler & handler )
{
    // streampos must be the first point!!!
    is.seekg( 0 );

    if ( ! is.good() )
    {
        return false;
    }

    std::string line;
    line.reserve( 8192 );

    // skip header line
    if ( ! std::getline( is, line )
         || line.length() < 4
         || line.compare( 0, 3, "ULG" ) != 0 )
    {
        return false;
    }

    const int version = std::stoi( line.substr( 3 ) );
    if ( version != REC_VERSION_4
         && version != REC_VERSION_5 )
    {
        return false;
    }

    if ( ! handler.handleLogVersion( version ) )
    {
        return false;
    }

    int n_line = 1;
    while ( std::getline( is, line ) )
    {
        ++n_line;
        if ( ! parseLine( n_line, line, handler ) )
        {
            return false;
        }
    }

    if ( ! is.eof() )
    {
        return false;
    }

    return handler.handleEOF();
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parseLine( const int n_line,
                     const std::string & line,
                     Handler & handler )
{
    char name[32];

    if ( std::sscanf( line.c_str(), " ( %s ", name ) != 1 )
    {
        std::cerr << n_line << ": Illegal line. [" << line << ']'
                  << std::endl;
        return false;
    }

    if ( ! std::strcmp( name, "show" ) )
    {
        parseShow( n_line, line, handler );
    }
    else if ( ! std::strcmp( name, "playmode" ) )
    {
        parsePlayMode( n_line, line, handler );
    }
    else if ( ! std::strcmp( name, "team" ) )
    {
        parseTeam( n_line, line, handler );
    }
    else if ( ! std::strcmp( name, "msg" ) )
    {
        parseMsg( n_line, line, handler );
    }
    else if ( ! std::strcmp( name, "player_type" ) )
    {
        parsePlayerType( n_line, line, handler );
    }
    else if ( ! std::strcmp( name, "player_param" ) )
    {
        parsePlayerParam( n_line, line, handler );
    }
    else if ( ! std::strcmp( name, "server_param" ) )
    {
        parseServerParam( n_line, line, handler );
    }
    else
    {
        std::cerr << n_line << ": Unknown message type. name=" << name << " line=[" << line << ']'
                  << std::endl;;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parseShow( const int n_line,
                     const std::string & line,
                     Handler & handler )
{
    /*
      (show <Time> <Ball> <Players>)
    */

    ShowInfoT show;

    const char * buf = line.c_str();
    char * next = nullptr;

    //
    // time
    //
    while ( *buf == ' ' ) ++buf;
    while ( *buf != '\0' && *buf != ' ' ) ++buf;
    long time = std::strtol( buf, &next, 10 ); buf = next;

    if ( time == LONG_MIN || time == LONG_MAX )
    {
        std::cerr << n_line << ": error: "
                  << " Illegal show info time. "
                  << " \"" << line << "\""
                  << std::endl;
        return false;
    }

    show.time_ = static_cast< UInt32 >( time );

    while ( *buf == ' ' ) ++buf;

    //
    // playmode
    //
    if ( ! std::strncmp( buf, "(pm", 3 ) )
    {
        int n_read = 0;
        int pm = 0;
        if ( std::sscanf( buf,
                          " ( pm %d ) %n ",
                          &pm, &n_read ) == 1 )
        {
            buf += n_read;
            handler.handlePlayMode( time, static_cast< PlayMode >( pm ) );
        }
    }

    //
    // team
    //
    if ( ! std::strncmp( buf, "(tm", 3 ) )
    {
        char name_l[32], name_r[32];
        int score_l = 0, score_r = 0;
        int pen_score_l = 0, pen_miss_l = 0, pen_score_r = 0, pen_miss_r = 0;

        int n = std::sscanf( buf,
                             " ( tm %31s %31s %d %d %d %d %d %d ",
                             name_l, name_r,
                             &score_l, &score_r,
                             &pen_score_l, &pen_miss_l,
                             &pen_score_r, &pen_miss_r );

        if ( n != 4 && n != 8 )
        {
            std::cerr << n_line << ": error: n=" << n << ' '
                      << "Illegal team info. \"" << line << "\"" << std::endl;;
            return false;
        }
        while ( *buf != ')' && *buf != '\0' ) ++buf;
        while ( *buf == ')' && *buf != '\0' ) ++buf;

        if ( ! std::strcmp( name_l, "null" ) ) std::memset( name_l, 0, 4 );
        if ( ! std::strcmp( name_r, "null" ) ) std::memset( name_r, 0, 4 );

        TeamT team_l( name_l, score_l, pen_score_l, pen_miss_l );
        TeamT team_r( name_r, score_r, pen_score_r, pen_miss_r );

        handler.handleTeam( time, team_l, team_r );
    }

    // ball
    {
        // ((b) x y vx vy)
        while ( *buf == ' ' ) ++buf;
        while ( *buf != '\0' && *buf != ')' ) ++buf; // skip "((b"
        while ( *buf == ')' ) ++buf;  // skip "((b)"
        BallT & ball = show.ball_;
        ball.x_ = strtof( buf, &next ); buf = next;
        ball.y_ = strtof( buf, &next ); buf = next;
        ball.vx_ = strtof( buf, &next ); buf = next;
        ball.vy_ = strtof( buf, &next ); buf = next;
        while ( *buf == ')' ) ++buf;
        while ( *buf == ' ' ) ++buf;

        if ( ball.vy_ == HUGE_VALF )
        {
            std::cerr << n_line << ": error: "
                      << " Illegal ball info. "
                      << " \"" << line << "\""
                      << std::endl;;
            return false;
        }
    }

    // players
    // ((side unum) type state x y vx vy body neck [pointx pointy] (v h 90) (s 4000 1 1)[(f side unum)])
    //              (c 1 1 1 1 1 1 1 1 1 1 1))
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        if ( *buf == '\0' || *buf == ')' ) break;

        // ((side unum)
        while ( *buf == ' ' ) ++buf;
        while ( *buf == '(' ) ++buf;

        const char side = *buf;
        if ( side != 'l' && side != 'r' )
        {
            std::cerr << n_line << ": error: "
                      << " Illegal player side. " << side << ' ' << i
                      << " \"" << buf << "\""
                      << std::endl;;
            return false;
        }

        ++buf;
        const long unum = std::strtol( buf, &next, 10 ); buf = next;
        if ( unum < 0 || MAX_PLAYER < unum )
        {
            std::cerr << n_line << ": error: "
                      << " Illegal player unum. " << side << ' ' << i
                      << " \"" << buf << "\""
                      << std::endl;;
            return false;
        }

        while ( *buf == ')' ) ++buf;

        const int idx = ( side == 'l' ? unum - 1 : unum - 1 + MAX_PLAYER );

        PlayerT & p = show.players_[idx];
        p.side_ = ( side == 'l' ? LEFT : side == 'r' ? RIGHT : NEUTRAL );
        p.unum_ = static_cast< Int16 >( unum );

        // x y vx vy body neck
        p.type_ = static_cast< Int16 >( std::strtol( buf, &next, 10 ) ); buf = next;
        p.state_ = static_cast< Int32 >( std::strtol( buf, &next, 16 ) ); buf = next;
        p.x_ = strtof( buf, &next ); buf = next;
        p.y_ = strtof( buf, &next ); buf = next;
        p.vx_ = strtof( buf, &next ); buf = next;
        p.vy_ = strtof( buf, &next ); buf = next;
        p.body_ = strtof( buf, &next ); buf = next;
        p.neck_ = strtof( buf, &next ); buf = next;
        while ( *buf == ' ' ) ++buf;

        // x y vx vy body neck
        if ( *buf != '\0' && *buf != '(' )
        {
            p.point_x_ = strtof( buf, &next ); buf = next;
            p.point_y_ = strtof( buf, &next ); buf = next;
        }

        // (v quality width)
        while ( *buf != '\0' && *buf != 'v' ) ++buf;
        ++buf; // skip 'v'
        while ( *buf == ' ' ) ++buf;
        p.high_quality_ = ( *buf == 'h' ? true : false ); ++buf;
        p.view_width_ = strtof( buf, &next ); buf = next;

        // (s stamina effort recovery[ capacity])
        while ( *buf != '\0' && *buf != 's' ) ++buf;
        ++buf; // skip 's' //while ( *buf != '\0' && *buf != ' ' ) ++buf;
        p.stamina_ = strtof( buf, &next ); buf = next;
        p.effort_ = strtof( buf, &next ); buf = next;
        p.recovery_ = strtof( buf, &next ); buf = next;
        while ( *buf == ' ' ) ++buf;
        if ( *buf != ')' )
        {
            p.stamina_capacity_ = strtof( buf, &next ); buf = next;
        }
        while ( *buf != '\0' && *buf != ')' ) ++buf;
        while ( *buf == ')' ) ++buf;

        while ( *buf != '\0' && *buf != '(' ) ++buf;

        // (f side unum)
        if ( *(buf + 1) == 'f' )
        {
            while ( *buf != '\0' && *buf != ' ' ) ++buf;
            while ( *buf == ' ' ) ++buf;
            p.focus_side_ = ( *buf == 'l' ? LEFT : *buf == 'r' ? RIGHT : NEUTRAL ); ++buf;
            p.focus_unum_ = static_cast< Int16 >( std::strtol( buf, &next, 10 ) ); buf = next;
            while ( *buf == ' ' ) ++buf;
            while ( *buf == ')' ) ++buf;
            while ( *buf == ' ' ) ++buf;
        }

        // (c kick dash turn catch move tneck cview say tackle pointto atttention)
        while ( *buf == '(' ) ++buf;
        ++buf; // skip 'c' //while ( *buf != '\0' && *buf != ' ' ) ++buf;
        p.kick_count_ = static_cast< UInt16 >( std::strtol( buf, &next, 10 ) ); buf = next;
        p.dash_count_ = static_cast< UInt16 >( std::strtol( buf, &next, 10 ) ); buf = next;
        p.turn_count_ = static_cast< UInt16 >( std::strtol( buf, &next, 10 ) ); buf = next;
        p.catch_count_ = static_cast< UInt16 >( std::strtol( buf, &next, 10 ) ); buf = next;
        p.move_count_ = static_cast< UInt16 >( std::strtol( buf, &next, 10 ) ); buf = next;
        p.turn_neck_count_ = static_cast< UInt16 >( std::strtol( buf, &next, 10 ) ); buf = next;
        p.change_view_count_ = static_cast< UInt16 >( std::strtol( buf, &next, 10 ) ); buf = next;
        p.say_count_ = static_cast< UInt16 >( std::strtol( buf, &next, 10 ) ); buf = next;
        p.tackle_count_ = static_cast< UInt16 >( std::strtol( buf, &next, 10 ) ); buf = next;
        p.pointto_count_ = static_cast< UInt16 >( std::strtol( buf, &next, 10 ) ); buf = next;
        p.attentionto_count_ = static_cast< UInt16 >( std::strtol( buf, &next, 10 ) ); buf = next;
        while ( *buf == ')' ) ++buf;
        while ( *buf == ' ' ) ++buf;

        if ( *buf == '\0'
             && i != MAX_PLAYER*2 - 1 )
        {
            std::cerr << n_line << ": error: "
                      << " Illegal player info. " << side << ' ' << unum
                      << " \"" << line << "\""
                      << std::endl;;
            return false;
        }
    }

    return handler.handleShow( show );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parseMsg( const int n_line,
                    const std::string & line,
                    Handler & handler )
{
    /*
      (msg <Time> <Board> "<Message>")
    */
    int n_read = 0;
    int time = 0;
    int board = 0;

    if ( std::sscanf( line.c_str(),
                      " ( msg %d %d \"%n",
                      &time, &board, &n_read ) != 2 )
    {
        std::cerr << n_line << ": error: "
                  << "Illegal msg line. \"" << line << "\"" << std::endl;;
        return false;
    }

    std::string msg( line, n_read, std::string::npos );
    if ( msg.length() <= 2 ) // at least, [")] + 1 char
    {
        return false;
    }

    std::string::size_type pos = msg.rfind( "\")" );
    if ( pos == std::string::npos )
    {
        std::cerr << n_line << ": error: "
                  << "Illegal msg [" << line << "]" << std::endl;;
        return false;
    }

    msg.erase( pos );

    return handler.handleMsg( time, board, msg );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parsePlayMode( const int n_line,
                         const std::string & line,
                         Handler & handler )
{
    /*
      (playmode <Time> <Playmode>)
    */

    static const char * playmode_strings[] = PLAYMODE_STRINGS;

    int time = 0;
    char pm_string[32];

    if ( std::sscanf( line.c_str(),
                      " ( playmode %d %31[^)] ) ",
                      &time, pm_string ) != 2 )
    {
        std::cerr << n_line << ": error: "
                  << "Illegal playmode line. \"" << line << "\"" << std::endl;;
        return false;
    }

    PlayMode pm = PM_Null;
    for ( int n = 0; n < PM_MAX; ++n )
    {
        if ( ! std::strcmp( playmode_strings[n], pm_string ) )
        {
            pm = static_cast< PlayMode >( n );
            break;
        }
    }

    return handler.handlePlayMode( time, pm );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parseTeam( const int n_line,
                     const std::string & line,
                     Handler & handler )
{
    int time = 0;
    char name_l[32], name_r[32];
    int score_l = 0, score_r = 0;
    int pen_score_l = 0, pen_miss_l = 0, pen_score_r = 0, pen_miss_r = 0;

    int n = std::sscanf( line.c_str(),
                         " ( team %d %31s %31s %d %d %d %d %d %d ",
                         &time,
                         name_l, name_r,
                         &score_l, &score_r,
                         &pen_score_l, &pen_miss_l,
                         &pen_score_r, &pen_miss_r );

    if ( n != 5 && n != 9 )
    {
        std::cerr << n_line << ": error: "
                  << "Illegal team line. \"" << line << "\"" << std::endl;;
        return false;
    }

    TeamT team_l( name_l, score_l, pen_score_l, pen_miss_l );
    TeamT team_r( name_r, score_r, pen_score_r, pen_miss_r );

    return handler.handleTeam( time, team_l, team_r );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parseServerParam( const int n_line,
                            const std::string & line,
                            Handler & handler )
{
    ServerParamT param;

    //
    // prepare param map
    //
    IntMap int_map;
    DoubleMap double_map;
    BoolMap bool_map;
    StringMap string_map;

    double_map.insert( DoubleMap::value_type( "goal_width", &param.goal_width_ ) );
    double_map.insert( DoubleMap::value_type( "inertia_moment", &param.inertia_moment_ ) );
    double_map.insert( DoubleMap::value_type( "player_size", &param.player_size_ ) );
    double_map.insert( DoubleMap::value_type( "player_decay", &param.player_decay_ ) );
    double_map.insert( DoubleMap::value_type( "player_rand", &param.player_rand_ ) );
    double_map.insert( DoubleMap::value_type( "player_weight", &param.player_weight_ ) );
    double_map.insert( DoubleMap::value_type( "player_speed_max", &param.player_speed_max_ ) );
    double_map.insert( DoubleMap::value_type( "player_accel_max", &param.player_accel_max_ ) );
    double_map.insert( DoubleMap::value_type( "stamina_max", &param.stamina_max_ ) );
    double_map.insert( DoubleMap::value_type( "stamina_inc_max", &param.stamina_inc_max_ ) );
    double_map.insert( DoubleMap::value_type( "recover_init", &param.recover_init_ ) ); // not necessary
    double_map.insert( DoubleMap::value_type( "recover_dec_thr", &param.recover_dec_thr_ ) );
    double_map.insert( DoubleMap::value_type( "recover_min", &param.recover_min_ ) );
    double_map.insert( DoubleMap::value_type( "recover_dec", &param.recover_dec_ ) );
    double_map.insert( DoubleMap::value_type( "effort_init", &param.effort_init_ ) );
    double_map.insert( DoubleMap::value_type( "effort_dec_thr", &param.effort_dec_thr_ ) );
    double_map.insert( DoubleMap::value_type( "effort_min", &param.effort_min_ ) );
    double_map.insert( DoubleMap::value_type( "effort_dec", &param.effort_dec_ ) );
    double_map.insert( DoubleMap::value_type( "effort_inc_thr", &param.effort_inc_thr_ ) );
    double_map.insert( DoubleMap::value_type( "effort_inc",  &param.effort_inc_ ) );
    double_map.insert( DoubleMap::value_type( "kick_rand", &param.kick_rand_ ) );
    bool_map.insert( BoolMap::value_type( "team_actuator_noise", &param.team_actuator_noise_ ) );
    double_map.insert( DoubleMap::value_type( "prand_factor_l", &param.player_rand_factor_l_ ) );
    double_map.insert( DoubleMap::value_type( "prand_factor_r", &param.player_rand_factor_r_ ) );
    double_map.insert( DoubleMap::value_type( "kick_rand_factor_l", &param.kick_rand_factor_l_ ) );
    double_map.insert( DoubleMap::value_type( "kick_rand_factor_r", &param.kick_rand_factor_r_ ) );
    double_map.insert( DoubleMap::value_type( "ball_size", &param.ball_size_ ) );
    double_map.insert( DoubleMap::value_type( "ball_decay", &param.ball_decay_ ) );
    double_map.insert( DoubleMap::value_type( "ball_rand", &param.ball_rand_ ) );
    double_map.insert( DoubleMap::value_type( "ball_weight", &param.ball_weight_ ) );
    double_map.insert( DoubleMap::value_type( "ball_speed_max", &param.ball_speed_max_ ) );
    double_map.insert( DoubleMap::value_type( "ball_accel_max", &param.ball_accel_max_ ) );
    double_map.insert( DoubleMap::value_type( "dash_power_rate", &param.dash_power_rate_ ) );
    double_map.insert( DoubleMap::value_type( "kick_power_rate", &param.kick_power_rate_ ) );
    double_map.insert( DoubleMap::value_type( "kickable_margin", &param.kickable_margin_ ) );
    double_map.insert( DoubleMap::value_type( "control_radius", &param.control_radius_ ) );
    //( "control_radius_width", &param.control_radius_width_ ) );
    //( "kickable_area", &param.kickable_area_ ) ); // not needed
    double_map.insert( DoubleMap::value_type( "catch_probability", &param.catch_probability_ ) );
    double_map.insert( DoubleMap::value_type( "catchable_area_l", &param.catchable_area_l_ ) );
    double_map.insert( DoubleMap::value_type( "catchable_area_w", &param.catchable_area_w_ ) );
    int_map.insert( IntMap::value_type( "goalie_max_moves", &param.goalie_max_moves_ ) );
    double_map.insert( DoubleMap::value_type( "maxpower", &param.max_power_ ) );
    double_map.insert( DoubleMap::value_type( "minpower", &param.min_power_ ) );
    double_map.insert( DoubleMap::value_type( "maxmoment", &param.max_moment_ ) );
    double_map.insert( DoubleMap::value_type( "minmoment", &param.min_moment_ ) );
    double_map.insert( DoubleMap::value_type( "maxneckmoment", &param.max_neck_moment_ ) );
    double_map.insert( DoubleMap::value_type( "minneckmoment", &param.min_neck_moment_ ) );
    double_map.insert( DoubleMap::value_type( "maxneckang", &param.max_neck_angle_ ) );
    double_map.insert( DoubleMap::value_type( "minneckang", &param.min_neck_angle_ ) );
    double_map.insert( DoubleMap::value_type( "visible_angle", &param.visible_angle_ ) );
    double_map.insert( DoubleMap::value_type( "visible_distance", &param.visible_distance_ ) );
    double_map.insert( DoubleMap::value_type( "audio_cut_dist", &param.audio_cut_dist_ ) );
    double_map.insert( DoubleMap::value_type( "quantize_step", &param.dist_quantize_step_ ) );
    double_map.insert( DoubleMap::value_type( "quantize_step_l", &param.landmark_dist_quantize_step_ ) );
    //( "quantize_step_dir", &param.dir_quantize_step_ ) );
    //( "quantize_step_dist_team_l", &param.dist_quantize_step_l_ ) );
    //( "quantize_step_dist_team_r", &param.dist_quantize_step_r_ ) );
    //( "quantize_step_dist_l_team_l", &param.landmark_dist_quantize_step_l_ ) );
    //( "quantize_step_dist_l_team_r", &param.landmark_dist_quantize_step_r_ ) );
    //( "quantize_step_dir_team_l", &param.dir_quantize_step_l_ ) );
    //( "quantize_step_dir_team_r", &param.dir_quantize_step_r_ ) );
    double_map.insert( DoubleMap::value_type( "ckick_margin", &param.corner_kick_margin_ ) );
    double_map.insert( DoubleMap::value_type( "wind_dir", &param.wind_dir_ ) );
    double_map.insert( DoubleMap::value_type( "wind_force", &param.wind_force_ ) );
    double_map.insert( DoubleMap::value_type( "wind_ang", &param.wind_angle_ ) );
    double_map.insert( DoubleMap::value_type( "wind_rand", &param.wind_rand_ ) );
    bool_map.insert( BoolMap::value_type( "wind_none", &param.wind_none_ ) );
    bool_map.insert( BoolMap::value_type( "wind_random", &param.use_wind_random_ ) );
    int_map.insert( IntMap::value_type( "half_time", &param.half_time_ ) );
    int_map.insert( IntMap::value_type( "drop_ball_time", &param.drop_ball_time_ ) );
    int_map.insert( IntMap::value_type( "port", &param.port_ ) );
    int_map.insert( IntMap::value_type( "coach_port", &param.coach_port_ ) );
    int_map.insert( IntMap::value_type( "olcoach_port", &param.online_coach_port_ ) );
    int_map.insert( IntMap::value_type( "say_coach_cnt_max", &param.say_coach_count_max_ ) );
    int_map.insert( IntMap::value_type( "say_coach_msg_size", &param.say_coach_msg_size_ ) );
    int_map.insert( IntMap::value_type( "simulator_step", &param.simulator_step_ ) );
    int_map.insert( IntMap::value_type( "send_step", &param.send_step_ ) );
    int_map.insert( IntMap::value_type( "recv_step", &param.recv_step_ ) );
    int_map.insert( IntMap::value_type( "sense_body_step", &param.sense_body_step_ ) );
    //( "lcm_step", &param.lcm_step_ ) ); // not needed
    int_map.insert( IntMap::value_type( "say_msg_size", &param.say_msg_size_ ) );
    int_map.insert( IntMap::value_type( "clang_win_size", &param.clang_win_size_ ) );
    int_map.insert( IntMap::value_type( "clang_define_win", &param.clang_define_win_ ) );
    int_map.insert( IntMap::value_type( "clang_meta_win", &param.clang_meta_win_ ) );
    int_map.insert( IntMap::value_type( "clang_advice_win", &param.clang_advice_win_ ) );
    int_map.insert( IntMap::value_type( "clang_info_win", &param.clang_info_win_ ) );
    int_map.insert( IntMap::value_type( "clang_del_win", &param.clang_del_win_ ) );
    int_map.insert( IntMap::value_type( "clang_rule_win", &param.clang_rule_win_ ) );
    int_map.insert( IntMap::value_type( "clang_mess_delay", &param.clang_mess_delay_ ) );
    int_map.insert( IntMap::value_type( "clang_mess_per_cycle", &param.clang_mess_per_cycle_ ) );
    int_map.insert( IntMap::value_type( "hear_max", &param.hear_max_ ) );
    int_map.insert( IntMap::value_type( "hear_inc", &param.hear_inc_ ) );
    int_map.insert( IntMap::value_type( "hear_decay", &param.hear_decay_ ) );
    int_map.insert( IntMap::value_type( "catch_ban_cycle", &param.catch_ban_cycle_ ) );
    bool_map.insert( BoolMap::value_type( "coach", &param.coach_mode_ ) );
    bool_map.insert( BoolMap::value_type( "coach_w_referee", &param.coach_with_referee_mode_ ) );
    bool_map.insert( BoolMap::value_type( "old_coach_hear", &param.use_old_coach_hear_ ) );
    int_map.insert( IntMap::value_type( "send_vi_step", &param.online_coach_look_step_ ) );
    bool_map.insert( BoolMap::value_type( "use_offside", &param.use_offside_ ) );
    double_map.insert( DoubleMap::value_type( "offside_kick_margin", &param.offside_kick_margin_ ) );
    bool_map.insert( BoolMap::value_type( "forbid_kick_off_offside", &param.kick_off_offside_ ) );
    bool_map.insert( BoolMap::value_type( "verbose", &param.verbose_ ) );
    double_map.insert( DoubleMap::value_type( "offside_active_area_size", &param.offside_active_area_size_ ) );
    int_map.insert( IntMap::value_type( "slow_down_factor", &param.slow_down_factor_ ) );
    bool_map.insert( BoolMap::value_type( "synch_mode", &param.synch_mode_ ) );
    int_map.insert( IntMap::value_type( "synch_offset", &param.synch_offset_ ) );
    int_map.insert( IntMap::value_type( "synch_micro_sleep", &param.synch_micro_sleep_ ) );
    int_map.insert( IntMap::value_type( "start_goal_l", &param.start_goal_l_ ) );
    int_map.insert( IntMap::value_type( "start_goal_r", &param.start_goal_r_ ) );
    bool_map.insert( BoolMap::value_type( "fullstate_l", &param.fullstate_l_ ) );
    bool_map.insert( BoolMap::value_type( "fullstate_r", &param.fullstate_r_ ) );
    double_map.insert( DoubleMap::value_type( "slowness_on_top_for_left_team", &param.slowness_on_top_for_left_team_ ) );
    double_map.insert( DoubleMap::value_type( "slowness_on_top_for_right_team", &param.slowness_on_top_for_right_team_ ) );
    string_map.insert( StringMap::value_type( "landmark_file", &param.landmark_file_ ) );
    bool_map.insert( BoolMap::value_type( "send_comms", &param.send_comms_ ) );
    bool_map.insert( BoolMap::value_type( "text_logging", &param.text_logging_ ) );
    bool_map.insert( BoolMap::value_type( "game_logging", &param.game_logging_ ) );
    int_map.insert( IntMap::value_type( "game_log_version", &param.game_log_version_ ) );
    string_map.insert( StringMap::value_type( "text_log_dir", &param.text_log_dir_ ) );
    string_map.insert( StringMap::value_type( "game_log_dir", &param.game_log_dir_ ) );
    string_map.insert( StringMap::value_type( "text_log_fixed_name", &param.text_log_fixed_name_ ) );
    string_map.insert( StringMap::value_type( "game_log_fixed_name", &param.game_log_fixed_name_ ) );
    bool_map.insert( BoolMap::value_type( "text_log_fixed", &param.text_log_fixed_ ) );
    bool_map.insert( BoolMap::value_type( "game_log_fixed", &param.game_log_fixed_ ) );
    bool_map.insert( BoolMap::value_type( "text_log_dated", &param.text_log_dated_ ) );
    bool_map.insert( BoolMap::value_type( "game_log_dated", &param.game_log_dated_ ) );
    string_map.insert( StringMap::value_type( "log_date_format", &param.log_date_format_ ) );
    bool_map.insert( BoolMap::value_type( "log_times", &param.log_times_ ) );
    bool_map.insert( BoolMap::value_type( "record_messages", &param.record_messages_ ) );
    int_map.insert( IntMap::value_type( "text_log_compression", &param.text_log_compression_ ) );
    int_map.insert( IntMap::value_type( "game_log_compression", &param.game_log_compression_ ) );
    bool_map.insert( BoolMap::value_type( "profile", &param.profile_ ) );
    int_map.insert( IntMap::value_type( "point_to_ban", &param.point_to_ban_ ) );
    int_map.insert( IntMap::value_type( "point_to_duration", &param.point_to_duration_ ) );
    double_map.insert( DoubleMap::value_type( "tackle_dist", &param.tackle_dist_ ) );
    double_map.insert( DoubleMap::value_type( "tackle_back_dist", &param.tackle_back_dist_ ) );
    double_map.insert( DoubleMap::value_type( "tackle_width", &param.tackle_width_ ) );
    double_map.insert( DoubleMap::value_type( "tackle_exponent", &param.tackle_exponent_ ) );
    int_map.insert( IntMap::value_type( "tackle_cycles", &param.tackle_cycles_ ) );
    double_map.insert( DoubleMap::value_type( "tackle_power_rate", &param.tackle_power_rate_ ) );
    int_map.insert( IntMap::value_type( "freeform_wait_period", &param.freeform_wait_period_ ) );
    int_map.insert( IntMap::value_type( "freeform_send_period", &param.freeform_send_period_ ) );
    bool_map.insert( BoolMap::value_type( "free_kick_faults", &param.free_kick_faults_ ) );
    bool_map.insert( BoolMap::value_type( "back_passes", &param.back_passes_ ) );
    bool_map.insert( BoolMap::value_type( "proper_goal_kicks", &param.proper_goal_kicks_ ) );
    double_map.insert( DoubleMap::value_type( "stopped_ball_vel", &param.stopped_ball_vel_ ) );
    int_map.insert( IntMap::value_type( "max_goal_kicks", &param.max_goal_kicks_ ) );
    bool_map.insert( BoolMap::value_type( "auto_mode", &param.auto_mode_ ) );
    int_map.insert( IntMap::value_type( "kick_off_wait", &param.kick_off_wait_ ) );
    int_map.insert( IntMap::value_type( "connect_wait", &param.connect_wait_ ) );
    int_map.insert( IntMap::value_type( "game_over_wait", &param.game_over_wait_ ) );
    string_map.insert( StringMap::value_type( "team_l_start", &param.team_l_start_ ) );
    string_map.insert( StringMap::value_type( "team_r_start", &param.team_r_start_ ) );
    bool_map.insert( BoolMap::value_type( "keepaway", &param.keepaway_mode_ ) );
    double_map.insert( DoubleMap::value_type( "keepaway_length", &param.keepaway_length_ ) );
    double_map.insert( DoubleMap::value_type( "keepaway_width", &param.keepaway_width_ ) );
    bool_map.insert( BoolMap::value_type( "keepaway_logging", &param.keepaway_logging_ ) );
    string_map.insert( StringMap::value_type( "keepaway_log_dir", &param.keepaway_log_dir_ ) );
    string_map.insert( StringMap::value_type( "keepaway_log_fixed_name", &param.keepaway_log_fixed_name_ ) );
    bool_map.insert( BoolMap::value_type( "keepaway_log_fixed", &param.keepaway_log_fixed_ ) );
    bool_map.insert( BoolMap::value_type( "keepaway_log_dated", &param.keepaway_log_dated_ ) );
    int_map.insert( IntMap::value_type( "keepaway_start", &param.keepaway_start_ ) );
    int_map.insert( IntMap::value_type( "nr_normal_halfs", &param.nr_normal_halfs_ ) );
    int_map.insert( IntMap::value_type( "nr_extra_halfs", &param.nr_extra_halfs_ ) );
    bool_map.insert( BoolMap::value_type( "penalty_shoot_outs", &param.penalty_shoot_outs_ ) );
    int_map.insert( IntMap::value_type( "pen_before_setup_wait", &param.pen_before_setup_wait_ ) );
    int_map.insert( IntMap::value_type( "pen_setup_wait", &param.pen_setup_wait_ ) );
    int_map.insert( IntMap::value_type( "pen_ready_wait", &param.pen_ready_wait_ ) );
    int_map.insert( IntMap::value_type( "pen_taken_wait", &param.pen_taken_wait_ ) );
    int_map.insert( IntMap::value_type( "pen_nr_kicks", &param.pen_nr_kicks_ ) );
    int_map.insert( IntMap::value_type( "pen_max_extra_kicks", &param.pen_max_extra_kicks_ ) );
    double_map.insert( DoubleMap::value_type( "pen_dist_x", &param.pen_dist_x_ ) );
    bool_map.insert( BoolMap::value_type( "pen_random_winner", &param.pen_random_winner_ ) );
    double_map.insert( DoubleMap::value_type( "pen_max_goalie_dist_x", &param.pen_max_goalie_dist_x_ ) );
    bool_map.insert( BoolMap::value_type( "pen_allow_mult_kicks", &param.pen_allow_mult_kicks_ ) );
    bool_map.insert( BoolMap::value_type( "pen_coach_moves_players", &param.pen_coach_moves_players_ ) );
    // v11
    double_map.insert( DoubleMap::value_type( "ball_stuck_area", &param.ball_stuck_area_ ) );
    string_map.insert( StringMap::value_type( "coach_msg_file", &param.coach_msg_file_ ) );
    // v12
    double_map.insert( DoubleMap::value_type( "max_tackle_power", &param.max_tackle_power_ ) );
    double_map.insert( DoubleMap::value_type( "max_back_tackle_power", &param.max_back_tackle_power_ ) );
    double_map.insert( DoubleMap::value_type( "player_speed_max_min", &param.player_speed_max_min_ ) );
    double_map.insert( DoubleMap::value_type( "extra_stamina", &param.extra_stamina_ ) );
    int_map.insert( IntMap::value_type( "synch_see_offset", &param.synch_see_offset_ ) );
    int_map.insert( IntMap::value_type( "max_monitors", &param.max_monitors_ ) );
    // v12.1.3
    int_map.insert( IntMap::value_type( "extra_half_time", &param.extra_half_time_ ) );
    // v13
    double_map.insert( DoubleMap::value_type( "stamina_capacity", &param.stamina_capacity_ ) );
    double_map.insert( DoubleMap::value_type( "max_dash_angle", &param.max_dash_angle_ ) );
    double_map.insert( DoubleMap::value_type( "min_dash_angle", &param.min_dash_angle_ ) );
    double_map.insert( DoubleMap::value_type( "dash_angle_step", &param.dash_angle_step_ ) );
    double_map.insert( DoubleMap::value_type( "side_dash_rate", &param.side_dash_rate_ ) );
    double_map.insert( DoubleMap::value_type( "back_dash_rate", &param.back_dash_rate_ ) );
    double_map.insert( DoubleMap::value_type( "max_dash_power", &param.max_dash_power_ ) );
    double_map.insert( DoubleMap::value_type( "min_dash_power", &param.min_dash_power_ ) );
    // 14.0.0
    double_map.insert( DoubleMap::value_type( "tackle_rand_factor", &param.tackle_rand_factor_ ) );
    double_map.insert( DoubleMap::value_type( "foul_detect_probability", &param.foul_detect_probability_ ) );
    double_map.insert( DoubleMap::value_type( "foul_exponent", &param.foul_exponent_ ) );
    int_map.insert( IntMap::value_type( "foul_cycles", &param.foul_cycles_ ) );
    bool_map.insert( BoolMap::value_type( "golden_goal", &param.golden_goal_ ) );
    // 15.0
    double_map.insert( DoubleMap::value_type( "red_card_probability", &param.red_card_probability_ ) );
    // 16.0
    int_map.insert( IntMap::value_type( "illegal_defense_duration", &param.illegal_defense_duration_ ) );
    int_map.insert( IntMap::value_type( "illegal_defense_number", &param.illegal_defense_number_ ) );
    double_map.insert( DoubleMap::value_type( "illegal_defense_dist_x", &param.illegal_defense_dist_x_ ) );
    double_map.insert( DoubleMap::value_type( "illegal_defense_width", &param.illegal_defense_width_ ) );
    string_map.insert( StringMap::value_type( "fixed_teamname_l", &param.fixed_teamname_l_ ) );
    string_map.insert( StringMap::value_type( "fixed_teamname_r", &param.fixed_teamname_r_ ) );
    // 17.0
    double_map.insert( DoubleMap::value_type( "max_catch_angle", &param.max_catch_angle_ ) );
    double_map.insert( DoubleMap::value_type( "min_catch_angle", &param.min_catch_angle_ ) );

    //
    // parse
    //

    if ( ! parse_param( n_line, line,
                        int_map, double_map, bool_map, string_map ) )
    {
        std::cerr << n_line << ": Illegal server_param line. [" << line << ']' << std::endl;;
        return false;
    }

    return handler.handleServerParam( param );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parsePlayerParam( const int n_line,
                            const std::string & line,
                            Handler & handler )
{
    PlayerParamT param;

    //
    // prepare param map
    //
    IntMap int_map;
    DoubleMap double_map;
    BoolMap bool_map;
    StringMap string_map;

    int_map.insert( IntMap::value_type( "player_types", &param.player_types_ ) );
    int_map.insert( IntMap::value_type( "subs_max", &param.subs_max_ ) );
    int_map.insert( IntMap::value_type( "pt_max", &param.pt_max_ ) );
    bool_map.insert( BoolMap::value_type( "allow_mult_default_type", &param.allow_mult_default_type_ ) );
    double_map.insert( DoubleMap::value_type( "player_speed_max_delta_min", &param.player_speed_max_delta_min_ ) );
    double_map.insert( DoubleMap::value_type( "player_speed_max_delta_max", &param.player_speed_max_delta_max_ ) );
    double_map.insert( DoubleMap::value_type( "stamina_inc_max_delta_factor", &param.stamina_inc_max_delta_factor_ ) );
    double_map.insert( DoubleMap::value_type( "player_decay_delta_min", &param.player_decay_delta_min_ ) );
    double_map.insert( DoubleMap::value_type( "player_decay_delta_max", &param.player_decay_delta_max_ ) );
    double_map.insert( DoubleMap::value_type( "inertia_moment_delta_factor", &param.inertia_moment_delta_factor_ ) );
    double_map.insert( DoubleMap::value_type( "dash_power_rate_delta_min", &param.dash_power_rate_delta_min_ ) );
    double_map.insert( DoubleMap::value_type( "dash_power_rate_delta_max", &param.dash_power_rate_delta_max_ ) );
    double_map.insert( DoubleMap::value_type( "player_size_delta_factor", &param.player_size_delta_factor_ ) );
    double_map.insert( DoubleMap::value_type( "kickable_margin_delta_min", &param.kickable_margin_delta_min_ ) );
    double_map.insert( DoubleMap::value_type( "kickable_margin_delta_max", &param.kickable_margin_delta_max_ ) );
    double_map.insert( DoubleMap::value_type( "kick_rand_delta_factor", &param.kick_rand_delta_factor_ ) );
    double_map.insert( DoubleMap::value_type( "extra_stamina_delta_min", &param.extra_stamina_delta_min_ ) );
    double_map.insert( DoubleMap::value_type( "extra_stamina_delta_max", &param.extra_stamina_delta_max_ ) );
    double_map.insert( DoubleMap::value_type( "effort_max_delta_factor", &param.effort_max_delta_factor_ ) );
    double_map.insert( DoubleMap::value_type( "effort_min_delta_factor", &param.effort_min_delta_factor_ ) );
    int_map.insert( IntMap::value_type( "random_seed", &param.random_seed_ ) );
    double_map.insert( DoubleMap::value_type( "new_dash_power_rate_delta_min", &param.new_dash_power_rate_delta_min_ ) );
    double_map.insert( DoubleMap::value_type( "new_dash_power_rate_delta_max", &param.new_dash_power_rate_delta_max_ ) );
    double_map.insert( DoubleMap::value_type( "new_stamina_inc_max_delta_factor", &param.new_stamina_inc_max_delta_factor_ ) );
    // 14.0.0
    double_map.insert( DoubleMap::value_type( "kick_power_rate_delta_min", &param.kick_power_rate_delta_min_ ) );
    double_map.insert( DoubleMap::value_type( "kick_power_rate_delta_max", &param.kick_power_rate_delta_max_ ) );
    double_map.insert( DoubleMap::value_type( "foul_detect_probability_delta_factor", &param.foul_detect_probability_delta_factor_ ) );
    double_map.insert( DoubleMap::value_type( "catchable_area_l_stretch_min", &param.catchable_area_l_stretch_min_ ) );
    double_map.insert( DoubleMap::value_type( "catchable_area_l_stretch_max", &param.catchable_area_l_stretch_max_ ) );

    //
    // parse
    //

    if ( ! parse_param( n_line, line,
                        int_map, double_map, bool_map, string_map ) )
    {
        std::cerr << n_line << ": Illegal player_param line. [" << line << ']' << std::endl;;
        return false;
    }

    return handler.handlePlayerParam( param );
}


/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parsePlayerType( const int n_line,
                           const std::string & line,
                           Handler & handler )
{
    PlayerTypeT ptype;

    //
    // prepare param map
    //
    IntMap int_map;
    DoubleMap double_map;
    BoolMap bool_map;
    StringMap string_map;

    int_map.insert( IntMap::value_type( "id", &ptype.id_ ) );

    double_map.insert( DoubleMap::value_type( "player_speed_max", &ptype.player_speed_max_ ) );
    double_map.insert( DoubleMap::value_type( "stamina_inc_max", &ptype.stamina_inc_max_ ) );
    double_map.insert( DoubleMap::value_type( "player_decay", &ptype.player_decay_ ) );
    double_map.insert( DoubleMap::value_type( "inertia_moment", &ptype.inertia_moment_ ) );
    double_map.insert( DoubleMap::value_type( "dash_power_rate", &ptype.dash_power_rate_ ) );
    double_map.insert( DoubleMap::value_type( "player_size", &ptype.player_size_ ) );
    double_map.insert( DoubleMap::value_type( "kickable_margin", &ptype.kickable_margin_ ) );
    double_map.insert( DoubleMap::value_type( "kick_rand", &ptype.kick_rand_ ) );
    double_map.insert( DoubleMap::value_type( "extra_stamina", &ptype.extra_stamina_ ) );
    double_map.insert( DoubleMap::value_type( "effort_max", &ptype.effort_max_ ) );
    double_map.insert( DoubleMap::value_type( "effort_min", &ptype.effort_min_ ) );
    // 14.0.0
    double_map.insert( DoubleMap::value_type( "kick_power_rate", &ptype.kick_power_rate_ ) );
    double_map.insert( DoubleMap::value_type( "foul_detect_probability", &ptype.foul_detect_probability_ ) );
    double_map.insert( DoubleMap::value_type( "catchable_area_l_stretch", &ptype.catchable_area_l_stretch_ ) );

    //
    // parse
    //

    if ( ! parse_param( n_line, line,
                        int_map, double_map, bool_map, string_map ) )
    {
        std::cerr << n_line << ": Illegal player_type line. [" << line << ']' << std::endl;
        return false;
    }

    return handler.handlePlayerType( ptype );
}

} // end of namespace
} // end of namespace
