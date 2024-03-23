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

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <climits>
#include <cstdio>

namespace rcss {
namespace rcg {

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parse( std::istream & is,
                 Handler & handler ) const
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
        std::cerr << "Unknown header line: [" << line << "]" << std::endl;
        return false;
    }

    const int version = std::stoi( line.substr( 3 ) );
    if ( version != REC_VERSION_4
         && version != REC_VERSION_5
         && version != REC_VERSION_6 )
    {
        std::cerr << "Unsupported rcg version: [" << line << "]" << std::endl;
        return false;
    }

    if ( ! handler.handleLogVersion( version ) )
    {
        std::cerr << "Unsupported game log version: [" << line << "]" << std::endl;
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

    if ( is.eof() )
    {
        return handler.handleEOF();
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parseLine( const int n_line,
                     const std::string & line,
                     Handler & handler ) const
{
    char name[32];

    if ( std::sscanf( line.c_str(), " ( %s ", name ) != 1 )
    {
        std::cerr << n_line << ": Illegal line: [" << line << ']'
                  << std::endl;;
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
        std::cerr << n_line << ": error:"
                  << " Unknown mode [" << line << ']'
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
                     Handler & handler ) const
{
    /*
      (show <Time> <Ball> <Players>)
    */

    const char * buf = line.c_str();

    if ( 0 )
    {
        int n_read = 0;

        // time
        int time = 0;

        if ( std::sscanf( buf, " ( show %d %n ",
                          &time, &n_read ) != 1 )
        {
            std::cerr << n_line << ": error: "
                      << "Illegal time info \"" << line << "\"" << std::endl;
            return false;
        }
        buf += n_read;

        //
        // playmode
        //
        if ( *(buf + 1) == 'p' )
        {
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
        if ( *(buf + 1) == 't' )
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

        //
        // show main
        //

        ShowInfoT show;

        show.time_ = time;

        //
        // ball
        //

        if ( std::sscanf( buf, " ((b) %f %f %f %f) %n",
                          &show.ball_.x_, &show.ball_.y_,
                          &show.ball_.vx_, &show.ball_.vy_,
                          &n_read ) != 4 )
        {
            std::cerr << n_line << ": error: "
                      << "Illegal ball info \"" << line << "\"" << std::endl;
            return false;
        }
        buf += n_read;

        //
        // players
        //

        char side;
        short unum;
        short type;
        int state;
        float x, y, vx, vy, body, neck;
        for ( int i = 0; i < MAX_PLAYER * 2; ++i )
        {
            if ( *buf == ')' ) break;

            if ( std::sscanf( buf,
                              " ((%c %hd) %hd %x %f %f %f %f %f %f %n",
                              &side, &unum,
                              &type, &state,
                              &x, &y, &vx, &vy, &body, &neck,
                              &n_read ) != 10 )
            {
                std::cerr << n_line << ": error: "
                          << " Illegal player id or pos " << i << " \"" << line << "\""
                          << std::endl;;
                return false;
            }
            buf += n_read;

            int idx = unum - 1;
            if ( side == 'r' ) idx += MAX_PLAYER;
            if ( idx < 0 || MAX_PLAYER*2 <= idx )
            {
                std::cerr << n_line << ": error: "
                          << " Illegal player id " << i << " \"" << line << "\""
                          << std::endl;;
                return false;
            }

            PlayerT & p = show.player_[idx];

            p.side_ = side;
            p.unum_ = unum;
            p.type_ = type;
            p.state_ = state;
            p.x_ = x;
            p.y_ = y;
            p.vx_ = vx;
            p.vy_ = vy;
            p.body_ = body;
            p.neck_ = neck;

            if ( *buf != '('
                 && std::sscanf( buf,
                                 " %f %f %n",
                                 &p.point_x_, &p.point_y_,
                                 &n_read ) == 2 )
            {
                buf += n_read;
            }

            if ( std::sscanf( buf,
                              " (v %c %f) (s %f %f %f) %n",
                              &p.view_quality_, &p.view_width_,
                              &p.stamina_, &p.effort_, &p.recovery_,
                              &n_read ) != 5 )
            {
                std::cerr << n_line << ": error: "
                          << "Illegal player view or stamina (" << side << ' ' << unum
                          << ") [" << line << "]" << std::endl;;
                return false;
            }
            buf += n_read;

            if ( *(buf + 1) == 'f'
                 && std::sscanf( buf,
                                 " ( f %c %hd ) %n",
                                 &p.focus_side_, &p.focus_unum_,
                                 &n_read ) == 2 )
            {
                buf += n_read;
            }

            if ( std::sscanf( buf,
                              " (c %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd)) %n",
                              &p.kick_count_,
                              &p.dash_count_,
                              &p.turn_count_,
                              &p.catch_count_,
                              &p.move_count_,
                              &p.turn_neck_count_,
                              &p.change_view_count_,
                              &p.say_count_,
                              &p.tackle_count_,
                              &p.pointto_count_,
                              &p.attentionto_count_,
                              &n_read ) != 11 )
            {
                std::cerr << n_line << ": error: "
                          << "Illegal player count (" << side << ' ' << unum
                          << ") [" << line << "]" << std::endl;;
                return false;
            }
            buf += n_read;
        }

        handler.handleShow( show );
    }
    else
    {
        ShowInfoT show;

        char * next;

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
            while ( *buf != '\0' && *buf != ')' ) ++buf;
            while ( *buf == ')' ) ++buf;
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
            char side = *buf;
            if ( side != 'l' && side != 'r' )
            {
                std::cerr << n_line << ": error: "
                          << " Illegal player side. " << side << ' ' << i
                          << " \"" << buf << "\""
                          << std::endl;;
                return false;
            }

            ++buf;
            long unum = std::strtol( buf, &next, 10 ); buf = next;
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

            PlayerT & p = show.player_[idx];
            p.side_ = side;
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

            // arm
            if ( *buf != '\0' && *buf != '(' )
            {
                p.point_x_ = strtof( buf, &next ); buf = next;
                p.point_y_ = strtof( buf, &next ); buf = next;
            }

            // (v quality width)
            while ( *buf != '\0' && *buf != 'v' ) ++buf;
            ++buf; // skip 'v'
            while ( *buf == ' ' ) ++buf;
            p.view_quality_ = *buf; ++buf;
            p.view_width_ = strtof( buf, &next ); buf = next;
            while ( *buf == ' ' || *buf == ')' ) ++buf;

            // (fp dist dir)
            // focus point is introduced in the monitor protocol v6
            if ( ! std::strncmp( buf, "(fp ", 4 ) )
            {
                buf += 4;
                p.focus_dist_ = strtof( buf, &next ); buf = next;
                p.focus_dir_ = strtof( buf, &next ); buf = next;
                while ( *buf == ' ' || *buf == ')' ) ++buf;
            }

            // (s stamina effort recovery[ capacity])
            // capacity is introduced in the monitor protocol v5
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
                p.focus_side_ = *buf; ++buf;
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
            while ( *buf == ' ' ) ++buf;
            if ( *buf != ')' )
            {
                p.change_focus_count_ = static_cast< UInt16 >( std::strtol( buf, &next, 10 ) ); buf = next;
            }

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

        handler.handleShow( show );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parseMsg( const int n_line,
                    const std::string & line,
                    Handler & handler ) const
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

    // find the last [")]
    std::string::size_type pos = msg.rfind( "\")" );
    if ( pos == std::string::npos )
    {
        std::cerr << n_line << ": ERROR Illegal msg [" << line << "]" << std::endl;;
        return false;
    }

    msg.erase( pos ); // remove the last 2 characters

    // team graphic
    if ( ! msg.compare( 0, std::strlen( "(team_graphic_" ), "(team_graphic_" ) )
    {
        return parseTeamGraphic( n_line, msg, handler );
    }

    // other message
    return handler.handleMsg( time, board, msg );
}

/*-------------------------------------------------------------------*/
bool
ParserV4::parseTeamGraphic( const int n_line,
                            const std::string & msg,
                            Handler & handler ) const
{
    char side = 'n';
    int x, y;
    int n_read = 0;
    if ( std::sscanf( msg.c_str(), "(team_graphic_%c ( %d %d %n",
                      &side, &x, &y, &n_read ) != 3
         || ( side != 'l' && side != 'r' )
         || x < 0
         || y < 0 )
    {
        std::cerr << n_line << ": ERROR Illegal team_graphic [" << msg << "]" << std::endl;;
        return false;
    }

    std::vector< std::string > xpm_data;

    const char * ptr = msg.c_str() + n_read;
    while ( *ptr != '\0' )
    {
        char buf[16];
        if ( std::sscanf( ptr, " \"%15[^\"]\" %n ", buf, &n_read ) != 1 )
        {
            std::cerr << n_line << ": ERROR Illegal team_graphic [" << ptr << "]" << std::endl;;
            return false;
        }
        ptr += n_read;

        xpm_data.push_back( buf );
        while ( *ptr != '\0' && *ptr != '"' ) ++ptr;
    }

    return handler.handleTeamGraphic( side, x, y, xpm_data );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parsePlayMode( const int n_line,
                         const std::string & line,
                         Handler & handler ) const
{
    /*
      (playmode <Time> <Playmode>)
    */

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

    return handler.handlePlayMode( time, pm_string );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parseTeam( const int n_line,
                     const std::string & line,
                     Handler & handler ) const
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

    handler.handleTeam( time, team_l, team_r );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parsePlayerType( const int n_line,
                           const std::string & line,
                           Handler & handler ) const
{
    if ( ! handler.handlePlayerType( PlayerTypeT( line ) ) )
    {
        std::cerr << n_line << ": error: "
                  << "Illegal player_type line. \"" << line << "\"" << std::endl;;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parseServerParam( const int n_line,
                            const std::string & line,
                            Handler & handler ) const
{
    if ( ! handler.handleServerParam( ServerParamT( line ) ) )
    {
        std::cerr << n_line << ": error: "
                  << "Illegal server_param line. \"" << line << "\"" << std::endl;;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parsePlayerParam( const int n_line,
                            const std::string & line,
                            Handler & handler ) const
{
    if ( ! handler.handlePlayerParam( PlayerParamT( line ) ) )
    {
        std::cerr << n_line << ": error: "
                  << "Illegal player_param line. \"" << line << "\"" << std::endl;;
    }

    return true;
}

} // end of namespace
} // end of namespace
