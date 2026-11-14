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
          // two cycles late (the figures' actions look two frames ahead), toward the next
          QCOMPARE( Interpolate::LIVE_DELAY, size_t( 2 ) );
          Interpolate::Position p = Interpolate::position( 10, 9, true, 0.0, 0.4 );
          QCOMPARE( p.index, size_t( 7 ) );
          QCOMPARE( p.next, size_t( 8 ) );
          QCOMPARE( p.alpha, 0.4 );

          // two frames yet: as late as they allow
          p = Interpolate::position( 2, 1, true, 0.0, 0.4 );
          QCOMPARE( p.index, size_t( 0 ) );
          QCOMPARE( p.next, size_t( 1 ) );
          p = Interpolate::position( 3, 2, true, 0.0, 0.4 );
          QCOMPARE( p.index, size_t( 0 ) );
          QCOMPARE( p.next, size_t( 1 ) );

          // the next frame is late: on into the frames there, up to the newest
          p = Interpolate::position( 10, 9, true, 0.0, 1.5 );
          QCOMPARE( p.index, size_t( 8 ) );
          QCOMPARE( p.next, size_t( 9 ) );
          QCOMPARE( p.alpha, 0.5 );
          p = Interpolate::position( 10, 9, true, 0.0, 3.0 );
          QCOMPARE( p.index, size_t( 9 ) );
          QCOMPARE( p.next, size_t( 9 ) );
          QCOMPARE( p.alpha, 0.0 );
          QCOMPARE( Interpolate::liveTime( 10, 0.4 ), 7.4 );
          QCOMPARE( Interpolate::liveTime( 10, 5.0 ), 9.0 );
          QCOMPARE( Interpolate::liveTime( 2, 0.4 ), 0.4 );
          QCOMPARE( Interpolate::liveTime( 2, 2.0 ), 1.0 );

          // the frames at a time
          p = Interpolate::positionAt( 10, 4.25 );
          QCOMPARE( p.index, size_t( 4 ) );
          QCOMPARE( p.next, size_t( 5 ) );
          QCOMPARE( p.alpha, 0.25 );
          p = Interpolate::positionAt( 10, 12.0 );
          QCOMPARE( p.index, size_t( 9 ) );
          QCOMPARE( p.alpha, 0.0 );
          p = Interpolate::positionAt( 10, -1.0 );
          QCOMPARE( p.index, size_t( 0 ) );
          QCOMPARE( p.next, size_t( 1 ) );
          QCOMPARE( p.alpha, 0.0 );

          // only one frame yet
          p = Interpolate::position( 1, 0, true, 0.0, 0.4 );
          QCOMPARE( p.index, size_t( 0 ) );
          QCOMPARE( p.next, size_t( 0 ) );
          QCOMPARE( p.alpha, 0.0 );
      }

    /*!
      the live clock: the newest frame seen at the next drawing (60 times a
      second, a frame every 0.1 s): where the frames put the time jumps at
      every frame, the clock goes on smoothly; it catches up, never goes
      back, jumps when far, and stops at the newest frame
    */
    void liveClock()
      {
          Interpolate::LiveClock clock;
          QCOMPARE( clock.advance( 7.4, 0.0, 9.0 ), 7.4 ); // the first: there

          // 60 drawings a second; frame k comes at 0.1 k s, seen at the next drawing
          clock.reset();
          const double draw = 1.0 / 60.0; // s
          const double cycle = 0.1; // s
          double raw_least = 1.0e9, raw_most = 0.0, least = 1.0e9, most = 0.0;
          double last_raw = -1.0, last = -1.0;
          int frames = 10;
          double seen = 0.9; // when the newest frame was seen
          for ( int k = 0; k < 600; ++k )
          {
              const double now = 1.0 + k * draw;
              // the frames come every cycle; one is seen at the first drawing after it
              while ( ( frames + 1 ) * cycle <= now )
              {
                  ++frames;
                  seen = now;
              }
              const double target = Interpolate::liveTime( frames, ( now - seen ) / cycle );
              const double time = clock.advance( target, draw / cycle, frames - 1 );
              if ( k > 60 ) // settled
              {
                  raw_least = std::min( raw_least, target - last_raw );
                  raw_most = std::max( raw_most, target - last_raw );
                  least = std::min( least, time - last );
                  most = std::max( most, time - last );
              }
              last_raw = target;
              last = time;
              // two frames behind the newest, give or take
              QVERIFY( k < 60 || std::fabs( time - ( frames - 1 - 2 ) ) < 1.2 );
          }
          // where the frames put it jumps (a drawing is a sixth of a cycle)
          QVERIFY2( raw_least < 0.05 || raw_most > 0.3, qPrintable( QString( "%1 %2" ).arg( raw_least ).arg( raw_most ) ) );
          // the clock goes on by nearly a sixth of a cycle at every drawing
          QVERIFY2( least > 0.12 && most < 0.22, qPrintable( QString( "%1 %2" ).arg( least ).arg( most ) ) );

          // ahead of where the frames put it: slower, not back
          clock.reset();
          clock.advance( 20.0, 0.0, 30.0 );
          const double ahead = clock.advance( 19.0, 0.2, 30.0 );
          QVERIFY( ahead > 20.0 && ahead < 20.2 );
          // far: jumps (another game, the oldest frames dropped)
          clock.reset();
          clock.advance( 20.0, 0.0, 30.0 );
          QCOMPARE( clock.advance( 5.0, 0.1, 30.0 ), 5.0 );
          // not beyond the newest frame, nor more than MAX_ELAPSED at once
          clock.reset();
          clock.advance( 9.5, 0.0, 10.0 );
          QCOMPARE( clock.advance( 9.5, 5.0, 10.0 ), 10.0 );
          clock.reset();
          clock.advance( 5.0, 0.0, 30.0 );
          QCOMPARE( clock.advance( 5.0, 10.0, 30.0 ), 5.0 + Interpolate::LiveClock::MAX_ELAPSED );
      }
};

QTEST_MAIN( TestInterpolate )
#include "test_interpolate.moc"
