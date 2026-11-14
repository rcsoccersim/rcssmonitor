// -*-c++-*-

/*!
  \file figure_looks.h
  \brief each player's looks as a human body. Header File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef RCSSMONITOR_FIGURE_LOOKS_H
#define RCSSMONITOR_FIGURE_LOOKS_H

#include "camera_mode.h"

#include <rcss/rcg/types.h>

#include <QString>
#include <QVector4D>

#include <cstdint>
#include <string>

/*!
  \struct FigureLook
  \brief how a player looks as a human body (PlayerModel::Human): its body,
  the hairstyles it wears, the colors of its skin and its hair
  (FigureLooks). The mannequin does not use it.
*/
struct FigureLook {
    int body = 0; //!< FigureLooks::MALE or FEMALE
    std::uint32_t hair = 1; //!< the hairstyles worn (FigureLooks::Hair bits): the standard one by default
    QVector4D skin = QVector4D( 0.84f, 0.64f, 0.50f, 1.0f ); //!< the standard skin by default
    QVector4D hair_color = QVector4D( 0.10f, 0.06f, 0.035f, 1.0f ); //!< the standard hair by default
};

/*!
  \brief each player's looks as a human body (docs/PLAN_3D-VIEW.md §5.8,
  7-5): its body (by the option, men and women mixed or one of them), its
  hairstyle (and a beard), the colors of its skin and its hair. They are
  drawn by lot, but the same for the same player of the same team (a hash
  of the team's name, the side and the number): a frame looks the same
  however it is reached, and a team looks the same in every game.

  The player camera's player knows another player's looks only as far as
  it knows who it is (the opacity of its number, PlayerSight): the colors
  come toward the standard ones by it, and the body and the hairstyle are
  the standard ones unless it is sure (blend()), as the height.
*/
namespace FigureLooks {

//! the bodies (the human bodies bundled, in this order)
enum Body {
    MALE = 0,
    FEMALE = 1,
};

/*!
  \enum Hair
  \brief the hairstyles, as bits: each the material "Hair_<style>" of a body
  (resources/models/pack_base_character.py)
*/
enum Hair : std::uint32_t {
    SIMPLE_PARTED = 1u << 0, //!< a man's
    BUZZED = 1u << 1, //!< a man's
    BEARD = 1u << 2, //!< a man's, with one of his hairstyles
    BUNS = 1u << 3, //!< a woman's
    LONG = 1u << 4, //!< a woman's
    BUZZED_FEMALE = 1u << 5, //!< a woman's
};

//! a man wears a beard by this chance
constexpr double BEARD_CHANCE = 0.3;

//! the bit of a hairstyle's material ("Hair_SimpleParted", ...), 0 if not a known one
std::uint32_t hairBit( const QString & material );

//! the standard looks (of a player not known for sure): a man's but with women only
FigureLook standard( const FigureBodies bodies );

/*!
  \brief a player's own looks
  \param side its side
  \param unum its number
  \param team its team's name (may be empty)
  \param bodies the option: men and women mixed, or one of them
*/
FigureLook of( const rcss::rcg::Side side,
               const int unum,
               const std::string & team,
               const FigureBodies bodies );

/*!
  \brief the looks as known for sure as certainty (0 to 1): the colors
  between the standard ones and the own ones, the body and the hairstyle
  the own ones only at 1
*/
FigureLook blend( const FigureLook & own,
                  const FigureLook & standard,
                  const float certainty );

}

#endif
