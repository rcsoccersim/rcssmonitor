// -*-c++-*-

/*!
  \file log_slider_tool_bar.h
  \brief log player slider tool bar class Header File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
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

#ifndef RCSSMONITOR_LOG_PLAYER_SLIDER_H
#define RCSSMONITOR_LOG_PLAYER_SLIDER_H

#include <QSlider>

class LogPlayerSlider
    : public QSlider {

    Q_OBJECT

public:

    LogPlayerSlider( QWidget * parent = 0 );

protected:

    void mousePressEvent( QMouseEvent * event );

public slots:

    void changeOrientation( Qt::Orientation orientation );
    void updateIndex( size_t current,
                      size_t maximum );

signals:

    void cycleChanged( int cycle );

};

#endif
