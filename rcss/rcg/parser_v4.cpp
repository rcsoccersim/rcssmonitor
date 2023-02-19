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
         && version != REC_VERSION_5
         && version != REC_VERSION_6 )
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
    // ((side unum) type state x y vx vy body neck [pointx pointy] (v h 90) [(fp dist dir)] (s 4000 1 1)[(f side unum)])
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
        p.high_quality_ = ( *buf == 'h' ? true : false ); ++buf;
        p.view_width_ = strtof( buf, &next ); buf = next;
        while ( *buf == ' ' || *buf == ')' ) ++buf;

        // (fp dist dir)
        if ( ! std::strncmp( buf, "(fp ", 4 ) )
        {
            buf += 4;
            p.focus_dist_ = strtof( buf, &next ); buf = next;
            p.focus_dir_ = strtof( buf, &next ); buf = next;
            while ( *buf == ' ' || *buf == ')' ) ++buf;
        }

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
        if ( ! std::strncmp( buf, "(f ", 3 ) )
        {
            buf += 3;
            while ( *buf == ' ' ) ++buf;
            p.focus_side_ = ( *buf == 'l' ? LEFT : *buf == 'r' ? RIGHT : NEUTRAL ); ++buf;
            p.focus_unum_ = static_cast< Int16 >( std::strtol( buf, &next, 10 ) ); buf = next;
            while ( *buf == ' ' || *buf == ')' ) ++buf;
        }

        // (c kick dash turn catch move tneck cview say tackle pointto atttention[ change_focus])
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
ParserV4::parseServerParam( const int /*n_line*/,
                            const std::string & line,
                            Handler & handler )
{
    ServerParamT param;

    param.createFromSExp( line );

    return handler.handleServerParam( param );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parsePlayerParam( const int /*n_line*/,
                            const std::string & line,
                            Handler & handler )
{
    PlayerParamT param;

    param.createFromSExp( line );

    return handler.handlePlayerParam( param );
}


/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserV4::parsePlayerType( const int /*n_line*/,
                           const std::string & line,
                           Handler & handler )
{
    PlayerTypeT ptype;

    ptype.createFromSExp( line );

    return handler.handlePlayerType( ptype );
}

} // end of namespace
} // end of namespace
