// -*-c++-*-

/*!
  \file test_interpolate.cpp
  \brief unit test of Interpolate, the frames between frames of the 3D view
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

#include "interpolate.h"

namespace {

rcss::rcg::DispInfoT
makeFrame( const float ball_x,
           const float player_x,
           const float body )
{
    rcss::rcg::DispInfoT d;
    d.pmode_ = rcss::rcg::PM_PlayOn;
    d.show_.ball_.x_ = ball_x;
    d.show_.ball_.y_ = 1.0f;
    rcss::rcg::PlayerT & p = d.show_.player_[0];
    p.side_ = 'l';
    p.unum_ = 1;
    p.state_ = rcss::rcg::STAND;
    p.x_ = player_x;
    p.y_ = -2.0f;
    p.body_ = body;
    return d;
}

}

class TestInterpolate
    : public QObject {
    Q_OBJECT

private slots:

    void lerpAngle()
      {
          QCOMPARE( Interpolate::lerpAngle( 10.0f, 30.0f, 0.5f ), 20.0f );
          // the short way round
          QCOMPARE( Interpolate::lerpAngle( 170.0f, -170.0f, 0.5f ), -180.0f );
          QCOMPARE( Interpolate::lerpAngle( 170.0f, -170.0f, 0.25f ), 175.0f );
          QCOMPARE( Interpolate::lerpAngle( -170.0f, 170.0f, 0.25f ), -175.0f );
          QCOMPARE( Interpolate::lerpAngle( 10.0f, 30.0f, 0.0f ), 10.0f );
      }

    void midpoint()
      {
          rcss::rcg::DispInfoT a = makeFrame( 0.0f, 10.0f, 170.0f );
          rcss::rcg::DispInfoT b = makeFrame( 2.0f, 11.0f, -170.0f );
          b.show_.ball_.z_ = 1.0f;
          a.show_.player_[0].neck_ = -20.0f;
          b.show_.player_[0].neck_ = 40.0f;

          const rcss::rcg::ShowInfoT s = Interpolate::show( a, &b, 0.5 );
          QCOMPARE( s.ball_.x_, 1.0f );
          QCOMPARE( s.ball_.y_, 1.0f );
          QCOMPARE( s.ball_.z_, 0.5f );
          QCOMPARE( s.player_[0].x_, 10.5f );
          QCOMPARE( s.player_[0].y_, -2.0f );
          QCOMPARE( s.player_[0].body_, -180.0f );
          // the neck is relative to the body: no wrap around
          QCOMPARE( s.player_[0].neck_, 10.0f );

          // the states are a's
          b.show_.player_[0].state_ |= rcss::rcg::KICK;
          QVERIFY( ! Interpolate::show( a, &b, 0.5 ).player_[0].isKicking() );
      }

    void ends()
      {
          const rcss::rcg::DispInfoT a = makeFrame( 0.0f, 10.0f, 0.0f );
          const rcss::rcg::DispInfoT b = makeFrame( 2.0f, 11.0f, 0.0f );

          QCOMPARE( Interpolate::show( a, &b, 0.0 ).ball_.x_, 0.0f );
          QCOMPARE( Interpolate::show( a, nullptr, 0.5 ).ball_.x_, 0.0f );
          QCOMPARE( Interpolate::show( a, &b, 1.0 ).ball_.x_, 2.0f );
          // no farther than b
          QCOMPARE( Interpolate::show( a, &b, 1.5 ).ball_.x_, 2.0f );
      }

    void playModeChange()
      {
          const rcss::rcg::DispInfoT a = makeFrame( 0.0f, 10.0f, 0.0f );
          rcss::rcg::DispInfoT b = makeFrame( 2.0f, 11.0f, 0.0f );
          b.pmode_ = rcss::rcg::PM_KickIn_Left;

          const rcss::rcg::ShowInfoT s = Interpolate::show( a, &b, 0.5 );
          QCOMPARE( s.ball_.x_, 0.0f );
          QCOMPARE( s.player_[0].x_, 10.0f );
      }

    void jumps()
      {
          // the ball placed for a free kick, and a player moved by the referee
          const rcss::rcg::DispInfoT a = makeFrame( 0.0f, 10.0f, 0.0f );
          const rcss::rcg::DispInfoT b = makeFrame( Interpolate::MAX_BALL_STEP + 0.1f,
                                                    10.0f + Interpolate::MAX_PLAYER_STEP + 0.1f,
                                                    90.0f );
          const rcss::rcg::ShowInfoT s = Interpolate::show( a, &b, 0.5 );
          QCOMPARE( s.ball_.x_, 0.0f );
          QCOMPARE( s.player_[0].x_, 10.0f );
          QCOMPARE( s.player_[0].body_, 0.0f );

          // just within the steps
          const rcss::rcg::DispInfoT c = makeFrame( Interpolate::MAX_BALL_STEP - 0.1f,
                                                    10.0f + Interpolate::MAX_PLAYER_STEP - 0.1f,
                                                    90.0f );
          QVERIFY( Interpolate::show( a, &c, 0.5 ).ball_.x_ > 1.0f );
          QVERIFY( Interpolate::show( a, &c, 0.5 ).player_[0].x_ > 11.0f );
      }

    void disconnected()
      {
          const rcss::rcg::DispInfoT a = makeFrame( 0.0f, 10.0f, 0.0f );
          rcss::rcg::DispInfoT b = makeFrame( 2.0f, 11.0f, 0.0f );
          b.show_.player_[0].state_ = 0;
          QCOMPARE( Interpolate::show( a, &b, 0.5 ).player_[0].x_, 10.0f );

          // another player in the slot
          b = makeFrame( 2.0f, 11.0f, 0.0f );
          b.show_.player_[0].unum_ = 2;
          QCOMPARE( Interpolate::show( a, &b, 0.5 ).player_[0].x_, 10.0f );
      }

    void positionOfLog()
      {
          Interpolate::Position p = Interpolate::position( 10, 3, false, 0.25, 0.9 );
          QCOMPARE( p.index, size_t( 3 ) );
          QCOMPARE( p.next, size_t( 4 ) );
          QCOMPARE( p.alpha, 0.25 );

          // the last frame
          p = Interpolate::position( 10, 9, false, 0.25, 0.0 );
          QCOMPARE( p.index, size_t( 9 ) );
          QCOMPARE( p.next, size_t( 9 ) );
          QCOMPARE( p.alpha, 0.0 );

          p = Interpolate::position( 10, 3, false, 1.7, 0.0 );
          QCOMPARE( p.alpha, 1.0 );

          p = Interpolate::position( 0, 0, false, 0.5, 0.5 );
          QCOMPARE( p.index, size_t( 0 ) );
          QCOMPARE( p.alpha, 0.0 );
      }

    void positionOfLive()
      {
          // one cycle late, toward the newest
          Interpolate::Position p = Interpolate::position( 10, 9, true, 0.0, 0.4 );
          QCOMPARE( p.index, size_t( 8 ) );
          QCOMPARE( p.next, size_t( 9 ) );
          QCOMPARE( p.alpha, 0.4 );

          // the next frame is late
          p = Interpolate::position( 10, 9, true, 0.0, 3.0 );
          QCOMPARE( p.alpha, 1.0 );

          // only one frame yet
          p = Interpolate::position( 1, 0, true, 0.0, 0.4 );
          QCOMPARE( p.index, size_t( 0 ) );
          QCOMPARE( p.next, size_t( 0 ) );
          QCOMPARE( p.alpha, 0.0 );
      }
};

QTEST_MAIN( TestInterpolate )
#include "test_interpolate.moc"
