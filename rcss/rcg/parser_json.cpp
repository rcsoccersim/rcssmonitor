// -*-c++-*-

/*!
  \file parser_json.cpp
  \brief rcg v6 (JSON format) parser Source File.
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

#include "parser_json.h"

#include "handler.h"
#include "types.h"
#include "xpm_tile.h"

#include "nlohmann/json.hpp"

#include <unordered_map>
#include <functional>
#include <iostream>

namespace rcss {
namespace rcg {

struct ParserJSON::Impl {

    typedef std::function< bool( const nlohmann::json & data, Handler & handler ) > Func;

    std::unordered_map< std::string, Func > funcs_;

    Impl();

    bool parseData( const nlohmann::json & data,
                    Handler & hander );

    bool parseHeader( const nlohmann::json & data,
                      Handler & handler );

    bool parseShow( const nlohmann::json & data,
                    Handler & handler );
    bool setBall( const nlohmann::json & data,
                  ShowInfoT & show );
    bool setPlayers( const nlohmann::json & data,
                     ShowInfoT & show );
    bool setPlayer( const nlohmann::json & player,
                    ShowInfoT & show );

    bool parsePlayMode( const nlohmann::json & data,
                        Handler & handler );
    PlayMode getPlayModeId( const std::string & pmode_str );

    bool parseTeam( const nlohmann::json & data,
                    Handler & handler );
    bool setTeams( const nlohmann::json & teams,
                   TeamT & team_l,
                   TeamT & team_r );

    bool parseMsg( const nlohmann::json & data,
                   Handler & handler );
    bool parseTeamGraphic( const nlohmann::json & data,
                           Handler & handler );

    bool parseServerParam( const nlohmann::json & data,
                           Handler & handler );
    bool parsePlayerParam( const nlohmann::json & data,
                           Handler & handler );
    bool parsePlayerType( const nlohmann::json & data,
                           Handler & handler );

    Side toSide( const std::string & str )
    {
        return ( str == "l"
                 ? LEFT
                 : str == "r"
                 ? RIGHT
                 : NEUTRAL );
    }
};


/*-------------------------------------------------------------------*/
ParserJSON::Impl::Impl()
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    funcs_["show"] = std::bind( &Impl::parseShow, this, _1, _2 );
    funcs_["playmode"] = std::bind( &Impl::parsePlayMode, this, _1, _2 );
    funcs_["team"] = std::bind( &Impl::parseTeam, this, _1, _2 );
    funcs_["msg"] = std::bind( &Impl::parseMsg, this, _1, _2 );

    funcs_["team_graphic"] = std::bind( &Impl::parseTeamGraphic, this, _1, _2 );

    funcs_["server_param"] = std::bind( &Impl::parseServerParam, this, _1, _2 );
    funcs_["player_param"] = std::bind( &Impl::parsePlayerParam, this, _1, _2 );
    funcs_["player_type"] = std::bind( &Impl::parsePlayerType, this, _1, _2 );

    funcs_["header"] = std::bind( &Impl::parseHeader, this, _1, _2 );
}

/*-------------------------------------------------------------------*/
bool
ParserJSON::Impl::parseData( const nlohmann::json & data,
                             Handler & handler )
{
    if ( ! data.contains( "type" ) )
    {
        return false;
    }

    std::unordered_map< std::string, Func >::iterator it = funcs_.find( data.at( "type" ) );
    if ( it == funcs_.end() )
    {
        return false;
    }

    return it->second( data, handler );
}

/*-------------------------------------------------------------------*/
bool
ParserJSON::Impl::parseHeader( const nlohmann::json & /*data*/,
                               Handler & /*handler*/ )
{
    return true;
}

/*-------------------------------------------------------------------*/
bool
ParserJSON::Impl::parseShow( const nlohmann::json & data,
                             Handler & handler )
{
    int time, stime;

    try
    {
        time = data.at( "time" );
        stime = ( data.contains( "stime" ) ? data.at( "stime" ).get< int >() : 0 );
    }
    catch ( std::exception & e )
    {
        std::cerr << "(rcss::rcg::ParserJSON::Impl::parseShow) Illegal elment in show data.\n"
                  << e.what() << std::endl;
        return false;
    }

    ShowInfoT show;

    show.time_ = time;
    show.stime_ = stime;

    if ( data.contains( "mode" ) )
    {
        PlayMode pm = getPlayModeId( data.at( "mode" ) );
        if ( ! handler.handlePlayMode( time, pm ) )
        {
            return false;
        }
    }

    if ( data.contains( "teams" ) )
    {
        TeamT team_l, team_r;
        setTeams( data.at( "teams" ), team_l, team_r );
        if ( ! handler.handleTeam( time, team_l, team_r ) )
        {
            return false;
        }
    }

    if ( ! data.contains( "ball" )
         || ! setBall( data, show ) )
    {
        return false;
    }

    if ( ! data.contains( "players" )
         || ! setPlayers( data, show ) )
    {
        return false;
    }

    //return handler.handleShow( time, stime, show );
    return handler.handleShow( show );
}

/*-------------------------------------------------------------------*/
bool
ParserJSON::Impl::setBall( const nlohmann::json & data,
                           ShowInfoT & show )
{
    try
    {
        const nlohmann::json & ball = data.at( "ball" );
        show.ball_.x_ = ball.at( "x" );
        show.ball_.y_ = ball.at( "y" );
        show.ball_.vx_ = ball.at( "vx" );
        show.ball_.vy_ = ball.at( "vx" );
    }
    catch ( std::exception & e )
    {
        std::cerr << "(rcss::rcg::ParserJSON::Impl::setBall) Illegal element in ball data.\n"
                  << e.what() << std::endl;
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
bool
ParserJSON::Impl::setPlayers( const nlohmann::json & data,
                              ShowInfoT & show )
{
    try
    {
        const nlohmann::json & player_array = data.at( "players" );
        if ( ! player_array.is_array() )
        {
            std::cerr << "(rcss::rcg::ParserJSON::Impl::setPlayers) No array in players data." << std::endl;
            return false;
        }

        for ( const nlohmann::json & p : player_array )
        {
            if ( ! setPlayer( p, show ) )
            {
                return false;
            }
        }
    }
    catch ( std::exception & e )
    {
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
bool
ParserJSON::Impl::setPlayer( const nlohmann::json & player,
                             ShowInfoT & show )
{
    try
    {
        const Side side = toSide( player.at( "side" ) );
        const int unum = player.at( "unum" );

        if ( side == NEUTRAL )
        {
            std::cerr << "(rcss::rcg::ParserJSON::Impl::setPlayer) Illegal side "
                      << player.at( "side" ) << " in player data." << std::endl;
            return false;
        }

        if ( unum < 1 || MAX_PLAYER < unum )
        {
            std::cerr << "(rcss::rcg::ParserJSON::Impl::setPlayer) Illegal unum " << unum
                      << " in player data." << std::endl;
            return false;
        }

        PlayerT * p = ( side == LEFT
                        ? &show.players_[unum-1]
                        : &show.players_[MAX_PLAYER+unum-1] );

        p->side_ = side;
        p->unum_ = unum;
        p->type_ = player.at( "type" );

        p->high_quality_ = ( player.at( "vq" ) == "h" ? true : false );

        if ( player.contains( "fside" ) ) p->focus_side_ = toSide( player.at( "fside" ) );
        if ( player.contains( "fnum" ) ) p->focus_unum_ = player.at( "fnum" );

        p->state_ = player.at( "state" );

        p->x_ = player.at( "x" );
        p->y_ = player.at( "y" );
        p->vx_ = player.at( "vx" );
        p->vy_ = player.at( "vy" );
        p->body_ = player.at( "body" );
        p->neck_ = player.at( "neck" );
        if ( player.contains( "px" ) ) p->point_x_ = player.at( "px" );
        if ( player.contains( "py" ) ) p->point_y_ = player.at( "py" );

        if ( player.contains("focusx") ) p->focus_point_x = player.at("focusx");
        if ( player.contains("focusy") ) p->focus_point_y = player.at("focusy");

        p->view_width_ = player.at( "vw" );

        p->stamina_ = player.at( "stamina" );
        p->effort_ = player.at( "effort" );
        p->recovery_ = player.at( "recovery" );
        p->stamina_capacity_ = player.at( "capacity" );

        if ( player.contains( "count" ) )
        {
            const nlohmann::json & counts = player.at( "count" );
            p->kick_count_ = counts.at( "kick" );
            p->dash_count_ = counts.at( "dash" );
            p->turn_count_ = counts.at( "turn" );
            p->catch_count_= counts.at( "catch" );
            p->move_count_ = counts.at( "move" );
            p->turn_neck_count_ = counts.at( "turn_neck" );
            p->change_view_count_ = counts.at( "change_view" );
            p->say_count_ = counts.at( "say" );
            p->tackle_count_ = counts.at( "tackle" );
            p->pointto_count_ = counts.at( "pointto" );
            p->attentionto_count_ = counts.at( "attentionto" );
            if ( counts.contains("set_focus") ) p->set_focus_count_ = counts.at("set_focus");
        }
    }
    catch ( std::exception & e )
    {
        std::cerr << "(rcss::rcg::ParserJSON::Impl::setPlayer) Illegal element in player data.\n"
                      << e.what() << std::endl;
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
bool
ParserJSON::Impl::parsePlayMode( const nlohmann::json & data,
                                 Handler & handler )
{
    int time = 0;
    // int stime = 0;
    std::string playmode;

    try
    {
        time = data.at( "time" );
        // stime = ( data.contains( "stime" )
        //           ? data.at( "stime" )
        //           : 0 );
        playmode = data.at( "mode" );
    }
    catch ( std::exception & e )
    {
        std::cerr << "(rcss::rcg::ParserJSON::Impl::parsePlayMode) Illegal element in playmode data.\n"
                  << e.what() << std::endl;
        return false;
    }

    PlayMode pm = getPlayModeId( data.at( "mode" ) );
    return handler.handlePlayMode( time, pm );
}

/*-------------------------------------------------------------------*/
PlayMode
ParserJSON::Impl::getPlayModeId( const std::string & pmode_str )
{
    static const char * playmode_strings[] = PLAYMODE_STRINGS;

    PlayMode pm = PM_Null;
    for ( int n = 0; n < PM_MAX; ++n )
    {
        if ( pmode_str == playmode_strings[n] )
        {
            pm = static_cast< PlayMode >( n );
            break;
        }
    }

    return pm;
}

/*-------------------------------------------------------------------*/
bool
ParserJSON::Impl::parseTeam( const nlohmann::json & data,
                             Handler & handler )
{
    int time;
    TeamT team_l, team_r;
    try
    {
        time = data.at( "time" );
        const nlohmann::json & teams = data.at( "teams" );
        if ( ! setTeams( teams, team_l, team_r ) )
        {
            return false;
        }
    }
    catch ( std::exception & e )
    {
        std::cerr << "(rcss::rcg::ParserJSON::Impl::parseTeam) Illegal element in team data.\n"
                  << e.what() << std::endl;
        return false;
    }


    return handler.handleTeam( time, team_l, team_r );
}

/*-------------------------------------------------------------------*/
bool
ParserJSON::Impl::setTeams( const nlohmann::json & teams,
                            TeamT & team_l,
                            TeamT & team_r )
{
    if ( ! teams.is_array() )
    {
        std::cerr << "(rcss::rcg::ParserJSON::Impl::setTeams) No array in team data." << std::endl;
        return false;
    }

    try
    {
        const nlohmann::json & data_l = teams.at( 0 );
        const nlohmann::json & data_r = teams.at( 1 );

        team_l.name_ = data_l.at( "name" ).is_null() ? "null" : data_l.at( "name" );
        team_l.score_ = data_l.at( "score" );
        if ( data_l.contains( "pen_score" ) ) team_l.pen_score_ = data_l.at( "pen_score" );
        if ( data_l.contains( "pen_miss" ) ) team_l.pen_miss_ = data_l.at( "pen_miss" );

        team_r.name_ = data_r.at( "name" ).is_null() ? "null" : data_r.at( "name" );
        team_r.score_ = data_r.at( "score" );
        if ( data_r.contains( "pen_score" ) ) team_r.pen_score_ = data_r.at( "pen_score" );
        if ( data_r.contains( "pen_miss" ) ) team_r.pen_miss_ = data_r.at( "pen_miss" );
    }
    catch ( std::exception & e )
    {
        std::cerr << "(rcss::rcg::ParserJSON::Impl::setTeams) Illegal data in team array.\n"
                  << e.what() << std::endl;
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
bool
ParserJSON::Impl::parseMsg( const nlohmann::json & data,
                            Handler & handler )
{
    try
    {
        return handler.handleMsg( data.at( "time" ),
                                  data.at( "board" ),
                                  data.at( "message" ) );
    }
    catch ( std::exception & e )
    {
        std::cerr << "(rcss::rcg::ParserJSON::Impl::parseMsg) Illegal elment in msg data.\n"
                  << e.what() << std::endl;
    }

    return false;
}

/*-------------------------------------------------------------------*/
bool
ParserJSON::Impl::parseTeamGraphic( const nlohmann::json & data,
                                    Handler & handler )
{
    try
    {
        std::shared_ptr< XpmTile > tile( new XpmTile() );

        const std::string side_str = data.at( "side" );
        const Side side = ( side_str == "l" ? LEFT :
                            side_str == "r" ? RIGHT :
                            NEUTRAL );

        const int x = data.at( "x" );
        const int y = data.at( "y" );

        //for ( const nlohmann::json & str : data.at( "xpm" ) )
        for ( const std::string & str : data.at( "xpm" ) )
        {
            if ( ! tile->addData( str.c_str() ) )
            {
                return false;
            }
        }

        return handler.handleTeamGraphic( side, x, y, tile );
    }
    catch ( std::exception & e )
    {
        std::cerr << "(rcss::rcg::ParserJSON::Impl::parseMsg) Illegal elment in team_graphic data.\n"
                  << e.what() << std::endl;
    }

    return true;
}

/*-------------------------------------------------------------------*/
bool
ParserJSON::Impl::parseServerParam( const nlohmann::json & data,
                                    Handler & handler )
{
    ServerParamT server_param;
    for ( const auto & param : data.at( "params" ).items() )
    {
        if ( param.value().is_number_integer() )
        {
            server_param.setInt( param.key(), param.value().get< int >() );
        }
        else if ( param.value().is_number() )
        {
            server_param.setDouble( param.key(), param.value().get< double >() );
        }
        else if ( param.value().is_boolean() )
        {
            server_param.setBool( param.key(), param.value().get< bool >() );
        }
        else if ( param.value().is_string() )
        {
            server_param.setString( param.key(), param.value().get< std::string >() );
        }
        else
        {
            std::cout << "Unsupported value type. name=" << param.key() << ", value=" << param.value() << std::endl;
        }
    }

    return handler.handleServerParam( server_param );
}

/*-------------------------------------------------------------------*/
bool
ParserJSON::Impl::parsePlayerParam( const nlohmann::json & data,
                                    Handler & handler )
{
    PlayerParamT player_param;
    for ( const auto & param : data.at( "params" ).items() )
    {
        if ( param.value().is_number_integer() )
        {
            player_param.setInt( param.key(), param.value().get< int >() );
        }
        else if ( param.value().is_number() )
        {
            player_param.setDouble( param.key(), param.value().get< double >() );
        }
        else if ( param.value().is_boolean() )
        {
            player_param.setBool( param.key(), param.value().get< bool >() );
        }
        else
        {
            std::cout << "Unsupported value type. name=" << param.key() << ", value=" << param.value() << std::endl;
        }
    }

    return handler.handlePlayerParam( player_param );
}

/*-------------------------------------------------------------------*/
bool
ParserJSON::Impl::parsePlayerType( const nlohmann::json & data,
                                   Handler & handler )
{
    if ( !  data.contains( "id" ) )
    {
        return false;
    }

    PlayerTypeT ptype;

    try
    {
        ptype.id_ = data.at( "id" );
    }
    catch ( std::exception & e )
    {
        std::cout << "Could not read the player type id.\n"
                  << e.what() << std::endl;
        return true;
    }

    for ( const auto & param : data.at( "params" ).items() )
    {
        if ( param.value().is_number() )
        {
            ptype.setDouble( param.key(), param.value().get< double >() );
        }
        else
        {
            std::cout << "Unsupported value type. name=" << param.key() << ", value=" << param.value() << std::endl;
        }
    }

    return handler.handlePlayerType( ptype );
}

/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
ParserJSON::ParserJSON()
    : M_impl( new Impl() )
{

}

/*-------------------------------------------------------------------*/
bool
ParserJSON::parse( std::istream & is,
                   Handler & handler )
{
    // streampos must be the first point!!!
    is.seekg( 0 );

    if ( ! is.good() )
    {
        return false;
    }


    std::string line;
    std::getline( is, line ); // skip the first line, "ULG6"

    nlohmann::json rcg;
    try
    {
        is >> rcg;
    }
    catch ( std::exception & e )
    {
        std::cerr << "(rcss::rcg::ParserJSON::parse) Could not read the valid json.\n"
                  << e.what() << std::endl;
        return false;
    }

    if ( ! rcg.is_array() )
    {
        std::cerr << "(rcss::rcg::ParserJSON::parse) Error. RCG is not an array" << std::endl;
        return false;
    }

    for ( const nlohmann::json & data : rcg )
    {
        if ( ! M_impl->parseData( data, handler ) )
        {
            return false;
        }
    }

    while ( std::getline( is, line ) )
    {
        // skip all
    }

    return handler.handleEOF();
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ParserJSON::parseData( const std::string & input,
                       Handler & handler )
{
    nlohmann::json data;
    try
    {
        data = nlohmann::json::parse( input );
    }
    catch ( std::exception & e )
    {
        std::cerr << "(rcss::rcg::ParserJSON::parseData) Could not read the valid json.\n"
                  << e.what() << std::endl;
        return false;
    }

    return M_impl->parseData( data, handler );
}

} // end of namespace
} // end of namespace
