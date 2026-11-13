// -*-c++-*-

/*!
  \file stadium_3d.h
  \brief the surroundings of the pitch in the team colors. Header File.
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


#ifndef RCSSMONITOR_STADIUM_3D_H
#define RCSSMONITOR_STADIUM_3D_H

#include "scene_content.h"

#include <QColor>
#include <QImage>
#include <QString>

/*!
  \brief the surroundings of the pitch in the team colors (rcssmonitor-web
  stadium.ts, docs/PLAN_3D-VIEW.md §4.3), so that the two ends can be told
  apart, above all from the player camera. Each can be hidden (the options
  boards_3d and banners_3d):
  - the boards around the pitch; along the touch lines they change color at
    the halfway line;
  - a banner floating high behind each goal, with the team name and logo.
  Both are a dark shade of the team color with the name, an edge and the
  logo in the team color, so that the players, in the team colors, stand
  out against them. rcssserver does not change ends, so the left team
  keeps the left side for the whole game. The goal nets are tinted by
  SceneBuilder::addGoals(). Here, the images (QPainter) and where the
  panels go: no OpenGL, so that the tests check them.
*/
namespace Stadium3D {

constexpr float BOARD_OFFSET = 5.5f; //!< from the touch lines and the goal lines (m): beyond the margin of the ground image
constexpr float BOARD_HEIGHT = 1.0f;
constexpr float BOARD_TILE = 10.0f; //!< the team name is written once in every this many meters
constexpr double BOARD_SHADE = 0.32; //!< how bright the boards and the banners are, of the team color
constexpr float BANNER_BEHIND = 22.0f; //!< behind the goal line (m): behind the goal camera (18 m), so that it does not hide the pitch from it
constexpr float BANNER_WIDTH = 50.0f;
constexpr float BANNER_HEIGHT = 8.0f;
constexpr float BANNER_BOTTOM = 12.0f; //!< high above the goal, so that the goal stands out against the sky and the ground beyond
constexpr int BOARD_PIXELS_PER_METER = 64;
constexpr int BANNER_PIXELS_PER_METER = 24;

/*!
  \struct TeamLook
  \brief what one end shows
*/
struct TeamLook {
    QColor color; //!< the team color
    QString name; //!< empty before the team connects
    QImage logo; //!< the team graphic, when the Team Graphic option is on and it has arrived; null otherwise

    //! what the images depend on: they are drawn again when it changes
    QString key() const;
};

//! a color times k (each of red, green and blue)
QColor shade( const QColor & color,
              const double k );

/*!
  \brief one tile of the boards (BOARD_TILE x BOARD_HEIGHT m): the dark
  shade, edges at the top and the bottom, and the name in the team color
*/
QImage boardImage( const TeamLook & look );

/*!
  \brief the banner (BANNER_WIDTH x BANNER_HEIGHT m): the dark shade in a
  frame of the team color, the logo and the name side by side, as large as
  they fit
*/
QImage bannerImage( const TeamLook & look );

/*!
  \brief the panels: the boards of each half and its goal line, and the
  banner behind each goal, their backs in the dark shade
  \param content where to add them (SceneContent::boards, banners)
  \param boards, banners which to add
  \param left, right the team colors
*/
void addPanels( SceneContent & content,
                const bool boards,
                const bool banners,
                const QColor & left,
                const QColor & right );

}

#endif
