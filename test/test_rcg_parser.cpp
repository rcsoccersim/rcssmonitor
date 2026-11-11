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
#include "rcss/rcg/types.h"

#include <fstream>
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
    double ball_size_ = 0.0;
    rcss::rcg::TeamT team_l_;
    rcss::rcg::TeamT team_r_;
    std::vector< PlayModeRecord > playmodes_;
    std::vector< rcss::rcg::ShowInfoT > shows_;

    bool handleEOF() override
      {
          eof_ = true;
          return true;
      }

    bool handleShow( const rcss::rcg::ShowInfoT & show ) override
      {
          shows_.push_back( show );
          return true;
      }

    bool handleMsg( const int,
                    const int,
                    const std::string & ) override
      {
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

}

/*!
  \brief unit test of the rcg parsers
*/
class TestRcgParser
    : public QObject {
    Q_OBJECT

private slots:

    void textV5();
    void json();
};

/*-------------------------------------------------------------------*/
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

/*-------------------------------------------------------------------*/
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

QTEST_APPLESS_MAIN( TestRcgParser )

#include "test_rcg_parser.moc"
