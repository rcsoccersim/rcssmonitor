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

    bool M_forward;
    bool M_live_mode;
    bool M_need_recovering;

    // not used
    LogPlayer();
    LogPlayer( const LogPlayer & );
    const LogPlayer & operator=( const LogPlayer & );
public:

    LogPlayer( DispHolder & disp_holder,
               QObject * parent );
    ~LogPlayer();

    void clear();

    bool isLiveMode() const { return M_live_mode; }

    void startTimer();

private:

    void adjustTimer();
    void stepBackImpl();
    void stepForwardImpl();


private slots:

    void handleTimer();

public slots:

    void stepBack();
    void stepForward();

    void playOrStop();
    void stop();

    void playBack();
    void playForward();

    void accelerateBack();
    void accelerateForward();

//     void goToPrevScore();
//     void goToNextScore();

    void goToFirst();
    void goToLast();

    void decelerate();
    void accelerate();

    void goToIndex( int index );
    void goToCycle( int cycle );

    void showLive();
    void setLiveMode();

signals:

    void updated();
    void recoverTimerHandled();
    void quitRequested();

};

#endif
