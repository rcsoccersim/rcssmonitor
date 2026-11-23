// -*-c++-*-

/*!
  \file test_log_file_name.cpp
  \brief unit test of LogFileName: the names of game log files
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

/*
  docs/plan/2026-11-22-02_remote-logs.md 4.2: names of the logs of RoboCup
  2026 on GitHub, and of the archive (2004 to 2023).
*/

/*!
  \brief unit test of LogFileName
*/
class TestLogFileName
    : public QObject {
    Q_OBJECT

private slots:

    void parse_data();
    void parse();
    void notLogs_data();
    void notLogs();
};

/*-------------------------------------------------------------------*/
void
TestLogFileName::parse_data()
{
    QTest::addColumn< QString >( "name" );
    QTest::addColumn< QString >( "date" );
    QTest::addColumn< QString >( "left" );
    QTest::addColumn< QString >( "right" );
    QTest::addColumn< QString >( "score" );

    QTest::newRow( "2026" ) << "202607050404-HELIOS2026_0-vs-YuShan2026_1.rcg.gz"
                            << "2026-07-05 04:04:00" << "HELIOS2026" << "YuShan2026" << "0 - 1";
    QTest::newRow( "a team with '-'" ) << "202607042328-KETE_1-vs-FC-AIT_2.rcg.gz"
                                       << "2026-07-04 23:28:00" << "KETE" << "FC-AIT" << "1 - 2";
    QTest::newRow( "a team with '_'" ) << "202607031200-RoboTech_Coach_base_0-vs-Oxsy_5.rcg.gz"
                                       << "2026-07-03 12:00:00" << "RoboTech_Coach_base" << "Oxsy" << "0 - 5";
    QTest::newRow( "a penalty shootout" ) << "202607050025-FRA-UNIted_0_4-vs-NexusPrime_0_2.rcg.gz"
                                          << "2026-07-05 00:25:00" << "FRA-UNIted" << "NexusPrime" << "0 (4) - 0 (2)";
    QTest::newRow( "seconds (2023)" ) << "20230709095223-Hermes2D_3-vs-R3CESBU_3.rcg.gz"
                                      << "2023-07-09 09:52:23" << "Hermes2D" << "R3CESBU" << "3 - 3";
    QTest::newRow( "2004" ) << "200407031414-Mersad_3-vs-TsinghuAeolus_2.rcg.gz"
                            << "2004-07-03 14:14:00" << "Mersad" << "TsinghuAeolus" << "3 - 2";
    QTest::newRow( "not compressed" ) << "202207160432-HELIOS2022_1-vs-CYRUS_0.rcg"
                                      << "2022-07-16 04:32:00" << "HELIOS2022" << "CYRUS" << "1 - 0";
}

/*-------------------------------------------------------------------*/
void
TestLogFileName::parse()
{
    QFETCH( QString, name );
    QFETCH( QString, date );
    QFETCH( QString, left );
    QFETCH( QString, right );
    QFETCH( QString, score );

    const LogFileName n = LogFileName::parse( name );
    QVERIFY( n.isValid() );
    QCOMPARE( n.dateTime().toString( "yyyy-MM-dd HH:mm:ss" ), date );
    QCOMPARE( n.leftTeam(), left );
    QCOMPARE( n.rightTeam(), right );
    QCOMPARE( n.score(), score );
}

/*-------------------------------------------------------------------*/
void
TestLogFileName::notLogs_data()
{
    QTest::addColumn< QString >( "name" );

    QTest::newRow( "a command log" ) << "202607050404-HELIOS2026_0-vs-YuShan2026_1.rcl.gz";
    QTest::newRow( "an old text log" ) << "200807171200-amoiensis-vs-brainstormers.log.gz";
    QTest::newRow( "no scores" ) << "200807171200-amoiensis-vs-brainstormers.rcg.gz";
    QTest::newRow( "no date" ) << "HELIOS2026_0-vs-YuShan2026_1.rcg.gz";
    QTest::newRow( "a wrong date" ) << "202613450404-A_0-vs-B_1.rcg.gz";
    QTest::newRow( "no team" ) << "202607050404-_0-vs-B_1.rcg.gz";
    QTest::newRow( "another name" ) << "game.rcg.gz";
    QTest::newRow( "nothing" ) << "";
}

/*-------------------------------------------------------------------*/
//! not read: not valid, no score
void
TestLogFileName::notLogs()
{
    QFETCH( QString, name );
    const LogFileName n = LogFileName::parse( name );
    QVERIFY( ! n.isValid() );
    QCOMPARE( n.score(), QString() );
    QCOMPARE( n.leftPenalties(), -1 );
}

QTEST_GUILESS_MAIN( TestLogFileName )
#include "test_log_file_name.moc"
