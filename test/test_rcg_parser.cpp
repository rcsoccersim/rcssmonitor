// -*-c++-*-

/*!
  \file test_rcg_parser.cpp
  \brief unit test of the rcg parsers
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
#include "rcss/rcg/parser.h"
#include "rcss/rcg/parser_simdjson.h"
#include "rcss/rcg/types.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

/*
  The fixtures in data/ are the first records of two full-game logs:

  head_v5.rcg    202207160432-HELIOS2022_1-vs-CYRUS_0.rcg (text, ULG5):
                 the header, server_param, player_param, the 18 player_type
                 lines, and then the first playmode, team and show lines
                 (shows 1, 2 and 4, playmode play_on at 4). The team_graphic
                 messages are left out.
  head_json.rcg  1-json.rcg (JSON, rcssserver 18.1.3): the records up to the
                 18 player_type records, then the same kind of records
                 (shows 1, 2 and 5, playmode play_on at 5), closed with "]".
*/

namespace {

/*!
  \brief a handler that records what the parser passes to it
*/
class RecordingHandler
    : public rcss::rcg::Handler {
public:

    struct PlayModeRecord {
        int time_;
        rcss::rcg::PlayMode mode_;
    };

    bool eof_ = false;
    int server_param_count_ = 0;
    int player_param_count_ = 0;
    int player_type_count_ = 0;
    int team_count_ = 0;
    int msg_count_ = 0;
    double ball_size_ = 0.0;
    rcss::rcg::TeamT team_l_;
    rcss::rcg::TeamT team_r_;
    std::vector< PlayModeRecord > playmodes_;
    std::vector< rcss::rcg::ShowInfoT > shows_;
    //! the shows ('s') and play modes ('p') with their times, in order
    std::vector< std::pair< char, int > > events_;

    bool handleEOF() override
      {
          eof_ = true;
          return true;
      }

    bool handleShow( const rcss::rcg::ShowInfoT & show ) override
      {
          shows_.push_back( show );
          events_.emplace_back( 's', static_cast< int >( show.time_ ) );
          return true;
      }

    bool handleMsg( const int,
                    const int,
                    const std::string & ) override
      {
          ++msg_count_;
          return true;
      }

    bool handleDraw( const int,
                     const rcss::rcg::drawinfo_t & ) override
      {
          return true;
      }

    bool handlePlayMode( const int time,
                         const rcss::rcg::PlayMode pm ) override
      {
          playmodes_.push_back( { time, pm } );
          events_.emplace_back( 'p', time );
          return true;
      }

    bool handleTeam( const int,
                     const rcss::rcg::TeamT & team_l,
                     const rcss::rcg::TeamT & team_r ) override
      {
          ++team_count_;
          team_l_ = team_l;
          team_r_ = team_r;
          return true;
      }

    bool handleServerParam( const rcss::rcg::ServerParamT & param ) override
      {
          ++server_param_count_;
          ball_size_ = param.ball_size_;
          return true;
      }

    bool handlePlayerParam( const rcss::rcg::PlayerParamT & ) override
      {
          ++player_param_count_;
          return true;
      }

    bool handlePlayerType( const rcss::rcg::PlayerTypeT & ) override
      {
          ++player_type_count_;
          return true;
      }

    bool handleTeamGraphic( const char,
                            const int,
                            const int,
                            const std::vector< std::string > & ) override
      {
          return true;
      }
};

std::string
dataPath( const char * name )
{
    return std::string( TEST_DATA_DIR ) + "/" + name;
}

/*!
  \brief check what the parser passed from head_v5.rcg.
*/
void
checkTextV5( const RecordingHandler & handler )
{
    QVERIFY( handler.eof_ );
    QCOMPARE( handler.logVersion(), rcss::rcg::REC_VERSION_5 );

    QCOMPARE( handler.server_param_count_, 1 );
    QCOMPARE( handler.ball_size_, 0.085 );
    QCOMPARE( handler.player_param_count_, 1 );
    QCOMPARE( handler.player_type_count_, 18 );

    QCOMPARE( handler.team_count_, 1 );
    QCOMPARE( handler.team_l_.name_, std::string( "HELIOS2022" ) );
    QCOMPARE( handler.team_r_.name_, std::string( "CYRUS" ) );

    QCOMPARE( handler.playmodes_.size(), size_t( 2 ) );
    QCOMPARE( handler.playmodes_[0].time_, 1 );
    QCOMPARE( handler.playmodes_[0].mode_, rcss::rcg::PM_KickOff_Left );
    QCOMPARE( handler.playmodes_[1].time_, 4 );
    QCOMPARE( handler.playmodes_[1].mode_, rcss::rcg::PM_PlayOn );

    QCOMPARE( handler.shows_.size(), size_t( 3 ) );
    QCOMPARE( handler.shows_[0].time_, rcss::rcg::UInt32( 1 ) );
    QCOMPARE( handler.shows_[1].time_, rcss::rcg::UInt32( 2 ) );
    QCOMPARE( handler.shows_[2].time_, rcss::rcg::UInt32( 4 ) );

    const rcss::rcg::ShowInfoT & first = handler.shows_.front();
    QCOMPARE( first.player_[0].side_, 'l' );
    QCOMPARE( int( first.player_[0].unum_ ), 1 );
    QCOMPARE( first.player_[0].x_, -49.0f );
    QCOMPARE( first.player_[21].side_, 'r' );
    QCOMPARE( int( first.player_[21].unum_ ), 11 );
    QCOMPARE( first.player_[21].x_, 11.0f );
    QCOMPARE( first.player_[21].y_, 11.0f );

    const rcss::rcg::ShowInfoT & last = handler.shows_.back();
    QCOMPARE( last.ball_.x_, -1.577f );
    QCOMPARE( last.ball_.y_, 1.074f );
    QCOMPARE( last.ball_.vx_, -1.4823f );
    QCOMPARE( last.ball_.vy_, 1.0095f );
}

/*!
  \brief check what the parser passed from head_json.rcg.
*/
void
checkJson( const RecordingHandler & handler )
{
    QVERIFY( handler.eof_ );
    QCOMPARE( handler.logVersion(), rcss::rcg::REC_VERSION_JSON );

    QCOMPARE( handler.server_param_count_, 1 );
    QCOMPARE( handler.ball_size_, 0.085 );
    QCOMPARE( handler.player_param_count_, 1 );
    QCOMPARE( handler.player_type_count_, 18 );

    QCOMPARE( handler.team_count_, 1 );
    QCOMPARE( handler.team_l_.name_, std::string( "HELIOS2023" ) );
    QCOMPARE( handler.team_r_.name_, std::string( "CYRUS" ) );

    QCOMPARE( handler.playmodes_.size(), size_t( 2 ) );
    QCOMPARE( handler.playmodes_[0].time_, 1 );
    QCOMPARE( handler.playmodes_[0].mode_, rcss::rcg::PM_KickOff_Left );
    QCOMPARE( handler.playmodes_[1].time_, 5 );
    QCOMPARE( handler.playmodes_[1].mode_, rcss::rcg::PM_PlayOn );

    QCOMPARE( handler.shows_.size(), size_t( 3 ) );
    QCOMPARE( handler.shows_[0].time_, rcss::rcg::UInt32( 1 ) );
    QCOMPARE( handler.shows_[1].time_, rcss::rcg::UInt32( 2 ) );
    QCOMPARE( handler.shows_[2].time_, rcss::rcg::UInt32( 5 ) );

    const rcss::rcg::ShowInfoT & first = handler.shows_.front();
    QCOMPARE( first.player_[0].side_, 'l' );
    QCOMPARE( int( first.player_[0].unum_ ), 1 );
    QCOMPARE( first.player_[0].x_, -49.0f );
    QCOMPARE( first.player_[21].side_, 'r' );
    QCOMPARE( int( first.player_[21].unum_ ), 11 );
    QCOMPARE( first.player_[21].x_, 11.0f );

    const rcss::rcg::ShowInfoT & last = handler.shows_.back();
    QCOMPARE( last.ball_.x_, -1.6867f );
    QCOMPARE( last.ball_.y_, 1.077f );
    QCOMPARE( last.ball_.vx_, -1.5855f );
    QCOMPARE( last.ball_.vy_, 1.0124f );
}

/*!
  \brief read a whole fixture file into memory.
*/
std::string
readFixture( const char * name )
{
    std::ifstream fin( dataPath( name ), std::ios::binary );
    return std::string( std::istreambuf_iterator< char >( fin ),
                        std::istreambuf_iterator< char >() );
}

}

/*!
  \brief parse JSON log data in memory.
  \return false if no parser was created or the parse failed.
*/
bool
parseJsonData( const std::string & data,
               RecordingHandler & handler )
{
    rcss::rcg::Parser::Ptr parser = rcss::rcg::Parser::create( data.data(), data.size() );
    return parser
        && parser->version() == rcss::rcg::REC_VERSION_JSON
        && parser->parse( data.data(), data.size(), handler );
}

/*!
  \brief the first lines of a fixture, and the line that starts with prefix.
*/
void
splitFixture( const std::string & data,
              const int n_header_lines,
              const std::string & prefix,
              std::string * header,
              std::string * line )
{
    std::size_t pos = 0;
    for ( int i = 0; i < n_header_lines; ++i )
    {
        pos = data.find( '\n', pos ) + 1;
    }
    *header = data.substr( 0, pos );

    const std::size_t start = data.find( "\n" + prefix ) + 1;
    *line = data.substr( start, data.find( '\n', start ) - start );
}

/*!
  \brief the events of a log of n shows, with a play mode at the shows
  1, 1001, 2001, ..., in order.
*/
std::vector< std::pair< char, int > >
manyEvents( const int n,
            const std::vector< int > & excluded = {} )
{
    std::vector< std::pair< char, int > > events;
    for ( int t = 1; t <= n; ++t )
    {
        if ( t % 1000 == 1 ) events.emplace_back( 'p', t );
        if ( std::find( excluded.begin(), excluded.end(), t ) == excluded.end() )
        {
            events.emplace_back( 's', t );
        }
    }
    return events;
}

/*!
  \brief insert a line after the line that starts with prefix.
*/
void
insertLineAfter( std::string * data,
                 const std::string & prefix,
                 const std::string & line )
{
    const std::size_t start = data->find( "\n" + prefix ) + 1;
    const std::size_t next = data->find( '\n', start ) + 1;
    data->insert( next, line + "\n" );
}

/*!
  \brief replace the first occurrence of from with to.
*/
void
replaceFirst( std::string * data,
              const std::string & from,
              const std::string & to )
{
    const std::size_t pos = data->find( from );
    if ( pos != std::string::npos )
    {
        data->replace( pos, from.size(), to );
    }
}

/*!
  \brief head_json.rcg with a record and keys that the parser does not know,
  as a newer server may write them.
*/
std::string
jsonWithUnknownKeys()
{
    std::string data = readFixture( "head_json.rcg" );
    insertLineAfter( &data, "{\"show\":{\"time\":1,",
                     "{\"future_record\":{\"a\":[1,2,{\"b\":\"c\"}]}}," );
    // an unknown key before a known one in a record
    insertLineAfter( &data, "{\"show\":{\"time\":1,",
                     "{\"future_key\":1,\"msg\":{\"time\":1,\"board\":1,\"message\":\"m\"}}," );
    replaceFirst( &data, "{\"show\":{\"time\":2,", "{\"show\":{\"time\":2,\"future_key\":[0],\"future_object\":{\"x\":1}," );
    replaceFirst( &data, "{\"side\":\"l\",\"unum\":1,", "{\"side\":\"l\",\"future_key\":true,\"unum\":1," );
    return data;
}

//! the number of shows in the large logs: the lines are parsed in parallel
const int MANY_SHOWS = 5000;

/*!
  \brief unit test of the rcg parsers
*/
class TestRcgParser
    : public QObject {
    Q_OBJECT

private slots:

    void textV5();
    void textV5Memory();
    void json();
    void jsonMemory();
    void jsonWithoutClosingBracket();
    void jsonBrokenLines();
    void jsonOneLine();
    void jsonCrLf();
    void jsonUnknownKeys();
    void jsonUnknownKeysOneLine();
    void jsonRecordThatFailsOneLine();
    void liveMessages();
    void jsonViewQuality();
    void textManyLines();
    void textManyLinesStopAtIllegalLine();
    void jsonManyLines();
};

/*-------------------------------------------------------------------*/
/*!
  A text log (v5) through a stream.
*/
void
TestRcgParser::textV5()
{
    std::ifstream fin( dataPath( "head_v5.rcg" ) );
    QVERIFY( fin.is_open() );

    rcss::rcg::Parser::Ptr parser = rcss::rcg::Parser::create( fin );
    QVERIFY( parser );
    // ParserV4 reads the text formats v4, v5 and v6, and reports the
    // version of the log itself to the handler.
    QCOMPARE( parser->version(), rcss::rcg::REC_VERSION_4 );

    RecordingHandler handler;
    QVERIFY( parser->parse( fin, handler ) );
    checkTextV5( handler );
}

/*-------------------------------------------------------------------*/
/*!
  A text log (v5) in memory, as the application reads it.
*/
void
TestRcgParser::textV5Memory()
{
    const std::string data = readFixture( "head_v5.rcg" );
    QVERIFY( ! data.empty() );

    rcss::rcg::Parser::Ptr parser = rcss::rcg::Parser::create( data.data(), data.size() );
    QVERIFY( parser );
    QCOMPARE( parser->version(), rcss::rcg::REC_VERSION_4 );

    RecordingHandler handler;
    QVERIFY( parser->parse( data.data(), data.size(), handler ) );
    checkTextV5( handler );
}

/*-------------------------------------------------------------------*/
/*!
  A JSON log through a stream.
*/
void
TestRcgParser::json()
{
    std::ifstream fin( dataPath( "head_json.rcg" ) );
    QVERIFY( fin.is_open() );

    rcss::rcg::Parser::Ptr parser = rcss::rcg::Parser::create( fin );
    QVERIFY( parser );
    QCOMPARE( parser->version(), rcss::rcg::REC_VERSION_JSON );

    RecordingHandler handler;
    QVERIFY( parser->parse( fin, handler ) );
    checkJson( handler );
}

/*-------------------------------------------------------------------*/
/*!
  A JSON log in memory, as the application reads it.
*/
void
TestRcgParser::jsonMemory()
{
    const std::string data = readFixture( "head_json.rcg" );
    QVERIFY( ! data.empty() );

    rcss::rcg::Parser::Ptr parser = rcss::rcg::Parser::create( data.data(), data.size() );
    QVERIFY( parser );
    QCOMPARE( parser->version(), rcss::rcg::REC_VERSION_JSON );

    RecordingHandler handler;
    QVERIFY( parser->parse( data.data(), data.size(), handler ) );
    checkJson( handler );
}

/*-------------------------------------------------------------------*/
/*!
  A JSON log whose last "]" is missing, with "," after the last record, as
  rcssserver leaves it when it stops abnormally.
*/
void
TestRcgParser::jsonWithoutClosingBracket()
{
    std::string data = readFixture( "head_json.rcg" );
    const std::size_t bracket = data.rfind( ']' );
    QVERIFY( bracket != std::string::npos );
    data.erase( bracket );
    const std::size_t brace = data.rfind( '}' );
    data.insert( brace + 1, "," );

    RecordingHandler handler;
    QVERIFY( parseJsonData( data, handler ) );
    checkJson( handler );
}

/*-------------------------------------------------------------------*/
/*!
  The lines that cannot be parsed are skipped, and the others are read.
*/
void
TestRcgParser::jsonBrokenLines()
{
    std::string data = readFixture( "head_json.rcg" );
    // after the show of time 2
    const std::size_t show2 = data.find( "{\"show\":{\"time\":2," );
    QVERIFY( show2 != std::string::npos );
    const std::size_t next = data.find( '\n', show2 ) + 1;
    data.insert( next, "{\"show\":{\"time\":3,\"ball\":{\"x\":1,\nnot a record\n" );

    RecordingHandler handler;
    QVERIFY( parseJsonData( data, handler ) );
    checkJson( handler );
}

/*-------------------------------------------------------------------*/
/*!
  A log that is not one record per line is parsed as a whole document.
*/
void
TestRcgParser::jsonOneLine()
{
    std::string data = readFixture( "head_json.rcg" );
    data.erase( std::remove( data.begin(), data.end(), '\n' ), data.end() );

    RecordingHandler handler;
    QVERIFY( parseJsonData( data, handler ) );
    checkJson( handler );
}

/*-------------------------------------------------------------------*/
/*!
  The lines may end with CR LF.
*/
void
TestRcgParser::jsonCrLf()
{
    const std::string lf = readFixture( "head_json.rcg" );
    std::string data;
    for ( const char c : lf )
    {
        if ( c == '\n' ) data += '\r';
        data += c;
    }

    RecordingHandler handler;
    QVERIFY( parseJsonData( data, handler ) );
    checkJson( handler );
}

/*-------------------------------------------------------------------*/
/*!
  A record, and keys in a record, that the parser does not know are
  skipped, and the rest is read (B3).
*/
void
TestRcgParser::jsonUnknownKeys()
{
    const std::string data = jsonWithUnknownKeys();
    QVERIFY( data.find( "future_record" ) != std::string::npos );
    QVERIFY( data.find( "future_object" ) != std::string::npos );
    QVERIFY( data.find( "\"future_key\":true" ) != std::string::npos );

    RecordingHandler handler;
    QVERIFY( parseJsonData( data, handler ) );
    checkJson( handler );
    QCOMPARE( handler.msg_count_, 1 );
}

/*-------------------------------------------------------------------*/
/*!
  The same, in a log parsed as a whole document.
*/
void
TestRcgParser::jsonUnknownKeysOneLine()
{
    std::string data = jsonWithUnknownKeys();
    data.erase( std::remove( data.begin(), data.end(), '\n' ), data.end() );

    RecordingHandler handler;
    QVERIFY( parseJsonData( data, handler ) );
    checkJson( handler );
    QCOMPARE( handler.msg_count_, 1 );
}

/*-------------------------------------------------------------------*/
/*!
  In a log parsed as a whole document, a record that is valid JSON but
  cannot be read is skipped, and the records after it are read.
*/
void
TestRcgParser::jsonRecordThatFailsOneLine()
{
    std::string data = readFixture( "head_json.rcg" );
    insertLineAfter( &data, "{\"show\":{\"time\":1,",
                     "{\"msg\":{\"time\":1,\"board\":1}}," );
    data.erase( std::remove( data.begin(), data.end(), '\n' ), data.end() );

    RecordingHandler handler;
    QVERIFY( parseJsonData( data, handler ) );
    checkJson( handler );
    QCOMPARE( handler.msg_count_, 0 );
}

/*-------------------------------------------------------------------*/
/*!
  The messages of a live connection: a broken message fails without an
  exception, and an unknown one is skipped.
*/
void
TestRcgParser::liveMessages()
{
    std::string header, show;
    splitFixture( readFixture( "head_json.rcg" ), 0, "{\"show\":{\"time\":1,", &header, &show );
    show.pop_back(); // the comma

    const rcss::rcg::ParserSimdJSON parser;
    RecordingHandler handler;
    QVERIFY( parser.parseData( show, handler ) );
    QCOMPARE( handler.shows_.size(), size_t( 1 ) );

    QVERIFY( parser.parseData( "{\"future_record\":{\"a\":1}}", handler ) );
    QVERIFY( ! parser.parseData( "{\"show\":{\"time\":2,\"ball\":", handler ) );
    QVERIFY( ! parser.parseData( "not a message", handler ) );
    QVERIFY( ! parser.parseData( "", handler ) );
    QVERIFY( ! parser.parseData( "{\"msg\":{\"time\":1,\"board\":1,\"message\":\"a\nb\"}}", handler ) );
    QCOMPARE( handler.shows_.size(), size_t( 1 ) );
}

/*-------------------------------------------------------------------*/
/*!
  The view quality is 'h' or 'l', as in the other formats (B2).
*/
void
TestRcgParser::jsonViewQuality()
{
    std::string data = readFixture( "head_json.rcg" );
    replaceFirst( &data, "\"vq\":\"h\"", "\"vq\":\"l\"" );

    RecordingHandler handler;
    QVERIFY( parseJsonData( data, handler ) );
    QCOMPARE( handler.shows_.size(), size_t( 3 ) );
    QCOMPARE( handler.shows_[0].player_[0].view_quality_, 'l' );
    QCOMPARE( handler.shows_[0].player_[1].view_quality_, 'h' );
    QCOMPARE( handler.shows_[1].player_[0].view_quality_, 'h' );
}

/*-------------------------------------------------------------------*/
/*!
  A large text log, parsed in parallel: the shows and play modes reach the
  handler in the order of the lines.
*/
void
TestRcgParser::textManyLines()
{
    std::string header, show;
    splitFixture( readFixture( "head_v5.rcg" ), 21, "(show 1 ", &header, &show );

    std::string data = header;
    for ( int t = 1; t <= MANY_SHOWS; ++t )
    {
        if ( t % 1000 == 1 ) data += "(playmode " + std::to_string( t ) + " play_on)\n";
        data += "(show " + std::to_string( t ) + show.substr( 7 ) + "\n";
    }

    RecordingHandler handler;
    rcss::rcg::Parser::Ptr parser = rcss::rcg::Parser::create( data.data(), data.size() );
    QVERIFY( parser );
    QVERIFY( parser->parse( data.data(), data.size(), handler ) );
    QVERIFY( handler.eof_ );
    QCOMPARE( handler.player_type_count_, 18 );
    QCOMPARE( handler.events_, manyEvents( MANY_SHOWS ) );
}

/*-------------------------------------------------------------------*/
/*!
  In a large text log, an illegal line ends the parse as before: nothing
  after it reaches the handler.
*/
void
TestRcgParser::textManyLinesStopAtIllegalLine()
{
    std::string header, show;
    splitFixture( readFixture( "head_v5.rcg" ), 21, "(show 1 ", &header, &show );

    std::string data = header;
    for ( int t = 1; t <= MANY_SHOWS; ++t )
    {
        if ( t % 1000 == 1 ) data += "(playmode " + std::to_string( t ) + " play_on)\n";
        data += "(show " + std::to_string( t ) + show.substr( 7 ) + "\n";
        if ( t == 3000 ) data += "an illegal line\n";
    }

    RecordingHandler handler;
    rcss::rcg::Parser::Ptr parser = rcss::rcg::Parser::create( data.data(), data.size() );
    QVERIFY( parser );
    QVERIFY( ! parser->parse( data.data(), data.size(), handler ) );
    QCOMPARE( handler.events_, manyEvents( 3000 ) );
}

/*-------------------------------------------------------------------*/
/*!
  A large JSON log, parsed in parallel: the records reach the handler in
  the order of the lines, and a broken line is skipped.
*/
void
TestRcgParser::jsonManyLines()
{
    std::string header, show;
    splitFixture( readFixture( "head_json.rcg" ), 23, "{\"show\":{\"time\":1,", &header, &show );
    const std::string after_time = show.substr( std::string( "{\"show\":{\"time\":1" ).size() );

    std::string data = header;
    for ( int t = 1; t <= MANY_SHOWS; ++t )
    {
        if ( t % 1000 == 1 )
        {
            data += "{\"playmode\":{\"time\":" + std::to_string( t ) + ",\"mode\":\"play_on\"}},\n";
        }
        if ( t == 2500 )
        {
            data += "{\"show\":{\"time\":2500,\"ball\":{\n"; // broken
            continue;
        }
        data += "{\"show\":{\"time\":" + std::to_string( t ) + after_time;
        if ( data.back() != ',' ) data += ',';
        data += '\n';
    }
    data += "]\n";

    RecordingHandler handler;
    QVERIFY( parseJsonData( data, handler ) );
    QVERIFY( handler.eof_ );
    QCOMPARE( handler.player_type_count_, 18 );
    QCOMPARE( handler.events_, manyEvents( MANY_SHOWS, { 2500 } ) );
}

QTEST_APPLESS_MAIN( TestRcgParser )

#include "test_rcg_parser.moc"
