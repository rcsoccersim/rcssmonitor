// -*-c++-*-

/*!
  \file field_painter.h
  \brief field painter class Header File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.	If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef RCSSMONITOR_FIELD_PAINTER_H
#define RCSSMONITOR_FIELD_PAINTER_H

#include "painter_interface.h"

#include <QPen>
#include <QBrush>

class DispHolder;

class FieldPainter
    : public PainterInterface {
private:

    DispHolder & M_disp_holder;

    // not used
    FieldPainter();
    FieldPainter( const FieldPainter & );
    const FieldPainter & operator=( const FieldPainter & );
public:

    FieldPainter( DispHolder & disp_holder );
    ~FieldPainter();

    void draw( QPainter & painter );

private:

    void drawBackGround( QPainter & painter ) const;
    void drawLines( QPainter & painter ) const;
    void drawPenaltyAreaLines( QPainter & painter ) const;
    void drawGoalAreaLines( QPainter & painter ) const;
    void drawGoals( QPainter & painter ) const;
    void drawFlags( QPainter & painter ) const;
    void drawGrid( QPainter & painter ) const;
};

#endif
