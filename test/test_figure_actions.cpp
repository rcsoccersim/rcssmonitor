// -*-c++-*-

/*!
  \file test_figure_actions.cpp
  \brief unit test of FigureActionLog (what the players do besides walking)
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

#include "figure_actions.h"

#include <cmath>
#include <memory>

namespace {

//! a frame: player 0 (left) at the origin facing +x unless told, the ball where told
struct Frame {
    float ball_x = 0.5f;
    float ball_y = 0.2f;
    float ball_vx = 0.0f;
    float ball_vy = 0.0f;
    int state = rcss::rcg::STAND;
    float x = 0.0f;
    float body = 0.0f;
    char side = 'l';
    float ball_z = 0.0f;
};

DispConstPtr
make( const Frame & f )
{
    auto d = std::make_shared< rcss::rcg::DispInfoT >();
    rcss::rcg::ShowInfoT & s = d->show_;
    s.ball_.x_ = f.ball_x;
    s.ball_.y_ = f.ball_y;
    s.ball_.vx_ = f.ball_vx;
    s.ball_.vy_ = f.ball_vy;
    s.ball_.z_ = f.ball_z;
    rcss::rcg::PlayerT & p = s.player_[0];
    p.side_ = f.side;
    p.unum_ = 1;
    p.state_ = f.state;
    p.x_ = f.x;
    p.y_ = 0.0f;
    p.body_ = f.body;
    return d;
}

//! frames 0 to count - 1, standing, the ball still, then changed by the caller
DispCont
frames( const int count,
        const Frame & base = Frame() )
{
    DispCont c;
    for ( int i = 0; i < count; ++i ) c.push_back( make( base ) );
    return c;
}

const rcss::rcg::ServerParamT &
params()
{
    static rcss::rcg::ServerParamT sp;
    return sp;
}

//! the actions of player 0 at a time (frames with the fraction)
FigureActions
at( const DispCont & c,
    const double time )
{
    const std::size_t index = static_cast< std::size_t >( std::floor( time ) );
    const std::size_t next = std::min( index + 1, c.size() - 1 );
    return FigureActionLog::read( c, 0, index, next, time - index, c[index]->show_.player_[0], params(), 0.2f );
}

//! a kick at frame k sending the ball (vx, vy) per cycle from where it was
void
kickAt( DispCont & c,
        const int k,
        const float vx,
        const float vy,
        Frame base = Frame() )
{
    base.state |= rcss::rcg::KICK;
    base.ball_x += vx;
    base.ball_y += vy;
    base.ball_vx = vx;
    base.ball_vy = vy;
    c[k] = make( base );
}

}

/*!
  \brief unit test of FigureActionLog
*/
class TestFigureActions
    : public QObject {
    Q_OBJECT

private slots:

    /*!
      a kick: its moment the frame before the bit (the ball already moved
      at the bit's frame), begun its lead before it, ended its tail after
    */
    void kickMoment()
      {
          DispCont c = frames( 12 );
          kickAt( c, 5, 1.5f, 0.0f ); // a pass: the moment is 4
          QVERIFY( ! at( c, 3.0 ).kick.on );
          const FigureActions before = at( c, 3.2 ); // 0.8 before: begun (a pass's lead is 0.9)
          QVERIFY( before.kick.on );
          QVERIFY( std::fabs( before.kick.phase + 0.8f ) < 1.0e-5f );
          QCOMPARE( at( c, 4.0 ).kick.phase, 0.0f );
          QCOMPARE( at( c, 4.0 ).kick.kind, KickKind::Pass );
          QVERIFY( at( c, 4.0 + FigureActionLog::KICK_TAIL[1] - 0.01 ).kick.on );
          QVERIFY( ! at( c, 4.0 + FigureActionLog::KICK_TAIL[1] + 0.05 ).kick.on );
          // not moving toward a next frame: at the frame
          const FigureActions still = FigureActionLog::read( c, 0, 3, 3, 0.5, c[3]->show_.player_[0], params(), 0.2f );
          QVERIFY( ! still.kick.on );
          // where the ball was at the moment, its way, and its radius as drawn
          const FigureKick place = at( c, 4.0 ).kick;
          QCOMPARE( place.ball_x, 0.5f );
          QCOMPARE( place.ball_y, 0.2f );
          QCOMPARE( place.dir_x, 1.0f );
          QCOMPARE( place.dir_y, 0.0f );
          QCOMPARE( place.ball_radius, 0.2f );
          // the ball on the player's right (+y facing +x), going straight on
          QVERIFY( at( c, 4.0 ).kick.right );
          QVERIFY( std::fabs( at( c, 4.0 ).kick.way ) < 1.0e-3f );

          // on the left, to the right: the left foot, the way +90
          Frame left;
          left.ball_y = -0.2f;
          DispCont d = frames( 12, left );
          kickAt( d, 5, 0.0f, 1.5f, left );
          QVERIFY( ! at( d, 4.0 ).kick.right );
          QVERIFY( std::fabs( at( d, 4.0 ).kick.way - 90.0f ) < 1.0e-3f );
          QCOMPARE( at( d, 4.0 ).kick.dir_y, 1.0f );
          QCOMPARE( at( d, 4.0 ).kick.dir_x, 0.0f );
          // facing +y: the same ball is ahead, on its left
          Frame up = left;
          up.body = 90.0f;
          d = frames( 12, up );
          kickAt( d, 5, 0.0f, 1.5f, up );
          QVERIFY( std::fabs( at( d, 4.0 ).kick.way ) < 1.0e-3f );
          QVERIFY( ! at( d, 4.0 ).kick.right ); // the ball at (0.5, -0.2) is on the left of facing +y
      }

    /*!
      kicks in a row are one, at the one sending the ball fastest; of two
      kicks, the latest begun
    */
    void kickRuns()
      {
          DispCont c = frames( 14 );
          kickAt( c, 5, 0.5f, 0.0f );
          kickAt( c, 6, 1.6f, 0.0f );
          kickAt( c, 7, 0.3f, 0.0f );
          QCOMPARE( at( c, 5.0 ).kick.phase, 0.0f ); // the moment of frame 6's kick
          QCOMPARE( at( c, 5.0 ).kick.kind, KickKind::Pass );

          // two kicks apart: the second one from its lead on
          DispCont d = frames( 14 );
          kickAt( d, 5, 1.5f, 0.0f ); // a pass: the moment 4, its tail 1.3
          kickAt( d, 7, 0.5f, 0.0f ); // a touch: the moment 6, its lead 0.6
          QVERIFY( std::fabs( at( d, 5.2 ).kick.phase - 1.2f ) < 1.0e-5f ); // the first one still
          QVERIFY( std::fabs( at( d, 5.5 ).kick.phase + 0.5f ) < 1.0e-5f ); // the second begun
          QCOMPARE( at( d, 5.5 ).kick.kind, KickKind::Tap );
      }

    /*!
      the kind: a touch below PASS_SPEED, a shot at SHOT_SPEED toward the
      opponents' goal, a pass else
    */
    void kickKind()
      {
          Frame f;
          f.x = 30.0f;
          f.ball_x = 30.5f;
          f.ball_y = 0.0f;
          auto kind = [&]( const char side,
                           const float vx,
                           const float vy )
              {
                  Frame g = f;
                  g.side = side;
                  DispCont c = frames( 10, g );
                  kickAt( c, 5, vx, vy, g );
                  return at( c, 4.0 ).kick.kind;
              };
          QCOMPARE( kind( 'l', 0.5f, 0.0f ), KickKind::Tap );
          QCOMPARE( kind( 'l', 1.2f, 0.0f ), KickKind::Pass );
          QCOMPARE( kind( 'l', 2.5f, 0.0f ), KickKind::Shot );
          QCOMPARE( kind( 'l', 2.5f, 0.4f ), KickKind::Shot ); // within the posts and the margin
          QCOMPARE( kind( 'l', 2.5f, 1.5f ), KickKind::Pass ); // wide of the goal
          QCOMPARE( kind( 'l', -2.5f, 0.0f ), KickKind::Pass ); // toward its own goal
          QCOMPARE( kind( 'r', -2.5f, 0.0f ), KickKind::Pass ); // too far (80 m)
          f.x = -30.0f;
          f.ball_x = -30.5f;
          QCOMPARE( kind( 'r', -2.5f, 0.0f ), KickKind::Shot );
      }

    /*!
      a tackle and a foul: from the frame before the bit, as long as the
      bit lasts, and their tails
    */
    void runs()
      {
          for ( const int bit : { int( rcss::rcg::TACKLE ), int( rcss::rcg::FOUL_CHARGED ) } )
          {
              DispCont c = frames( 16 );
              for ( int i = 5; i <= 9; ++i )
              {
                  Frame f;
                  f.state |= bit;
                  c[i] = make( f );
              }
              auto run = [&]( const double t ) { const FigureActions a = at( c, t ); return ( bit == rcss::rcg::TACKLE ? a.tackle : a.fouled ); };
              const bool tackle = ( bit == rcss::rcg::TACKLE );
              const float tail = ( tackle ? FigureActionLog::TACKLE_TAIL : FigureActionLog::FOULED_TAIL );
              if ( tackle )
              {
                  QVERIFY( ! run( 3.3 ).on );
                  QVERIFY( run( 3.5 ).on ); // the lead is 0.6
                  QCOMPARE( run( 4.0 ).phase, 0.0f );
              }
              else
              {
                  // the fouled one from after the tackler slid in
                  QVERIFY( ! run( 4.0 ).on );
                  QVERIFY( ! run( 4.9 ).on );
                  QVERIFY( run( 5.0 ).on );
                  QCOMPARE( run( 5.0 ).phase, FigureActionLog::FOULED_START );
              }
              QCOMPARE( run( 5.0 ).last, 4.0f );
              QCOMPARE( run( 8.0 ).phase, 4.0f );
              QVERIFY( run( 4.0 + 4.0 + tail - 0.05 ).on );
              QVERIFY( ! run( 4.0 + 4.0 + tail + 0.05 ).on );
              // one frame (a tackle that is a foul): last 0
              DispCont one = frames( 12 );
              Frame f;
              f.state |= bit;
              one[5] = make( f );
              const FigureActions a = at( one, 5.5 );
              QCOMPARE( ( bit == rcss::rcg::TACKLE ? a.tackle : a.fouled ).last, 0.0f );
              QVERIFY( ! a.kick.on );
          }
      }

    //! a catch: where the ball was, held for catch_ban_cycle
    void caught()
      {
          Frame f;
          f.ball_x = 1.0f;
          f.ball_y = -1.2f; // ahead, on the left
          f.ball_z = 0.7f;
          DispCont c = frames( 16, f );
          Frame hold = f;
          hold.state |= rcss::rcg::CATCH;
          c[5] = make( hold );
          const float ban = static_cast< float >( params().catch_ban_cycle_ );
          QVERIFY( ! at( c, 2.9 ).caught.on );
          const FigureCatch a = at( c, 4.0 ).caught;
          QVERIFY( a.on );
          QCOMPARE( a.phase, 0.0f );
          QCOMPARE( a.hold, ban );
          QVERIFY( std::fabs( a.side + 1.2f ) < 1.0e-5f );
          QVERIFY( std::fabs( a.way - std::atan2( -1.2f, 1.0f ) * 180.0f / float( M_PI ) ) < 1.0e-3f );
          QCOMPARE( a.height, 0.7f );
          QVERIFY( at( c, 4.0 + ban + FigureActionLog::CATCH_TAIL - 0.05 ).caught.on );
          QVERIFY( ! at( c, 4.0 + ban + FigureActionLog::CATCH_TAIL + 0.05 ).caught.on );
      }

    //! the neck and the arm, from the player drawn; nothing for a player gone
    void drawn()
      {
          DispCont c = frames( 3 );
          rcss::rcg::PlayerT p = c[1]->show_.player_[0];
          FigureActions a = FigureActionLog::read( c, 0, 1, 2, 0.0, p, params(), 0.2f );
          QCOMPARE( a.neck, 0.0f );
          QVERIFY( ! a.pointing );
          p.neck_ = 30.0f;
          p.point_x_ = 0.0f;
          p.point_y_ = 10.0f; // to the right of facing +x
          p.body_ = 0.0f;
          a = FigureActionLog::read( c, 0, 1, 2, 0.0, p, params(), 0.2f );
          QCOMPARE( a.neck, 30.0f );
          QVERIFY( a.pointing );
          QVERIFY( std::fabs( a.point_way - 90.0f ) < 1.0e-3f );
          QCOMPARE( a.point_distance, 10.0f );

          // not connected at the kick's frame (no bits): no kick
          DispCont d = frames( 12 );
          kickAt( d, 5, 1.5f, 0.0f );
          QVERIFY( at( d, 4.0 ).kick.on );
          rcss::rcg::DispInfoT gone = *d[5];
          gone.show_.player_[0].state_ = 0;
          d[5] = std::make_shared< rcss::rcg::DispInfoT >( gone );
          QVERIFY( ! at( d, 4.0 ).kick.on );
          // out of the frames, another player
          kickAt( d, 5, 1.5f, 0.0f );
          QVERIFY( ! FigureActionLog::read( d, 0, 20, 20, 0.0, p, params(), 0.2f ).kick.on );
          QVERIFY( ! FigureActionLog::read( d, 22, 4, 5, 0.0, p, params(), 0.2f ).kick.on );
          QVERIFY( ! FigureActionLog::read( d, 1, 4, 5, 0.0, p, params(), 0.2f ).kick.on );
      }
};

QTEST_MAIN( TestFigureActions )

#include "test_figure_actions.moc"
