// -*-c++-*-

/*!
  \file ball_painer.h
  \brief ball painter class Header File.
*/

/*
 *Copyright:

 Copyright (C)  The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

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

#ifndef RCSSMONITOR_BALL_PAINTER_H
#define RCSSMONITOR_BALL_PAINTER_H

#include "painter_interface.h"

class DispHolder;

class BallPainter
    : public PainterInterface {
private:
    const DispHolder & M_disp_holder;

    // not used
    BallPainter() = delete;
    BallPainter( const BallPainter & ) = delete;
    const BallPainter & operator=( const BallPainter & ) = delete;
public:

    BallPainter( const DispHolder & disp_holder );
    ~BallPainter();

    void draw( QPainter & painter ) override;

private:

    void drawVelocity( QPainter & painter ) const;

};

#endif
