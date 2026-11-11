// -*-c++-*-

/*!
  \file test_line_parser.cpp
  \brief unit test of split_lines() and parse_lines()
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

#include "rcss/rcg/handler.h"
#include "rcss/rcg/line_parser.h"
#include "rcss/rcg/types.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

/*!
  \brief a handler that records the times of the play modes passed to it
*/
class TimeHandler
    : public rcss::rcg::Handler {
public:
    std::vector< int > times_;

    bool handleEOF() override { return true; }
    bool handleShow( const rcss::rcg::ShowInfoT & ) override { return true; }
    bool handleMsg( const int, const int, const std::string & ) override { return true; }
    bool handleDraw( const int, const rcss::rcg::drawinfo_t & ) override { return true; }
    bool handlePlayMode( const int time, const rcss::rcg::PlayMode ) override { times_.push_back( time ); return true; }
    bool handleTeam( const int, const rcss::rcg::TeamT &, const rcss::rcg::TeamT & ) override { return true; }
    bool handleServerParam( const rcss::rcg::ServerParamT & ) override { return true; }
    bool handlePlayerParam( const rcss::rcg::PlayerParamT & ) override { return true; }
    bool handlePlayerType( const rcss::rcg::PlayerTypeT & ) override { return true; }
    bool handleTeamGraphic( const char, const int, const int, const std::vector< std::string > & ) override { return true; }
};

/*!
  \brief a line parser: a number becomes a play mode at that time, "bad"
  fails, and "throw" throws.
*/
rcss::rcg::LineParser
makeNumberParser()
{
    return []( const rcss::rcg::LineRef & line, rcss::rcg::Handler & handler ) -> bool
        {
            const std::string text( line.begin_, line.length_ );
            if ( text == "bad" ) return false;
            if ( text == "throw" ) throw std::runtime_error( "a test exception" );
            return handler.handlePlayMode( std::stoi( text ), rcss::rcg::PM_PlayOn );
        };
}

/*!
  \brief the lines "1" to "n", each line has its own number, with some
  lines replaced.
*/
std::string
numberLines( const int n,
             const std::vector< std::pair< int, std::string > > & replaced = {} )
{
    std::string data;
    for ( int i = 1; i <= n; ++i )
    {
        std::string line = std::to_string( i );
        for ( const std::pair< int, std::string > & r : replaced )
        {
            if ( r.first == i ) line = r.second;
        }
        data += line;
        data += '\n';
    }
    return data;
}

std::vector< int >
range( const int first,
       const int last,
       const std::vector< int > & excluded = {} )
{
    std::vector< int > v;
    for ( int i = first; i <= last; ++i )
    {
        if ( std::find( excluded.begin(), excluded.end(), i ) == excluded.end() )
        {
            v.push_back( i );
        }
    }
    return v;
}

}

/*!
  \brief unit test of split_lines() and parse_lines()
*/
class TestLineParser
    : public QObject {
    Q_OBJECT

private slots:

    void splitLines();
    void manyLinesInOrder();
    void failedLinesSkipped();
    void stopAtFailure();
};

/*-------------------------------------------------------------------*/
/*!
  Lines as std::getline() makes them.
*/
void
TestLineParser::splitLines()
{
    const std::string data = "a\nb\r\n\nc";
    const std::vector< rcss::rcg::LineRef > lines = rcss::rcg::split_lines( data.data(), data.size() );
    QCOMPARE( lines.size(), size_t( 4 ) );
    QCOMPARE( std::string( lines[0].begin_, lines[0].length_ ), std::string( "a" ) );
    QCOMPARE( std::string( lines[1].begin_, lines[1].length_ ), std::string( "b\r" ) );
    QCOMPARE( std::string( lines[2].begin_, lines[2].length_ ), std::string() );
    QCOMPARE( std::string( lines[3].begin_, lines[3].length_ ), std::string( "c" ) );
    QCOMPARE( lines[3].number_, 4 );

    const std::string ended = "a\n";
    QCOMPARE( rcss::rcg::split_lines( ended.data(), ended.size() ).size(), size_t( 1 ) );
    QCOMPARE( rcss::rcg::split_lines( "", 0 ).size(), size_t( 0 ) );
}

/*-------------------------------------------------------------------*/
/*!
  Many lines, parsed in parallel, reach the handler in their order.
*/
void
TestLineParser::manyLinesInOrder()
{
    const std::string data = numberLines( 20000 );
    TimeHandler handler;
    const std::vector< int > failed
        = rcss::rcg::parse_lines( rcss::rcg::split_lines( data.data(), data.size() ),
                                  makeNumberParser, false, handler );
    QVERIFY( failed.empty() );
    QCOMPARE( handler.times_, range( 1, 20000 ) );
}

/*-------------------------------------------------------------------*/
/*!
  The lines that fail (or throw) are skipped, and reported by number.
*/
void
TestLineParser::failedLinesSkipped()
{
    const std::string data = numberLines( 20000, { { 5000, "bad" }, { 15000, "throw" } } );
    TimeHandler handler;
    const std::vector< int > failed
        = rcss::rcg::parse_lines( rcss::rcg::split_lines( data.data(), data.size() ),
                                  makeNumberParser, false, handler );
    QCOMPARE( failed, std::vector< int >( { 5000, 15000 } ) );
    QCOMPARE( handler.times_, range( 1, 20000, { 5000, 15000 } ) );
}

/*-------------------------------------------------------------------*/
/*!
  With stop_at_failure, nothing after the first failed line reaches the
  handler, also from the blocks that other threads parsed.
*/
void
TestLineParser::stopAtFailure()
{
    const std::string data = numberLines( 20000, { { 12345, "bad" }, { 18000, "bad" } } );
    TimeHandler handler;
    const std::vector< int > failed
        = rcss::rcg::parse_lines( rcss::rcg::split_lines( data.data(), data.size() ),
                                  makeNumberParser, true, handler );
    QCOMPARE( failed, std::vector< int >( { 12345 } ) );
    QCOMPARE( handler.times_, range( 1, 12344 ) );
}

QTEST_APPLESS_MAIN( TestLineParser )

#include "test_line_parser.moc"
