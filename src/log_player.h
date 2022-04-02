// -*-c++-*-

/*!
  \file log_player.h
  \brief log player class Header File.
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

#ifndef RCSSMONITOR_LOG_PLAYER_H
#define RCSSMONITOR_LOG_PLAYER_H

#include <QObject>

class QTimer;

class DispHolder;

class LogPlayer
    : public QObject {

    Q_OBJECT

private:

    DispHolder & M_disp_holder;

    QTimer * M_timer;

    bool M_live_mode;

    // not used
    LogPlayer() = delete;
    LogPlayer( const LogPlayer & ) = delete;
    const LogPlayer & operator=( const LogPlayer & ) = delete;
public:

    LogPlayer( DispHolder & disp_holder,
               QObject * parent );
    ~LogPlayer();

    void clear();

    bool isLiveMode() const { return M_live_mode; }

    void startTimer();

private:

    void stepBackwardImpl();
    void stepForwardImpl();


private slots:

    void handleTimer();

public slots:

    void stepBackward();
    void stepForward();

    void playOrStop( bool play );
    void stop();

    void playBackward();
    void playForward();

    void decelerate();
    void accelerate();

    void goToPreviousScore();
    void goToNextScore();

    void goToFirst();
    void goToLast();

    void goToIndex( int index );
    void goToCycle( int cycle );

    void showLive();
    void setLiveMode();

signals:

    void signalPlayOrStop( bool play );
    // void updated();
    void indexUpdated( size_t current,
                       size_t maximum );
    void quitRequested();

};

#endif
