// -*-c++-*-

/*!
  \file score_board_painter.h
  \brief score board painter class Header File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa Akiyama

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
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

#ifndef RCSSMONITOR_SCORE_BOARD_PAINTER_H
#define RCSSMONITOR_SCORE_BOARD_PAINTER_H

#include "painter_interface.h"

#include <QPen>
#include <QBrush>
#include <QFont>

class DispHolder;

class ScoreBoardPainter
    : public PainterInterface {
private:

    const DispHolder & M_disp_holder;

    // not used
    ScoreBoardPainter();
    ScoreBoardPainter( const ScoreBoardPainter & );
    const ScoreBoardPainter & operator=( const ScoreBoardPainter & );
public:

    explicit
    ScoreBoardPainter( const DispHolder & disp_holder );
    ~ScoreBoardPainter();

    void draw( QPainter & painter );

};

#endif
