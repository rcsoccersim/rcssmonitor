// -*-c++-*-

/*!
  \file test_player_selection.cpp
  \brief unit test of selectPlayerNearestBall, the automatic selection in 2D and 3D
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
#include "player_selection.h"

#include <rcss/rcg/types.h>

namespace {

/*!
  \brief the ball at the center; the left team's 3 at 5 m, the right team's
  7 at 2 m, the others far away
*/
rcss::rcg::ShowInfoT
makeShow()
{
    rcss::rcg::ShowInfoT show;
    for ( int i = 0; i < rcss::rcg::MAX_PLAYER * 2; ++i )
    {
        rcss::rcg::PlayerT & p = show.player_[i];
        const bool left = ( i < rcss::rcg::MAX_PLAYER );
        p.side_ = ( left ? 'l' : 'r' );
        p.unum_ = static_cast< rcss::rcg::Int16 >( i % rcss::rcg::MAX_PLAYER + 1 );
        p.state_ = rcss::rcg::STAND;
        p.x_ = ( left ? -40.0f : 40.0f );
        p.y_ = 0.0f;
    }
    show.player_[2].x_ = -5.0f;
    show.player_[rcss::rcg::MAX_PLAYER + 6].x_ = 2.0f;
    return show;
}

//! select by the type, from no selection
void
selectBy( const Options::PlayerSelectType type )
{
    Options::instance().unselectPlayer();
    Options::instance().setPlayerSelectType( type );
}

}

class TestPlayerSelection
    : public QObject {
    Q_OBJECT

private slots:

    void nearestOfBoth()
      {
          selectBy( Options::SELECT_AUTO_ALL );
          QVERIFY( selectPlayerNearestBall( makeShow(), Options::instance() ) );
          QVERIFY( Options::instance().selectedPlayer( rcss::rcg::RIGHT, 7 ) );
      }

    void nearestOfATeam()
      {
          selectBy( Options::SELECT_AUTO_LEFT );
          QVERIFY( selectPlayerNearestBall( makeShow(), Options::instance() ) );
          QVERIFY( Options::instance().selectedPlayer( rcss::rcg::LEFT, 3 ) );

          selectBy( Options::SELECT_AUTO_RIGHT );
          QVERIFY( selectPlayerNearestBall( makeShow(), Options::instance() ) );
          QVERIFY( Options::instance().selectedPlayer( rcss::rcg::RIGHT, 7 ) );

          // of its team, even with a nearer player of the other
          rcss::rcg::ShowInfoT show = makeShow();
          show.player_[2].x_ = -1.0f;
          selectBy( Options::SELECT_AUTO_RIGHT );
          QVERIFY( selectPlayerNearestBall( show, Options::instance() ) );
          QVERIFY( Options::instance().selectedPlayer( rcss::rcg::RIGHT, 7 ) );
          show = makeShow();
          show.player_[rcss::rcg::MAX_PLAYER + 6].x_ = 0.5f;
          show.player_[2].x_ = -3.0f;
          selectBy( Options::SELECT_AUTO_LEFT );
          QVERIFY( selectPlayerNearestBall( show, Options::instance() ) );
          QVERIFY( Options::instance().selectedPlayer( rcss::rcg::LEFT, 3 ) );
      }

    void notConnected()
      {
          // the nearest is not in the game: the next nearest
          rcss::rcg::ShowInfoT show = makeShow();
          show.player_[rcss::rcg::MAX_PLAYER + 6].state_ = 0;
          selectBy( Options::SELECT_AUTO_ALL );
          QVERIFY( selectPlayerNearestBall( show, Options::instance() ) );
          QVERIFY( Options::instance().selectedPlayer( rcss::rcg::LEFT, 3 ) );

          // nobody: the selection stays
          for ( auto & p : show.player_ ) p.state_ = 0;
          QVERIFY( ! selectPlayerNearestBall( show, Options::instance() ) );
          QVERIFY( Options::instance().selectedPlayer( rcss::rcg::LEFT, 3 ) );
      }

    void notAutomatic()
      {
          // fixed, or nothing selected: no change
          Options & opt = Options::instance();
          opt.unselectPlayer();
          opt.setSelectedNumber( rcss::rcg::LEFT, 9 );
          opt.setPlayerSelectType( Options::SELECT_FIX );
          QVERIFY( ! selectPlayerNearestBall( makeShow(), opt ) );
          QVERIFY( opt.selectedPlayer( rcss::rcg::LEFT, 9 ) );

          opt.unselectPlayer();
          QVERIFY( ! selectPlayerNearestBall( makeShow(), opt ) );
          QCOMPARE( opt.selectedNumber(), 0 );
      }
};

QTEST_MAIN( TestPlayerSelection )
#include "test_player_selection.moc"
