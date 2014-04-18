// -*-c++-*-

/*!
  \file parser.cpp
  \brief Class for parsing rcg files
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

#include "parser.h"

#include "handler.h"
#include "util.h"

#include <iostream>
#include <string>
#include <map>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <boost/lexical_cast.hpp>


typedef std::map< std::string, int * > IntMap;
typedef std::map< std::string, double * > DoubleMap;
typedef std::map< std::string, bool * > BoolMap;
typedef std::map< std::string, std::string * > StringMap;

namespace {

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


bool
parse_param_line( const int n_line,
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
        std::cerr << ":error: failed to the parse message id."
                  << std::endl;
        return false;
    }

    for ( std::string::size_type pos = line.find_first_of( '(', n_read );
          pos != std::string::npos;
          pos = line.find_first_of( '(', pos ) )
    {
        std::string::size_type end_pos = line.find_first_of( ' ', pos );
        if ( end_pos == std::string::npos )
        {
            std::cerr << n_line << ":error: failed to find parameter name."
                      << std::endl;
            return false;
        }
        pos += 1;

        const std::string name_str( line, pos, end_pos - pos );

        pos = end_pos;

        // search end paren or double quatation
        end_pos = line.find_first_of( ")\"", end_pos ); //"
        if ( end_pos == std::string::npos )
        {
            std::cerr << n_line << ":error: failed to parse parameter value for ["
                      << name_str << "] "
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
                std::cerr << n_line << ":error: ailed to parse the quated value for ["
                          << name_str << "] "
                          << std::endl;
                return false;
            }
            end_pos += 1; // skip double quatation
        }
        else
        {
            pos += 1; // skip white space
        }

        std::string value_str( line, pos, end_pos - pos );
        pos = end_pos;


        //
        // check parameter maps
        //
        try
        {
            IntMap::iterator int_it = int_map.find( name_str );
            if ( int_it != int_map.end() )
            {
                *(int_it->second) = boost::lexical_cast< int >( value_str );
                continue;
            }

            DoubleMap::iterator double_it = double_map.find( name_str );
            if ( double_it != double_map.end() )
            {
                *(double_it->second) = boost::lexical_cast< double >( value_str );
                continue;
            }

            BoolMap::iterator bool_it = bool_map.find( name_str );
            if ( bool_it != bool_map.end() )
            {
                *(bool_it->second) = !( value_str == "0"
                                        || value_str == "false"
                                        || value_str == "off" );
                continue;
            }

            StringMap::iterator string_it = string_map.find( name_str );
            if ( string_it != string_map.end() )
            {
                *(string_it->second) = clean_string( value_str );
                continue;
            }


            std::cerr << n_line << ": warning: " << message_name
                      << " unsupported parameter=(" << name_str
                      << ' ' << value_str << ")"
                      << std::endl;
        }
        catch ( boost::bad_lexical_cast & e )
        {
            std::cerr << e.what() << '\n'
                      << " param_name=" << name_str
                      << " value=" << value_str
                      << std::endl;
        }
    }

    return true;
}

}


namespace rcss {
namespace rcg {

Parser::Parser( Handler & handler )
    : M_handler( handler )
    , M_safe_mode( false )
    , M_header_parsed( false )
    , M_line_count( 0 )
    , M_time( 0 )
{

}


bool
Parser::parse( std::istream & is )
{
    // check stream status
    if ( ! is.good() )
    {
        return strmErr( is );
    }

    // parse header

    if ( ! M_header_parsed )
    {
        M_header_parsed = true;
        if ( ! parseHeader( is ) )
        {
            return false;
        }
    }

    // parse data

    if ( M_handler.getLogVersion() >= REC_VERSION_4 )
    {
        return parseLine( is );
    }

    return parseData( is );
}


bool
Parser::parseHeader( std::istream & is )
{
    char buf[4];
    is.read( buf, 4 );

    if ( ! is.good() )
    {
        return strmErr( is );
    }

    if ( buf[0] == 'U'
         && buf[1] == 'L'
         && buf[2] == 'G' )
    {
        int ver = static_cast< int >( buf[3] );
        if ( ver != REC_VERSION_2
             && ver != REC_VERSION_3 )
        {
            ver -= static_cast< int >( '0' );
            if ( ver != REC_VERSION_4
                 && ver != REC_VERSION_5 )
            {
                return false;
            }
        }

        M_handler.handleLogVersion( ver );
    }
    else
    {
        is.seekg( 0 );
        M_handler.handleLogVersion( 1 );
    }

    return true;
}


bool
Parser::parseData( std::istream & is )
{
    if ( M_handler.getLogVersion() == REC_OLD_VERSION )
    {
        return parseDispInfo( is );
    }
    else
    {
        return parseMode( is );
    }
}


bool
Parser::parseDispInfo( std::istream & is )
{
    dispinfo_t disp;
    is.read( reinterpret_cast< char * >( &disp ), sizeof( dispinfo_t ) );

    if ( is.gcount() != sizeof( dispinfo_t ) )
    {
        return strmErr( is );
    }

    Int16 mode = ntohs( disp.mode );

    switch ( mode ) {
    case NO_INFO:
        return true;
    case SHOW_MODE:
        {
            ShowInfoT show;
            TeamT team[2];
            convert( disp.body.show, show );
            convert( disp.body.show.team[0], team[0] );
            convert( disp.body.show.team[1], team[1] );

            M_time = show.time_;
            M_handler.handlePlayMode( M_time, static_cast< PlayMode >( disp.body.show.pmode ) );
            M_handler.handleTeamInfo( M_time, team[0], team[1] );
            M_handler.handleShowInfo( show );
        }
        return true;
    case MSG_MODE:
        M_handler.handleMsgInfo( M_time,
                                 ntohs( disp.body.msg.board ),
                                 disp.body.msg.message );
        return true;
    case DRAW_MODE:
        return parseDrawInfo( is.tellg(), disp.body.draw );
    case BLANK_MODE:
        return true;
    default:
        std::cerr << is.tellg() <<": Unknown mode " << mode
                  << std::endl;
        break;
    }

    return false;
}


bool
Parser::parseMode( std::istream & is )
{
    Int16 mode;
    is.read( reinterpret_cast< char * >( &mode ), sizeof( Int16 ) );

    if ( is.gcount() != sizeof( Int16 ) )
    {
        return strmErr( is );
    }

    switch ( ntohs( mode ) ) {
    case NO_INFO:
        return true;
    case SHOW_MODE:
        return parseShowInfo( is );
    case MSG_MODE:
        return parseMsgInfo( is );
    case DRAW_MODE:
        return parseDrawInfo( is );
    case BLANK_MODE:
        return true;
    case PM_MODE:
        return parsePlayMode( is );
    case TEAM_MODE:
        return parseTeamInfo( is );
    case PARAM_MODE:
        return parseServerParam( is );
    case PPARAM_MODE:
        return parsePlayerParam( is );
    case PT_MODE:
        return parsePlayerType( is );
    default:
        std::cerr << is.tellg() <<": Unknown mode " << ntohs( mode )
                  << std::endl;
        break;
    }

    return false;
}


bool
Parser::parseShowInfo( std::istream & is )
{
    ShowInfoT new_show;

    if ( M_handler.getLogVersion() == REC_VERSION_3 )
    {
        short_showinfo_t2 show;
        is.read( reinterpret_cast< char * >( &show ), sizeof( short_showinfo_t2 ) );

        if ( is.gcount() != sizeof( short_showinfo_t2 ) )
        {
            return strmErr( is );
        }

        convert( show, new_show );

        M_time = new_show.time_;
        M_handler.handleShowInfo( new_show );
    }
    else
    {
        showinfo_t show;
        is.read( reinterpret_cast< char * >( &show ), sizeof( showinfo_t ) );

        if ( is.gcount() != sizeof( showinfo_t ) )
        {
            return strmErr( is );
        }


        TeamT team[2];

        convert( show, new_show );
        convert( show.team[0], team[0] );
        convert( show.team[1], team[1] );

        M_time = new_show.time_;
        M_handler.handlePlayMode( M_time, static_cast< PlayMode >( show.pmode ) );
        M_handler.handleTeamInfo( M_time, team[0], team[1] );
        M_handler.handleShowInfo( new_show );
    }

    return true;
}


bool
Parser::parseMsgInfo( std::istream & is )
{
    Int16 board;
    is.read( reinterpret_cast< char * >( &board ), sizeof( Int16 ) );

    if ( is.gcount() != sizeof( Int16 ) )
    {
        return strmErr( is );
    }

    Int16 len;
    is.read( reinterpret_cast< char * >( &len ), sizeof( Int16 ) );

    if ( is.gcount() != sizeof( Int16 ) )
    {
        return strmErr( is );
    }

    len = ntohs( len );
    char * msg = new char[len];
    is.read( msg, len );

    if ( is.gcount() != len )
    {
        delete [] msg;
        return strmErr( is );
    }

    if ( msg[ len - 1 ] == 0 )
    {
        len = std::strlen( msg );
    }

    // copy the message to a string and delete the
    // original.  That was if handleMsgInfo throws,
    // the memory won't be lost.
    std::string str( msg );
    delete [] msg;

    M_handler.handleMsgInfo( M_time, ntohs( board ), str );
    return true;
}


bool
Parser::parseDrawInfo( std::istream & is )
{
    drawinfo_t draw;
    is.read( reinterpret_cast< char * >( &draw ), sizeof( drawinfo_t ) );

    if ( is.gcount() != sizeof( drawinfo_t ) )
    {
        return strmErr( is );
    }

    return parseDrawInfo( is.tellg(), draw );
}


bool
Parser::parseDrawInfo( const std::streampos pos,
                       const drawinfo_t & draw )
{
    switch ( ntohs( draw.mode ) ) {
    case DrawClear:
        M_handler.handleDrawClear( M_time );
        return true;
    case DrawPoint:
        M_handler.handleDrawPointInfo( M_time,
                                       PointInfoT( nstohf( draw.object.pinfo.x ),
                                                   nstohf( draw.object.pinfo.y ),
                                                   draw.object.pinfo.color ) );
        return true;
    case DrawCircle:
        M_handler.handleDrawCircleInfo( M_time,
                                        CircleInfoT( nstohf( draw.object.cinfo.x ),
                                                     nstohf( draw.object.cinfo.y ),
                                                     nstohf( draw.object.cinfo.r ),
                                                     draw.object.cinfo.color ) );
        return true;
    case DrawLine:
        M_handler.handleDrawLineInfo( M_time,
                                      LineInfoT( nstohf( draw.object.linfo.x1 ),
                                                 nstohf( draw.object.linfo.y1 ),
                                                 nstohf( draw.object.linfo.x2 ),
                                                 nstohf( draw.object.linfo.y2 ),
                                                 draw.object.linfo.color ) );
        return true;
    default:
        std::cerr << pos <<": Unknown draw mode " << ntohs( draw.mode )
                  << std::endl;
        return false;
    }

    return false;
}


bool
Parser::parsePlayMode( std::istream & is )
{
    char playmode;
    is.read( &playmode, sizeof( char ) );

    if ( is.gcount() != sizeof( char ) )
    {
        return strmErr( is );
    }

    M_handler.handlePlayMode( M_time, static_cast< PlayMode >( playmode ) );
    return true;
}


bool
Parser::parseTeamInfo( std::istream & is )
{
    team_t teams[2];
    is.read( reinterpret_cast< char * >( teams ), sizeof( team_t ) * 2 );

    if ( is.gcount() != sizeof( team_t ) * 2 )
    {
        return strmErr( is );
    }

    TeamT new_teams[2];
    convert( teams[0], new_teams[0] );
    convert( teams[1], new_teams[1] );

    M_handler.handleTeamInfo( M_time, new_teams[0], new_teams[1] );
    return true;
}


bool
Parser::parseServerParam( std::istream & is )
{
    server_params_t params;
    is.read( reinterpret_cast< char * >( &params ), sizeof( server_params_t ) );

    if ( is.gcount() != sizeof( server_params_t ) )
    {
        return strmErr( is );
    }

    ServerParamT new_params;
    convert( params, new_params );

    PlayerTypeT::set_default_param( new_params );

    M_handler.handleServerParam( new_params );
    return true;
}


bool
Parser::parsePlayerParam( std::istream & is )
{
    player_params_t params;
    is.read( reinterpret_cast< char * >( &params ), sizeof( player_params_t ) );

    if ( is.gcount() != sizeof( player_params_t ) )
    {
        return strmErr( is );
    }

    PlayerParamT new_params;
    convert( params, new_params );

    M_handler.handlePlayerParam( new_params );
    return true;
}


bool
Parser::parsePlayerType( std::istream & is )
{
    player_type_t type;
    is.read( reinterpret_cast< char * >( &type ), sizeof( player_type_t ) );

    if ( is.gcount() != sizeof( player_type_t ) )
    {
        return strmErr( is );
    }

    PlayerTypeT new_type;
    convert( type, new_type );

    M_handler.handlePlayerType( new_type );
    return true;
}


bool
Parser::strmErr( std::istream & is )
{
    if ( is.eof() )
    {
        M_handler.handleEOF();
        return false;
    }

    return false;
}


bool
Parser::parseLine( std::istream & is )
{
    std::string line;

    if ( std::getline( is, line ) )
    {
        ++M_line_count;
        if ( line.empty() )
        {
            return true;
        }

        parseLine( M_line_count, line );
        return true;
    }

    if ( ! is.eof() )
    {
        std::cerr << M_line_count << ": error: "
                  << "Failed to get line."
                  << std::endl;
    }

    return true;
}


bool
Parser::parseLine( const int n_line,
                   const std::string & line )
{
    if ( line.compare( 0, 6, "(show " ) == 0 )
    {
        parseShowLine( n_line, line );
    }
    else if ( line.compare( 0, 6, "(draw " ) == 0 )
    {
        parseDrawLine( n_line, line );
    }
    else if ( line.compare( 0, 5, "(msg " ) == 0 )
    {
        parseMsgLine( n_line, line );
    }
    else if ( line.compare( 0, 10, "(playmode " ) == 0 )
    {
        parsePlayModeLine( n_line, line );
    }
    else if ( line.compare( 0, 6, "(team " ) == 0 )
    {
        parseTeamLine( n_line, line );
    }
    else if ( line.compare( 0, 13, "(player_type " ) == 0 )
    {
        parsePlayerTypeLine( n_line, line );
    }
    else if ( line.compare( 0, 14, "(server_param " ) == 0 )
    {
        parseServerParamLine( n_line, line );
    }
    else if ( line.compare( 0, 14, "(player_param " ) == 0 )
    {
        parsePlayerParamLine( n_line, line );
    }
    else
    {
        std::cerr << n_line << ": error: "
                  << "Unknown info. " << "\"" << line << "\""
                  << std::endl;
    }

    return true;
}


bool
Parser::parseShowLine( const int n_line,
                       const std::string & line )
{
    const char * buf = line.c_str();
    int n_read = 0;

    ShowInfoT show;

    // time
    int time = 0;
    {
        if ( std::sscanf( buf, "(show %d %n",
                          &time, &n_read ) != 1 )
        {
            std::cerr << n_line << ": error: "
                      << "Illegal time info \"" << line << "\""
                      << std::endl;
            return false;
        }
        buf += n_read;

        M_time = time;
        show.time_ = static_cast< UInt32 >( time );
    }

    // playmode
    if ( *(buf + 1) == 'p' )
    {
        int pm = 0;
        if ( std::sscanf( buf,
                          "(pm %d) %n ",
                          &pm, &n_read ) != 1 )
        {
            std::cerr << n_line << ": error: "
                      << "Illegal playmode info \"" << line << "\""
                      << std::endl;
            return false;
        }
        buf += n_read;

        M_handler.handlePlayMode( time, static_cast< PlayMode >( pm ) );
    }

    // team
    if ( *(buf + 1) == 't' )
    {
        char name_l[32], name_r[32];
        int score_l = 0, score_r = 0;
        int pen_score_l = 0, pen_miss_l = 0, pen_score_r = 0, pen_miss_r = 0;

        int n = std::sscanf( buf,
                             "(tm %31s %31s %d %d %d %d %d %d ",
                             name_l, name_r,
                             &score_l, &score_r,
                             &pen_score_l, &pen_miss_l,
                             &pen_score_r, &pen_miss_r );

        if ( n != 4 && n != 8 )
        {
            std::cerr << n_line << ": error: "
                      << "Illegal team info. n=" << n << ' '
                      << "\"" << line << "\"" << std::endl;;
            return false;
        }
        while ( *buf != ')' && *buf != '\0' ) ++buf;
        while ( *buf == ')' ) ++buf;

        if ( ! std::strcmp( name_l, "null" ) ) std::memset( name_l, 0, 4 );
        if ( ! std::strcmp( name_r, "null" ) ) std::memset( name_r, 0, 4 );

        TeamT team_l( name_l, score_l, pen_score_l, pen_miss_l );
        TeamT team_r( name_r, score_r, pen_score_r, pen_miss_r );

        M_handler.handleTeamInfo( time, team_l, team_r );
    }


    if ( M_safe_mode )
    {
        // ball
        {
            BallT & ball = show.ball_;
            if ( std::sscanf( buf, " ((b) %f %f %f %f) %n",
                              &ball.x_, &ball.y_, &ball.vx_, &ball.vy_,
                              &n_read ) != 4 )
            {
                std::cerr << n_line << ": error: "
                          << "Illegal ball info \"" << line << "\""
                          << std::endl;
                return false;
            }
            buf += n_read;
        }

        // players
        char side;
        short unum;
        short type;
        int state;
        float x, y, vx, vy, body, neck;
        for ( int i = 0; i < MAX_PLAYER * 2; ++i )
        {
            if ( *buf == '\0' || *buf == ')' ) break;

            if ( std::sscanf( buf,
                              " ((%c %hd) %hd %x %f %f %f %f %f %f %n",
                              &side, &unum,
                              &type, &state,
                              &x, &y, &vx, &vy, &body, &neck,
                              &n_read ) != 10 )
            {
                std::cerr << n_line << ": error: "
                          << " Illegal player id or pos. " << side << ' ' << unum
                          << " \"" << line << "\""
                          << std::endl;
                return false;
            }
            buf += n_read;

            int idx = unum - 1;
            if ( side == 'r' ) idx += MAX_PLAYER;
            if ( idx < 0 || MAX_PLAYER*2 <= idx )
            {
                std::cerr << n_line << ": error: "
                          << " Illegal player id. " << side << ' ' << unum
                          << " \"" << line << "\""
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
                                 "%f %f %n",
                                 &p.point_x_, &p.point_y_,
                                 &n_read ) == 2 )
            {
                buf += n_read;
            }

            if ( std::sscanf( buf,
                              " (v %c %f) %n ",
                              &p.view_quality_, &p.view_width_,
                              &n_read ) != 2 )
            {
                std::cerr << n_line << ": error: "
                          << " Illegal player view . " << side << ' ' << unum
                          << " \"" << line << "\""
                          << std::endl;;
                return false;
            }
            buf += n_read;

            if ( std::sscanf( buf,
                              "(s %f %f %f %f) %n",
                              &p.stamina_, &p.effort_, &p.recovery_, &p.stamina_capacity_,
                              &n_read ) != 4
                 && std::sscanf( buf,
                                 "(s %f %f %f) %n",
                                 &p.stamina_, &p.effort_, &p.recovery_,
                                 &n_read ) != 3 )
            {
                std::cerr << n_line << ": error: "
                          << " Illegal player view or stamina. " << side << ' ' << unum
                          << " \"" << line << "\""
                          << std::endl;;
                return false;
            }
            buf += n_read;

            if ( *(buf + 1) == 'f'
                 && std::sscanf( buf,
                                 " (f %c %hd) %n",
                                 &p.focus_side_, &p.focus_unum_,
                                 &n_read ) == 2 )
            {
                buf += n_read;
            }

            if ( std::sscanf( buf,
                              " (c %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd )) %n",
                              &p.kick_count_, &p.dash_count_, &p.turn_count_, &p.catch_count_, &p.move_count_,
                              &p.turn_neck_count_, &p.change_view_count_, &p.say_count_, &p.tackle_count_,
                              &p.pointto_count_, &p.attentionto_count_,
                              &n_read ) != 11 )
            {
                std::cerr << n_line << ": error: "
                          << " Illegal player count. " << side << ' ' << unum
                          << " \"" << line << "\""
                          << std::endl;;
                return false;
            }
            buf += n_read;
        }
    }
    else
    {
        char * next;

        // ball
        {
            // ((b) x y vx vy)
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
        for ( int i = 0; i < MAX_PLAYER * 2; ++i )
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
            p.view_quality_ = *buf; ++buf;
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
    }

    M_handler.handleShowInfo( show );

    return true;
}


bool
Parser::parseDrawLine( const int n_line,
                       const std::string & line )
{
    // ( draw <time> (clear))
    // ( draw <time> (point <x> <y> "<color>"))
    // ( draw <time> (circle <x> <y> <radius> "<color>"))
    // ( draw <time> (line <sx> <sy> <ex> <ey> "<color>"))

    const char * buf = line.c_str();

    int n_read = 0;

    // parse time

    int time = 0;
    if ( std::sscanf( buf, " ( draw %d %n ",
                      &time, &n_read ) != 1 )
    {
        std::cerr << n_line << ": error: "
                  << "Illegal time info \"" << line << "\""
                  << std::endl;
        return false;
    }
    buf += n_read;

    M_time = time;

    // parse object

    if ( ! std::strncmp( buf, "(point ", 7 ) )
    {
        float x, y;
        char col[64];
        if ( std::sscanf( buf,
                          " (point %f %f \"%63[^\"]\" ) ",
                          &x, &y, col ) != 3 )
        {
            std::cerr << n_line << ": error: "
                      << "Illegal draw point info \"" << line << "\""
                      << std::endl;
            return false;
        }

        M_handler.handleDrawPointInfo( M_time, PointInfoT( x, y, col ) );
    }
    else if ( ! std::strncmp( buf, "(circle ", 8 ) )
    {
        float x, y, r;
        char col[64];
        if ( std::sscanf( buf,
                          " (circle %f %f %f \"%63[^\"]\" ) ",
                          &x, &y, &r, col ) != 4 )
        {
            std::cerr << n_line << ": error: "
                      << "Illegal draw circle info \"" << line << "\""
                      << std::endl;
            return false;
        }

        M_handler.handleDrawCircleInfo( M_time, CircleInfoT( x, y, r, col ) );
    }
    else if ( ! std::strncmp( buf, "(line ", 6 ) )
    {
        float x1, y1, x2, y2;
        char col[64];

        if ( std::sscanf( buf,
                          " (line %f %f %f %f \"%63[^\"]\" ) ",
                          &x1, &y1, &x2, &y2, col ) != 5 )
        {
            std::cerr << n_line << ": error: "
                      << "Illegal draw line info \"" << line << "\""
                      << std::endl;
            return false;
        }

        M_handler.handleDrawLineInfo( M_time, LineInfoT( x1, y1, x2, y2, col ) );
    }
    else if ( ! std::strncmp( buf, "(clear)", 7 ) )
    {
        M_handler.handleDrawClear( M_time );
    }
    else
    {
        std::cerr << n_line << ": error: "
                  << "Illegal draw info \"" << line << "\""
                  << std::endl;
        return false;
    }

    return true;
}


bool
Parser::parseMsgLine( const int n_line,
                      const std::string & line )
{
    /*
      (msg <Time> <Board> "<Message>")

      NOTE: <Message> may include '"', e.g. xpm line in team_graphic_[lr] message.
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

    M_time = time;

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

    M_handler.handleMsgInfo( M_time, board, msg );

    return true;
}


bool
Parser::parsePlayModeLine( const int n_line,
                           const std::string & line )
{
    static const char * s_playmode_strings[] = PLAYMODE_STRINGS;

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

    M_time = time;

    PlayMode pm = PM_Null;
    for ( int n = 0; n < PM_MAX; ++n )
    {
        if ( ! std::strcmp( s_playmode_strings[n], pm_string ) )
        {
            pm = static_cast< PlayMode >( n );
            break;
        }
    }

    M_handler.handlePlayMode( M_time, pm );

    return true;
}


bool
Parser::parseTeamLine( const int n_line,
                       const std::string & line )
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

    M_time = time;

    TeamT team_l( name_l, score_l, pen_score_l, pen_miss_l );
    TeamT team_r( name_r, score_r, pen_score_r, pen_miss_r );

    M_handler.handleTeamInfo( M_time, team_l, team_r );

    return true;
}


bool
Parser::parsePlayerTypeLine( const int n_line,
                             const std::string & line )
{
    PlayerTypeT param;

    //
    // prepare param map
    //
    IntMap int_map;
    DoubleMap double_map;
    BoolMap bool_map;
    StringMap string_map;

    int_map.insert( IntMap::value_type( "id", &param.id_ ) );

    double_map.insert( DoubleMap::value_type( "player_speed_max", &param.player_speed_max_ ) );
    double_map.insert( DoubleMap::value_type( "stamina_inc_max", &param.stamina_inc_max_ ) );
    double_map.insert( DoubleMap::value_type( "player_decay", &param.player_decay_ ) );
    double_map.insert( DoubleMap::value_type( "inertia_moment", &param.inertia_moment_ ) );
    double_map.insert( DoubleMap::value_type( "dash_power_rate", &param.dash_power_rate_ ) );
    double_map.insert( DoubleMap::value_type( "player_size", &param.player_size_ ) );
    double_map.insert( DoubleMap::value_type( "kickable_margin", &param.kickable_margin_ ) );
    double_map.insert( DoubleMap::value_type( "kick_rand", &param.kick_rand_ ) );
    double_map.insert( DoubleMap::value_type( "extra_stamina", &param.extra_stamina_ ) );
    double_map.insert( DoubleMap::value_type( "effort_max", &param.effort_max_ ) );
    double_map.insert( DoubleMap::value_type( "effort_min", &param.effort_min_ ) );
    // 14.0.0
    double_map.insert( DoubleMap::value_type( "kick_power_rate", &param.kick_power_rate_ ) );
    double_map.insert( DoubleMap::value_type( "foul_detect_probability", &param.foul_detect_probability_ ) );
    double_map.insert( DoubleMap::value_type( "catchable_area_l_stretch", &param.catchable_area_l_stretch_ ) );

    //
    // parse
    //

    if ( ! parse_param_line( n_line, line,
                             int_map, double_map, bool_map, string_map ) )
    {
        std::cerr << n_line << ": error: "
                  << "Illegal player_type line. \"" << line << "\"" << std::endl;;
        return false;
    }

    M_handler.handlePlayerType( param );

    return true;
}


bool
Parser::parsePlayerParamLine( const int n_line,
                              const std::string & line )
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

    if ( ! parse_param_line( n_line, line,
                             int_map, double_map, bool_map, string_map ) )
    {
        std::cerr << n_line << ": error: "
                  << "Illegal player_param line. \"" << line << "\"" << std::endl;;
        return false;
    }

    M_handler.handlePlayerParam( param );

    return true;
}


bool
Parser::parseServerParamLine( const int n_line,
                              const std::string & line )
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
    double_map.insert( DoubleMap::value_type( "quantize_step", &param.quantize_step_ ) );
    double_map.insert( DoubleMap::value_type( "quantize_step_l", &param.landmark_quantize_step_ ) );
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
    bool_map.insert( BoolMap::value_type( "wind_random", &param.wind_random_ ) );
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
    bool_map.insert( BoolMap::value_type( "old_coach_hear", &param.old_coach_hear_ ) );
    int_map.insert( IntMap::value_type( "send_vi_step", &param.send_vi_step_ ) );
    bool_map.insert( BoolMap::value_type( "use_offside", &param.use_offside_ ) );
    double_map.insert( DoubleMap::value_type( "offside_kick_margin", &param.offside_kick_margin_ ) );
    bool_map.insert( BoolMap::value_type( "forbid_kick_off_offside", &param.forbid_kick_off_offside_ ) );
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

    //
    // parse
    //

    if ( ! parse_param_line( n_line, line,
                             int_map, double_map, bool_map, string_map ) )
    {
        std::cerr << n_line << ": error: "
                  << "Illegal server_param line. \"" << line << "\"" << std::endl;;
        return false;
    }

    M_handler.handleServerParam( param );
    PlayerTypeT::set_default_param( param );

    return true;
}

}
}
