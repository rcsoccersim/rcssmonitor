// -*-c++-*-

/*!
  \file test_log_player.cpp
  \brief unit test of LogPlayer: the delayed live view
*/

/*
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
*/

/////////////////////////////////////////////////////////////////////

#include <QtTest>

#include "disp_holder.h"
#include "log_player.h"
#include "options.h"

#include <cmath>
#include <limits>
#include <string>

/*
  The delayed live view (docs/plan/2026-11-11_3d-view.md §5.10): a live game drawn
  some seconds behind its newest frame, so that the 3D view knows the
  touches ahead. Options reads and writes ~/.rcssmonitor.conf; CTest runs
  this test with HOME in the build tree (test/CMakeLists.txt).
*/

namespace {

//! add a frame of the given time as a live JSON message
bool
addLiveFrame( DispHolder & holder,
              const int time )
{
    const std::string msg = "{\"show\":{\"time\":" + std::to_string( time )
        + ",\"ball\":{\"x\":0,\"y\":0,\"vx\":0,\"vy\":0},\"players\":[]}}";
    return holder.addJSON( msg.c_str() );
}

//! add the frames of the times [first, last]
void
addLiveFrames( DispHolder & holder,
               const int first,
               const int last )
{
    for ( int t = first; t <= last; ++t )
    {
        QVERIFY( addLiveFrame( holder, t ) );
    }
}

//! the server's simulation step (ms), as the server_param message gives it
void
setSimulatorStep( DispHolder & holder,
                  const int step )
{
    rcss::rcg::ServerParamT param;
    param.simulator_step_ = step;
    holder.handleServerParam( param );
}

//! the options of the delayed live view
void
setDelay( const bool on,
          const double seconds )
{
    Options::instance().setLiveDelay( on );
    Options::instance().setLiveDelaySeconds( seconds );
}

}

/*!
  \brief unit test of LogPlayer's delayed live view
*/
class TestLogPlayer
    : public QObject {
    Q_OBJECT

private slots:

    void cleanup()
      {
          setDelay( false, Options::DEFAULT_LIVE_DELAY_SECONDS );
      }

    void options();
    void liveIndex();
    void delayFrames();
    void following();
    void switching();
    void drawnToTheEnd();
    void notLive();
};

/*-------------------------------------------------------------------*/
/*!
  The seconds: off and 3 s by default; within 1 to 7 s, to a step of 0.5.
*/
void
TestLogPlayer::options()
{
    Options & opt = Options::instance();
    QCOMPARE( Options::DEFAULT_LIVE_DELAY_SECONDS, 3.0 );
    QCOMPARE( Options::MIN_LIVE_DELAY_SECONDS, 1.0 );
    QCOMPARE( Options::MAX_LIVE_DELAY_SECONDS, 7.0 );
    QVERIFY( ! opt.liveDelay() );
    QCOMPARE( opt.liveDelaySeconds(), 3.0 );

    opt.setLiveDelaySeconds( 3.2 );
    QCOMPARE( opt.liveDelaySeconds(), 3.0 );
    opt.setLiveDelaySeconds( 3.3 );
    QCOMPARE( opt.liveDelaySeconds(), 3.5 );
    opt.setLiveDelaySeconds( 0.3 );
    QCOMPARE( opt.liveDelaySeconds(), 1.0 );
    opt.setLiveDelaySeconds( 10.0 );
    QCOMPARE( opt.liveDelaySeconds(), 7.0 );
    opt.setLiveDelaySeconds( std::numeric_limits< double >::quiet_NaN() );
    QCOMPARE( opt.liveDelaySeconds(), 7.0 ); // kept
}

/*-------------------------------------------------------------------*/
/*!
  The frame shown: delay frames before the newest, on by the steps since
  it came, up to the newest; the first while there are no more than delay.
*/
void
TestLogPlayer::liveIndex()
{
    QCOMPARE( LogPlayer::liveIndex( 100, 30, 0.0 ), std::size_t( 69 ) );
    QCOMPARE( LogPlayer::liveIndex( 100, 30, 0.9 ), std::size_t( 69 ) );
    QCOMPARE( LogPlayer::liveIndex( 100, 30, 1.0 ), std::size_t( 70 ) );
    QCOMPARE( LogPlayer::liveIndex( 100, 30, 12.5 ), std::size_t( 81 ) );
    QCOMPARE( LogPlayer::liveIndex( 100, 30, 40.0 ), std::size_t( 99 ) );
    QCOMPARE( LogPlayer::liveIndex( 100, 30, -1.0 ), std::size_t( 69 ) );
    QCOMPARE( LogPlayer::liveIndex( 20, 30, 0.5 ), std::size_t( 0 ) );
    QCOMPARE( LogPlayer::liveIndex( 30, 30, 5.0 ), std::size_t( 0 ) ); // waiting, also when a frame is late
    QCOMPARE( LogPlayer::liveIndex( 31, 30, 0.0 ), std::size_t( 0 ) );
    QCOMPARE( LogPlayer::liveIndex( 32, 30, 0.0 ), std::size_t( 1 ) );
    QCOMPARE( LogPlayer::liveIndex( 0, 30, 0.0 ), std::size_t( 0 ) );
    QCOMPARE( LogPlayer::liveIndex( 100, 0, 5.0 ), std::size_t( 99 ) ); // not delayed: the newest
}

/*-------------------------------------------------------------------*/
/*!
  The frames behind the newest: the seconds by the server's simulation
  step; none when off.
*/
void
TestLogPlayer::delayFrames()
{
    DispHolder holder;
    LogPlayer player( holder, nullptr );
    QCOMPARE( player.liveDelayFrames(), std::size_t( 0 ) );
    setDelay( true, 3.0 );
    QCOMPARE( player.liveDelayFrames(), std::size_t( 30 ) ); // 100 ms by default
    setDelay( true, 1.5 );
    QCOMPARE( player.liveDelayFrames(), std::size_t( 15 ) );
    setSimulatorStep( holder, 50 );
    QCOMPARE( player.liveDelayFrames(), std::size_t( 30 ) );
    setDelay( false, 1.5 );
    QCOMPARE( player.liveDelayFrames(), std::size_t( 0 ) );
    QVERIFY( ! player.isDelayedLive() );
}

/*-------------------------------------------------------------------*/
/*!
  Live and delayed, the frame shown follows the frames as they come, the
  delay behind the newest; a burst of frames after a late one does not
  take it back; with fewer frames than the delay, the first one waits.
*/
void
TestLogPlayer::following()
{
    setDelay( true, 3.0 );

    // the first frame waits until there are more than the delay
    {
        DispHolder holder;
        LogPlayer player( holder, nullptr );
        player.setLiveMode();
        QVERIFY( player.isDelayedLive() );
        addLiveFrames( holder, 1, 5 );
        player.receivedFrame();
        QCOMPARE( holder.currentIndex(), std::size_t( 0 ) );
        addLiveFrames( holder, 6, 31 );
        player.receivedFrame();
        QCOMPARE( holder.currentIndex(), std::size_t( 0 ) );
        addLiveFrames( holder, 32, 33 );
        player.receivedFrame();
        QCOMPARE( holder.currentIndex(), std::size_t( 2 ) );
    }

    DispHolder holder;
    LogPlayer player( holder, nullptr );
    QSignalSpy updated( &player, SIGNAL( indexUpdated( size_t, size_t ) ) );
    addLiveFrames( holder, 1, 100 );
    player.setLiveMode();
    QCOMPARE( holder.currentIndex(), std::size_t( 69 ) );
    QCOMPARE( updated.count(), 1 );

    QVERIFY( addLiveFrame( holder, 101 ) );
    player.receivedFrame();
    QCOMPARE( holder.currentIndex(), std::size_t( 70 ) );
    QCOMPARE( holder.currentDisp()->show_.time_, rcss::rcg::UInt32( 71 ) );
    QCOMPARE( updated.count(), 2 );

    // frames in a burst: the delay behind the newest
    addLiveFrames( holder, 102, 106 );
    player.receivedFrame();
    QCOMPARE( holder.currentIndex(), std::size_t( 75 ) );

    // ahead of where the frames put it (moved on by the clock while a
    // frame was late): not back
    QVERIFY( holder.setIndex( 78 ) );
    QVERIFY( addLiveFrame( holder, 107 ) );
    player.receivedFrame();
    QCOMPARE( holder.currentIndex(), std::size_t( 78 ) );
}

/*-------------------------------------------------------------------*/
/*!
  Turned on live, the view jumps back by the delay; the seconds changed,
  to the new delay; turned off, on to the newest frame. The Live Mode
  button goes back to the delayed view.
*/
void
TestLogPlayer::switching()
{
    DispHolder holder;
    LogPlayer player( holder, nullptr );
    addLiveFrames( holder, 1, 100 );
    player.setLiveMode();
    QCOMPARE( holder.currentIndex(), std::size_t( 99 ) );
    QVERIFY( ! player.isDelayedLive() );

    setDelay( true, 3.0 );
    player.applyLiveDelay();
    QCOMPARE( holder.currentIndex(), std::size_t( 69 ) );
    QVERIFY( player.isDelayedLive() );

    setDelay( true, 5.0 );
    player.applyLiveDelay();
    QCOMPARE( holder.currentIndex(), std::size_t( 49 ) );
    setDelay( true, 2.0 );
    player.applyLiveDelay();
    QCOMPARE( holder.currentIndex(), std::size_t( 79 ) );

    setDelay( false, 2.0 );
    player.applyLiveDelay();
    QCOMPARE( holder.currentIndex(), std::size_t( 99 ) );
    QVERIFY( ! player.isDelayedLive() );

    // back to live from the log player's buttons
    setDelay( true, 3.0 );
    player.applyLiveDelay();
    player.goToFirst();
    QVERIFY( ! player.isLiveMode() );
    QCOMPARE( holder.currentIndex(), std::size_t( 0 ) );
    player.setLiveMode();
    QCOMPARE( holder.currentIndex(), std::size_t( 69 ) );
}

/*-------------------------------------------------------------------*/
/*!
  When the frames stop (the game is over, the connection is lost), the
  view goes on by the clock at the speed of the simulation, and draws the
  delayed frames to the end.
*/
void
TestLogPlayer::drawnToTheEnd()
{
    DispHolder holder;
    LogPlayer player( holder, nullptr );
    setSimulatorStep( holder, 10 ); // a fast server: the test does not wait long
    setDelay( true, 1.0 ); // 100 frames
    addLiveFrames( holder, 1, 200 );
    player.receivedFrame();
    player.setLiveMode();
    QCOMPARE( player.liveDelayFrames(), std::size_t( 100 ) );
    QVERIFY( holder.currentIndex() <= std::size_t( 100 ) );

    // on by the clock while no frame comes
    QTRY_VERIFY_WITH_TIMEOUT( holder.currentIndex() >= std::size_t( 110 ), 3000 );
    QVERIFY( holder.currentIndex() < std::size_t( 199 ) );
    // a frame coming late does not take it back
    const std::size_t before = holder.currentIndex();
    QVERIFY( addLiveFrame( holder, 201 ) );
    player.receivedFrame();
    QVERIFY( holder.currentIndex() >= before );
    // to the end
    QTRY_COMPARE_WITH_TIMEOUT( holder.currentIndex(), std::size_t( 200 ), 5000 );
}

/*-------------------------------------------------------------------*/
/*!
  Not live (a log, or the frames looked at with the player's buttons), the
  options do not move the view.
*/
void
TestLogPlayer::notLive()
{
    DispHolder holder;
    LogPlayer player( holder, nullptr );
    addLiveFrames( holder, 1, 100 );
    QVERIFY( holder.setIndex( 10 ) );
    setDelay( true, 3.0 );
    player.applyLiveDelay();
    QCOMPARE( holder.currentIndex(), std::size_t( 10 ) );
    QVERIFY( ! player.isDelayedLive() );

    // live, then the buttons: no longer moved by the clock
    player.setLiveMode();
    QCOMPARE( holder.currentIndex(), std::size_t( 69 ) );
    player.stepBackward();
    QCOMPARE( holder.currentIndex(), std::size_t( 68 ) );
    QVERIFY( addLiveFrame( holder, 101 ) );
    player.receivedFrame();
    QTest::qWait( 100 );
    QCOMPARE( holder.currentIndex(), std::size_t( 68 ) );
}

QTEST_MAIN( TestLogPlayer )

#include "test_log_player.moc"
