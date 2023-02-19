// -*-c++-*-

/*!
  \file types.cpp
  \brief Type definitions for rcg data format.
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

#include "types.h"

#include <iostream>
#include <iomanip>
#include <cmath>

namespace {

inline
float
quantize( const double val,
          const double prec )
{
    return rint( val / prec ) * prec;
}

template < typename T >
void
to_sexp( std::ostream & os,
         const char * name,
         const T & value )
{
    os << '(' << name << ' ' << value << ')';
}

template <>
void
to_sexp< std::string >( std::ostream & os,
                        const char * name,
                        const std::string & value )
{
    os << '(' << name << ' ' << std::quoted( value ) << ')';
}

/*-------------------------------------------------------------------*/
// remove quatations and replace escaped quoation characters
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
template< typename ParamHolder >
bool
parse_param( const std::string & msg,
             ParamHolder & holder )
             // IntMap & int_map,
             // DoubleMap & double_map,
             // BoolMap & bool_map,
             // StringMap & string_map )
{
    int n_read = 0;

    char message_name[32];
    if ( std::sscanf( msg.c_str(), " ( %31s %n ", message_name, &n_read ) != 1 )
    {
        std::cerr << ":error: failed to parse the message type." << std::endl;
        return false;
    }

    for ( std::string::size_type pos = msg.find_first_of( '(', n_read );
          pos != std::string::npos;
          pos = msg.find_first_of( '(', pos ) )
    {
        std::string::size_type end_pos = msg.find_first_of( ' ', pos );
        if ( end_pos == std::string::npos )
        {
            std::cerr << "(rcss::rcg::parse_param) Could not find the seprator space." << std::endl;
            return false;
        }
        pos += 1;

        const std::string name_str( msg, pos, end_pos - pos );

        pos = end_pos; // pos indcates the position of the white space after the param name

        // search end paren or double quatation
        end_pos = msg.find_first_of( ")\"", end_pos ); //"
        if ( end_pos == std::string::npos )
        {
            std::cerr << "(rcss::rcg::parse_param) Could not find the parameter value for [" << name_str << ']'
                      << std::endl;
            return false;
        }

        // found quated value
        if ( msg[end_pos] == '\"' )
        {
            pos = end_pos;
            end_pos = msg.find_first_of( '\"', end_pos + 1 ); //"
            if ( end_pos == std::string::npos )
            {
                std::cerr << "(rcss::rcg::parse_param) Could not parse the quated value for [" << name_str << ']'
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

        std::string value_str( msg, pos, end_pos - pos );
        pos = end_pos;

        // pos indicates the position of the end of paren

        holder.setValue( name_str, value_str );
#if 0
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
#endif
    }

    return true;
}

}

namespace rcss {
namespace rcg {

PlayerTypeT::PlayerTypeT()
    : id_( 0 ),
      player_speed_max_( 1.2 ),
      stamina_inc_max_( 45.0 ),
      player_decay_( 0.4 ),
      inertia_moment_( 5.0 ),
      dash_power_rate_( 0.06 ),
      player_size_( 0.3 ),
      kickable_margin_( 0.7 ),
      kick_rand_( 0.1 ),
      extra_stamina_( 0.0 ),
      effort_max_( 1.0 ),
      effort_min_( 0.6 ),
      kick_power_rate_( 0.027 ),
      foul_detect_probability_( 0.5 ),
      catchable_area_l_stretch_( 1.0 )
{
    int_map_.insert( IntMap::value_type( "id", &id_ ) );

    double_map_.insert( DoubleMap::value_type( "player_speed_max", &player_speed_max_ ) );
    double_map_.insert( DoubleMap::value_type( "stamina_inc_max", &stamina_inc_max_ ) );
    double_map_.insert( DoubleMap::value_type( "player_decay", &player_decay_ ) );
    double_map_.insert( DoubleMap::value_type( "inertia_moment", &inertia_moment_ ) );
    double_map_.insert( DoubleMap::value_type( "dash_power_rate", &dash_power_rate_ ) );
    double_map_.insert( DoubleMap::value_type( "player_size", &player_size_ ) );
    double_map_.insert( DoubleMap::value_type( "kickable_margin", &kickable_margin_ ) );
    double_map_.insert( DoubleMap::value_type( "kick_rand", &kick_rand_ ) );
    double_map_.insert( DoubleMap::value_type( "extra_stamina", &extra_stamina_ ) );
    double_map_.insert( DoubleMap::value_type( "effort_max", &effort_max_ ) );
    double_map_.insert( DoubleMap::value_type( "effort_min", &effort_min_ ) );
    // 14.0.0
    double_map_.insert( DoubleMap::value_type( "kick_power_rate", &kick_power_rate_ ) );
    double_map_.insert( DoubleMap::value_type( "foul_detect_probability", &foul_detect_probability_ ) );
    double_map_.insert( DoubleMap::value_type( "catchable_area_l_stretch", &catchable_area_l_stretch_ ) );
    // 18.0
    double_map_.insert( DoubleMap::value_type( "unum_far_length", &unum_far_length_ ) );
    double_map_.insert( DoubleMap::value_type( "unum_too_far_length", &unum_too_far_length_ ) );
    double_map_.insert( DoubleMap::value_type( "team_far_length", &team_far_length_ ) );
    double_map_.insert( DoubleMap::value_type( "team_too_far_length", &team_too_far_length_ ) );
    double_map_.insert( DoubleMap::value_type( "player_max_observation_length", &player_max_observation_length_ ) );
    double_map_.insert( DoubleMap::value_type( "ball_vel_far_length", &ball_vel_far_length_ ) );
    double_map_.insert( DoubleMap::value_type( "ball_vel_too_far_length", &ball_vel_too_far_length_ ) );
    double_map_.insert( DoubleMap::value_type( "ball_max_observation_length", &ball_max_observation_length_ ) );
    double_map_.insert( DoubleMap::value_type( "flag_chg_far_length", &flag_chg_far_length_ ) );
    double_map_.insert( DoubleMap::value_type( "flag_chg_too_far_length", &flag_chg_too_far_length_ ) );
    double_map_.insert( DoubleMap::value_type( "flag_max_observation_length", &flag_max_observation_length_ ) );
}


std::ostream &
PlayerTypeT::toSExp( std::ostream & os ) const
{
    os << "(player_type ";
    to_sexp( os, "id", id_ );
    to_sexp( os, "player_speed_max", quantize( player_speed_max_, 0.00001 ) );
    to_sexp( os, "stamina_inc_max", quantize( stamina_inc_max_, 0.00001 ) );
    to_sexp( os, "player_decay", quantize(player_decay_, 0.000001 ) );
    to_sexp( os, "inertia_moment", quantize( inertia_moment_, 0.00001 ) );
    to_sexp( os, "dash_power_rate", quantize( dash_power_rate_, 0.00000001 ) );
    to_sexp( os, "player_size", quantize( player_size_, 0.00001 ) );
    to_sexp( os, "kickable_margin", quantize( kickable_margin_, 0.000001 ) );
    to_sexp( os, "kick_rand", quantize( kick_rand_, 0.000001 ) );
    to_sexp( os, "extra_stamina", quantize( extra_stamina_, 0.00001 ) );
    to_sexp( os, "effort_max", quantize( effort_max_, 0.000001 ) );
    to_sexp( os, "effort_min", quantize( effort_min_, 0.000001 ) );
    // 14.0
    to_sexp( os, "kick_power_rate", quantize( kick_power_rate_, 0.000001 ) );
    to_sexp( os, "foul_detect_probability", quantize( foul_detect_probability_, 0.000001 ) );
    to_sexp( os, "catchable_area_l_stretch", quantize( catchable_area_l_stretch_, 0.000001 ) );
    // 18.0
    to_sexp( os, "unum_far_length", quantize( unum_far_length_, 0.000001 ) );
    to_sexp( os, "unum_too_far_length", quantize( unum_too_far_length_, 0.000001 ) );
    to_sexp( os, "team_far_length", quantize( team_far_length_, 0.000001 ) );
    to_sexp( os, "team_too_far_length", quantize( team_too_far_length_, 0.000001 ) );
    to_sexp( os, "player_max_observation_length", quantize( player_max_observation_length_, 0.000001 ) );
    to_sexp( os, "ball_vel_far_length", quantize( ball_vel_far_length_, 0.000001 ) );
    to_sexp( os, "ball_vel_too_far_length", quantize( ball_vel_too_far_length_, 0.000001 ) );
    to_sexp( os, "ball_max_observation_length", quantize( ball_max_observation_length_, 0.000001 ) );
    to_sexp( os, "flag_chg_far_length", quantize( flag_chg_far_length_, 0.000001 ) );
    to_sexp( os, "flag_chg_too_far_length", quantize( flag_chg_too_far_length_, 0.000001 ) );
    to_sexp( os, "flag_max_observation_length", quantize( flag_max_observation_length_, 0.000001 ) );
    os << ')';

    return os;
}

bool
PlayerTypeT::createFromSExp( const std::string & msg )
{
    BoolMap bool_map;
    StringMap string_map;

    //if ( ! parse_param( msg, int_map_, double_map_, bool_map, string_map ) )
    if ( ! parse_param( msg, *this ) )
    {
        // nothing to do
    }

    return true;
}

bool
PlayerTypeT::setValue( const std::string & name,
                       const std::string & value )
{
    try
    {
        IntMap::iterator int_it = int_map_.find( name );
        if ( int_it != int_map_.end() )
        {
            *(int_it->second) = std::stoi( value );
            return true;
        }

        DoubleMap::iterator double_it = double_map_.find( name );
        if ( double_it != double_map_.end() )
        {
            *(double_it->second) = std::stod( value );
            return true;
        }
    }
    catch ( std::exception & e )
    {
        std::cerr << e.what() << std::endl;
    }

    std::cerr << "(playerTypeT::setValue) Unsupported parameter. name=" << name << " value=" << value << std::endl;
    return true;
}


bool
PlayerTypeT::setInt( const std::string & name,
                     const int value )
{
    IntMap::iterator int_it = int_map_.find( name );
    if ( int_it != int_map_.end() )
    {
        *(int_it->second) = value;
        return true;
    }

    DoubleMap::iterator double_it = double_map_.find( name );
    if ( double_it != double_map_.end() )
    {
        *(double_it->second) = value;
        return true;
    }

    std::cerr << "(playerTypeT::setInt) Unsupported parameter. name=" << name << " value=" << value << std::endl;
    return true;
}

bool
PlayerTypeT::setDouble( const std::string & name,
                        const double value )
{
    DoubleMap::iterator double_it = double_map_.find( name );
    if ( double_it != double_map_.end() )
    {
        *(double_it->second) = value;
        return true;
    }

    std::cerr << "(playerTypeT::setDouble) Unsupported parameter. name=" << name << " value=" << value << std::endl;
    return true;
}


PlayerParamT::PlayerParamT()
    : player_types_( 18 ),
      subs_max_( 3 ),
      pt_max_( 1 ),
      allow_mult_default_type_( false ),
      player_speed_max_delta_min_( 0.0 ),
      player_speed_max_delta_max_( 0.0 ),
      stamina_inc_max_delta_factor_( 0.0 ),
      player_decay_delta_min_( -0.05 ),
      player_decay_delta_max_( 0.1 ),
      inertia_moment_delta_factor_( 25.0 ),
      dash_power_rate_delta_min_( 0.0 ),
      dash_power_rate_delta_max_( 0.0 ),
      player_size_delta_factor_( -100.0 ),
      kickable_margin_delta_min_( -0.1 ),
      kickable_margin_delta_max_( 0.1 ),
      kick_rand_delta_factor_( 1.0 ),
      extra_stamina_delta_min_( 0.0 ),
      extra_stamina_delta_max_( 100.0 ),
      effort_max_delta_factor_( -0.002 ),
      effort_min_delta_factor_( -0.002 ),
      new_dash_power_rate_delta_min_( -0.0005 ),
      new_dash_power_rate_delta_max_( 0.0015 ),
      new_stamina_inc_max_delta_factor_( -6000.0 ),
      random_seed_( -1 ),
      kick_power_rate_delta_min_( 0.0 ),
      kick_power_rate_delta_max_( 0.0 ),
      foul_detect_probability_delta_factor_( 0.0 ),
      catchable_area_l_stretch_min_( 0.0 ),
      catchable_area_l_stretch_max_( 0.0 )
{
    int_map_.insert( IntMap::value_type( "player_types", &player_types_ ) );
    int_map_.insert( IntMap::value_type( "subs_max", &subs_max_ ) );
    int_map_.insert( IntMap::value_type( "pt_max", &pt_max_ ) );
    bool_map_.insert( BoolMap::value_type( "allow_mult_default_type", &allow_mult_default_type_ ) );
    double_map_.insert( DoubleMap::value_type( "player_speed_max_delta_min", &player_speed_max_delta_min_ ) );
    double_map_.insert( DoubleMap::value_type( "player_speed_max_delta_max", &player_speed_max_delta_max_ ) );
    double_map_.insert( DoubleMap::value_type( "stamina_inc_max_delta_factor", &stamina_inc_max_delta_factor_ ) );
    double_map_.insert( DoubleMap::value_type( "player_decay_delta_min", &player_decay_delta_min_ ) );
    double_map_.insert( DoubleMap::value_type( "player_decay_delta_max", &player_decay_delta_max_ ) );
    double_map_.insert( DoubleMap::value_type( "inertia_moment_delta_factor", &inertia_moment_delta_factor_ ) );
    double_map_.insert( DoubleMap::value_type( "dash_power_rate_delta_min", &dash_power_rate_delta_min_ ) );
    double_map_.insert( DoubleMap::value_type( "dash_power_rate_delta_max", &dash_power_rate_delta_max_ ) );
    double_map_.insert( DoubleMap::value_type( "player_size_delta_factor", &player_size_delta_factor_ ) );
    double_map_.insert( DoubleMap::value_type( "kickable_margin_delta_min", &kickable_margin_delta_min_ ) );
    double_map_.insert( DoubleMap::value_type( "kickable_margin_delta_max", &kickable_margin_delta_max_ ) );
    double_map_.insert( DoubleMap::value_type( "kick_rand_delta_factor", &kick_rand_delta_factor_ ) );
    double_map_.insert( DoubleMap::value_type( "extra_stamina_delta_min", &extra_stamina_delta_min_ ) );
    double_map_.insert( DoubleMap::value_type( "extra_stamina_delta_max", &extra_stamina_delta_max_ ) );
    double_map_.insert( DoubleMap::value_type( "effort_max_delta_factor", &effort_max_delta_factor_ ) );
    double_map_.insert( DoubleMap::value_type( "effort_min_delta_factor", &effort_min_delta_factor_ ) );
    int_map_.insert( IntMap::value_type( "random_seed", &random_seed_ ) );
    double_map_.insert( DoubleMap::value_type( "new_dash_power_rate_delta_min", &new_dash_power_rate_delta_min_ ) );
    double_map_.insert( DoubleMap::value_type( "new_dash_power_rate_delta_max", &new_dash_power_rate_delta_max_ ) );
    double_map_.insert( DoubleMap::value_type( "new_stamina_inc_max_delta_factor", &new_stamina_inc_max_delta_factor_ ) );
    // 14.0.0
    double_map_.insert( DoubleMap::value_type( "kick_power_rate_delta_min", &kick_power_rate_delta_min_ ) );
    double_map_.insert( DoubleMap::value_type( "kick_power_rate_delta_max", &kick_power_rate_delta_max_ ) );
    double_map_.insert( DoubleMap::value_type( "foul_detect_probability_delta_factor", &foul_detect_probability_delta_factor_ ) );
    double_map_.insert( DoubleMap::value_type( "catchable_area_l_stretch_min", &catchable_area_l_stretch_min_ ) );
    double_map_.insert( DoubleMap::value_type( "catchable_area_l_stretch_max", &catchable_area_l_stretch_max_ ) );
}


std::ostream &
PlayerParamT::toSExp( std::ostream & os ) const
{
    os << "(player_param ";
    to_sexp( os, "player_types", player_types_ );
    to_sexp( os, "subs_max", subs_max_ );
    to_sexp( os, "pt_max", pt_max_ );
    to_sexp( os, "allow_mult_default_type", allow_mult_default_type_ );
    to_sexp( os, "player_speed_max_delta_min", quantize( player_speed_max_delta_min_, 0.00001 ) );
    to_sexp( os, "player_speed_max_delta_max", quantize( player_speed_max_delta_max_, 0.00001 ) );
    to_sexp( os, "stamina_inc_max_delta_factor", quantize( stamina_inc_max_delta_factor_, 0.00001 ) );
    to_sexp( os, "player_decay_delta_min", quantize( player_decay_delta_min_, 0.00001 ) );
    to_sexp( os, "player_decay_delta_max", quantize( player_decay_delta_max_, 0.00001 ) );
    to_sexp( os, "inertia_moment_delta_factor", quantize( inertia_moment_delta_factor_, 0.00001 ) );
    to_sexp( os, "dash_power_rate_delta_min", quantize( dash_power_rate_delta_min_, 0.00001 ) );
    to_sexp( os, "dash_power_rate_delta_max", quantize( dash_power_rate_delta_max_, 0.00001 ) );
    to_sexp( os, "player_size_delta_factor", quantize( player_size_delta_factor_, 0.00001 ) );
    to_sexp( os, "kickable_margin_delta_min", quantize( kickable_margin_delta_min_, 0.00001 ) );
    to_sexp( os, "kickable_margin_delta_max", quantize( kickable_margin_delta_max_, 0.00001 ) );
    to_sexp( os, "kick_rand_delta_factor", quantize( kick_rand_delta_factor_, 0.00001 ) );
    to_sexp( os, "extra_stamina_delta_min", quantize( extra_stamina_delta_min_, 0.00001 ) );
    to_sexp( os, "extra_stamina_delta_max", quantize( extra_stamina_delta_max_, 0.00001 ) );
    to_sexp( os, "effort_max_delta_factor", quantize( effort_max_delta_factor_, 0.00001 ) );
    to_sexp( os, "effort_min_delta_factor", quantize( effort_min_delta_factor_, 0.00001 ) );
    to_sexp( os, "random_seed", random_seed_ );
    to_sexp( os, "new_dash_power_rate_delta_min", quantize( new_dash_power_rate_delta_min_, 0.00001 ) );
    to_sexp( os, "new_dash_power_rate_delta_max", quantize( new_dash_power_rate_delta_max_, 0.00001 ) );
    to_sexp( os, "new_stamina_inc_max_delta_factor", quantize( new_stamina_inc_max_delta_factor_, 0.00001 ) );
    // 14.0
    to_sexp( os, "kick_power_rate_delta_min", quantize( kick_power_rate_delta_min_, 0.000001 ) );
    to_sexp( os, "kick_power_rate_delta_max", quantize( kick_power_rate_delta_max_, 0.000001 ) );
    to_sexp( os, "foul_detect_probability_delta_factor", quantize( foul_detect_probability_delta_factor_, 0.00001 ) );
    to_sexp( os, "catchable_area_l_stretch_min", quantize( catchable_area_l_stretch_min_, 0.00001 ) );
    to_sexp( os, "catchable_area_l_stretch_max", quantize( catchable_area_l_stretch_max_, 0.00001 ) );
    os << ')';

    return os;
}


bool
PlayerParamT::createFromSExp( const std::string & msg )
{
    StringMap string_map;

    //if ( ! parse_param( msg, int_map_, double_map_, bool_map_, string_map ) )
    if ( ! parse_param( msg, *this ) )
    {
        // nothing to do
    }

    return true;
}

bool
PlayerParamT::setValue( const std::string & name,
                        const std::string & value )
{
    try
    {
        IntMap::iterator int_it = int_map_.find( name );
        if ( int_it != int_map_.end() )
        {
            *(int_it->second) = std::stoi( value );
            return true;
        }

        DoubleMap::iterator double_it = double_map_.find( name );
        if ( double_it != double_map_.end() )
        {
            *(double_it->second) = std::stod( value );
            return true;
        }

        BoolMap::iterator bool_it = bool_map_.find( name );
        if ( bool_it != bool_map_.end() )
        {
            if ( value == "0" || value == "false" || value == "off" )
            {
                *(bool_it->second) = false;
            }
            else if ( value == "1" || value == "true" || value == "on" )
            {
                *(bool_it->second) = true;
            }
            else
            {
                std::cerr << "Unknown bool value. name=" << name << " value=" << value << std::endl;
            }
            return true;
        }
    }
    catch ( std::exception & e )
    {
        std::cerr << e.what() << '\n'
                  << "name=" << name << " value=" << value << std::endl;
        return true;
    }

    std::cerr << "Unsupported parameter. name=" << name << " value=" << value << std::endl;
    return true;
}


bool
PlayerParamT::setInt( const std::string & name,
                      const int value )
{
    IntMap::iterator int_it = int_map_.find( name );
    if ( int_it != int_map_.end() )
    {
        *(int_it->second) = value;
        return true;
    }

    DoubleMap::iterator double_it = double_map_.find( name );
    if ( double_it != double_map_.end() )
    {
        *(double_it->second) = value;
        return true;
    }

    BoolMap::iterator bool_it = bool_map_.find( name );
    if ( bool_it != bool_map_.end() )
    {
        if ( value == 0 )
        {
            *(bool_it->second) = false;
            return true;
        }

        if ( value == 1 )
        {
            *(bool_it->second) = true;
            return true;
        }
    }

    std::cerr << "Unsupported parameter. name=" << name << " value=" << value << std::endl;
    return true;
}


bool
PlayerParamT::setDouble( const std::string & name,
                         const double value )
{
    DoubleMap::iterator it = double_map_.find( name );
    if ( it == double_map_.end() )
    {
        std::cerr << "Unsupported double parameter. name=" << name << " value=" << value << std::endl;
    }
    else
    {
        *(it->second) = value;
    }

    return true;
}


bool
PlayerParamT::setBool( const std::string & name,
                       const bool value )
{
    BoolMap::iterator it = bool_map_.find( name );
    if ( it == bool_map_.end() )
    {
        std::cerr << "Unsupported bool parameter. name=" << name << " value=" << value << std::endl;
    }
    else
    {
        *(it->second) = value;
    }

    return true;
}


ServerParamT::ServerParamT()
    : goal_width_( 14.02 ),
      inertia_moment_( 5.0 ),
      player_size_( 0.3 ),
      player_decay_( 0.4 ),
      player_rand_( 0.1 ),
      player_weight_( 60.0 ),
      player_speed_max_( 1.2 ),
      player_accel_max_( 1.0 ),
      stamina_max_( 4000.0 ),
      stamina_inc_max_( 45.0 ),
      recover_init_( 1.0 ),
      recover_dec_thr_( 0.3 ),
      recover_min_( 0.5 ),
      recover_dec_( 0.002 ),
      effort_init_( 1.0 ),
      effort_dec_thr_( 0.3 ),
      effort_min_( 0.6 ),
      effort_dec_( 0.005 ),
      effort_inc_thr_( 0.6 ),
      effort_inc_( 0.01 ),
      kick_rand_( 0.1 ),
      team_actuator_noise_( false ),
      player_rand_factor_l_( 1.0 ),
      player_rand_factor_r_( 1.0 ),
      kick_rand_factor_l_( 1.0 ),
      kick_rand_factor_r_( 1.0 ),
      ball_size_( 0.085 ),
      ball_decay_( 0.94 ),
      ball_rand_( 0.05 ),
      ball_weight_( 0.2 ),
      ball_speed_max_( 3.0 ),
      ball_accel_max_( 2.7 ),
      dash_power_rate_( 0.006 ),
      kick_power_rate_( 0.027 ),
      kickable_margin_( 0.7 ),
      control_radius_( 2.0 ),
      //control_radius_width_( 1.7 ),
      catch_probability_( 1.0 ),
      catchable_area_l_( 1.2 ),
      catchable_area_w_( 1.0 ),
      goalie_max_moves_( 2 ),
      max_power_( 100.0 ),
      min_power_( -100.0 ),
      max_moment_( 180.0 ),
      min_moment_( -180.0 ),
      max_neck_moment_( 180.0 ),
      min_neck_moment_( -180.0 ),
      max_neck_angle_( 90.0 ),
      min_neck_angle_( -90.0 ),
      visible_angle_( 90.0 ),
      visible_distance_( 3.0 ),
      audio_cut_dist_( 50.0 ),
      dist_quantize_step_( 0.1 ),
      landmark_dist_quantize_step_( 0.01 ),
      corner_kick_margin_( 1.0 ),
      wind_dir_( 0.0 ),
      wind_force_( 0.0 ),
      wind_angle_( 0.0 ),
      wind_rand_( 0.0 ),
      wind_none_( false ),
      use_wind_random_( false ),
      half_time_( 300 ),
      drop_ball_time_( 200 ),
      port_( 6000 ),
      coach_port_( 6001 ),
      online_coach_port_( 6002 ),
      say_coach_count_max_( 128 ),
      say_coach_msg_size_( 128 ),
      simulator_step_( 100 ),
      send_step_( 150 ),
      recv_step_( 10 ),
      sense_body_step_( 100 ),
      say_msg_size_( 10 ),
      clang_win_size_( 300 ),
      clang_define_win_( 1 ),
      clang_meta_win_( 1 ),
      clang_advice_win_( 1 ),
      clang_info_win_( 1 ),
      clang_del_win_( 1 ),
      clang_rule_win_( 1 ),
      clang_mess_delay_( 50 ),
      clang_mess_per_cycle_( 1 ),
      hear_max_( 1 ),
      hear_inc_( 1 ),
      hear_decay_( 1 ),
      catch_ban_cycle_( 5 ),
      coach_mode_( false ),
      coach_with_referee_mode_( false ),
      use_old_coach_hear_( false ),
      online_coach_look_step_( 100 ),
      use_offside_( true ),
      offside_active_area_size_( 2.5 ),
      kick_off_offside_( true ),
      verbose_( false ),
      offside_kick_margin_( 9.15 ),
      slow_down_factor_( 1 ),
      synch_mode_( false ),
      synch_offset_( 60 ),
      synch_micro_sleep_( 1 ),
      start_goal_l_( 0 ),
      start_goal_r_( 0 ),
      fullstate_l_( false ),
      fullstate_r_( false ),
      slowness_on_top_for_left_team_( 1.0 ),
      slowness_on_top_for_right_team_( 1.0 ),
      landmark_file_(),
      send_comms_( false ),
      text_logging_( true ),
      game_logging_( true ),
      game_log_version_( 4 ),
      text_log_dir_( "." ),
      game_log_dir_( "." ),
      text_log_fixed_name_( "rcssserver" ),
      game_log_fixed_name_( "rcssserver" ),
      text_log_fixed_( false ),
      game_log_fixed_( false ),
      text_log_dated_( true ),
      game_log_dated_( true ),
      log_date_format_( "%Y%m%d%H%M-" ),
      log_times_( false ),
      record_messages_( false ),
      text_log_compression_( 0 ),
      game_log_compression_( 0 ),
      profile_( false ),
      point_to_ban_( 5 ),
      point_to_duration_( 20 ),
      tackle_dist_( 2.0 ),
      tackle_back_dist_( 0.5 ),
      tackle_width_( 1.0 ),
      tackle_exponent_( 6.0 ),
      tackle_cycles_( 10 ),
      tackle_power_rate_( 0.027 ),
      freeform_wait_period_( 600 ),
      freeform_send_period_( 20 ),
      free_kick_faults_( true ),
      back_passes_( true ),
      proper_goal_kicks_( false ),
      stopped_ball_vel_( 0.01 ),
      max_goal_kicks_( 3 ),
      auto_mode_( false ),
      kick_off_wait_( 100 ),
      connect_wait_( 300 ),
      game_over_wait_( 100 ),
      team_l_start_(),
      team_r_start_(),
      keepaway_mode_( false ),
      keepaway_length_( 20.0 ),
      keepaway_width_( 20.0 ),
      keepaway_logging_( true ),
      keepaway_log_dir_( "." ),
      keepaway_log_fixed_name_( "rcssserver" ),
      keepaway_log_fixed_( false ),
      keepaway_log_dated_( true ),
      keepaway_start_( -1 ),
      nr_normal_halfs_( 2 ),
      nr_extra_halfs_( 2 ),
      penalty_shoot_outs_( true ),
      pen_before_setup_wait_( 30 ),
      pen_setup_wait_( 100 ),
      pen_ready_wait_( 50 ),
      pen_taken_wait_( 200 ),
      pen_nr_kicks_( 5 ),
      pen_max_extra_kicks_( 10 ),
      pen_dist_x_( 42.5 ),
      pen_random_winner_( false ),
      pen_max_goalie_dist_x_( 14.0 ),
      pen_allow_mult_kicks_( true ),
      pen_coach_moves_players_( true ),
      ball_stuck_area_( 3.0 ),
      coach_msg_file_(),
      max_tackle_power_( 100.0 ),
      max_back_tackle_power_( 50.0 ),
      player_speed_max_min_( 0.8 ),
      extra_stamina_( 0.0 ),
      synch_see_offset_( 30 ),
      max_monitors_( -1 ),
      extra_half_time_( 300 ),
      stamina_capacity_( -1.0 ),
      max_dash_angle_( 0.0 ),
      min_dash_angle_( 0.0 ),
      dash_angle_step_( 90.0 ),
      side_dash_rate_( 0.25 ),
      back_dash_rate_( 0.5 ),
      max_dash_power_( 100.0 ),
      min_dash_power_( -100.0 ),
      tackle_rand_factor_( 1.0 ),
      foul_detect_probability_( 0.5 ),
      foul_exponent_( 10.0 ),
      foul_cycles_( 5 ),
      golden_goal_( true ),
      red_card_probability_( 0.0 ),
      illegal_defense_duration_( 20 ),
      illegal_defense_number_( 0 ),
      illegal_defense_dist_x_( 16.5 ),
      illegal_defense_width_( 40.32 ),
      fixed_teamname_l_( "" ),
      fixed_teamname_r_( "" ),
      max_catch_angle_( 180.0 ),
      min_catch_angle_( -180.0 )
{
    double_map_.insert( DoubleMap::value_type( "goal_width", &goal_width_ ) );
    double_map_.insert( DoubleMap::value_type( "inertia_moment", &inertia_moment_ ) );
    double_map_.insert( DoubleMap::value_type( "player_size", &player_size_ ) );
    double_map_.insert( DoubleMap::value_type( "player_decay", &player_decay_ ) );
    double_map_.insert( DoubleMap::value_type( "player_rand", &player_rand_ ) );
    double_map_.insert( DoubleMap::value_type( "player_weight", &player_weight_ ) );
    double_map_.insert( DoubleMap::value_type( "player_speed_max", &player_speed_max_ ) );
    double_map_.insert( DoubleMap::value_type( "player_accel_max", &player_accel_max_ ) );
    double_map_.insert( DoubleMap::value_type( "stamina_max", &stamina_max_ ) );
    double_map_.insert( DoubleMap::value_type( "stamina_inc_max", &stamina_inc_max_ ) );
    double_map_.insert( DoubleMap::value_type( "recover_init", &recover_init_ ) ); // not necessary
    double_map_.insert( DoubleMap::value_type( "recover_dec_thr", &recover_dec_thr_ ) );
    double_map_.insert( DoubleMap::value_type( "recover_min", &recover_min_ ) );
    double_map_.insert( DoubleMap::value_type( "recover_dec", &recover_dec_ ) );
    double_map_.insert( DoubleMap::value_type( "effort_init", &effort_init_ ) );
    double_map_.insert( DoubleMap::value_type( "effort_dec_thr", &effort_dec_thr_ ) );
    double_map_.insert( DoubleMap::value_type( "effort_min", &effort_min_ ) );
    double_map_.insert( DoubleMap::value_type( "effort_dec", &effort_dec_ ) );
    double_map_.insert( DoubleMap::value_type( "effort_inc_thr", &effort_inc_thr_ ) );
    double_map_.insert( DoubleMap::value_type( "effort_inc",  &effort_inc_ ) );
    double_map_.insert( DoubleMap::value_type( "kick_rand", &kick_rand_ ) );
    bool_map_.insert( BoolMap::value_type( "team_actuator_noise", &team_actuator_noise_ ) );
    double_map_.insert( DoubleMap::value_type( "prand_factor_l", &player_rand_factor_l_ ) );
    double_map_.insert( DoubleMap::value_type( "prand_factor_r", &player_rand_factor_r_ ) );
    double_map_.insert( DoubleMap::value_type( "kick_rand_factor_l", &kick_rand_factor_l_ ) );
    double_map_.insert( DoubleMap::value_type( "kick_rand_factor_r", &kick_rand_factor_r_ ) );
    double_map_.insert( DoubleMap::value_type( "ball_size", &ball_size_ ) );
    double_map_.insert( DoubleMap::value_type( "ball_decay", &ball_decay_ ) );
    double_map_.insert( DoubleMap::value_type( "ball_rand", &ball_rand_ ) );
    double_map_.insert( DoubleMap::value_type( "ball_weight", &ball_weight_ ) );
    double_map_.insert( DoubleMap::value_type( "ball_speed_max", &ball_speed_max_ ) );
    double_map_.insert( DoubleMap::value_type( "ball_accel_max", &ball_accel_max_ ) );
    double_map_.insert( DoubleMap::value_type( "dash_power_rate", &dash_power_rate_ ) );
    double_map_.insert( DoubleMap::value_type( "kick_power_rate", &kick_power_rate_ ) );
    double_map_.insert( DoubleMap::value_type( "kickable_margin", &kickable_margin_ ) );
    double_map_.insert( DoubleMap::value_type( "control_radius", &control_radius_ ) );
    //( "control_radius_width", &control_radius_width_ ) );
    //( "kickable_area", &kickable_area_ ) ); // not needed
    double_map_.insert( DoubleMap::value_type( "catch_probability", &catch_probability_ ) );
    double_map_.insert( DoubleMap::value_type( "catchable_area_l", &catchable_area_l_ ) );
    double_map_.insert( DoubleMap::value_type( "catchable_area_w", &catchable_area_w_ ) );
    int_map_.insert( IntMap::value_type( "goalie_max_moves", &goalie_max_moves_ ) );
    double_map_.insert( DoubleMap::value_type( "maxpower", &max_power_ ) );
    double_map_.insert( DoubleMap::value_type( "minpower", &min_power_ ) );
    double_map_.insert( DoubleMap::value_type( "maxmoment", &max_moment_ ) );
    double_map_.insert( DoubleMap::value_type( "minmoment", &min_moment_ ) );
    double_map_.insert( DoubleMap::value_type( "maxneckmoment", &max_neck_moment_ ) );
    double_map_.insert( DoubleMap::value_type( "minneckmoment", &min_neck_moment_ ) );
    double_map_.insert( DoubleMap::value_type( "maxneckang", &max_neck_angle_ ) );
    double_map_.insert( DoubleMap::value_type( "minneckang", &min_neck_angle_ ) );
    double_map_.insert( DoubleMap::value_type( "visible_angle", &visible_angle_ ) );
    double_map_.insert( DoubleMap::value_type( "visible_distance", &visible_distance_ ) );
    double_map_.insert( DoubleMap::value_type( "audio_cut_dist", &audio_cut_dist_ ) );
    double_map_.insert( DoubleMap::value_type( "quantize_step", &dist_quantize_step_ ) );
    double_map_.insert( DoubleMap::value_type( "quantize_step_l", &landmark_dist_quantize_step_ ) );
    //( "quantize_step_dir", &dir_quantize_step_ ) );
    //( "quantize_step_dist_team_l", &dist_quantize_step_l_ ) );
    //( "quantize_step_dist_team_r", &dist_quantize_step_r_ ) );
    //( "quantize_step_dist_l_team_l", &landmark_dist_quantize_step_l_ ) );
    //( "quantize_step_dist_l_team_r", &landmark_dist_quantize_step_r_ ) );
    //( "quantize_step_dir_team_l", &dir_quantize_step_l_ ) );
    //( "quantize_step_dir_team_r", &dir_quantize_step_r_ ) );
    double_map_.insert( DoubleMap::value_type( "ckick_margin", &corner_kick_margin_ ) );
    double_map_.insert( DoubleMap::value_type( "wind_dir", &wind_dir_ ) );
    double_map_.insert( DoubleMap::value_type( "wind_force", &wind_force_ ) );
    double_map_.insert( DoubleMap::value_type( "wind_ang", &wind_angle_ ) );
    double_map_.insert( DoubleMap::value_type( "wind_rand", &wind_rand_ ) );
    bool_map_.insert( BoolMap::value_type( "wind_none", &wind_none_ ) );
    bool_map_.insert( BoolMap::value_type( "wind_random", &use_wind_random_ ) );
    int_map_.insert( IntMap::value_type( "half_time", &half_time_ ) );
    int_map_.insert( IntMap::value_type( "drop_ball_time", &drop_ball_time_ ) );
    int_map_.insert( IntMap::value_type( "port", &port_ ) );
    int_map_.insert( IntMap::value_type( "coach_port", &coach_port_ ) );
    int_map_.insert( IntMap::value_type( "olcoach_port", &online_coach_port_ ) );
    int_map_.insert( IntMap::value_type( "say_coach_cnt_max", &say_coach_count_max_ ) );
    int_map_.insert( IntMap::value_type( "say_coach_msg_size", &say_coach_msg_size_ ) );
    int_map_.insert( IntMap::value_type( "simulator_step", &simulator_step_ ) );
    int_map_.insert( IntMap::value_type( "send_step", &send_step_ ) );
    int_map_.insert( IntMap::value_type( "recv_step", &recv_step_ ) );
    int_map_.insert( IntMap::value_type( "sense_body_step", &sense_body_step_ ) );
    //( "lcm_step", &lcm_step_ ) ); // not needed
    int_map_.insert( IntMap::value_type( "say_msg_size", &say_msg_size_ ) );
    int_map_.insert( IntMap::value_type( "clang_win_size", &clang_win_size_ ) );
    int_map_.insert( IntMap::value_type( "clang_define_win", &clang_define_win_ ) );
    int_map_.insert( IntMap::value_type( "clang_meta_win", &clang_meta_win_ ) );
    int_map_.insert( IntMap::value_type( "clang_advice_win", &clang_advice_win_ ) );
    int_map_.insert( IntMap::value_type( "clang_info_win", &clang_info_win_ ) );
    int_map_.insert( IntMap::value_type( "clang_del_win", &clang_del_win_ ) );
    int_map_.insert( IntMap::value_type( "clang_rule_win", &clang_rule_win_ ) );
    int_map_.insert( IntMap::value_type( "clang_mess_delay", &clang_mess_delay_ ) );
    int_map_.insert( IntMap::value_type( "clang_mess_per_cycle", &clang_mess_per_cycle_ ) );
    int_map_.insert( IntMap::value_type( "hear_max", &hear_max_ ) );
    int_map_.insert( IntMap::value_type( "hear_inc", &hear_inc_ ) );
    int_map_.insert( IntMap::value_type( "hear_decay", &hear_decay_ ) );
    int_map_.insert( IntMap::value_type( "catch_ban_cycle", &catch_ban_cycle_ ) );
    bool_map_.insert( BoolMap::value_type( "coach", &coach_mode_ ) );
    bool_map_.insert( BoolMap::value_type( "coach_w_referee", &coach_with_referee_mode_ ) );
    bool_map_.insert( BoolMap::value_type( "old_coach_hear", &use_old_coach_hear_ ) );
    int_map_.insert( IntMap::value_type( "send_vi_step", &online_coach_look_step_ ) );
    bool_map_.insert( BoolMap::value_type( "use_offside", &use_offside_ ) );
    double_map_.insert( DoubleMap::value_type( "offside_kick_margin", &offside_kick_margin_ ) );
    bool_map_.insert( BoolMap::value_type( "forbid_kick_off_offside", &kick_off_offside_ ) );
    bool_map_.insert( BoolMap::value_type( "verbose", &verbose_ ) );
    double_map_.insert( DoubleMap::value_type( "offside_active_area_size", &offside_active_area_size_ ) );
    int_map_.insert( IntMap::value_type( "slow_down_factor", &slow_down_factor_ ) );
    bool_map_.insert( BoolMap::value_type( "synch_mode", &synch_mode_ ) );
    int_map_.insert( IntMap::value_type( "synch_offset", &synch_offset_ ) );
    int_map_.insert( IntMap::value_type( "synch_micro_sleep", &synch_micro_sleep_ ) );
    int_map_.insert( IntMap::value_type( "start_goal_l", &start_goal_l_ ) );
    int_map_.insert( IntMap::value_type( "start_goal_r", &start_goal_r_ ) );
    bool_map_.insert( BoolMap::value_type( "fullstate_l", &fullstate_l_ ) );
    bool_map_.insert( BoolMap::value_type( "fullstate_r", &fullstate_r_ ) );
    double_map_.insert( DoubleMap::value_type( "slowness_on_top_for_left_team", &slowness_on_top_for_left_team_ ) );
    double_map_.insert( DoubleMap::value_type( "slowness_on_top_for_right_team", &slowness_on_top_for_right_team_ ) );
    string_map_.insert( StringMap::value_type( "landmark_file", &landmark_file_ ) );
    bool_map_.insert( BoolMap::value_type( "send_comms", &send_comms_ ) );
    bool_map_.insert( BoolMap::value_type( "text_logging", &text_logging_ ) );
    bool_map_.insert( BoolMap::value_type( "game_logging", &game_logging_ ) );
    int_map_.insert( IntMap::value_type( "game_log_version", &game_log_version_ ) );
    string_map_.insert( StringMap::value_type( "text_log_dir", &text_log_dir_ ) );
    string_map_.insert( StringMap::value_type( "game_log_dir", &game_log_dir_ ) );
    string_map_.insert( StringMap::value_type( "text_log_fixed_name", &text_log_fixed_name_ ) );
    string_map_.insert( StringMap::value_type( "game_log_fixed_name", &game_log_fixed_name_ ) );
    bool_map_.insert( BoolMap::value_type( "text_log_fixed", &text_log_fixed_ ) );
    bool_map_.insert( BoolMap::value_type( "game_log_fixed", &game_log_fixed_ ) );
    bool_map_.insert( BoolMap::value_type( "text_log_dated", &text_log_dated_ ) );
    bool_map_.insert( BoolMap::value_type( "game_log_dated", &game_log_dated_ ) );
    string_map_.insert( StringMap::value_type( "log_date_format", &log_date_format_ ) );
    bool_map_.insert( BoolMap::value_type( "log_times", &log_times_ ) );
    bool_map_.insert( BoolMap::value_type( "record_messages", &record_messages_ ) );
    int_map_.insert( IntMap::value_type( "text_log_compression", &text_log_compression_ ) );
    int_map_.insert( IntMap::value_type( "game_log_compression", &game_log_compression_ ) );
    bool_map_.insert( BoolMap::value_type( "profile", &profile_ ) );
    int_map_.insert( IntMap::value_type( "point_to_ban", &point_to_ban_ ) );
    int_map_.insert( IntMap::value_type( "point_to_duration", &point_to_duration_ ) );
    double_map_.insert( DoubleMap::value_type( "tackle_dist", &tackle_dist_ ) );
    double_map_.insert( DoubleMap::value_type( "tackle_back_dist", &tackle_back_dist_ ) );
    double_map_.insert( DoubleMap::value_type( "tackle_width", &tackle_width_ ) );
    double_map_.insert( DoubleMap::value_type( "tackle_exponent", &tackle_exponent_ ) );
    int_map_.insert( IntMap::value_type( "tackle_cycles", &tackle_cycles_ ) );
    double_map_.insert( DoubleMap::value_type( "tackle_power_rate", &tackle_power_rate_ ) );
    int_map_.insert( IntMap::value_type( "freeform_wait_period", &freeform_wait_period_ ) );
    int_map_.insert( IntMap::value_type( "freeform_send_period", &freeform_send_period_ ) );
    bool_map_.insert( BoolMap::value_type( "free_kick_faults", &free_kick_faults_ ) );
    bool_map_.insert( BoolMap::value_type( "back_passes", &back_passes_ ) );
    bool_map_.insert( BoolMap::value_type( "proper_goal_kicks", &proper_goal_kicks_ ) );
    double_map_.insert( DoubleMap::value_type( "stopped_ball_vel", &stopped_ball_vel_ ) );
    int_map_.insert( IntMap::value_type( "max_goal_kicks", &max_goal_kicks_ ) );
    bool_map_.insert( BoolMap::value_type( "auto_mode", &auto_mode_ ) );
    int_map_.insert( IntMap::value_type( "kick_off_wait", &kick_off_wait_ ) );
    int_map_.insert( IntMap::value_type( "connect_wait", &connect_wait_ ) );
    int_map_.insert( IntMap::value_type( "game_over_wait", &game_over_wait_ ) );
    string_map_.insert( StringMap::value_type( "team_l_start", &team_l_start_ ) );
    string_map_.insert( StringMap::value_type( "team_r_start", &team_r_start_ ) );
    bool_map_.insert( BoolMap::value_type( "keepaway", &keepaway_mode_ ) );
    double_map_.insert( DoubleMap::value_type( "keepaway_length", &keepaway_length_ ) );
    double_map_.insert( DoubleMap::value_type( "keepaway_width", &keepaway_width_ ) );
    bool_map_.insert( BoolMap::value_type( "keepaway_logging", &keepaway_logging_ ) );
    string_map_.insert( StringMap::value_type( "keepaway_log_dir", &keepaway_log_dir_ ) );
    string_map_.insert( StringMap::value_type( "keepaway_log_fixed_name", &keepaway_log_fixed_name_ ) );
    bool_map_.insert( BoolMap::value_type( "keepaway_log_fixed", &keepaway_log_fixed_ ) );
    bool_map_.insert( BoolMap::value_type( "keepaway_log_dated", &keepaway_log_dated_ ) );
    int_map_.insert( IntMap::value_type( "keepaway_start", &keepaway_start_ ) );
    int_map_.insert( IntMap::value_type( "nr_normal_halfs", &nr_normal_halfs_ ) );
    int_map_.insert( IntMap::value_type( "nr_extra_halfs", &nr_extra_halfs_ ) );
    bool_map_.insert( BoolMap::value_type( "penalty_shoot_outs", &penalty_shoot_outs_ ) );
    int_map_.insert( IntMap::value_type( "pen_before_setup_wait", &pen_before_setup_wait_ ) );
    int_map_.insert( IntMap::value_type( "pen_setup_wait", &pen_setup_wait_ ) );
    int_map_.insert( IntMap::value_type( "pen_ready_wait", &pen_ready_wait_ ) );
    int_map_.insert( IntMap::value_type( "pen_taken_wait", &pen_taken_wait_ ) );
    int_map_.insert( IntMap::value_type( "pen_nr_kicks", &pen_nr_kicks_ ) );
    int_map_.insert( IntMap::value_type( "pen_max_extra_kicks", &pen_max_extra_kicks_ ) );
    double_map_.insert( DoubleMap::value_type( "pen_dist_x", &pen_dist_x_ ) );
    bool_map_.insert( BoolMap::value_type( "pen_random_winner", &pen_random_winner_ ) );
    double_map_.insert( DoubleMap::value_type( "pen_max_goalie_dist_x", &pen_max_goalie_dist_x_ ) );
    bool_map_.insert( BoolMap::value_type( "pen_allow_mult_kicks", &pen_allow_mult_kicks_ ) );
    bool_map_.insert( BoolMap::value_type( "pen_coach_moves_players", &pen_coach_moves_players_ ) );
    // v11
    double_map_.insert( DoubleMap::value_type( "ball_stuck_area", &ball_stuck_area_ ) );
    string_map_.insert( StringMap::value_type( "coach_msg_file", &coach_msg_file_ ) );
    // v12
    double_map_.insert( DoubleMap::value_type( "max_tackle_power", &max_tackle_power_ ) );
    double_map_.insert( DoubleMap::value_type( "max_back_tackle_power", &max_back_tackle_power_ ) );
    double_map_.insert( DoubleMap::value_type( "player_speed_max_min", &player_speed_max_min_ ) );
    double_map_.insert( DoubleMap::value_type( "extra_stamina", &extra_stamina_ ) );
    int_map_.insert( IntMap::value_type( "synch_see_offset", &synch_see_offset_ ) );
    int_map_.insert( IntMap::value_type( "max_monitors", &max_monitors_ ) );
    // v12.1.3
    int_map_.insert( IntMap::value_type( "extra_half_time", &extra_half_time_ ) );
    // v13
    double_map_.insert( DoubleMap::value_type( "stamina_capacity", &stamina_capacity_ ) );
    double_map_.insert( DoubleMap::value_type( "max_dash_angle", &max_dash_angle_ ) );
    double_map_.insert( DoubleMap::value_type( "min_dash_angle", &min_dash_angle_ ) );
    double_map_.insert( DoubleMap::value_type( "dash_angle_step", &dash_angle_step_ ) );
    double_map_.insert( DoubleMap::value_type( "side_dash_rate", &side_dash_rate_ ) );
    double_map_.insert( DoubleMap::value_type( "back_dash_rate", &back_dash_rate_ ) );
    double_map_.insert( DoubleMap::value_type( "max_dash_power", &max_dash_power_ ) );
    double_map_.insert( DoubleMap::value_type( "min_dash_power", &min_dash_power_ ) );
    // 14.0.0
    double_map_.insert( DoubleMap::value_type( "tackle_rand_factor", &tackle_rand_factor_ ) );
    double_map_.insert( DoubleMap::value_type( "foul_detect_probability", &foul_detect_probability_ ) );
    double_map_.insert( DoubleMap::value_type( "foul_exponent", &foul_exponent_ ) );
    int_map_.insert( IntMap::value_type( "foul_cycles", &foul_cycles_ ) );
    bool_map_.insert( BoolMap::value_type( "golden_goal", &golden_goal_ ) );
    // 15.0
    double_map_.insert( DoubleMap::value_type( "red_card_probability", &red_card_probability_ ) );
    // 16.0
    int_map_.insert( IntMap::value_type( "illegal_defense_duration", &illegal_defense_duration_ ) );
    int_map_.insert( IntMap::value_type( "illegal_defense_number", &illegal_defense_number_ ) );
    double_map_.insert( DoubleMap::value_type( "illegal_defense_dist_x", &illegal_defense_dist_x_ ) );
    double_map_.insert( DoubleMap::value_type( "illegal_defense_width", &illegal_defense_width_ ) );
    string_map_.insert( StringMap::value_type( "fixed_teamname_l", &fixed_teamname_l_ ) );
    string_map_.insert( StringMap::value_type( "fixed_teamname_r", &fixed_teamname_r_ ) );
    // 17.0
    double_map_.insert( DoubleMap::value_type( "max_catch_angle", &max_catch_angle_ ) );
    double_map_.insert( DoubleMap::value_type( "min_catch_angle", &min_catch_angle_ ) );

    //
}


std::ostream &
ServerParamT::toSExp( std::ostream & os ) const
{
    os << "(server_param ";
    to_sexp( os, "goal_width", quantize( goal_width_, 0.00001 ) );
    to_sexp( os, "inertia_moment", quantize( inertia_moment_, 0.00001 ) );
    to_sexp( os, "player_size", quantize( player_size_, 0.00001 ) );
    to_sexp( os, "player_decay", quantize( player_decay_, 0.00001 ) );
    to_sexp( os, "player_rand", quantize( player_rand_, 0.00001 ) );
    to_sexp( os, "player_weight", quantize( player_weight_, 0.00001 ) );
    to_sexp( os, "player_speed_max", quantize( player_speed_max_, 0.00001 ) );
    to_sexp( os, "player_accel_max", quantize( player_accel_max_ , 0.00001 ) );
    to_sexp( os, "stamina_max", quantize( stamina_max_, 0.00001 ) );
    to_sexp( os, "stamina_inc_max", quantize( stamina_inc_max_, 0.00001 ) );
    to_sexp( os, "recover_init", quantize( recover_init_, 0.00001 ) );
    to_sexp( os, "recover_dec_thr", quantize( recover_dec_thr_, 0.00001 ) );
    to_sexp( os, "recover_min", quantize( recover_min_, 0.00001 ) );
    to_sexp( os, "recover_dec", quantize( recover_dec_, 0.00001 ) );
    to_sexp( os, "effort_init", quantize( effort_init_, 0.00001 ) );
    to_sexp( os, "effort_dec_thr", quantize( effort_dec_thr_, 0.00001 ) );
    to_sexp( os, "effort_min", quantize( effort_min_, 0.00001 ) );
    to_sexp( os, "effort_dec", quantize( effort_dec_, 0.00001 ) );
    to_sexp( os, "effort_inc_thr", quantize( effort_inc_thr_, 0.00001 ) );
    to_sexp( os, "effort_inc", quantize( effort_inc_, 0.00001 ) );
    to_sexp( os, "kick_rand", quantize( kick_rand_, 0.00001 ) );
    to_sexp( os, "team_actuator_noise", team_actuator_noise_ );
    to_sexp( os, "prand_factor_l", quantize( player_rand_factor_l_, 0.00001 ) );
    to_sexp( os, "prand_factor_r", quantize( player_rand_factor_r_, 0.00001 ) );
    to_sexp( os, "kick_rand_factor_l", quantize( kick_rand_factor_l_, 0.00001 ) );
    to_sexp( os, "kick_rand_factor_r", quantize( kick_rand_factor_r_, 0.00001 ) );
    to_sexp( os, "ball_size", quantize( ball_size_, 0.00001 ) );
    to_sexp( os, "ball_decay", quantize( ball_decay_, 0.00001 ) );
    to_sexp( os, "ball_rand", quantize( ball_rand_, 0.00001 ) );
    to_sexp( os, "ball_weight", quantize( ball_weight_, 0.00001 ) );
    to_sexp( os, "ball_speed_max", quantize( ball_speed_max_, 0.00001 ) );
    to_sexp( os, "ball_accel_max", quantize( ball_accel_max_, 0.00001 ) );
    to_sexp( os, "dash_power_rate", quantize( dash_power_rate_, 0.00001 ) );
    to_sexp( os, "kick_power_rate", quantize( kick_power_rate_, 0.00001 ) );
    to_sexp( os, "kickable_margin", quantize( kickable_margin_, 0.00001 ) );
    to_sexp( os, "control_radius", quantize( control_radius_, 0.00001 ) );
    //( "control_radius_width", control_radius_width_ );
    //( "kickable_area", kickable_area_ );
    to_sexp( os, "catch_probability", quantize( catch_probability_, 0.00001 ) );
    to_sexp( os, "catchable_area_l", quantize( catchable_area_l_, 0.00001 ) );
    to_sexp( os, "catchable_area_w", quantize( catchable_area_w_, 0.00001 ) );
    to_sexp( os, "goalie_max_moves", goalie_max_moves_ );
    to_sexp( os, "maxpower", quantize( max_power_, 0.00001 ) );
    to_sexp( os, "minpower", quantize( min_power_, 0.00001 ) );
    to_sexp( os, "maxmoment", quantize( max_moment_, 0.00001 ) );
    to_sexp( os, "minmoment", quantize( min_moment_, 0.00001 ) );
    to_sexp( os, "maxneckmoment", quantize( max_neck_moment_, 0.00001 ) );
    to_sexp( os, "minneckmoment", quantize( min_neck_moment_, 0.00001 ) );
    to_sexp( os, "maxneckang", quantize( max_neck_angle_, 0.00001 ) );
    to_sexp( os, "minneckang", quantize( min_neck_angle_, 0.00001 ) );
    to_sexp( os, "visible_angle", quantize( visible_angle_, 0.00001 ) );
    to_sexp( os, "visible_distance", quantize( visible_distance_, 0.00001 ) );
    to_sexp( os, "audio_cut_dist", quantize( audio_cut_dist_, 0.00001 ) );
    to_sexp( os, "quantize_step", quantize( dist_quantize_step_, 0.00001 ) );
    to_sexp( os, "quantize_step_l", quantize( landmark_dist_quantize_step_, 0.00001 ) );
    //( "quantize_step_dir", dir_quantize_step_ );
    //( "quantize_step_dist_team_l", dist_quantize_step_l_ );
    //( "quantize_step_dist_team_r", dist_quantize_step_r_ );
    //( "quantize_step_dist_l_team_l", landmark_dist_quantize_step_l_ );
    //( "quantize_step_dist_l_team_r", landmark_dist_quantize_step_r_ );
    //( "quantize_step_dir_team_l", dir_quantize_step_l_ );
    //( "quantize_step_dir_team_r", dir_quantize_step_r_ );
    to_sexp( os, "ckick_margin", quantize( corner_kick_margin_, 0.00001 ) );
    to_sexp( os, "wind_dir", quantize( wind_dir_, 0.00001 ) );
    to_sexp( os, "wind_force", quantize( wind_force_, 0.00001 ) );
    to_sexp( os, "wind_ang", quantize( wind_angle_, 0.00001 ) );
    to_sexp( os, "wind_rand", quantize( wind_rand_, 0.00001 ) );
    to_sexp( os, "wind_none", wind_none_ );
    to_sexp( os, "wind_random", use_wind_random_ );
    to_sexp( os, "half_time", half_time_ );
    to_sexp( os, "drop_ball_time", drop_ball_time_ );
    to_sexp( os, "port", port_ );
    to_sexp( os, "coach_port", coach_port_ );
    to_sexp( os, "olcoach_port", online_coach_port_ );
    to_sexp( os, "say_coach_cnt_max", say_coach_count_max_ );
    to_sexp( os, "say_coach_msg_size", say_coach_msg_size_ );
    to_sexp( os, "simulator_step", simulator_step_ );
    to_sexp( os, "send_step", send_step_ );
    to_sexp( os, "recv_step", recv_step_ );
    to_sexp( os, "sense_body_step", sense_body_step_ );
    //( "lcm_step", lcm_step_ ); // not needed
    to_sexp( os, "say_msg_size", say_msg_size_ );
    to_sexp( os, "clang_win_size", clang_win_size_ );
    to_sexp( os, "clang_define_win", clang_define_win_ );
    to_sexp( os, "clang_meta_win", clang_meta_win_ );
    to_sexp( os, "clang_advice_win", clang_advice_win_ );
    to_sexp( os, "clang_info_win", clang_info_win_ );
    to_sexp( os, "clang_del_win", clang_del_win_ );
    to_sexp( os, "clang_rule_win", clang_rule_win_ );
    to_sexp( os, "clang_mess_delay", clang_mess_delay_ );
    to_sexp( os, "clang_mess_per_cycle", clang_mess_per_cycle_ );
    to_sexp( os, "hear_max", hear_max_ );
    to_sexp( os, "hear_inc", hear_inc_ );
    to_sexp( os, "hear_decay", hear_decay_ );
    to_sexp( os, "catch_ban_cycle", catch_ban_cycle_ );
    to_sexp( os, "coach", coach_mode_ );
    to_sexp( os, "coach_w_referee", coach_with_referee_mode_ );
    to_sexp( os, "old_coach_hear", use_old_coach_hear_ );
    to_sexp( os, "send_vi_step", online_coach_look_step_ );
    to_sexp( os, "use_offside", use_offside_ );
    to_sexp( os, "offside_kick_margin", quantize( offside_kick_margin_, 0.00001 ) );
    to_sexp( os, "forbid_kick_off_offside", kick_off_offside_ );
    to_sexp( os, "verbose", verbose_ );
    to_sexp( os, "offside_active_area_size", quantize( offside_active_area_size_, 0.00001 ) );
    to_sexp( os, "slow_down_factor", slow_down_factor_ );
    to_sexp( os, "synch_mode", synch_mode_ );
    to_sexp( os, "synch_offset", synch_offset_ );
    to_sexp( os, "synch_micro_sleep", synch_micro_sleep_ );
    to_sexp( os, "start_goal_l", start_goal_l_ );
    to_sexp( os, "start_goal_r", start_goal_r_ );
    to_sexp( os, "fullstate_l", fullstate_l_ );
    to_sexp( os, "fullstate_r", fullstate_r_ );
    to_sexp( os, "slowness_on_top_for_left_team", quantize( slowness_on_top_for_left_team_, 0.00001 ) );
    to_sexp( os, "slowness_on_top_for_right_team", quantize( slowness_on_top_for_right_team_, 0.00001 ) );
    to_sexp( os, "landmark_file", landmark_file_ );
    to_sexp( os, "send_comms", send_comms_ );
    to_sexp( os, "text_logging", text_logging_ );
    to_sexp( os, "game_logging", game_logging_ );
    to_sexp( os, "game_log_version", game_log_version_ );
    to_sexp( os, "text_log_dir", text_log_dir_ );
    to_sexp( os, "game_log_dir", game_log_dir_ );
    to_sexp( os, "text_log_fixed_name", text_log_fixed_name_ );
    to_sexp( os, "game_log_fixed_name", game_log_fixed_name_ );
    to_sexp( os, "text_log_fixed", text_log_fixed_ );
    to_sexp( os, "game_log_fixed", game_log_fixed_ );
    to_sexp( os, "text_log_dated", text_log_dated_ );
    to_sexp( os, "game_log_dated", game_log_dated_ );
    to_sexp( os, "log_date_format", log_date_format_ );
    to_sexp( os, "log_times", log_times_ );
    to_sexp( os, "record_messages", record_messages_ );
    to_sexp( os, "text_log_compression", text_log_compression_ );
    to_sexp( os, "game_log_compression", game_log_compression_ );
    to_sexp( os, "profile", profile_ );
    to_sexp( os, "point_to_ban", point_to_ban_ );
    to_sexp( os, "point_to_duration", point_to_duration_ );
    to_sexp( os, "tackle_dist", quantize( tackle_dist_, 0.00001 ) );
    to_sexp( os, "tackle_back_dist", quantize( tackle_back_dist_, 0.00001 ) );
    to_sexp( os, "tackle_width", quantize( tackle_width_, 0.00001 ) );
    to_sexp( os, "tackle_exponent", quantize( tackle_exponent_, 0.00001 ) );
    to_sexp( os, "tackle_cycles", tackle_cycles_ );
    to_sexp( os, "tackle_power_rate", quantize( tackle_power_rate_, 0.00001 ) );
    to_sexp( os, "freeform_wait_period", freeform_wait_period_ );
    to_sexp( os, "freeform_send_period", freeform_send_period_ );
    to_sexp( os, "free_kick_faults", free_kick_faults_ );
    to_sexp( os, "back_passes", back_passes_ );
    to_sexp( os, "proper_goal_kicks", proper_goal_kicks_ );
    to_sexp( os, "stopped_ball_vel", quantize( stopped_ball_vel_, 0.00001 ) );
    to_sexp( os, "max_goal_kicks", max_goal_kicks_ );
    to_sexp( os, "auto_mode", auto_mode_ );
    to_sexp( os, "kick_off_wait", kick_off_wait_ );
    to_sexp( os, "connect_wait", connect_wait_ );
    to_sexp( os, "game_over_wait", game_over_wait_ );
    to_sexp( os, "team_l_start", team_l_start_ );
    to_sexp( os, "team_r_start", team_r_start_ );
    to_sexp( os, "keepaway", keepaway_mode_ );
    to_sexp( os, "keepaway_length", quantize( keepaway_length_, 0.00001 ) );
    to_sexp( os, "keepaway_width", quantize( keepaway_width_, 0.00001 ) );
    to_sexp( os, "keepaway_logging", keepaway_logging_ );
    to_sexp( os, "keepaway_log_dir", keepaway_log_dir_ );
    to_sexp( os, "keepaway_log_fixed_name", keepaway_log_fixed_name_ );
    to_sexp( os, "keepaway_log_fixed", keepaway_log_fixed_ );
    to_sexp( os, "keepaway_log_dated", keepaway_log_dated_ );
    to_sexp( os, "keepaway_start", keepaway_start_ );
    to_sexp( os, "nr_normal_halfs", nr_normal_halfs_ );
    to_sexp( os, "nr_extra_halfs", nr_extra_halfs_ );
    to_sexp( os, "penalty_shoot_outs", penalty_shoot_outs_ );
    to_sexp( os, "pen_before_setup_wait", pen_before_setup_wait_ );
    to_sexp( os, "pen_setup_wait", pen_setup_wait_ );
    to_sexp( os, "pen_ready_wait", pen_ready_wait_ );
    to_sexp( os, "pen_taken_wait", pen_taken_wait_ );
    to_sexp( os, "pen_nr_kicks", pen_nr_kicks_ );
    to_sexp( os, "pen_max_extra_kicks", pen_max_extra_kicks_ );
    to_sexp( os, "pen_dist_x", quantize( pen_dist_x_, 0.00001 ) );
    to_sexp( os, "pen_random_winner", pen_random_winner_ );
    to_sexp( os, "pen_max_goalie_dist_x", quantize( pen_max_goalie_dist_x_, 0.00001 ) );
    to_sexp( os, "pen_allow_mult_kicks", pen_allow_mult_kicks_ );
    to_sexp( os, "pen_coach_moves_players", pen_coach_moves_players_ );
    // 11
    to_sexp( os, "ball_stuck_area", quantize( ball_stuck_area_, 0.00001 ) );
    to_sexp( os, "coach_msg_file", coach_msg_file_ );
    // 12
    to_sexp( os, "max_tackle_power", quantize( max_tackle_power_, 0.00001 ) );
    to_sexp( os, "max_back_tackle_power", quantize( max_back_tackle_power_, 0.00001 ) );
    to_sexp( os, "player_speed_max_min", quantize( player_speed_max_min_, 0.00001 ) );
    to_sexp( os, "extra_stamina", quantize( extra_stamina_, 0.00001 ) );
    to_sexp( os, "synch_see_offset", synch_see_offset_ );
    to_sexp( os, "max_monitors", max_monitors_ );
    // 12.1.3
    to_sexp( os, "extra_half_time", extra_half_time_ );
    // v13
    to_sexp( os, "stamina_capacity", quantize( stamina_capacity_, 0.00001 ) );
    to_sexp( os, "max_dash_angle", quantize( max_dash_angle_, 0.00001 ) );
    to_sexp( os, "min_dash_angle", quantize( min_dash_angle_, 0.00001 ) );
    to_sexp( os, "dash_angle_step", quantize( dash_angle_step_, 0.00001 ) );
    to_sexp( os, "side_dash_rate", quantize( side_dash_rate_, 0.00001 ) );
    to_sexp( os, "back_dash_rate", quantize( back_dash_rate_, 0.00001 ) );
    to_sexp( os, "max_dash_power", quantize( max_dash_power_, 0.00001 ) );
    to_sexp( os, "min_dash_power", quantize( min_dash_power_, 0.00001 ) );
    // 14.0
    to_sexp( os, "tackle_rand_factor", quantize( tackle_rand_factor_, 0.00001 ) );
    to_sexp( os, "foul_detect_probability", quantize( foul_detect_probability_, 0.00001 ) );
    to_sexp( os, "foul_exponent", quantize( foul_exponent_, 0.00001 ) );
    to_sexp( os, "foul_cycles", foul_cycles_ );
    to_sexp( os, "golden_goal", golden_goal_ );
    // 15.0
    to_sexp( os, "red_card_probability", quantize( red_card_probability_, 0.00001 ) );
    // 16.0
    to_sexp( os, "illegal_defense_duration", illegal_defense_duration_ );
    to_sexp( os, "illegal_defense_number", illegal_defense_number_ );
    to_sexp( os, "illegal_defense_dist_x", illegal_defense_dist_x_ );
    to_sexp( os, "illegal_defense_width", illegal_defense_width_ );
    to_sexp( os, "fixed_teamname_l", fixed_teamname_l_ );
    to_sexp( os, "fixed_teamname_r", fixed_teamname_r_ );
    // 17.0
    to_sexp( os, "max_catch_angle", quantize( max_catch_angle_, 0.00001 ) );
    to_sexp( os, "min_catch_angle", quantize( min_catch_angle_, 0.00001 ) );

    os << ')';

    return os;
}

bool
ServerParamT::createFromSExp( const std::string & msg )
{
    //if ( ! parse_param( msg, int_map_, double_map_, bool_map_, string_map_ ) )
    if ( ! parse_param( msg, *this ) )
    {
        // nothing to do
    }

    return true;
}

bool
ServerParamT::setValue( const std::string & name,
                        const std::string & value )
{
    try
    {
        IntMap::iterator int_it = int_map_.find( name );
        if ( int_it != int_map_.end() )
        {
            *(int_it->second) = std::stoi( value );
            return true;
        }

        DoubleMap::iterator double_it = double_map_.find( name );
        if ( double_it != double_map_.end() )
        {
            *(double_it->second) = std::stod( value );
            return true;
        }

        BoolMap::iterator bool_it = bool_map_.find( name );
        if ( bool_it != bool_map_.end() )
        {
            if ( value == "0" || value == "false" || value == "off" )
            {
                *(bool_it->second) = false;
            }
            else if ( value == "1" || value == "true" || value == "on" )
            {
                *(bool_it->second) = true;
            }
            else
            {
                std::cerr << "Unknown bool value. name=" << name << " value=" << value << std::endl;
            }
            return true;
        }

        StringMap::iterator string_it = string_map_.find( name );
        if ( string_it != string_map_.end() )
        {
            *(string_it->second) = clean_string( value );
            return true;
        }
    }
    catch ( std::exception & e )
    {
        std::cerr << e.what() << '\n'
                  << "name=" << name << " value=" << value << std::endl;
        return true;
    }

    std::cerr << "Unsupported parameter. name=" << name << " value=" << value << std::endl;
    return false;
}


bool
ServerParamT::setInt( const std::string & name,
                      const int value )
{
    IntMap::iterator int_it = int_map_.find( name );
    if ( int_it != int_map_.end() )
    {
        *(int_it->second) = value;
        return false;
    }

    DoubleMap::iterator double_it = double_map_.find( name );
    if ( double_it != double_map_.end() )
    {
        *(double_it->second) = value;
        return true;
    }

    BoolMap::iterator bool_it = bool_map_.find( name );
    if ( bool_it != bool_map_.end() )
    {
        if ( value == 0 )
        {
            *(bool_it->second) = false;
            return true;
        }

        if ( value == 1 )
        {
            *(bool_it->second) = true;
            return true;
        }
    }

    std::cerr << "Unsupported parameter. name=" << name << " value=" << value << std::endl;
    return true;
}


bool
ServerParamT::setDouble( const std::string & name,
                         const double value )
{
    DoubleMap::iterator it = double_map_.find( name );
    if ( it == double_map_.end() )
    {
        std::cerr << "Unsupported double parameter. name=" << name << " value=" << value << std::endl;
        return false;
    }

    *(it->second) = value;
    return true;
}


bool
ServerParamT::setBool( const std::string & name,
                       const bool value )
{
    BoolMap::iterator it = bool_map_.find( name );
    if ( it == bool_map_.end() )
    {
        std::cerr << "Unsupported bool parameter. name=" << name << " value=" << value << std::endl;
        return false;
    }

    *(it->second) = value;
    return true;
}


bool
ServerParamT::setString( const std::string & name,
                         const std::string & value )
{
    StringMap::iterator it = string_map_.find( name );
    if ( it == string_map_.end() )
    {
        std::cerr << "Unsupported string parameter. name=" << name << " value=" << value << std::endl;
        return false;
    }

    *(it->second) = value;
    return true;
}

}
}
