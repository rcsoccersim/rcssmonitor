// -*-c++-*-

/*!
  \file test_log_filter.cpp
  \brief unit test of LogFilter: the filter of the list of game logs
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

#include "log_file_name.h"
#include "log_filter.h"

/*
  docs/plan/2026-11-22-02_remote-logs.md 4.4, D14.
*/

namespace {

// games of RoboCup 2026 (and one of 2023)
const QStringList GAMES = {
    "202607050404-HELIOS2026_0-vs-YuShan2026_1.rcg.gz", // 0: HELIOS loses on the left
    "202607050150-TitasDaRobotica_0_2-vs-HELIOS2026_0_3.rcg.gz", // 1: HELIOS wins the shootout on the right
    "202607040043-HELIOS2026_0-vs-ITAndroids_0.rcg.gz", // 2: a draw
    "202607031200-HELIOS2026_5-vs-Oxsy_0.rcg.gz", // 3: HELIOS wins on the left
    "202607050025-FRA-UNIted_0_4-vs-NexusPrime_0_2.rcg.gz", // 4: FRA-UNIted wins the shootout
    "20230709123628-HELIOS2023_1-vs-CYRUS_0.rcg.gz", // 5: 2023
    "game.rcg", // 6: not read
};

//! the indexes of the games that match
QString
matching( const LogFilter & filter )
{
    QStringList list;
    for ( int i = 0; i < GAMES.size(); ++i )
    {
        if ( filter.matches( LogFileName::parse( GAMES[i] ), GAMES[i] ) ) list << QString::number( i );
    }
    return list.join( ' ' );
}

LogFilter
makeFilter( const QString & team,
            const LogFilter::Side side,
            const QString & opponent,
            const LogFilter::Result result,
            const QDate & from = QDate(),
            const QDate & to = QDate() )
{
    LogFilter f;
    f.setTeam( team );
    f.setSide( side );
    f.setOpponent( opponent );
    f.setResult( result );
    f.setFrom( from );
    f.setTo( to );
    return f;
}

}

Q_DECLARE_METATYPE( LogFilter )

/*!
  \brief unit test of LogFilter
*/
class TestLogFilter
    : public QObject {
    Q_OBJECT

private slots:

    void empty();
    void resultOf();
    void matches_data();
    void matches();
};

/*-------------------------------------------------------------------*/
void
TestLogFilter::empty()
{
    LogFilter f;
    QVERIFY( f.isEmpty() );
    QCOMPARE( matching( f ), QString( "0 1 2 3 4 5 6" ) );

    f.setTeam( "  " );
    QVERIFY( f.isEmpty() );
    f.setTeam( " helios " );
    QCOMPARE( f.team(), QString( "helios" ) );
    QVERIFY( ! f.isEmpty() );
    QVERIFY( f != LogFilter() );

    for ( const LogFilter & g : { makeFilter( "", LogFilter::LEFT_SIDE, "", LogFilter::ANY_RESULT ),
                                  makeFilter( "", LogFilter::EITHER_SIDE, "x", LogFilter::ANY_RESULT ),
                                  makeFilter( "", LogFilter::EITHER_SIDE, "", LogFilter::DRAW ),
                                  makeFilter( "", LogFilter::EITHER_SIDE, "", LogFilter::ANY_RESULT, QDate( 2026, 1, 1 ) ),
                                  makeFilter( "", LogFilter::EITHER_SIDE, "", LogFilter::ANY_RESULT, QDate(), QDate( 2026, 1, 1 ) ) } )
    {
        QVERIFY( ! g.isEmpty() );
    }
    QVERIFY( makeFilter( "a", LogFilter::LEFT_SIDE, "b", LogFilter::WIN ) == makeFilter( "a", LogFilter::LEFT_SIDE, "b", LogFilter::WIN ) );
}

/*-------------------------------------------------------------------*/
//! by the goals; in a tie, by the shootout if any
void
TestLogFilter::resultOf()
{
    QCOMPARE( LogFilter::resultOf( 2, -1, 1, -1 ), LogFilter::WIN );
    QCOMPARE( LogFilter::resultOf( 0, -1, 3, -1 ), LogFilter::LOSS );
    QCOMPARE( LogFilter::resultOf( 1, -1, 1, -1 ), LogFilter::DRAW );
    QCOMPARE( LogFilter::resultOf( 0, 4, 0, 2 ), LogFilter::WIN );
    QCOMPARE( LogFilter::resultOf( 1, 2, 1, 3 ), LogFilter::LOSS );
    QCOMPARE( LogFilter::resultOf( 1, 3, 1, 3 ), LogFilter::DRAW );
    QCOMPARE( LogFilter::resultOf( 2, 0, 1, 5 ), LogFilter::WIN ); // the goals first
}

/*-------------------------------------------------------------------*/
void
TestLogFilter::matches_data()
{
    QTest::addColumn< LogFilter >( "filter" );
    QTest::addColumn< QString >( "games" );

    const auto E = LogFilter::EITHER_SIDE;
    const auto L = LogFilter::LEFT_SIDE;
    const auto R = LogFilter::RIGHT_SIDE;
    const auto A = LogFilter::ANY_RESULT;

    // a team
    QTest::newRow( "a team" ) << makeFilter( "helios", E, "", A ) << "0 1 2 3 5";
    QTest::newRow( "a part of the name" ) << makeFilter( "HELIOS2026", E, "", A ) << "0 1 2 3";
    QTest::newRow( "on the left" ) << makeFilter( "helios", L, "", A ) << "0 2 3 5";
    QTest::newRow( "on the right" ) << makeFilter( "helios", R, "", A ) << "1";
    QTest::newRow( "a name with '-'" ) << makeFilter( "fra-united", E, "", A ) << "4";
    QTest::newRow( "the name of a log not read" ) << makeFilter( "game", E, "", A ) << "6";
    QTest::newRow( "no team" ) << makeFilter( "nobody", E, "", A ) << "";

    // both teams
    QTest::newRow( "a match" ) << makeFilter( "helios", E, "yushan", A ) << "0";
    QTest::newRow( "a match, either way" ) << makeFilter( "yushan", E, "helios", A ) << "0";
    QTest::newRow( "a match, the left one given" ) << makeFilter( "yushan", L, "helios", A ) << "";
    QTest::newRow( "a match, the right one given" ) << makeFilter( "yushan", R, "helios", A ) << "0";
    QTest::newRow( "an opponent only" ) << makeFilter( "", E, "oxsy", A ) << "3";
    QTest::newRow( "an opponent on the right" ) << makeFilter( "", L, "helios", A ) << "1";

    // the result of the team
    QTest::newRow( "wins" ) << makeFilter( "helios", E, "", LogFilter::WIN ) << "1 3 5";
    QTest::newRow( "draws" ) << makeFilter( "helios", E, "", LogFilter::DRAW ) << "2";
    QTest::newRow( "losses" ) << makeFilter( "helios", E, "", LogFilter::LOSS ) << "0";
    QTest::newRow( "wins on the left" ) << makeFilter( "helios", L, "", LogFilter::WIN ) << "3 5";
    QTest::newRow( "a shootout lost" ) << makeFilter( "titas", E, "", LogFilter::LOSS ) << "1";
    QTest::newRow( "the wins of the left" ) << makeFilter( "", L, "", LogFilter::WIN ) << "3 4 5";
    QTest::newRow( "the wins of the right" ) << makeFilter( "", R, "", LogFilter::WIN ) << "0 1";
    QTest::newRow( "a win against" ) << makeFilter( "helios", E, "oxsy", LogFilter::WIN ) << "3";
    QTest::newRow( "a loss against" ) << makeFilter( "oxsy", E, "helios", LogFilter::LOSS ) << "3";

    // the dates, both ends included
    QTest::newRow( "a year" ) << makeFilter( "", E, "", A, QDate( 2023, 1, 1 ), QDate( 2023, 12, 31 ) ) << "5";
    QTest::newRow( "from a day" ) << makeFilter( "", E, "", A, QDate( 2026, 7, 5 ) ) << "0 1 4";
    QTest::newRow( "until a day" ) << makeFilter( "", E, "", A, QDate(), QDate( 2026, 7, 4 ) ) << "2 3 5";
    QTest::newRow( "a day" ) << makeFilter( "", E, "", A, QDate( 2026, 7, 4 ), QDate( 2026, 7, 4 ) ) << "2";
    QTest::newRow( "the dates and a team" ) << makeFilter( "helios", E, "", LogFilter::WIN, QDate( 2026, 1, 1 ) ) << "1 3";
    QTest::newRow( "reversed" ) << makeFilter( "", E, "", A, QDate( 2026, 7, 5 ), QDate( 2026, 7, 4 ) ) << "";
}

/*-------------------------------------------------------------------*/
void
TestLogFilter::matches()
{
    QFETCH( LogFilter, filter );
    QFETCH( QString, games );

    QCOMPARE( matching( filter ), games );
}

QTEST_GUILESS_MAIN( TestLogFilter )
#include "test_log_filter.moc"
