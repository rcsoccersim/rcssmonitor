// -*-c++-*-

/*!
  \file test_whistle.cpp
  \brief unit test of Whistle: the referee's whistle at the changes of the
  play mode of the frames shown, its options, its menu and its settings
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtTest>

#include "config_dialog.h"
#include "disp_holder.h"
#include "log_player.h"
#include "main_window.h"
#include "options.h"
#include "whistle.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QFile>
#include <QGroupBox>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QTemporaryDir>

#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

/*
  The referee's whistle (docs/plan/2026-11-22-00_v20-features.md §C). The
  frames are made by live JSON messages with their play modes; a fake
  sound output records what is played. Options reads and writes
  ~/.rcssmonitor.conf; CTest runs this test with HOME in the build tree
  (test/CMakeLists.txt).
*/

namespace {

//! the environment variable that makes the test the child of commandLine()
const char * const CHILD_ENV = "RCSSMONITOR_TEST_WHISTLE_CHILD";
//! the child prints the options after it
const QString CHILD_PREFIX = QStringLiteral( "whistle=" );

/*!
  \class FakeSound
  \brief records the whistles played, as "short" or "long" with the volume,
  and whether the output is held open
*/
class FakeSound
    : public WhistleSound {
public:
    struct Record {
        std::vector< std::string > plays; //!< "short" or "long"
        std::vector< int > volumes;
        int stops = 0;
        bool held = false;
    };

private:
    Record & M_record;

public:
    explicit FakeSound( Record & record )
        : M_record( record )
      { }

    void play( const bool long_whistle,
               const int volume ) override
      {
          M_record.plays.push_back( long_whistle ? "long" : "short" );
          M_record.volumes.push_back( volume );
      }

    void stop() override
      {
          ++M_record.stops;
      }

    void hold( const bool on ) override
      {
          M_record.held = on;
      }
};

//! the factory of a fake sound that records to a record
Whistle::SoundFactory
fakeFactory( FakeSound::Record & record )
{
    return [&record]() {
        return std::unique_ptr< WhistleSound >( new FakeSound( record ) );
    };
}

//! add a frame of a time and a play mode as a live JSON message
bool
addFrame( DispHolder & holder,
          const int time,
          const char * mode )
{
    const std::string msg = "{\"show\":{\"time\":" + std::to_string( time )
        + ",\"mode\":\"" + mode + "\""
        + ",\"ball\":{\"x\":0,\"y\":0,\"vx\":0,\"vy\":0},\"players\":[]}}";
    return holder.addJSON( msg.c_str() );
}

/*!
  a game in short (as the logs have them, C.3.4), with the whistles at its
  frames: the kick-off, a foul and its free kick, a goal (none) and its kick-off,
  the kick-off of the second half (right after play_on, as in the logs), and
  the end of the game
*/
const std::vector< std::pair< const char *, const char * > > &
game()
{
    static const std::vector< std::pair< const char *, const char * > > frames = {
        { "before_kick_off", "" }, // 0
        { "kick_off_l", "long" }, // 1
        { "play_on", "" }, // 2
        { "play_on", "" }, // 3
        { "foul_charge_l", "short" }, // 4
        { "foul_charge_l", "" }, // 5
        { "free_kick_r", "short" }, // 6
        { "play_on", "" }, // 7
        { "goal_l", "" }, // 8: no whistle at a goal
        { "goal_l", "" }, // 9
        { "kick_off_r", "long" }, // 10
        { "play_on", "" }, // 11
        { "kick_off_l", "long" }, // 12: the second half (one long; no end of the half in the log)
        { "play_on", "" }, // 13
        { "kick_in_r", "" }, // 14
        { "play_on", "" }, // 15
        { "time_over", "long" }, // 16: the end of the game (three long)
    };
    return frames;
}

//! add the frames of game() at the times 0, 1, 2, ...
void
addGame( DispHolder & holder )
{
    int time = 0;
    for ( const auto & f : game() )
    {
        QVERIFY( addFrame( holder, time++, f.first ) );
    }
}

//! the whistles of the frames of game() in (from, to]
std::vector< std::string >
gameWhistles( const std::size_t from,
              const std::size_t to )
{
    std::vector< std::string > whistles;
    for ( std::size_t i = from + 1; i <= to && i < game().size(); ++i )
    {
        if ( *game()[i].second ) whistles.push_back( game()[i].second );
    }
    return whistles;
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

//! the saved value of a key, or a null QVariant
QVariant
savedValue( const QString & key )
{
    QSettings settings( Options::CONF_FILE, QSettings::IniFormat );
    return settings.value( key );
}

/*!
  \struct Fixture
  \brief the frames, the player, and the whistle with a fake sound,
  connected as in MainWindow
*/
struct Fixture {
    DispHolder holder;
    LogPlayer player;
    FakeSound::Record record;
    Whistle whistle;

    Fixture()
        : player( holder, nullptr ),
          whistle( holder, player, fakeFactory( record ), nullptr )
      {
          QObject::connect( &player, &LogPlayer::indexUpdated,
                            &whistle, &Whistle::handleIndexUpdated );
      }
};

}

/*!
  \brief unit test of the referee's whistle
*/
class TestWhistle
    : public QObject {
    Q_OBJECT

private slots:

    void init()
      {
          Options & opt = Options::instance();
          opt.setWhistle( true );
          opt.setWhistleVolume( Options::DEFAULT_WHISTLE_VOLUME );
          opt.setAutoLoopMode( false );
          opt.setLiveDelay( false );
          opt.setLiveDelaySeconds( Options::DEFAULT_LIVE_DELAY_SECONDS );
      }

    void cleanupTestCase()
      {
          Options::instance().setWhistle( false );
      }

    void options();
    void replay();
    void firstKickOff();
    void notFollowing();
    void autoLoop();
    void off();
    void live();
    void liveBurst();
    void liveTrimmed();
    void delayedLive();
    void reopened();
    void longWhistles();
    void stopped();
    void held();
    void volume();
    void wav();
    void mixer();
    void sound();
    void menu();
    void startup();
    void settingsDialog();
    void commandLine_data();
    void commandLine();
};

/*-------------------------------------------------------------------*/
/*!
  Off and 50 by default; the volume within 0 to 100. writeWhistle() and
  writeWhistleVolume() save each of them now, in [Sound].
*/
void
TestWhistle::options()
{
    QCOMPARE( Options::MIN_WHISTLE_VOLUME, 0 );
    QCOMPARE( Options::MAX_WHISTLE_VOLUME, 100 );
    QCOMPARE( Options::DEFAULT_WHISTLE_VOLUME, 50 );
    // the file made with the defaults (when the options were made)
    QCOMPARE( savedValue( "Sound/whistle" ).toBool(), false );
    QCOMPARE( savedValue( "Sound/whistle_volume" ).toInt(), 50 );

    Options & opt = Options::instance();
    opt.setWhistleVolume( 120 );
    QCOMPARE( opt.whistleVolume(), 100 );
    opt.setWhistleVolume( -5 );
    QCOMPARE( opt.whistleVolume(), 0 );
    opt.setWhistleVolume( 30 );
    QCOMPARE( opt.whistleVolume(), 30 );

    opt.setWhistle( true );
    opt.writeWhistle();
    QCOMPARE( savedValue( "Sound/whistle" ).toBool(), true );
    QCOMPARE( savedValue( "Sound/whistle_volume" ).toInt(), 50 ); // not yet
    opt.writeWhistleVolume();
    QCOMPARE( savedValue( "Sound/whistle_volume" ).toInt(), 30 );

    opt.setWhistle( false );
    opt.setWhistleVolume( Options::DEFAULT_WHISTLE_VOLUME );
    opt.writeWhistle();
    opt.writeWhistleVolume();
    QCOMPARE( savedValue( "Sound/whistle" ).toBool(), false );
    QCOMPARE( savedValue( "Sound/whistle_volume" ).toInt(), 50 );
}

/*-------------------------------------------------------------------*/
/*!
  Log replay, playing or a step forward: the whistle of each change, at
  the frame of the change. None at the first frame.
*/
void
TestWhistle::replay()
{
    Fixture f;
    addGame( f.holder );

    f.player.goToFirst();
    QVERIFY( f.record.plays.empty() );
    QVERIFY( f.whistle.hasSound() ); // made at the first frame shown

    for ( std::size_t i = 1; i < game().size(); ++i )
    {
        const std::size_t before = f.record.plays.size();
        f.player.stepForward();
        QCOMPARE( f.holder.currentIndex(), i );
        const std::string expected = game()[i].second;
        if ( expected.empty() )
        {
            QCOMPARE( f.record.plays.size(), before );
        }
        else
        {
            QCOMPARE( f.record.plays.size(), before + 1 );
            QCOMPARE( f.record.plays.back(), expected );
        }
    }
    QCOMPARE( f.record.plays, gameWhistles( 0, game().size() - 1 ) );
}

/*-------------------------------------------------------------------*/
/*!
  Log replay from the first frame, a kick-off (the logs start with kick_off
  at the time 1): the kick-off's whistle when the play starts (the step
  to the next frame), not when the log is opened. Not live, and not a log
  that starts with another mode.
*/
void
TestWhistle::firstKickOff()
{
    {
        Fixture f;
        QVERIFY( addFrame( f.holder, 1, "kick_off_l" ) );
        QVERIFY( addFrame( f.holder, 2, "kick_off_l" ) );
        QVERIFY( addFrame( f.holder, 3, "play_on" ) );

        f.player.goToFirst(); // the log opened
        QVERIFY( f.record.plays.empty() );
        f.player.stepForward(); // played
        QCOMPARE( f.record.plays, std::vector< std::string >( { "long" } ) );
        f.player.stepForward();
        QCOMPARE( f.record.plays.size(), std::size_t( 1 ) );

        // again from the first frame
        f.player.goToFirst();
        f.player.stepForward();
        QCOMPARE( f.record.plays, std::vector< std::string >( { "long", "long" } ) );
        f.player.stepBackward();
        f.player.stepForward();
        QCOMPARE( f.record.plays.size(), std::size_t( 3 ) );

        // a jump from the first frame
        f.player.goToFirst();
        f.player.goToIndex( 2 );
        QCOMPARE( f.record.plays.size(), std::size_t( 3 ) );
    }
    {
        // a log that starts in the play (cut)
        Fixture f;
        QVERIFY( addFrame( f.holder, 100, "play_on" ) );
        QVERIFY( addFrame( f.holder, 101, "play_on" ) );
        f.player.goToFirst();
        f.player.stepForward();
        QVERIFY( f.record.plays.empty() );
    }
    {
        // live: the first frame of a connection is not the start of the game
        Fixture f;
        f.player.setLiveMode();
        QVERIFY( addFrame( f.holder, 1, "kick_off_l" ) );
        f.player.receivedFrame();
        QVERIFY( addFrame( f.holder, 2, "kick_off_l" ) );
        f.player.receivedFrame();
        QVERIFY( f.record.plays.empty() );
    }
}

/*-------------------------------------------------------------------*/
/*!
  Log replay: no whistle after a jump (the slider, a cycle, the first or the
  last frame) or a step back; the frame shown is kept, so that the next step
  forward blows the whistle of its change.
*/
void
TestWhistle::notFollowing()
{
    Fixture f;
    addGame( f.holder );
    f.player.goToFirst();

    // over the kick-off and the foul
    f.player.goToIndex( 5 );
    QCOMPARE( f.holder.currentIndex(), std::size_t( 5 ) );
    QVERIFY( f.record.plays.empty() );

    // the free kick after the foul
    f.player.stepForward();
    QCOMPARE( f.record.plays, std::vector< std::string >( { "short" } ) );

    // back onto the foul: no whistle
    f.player.stepBackward();
    QCOMPARE( f.holder.currentIndex(), std::size_t( 5 ) );
    f.player.stepBackward();
    QCOMPARE( f.holder.currentIndex(), std::size_t( 4 ) );
    QCOMPARE( f.record.plays.size(), std::size_t( 1 ) );

    // a cycle, onto the goal
    f.player.goToCycle( 8 );
    QCOMPARE( f.holder.currentIndex(), std::size_t( 8 ) );
    QCOMPARE( f.record.plays.size(), std::size_t( 1 ) );

    // to the last and the first
    f.player.goToLast();
    f.player.goToFirst();
    QCOMPARE( f.record.plays.size(), std::size_t( 1 ) );

    // from the frame before the kick-off of the second half
    f.player.goToIndex( 11 );
    f.player.stepForward();
    QCOMPARE( f.record.plays, std::vector< std::string >( { "short", "long" } ) );
}

/*-------------------------------------------------------------------*/
/*!
  The loop playback from the last frame to the first: no whistle; on from
  the first frame, the kick-off of the game again.
*/
void
TestWhistle::autoLoop()
{
    Options::instance().setAutoLoopMode( true );

    Fixture f;
    QVERIFY( addFrame( f.holder, 0, "kick_off_l" ) );
    QVERIFY( addFrame( f.holder, 1, "play_on" ) );
    QVERIFY( addFrame( f.holder, 2, "time_over" ) );
    f.player.goToIndex( 1 );
    f.player.stepForward();
    QCOMPARE( f.record.plays, std::vector< std::string >( { "long" } ) );
    f.player.stepForward();
    QCOMPARE( f.holder.currentIndex(), std::size_t( 0 ) );
    QCOMPARE( f.record.plays.size(), std::size_t( 1 ) );
    f.player.stepForward();
    QCOMPARE( f.record.plays, std::vector< std::string >( { "long", "long" } ) );
}

/*-------------------------------------------------------------------*/
/*!
  Off: no whistle, and the sound is not made (not loaded). Turned on, the
  next change blows its whistle.
*/
void
TestWhistle::off()
{
    Options::instance().setWhistle( false );

    Fixture f;
    addGame( f.holder );
    f.player.goToFirst();
    for ( int i = 0; i < 9; ++i )
    {
        f.player.stepForward();
    }
    QCOMPARE( f.holder.currentIndex(), std::size_t( 9 ) );
    QVERIFY( f.record.plays.empty() );
    QVERIFY( ! f.whistle.hasSound() );

    Options::instance().setWhistle( true );
    f.player.stepForward();
    QCOMPARE( f.record.plays, std::vector< std::string >( { "long" } ) );
}

/*-------------------------------------------------------------------*/
/*!
  Live: each frame as it comes, the whistle at its change.
*/
void
TestWhistle::live()
{
    Fixture f;
    f.player.setLiveMode();
    QVERIFY( f.player.isLiveMode() );

    int time = 0;
    for ( const auto & frame : game() )
    {
        const std::size_t before = f.record.plays.size();
        QVERIFY( addFrame( f.holder, time++, frame.first ) );
        f.player.receivedFrame();
        QCOMPARE( f.holder.currentIndex(), f.holder.dispCont().size() - 1 );
        QCOMPARE( f.record.plays.size(), before + ( *frame.second ? 1 : 0 ) );

        // a message without a frame (e.g. the play mode, the team): no
        // more whistles
        f.player.receivedFrame();
        QCOMPARE( f.record.plays.size(), before + ( *frame.second ? 1 : 0 ) );
    }
    QCOMPARE( f.record.plays, gameWhistles( 0, game().size() - 1 ) );
}

/*-------------------------------------------------------------------*/
/*!
  Live: frames in a burst blow the strongest whistle of them, once; more
  than MAX_LIVE_STEP frames at once is a jump (no whistle).
*/
void
TestWhistle::liveBurst()
{
    Fixture f;
    f.player.setLiveMode();
    QVERIFY( addFrame( f.holder, 0, "play_on" ) );
    f.player.receivedFrame();

    // a foul, its free kick and a goal at once: the short whistle, once
    QVERIFY( addFrame( f.holder, 1, "foul_charge_l" ) );
    QVERIFY( addFrame( f.holder, 2, "free_kick_r" ) );
    QVERIFY( addFrame( f.holder, 3, "play_on" ) );
    QVERIFY( addFrame( f.holder, 4, "goal_l" ) );
    f.player.receivedFrame();
    QCOMPARE( f.record.plays, std::vector< std::string >( { "short" } ) );

    // the end of the game with others: three long
    QVERIFY( addFrame( f.holder, 5, "kick_off_r" ) );
    QVERIFY( addFrame( f.holder, 6, "play_on" ) );
    QVERIFY( addFrame( f.holder, 7, "time_over" ) );
    f.player.receivedFrame();
    QCOMPARE( f.record.plays, std::vector< std::string >( { "short", "long" } ) );
    f.whistle.stop(); // not the other two

    int time = 8;
    QVERIFY( addFrame( f.holder, time++, "play_on" ) );
    f.player.receivedFrame();
    QCOMPARE( f.record.plays.size(), std::size_t( 2 ) );

    // MAX_LIVE_STEP frames at once, and one more
    QCOMPARE( Whistle::MAX_LIVE_STEP, std::size_t( 20 ) );
    QVERIFY( addFrame( f.holder, time++, "foul_push_r" ) );
    for ( std::size_t i = 1; i < Whistle::MAX_LIVE_STEP; ++i )
    {
        QVERIFY( addFrame( f.holder, time++, "foul_push_r" ) );
    }
    f.player.receivedFrame();
    QCOMPARE( f.record.plays, std::vector< std::string >( { "short", "long", "short" } ) );

    QVERIFY( addFrame( f.holder, time++, "free_kick_l" ) );
    for ( std::size_t i = 0; i < Whistle::MAX_LIVE_STEP; ++i )
    {
        QVERIFY( addFrame( f.holder, time++, "play_on" ) );
    }
    f.player.receivedFrame();
    QCOMPARE( f.record.plays.size(), std::size_t( 3 ) );

    // and on from there
    QVERIFY( addFrame( f.holder, time++, "offside_r" ) );
    f.player.receivedFrame();
    QCOMPARE( f.record.plays, std::vector< std::string >( { "short", "long", "short", "short" } ) );
}

/*-------------------------------------------------------------------*/
/*!
  Live: when the oldest frames are dropped (DispHolder::trimLiveFrames()),
  the indexes move, and the frame shown before is still found.
*/
void
TestWhistle::liveTrimmed()
{
    Fixture f;
    f.player.setLiveMode();
    int time = 0;
    while ( f.holder.dispCont().size() < DispHolder::MAX_LIVE_FRAMES )
    {
        QVERIFY( addFrame( f.holder, time++, "play_on" ) );
    }
    f.player.receivedFrame();
    QCOMPARE( f.holder.currentIndex(), DispHolder::MAX_LIVE_FRAMES - 1 );

    QVERIFY( addFrame( f.holder, time++, "offside_l" ) );
    QVERIFY( f.holder.dispCont().size() < DispHolder::MAX_LIVE_FRAMES ); // trimmed
    f.player.receivedFrame();
    QCOMPARE( f.holder.currentIndex(), f.holder.dispCont().size() - 1 );
    QCOMPARE( f.record.plays, std::vector< std::string >( { "short" } ) );
}

/*-------------------------------------------------------------------*/
/*!
  The delayed live view: the whistle when the frame of the change is shown
  (the delay after it came), not when it comes.
*/
void
TestWhistle::delayedLive()
{
    Options::instance().setLiveDelay( true );
    Options::instance().setLiveDelaySeconds( 1.0 );

    Fixture f;
    setSimulatorStep( f.holder, 100 );
    QCOMPARE( f.player.liveDelayFrames(), std::size_t( 10 ) );

    int time = 0;
    for ( ; time <= 20; ++time )
    {
        QVERIFY( addFrame( f.holder, time, "play_on" ) );
    }
    f.player.setLiveMode();
    QVERIFY( f.player.isDelayedLive() );
    QCOMPARE( f.holder.currentIndex(), std::size_t( 10 ) );

    // the offside comes (time 21): shown 10 frames later
    QVERIFY( addFrame( f.holder, time++, "offside_l" ) );
    f.player.receivedFrame();
    QVERIFY( f.record.plays.empty() );
    for ( int i = 0; i < 9; ++i )
    {
        QVERIFY( addFrame( f.holder, time++, "offside_l" ) );
        f.player.receivedFrame();
    }
    QCOMPARE( f.holder.currentDisp()->show_.time_, rcss::rcg::UInt32( 20 ) );
    QVERIFY( f.record.plays.empty() );

    QVERIFY( addFrame( f.holder, time++, "offside_l" ) );
    f.player.receivedFrame();
    QCOMPARE( f.holder.currentDisp()->show_.time_, rcss::rcg::UInt32( 21 ) );
    QCOMPARE( f.record.plays, std::vector< std::string >( { "short" } ) );
}

/*-------------------------------------------------------------------*/
/*!
  A new log (or a new connection): no whistle at its first frame shown,
  though the frame shown before was in another mode (only its kick-off when
  the play starts); clear() stops the whistle.
*/
void
TestWhistle::reopened()
{
    Fixture f;
    addGame( f.holder );
    f.player.goToIndex( 15 );
    f.player.stepForward(); // time_over: three long
    QCOMPARE( f.record.plays, std::vector< std::string >( { "long" } ) );

    // as MainWindow opens a log
    f.holder.clear();
    f.whistle.clear();
    QCOMPARE( f.record.stops, 2 ); // the blow, and clear()
    QVERIFY( addFrame( f.holder, 0, "kick_off_l" ) );
    QVERIFY( addFrame( f.holder, 1, "play_on" ) );
    QVERIFY( addFrame( f.holder, 2, "back_pass_r" ) );
    f.player.goToFirst();
    QCOMPARE( f.record.plays, std::vector< std::string >( { "long" } ) );
    QTest::qWait( Whistle::LONG_REPEAT_INTERVAL + 200 );
    QCOMPARE( f.record.plays, std::vector< std::string >( { "long" } ) ); // not the other two
    f.player.stepForward(); // the kick-off of the new log
    QCOMPARE( f.record.plays, std::vector< std::string >( { "long", "long" } ) );
    f.player.stepForward();
    QCOMPARE( f.record.plays, std::vector< std::string >( { "long", "long", "short" } ) );

    // without clear(), the frames of the new log do not follow the one
    // shown before either
    f.holder.clear();
    QVERIFY( addFrame( f.holder, 0, "play_on" ) );
    QVERIFY( addFrame( f.holder, 1, "play_on" ) );
    f.player.goToIndex( 1 );
    QCOMPARE( f.record.plays.size(), std::size_t( 3 ) );
}

/*-------------------------------------------------------------------*/
/*!
  The two and the three long whistles: the long one again every
  LONG_REPEAT_INTERVAL.
*/
void
TestWhistle::longWhistles()
{
    QCOMPARE( Whistle::LONG_REPEAT_INTERVAL, 1300 );

    Fixture f;
    QElapsedTimer clock;
    clock.start();
    f.whistle.blow( WhistleKind::LongThrice );
    QCOMPARE( f.record.plays, std::vector< std::string >( { "long" } ) );
    QTRY_COMPARE_WITH_TIMEOUT( f.record.plays.size(), std::size_t( 2 ), 3000 );
    QVERIFY( clock.elapsed() >= Whistle::LONG_REPEAT_INTERVAL - 50 );
    QTRY_COMPARE_WITH_TIMEOUT( f.record.plays.size(), std::size_t( 3 ), 3000 );
    QVERIFY( clock.elapsed() >= 2 * Whistle::LONG_REPEAT_INTERVAL - 50 );
    QCOMPARE( f.record.plays, std::vector< std::string >( { "long", "long", "long" } ) );

    // two, and no more
    f.record.plays.clear();
    f.whistle.blow( WhistleKind::LongTwice );
    QTRY_COMPARE_WITH_TIMEOUT( f.record.plays.size(), std::size_t( 2 ), 3000 );
    QTest::qWait( Whistle::LONG_REPEAT_INTERVAL + 200 );
    QCOMPARE( f.record.plays, std::vector< std::string >( { "long", "long" } ) );

    f.record.plays.clear();
    f.whistle.blow( WhistleKind::Long );
    f.whistle.blow( WhistleKind::Short );
    f.whistle.blow( WhistleKind::None );
    QCOMPARE( f.record.plays, std::vector< std::string >( { "long", "short" } ) );
}

/*-------------------------------------------------------------------*/
/*!
  A whistle stops the one playing and the long ones still to blow (a log
  played fast); so does stop().
*/
void
TestWhistle::stopped()
{
    Fixture f;
    f.whistle.blow( WhistleKind::LongThrice );
    const int stops = f.record.stops;
    f.whistle.blow( WhistleKind::Short );
    QCOMPARE( f.record.stops, stops + 1 );
    QTest::qWait( Whistle::LONG_REPEAT_INTERVAL + 200 );
    QCOMPARE( f.record.plays, std::vector< std::string >( { "long", "short" } ) );

    f.record.plays.clear();
    f.whistle.blow( WhistleKind::LongTwice );
    f.whistle.stop();
    QCOMPARE( f.record.stops, stops + 3 );
    QTest::qWait( Whistle::LONG_REPEAT_INTERVAL + 200 );
    QCOMPARE( f.record.plays, std::vector< std::string >( { "long" } ) );
}

/*-------------------------------------------------------------------*/
/*!
  The audio output is kept open while the whistle is on (so that an HDMI
  output is not suspended between the whistles): from the first frame
  shown, or when the option is turned on (updateHold()); closed when it is
  turned off. Whistles, stopping them and clear() leave it.
*/
void
TestWhistle::held()
{
    Fixture f;
    addGame( f.holder );

    // off: not made, not held
    Options::instance().setWhistle( false );
    f.whistle.updateHold();
    f.player.goToFirst();
    QVERIFY( ! f.whistle.hasSound() );
    QVERIFY( ! f.record.held );

    // turned on
    Options::instance().setWhistle( true );
    f.whistle.updateHold();
    QVERIFY( f.whistle.hasSound() );
    QVERIFY( f.record.held );
    f.player.stepForward(); // the kick-off
    f.whistle.stop();
    f.whistle.clear();
    QCOMPARE( f.record.plays.size(), std::size_t( 1 ) );
    QVERIFY( f.record.held );

    // turned off, and on by a frame shown (the option on from the start)
    Options::instance().setWhistle( false );
    f.whistle.updateHold();
    QVERIFY( ! f.record.held );
    f.player.stepForward();
    QVERIFY( ! f.record.held );
    Options::instance().setWhistle( true );
    f.player.stepForward();
    QVERIFY( f.record.held );
}

/*-------------------------------------------------------------------*/
/*!
  The volume of the options when a whistle is played (also the repeats).
*/
void
TestWhistle::volume()
{
    Fixture f;
    Options::instance().setWhistleVolume( 80 );
    f.whistle.blow( WhistleKind::Short );
    Options::instance().setWhistleVolume( 20 );
    f.whistle.blow( WhistleKind::LongTwice );
    Options::instance().setWhistleVolume( 0 );
    QTRY_COMPARE_WITH_TIMEOUT( f.record.plays.size(), std::size_t( 3 ), 3000 );
    QCOMPARE( f.record.volumes, std::vector< int >( { 80, 20, 0 } ) );
}

/*-------------------------------------------------------------------*/
/*!
  WhistleMixer::readWav(): the samples of the WAV files of
  tools/make_whistle.py (the short whistle 0.10 s, the long one 0.95 s at
  44.1 kHz); none for a file that is not one.
*/
void
TestWhistle::wav()
{
    const QString dir = QString( TEST_SOURCE_DIR ) + "/resources/sounds/";
    const std::vector< qint16 > s = WhistleMixer::readWav( dir + "whistle_short.wav" );
    const std::vector< qint16 > l = WhistleMixer::readWav( dir + "whistle_long.wav" );
    QCOMPARE( s.size(), std::size_t( 4410 ) );
    QCOMPARE( l.size(), std::size_t( 41895 ) );
    int peak = 0;
    for ( qint16 v : s ) peak = std::max( peak, std::abs( int( v ) ) );
    QVERIFY( peak > 20000 );

    QVERIFY( WhistleMixer::readWav( dir + "none.wav" ).empty() );
    QVERIFY( WhistleMixer::readWav( dir + "sounds.qrc" ).empty() );

    // another format: stereo
    QTemporaryDir tmp;
    QFile in( dir + "whistle_short.wav" );
    QVERIFY( in.open( QIODevice::ReadOnly ) );
    QByteArray bytes = in.readAll();
    bytes[22] = 2;
    QFile out( tmp.filePath( "stereo.wav" ) );
    QVERIFY( out.open( QIODevice::WriteOnly ) );
    out.write( bytes );
    out.close();
    QVERIFY( WhistleMixer::readWav( out.fileName() ).empty() );
}

/*-------------------------------------------------------------------*/
/*!
  WhistleMixer: silence, and a whistle from its start at its gain when one
  is played; another one instead of it; silence after its end or stop().
  The output never runs out of data.
*/
void
TestWhistle::mixer()
{
    WhistleMixer mixer;
    QVERIFY( mixer.isOpen() );
    QVERIFY( mixer.isSequential() );
    QVERIFY( mixer.bytesAvailable() > 0 );
    mixer.setWhistles( { 1000, 2000, 3000 }, { -4000, 4000, -4000, 4000 } );

    const auto read = [&mixer]( const int count ) {
        const QByteArray bytes = mixer.read( 2 * count );
        std::vector< int > samples;
        for ( int i = 0; i + 1 < bytes.size(); i += 2 )
        {
            qint16 v;
            std::memcpy( &v, bytes.constData() + i, 2 );
            samples.push_back( v );
        }
        return samples;
    };

    QCOMPARE( read( 4 ), std::vector< int >( { 0, 0, 0, 0 } ) );
    QVERIFY( ! mixer.isPlaying() );

    mixer.play( false, 0.5 );
    QVERIFY( mixer.isPlaying() );
    QCOMPARE( read( 2 ), std::vector< int >( { 500, 1000 } ) );
    QCOMPARE( read( 4 ), std::vector< int >( { 1500, 0, 0, 0 } ) );
    QVERIFY( ! mixer.isPlaying() );

    // the long one instead, from its start
    mixer.play( false, 1.0 );
    QCOMPARE( read( 1 ), std::vector< int >( { 1000 } ) );
    mixer.play( true, 1.0 );
    QCOMPARE( read( 5 ), std::vector< int >( { -4000, 4000, -4000, 4000, 0 } ) );

    // stopped
    mixer.play( true, 0.25 );
    QCOMPARE( read( 1 ), std::vector< int >( { -1000 } ) );
    mixer.stop();
    QVERIFY( ! mixer.isPlaying() );
    QCOMPARE( read( 3 ), std::vector< int >( { 0, 0, 0 } ) );

    // the gain within 0 to 1
    mixer.play( false, 3.0 );
    QCOMPARE( read( 1 ), std::vector< int >( { 1000 } ) );
    mixer.play( false, -1.0 );
    QCOMPARE( read( 1 ), std::vector< int >( { 0 } ) );

    // a large read
    QCOMPARE( mixer.read( 100000 ).size(), 100000 );
}

/*-------------------------------------------------------------------*/
/*!
  The real sound output: with Qt Multimedia, one audio output with a
  WhistleMixer on the sounds in the resources (played silently here);
  without, none.
*/
void
TestWhistle::sound()
{
    std::unique_ptr< WhistleSound > sound = Whistle::createSound();
#ifdef HAVE_QT_MULTIMEDIA
    QVERIFY( Whistle::isAvailable() );
    QVERIFY( sound );
    QVERIFY( QFile::exists( ":/sounds/whistle_short.wav" ) );
    QVERIFY( QFile::exists( ":/sounds/whistle_long.wav" ) );
    sound->play( false, 0 ); // opens the output
    sound->play( true, 0 );
    sound->stop();
    sound->hold( true );
    sound->hold( true );
    sound->hold( false );
    sound->play( false, 0 );
    sound->hold( false );
#else
    QVERIFY( ! Whistle::isAvailable() );
    QVERIFY( ! sound );
#endif
    sound.reset();

    // with no sound output, nothing is played
    DispHolder holder;
    LogPlayer player( holder, nullptr );
    Whistle whistle( holder, player, Whistle::SoundFactory(), nullptr );
    whistle.blow( WhistleKind::LongThrice );
    QVERIFY( ! whistle.hasSound() );
    whistle.stop();
}

/*-------------------------------------------------------------------*/
/*!
  View > Referee's Whistle: checked as the options; chosen, it sets the
  option and saves it now, and the settings dialog follows. Disabled
  without Qt Multimedia.
*/
void
TestWhistle::menu()
{
    Options::instance().setWhistle( false );
    Options::instance().writeWhistle();

    MainWindow window;
    QAction * act = window.findChild< QAction * >( "toggle_whistle" );
    QVERIFY( act );
    QVERIFY( act->isCheckable() );
    QVERIFY( ! act->isChecked() );
    QCOMPARE( act->isEnabled(), Whistle::isAvailable() );
    if ( ! Whistle::isAvailable() )
    {
        QVERIFY( act->statusTip().contains( "Qt Multimedia" ) );
        return;
    }

    ConfigDialog * dialog = window.findChild< ConfigDialog * >();
    QVERIFY( dialog );
    QCheckBox * cb = dialog->findChild< QCheckBox * >( "whistle" );
    QVERIFY( cb );
    QVERIFY( ! cb->isChecked() );

    act->trigger();
    QVERIFY( Options::instance().whistle() );
    QCOMPARE( savedValue( "Sound/whistle" ).toBool(), true );
    QVERIFY( cb->isChecked() );

    // the settings dialog's check box: the action follows
    cb->click();
    QVERIFY( ! Options::instance().whistle() );
    QCOMPARE( savedValue( "Sound/whistle" ).toBool(), false );
    QVERIFY( ! act->isChecked() );

    cb->click();
    QVERIFY( act->isChecked() );
    act->trigger();
    QVERIFY( ! Options::instance().whistle() );
    QVERIFY( ! cb->isChecked() );
}

/*-------------------------------------------------------------------*/
/*!
  When rcssmonitor starts (MainWindow::init()) with the whistle on, the
  audio output is opened at once, before a log is opened (a moment later)
  or a frame is shown, so that a display's speakers on HDMI are awake by
  the first whistle. Not with the whistle off.
*/
void
TestWhistle::startup()
{
    for ( const bool on : { false, true } )
    {
        Options::instance().setWhistle( on );
        MainWindow window;
        Whistle * whistle = window.findChild< Whistle * >();
        QVERIFY( whistle );
        QVERIFY( ! whistle->hasSound() );

        // a game log, not to connect to a server
        Options::instance().setGameLogFile( std::string( TEST_DATA_DIR ) + "/head_v5.rcg" );
        window.show();
        window.init();
        QCOMPARE( whistle->hasSound(), on && Whistle::isAvailable() );

        // the log opened (init() opens it a moment later; not while the
        // window is destroyed). The output is closed before the widgets are
        // destroyed (releaseSound(): closing it runs the event loop, which
        // crashed in the widgets' destructor)
        QLabel * frame = window.findChild< QLabel * >( "frame_label" );
        QVERIFY( frame );
        QTRY_VERIFY( ! frame->text().isEmpty() );
        Options::instance().setGameLogFile( std::string() );

        // an event still to deliver to the window when it is destroyed (as
        // after show())
        QCoreApplication::postEvent( &window, new QEvent( QEvent::PolishRequest ) );
    }
    Options::instance().setWhistle( true );
}

/*-------------------------------------------------------------------*/
/*!
  The Sound group of the Show tab: the volume (saved when the slider is let
  go, not while it is dragged), its number, and Test (only while the
  whistle is on: the output is kept open then). Disabled without Qt
  Multimedia.
*/
void
TestWhistle::settingsDialog()
{
    Options & opt = Options::instance();
    opt.setWhistle( false );
    opt.setWhistleVolume( 40 );
    opt.writeWhistleVolume();

    DispHolder holder;
    ConfigDialog dialog( nullptr, holder );
    QGroupBox * group = dialog.findChild< QGroupBox * >( "sound" );
    QSlider * slider = dialog.findChild< QSlider * >( "whistle_volume" );
    QLabel * number = dialog.findChild< QLabel * >( "whistle_volume_value" );
    QPushButton * test = dialog.findChild< QPushButton * >( "whistle_test" );
    QVERIFY( group );
    QVERIFY( slider );
    QVERIFY( number );
    QVERIFY( test );
    QCOMPARE( group->isEnabled(), Whistle::isAvailable() );
    QCOMPARE( slider->minimum(), 0 );
    QCOMPARE( slider->maximum(), 100 );
    QCOMPARE( slider->value(), 40 );
    QCOMPARE( number->text(), QString( "40" ) );

    // by the keys: saved at once
    slider->setValue( 70 );
    QCOMPARE( opt.whistleVolume(), 70 );
    QCOMPARE( number->text(), QString( "70" ) );
    QCOMPARE( savedValue( "Sound/whistle_volume" ).toInt(), 70 );

    // dragged: saved when let go
    slider->setSliderDown( true );
    slider->setValue( 60 );
    QCOMPARE( opt.whistleVolume(), 60 );
    QCOMPARE( savedValue( "Sound/whistle_volume" ).toInt(), 70 );
    slider->setSliderDown( false );
    QCOMPARE( savedValue( "Sound/whistle_volume" ).toInt(), 60 );

    // the options changed elsewhere (the menu, a dialog shown again)
    opt.setWhistleVolume( 25 );
    dialog.updateWhistle();
    QCOMPARE( slider->value(), 25 );
    QCOMPARE( number->text(), QString( "25" ) );
    QCOMPARE( savedValue( "Sound/whistle_volume" ).toInt(), 60 ); // not by updating

    QSignalSpy tried( &dialog, SIGNAL( whistleTried() ) );
    QSignalSpy changed( &dialog, SIGNAL( whistleChanged() ) );
    QVERIFY( ! test->isEnabled() ); // the whistle off
    if ( Whistle::isAvailable() )
    {
        QCheckBox * cb = dialog.findChild< QCheckBox * >( "whistle" );
        cb->click();
        QCOMPARE( changed.count(), 1 );
        QVERIFY( opt.whistle() );
        QVERIFY( test->isEnabled() );
        test->click();
        QCOMPARE( tried.count(), 1 );
        cb->click();
        QVERIFY( ! opt.whistle() );
        QVERIFY( ! test->isEnabled() );

        // turned on in the menu (updateWhistle())
        opt.setWhistle( true );
        dialog.updateWhistle();
        QVERIFY( test->isEnabled() );
        opt.setWhistle( false );
        dialog.updateWhistle();
        QVERIFY( ! test->isEnabled() );
    }
    else
    {
        QVERIFY( test->toolTip().contains( "Qt Multimedia" ) );
    }

    opt.setWhistleVolume( Options::DEFAULT_WHISTLE_VOLUME );
    opt.writeWhistleVolume();
    opt.setWhistle( false );
    opt.writeWhistle();
}

/*-------------------------------------------------------------------*/
void
TestWhistle::commandLine_data()
{
    QTest::addColumn< QStringList >( "args" );
    QTest::addColumn< QString >( "used" );
    QTest::addColumn< bool >( "warned" );

    QTest::newRow( "none" ) << QStringList() << "off 50" << false;
    QTest::newRow( "on" ) << QStringList( { "--whistle", "on" } ) << "on 50" << false;
    QTest::newRow( "volume" ) << QStringList( { "--whistle", "on", "--whistle-volume", "80" } ) << "on 80" << false;
    QTest::newRow( "out of range" ) << QStringList( { "--whistle-volume", "101" } ) << "off 50" << true;
    QTest::newRow( "not a number" ) << QStringList( { "--whistle-volume", "loud" } ) << "off 50" << true;
}

/*-------------------------------------------------------------------*/
/*!
  --whistle and --whistle-volume set the options of this run, and are not
  saved (the settings file keeps its values). In a child process (main()).
*/
void
TestWhistle::commandLine()
{
    QFETCH( QStringList, args );
    QFETCH( QString, used );
    QFETCH( bool, warned );

    QTemporaryDir home;
    QVERIFY( home.isValid() );
    const QString conf = home.filePath( ".rcssmonitor.conf" );

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert( CHILD_ENV, "1" );
    env.insert( "HOME", home.path() );
    env.remove( "RCSSMONITOR_TEST_HOME" ); // the settings file kept (fresh_settings.cpp)
    env.insert( "QT_QPA_PLATFORM", "offscreen" );
    QProcess child;
    child.setProcessEnvironment( env );
    child.start( QCoreApplication::applicationFilePath(), args );
    QVERIFY( child.waitForFinished( 30000 ) );
    const QString out = QString::fromLocal8Bit( child.readAllStandardOutput() );
    const QString err = QString::fromLocal8Bit( child.readAllStandardError() );
    QVERIFY2( child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0,
              qPrintable( out + err ) );

    QString printed;
    for ( const QString & line : out.split( '\n' ) )
    {
        if ( line.startsWith( CHILD_PREFIX ) ) printed = line.mid( CHILD_PREFIX.size() ).trimmed();
    }
    QCOMPARE( printed, used );
    QCOMPARE( err.contains( "whistle-volume" ), warned );

    // the file made with the defaults, not the command line's
    QVERIFY( QFile::exists( conf ) );
    QSettings settings( conf, QSettings::IniFormat );
    QCOMPARE( settings.value( "Sound/whistle" ).toBool(), false );
    QCOMPARE( settings.value( "Sound/whistle_volume" ).toInt(), 50 );
}

/*-------------------------------------------------------------------*/
/*!
  The test, or the child of commandLine(): it parses its command line as
  rcssmonitor does, prints the whistle's options, and ends (Options writes
  the settings file).
*/
int
main( int argc,
      char ** argv )
{
    if ( qEnvironmentVariableIsSet( CHILD_ENV ) )
    {
        QApplication app( argc, argv );
        if ( ! Options::instance().parseCmdLine( argc, argv ) )
        {
            return 1;
        }
        const Options & opt = Options::instance();
        std::cout << CHILD_PREFIX.toStdString()
                  << ( opt.whistle() ? "on" : "off" ) << ' ' << opt.whistleVolume()
                  << std::endl;
        return 0;
    }

    QApplication app( argc, argv );
    TestWhistle test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec( &test, argc, argv );
}

#include "test_whistle.moc"
