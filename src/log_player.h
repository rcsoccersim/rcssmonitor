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

#include <QElapsedTimer>
#include <QObject>

#include <cstddef>

class QTimer;

class DispHolder;

class LogPlayer
    : public QObject {

    Q_OBJECT

private:

    DispHolder & M_disp_holder;

    QTimer * M_timer;

    bool M_live_mode;

    //! since the last step while playing
    QElapsedTimer M_step_clock;

    //! the delayed live view: moves the shown frame on toward liveIndex() (docs/plan/2026-11-11_3d-view.md §5.10)
    QTimer * M_delay_timer;
    //! since the newest frame of the live game came (receivedFrame())
    QElapsedTimer M_newest_clock;

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

    //! playing the log by the timer
    bool isPlaying() const;

    /*!
      \brief while playing, the fraction (0 to 1) of the interval since the
      last step: how far the 3D view is between the current frame and the
      next (docs/plan/2026-11-11_3d-view.md §5.2). 0 when stopped.
    */
    double alpha() const;

    void startTimer();

    //! the time per frame while playing (ms): the options' timer interval, or as made slower or faster
    int interval() const;

    /*!
      \brief the delayed live view (§5.10): how many frames a live game is
      drawn behind its newest one, from the options (the seconds by the
      server's simulator_step); 0 when it is off. Both views use it: the 2D
      view shows liveIndex(), and the 3D view draws the same time
      (Interpolate::liveTime()).
    */
    std::size_t liveDelayFrames() const;

    //! live, and drawn behind the newest frame (liveDelayFrames())
    bool isDelayedLive() const { return M_live_mode && liveDelayFrames() > 0; }

    /*!
      \brief the frame the delayed live view shows: delay frames before the
      newest, going on by the simulation steps since the newest came; when
      the next one is late, on into the frames up to the newest (after the
      game or the connection, the frames are drawn to the end). The first
      frame while there are no more than delay (waiting, right after
      connecting). Interpolate::liveTime() in whole frames.
      \param frame_count the number of the frames
      \param delay the frames behind the newest
      \param since_newest the simulation steps since the newest frame came
    */
    static std::size_t liveIndex( const std::size_t frame_count,
                                  const std::size_t delay,
                                  const double since_newest );

    /*!
      \brief a frame of the live game came: the time of its coming (the
      delayed live view goes on from it), and the frame to show if live
    */
    void receivedFrame();

private:

    void stepBackwardImpl();
    void stepForwardImpl();

    //! the simulation steps since the newest frame came (0 if none has)
    double stepsSinceNewest() const;

    /*!
      \brief show the frame of the delayed live view (liveIndex()): only on,
      as the frames come and the clock goes (a burst of frames after a late
      one does not take it back), or there at once if jump
    */
    void showDelayedLive( const bool jump );

private slots:

    void handleTimer();
    void handleDelayTimer();

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

    /*!
      \brief the options of the delayed live view changed: live, the shown
      frame jumps to where they put it (back by the delay when turned on,
      to the newest when turned off)
    */
    void applyLiveDelay();

signals:

    void signalPlayOrStop( bool play );
    // void updated();
    void indexUpdated( size_t current,
                       size_t maximum );
    void quitRequested();

};

#endif
