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

#ifndef RCSSLOGPLAYER_BALL_PAINTER_H
#define RCSSLOGPLAYER_BALL_PAINTER_H

#include "painter_interface.h"

class MainData;

class BallPainter
    : public PainterInterface {
private:
    const MainData & M_main_data;

    QPen M_ball_pen;
    QPen M_ball_vel_pen;
    QBrush M_ball_brush;

    // not used
    BallPainter();
    BallPainter( const BallPainter & );
    const BallPainter & operator=( const BallPainter & );
public:

    BallPainter( const MainData & main_data );
    ~BallPainter();

    void draw( QPainter & painter );

private:


    void readSettings();
    void writeSettings();

    void drawVelocity( QPainter & painter ) const;
    void drawTrace( QPainter & painter ) const;

};

#endif
