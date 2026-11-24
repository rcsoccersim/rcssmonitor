// -*-c++-*-

/*!
  \file test_whistle_rules.cpp
  \brief unit test of the referee's whistle at the changes of the play mode,
  and of its sounds
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

#include <QtTest>

#include "whistle_rules.h"

#include <QFile>

#include <cstdlib>
#include <utility>
#include <vector>

using namespace rcss::rcg;

Q_DECLARE_METATYPE( rcss::rcg::PlayMode )
Q_DECLARE_METATYPE( WhistleKind )

namespace {

const char *
kindName( const WhistleKind kind )
{
    switch ( kind ) {
    case WhistleKind::None: return "none";
    case WhistleKind::Short: return "short";
    case WhistleKind::Long: return "long";
    case WhistleKind::LongTwice: return "long x2";
    case WhistleKind::LongThrice: return "long x3";
    }
    return "?";
}

//! a little-endian integer in a byte array
quint32
le( const QByteArray & bytes,
    const int pos,
    const int size )
{
    quint32 v = 0;
    for ( int i = size - 1; i >= 0; --i )
    {
        v = ( v << 8 ) | static_cast< quint8 >( bytes.at( pos + i ) );
    }
    return v;
}

}

/*!
  \brief unit test of the whistle rules
*/
class TestWhistleRules
    : public QObject {
    Q_OBJECT

private slots:

    void table_data();
    void table();
    void sequence();
    void firstFrame();
    void stronger();
    void sounds_data();
    void sounds();
};

/*-------------------------------------------------------------------*/
void
TestWhistleRules::table_data()
{
    QTest::addColumn< PlayMode >( "prev" );
    QTest::addColumn< PlayMode >( "next" );
    QTest::addColumn< int >( "time" );
    QTest::addColumn< WhistleKind >( "expected" );

    // the kick-off: long, once (U15); also a new half in a log, right after
    // the play without before_kick_off (U16)
    QTest::newRow( "goal -> kick off" ) << PM_AfterGoal_Left << PM_KickOff_Right << 6714 << WhistleKind::Long;
    QTest::newRow( "before kick off -> kick off" ) << PM_BeforeKickOff << PM_KickOff_Left << 0 << WhistleKind::Long;
    QTest::newRow( "before kick off (half) -> kick off" ) << PM_BeforeKickOff << PM_KickOff_Right << 3000 << WhistleKind::Long;
    QTest::newRow( "pause -> kick off" ) << PM_Pause << PM_KickOff_Left << 1200 << WhistleKind::Long;
    QTest::newRow( "play on -> kick off (new half)" ) << PM_PlayOn << PM_KickOff_Right << 3001 << WhistleKind::Long;
    QTest::newRow( "kick in -> kick off (new half)" ) << PM_KickIn_Left << PM_KickOff_Right << 3001 << WhistleKind::Long;
    // the end of a half in live, and of the game
    QTest::newRow( "play on -> before kick off (half)" ) << PM_PlayOn << PM_BeforeKickOff << 3000 << WhistleKind::LongTwice;
    QTest::newRow( "before kick off at 0" ) << PM_PlayOn << PM_BeforeKickOff << 0 << WhistleKind::None;
    QTest::newRow( "first half over" ) << PM_PlayOn << PM_FirstHalfOver << 3000 << WhistleKind::LongTwice;
    QTest::newRow( "time over" ) << PM_PlayOn << PM_TimeOver << 6000 << WhistleKind::LongThrice;
    QTest::newRow( "time over after a penalty" ) << PM_PenaltyScore_Left << PM_TimeOver << 6000 << WhistleKind::LongThrice;
    // a goal: none (a clear goal needs none; U18)
    QTest::newRow( "goal l" ) << PM_PlayOn << PM_AfterGoal_Left << 6713 << WhistleKind::None;
    QTest::newRow( "goal r" ) << PM_PlayOn << PM_AfterGoal_Right << 2711 << WhistleKind::None;
    // the stops for a free kick, and the restarts after them
    QTest::newRow( "foul charge" ) << PM_PlayOn << PM_Foul_Charge_Left << 1390 << WhistleKind::Short;
    QTest::newRow( "foul push" ) << PM_PlayOn << PM_Foul_Push_Right << 100 << WhistleKind::Short;
    QTest::newRow( "multiple attack" ) << PM_PlayOn << PM_Foul_MultipleAttacker_Left << 100 << WhistleKind::Short;
    QTest::newRow( "ball out" ) << PM_PlayOn << PM_Foul_BallOut_Right << 100 << WhistleKind::Short;
    QTest::newRow( "offside" ) << PM_PlayOn << PM_OffSide_Right << 789 << WhistleKind::Short;
    QTest::newRow( "back pass" ) << PM_PlayOn << PM_Back_Pass_Left << 100 << WhistleKind::Short;
    QTest::newRow( "free kick fault" ) << PM_FreeKick_Left << PM_Free_Kick_Fault_Left << 100 << WhistleKind::Short;
    QTest::newRow( "catch fault" ) << PM_PlayOn << PM_CatchFault_Right << 100 << WhistleKind::Short;
    QTest::newRow( "illegal defense" ) << PM_PlayOn << PM_Illegal_Defense_Left << 100 << WhistleKind::Short;
    QTest::newRow( "foul -> free kick" ) << PM_Foul_Charge_Left << PM_FreeKick_Right << 1391 << WhistleKind::Short;
    QTest::newRow( "offside -> free kick" ) << PM_OffSide_Right << PM_FreeKick_Left << 790 << WhistleKind::Short;
    QTest::newRow( "foul -> indirect free kick" ) << PM_Foul_Charge_Left << PM_IndFreeKick_Right << 5707 << WhistleKind::Short;
    QTest::newRow( "back pass -> indirect free kick" ) << PM_Back_Pass_Left << PM_IndFreeKick_Right << 101 << WhistleKind::Short;
    // the penalty shootout: only when the kicker may kick
    QTest::newRow( "penalty setup" ) << PM_PlayOn << PM_PenaltySetup_Left << 6000 << WhistleKind::None;
    QTest::newRow( "penalty ready" ) << PM_PenaltySetup_Left << PM_PenaltyReady_Left << 6000 << WhistleKind::Short;
    QTest::newRow( "penalty taken" ) << PM_PenaltyReady_Left << PM_PenaltyTaken_Left << 6000 << WhistleKind::None;
    QTest::newRow( "penalty score" ) << PM_PenaltyTaken_Left << PM_PenaltyScore_Left << 6000 << WhistleKind::None;
    QTest::newRow( "penalty miss" ) << PM_PenaltyTaken_Right << PM_PenaltyMiss_Right << 6000 << WhistleKind::None;
    // no whistle
    QTest::newRow( "free kick after a catch" ) << PM_PlayOn << PM_FreeKick_Left << 300 << WhistleKind::None;
    QTest::newRow( "indirect free kick from play" ) << PM_PlayOn << PM_IndFreeKick_Left << 300 << WhistleKind::None;
    QTest::newRow( "kick in" ) << PM_PlayOn << PM_KickIn_Right << 980 << WhistleKind::None;
    QTest::newRow( "corner kick" ) << PM_PlayOn << PM_CornerKick_Right << 3814 << WhistleKind::None;
    QTest::newRow( "goal kick" ) << PM_PlayOn << PM_GoalKick_Left << 581 << WhistleKind::None;
    QTest::newRow( "drop ball" ) << PM_PlayOn << PM_Drop_Ball << 300 << WhistleKind::None;
    QTest::newRow( "play on" ) << PM_KickOff_Left << PM_PlayOn << 4 << WhistleKind::None;
    QTest::newRow( "play on after a free kick" ) << PM_FreeKick_Right << PM_PlayOn << 1460 << WhistleKind::None;
    QTest::newRow( "pause" ) << PM_PlayOn << PM_Pause << 300 << WhistleKind::None;
    QTest::newRow( "human judge" ) << PM_PlayOn << PM_Human << 300 << WhistleKind::None;
    QTest::newRow( "penalty kick" ) << PM_PlayOn << PM_PK_Left << 300 << WhistleKind::None;
    // no change, an unknown mode before, a stop after a stop
    QTest::newRow( "same mode" ) << PM_Foul_Charge_Left << PM_Foul_Charge_Left << 1390 << WhistleKind::None;
    QTest::newRow( "unknown before" ) << PM_Null << PM_KickOff_Left << 1 << WhistleKind::None;
    QTest::newRow( "stop after a stop" ) << PM_Foul_Charge_Left << PM_OffSide_Left << 100 << WhistleKind::None;
}

/*-------------------------------------------------------------------*/
/*!
  The table of docs/plan/2026-11-22-00_v20-features.md C.3.4.
*/
void
TestWhistleRules::table()
{
    QFETCH( PlayMode, prev );
    QFETCH( PlayMode, next );
    QFETCH( int, time );
    QFETCH( WhistleKind, expected );

    const WhistleKind kind = WhistleRules::whistle( prev, next, time );
    QVERIFY2( kind == expected,
              qPrintable( QString( "%1 (expected %2)" ).arg( kindName( kind ), kindName( expected ) ) ) );
}

/*-------------------------------------------------------------------*/
/*!
  The changes of the modes of a game log, as the frames come one by one:
  the modes of the frames of stopped time stay, so each change whistles
  once (the stop and the restart of a foul about 3 seconds apart).
*/
void
TestWhistleRules::sequence()
{
    // (time, mode) of the frames, after a real log (202207160432-HELIOS2022_1-vs-CYRUS_0)
    std::vector< std::pair< int, PlayMode > > frames;
    const auto add = [&]( const int time, const PlayMode mode, const int count ) {
        for ( int i = 0; i < count; ++i ) frames.emplace_back( time, mode );
    };
    add( 1, PM_KickOff_Left, 1 );
    add( 4, PM_PlayOn, 3 );
    add( 1390, PM_Foul_Charge_Left, 31 ); // the stopped time
    add( 1391, PM_FreeKick_Right, 2 );
    add( 1460, PM_PlayOn, 2 );
    add( 2999, PM_PlayOn, 1 );
    add( 3001, PM_KickOff_Right, 2 ); // the second half, no before_kick_off in the log
    add( 3091, PM_PlayOn, 2 );
    add( 6713, PM_AfterGoal_Left, 51 );
    add( 6714, PM_KickOff_Right, 2 );
    add( 6804, PM_PlayOn, 2 );
    add( 8000, PM_TimeOver, 1 );

    std::vector< WhistleKind > whistles;
    for ( std::size_t i = 1; i < frames.size(); ++i )
    {
        const WhistleKind k = WhistleRules::whistle( frames[i - 1].second, frames[i].second, frames[i].first );
        if ( k != WhistleKind::None ) whistles.push_back( k );
    }

    const std::vector< WhistleKind > expected = {
        WhistleKind::Short, // the foul
        WhistleKind::Short, // the free kick after it
        WhistleKind::Long, // the kick-off of the second half (the end of the first half in the log)
        WhistleKind::Long, // the kick-off after the goal (none at the goal)
        WhistleKind::LongThrice, // the end of the game
    };
    QCOMPARE( whistles.size(), expected.size() );
    for ( std::size_t i = 0; i < expected.size(); ++i )
    {
        QVERIFY2( whistles[i] == expected[i], qPrintable( QString( "%1: %2" ).arg( i ).arg( kindName( whistles[i] ) ) ) );
    }
}

/*-------------------------------------------------------------------*/
/*!
  The play from the first frame of a log: the kick-off's whistle if the log
  starts with one (the logs start with kick_off at the time 1).
*/
void
TestWhistleRules::firstFrame()
{
    QVERIFY( WhistleRules::atFirstFrame( PM_KickOff_Left ) == WhistleKind::Long );
    QVERIFY( WhistleRules::atFirstFrame( PM_KickOff_Right ) == WhistleKind::Long );
    QVERIFY( WhistleRules::atFirstFrame( PM_BeforeKickOff ) == WhistleKind::None );
    QVERIFY( WhistleRules::atFirstFrame( PM_PlayOn ) == WhistleKind::None );
    QVERIFY( WhistleRules::atFirstFrame( PM_Foul_Charge_Left ) == WhistleKind::None );
    QVERIFY( WhistleRules::atFirstFrame( PM_Null ) == WhistleKind::None );
}

/*-------------------------------------------------------------------*/
void
TestWhistleRules::stronger()
{
    QVERIFY( WhistleRules::stronger( WhistleKind::Short, WhistleKind::Long ) == WhistleKind::Long );
    QVERIFY( WhistleRules::stronger( WhistleKind::LongThrice, WhistleKind::LongTwice ) == WhistleKind::LongThrice );
    QVERIFY( WhistleRules::stronger( WhistleKind::None, WhistleKind::Short ) == WhistleKind::Short );
    QVERIFY( WhistleRules::stronger( WhistleKind::None, WhistleKind::None ) == WhistleKind::None );
}

/*-------------------------------------------------------------------*/
void
TestWhistleRules::sounds_data()
{
    QTest::addColumn< QString >( "name" );
    QTest::addColumn< double >( "seconds" );
    QTest::newRow( "short" ) << "whistle_short" << 0.10;
    QTest::newRow( "long" ) << "whistle_long" << 0.95;
}

/*-------------------------------------------------------------------*/
/*!
  The sounds made by tools/make_whistle.py: WAV files of 16 bit mono
  PCM at 44.1 kHz, of their lengths, not silent, and listed in
  resources/sounds/sounds.qrc.
*/
void
TestWhistleRules::sounds()
{
    QFETCH( QString, name );
    QFETCH( double, seconds );

    QFile file( QString( TEST_SOURCE_DIR ) + "/resources/sounds/" + name + ".wav" );
    QVERIFY( file.open( QIODevice::ReadOnly ) );
    const QByteArray wav = file.readAll();
    QVERIFY( wav.size() > 44 );
    QCOMPARE( wav.mid( 0, 4 ), QByteArray( "RIFF" ) );
    QCOMPARE( wav.mid( 8, 4 ), QByteArray( "WAVE" ) );
    QCOMPARE( wav.mid( 12, 4 ), QByteArray( "fmt " ) );
    QCOMPARE( le( wav, 20, 2 ), 1u ); // PCM
    QCOMPARE( le( wav, 22, 2 ), 1u ); // mono
    QCOMPARE( le( wav, 24, 4 ), 44100u );
    QCOMPARE( le( wav, 34, 2 ), 16u );
    QCOMPARE( wav.mid( 36, 4 ), QByteArray( "data" ) );
    const quint32 bytes = le( wav, 40, 4 );
    QCOMPARE( int( bytes ), wav.size() - 44 );
    QVERIFY( std::abs( bytes / 2 / 44100.0 - seconds ) < 0.01 );

    int peak = 0;
    for ( quint32 i = 0; i < bytes; i += 2 )
    {
        peak = std::max( peak, std::abs( static_cast< qint16 >( le( wav, 44 + i, 2 ) ) ) );
    }
    QVERIFY2( peak > 20000, qPrintable( QString::number( peak ) ) );

    QFile qrc( QString( TEST_SOURCE_DIR ) + "/resources/sounds/sounds.qrc" );
    QVERIFY( qrc.open( QIODevice::ReadOnly ) );
    QVERIFY( qrc.readAll().contains( QString( "<file>%1.wav</file>" ).arg( name ).toUtf8() ) );
}

QTEST_GUILESS_MAIN( TestWhistleRules )
#include "test_whistle_rules.moc"
