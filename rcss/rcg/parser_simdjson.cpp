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

#include "handler.h"
#include "types.h"
#include "util.h"

#include "simdjson/simdjson.h"

#include <string_view>
#include <functional>

namespace rcss {
namespace rcg {


struct ParserSimdJSON::Impl {

    using Func = std::function< bool( simdjson::ondemand::value & val, Handler & handler ) >;
    std::unordered_map< std::string, Func > funcs_;
    //std::unordered_map< simdjson::ondemand::raw_json_string, Func > funcs_;

    Impl();

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
    bool parseMsg( simdjson::ondemand::value & val,
                   Handler & handler );
    bool parseShow( simdjson::ondemand::value & val,
                    Handler & handler );
};

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
        std::cerr << "(ParserSimdJSON::Impl::parseData) func not found. key=" << key
                  << std::endl;
        return false;
    }

    if ( ! it->second( field.value(), handler ) )
    {
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
bool
ParserSimdJSON::Impl::parseVersion( simdjson::ondemand::value & /*val*/,
                                    Handler & /*handler*/ )
{
    //std::cerr << "version " << val << std::endl;
    return true;
}

/*-------------------------------------------------------------------*/
bool
ParserSimdJSON::Impl::parseTimestamp( simdjson::ondemand::value & /*val*/,
                                    Handler & /*handler*/ )
{
    //std::cerr << "timestamp " << val << std::endl;
    return true;
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
            std::string str;
            s.get_string( str );
            xpm_data.push_back( str );
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
    bool result = false;
    try
    {
        simdjson::error_code err = simdjson::SUCCESS;

        TeamT team_l;
        TeamT team_r;

        int64_t time = -1;
        err = val["time"].get_int64().get( time );
        if ( err != simdjson::SUCCESS ) time = -1;

        uint64_t i = 0;

        if ( val["l"]["name"].is_string() ) team_l.name_ = val["l"]["name"].get_string().value();
        team_l.score_ = val["l"]["score"].get_uint64();

        err = val["l"]["pen_score"].get_uint64().get( i );
        if ( err == simdjson::SUCCESS ) team_l.pen_score_ = i;

        err = val["l"]["pen_miss"].get_uint64().get( i );
        if ( err == simdjson::SUCCESS ) team_l.pen_miss_ = i;

        if ( val["r"]["name"].is_string() ) team_r.name_ = val["r"]["name"].get_string().value();
        team_r.score_ = val["r"]["score"].get_uint64();

        err = val["r"]["pen_score"].get_uint64().get( i );
        if ( err == simdjson::SUCCESS ) team_r.pen_score_ = i;

        err = val["r"]["pen_miss"].get_uint64().get( i );
        if ( err == simdjson::SUCCESS ) team_r.pen_miss_ = i;

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
        result = handler.handleMsg( val["time"].get_int64(),
                                    val["board"].get_int64(),
                                    std::string( val["message"].get_string().value() ) );
    }
    catch ( std::exception & e )
    {
        std::cerr << "(ParserSimdJSON::parseMsg) " << e.what() << std::endl;
    }

    return result;
}

/*-------------------------------------------------------------------*/
bool
ParserSimdJSON::Impl::parseShow( simdjson::ondemand::value & val,
                                 Handler & handler )
{
    ShowInfoT show;

    simdjson::error_code err = simdjson::SUCCESS;
    int64_t itmp;
    double dtmp;
    std::string_view stmp;

    try
    {
        show.time_ = val["time"].get_int64();
        err = val["stime"].get_int64().get( itmp );
        if ( err == simdjson::SUCCESS ) show.stime_ = itmp;
    }
    catch ( std::exception & e )
    {
        std::cerr << "(ParserSimdJSON::parseShow) time part: " << e.what() << std::endl;
        return false;
    }

    {
        err = val["mode"].get_string().get( stmp );
        if ( err == simdjson::SUCCESS )
        {
            if ( ! handler.handlePlayMode( show.time_, to_playmode_enum( std::string( stmp ) ) ) )
            {
                return false;
            }
        }
    }
    {
        simdjson::simdjson_result< simdjson::ondemand::value > teams = val["team"];
        if ( teams.error() == simdjson::SUCCESS )
        {
            if ( ! parseTeam( teams.value(), handler ) )
            {
                return false;
            }
        }
    }

    try
    {
        show.ball_.x_ = val["ball"]["x"].get_double();
        show.ball_.y_ = val["ball"]["y"].get_double();
        show.ball_.vx_ = val["ball"]["vx"].get_double();
        show.ball_.vy_ = val["ball"]["vy"].get_double();
    }
    catch ( std::exception & e )
    {
        std::cerr << "(ParserSimdJSON::parseShow) ball part: " << e.what() << std::endl;
        return false;
    }

    size_t i = 0;
    try
    {
        for ( simdjson::ondemand::value p : val["players"].get_array() )
        {
            if ( i >= 22 )
            {
                std::cerr << "(ParserSimdJSON::parseShow) player index overflow " << i << std::endl;
                return false;
            }

            // begin
            stmp = p["side"].get_string();
            show.player_[i].side_ = stmp[0];
            show.player_[i].unum_ = p["unum"].get_int64();
            show.player_[i].type_ = p["type"].get_int64();
            show.player_[i].state_ = p["state"].get_int64();

            // pos
            show.player_[i].x_ = p["x"].get_double();
            show.player_[i].y_ = p["y"].get_double();
            show.player_[i].vx_ = p["vx"].get_double();
            show.player_[i].vy_ = p["vy"].get_double();
            show.player_[i].body_ = p["body"].get_double();
            show.player_[i].neck_ = p["neck"].get_double();

            // arm
            err = p["px"].get_double().get( dtmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].point_x_ = dtmp;
            err = p["py"].get_double().get( dtmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].point_y_ = dtmp;

            // view mode
            stmp = p["vq"].get_string();
            show.player_[i].view_quality_ = ( stmp == "h" ? true : false );
            show.player_[i].view_width_ = p["vw"].get_double();

            // focus point
            err = p["fdist"].get_double().get( dtmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].focus_dist_ = dtmp;
            err = p["fdir"].get_double().get( dtmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].focus_dir_ = dtmp;

            // stamina
            show.player_[i].stamina_ = p["stamina"].get_double();
            show.player_[i].effort_ = p["effort"].get_double();
            show.player_[i].recovery_ = p["recovery"].get_double();
            show.player_[i].stamina_capacity_ = p["capacity"].get_double();

            // focus
            err = p["fside"].get_string().get( stmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].focus_side_ = stmp[0];
            err = p["fnum"].get_int64().get( itmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].focus_unum_ = itmp;

            // count
            err = p["kick"].get_int64().get( itmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].kick_count_ = itmp;
            err = p["dash"].get_int64().get( itmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].dash_count_ = itmp;
            err = p["turn"].get_int64().get( itmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].turn_count_ = itmp;
            err = p["catch"].get_int64().get( itmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].catch_count_ = itmp;
            err = p["move"].get_int64().get( itmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].move_count_ = itmp;
            err = p["turn_neck"].get_int64().get( itmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].turn_neck_count_ = itmp;
            err = p["change_view"].get_int64().get( itmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].change_view_count_ = itmp;
            err = p["say"].get_int64().get( itmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].say_count_ = itmp;
            err = p["tackle"].get_int64().get( itmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].tackle_count_ = itmp;
            err = p["pointto"].get_int64().get( itmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].pointto_count_ = itmp;
            err = p["attentionto"].get_int64().get( itmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].attentionto_count_ = itmp;
            err = p["change_focus"].get_int64().get( itmp );
            if ( err == simdjson::SUCCESS ) show.player_[i].change_focus_count_ = itmp;

            ++i;
        }
    }
    catch ( std::exception & e )
    {
        std::cerr << "(ParserSimdJSON::parseShow) player part " << i << ": " << e.what() << std::endl;
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

    handler.handleLogVersion( REC_VERSION_JSON );

    try
    {
        simdjson::ondemand::parser parser;
        simdjson::padded_string json( std::string( std::istreambuf_iterator< char >( is ), {} ) );
        simdjson::ondemand::document rcg = parser.iterate( json );
        simdjson::ondemand::array root_array = rcg.get_array();

        for ( simdjson::ondemand::object data : root_array )
        {
            for ( simdjson::ondemand::field field : data )
            {
                if ( ! M_impl->parseData( field, handler ) )
                {
                    return false;
                }
            }
        }
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
bool
ParserSimdJSON::parse( const std::string & filepath,
                       Handler & handler ) const
{
    handler.handleLogVersion( REC_VERSION_JSON );

    try
    {
        simdjson::ondemand::parser parser;
        simdjson::padded_string json = simdjson::padded_string::load( filepath );
        simdjson::ondemand::document rcg = parser.iterate( json );
        simdjson::ondemand::array root_array = rcg.get_array();

        for ( simdjson::ondemand::object data : root_array )
        {
            for ( simdjson::ondemand::field field : data )
            {
                if ( ! M_impl->parseData( field, handler ) )
                {
                    return false;
                }
            }
        }
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
bool
ParserSimdJSON::parseData( const std::string & input,
                           Handler & handler ) const
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

    return true;
}

}
}
