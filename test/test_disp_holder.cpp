// -*-c++-*-

/*!
  \file test_disp_holder.cpp
  \brief unit test of DispHolder
*/

/*
 *Copyright:

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

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#include <QtTest>

#include "disp_holder.h"

#include <string>

/*
  DispHolder used to stop storing frames at 65535, which froze the live
  view. Now a live connection keeps the newest frames, dropping the oldest
  ones, and a log file is kept whole.
*/

namespace {

/*!
  \brief add a frame of the given time as a live JSON message.
*/
bool
addLiveFrame( DispHolder & holder,
              const int time )
{
    const std::string msg = "{\"show\":{\"time\":" + std::to_string( time )
        + ",\"ball\":{\"x\":0,\"y\":0,\"vx\":0,\"vy\":0},\"players\":[]}}";
    return holder.addJSON( msg.c_str() );
}

/*!
  \brief add the frames of the times [first, last] as live messages.
*/
bool
addLiveFrames( DispHolder & holder,
               const int first,
               const int last )
{
    for ( int t = first; t <= last; ++t )
    {
        if ( ! addLiveFrame( holder, t ) )
        {
            return false;
        }
    }
    return true;
}

/*!
  \brief add a live team message with the given score of the left team.
*/
bool
addLiveScore( DispHolder & holder,
              const int time,
              const int score_l )
{
    const std::string msg = "{\"team\":{\"time\":" + std::to_string( time )
        + ",\"l\":{\"name\":\"L\",\"score\":" + std::to_string( score_l )
        + "},\"r\":{\"name\":\"R\",\"score\":0}}}";
    return holder.addJSON( msg.c_str() );
}

rcss::rcg::UInt32
timeAt( const DispHolder & holder,
        const size_t idx )
{
    return holder.dispCont().at( idx )->show_.time_;
}

const int MAX = static_cast< int >( DispHolder::MAX_LIVE_FRAMES );
const int TRIM = static_cast< int >( DispHolder::LIVE_TRIM_FRAMES );
//! the number of frames left after a trim
const int LEFT = MAX - TRIM;

}

/*!
  \brief unit test of DispHolder
*/
class TestDispHolder
    : public QObject {
    Q_OBJECT

private slots:

    void liveFramesTrimmed();
    void liveCurrentIndex();
    void liveScoreChangedIndex();
    void liveDrawData();
    void logFramesKept();
};

/*-------------------------------------------------------------------*/
/*!
  A live connection keeps up to MAX_LIVE_FRAMES frames. One more drops the
  oldest ones at once, and the newest frame can still be shown.
*/
void
TestDispHolder::liveFramesTrimmed()
{
    DispHolder holder;

    QVERIFY( addLiveFrames( holder, 1, MAX ) );
    QCOMPARE( int( holder.dispCont().size() ), MAX );
    QCOMPARE( timeAt( holder, 0 ), rcss::rcg::UInt32( 1 ) );

    QVERIFY( addLiveFrame( holder, MAX + 1 ) );
    QCOMPARE( int( holder.dispCont().size() ), LEFT );
    QCOMPARE( timeAt( holder, 0 ), rcss::rcg::UInt32( MAX + 1 - LEFT + 1 ) );

    QVERIFY( holder.setIndexLast() );
    QCOMPARE( holder.currentDisp()->show_.time_, rcss::rcg::UInt32( MAX + 1 ) );

    // and again, after the same number of frames
    QVERIFY( addLiveFrames( holder, MAX + 2, MAX + 1 + TRIM ) );
    QCOMPARE( int( holder.dispCont().size() ), MAX );
    QVERIFY( addLiveFrame( holder, MAX + 2 + TRIM ) );
    QCOMPARE( int( holder.dispCont().size() ), LEFT );
    QCOMPARE( timeAt( holder, holder.dispCont().size() - 1 ), rcss::rcg::UInt32( MAX + 2 + TRIM ) );
}

/*-------------------------------------------------------------------*/
/*!
  The current index keeps showing the same frame after a trim. If that
  frame was dropped, it moves to the oldest frame left.
*/
void
TestDispHolder::liveCurrentIndex()
{
    DispHolder holder;
    QVERIFY( addLiveFrames( holder, 1, MAX ) );

    QVERIFY( holder.setIndex( 9999 ) );
    QCOMPARE( holder.currentDisp()->show_.time_, rcss::rcg::UInt32( 10000 ) );

    QVERIFY( addLiveFrame( holder, MAX + 1 ) );
    QCOMPARE( holder.currentDisp()->show_.time_, rcss::rcg::UInt32( 10000 ) );
    QCOMPARE( int( holder.currentIndex() ), 9999 - ( MAX + 1 - LEFT ) );

    // a frame that the next trim drops
    QVERIFY( holder.setIndex( 10 ) );
    const int last = MAX + 1 + ( MAX - LEFT ) + 1;
    QVERIFY( addLiveFrames( holder, MAX + 2, last ) );
    QCOMPARE( int( holder.currentIndex() ), 0 );
    QCOMPARE( holder.currentDisp()->show_.time_, timeAt( holder, 0 ) );
}

/*-------------------------------------------------------------------*/
/*!
  The indices of the score changes shift with a trim, and those of the
  dropped frames are removed.
*/
void
TestDispHolder::liveScoreChangedIndex()
{
    DispHolder holder;
    QVERIFY( addLiveFrames( holder, 1, 10 ) );
    QVERIFY( addLiveScore( holder, 10, 1 ) ); // at the frame of time 10
    QVERIFY( addLiveFrames( holder, 11, 5000 ) );
    QVERIFY( addLiveScore( holder, 5000, 2 ) ); // at the frame of time 5000
    QCOMPARE( holder.scoreChangedIndex().size(), size_t( 2 ) );

    QVERIFY( addLiveFrames( holder, 5001, MAX + 1 ) );

    QCOMPARE( holder.scoreChangedIndex().size(), size_t( 1 ) );
    QCOMPARE( timeAt( holder, holder.scoreChangedIndex().front() ), rcss::rcg::UInt32( 5000 ) );
}

/*-------------------------------------------------------------------*/
/*!
  The draw data before the oldest frame left are dropped with a trim.
*/
void
TestDispHolder::liveDrawData()
{
    DispHolder holder;
    const rcss::rcg::PointT point( 0.0f, 0.0f, "red" );
    QVERIFY( holder.handleDrawPoint( 100, point ) );
    QVERIFY( holder.handleDrawPoint( 6000, point ) );

    QVERIFY( addLiveFrames( holder, 1, MAX + 1 ) );

    QCOMPARE( holder.pointCont().count( 100 ), size_t( 0 ) );
    QCOMPARE( holder.pointCont().count( 6000 ), size_t( 1 ) );
}

/*-------------------------------------------------------------------*/
/*!
  The frames of a log file are kept whole, also after MAX_LIVE_FRAMES.
*/
void
TestDispHolder::logFramesKept()
{
    const size_t count = DispHolder::MAX_LIVE_FRAMES + 2;

    DispHolder holder;
    rcss::rcg::ShowInfoT show;
    for ( size_t i = 0; i < count; ++i )
    {
        show.time_ = static_cast< rcss::rcg::UInt32 >( i + 1 );
        QVERIFY( holder.handleShow( show ) );
    }

    QCOMPARE( holder.dispCont().size(), count );
    QVERIFY( holder.setIndexLast() );
    QCOMPARE( holder.currentIndex(), count - 1 );
    QCOMPARE( holder.currentDisp()->show_.time_, rcss::rcg::UInt32( count ) );
}

QTEST_MAIN( TestDispHolder )

#include "test_disp_holder.moc"
