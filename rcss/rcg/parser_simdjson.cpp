// -*-c++-*-

/*!
  \file parser_json.cpp
  \brief rcg v6 (json) parser Source File.
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

#include "parser_simdjson.h"

#include "line_parser.h"

#include "handler.h"
#include "types.h"
#include "util.h"

#include "simdjson/simdjson.h"

#include <string_view>
#include <functional>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <set>

namespace rcss {
namespace rcg {


struct ParserSimdJSON::Impl {

    using Func = std::function< bool( simdjson::ondemand::value & val, Handler & handler ) >;
    std::unordered_map< std::string, Func > funcs_;
    //std::unordered_map< simdjson::ondemand::raw_json_string, Func > funcs_;

    Impl();

    bool parseLog( simdjson::padded_string & json,
                   Handler & handler );

    bool parseLines( const char * data,
                     const std::size_t size,
                     Handler & handler );

    bool parseRecord( simdjson::ondemand::parser & parser,
                      const char * record,
                      const std::size_t length,
                      const char * buffer_end,
                      Handler & handler );

    bool parseData( simdjson::ondemand::field & field,
                    Handler & handler );

    bool parseVersion( simdjson::ondemand::value & val,
                       Handler & handler );
    bool parseTimestamp( simdjson::ondemand::value & val,
                         Handler & handler );
    bool parseServerParam( simdjson::ondemand::value & val,
                           Handler & handler );
    bool parsePlayerParam( simdjson::ondemand::value & val,
                           Handler & handler );
    bool parsePlayerType( simdjson::ondemand::value & val,
                          Handler & handler );
    bool parseTeamGraphic( simdjson::ondemand::value & val,
                           Handler & handler );
    bool parsePlaymode( simdjson::ondemand::value & val,
                        Handler & handler );
    bool parseTeam( simdjson::ondemand::value & val,
                    Handler & handler );
    bool parseTeams( simdjson::ondemand::value & val,
                     const int time,
                     Handler & handler );
    bool parseMsg( simdjson::ondemand::value & val,
                   Handler & handler );
    bool parseShow( simdjson::ondemand::value & val,
                    Handler & handler );
};

namespace {

/*!
  \brief report a key that the parser does not know, once in a process: a
  newer server may add records, and a live connection would report them in
  every message.
*/
void
report_unknown_key( const std::string & key )
{
    static std::mutex s_mutex;
    static std::set< std::string > s_reported;

    std::lock_guard< std::mutex > lock( s_mutex );
    if ( s_reported.insert( key ).second )
    {
        std::cerr << "(ParserSimdJSON) skipped the unknown record " << key << std::endl;
    }
}

inline
bool
is_space( const char c )
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/*!
  \brief read a number of an object, if it is there.
  \return false if the key is missing or its value is not a number
*/
template < typename Object, typename T >
bool
read_double( Object & obj,
             const char * key,
             T * value )
{
    double d = 0.0;
    if ( obj[key].get_double().get( d ) != simdjson::SUCCESS )
    {
        return false;
    }
    *value = static_cast< T >( d );
    return true;
}

/*!
  \brief read an integer of an object, if it is there.
  \return false if the key is missing or its value is not an integer
*/
template < typename Object, typename T >
bool
read_int( Object & obj,
          const char * key,
          T * value )
{
    int64_t i = 0;
    if ( obj[key].get_int64().get( i ) != simdjson::SUCCESS )
    {
        return false;
    }
    *value = static_cast< T >( i );
    return true;
}

/*!
  \brief read a team: {"name": string or null, "score": int, "pen_score":
  int, "pen_miss": int}. The name is null before the team connects, and
  pen_score and pen_miss are there only for a team that took a penalty
  kick.
*/
bool
read_team( simdjson::ondemand::value & teams,
           const char * key,
           TeamT * team )
{
    simdjson::ondemand::object obj;
    if ( teams[key].get_object().get( obj ) != simdjson::SUCCESS )
    {
        return false;
    }

    std::string_view name;
    if ( obj["name"].get_string().get( name ) == simdjson::SUCCESS )
    {
        team->name_ = std::string( name );
    }
    read_int( obj, "score", &team->score_ );
    read_int( obj, "pen_score", &team->pen_score_ );
    read_int( obj, "pen_miss", &team->pen_miss_ );
    return true;
}

/*!
  \brief read a player of a show.
*/
void
parsePlayer( simdjson::ondemand::value & p,
             const char side,
             const Int16 unum,
             PlayerT * player )
{
    player->side_ = side;
    player->unum_ = unum;
    read_int( p, "type", &player->type_ );
    read_int( p, "state", &player->state_ );

    read_double( p, "x", &player->x_ );
    read_double( p, "y", &player->y_ );
    read_double( p, "vx", &player->vx_ );
    read_double( p, "vy", &player->vy_ );
    read_double( p, "body", &player->body_ );
    read_double( p, "neck", &player->neck_ );

    // only while pointing
    read_double( p, "px", &player->point_x_ );
    read_double( p, "py", &player->point_y_ );

    std::string_view str;
    if ( p["vq"].get_string().get( str ) == simdjson::SUCCESS )
    {
        player->view_quality_ = ( str == "l" ? 'l' : 'h' );
    }
    read_double( p, "vw", &player->view_width_ );

    read_double( p, "fdist", &player->focus_dist_ );
    read_double( p, "fdir", &player->focus_dir_ );

    read_double( p, "stamina", &player->stamina_ );
    read_double( p, "effort", &player->effort_ );
    read_double( p, "recovery", &player->recovery_ );
    read_double( p, "capacity", &player->stamina_capacity_ );

    // only with an attentionto target
    if ( p["fside"].get_string().get( str ) == simdjson::SUCCESS
         && ( str == "l" || str == "r" ) )
    {
        player->focus_side_ = str[0];
        read_int( p, "fnum", &player->focus_unum_ );
    }

    // only in a log
    read_int( p, "kick", &player->kick_count_ );
    read_int( p, "dash", &player->dash_count_ );
    read_int( p, "turn", &player->turn_count_ );
    read_int( p, "catch", &player->catch_count_ );
    read_int( p, "move", &player->move_count_ );
    read_int( p, "turn_neck", &player->turn_neck_count_ );
    read_int( p, "change_view", &player->change_view_count_ );
    read_int( p, "say", &player->say_count_ );
    read_int( p, "tackle", &player->tackle_count_ );
    read_int( p, "pointto", &player->pointto_count_ );
    read_int( p, "attentionto", &player->attentionto_count_ );
    read_int( p, "change_focus", &player->change_focus_count_ );
}

/*!
  \brief check if the log has one record per line, as rcssserver writes it:
  "[" alone on the first line, and a record ("{...},") on each of the next.
*/
bool
is_record_per_line( const char * data,
                    const std::size_t size )
{
    const char * p = data;
    const char * const end = data + size;

    while ( p < end && is_space( *p ) ) ++p;
    if ( p == end || *p != '[' ) return false;
    ++p;
    while ( p < end && ( *p == ' ' || *p == '\t' || *p == '\r' ) ) ++p;
    if ( p == end || *p != '\n' ) return false;
    while ( p < end && is_space( *p ) ) ++p;
    return p < end && *p == '{';
}

}

/*-------------------------------------------------------------------*/
ParserSimdJSON::Impl::Impl()
{
    funcs_["\"version\""]
        = [this]( simdjson::ondemand::value & val, Handler & handler ) -> bool
          {
              return this->parseVersion( val, handler );
          };
    funcs_["\"timestamp\""]
        = [this]( simdjson::ondemand::value & val, Handler & handler ) -> bool
          {
              return this->parseTimestamp( val, handler );
          };
    funcs_["\"server_param\""]
        = [this]( simdjson::ondemand::value & val, Handler & handler ) -> bool
          {
              return this->parseServerParam( val, handler );
          };
    funcs_["\"player_param\""]
        = [this]( simdjson::ondemand::value & val, Handler & handler ) -> bool
          {
              return this->parsePlayerParam( val, handler );
          };
    funcs_["\"player_type\""]
        = [this]( simdjson::ondemand::value & val, Handler & handler ) -> bool
          {
              return this->parsePlayerType( val, handler );
          };
    funcs_["\"team_graphic\""]
        = [this]( simdjson::ondemand::value & val, Handler & handler ) -> bool
          {
              return this->parseTeamGraphic( val, handler );
          };
    funcs_["\"playmode\""]
        = [this]( simdjson::ondemand::value & val, Handler & handler ) -> bool
          {
              return this->parsePlaymode( val, handler );
          };
    funcs_["\"team\""]
        = [this]( simdjson::ondemand::value & val, Handler & handler ) -> bool
          {
              return this->parseTeam( val, handler );
          };
    funcs_["\"msg\""]
        = [this]( simdjson::ondemand::value & val, Handler & handler ) -> bool
          {
              return this->parseMsg( val, handler );
          };
    funcs_["\"show\""]
        = [this]( simdjson::ondemand::value & val, Handler & handler ) -> bool
          {
              return this->parseShow( val, handler );
          };
}

/*-------------------------------------------------------------------*/
/*!
  parse a whole log: an array of the records
 */
bool
ParserSimdJSON::Impl::parseLog( simdjson::padded_string & json,
                                Handler & handler )
{
    try
    {
        simdjson::ondemand::parser parser;
        simdjson::ondemand::document rcg = parser.iterate( json );
        simdjson::ondemand::array root_array = rcg.get_array();

        int index = 0;
        std::vector< int > skipped;
        for ( simdjson::ondemand::object data : root_array )
        {
            ++index;
            for ( simdjson::ondemand::field field : data )
            {
                if ( ! parseData( field, handler ) )
                {
                    // the rest of the record is skipped
                    skipped.push_back( index );
                    break;
                }
            }
        }

        report_skipped( "ParserSimdJSON::parse", "record", skipped );
    }
    catch ( std::exception & e )
    {
        std::cerr << "(ParserSimdJSON::parse) ERROR\n\t" << e.what() << std::endl;
        return false;
    }

    handler.handleEOF();
    return true;
}

/*-------------------------------------------------------------------*/
/*!
  parse a log of one record per line. A line that cannot be parsed is
  skipped with a warning; the "[" and "]" lines are skipped, so a log whose
  last "]" is missing (rcssserver stopped abnormally) is read too.
 */
bool
ParserSimdJSON::Impl::parseLines( const char * data,
                                  const std::size_t size,
                                  Handler & handler )
{
    const char * const end = data + size;

    std::vector< LineRef > records;
    for ( const LineRef & line : split_lines( data, size ) )
    {
        const char * first = line.begin_;
        const char * last = line.begin_ + line.length_;

        while ( first < last && is_space( *first ) ) ++first;
        while ( first < last && is_space( *( last - 1 ) ) ) --last;
        if ( first < last && *( last - 1 ) == ',' )
        {
            --last;
            while ( first < last && is_space( *( last - 1 ) ) ) --last;
        }

        if ( first == last
             || ( last - first == 1 && ( *first == '[' || *first == ']' ) ) )
        {
            continue;
        }

        records.push_back( LineRef{ first, static_cast< std::size_t >( last - first ), line.number_ } );
    }

    // each thread has its own simdjson parser
    const std::vector< int > skipped
        = parse_lines( records,
                       [this, end]() -> LineParser
                       {
                           std::shared_ptr< simdjson::ondemand::parser > parser
                               = std::make_shared< simdjson::ondemand::parser >();
                           return [this, end, parser]( const LineRef & record, Handler & h ) -> bool
                               {
                                   return parseRecord( *parser, record.begin_, record.length_, end, h );
                               };
                       },
                       handler );
    report_skipped( "ParserSimdJSON::parse", "line", skipped );

    handler.handleEOF();
    return true;
}

/*-------------------------------------------------------------------*/
/*!
  parse one record. simdjson reads up to SIMDJSON_PADDING bytes beyond the
  end of its input; the lines after the record are those bytes, so the
  record is parsed in place, and only a record near the end of the buffer
  is copied.
 */
bool
ParserSimdJSON::Impl::parseRecord( simdjson::ondemand::parser & parser,
                                   const char * record,
                                   const std::size_t length,
                                   const char * buffer_end,
                                   Handler & handler )
{
    auto dispatch = [&]( simdjson::ondemand::document & doc ) -> bool
        {
            for ( simdjson::ondemand::field field : doc.get_object() )
            {
                if ( ! parseData( field, handler ) )
                {
                    return false;
                }
            }
            return true;
        };

    try
    {
        const std::size_t capacity = static_cast< std::size_t >( buffer_end - record );
        if ( capacity >= length + simdjson::SIMDJSON_PADDING )
        {
            simdjson::ondemand::document doc
                = parser.iterate( simdjson::padded_string_view( record, length, capacity ) );
            return dispatch( doc );
        }

        simdjson::padded_string copy( record, length );
        simdjson::ondemand::document doc = parser.iterate( copy );
        return dispatch( doc );
    }
    catch ( std::exception & e )
    {
        std::cerr << "(ParserSimdJSON::parseRecord) " << e.what() << std::endl;
    }

    return false;
}

/*-------------------------------------------------------------------*/
bool
ParserSimdJSON::Impl::parseData( simdjson::ondemand::field & field,
                                 Handler & handler )
{
    //std::string_view key = field.key_raw_json_token();
    //std::string str_key( key.substr( 1, key.length() - 2 ) );
    //std::string str_key( key );
    //std::cerr << "(ParserSimdJSON::Impl::parseData) key=" << field.key() << " str=" << str_key << std::endl;
    //std::unordered_map< std::string, Func >::iterator it = funcs_.find( str_key );
    std::string key( field.key_raw_json_token() );
    std::unordered_map< std::string, Func >::iterator it = funcs_.find( key );
    if ( it == funcs_.end() )
    {
        // a newer server may add records: skip the value
        report_unknown_key( key );
        return true;
    }

    if ( ! it->second( field.value(), handler ) )
    {
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!
  the version of the server that wrote the log, e.g. "18.1.3"
 */
bool
ParserSimdJSON::Impl::parseVersion( simdjson::ondemand::value & val,
                                    Handler & handler )
{
    std::string_view version;
    if ( val.get_string().get( version ) != simdjson::SUCCESS )
    {
        std::cerr << "(ParserSimdJSON::parseVersion) not a string" << std::endl;
        return false;
    }

    return handler.handleServerVersion( std::string( version ) );
}

/*-------------------------------------------------------------------*/
/*!
  the time when the log was written, e.g. "2024-03-19T18:13:33+0900"
  (strftime's "%FT%T%z": the offset has no colon). It is passed as it is.
 */
bool
ParserSimdJSON::Impl::parseTimestamp( simdjson::ondemand::value & val,
                                      Handler & handler )
{
    std::string_view timestamp;
    if ( val.get_string().get( timestamp ) != simdjson::SUCCESS )
    {
        std::cerr << "(ParserSimdJSON::parseTimestamp) not a string" << std::endl;
        return false;
    }

    return handler.handleTimestamp( std::string( timestamp ) );
}

/*-------------------------------------------------------------------*/
bool
ParserSimdJSON::Impl::parseServerParam( simdjson::ondemand::value & val,
                                        Handler & handler )
{
    ServerParamT server_param;

    for ( simdjson::ondemand::field param : val.get_object() )
    {
        std::string_view key = param.key_raw_json_token();
        if ( key.length() < 3
             || *key.begin() != '\"'
             || *key.rbegin() != '\"' )
        {
            std::cerr << "(ParserSimdJSON::parseServerParam) Error: unexpected key [" << key << "]" << std::endl;
            return false;
        }

        const std::string name( key.substr( 1, key.length() - 2 ) );
        const std::string value( param.value().raw_json_token() );

        server_param.setValue( name, value );
    }

    return handler.handleServerParam( server_param );
}

/*-------------------------------------------------------------------*/
bool
ParserSimdJSON::Impl::parsePlayerParam( simdjson::ondemand::value & val,
                                        Handler & handler )
{
    PlayerParamT player_param;

    for ( simdjson::ondemand::field param : val.get_object() )
    {
        std::string_view key = param.key_raw_json_token();
        if ( key.length() < 3
             || *key.begin() != '\"'
             || *key.rbegin() != '\"' )
        {
            std::cerr << "(ParserSimdJSON::parsePlayerParam) Error: unexpected key [" << key << "]" << std::endl;
            return false;
        }

        const std::string name( key.substr( 1, key.length() - 2 ) );
        const std::string value( param.value().raw_json_token() );

        player_param.setValue( name, value );
    }

    return handler.handlePlayerParam( player_param );
}

/*-------------------------------------------------------------------*/
bool
ParserSimdJSON::Impl::parsePlayerType( simdjson::ondemand::value & val,
                                       Handler & handler )
{
    PlayerTypeT player_type;

    for ( simdjson::ondemand::field param : val.get_object() )
    {
        std::string_view key = param.key_raw_json_token();
        if ( key.length() < 3
             || *key.begin() != '\"'
             || *key.rbegin() != '\"' )
        {
            std::cerr << "(ParserSimdJSON::parsePlayerType) Error: unexpected key [" << key << "]" << std::endl;
            return false;
        }

        const std::string name( key.substr( 1, key.length() - 2 ) );
        const std::string value( param.value().raw_json_token() );

        player_type.setValue( name, value );
    }

    return handler.handlePlayerType( player_type );
}

/*-------------------------------------------------------------------*/
bool
ParserSimdJSON::Impl::parseTeamGraphic( simdjson::ondemand::value & val,
                                        Handler & handler )
{
    bool result = false;
    try
    {
        std::string_view side = val["side"].get_string();
        int64_t x = val["x"].get_int64();
        int64_t y = val["y"].get_int64();

        std::vector< std::string > xpm_data;
        for ( simdjson::ondemand::value s : val["xpm"].get_array() )
        {
            // throws on an error, as the other values here
            xpm_data.emplace_back( std::string_view( s.get_string() ) );
        }

        result = handler.handleTeamGraphic( side[0], x, y, xpm_data );
    }
    catch ( std::exception & e )
    {
        std::cerr << "(ParserSimdJSON::TeamGraphic) " << e.what() << std::endl;
    }

    return result;
}

/*-------------------------------------------------------------------*/
bool
ParserSimdJSON::Impl::parsePlaymode( simdjson::ondemand::value & val,
                                     Handler & handler )
{
    bool result = false;
    try
    {
        int64_t time = -1;
        simdjson::error_code err = val["time"].get_int64().get( time );
        if ( err != simdjson::SUCCESS ) time = -1;

        std::string_view playmode = val["mode"].get_string();

        result = handler.handlePlayMode( time, std::string( playmode ) );
    }
    catch ( std::exception & e )
    {
        std::cerr << "(ParserSimdJSON::parsePlaymode) " << e.what() << std::endl;
    }

    return result;
}

/*-------------------------------------------------------------------*/
bool
ParserSimdJSON::Impl::parseTeam( simdjson::ondemand::value & val,
                                 Handler & handler )
{
    int time = -1;
    read_int( val, "time", &time );
    return parseTeams( val, time, handler );
}

/*-------------------------------------------------------------------*/
/*!
  read the teams {"l": {...}, "r": {...}} of a team record or a show
 */
bool
ParserSimdJSON::Impl::parseTeams( simdjson::ondemand::value & val,
                                  const int time,
                                  Handler & handler )
{
    bool result = false;
    try
    {
        TeamT team_l;
        TeamT team_r;
        if ( ! read_team( val, "l", &team_l )
             || ! read_team( val, "r", &team_r ) )
        {
            std::cerr << "(ParserSimdJSON::parseTeam) no l or r" << std::endl;
            return false;
        }

        result = handler.handleTeam( time, team_l, team_r );
    }
    catch ( std::exception & e )
    {
        std::cerr << "(ParserSimdJSON::parseTeam) " << e.what() << std::endl;
    }

    return result;
}

/*-------------------------------------------------------------------*/
bool
ParserSimdJSON::Impl::parseMsg( simdjson::ondemand::value & val,
                                Handler & handler )
{
    bool result = false;
    try
    {
        int time = 0;
        int board = 0;
        read_int( val, "time", &time );
        read_int( val, "board", &board );
        result = handler.handleMsg( time, board,
                                    std::string( val["message"].get_string().value() ) );
    }
    catch ( std::exception & e )
    {
        std::cerr << "(ParserSimdJSON::parseMsg) " << e.what() << std::endl;
    }

    return result;
}

/*-------------------------------------------------------------------*/
/*!
  read a show. Only the time and the ball are needed; the fields that are
  missing keep the defaults of ShowInfoT, which mean "no value" (e.g. no
  stamina capacity, not pointing). The server writes some fields only
  when they have a value: stime (when > 0), px and py (while pointing),
  fside and fnum (with an attentionto target), and the command counts
  (only in a log).
 */
bool
ParserSimdJSON::Impl::parseShow( simdjson::ondemand::value & val,
                                 Handler & handler )
{
    ShowInfoT show;

    try
    {
        show.time_ = val["time"].get_int64();
        read_int( val, "stime", &show.stime_ );
    }
    catch ( std::exception & e )
    {
        std::cerr << "(ParserSimdJSON::parseShow) time part: " << e.what() << std::endl;
        return false;
    }

    std::string_view stmp;

    // a live show has the play mode and the teams
    if ( val["mode"].get_string().get( stmp ) == simdjson::SUCCESS )
    {
        handler.handlePlayMode( show.time_, to_playmode_enum( std::string( stmp ) ) );
    }
    {
        simdjson::ondemand::value teams;
        if ( val["team"].get( teams ) == simdjson::SUCCESS )
        {
            parseTeams( teams, show.time_, handler );
        }
    }

    {
        simdjson::ondemand::object ball;
        if ( val["ball"].get_object().get( ball ) != simdjson::SUCCESS )
        {
            std::cerr << "(ParserSimdJSON::parseShow) time " << show.time_ << ": no ball" << std::endl;
            return false;
        }

        read_double( ball, "x", &show.ball_.x_ );
        read_double( ball, "y", &show.ball_.y_ );
        read_double( ball, "vx", &show.ball_.vx_ );
        read_double( ball, "vy", &show.ball_.vy_ );
        // the height, for a planned 2.5D simulator (the keys are provisional)
        read_double( ball, "z", &show.ball_.z_ );
        read_double( ball, "vz", &show.ball_.vz_ );
    }

    // a player is placed by side and unum, not by the order in the array
    int64_t unum = 0;
    try
    {
        simdjson::ondemand::array players;
        if ( val["players"].get_array().get( players ) == simdjson::SUCCESS )
        {
            for ( simdjson::ondemand::value p : players )
            {
                unum = 0;
                if ( p["side"].get_string().get( stmp ) != simdjson::SUCCESS
                     || stmp.length() != 1
                     || ( stmp[0] != 'l' && stmp[0] != 'r' )
                     || p["unum"].get_int64().get( unum ) != simdjson::SUCCESS
                     || unum < 1
                     || MAX_PLAYER < unum )
                {
                    std::cerr << "(ParserSimdJSON::parseShow) time " << show.time_
                              << ": skipped an illegal player " << stmp << ' ' << unum << std::endl;
                    continue;
                }

                PlayerT & player = show.player_[unum - 1 + ( stmp[0] == 'r' ? MAX_PLAYER : 0 )];
                if ( player.unum_ != 0 )
                {
                    std::cerr << "(ParserSimdJSON::parseShow) time " << show.time_
                              << ": duplicated player " << stmp << ' ' << unum << std::endl;
                    player = PlayerT();
                }

                parsePlayer( p, stmp[0], static_cast< Int16 >( unum ), &player );
            }
        }
    }
    catch ( std::exception & e )
    {
        std::cerr << "(ParserSimdJSON::parseShow) time " << show.time_
                  << " player part " << unum << ": " << e.what() << std::endl;
        return false;
    }

    return handler.handleShow( show );
}

/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
ParserSimdJSON::ParserSimdJSON()
    : M_impl( new Impl() )
{
    // std::cerr << "(ParserSimdJSON) create" << std::endl;
}

/*-------------------------------------------------------------------*/
bool
ParserSimdJSON::parse( std::istream & is,
                       Handler & handler ) const
{
    // streampos must be the first point!!!
    is.seekg( 0 );

    if ( ! is.good() )
    {
        return false;
    }

    const std::string data( ( std::istreambuf_iterator< char >( is ) ), std::istreambuf_iterator< char >() );
    return parse( data.data(), data.size(), handler );
}

/*-------------------------------------------------------------------*/
bool
ParserSimdJSON::parse( const std::string & filepath,
                       Handler & handler ) const
{
    std::ifstream fin( filepath, std::ios::binary );
    if ( ! fin )
    {
        std::cerr << "(ParserSimdJSON::parse) ERROR\n\tcould not read " << filepath << std::endl;
        return false;
    }

    const std::string data( ( std::istreambuf_iterator< char >( fin ) ), std::istreambuf_iterator< char >() );
    return parse( data.data(), data.size(), handler );
}

/*-------------------------------------------------------------------*/
bool
ParserSimdJSON::parse( const char * data,
                       const std::size_t size,
                       Handler & handler ) const
{
    handler.handleLogVersion( REC_VERSION_JSON );

    if ( is_record_per_line( data, size ) )
    {
        // as rcssserver writes it: parse each line in place
        return M_impl->parseLines( data, size, handler );
    }

    // any other layout: the whole document, with one copy into a buffer
    // with the padding that simdjson reads beyond the end
    simdjson::padded_string json( data, size );
    return M_impl->parseLog( json, handler );
}

/*-------------------------------------------------------------------*/
bool
ParserSimdJSON::parseData( const std::string & input,
                           Handler & handler ) const
{
    try
    {
        simdjson::ondemand::parser parser;
        simdjson::padded_string json( input );
        simdjson::ondemand::document data = parser.iterate( json );

        for ( simdjson::ondemand::field field : data.get_object() )
        {
            if ( ! M_impl->parseData( field, handler ) )
            {
                return false;
            }
        }
    }
    catch ( std::exception & e )
    {
        // a broken message, e.g. a control character in a string
        std::cerr << "(ParserSimdJSON::parseData) " << e.what() << std::endl;
        return false;
    }

    return true;
}

}
}
