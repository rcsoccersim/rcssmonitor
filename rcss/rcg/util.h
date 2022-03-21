// -*-c++-*-

/*!
  \file util.h
  \brief utilities for rcg data Header File.
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

#ifndef RCSS_RCG_UTIL_H
#define RCSS_RCG_UTIL_H

#include <rcss/rcg/types.h>

namespace rcss {
namespace rcg {

/*-------------------------------------------------------------------*/
/*!
  \brief convert network byte order Int15 value to integer
  \param val network byte order variable
  \return local byte order integer
*/
int
nstohi( const Int16 val );

/*-------------------------------------------------------------------*/
/*!
  \brief convert local integer value to network byte order Int16 value
  \param val local integer value
  \return network byte order short value.
*/
Int16
hitons( const int val );

/*-------------------------------------------------------------------*/
/*!
  \brief convert network byte order Int16 value to bool
  \param val network byte order variable
  \return boolean value
*/
bool
nstohb( const Int16 val );

/*-------------------------------------------------------------------*/
/*!
  \brief convert local boolean value -> network byte order short value
  \param val local boolean value
  \return network byte order short value.
*/
Int16
hbtons( const bool val );

/*-------------------------------------------------------------------*/
/*!
  \brief convert network byte order short value to floating point number
  \param val network byte order variable
  \return floating point number filtered by SHOWINFO_SCALE.
*/
double
nstohd( const Int16 val );

/*-------------------------------------------------------------------*/
/*!
  \brief convert network byte order short value to floating point number
  \param val network byte order variable
  \return floating point number filtered by SHOWINFO_SCALE.
*/
float
nstohf( const Int16 val );

/*-------------------------------------------------------------------*/
/*!
  \brief convert local floating point value to thnetwork byte order short value
  \param val local floating point value
  \return network byte order short value
*/
Int16
hdtons( const double & val );

/*-------------------------------------------------------------------*/
/*!
  \brief convert local floating point value to thnetwork byte order short value
  \param val local floating point value
  \return network byte order short value
*/
Int16
hftons( const float & val );

/*-------------------------------------------------------------------*/
/*!
  \brief convert network byte order long value to floating point number
  for rcsmonitor v2 protocol
  \param val network byte order variable
  \return floating point number filtered by SHOWINFO_SCALE2.
*/
double
nltohd( const Int32 & val );

/*-------------------------------------------------------------------*/
/*!
  \brief convert network byte order Int32 value to floating point number
  for rcsmonitor v2 protocol
  \param val network byte order variable
  \return floating point number filtered by SHOWINFO_SCALE2.
*/
float
nltohf( const Int32 & val );

/*-------------------------------------------------------------------*/
/*!
  \brief convert a floating point number to the network byte order Int32 value
  for rcsmonitor v2 protocol
  \param val local floating point number value
  \return network byte order value.
*/
Int32
hdtonl( const double & val );

/*-------------------------------------------------------------------*/
/*!
  \brief convert a floating point number to the network byte order Int32 value
  for rcsmonitor v2 protocol
  \param val local floating point number value
  \return network byte order value.
*/
Int32
hftonl( const float & val );

/*-------------------------------------------------------------------*/
/*!
  \brief convert network byle order short value to network byte order long value
  v1 protocol -> v2 protocol.
  \param val network byte order variable
  \return network byte order long value
*/
Int32
nstonl( const Int16 val );

/*-------------------------------------------------------------------*/
/*!
  \brief convert network byte order long value to network byte order short value
  convert v2 protocol -> v1 protocol
  \param val network byte order variable
  \return network byte order short value
*/
Int16
nltons( const Int32 & val );


/*-------------------------------------------------------------------*/
/*!
  \brief convert pos_t to BallT
  \param from source variable
  \param to destination variable
*/
void
convert( const pos_t & from,
         BallT & to );


/*-------------------------------------------------------------------*/
/*!
  \brief convert BallT to pos_t
  \param from source variable
  \param to destination variable
*/
void
convert( const BallT & from,
         pos_t & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert ball_t to BallT
  \param from source variable
  \param to destination variable
*/
void
convert( const ball_t & from,
         BallT & to );
/*-------------------------------------------------------------------*/
/*!
  \brief convert BallT to ball_t
  \param from source variable
  \param to destination variable
*/
void
convert( const BallT & from,
         ball_t & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert pos_t to player_t
  \param from source variable
  \param to destination variable
*/
void
convert( const pos_t & from,
         player_t & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert player_t to pos_t
  \param side player's side id
  \param unum uniform number
  \param from source variable
  \param to destination variable
*/
void
convert( const Side side,
         const int unum,
         const player_t & from,
         pos_t & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert pos_t to PlayerT
  \param from source variable
  \param to destination variable
*/
void
convert( const pos_t & from,
         PlayerT & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert player info to player_t
  \param from source player info
  \param to destination player_t variable
*/
void
convert( const PlayerT & from,
         player_t & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert player_t to PlayerT
  \param from source variable
  \param to destination variable
*/
void
convert( const Side side,
         const int unum,
         const player_t & from,
         PlayerT & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert team info to team_t
  \param name source team name string
  \param score source team score
  \param to destination team_t variable
*/
void
convert( const std::string & name,
         const int score,
         team_t & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert team_t to TeamT
  \param from source data
  \param to destination team_t variable
*/
void
convert( const TeamT & from,
         team_t & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert TeamT to team_t
  \param from source data
  \param to destination TeamT variable
*/
void
convert( const team_t & from,
         TeamT & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert showinfo_t to showinfo_t2
  \param from source showinfo_t variable
  \param to destination showinfo_t2 variable
*/
void
convert( const showinfo_t & from,
         showinfo_t2 & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert showinfo_t to short_showinfo_t2
  \param from source showinfo_t variable
  \param to destination short_showinfo_t2 variable
*/
void
convert( const showinfo_t & from,
         short_showinfo_t2 & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert showinfo_t2 to showinfo_t
  \param from source showinfo_t2 variable
  \param to destination showinfo_t variable
*/
void
convert( const showinfo_t2 & from,
         showinfo_t & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert short_showinfo_t2 to showinfo_t
  \param playmode playmode variable
  \param team_l left team variable
  \param team_r right team variable
  \param from source short_showinfo_t2 variable
  \param to destination showinfo_t variable
*/
void
convert( const char playmode,
         const TeamT & team_l,
         const TeamT & team_r,
         const short_showinfo_t2 & from,
         showinfo_t & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert ShowInfoT to showinfo_t
  \param playmode playmode variable
  \param team_l left team variable
  \param team_r right team variable
  \param from source ShowInfoT variable
  \param to destination showinfo_t variable
*/
void
convert( const char playmode,
         const TeamT & team_l,
         const TeamT & team_r,
         const ShowInfoT & from,
         showinfo_t & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert showinfot_t to ShowInfoT
  \param from source variable
  \param to destination variable
*/
void
convert( const showinfo_t & from,
         ShowInfoT & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert ShowInfoT to showinfo_t2
  \param playmode playmode variable
  \param team_l left team variable
  \param team_r right team variable
  \param from source ShowInfoT variable
  \param to destination showinfo_t2 variable
*/
void
convert( const char playmode,
         const TeamT & team_l,
         const TeamT & team_r,
         const ShowInfoT & from,
         showinfo_t2 & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert showinfot_t2 to ShowInfoT
  \param from source variable
  \param to destination variable
*/
void
convert( const showinfo_t2 & from,
         ShowInfoT & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert ShowInfoT to short_showinfo_t2
  \param from source ShowInfoT variable
  \param to destination short_showinfo_t2 variable
*/
void
convert( const ShowInfoT & from,
         short_showinfo_t2 & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert short_showinfot_t2 to ShowInfoT
  \param from source variable
  \param to destination variable
*/
void
convert( const short_showinfo_t2 & from,
         ShowInfoT & to );


/*-------------------------------------------------------------------*/
/*!
  \brief convert player_type_t to PlayerTypeT
  \param from source variable
  \param to destination variable
*/
void
convert( const player_type_t & from,
         PlayerTypeT & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert PlayerTypeT to player_type_t
  \param from source variable
  \param to destination variable
*/
void
convert( const PlayerTypeT & from,
         player_type_t & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert player_param_t to PlayerParamT
  \param from source variable
  \param to destination variable
*/
void
convert( const player_params_t & from,
         PlayerParamT & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert PlayerParamT to player_param_t
  \param from source variable
  \param to destination variable
*/
void
convert( const PlayerParamT & from,
         player_params_t & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert server_param_t to ServerParamT
  \param from source variable
  \param to destination variable
*/
void
convert( const server_params_t & from,
         ServerParamT & to );

/*-------------------------------------------------------------------*/
/*!
  \brief convert ServerParamT to server_param_t
  \param from source variable
  \param to destination variable
*/
void
convert( const ServerParamT & from,
         server_params_t & to );

/*-------------------------------------------------------------------*/
/*!
  \brief make msginfo_t from string
  \param from source message string
  \param to destination msginfo_t variable
*/
void
convert( const std::string & from,
         msginfo_t & to );


/*-------------------------------------------------------------------*/
/*!
  \brief build server message from old format data
  \param from source data
  \return result string
*/
std::string
to_sexp( const player_type_t & from );

/*-------------------------------------------------------------------*/
/*!
  \brief build server message from old format data
  \param from source data
  \return result string
*/
std::string
to_sexp( const server_params_t & from );

/*-------------------------------------------------------------------*/
/*!
  \brief build server message from old format data
  \param from source data
  \return result string
*/
std::string
to_sexp( const player_params_t & from );

} // end namespace
} // end namespace

#endif
