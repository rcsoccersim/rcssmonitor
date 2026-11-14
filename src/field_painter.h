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

    const DispHolder & M_disp_holder;

    //! the width of the pens, in pixels of the 2D view (1 for the 2D view)
    double M_pen_scale;

    // not used
    FieldPainter() = delete;
    FieldPainter( const FieldPainter & ) = delete;
    const FieldPainter & operator=( const FieldPainter & ) = delete;
public:

    FieldPainter( DispHolder & disp_holder );
    ~FieldPainter();

    void draw( QPainter & painter ) override;

    //! the grass mowed in stripes (the ground of the 3D view): this wide (m), across the pitch, one edge on the halfway line (20 on the pitch)
    static constexpr double GRASS_STRIPE_WIDTH = 105.0 / 20.0;
    //! the light stripes are the field color times this, the dark ones times that (rcssmonitor-web GRASS)
    static constexpr double GRASS_LIGHT = 1.05;
    static constexpr double GRASS_DARK = 0.92;

    /*!
      \brief draw the field without the goals, which the 3D view builds
      itself, with the pens pen_scale times wider: the ground of the 3D view.
      \param stripes the grass mowed in stripes, over the field color and
      under the lines: light from the halfway line to the right, then dark,
      and so on both ways (the stripes left and right of the halfway line
      differ, as the mower turns there)
    */
    void drawGround( QPainter & painter,
                     const double pen_scale,
                     const bool stripes = false );

private:

    QPen pen( const QPen & pen ) const;

    void drawBackGround( QPainter & painter ) const;
    void drawGrassStripes( QPainter & painter ) const;
    void drawLines( QPainter & painter ) const;
    void drawPenaltyAreaLines( QPainter & painter ) const;
    void drawGoalAreaLines( QPainter & painter ) const;
    void drawGoals( QPainter & painter ) const;
    void drawFlags( QPainter & painter ) const;
    void drawGrid( QPainter & painter ) const;
};

#endif
