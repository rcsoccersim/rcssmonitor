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
#include <vector>

/*!
  \struct FigureLook
  \brief how a player looks as a human figure: its body (of the figure
  pack's), the hairstyles it wears, the colors of its skin and its hair
  (FigureLooks). A body without skin or hair to paint (the Mannequin) does
  not use the colors.
*/
struct FigureLook {
    int body = 0; //!< of the figure pack's bodies (FigureCast::bodies)
    std::uint32_t hair = 1; //!< the hairstyles worn (FigureCast::Body bits): the body's first by default
    QVector4D skin = QVector4D( 0.84f, 0.64f, 0.50f, 1.0f ); //!< the standard skin by default
    QVector4D hair_color = QVector4D( 0.10f, 0.06f, 0.035f, 1.0f ); //!< the standard hair by default
};

//! whose a body of a figure pack is: a man's, a woman's, or anyone's
enum class FigureSex {
    Any,
    Male,
    Female,
};

/*!
  \struct FigureCast
  \brief the bodies of a figure pack, as the players' looks are drawn from
  them (FigurePack::cast()): whose each is, and the hairstyles and the
  extras it can wear, as bits (a hairstyle or an extra is a material of the
  body's model, drawn on the figures wearing its bit: Figures3D::paints())
*/
struct FigureCast {
    struct Extra {
        std::uint32_t bit = 0;
        double chance = 0.0; //!< worn by this chance, over a hairstyle
    };
    struct Body {
        FigureSex sex = FigureSex::Any;
        std::vector< std::uint32_t > hairstyles; //!< one of them worn, each as often
        std::uint32_t standard_hair = 0; //!< worn by a player not known for sure
        std::vector< Extra > extras; //!< each worn by its chance (a beard)
    };
    std::vector< Body > bodies;
};

/*!
  \brief each player's looks as a human figure (docs/plan/2026-11-11_3d-view.md §5.8,
  7-5, §5.9): its body of the figure pack's (by the option, men and women
  mixed or one of them, of a pack with both), its hairstyle (and the
  extras, a beard), the colors of its skin and its hair. They are drawn by
  lot, but the same for the same player of the same team (a hash of the
  team's name, the side and the number): a frame looks the same however
  it is reached, and a team looks the same in every game.

  The player camera's player knows another player's looks only as far as
  it knows who it is (the opacity of its number, PlayerSight): the colors
  come toward the standard ones by it, and the body and the hairstyle are
  the standard ones unless it is sure (blend()), as the height.
*/
namespace FigureLooks {

/*!
  \brief the standard looks (of a player not known for sure): the first
  man's body of the pack, with its standard hairstyle (the first woman's
  with women only), and the standard colors
*/
FigureLook standard( const FigureBodies bodies,
                     const FigureCast & cast );

/*!
  \brief a player's own looks
  \param side its side
  \param unum its number
  \param team its team's name (may be empty)
  \param bodies the option: men and women mixed, or one of them
  \param cast the bodies of the figure pack
*/
FigureLook of( const rcss::rcg::Side side,
               const int unum,
               const std::string & team,
               const FigureBodies bodies,
               const FigureCast & cast );

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
