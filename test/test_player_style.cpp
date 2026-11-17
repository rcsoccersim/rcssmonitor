// -*-c++-*-

/*!
  \file test_player_style.cpp
  \brief unit test of playerStyle, the colors of a player in 2D and 3D
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

#include "options.h"
#include "player_style.h"

#include <rcss/rcg/types.h>

namespace {

rcss::rcg::PlayerT
makePlayer( const char side,
            const int unum )
{
    rcss::rcg::PlayerT p;
    p.side_ = side;
    p.unum_ = static_cast< rcss::rcg::Int16 >( unum );
    p.state_ = rcss::rcg::STAND;
    return p;
}

}

class TestPlayerStyle
    : public QObject {
    Q_OBJECT

private:
    rcss::rcg::PlayerTypeT M_type;
    rcss::rcg::ServerParamT M_param;

    PlayerStyle style( const rcss::rcg::PlayerT & p )
      {
          return playerStyle( p, M_type, M_param, Options::instance() );
      }

private slots:

    void init()
      {
          Options::instance().unselectPlayer();
      }

    void teamColors()
      {
          const Options & opt = Options::instance();

          rcss::rcg::PlayerT l = makePlayer( 'l', 2 );
          QCOMPARE( style( l ).brush, opt.leftTeamBrush() );
          QCOMPARE( style( l ).pen, opt.playerPen() );
          QVERIFY( ! style( l ).highlighted() );

          l.state_ |= rcss::rcg::GOALIE;
          QCOMPARE( style( l ).brush, opt.leftGoalieBrush() );

          rcss::rcg::PlayerT r = makePlayer( 'r', 2 );
          QCOMPARE( style( r ).brush, opt.rightTeamBrush() );
          r.state_ |= rcss::rcg::GOALIE;
          QCOMPARE( style( r ).brush, opt.rightGoalieBrush() );

          // not connected
          r.state_ = 0;
          QCOMPARE( style( r ).brush.color(), QColor( Qt::black ) );
          QCOMPARE( style( makePlayer( 'n', 2 ) ).brush.color(), QColor( Qt::black ) );
      }

    /*!
      a goalie as far as it is known to be one (the player camera: only with
      its number does rcssserver tell a goalie): its team's field color at 0,
      the goalie color at 1, between them in between; a field player and a
      state's color as they are
    */
    void goalieKnown()
      {
          const Options & opt = Options::instance();
          for ( const char side : { 'l', 'r' } )
          {
              rcss::rcg::PlayerT g = makePlayer( side, 1 );
              g.state_ |= rcss::rcg::GOALIE;
              const QColor field = ( side == 'l' ? opt.leftTeamBrush() : opt.rightTeamBrush() ).color();
              const QColor keeper = ( side == 'l' ? opt.leftGoalieBrush() : opt.rightGoalieBrush() ).color();
              QVERIFY( field != keeper );
              const auto known = [&]( const float k ) {
                  return playerStyle( g, M_type, M_param, opt, k ).brush.color();
              };
              QCOMPARE( known( 1.0f ), keeper );
              QCOMPARE( known( 0.0f ).rgb(), field.rgb() );
              QCOMPARE( known( -1.0f ).rgb(), field.rgb() );
              const QColor half = known( 0.5f );
              QVERIFY( std::abs( half.red() - ( field.red() + keeper.red() ) / 2 ) <= 1 );
              QVERIFY( std::abs( half.green() - ( field.green() + keeper.green() ) / 2 ) <= 1 );
              QVERIFY( std::abs( half.blue() - ( field.blue() + keeper.blue() ) / 2 ) <= 1 );
              // the body too (darker by the stamina, of the brush)
              QCOMPARE( playerStyle( g, M_type, M_param, opt, 0.0f ).body.color().rgb(), field.rgb() );

              // a field player: its color whatever is known
              const rcss::rcg::PlayerT f = makePlayer( side, 2 );
              QCOMPARE( playerStyle( f, M_type, M_param, opt, 0.0f ).brush.color(), field );
              // a state's color over it
              g.state_ |= rcss::rcg::TACKLE;
              QCOMPARE( playerStyle( g, M_type, M_param, opt, 0.0f ).brush, opt.tackleBrush() );
          }
      }

    void selected()
      {
          Options & opt = Options::instance();
          opt.setSelectedNumber( rcss::rcg::RIGHT, 7 );

          QCOMPARE( style( makePlayer( 'r', 7 ) ).pen, opt.selectedPlayerPen() );
          QVERIFY( style( makePlayer( 'r', 7 ) ).highlighted() );
          QCOMPARE( style( makePlayer( 'l', 7 ) ).pen, opt.playerPen() );
      }

    void states()
      {
          const Options & opt = Options::instance();

          rcss::rcg::PlayerT p = makePlayer( 'l', 3 );
          p.state_ |= rcss::rcg::KICK;
          QCOMPARE( style( p ).pen, opt.kickPen() );
          QCOMPARE( style( p ).brush, opt.leftTeamBrush() );
          QVERIFY( style( p ).highlighted() );

          p.state_ = rcss::rcg::STAND | rcss::rcg::KICK | rcss::rcg::KICK_FAULT;
          QCOMPARE( style( p ).brush, opt.kickFaultBrush() );

          p.state_ = rcss::rcg::STAND | rcss::rcg::CATCH;
          QCOMPARE( style( p ).brush, opt.catchBrush() );

          // the tackle comes after the kick
          p.state_ = rcss::rcg::STAND | rcss::rcg::KICK | rcss::rcg::TACKLE;
          QCOMPARE( style( p ).pen, opt.tacklePen() );
          QCOMPARE( style( p ).brush, opt.tackleBrush() );

          p.state_ = rcss::rcg::STAND | rcss::rcg::FOUL_CHARGED;
          QCOMPARE( style( p ).brush, opt.foulChargedBrush() );

          p.state_ = rcss::rcg::STAND | rcss::rcg::BALL_COLLIDE;
          QCOMPARE( style( p ).brush, opt.ballCollideBrush() );
      }

    void stamina()
      {
          rcss::rcg::PlayerT p = makePlayer( 'l', 4 );
          const QColor team = Options::instance().leftTeamBrush().color();

          // without the stamina
          QCOMPARE( style( p ).body, style( p ).brush );
          QVERIFY( ! style( p ).decayed() );

          p.effort_ = static_cast< float >( M_type.effort_max_ );
          p.recovery_ = 1.0f;

          p.stamina_ = static_cast< float >( M_param.stamina_max_ );
          QCOMPARE( style( p ).body, style( p ).brush );

          // darker by steps of 1/8: 200 - 200 * 4/8 - 50 = 50
          p.stamina_ = static_cast< float >( M_param.stamina_max_ * 0.5 );
          QCOMPARE( style( p ).body.color(), team.darker( 150 ) );

          // 200 - 0 - 50 = 150
          p.stamina_ = 0.0f;
          QCOMPARE( style( p ).body.color(), team.darker( 250 ) );
          QCOMPARE( style( p ).brush.color(), team );
      }

    void decay()
      {
          const Options & opt = Options::instance();

          rcss::rcg::PlayerT p = makePlayer( 'l', 5 );
          p.stamina_ = static_cast< float >( M_param.stamina_max_ );
          p.effort_ = static_cast< float >( M_type.effort_max_ );
          p.recovery_ = 1.0f;
          QVERIFY( ! style( p ).decayed() );

          p.recovery_ = 0.8f;
          QVERIFY( style( p ).decayed() );
          QCOMPARE( style( p ).decay, opt.recoveryDecayedPen() );

          // the effort comes first
          p.effort_ = static_cast< float >( M_type.effort_max_ - 0.1 );
          QCOMPARE( style( p ).decay, opt.effortDecayedPen() );
      }
};

QTEST_MAIN( TestPlayerStyle )
#include "test_player_style.moc"
