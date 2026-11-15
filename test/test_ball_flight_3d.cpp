// -*-c++-*-

/*!
  \file test_ball_flight_3d.cpp
  \brief unit test of the ball flying in the air in log replay, virtually
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

#include "ball_aids_3d.h"
#include "ball_flight_3d.h"
#include "figure_actions.h"
#include "options.h"

#include <cmath>
#include <memory>

namespace {

constexpr float RADIUS = 0.11f;
constexpr double G = 9.8 * 0.01; // m/cycle^2 at 100 ms a cycle

/*!
  \class Log
  \brief frames made one by one: the ball's place, the play mode, and the
  players with their bits
*/
class Log {
public:
    DispCont frames;
    unsigned int start = 0; //!< the game's time of the first frame (the variety of the kicks)

    //! a frame with the ball at (x, y) and every player standing far away
    rcss::rcg::DispInfoT & add( const float x,
                                const float y,
                                const rcss::rcg::PlayMode mode = rcss::rcg::PM_PlayOn )
      {
          auto d = std::make_shared< rcss::rcg::DispInfoT >();
          d->pmode_ = mode;
          d->show_.time_ = start + static_cast< unsigned int >( frames.size() );
          d->show_.ball_.x_ = x;
          d->show_.ball_.y_ = y;
          for ( int i = 0; i < rcss::rcg::MAX_PLAYER * 2; ++i )
          {
              rcss::rcg::PlayerT & p = d->show_.player_[i];
              const bool left = ( i < rcss::rcg::MAX_PLAYER );
              p.side_ = ( left ? 'l' : 'r' );
              p.unum_ = static_cast< rcss::rcg::Int16 >( i % rcss::rcg::MAX_PLAYER + 1 );
              p.state_ = rcss::rcg::STAND;
              p.x_ = ( left ? -60.0f : 60.0f ); // off the pitch, away from the ball
              p.y_ = -30.0f + 2.0f * float( i % rcss::rcg::MAX_PLAYER );
          }
          frames.push_back( d );
          return *d;
      }

    rcss::rcg::DispInfoT & at( const std::size_t i ) { return const_cast< rcss::rcg::DispInfoT & >( *frames[i] ); }

    //! the ball kicked from (x, y) toward (dx, dy) (a unit way) at speed, decaying 0.94 a cycle, for cycles
    void roll( float x,
               float y,
               const float dx,
               const float dy,
               float speed,
               const int cycles,
               const rcss::rcg::PlayMode mode = rcss::rcg::PM_PlayOn )
      {
          for ( int c = 0; c < cycles; ++c )
          {
              x += dx * speed;
              y += dy * speed;
              speed *= 0.94f;
              add( x, y, mode );
          }
      }

    //! player p touches the ball at frame k: its bit at frame k + 1, standing by the ball at both
    void touch( const std::size_t k,
                const int p,
                const rcss::rcg::Int32 bit = rcss::rcg::KICK )
      {
          // the frame of the bit, the ball still there, if there is none yet
          if ( frames.size() <= k + 1 )
          {
              add( frames.back()->show_.ball_.x_, frames.back()->show_.ball_.y_, frames.back()->pmode_ );
          }
          for ( const std::size_t f : { k, k + 1 } )
          {
              rcss::rcg::PlayerT & pl = at( f ).show_.player_[p];
              pl.x_ = at( k ).show_.ball_.x_ - 0.5f;
              pl.y_ = at( k ).show_.ball_.y_;
          }
          at( k + 1 ).show_.player_[p].state_ |= bit;
      }

    //! the last frame
    std::size_t last() const { return frames.size() - 1; }
};

const BallFlight::Segment *
segmentFrom( const BallFlight & flight,
             const std::size_t first )
{
    for ( const BallFlight::Segment & s : flight.segments() )
    {
        if ( s.first == first ) return &s;
    }
    return nullptr;
}

//! the server parameters of the tests (not copyable: one of them)
const rcss::rcg::ServerParamT &
params()
{
    static rcss::rcg::ServerParamT sp;
    sp.simulator_step_ = 100;
    sp.ball_decay_ = 0.94;
    sp.goal_width_ = 14.02;
    return sp;
}

//! a long ball: the left 5 kicks at frame 0 from (-10, 0) along +x, 2.5 m a cycle, the left 9 kicks it at frame 15 (about 24 m away)
Log
longBall( const int receiver_bit = rcss::rcg::KICK,
          const int receiver = 8,
          const unsigned int start = 0 )
{
    Log log;
    log.start = start;
    log.add( -10.0f, 0.0f );
    log.roll( -10.0f, 0.0f, 1.0f, 0.0f, 2.5f, 15 );
    log.roll( log.frames.back()->show_.ball_.x_, 0.0f, -1.0f, 0.0f, 1.0f, 5 ); // and away again
    log.touch( 0, 4 );
    log.touch( 15, receiver, receiver_bit );
    return log;
}

}

/*!
  \brief unit test of BallFlight
*/
class TestBallFlight3D
    : public QObject {
    Q_OBJECT

private slots:

    /*!
      the height of a segment: up and down under gravity from h0 at v,
      bouncing RESTITUTION times as fast up, then rolling; never under the
      ground
    */
    void height()
      {
          BallFlight::Segment s;
          QCOMPARE( BallFlight::height( s, 3.0, G ), 0.0f );
          s.v = 0.49f; // the top after 5 cycles, 1.225 m up, down after 10
          QVERIFY( std::fabs( BallFlight::height( s, 5.0, G ) - 1.225f ) < 1.0e-4f );
          QVERIFY( std::fabs( BallFlight::height( s, 2.0, G ) - float( 0.49 * 2 - 0.5 * G * 4 ) ) < 1.0e-5f );
          QVERIFY( BallFlight::height( s, 10.0, G ) < 1.0e-4f );
          // the first bounce: 0.6 times as fast up, 0.36 times as high, 6 cycles long
          QVERIFY( std::fabs( BallFlight::height( s, 13.0, G ) - 1.225f * 0.36f ) < 1.0e-3f );
          QVERIFY( BallFlight::height( s, 16.0, G ) < 1.0e-4f );
          // then rolling
          QCOMPARE( BallFlight::height( s, 60.0, G ), 0.0f );
          for ( double t = 0.0; t < 40.0; t += 0.25 ) QVERIFY( BallFlight::height( s, t, G ) >= 0.0f );
          // from a height, dropped
          s.v = 0.0f;
          s.h0 = 0.441f; // down after 3 cycles
          QCOMPARE( BallFlight::height( s, 0.0, G ), 0.441f );
          QVERIFY( BallFlight::height( s, 3.0, G ) < 1.0e-4f );
          QVERIFY( BallFlight::height( s, 1.5, G ) > 0.0f );
      }

    /*!
      a long ball, kicked fast and far to a teammate: lofted from the
      ground, its highest point near the one preferred, and low enough at
      the next kick for a foot; the next segment from there; the frames
      between at heights on the arc
    */
    void longBall()
      {
          const Log log = ::longBall();
          BallFlight flight;
          flight.update( log.frames, params(), RADIUS );
          QVERIFY( flight.active() );
          const BallFlight::Segment * s = segmentFrom( flight, 0 );
          QVERIFY( s );
          QCOMPARE( s->last, std::size_t( 15 ) );
          QCOMPARE( s->start, BallFlight::Touch::Kick );
          QCOMPARE( s->end, BallFlight::Touch::Kick );
          QCOMPARE( s->loft, BallFlight::Loft::LongBall );
          QCOMPARE( s->h0, 0.0f );
          const float far = std::hypot( log.frames[15]->show_.ball_.x_ - log.frames[0]->show_.ball_.x_, 0.0f );
          QVERIFY( far >= BallFlight::LONG_BALL );
          QCOMPARE( flight.heightAtFrame( 0 ), 0.0f );
          float top = 0.0f;
          for ( std::size_t k = 1; k < 15; ++k ) top = std::max( top, flight.heightAtFrame( k ) );
          QVERIFY( top > 0.5f );
          QVERIFY( flight.heightAtFrame( 15 ) <= BallFlight::FOOT_REACH + 1.0e-4f );
          // the next segment from the height it arrived at
          const BallFlight::Segment * next = segmentFrom( flight, 15 );
          QVERIFY( next );
          QCOMPARE( next->h0, BallFlight::height( *s, 15.0, G ) );
          QCOMPARE( next->loft, BallFlight::Loft::None ); // slow
          // between two frames, on the arc: continuous
          for ( std::size_t k = 0; k < 15; ++k )
          {
              QVERIFY( std::fabs( flight.heightAt( k, 1.0 ) - flight.heightAt( k + 1, 0.0 ) ) < 1.0e-4f );
              const float mid = flight.heightAt( k, 0.5 );
              QCOMPARE( mid, BallFlight::height( *s, double( k ) + 0.5, G ) );
          }
          // a catch at the end: as high as the hands reach
          const Log caught = ::longBall( rcss::rcg::CATCH, 11 );
          flight.update( caught.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->end, BallFlight::Touch::Catch );
          QVERIFY( flight.heightAtFrame( 15 ) <= BallFlight::CATCH_REACH + 1.0e-4f );
      }

    /*!
      the long ones vary from kick to kick (by the time and the kicker):
      some land halfway, at MIN_LANDING to MAX_LANDING of the time to the
      next touch, and bounce on to it; the others fly to it, their highest
      point near the one preferred. Both come in a few kicks; the same kick
      is the same every time.
    */
    void bounces()
      {
          QCOMPARE( BallFlight::variety( 120, 0, 4, 0 ), BallFlight::variety( 120, 0, 4, 0 ) );
          QVERIFY( BallFlight::variety( 120, 0, 4, 0 ) != BallFlight::variety( 121, 0, 4, 0 ) );
          QVERIFY( BallFlight::variety( 120, 0, 4, 0 ) != BallFlight::variety( 120, 0, 5, 0 ) );
          QVERIFY( BallFlight::variety( 120, 0, 4, 0 ) != BallFlight::variety( 120, 0, 4, 1 ) );
          int bounced = 0, flew = 0, n = 0;
          for ( unsigned int start = 0; start < 200; start += 7 )
          {
              const Log log = ::longBall( rcss::rcg::KICK, 8, start );
              BallFlight flight;
              flight.update( log.frames, params(), RADIUS );
              const BallFlight::Segment * s = segmentFrom( flight, 0 );
              QCOMPARE( s->loft, BallFlight::Loft::LongBall );
              QCOMPARE( s->bounce, BallFlight::variety( start, 0, 4, 0 ) < BallFlight::BOUNCE_SHARE );
              QVERIFY( flight.heightAtFrame( 15 ) <= BallFlight::FOOT_REACH + 1.0e-4f );
              const double land = 2.0 * s->v / G;
              if ( s->bounce )
              {
                  ++bounced;
                  // landed where preferred, or as near as the next touch's
                  // reach allows (a later landing bounces higher there), and
                  // up again before the next touch
                  const double share = BallFlight::MIN_LANDING
                      + ( BallFlight::MAX_LANDING - BallFlight::MIN_LANDING ) * BallFlight::variety( start, 0, 4, 1 );
                  QVERIFY2( land <= 15.0 * share + 0.1, qPrintable( QString( "%1 %2" ).arg( land ).arg( 15.0 * share ) ) );
                  QVERIFY( land >= 15.0 * BallFlight::MIN_LANDING - 1.5 );
                  if ( std::fabs( land - 15.0 * share ) > 0.1 )
                  {
                      // a little later would be too high at the next touch
                      BallFlight::Segment later = *s;
                      later.v += 0.01f;
                      QVERIFY( BallFlight::height( later, 15.0, G ) > BallFlight::FOOT_REACH );
                  }
                  QVERIFY( BallFlight::height( *s, land + 1.0, G ) > 0.0f );
              }
              else
              {
                  ++flew;
                  const float far = log.frames[15]->show_.ball_.x_ - log.frames[0]->show_.ball_.x_;
                  const float apex = s->v * s->v / float( 2.0 * G );
                  QVERIFY2( apex <= BallFlight::APEX_PER_DISTANCE * far + 0.05f, qPrintable( QString::number( apex ) ) );
              }
              ++n;
          }
          QVERIFY2( bounced > n / 3 && flew > n / 6, qPrintable( QString( "%1 %2" ).arg( bounced ).arg( flew ) ) );
      }

    /*!
      a lift: a fast kick without a reason to loft goes a little up (its
      highest point MIN_LIFT to MAX_LIFT, by the kick), lands, bounces low
    */
    void lift()
      {
          for ( unsigned int start = 0; start < 50; start += 5 )
          {
              Log pass;
              pass.start = start;
              pass.add( 0.0f, 0.0f );
              pass.roll( 0.0f, 0.0f, 1.0f, 0.0f, 1.6f, 7 ); // about 9.6 m
              pass.touch( 0, 4 );
              pass.touch( 7, 8 );
              BallFlight flight;
              flight.update( pass.frames, params(), RADIUS );
              const BallFlight::Segment * s = segmentFrom( flight, 0 );
              QCOMPARE( s->loft, BallFlight::Loft::Lift );
              QVERIFY( ! s->bounce );
              const float apex = s->v * s->v / float( 2.0 * G );
              const float preferred = float( BallFlight::MIN_LIFT
                                             + ( BallFlight::MAX_LIFT - BallFlight::MIN_LIFT ) * BallFlight::variety( start, 0, 4, 0 ) );
              QVERIFY2( std::fabs( apex - preferred ) < 0.02f, qPrintable( QString( "%1 %2" ).arg( apex ).arg( preferred ) ) );
              QVERIFY( apex >= BallFlight::MIN_LIFT - 0.02f && apex <= BallFlight::MAX_LIFT + 0.02f );
              QVERIFY( flight.heightAtFrame( 7 ) <= BallFlight::FOOT_REACH + 1.0e-4f );
          }
      }

    /*!
      on the ground: a short slow pass, a slow kick, a tackle's touch, and a
      lofted one that cannot meet its constraints
    */
    void ground()
      {
          // a short slow pass, about 5 m
          Log short_pass;
          short_pass.add( 0.0f, 0.0f );
          short_pass.roll( 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 6 );
          short_pass.touch( 0, 4 );
          short_pass.touch( 6, 8 );
          BallFlight flight;
          flight.update( short_pass.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->loft, BallFlight::Loft::None );
          for ( std::size_t k = 0; k <= short_pass.last(); ++k ) QCOMPARE( flight.heightAtFrame( k ), 0.0f );

          // slow and far: rolled
          Log slow;
          slow.add( 0.0f, 0.0f );
          slow.roll( 0.0f, 0.0f, 1.0f, 0.0f, 1.1f, 40 );
          slow.touch( 0, 4 );
          flight.update( slow.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->loft, BallFlight::Loft::None );

          // a tackle's touch is not a kick
          Log tackle = ::longBall();
          tackle.at( 1 ).show_.player_[4].state_ = rcss::rcg::STAND | rcss::rcg::TACKLE;
          flight.update( tackle.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->start, BallFlight::Touch::Tackle );
          QCOMPARE( segmentFrom( flight, 0 )->loft, BallFlight::Loft::None );

          // over an opponent 3 cycles after the kick, and at the next kick 3
          // later low: it cannot go up and down so fast, stays on the ground
          Log tight;
          tight.add( 0.0f, 0.0f );
          tight.roll( 0.0f, 0.0f, 1.0f, 0.0f, 2.0f, 6 );
          tight.touch( 0, 4 );
          tight.touch( 6, 8 );
          tight.at( 3 ).show_.player_[15].x_ = tight.frames[3]->show_.ball_.x_;
          tight.at( 3 ).show_.player_[15].y_ = 1.0f;
          flight.update( tight.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->loft, BallFlight::Loft::None );
          QCOMPARE( flight.heightAtFrame( 3 ), 0.0f );
      }

    /*!
      over the opponents: a pass not long enough to be a long ball, past an
      opponent near its way that did not touch it, over its head then
    */
    void lane()
      {
          Log log;
          log.add( 0.0f, 0.0f );
          log.roll( 0.0f, 0.0f, 1.0f, 0.0f, 2.0f, 12 ); // about 17 m
          log.touch( 0, 4 );
          log.touch( 12, 8 );
          const std::size_t past = 6;
          log.at( past ).show_.player_[15].x_ = log.frames[past]->show_.ball_.x_ + 0.3f;
          log.at( past ).show_.player_[15].y_ = 1.5f;
          BallFlight flight;
          flight.update( log.frames, params(), RADIUS );
          const BallFlight::Segment * s = segmentFrom( flight, 0 );
          QCOMPARE( s->loft, BallFlight::Loft::Lane );
          QVERIFY( flight.heightAtFrame( past ) >= BallFlight::HEAD_CLEAR );
          QVERIFY( flight.heightAtFrame( 12 ) <= BallFlight::FOOT_REACH + 1.0e-4f );
          // a teammate there: no reason
          log.at( past ).show_.player_[15].x_ = 50.0f;
          log.at( past ).show_.player_[6].x_ = log.frames[past]->show_.ball_.x_ + 0.3f;
          log.at( past ).show_.player_[6].y_ = 1.5f;
          flight.clear(); // the same frames, changed
          flight.update( log.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->loft, BallFlight::Loft::Lift ); // only a little up
          QVERIFY( flight.heightAtFrame( past ) < 1.0f );
      }

    /*!
      the set pieces and the crosses: a corner kick, which changes the play
      mode itself (not a placement); a cross from a wing into the penalty
      area
    */
    void setPieces()
      {
          Log corner;
          corner.add( 51.5f, -33.0f, rcss::rcg::PM_CornerKick_Left );
          corner.roll( 51.5f, -33.0f, -0.4f, 0.9165f, 2.2f, 10 ); // about 16 m toward the goal
          corner.touch( 0, 4 );
          corner.touch( 10, 8 );
          BallFlight flight;
          flight.update( corner.frames, params(), RADIUS );
          const BallFlight::Segment * s = segmentFrom( flight, 0 );
          QVERIFY( s );
          QCOMPARE( s->last, std::size_t( 10 ) ); // not cut at the play mode change
          QCOMPARE( s->loft, BallFlight::Loft::SetPiece );
          float top = 0.0f;
          for ( std::size_t k = 1; k < 10; ++k ) top = std::max( top, flight.heightAtFrame( k ) );
          QVERIFY( top > 0.2f );

          const auto cross = []( const int kicker, const int receiver ) {
              Log log;
              log.add( 40.0f, 28.0f );
              log.roll( 40.0f, 28.0f, 0.12f, -0.9928f, 2.2f, 10 ); // into the area
              log.touch( 0, kicker );
              log.touch( 10, receiver );
              return log;
          };
          const Log left = cross( 4, 8 );
          flight.update( left.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->loft, BallFlight::Loft::Cross );
          // the right team crossing from there: away from the goal it attacks
          const Log right = cross( 15, 19 );
          flight.update( right.frames, params(), RADIUS );
          QVERIFY( segmentFrom( flight, 0 )->loft != BallFlight::Loft::Cross );
      }

    /*!
      a shot into the goal: a little in the air, under the crossbar where it
      crosses the goal line; a long ball into it too
    */
    void shots()
      {
          // (kicker, from, speed): a shot by the left team; a long one into
          // its own goal by the right team, which wants to go higher than
          // the crossbar (a long ball, not a shot)
          struct Case {
              int kicker;
              float from;
              float speed;
              BallFlight::Loft loft;
          };
          for ( const Case c : { Case{ 4, 36.0f, 2.2f, BallFlight::Loft::Shot },
                                 Case{ 15, 25.0f, 3.0f, BallFlight::Loft::LongBall } } )
          {
              Log shot;
              shot.add( c.from, 0.0f );
              shot.roll( c.from, 0.0f, 1.0f, 0.02f, c.speed, 14 ); // over the goal line
              shot.touch( 0, c.kicker );
              for ( std::size_t k = 1; k <= shot.last(); ++k )
              {
                  if ( shot.frames[k]->show_.ball_.x_ > 52.5f ) shot.at( k ).pmode_ = rcss::rcg::PM_AfterGoal_Left;
              }
              shot.add( 0.0f, 0.0f, rcss::rcg::PM_AfterGoal_Left ); // placed at the center
              BallFlight flight;
              flight.update( shot.frames, params(), RADIUS );
              const BallFlight::Segment * s = segmentFrom( flight, 0 );
              QVERIFY( s );
              QCOMPARE( s->loft, c.loft );
              // where it crosses the line
              std::size_t k = 0;
              while ( shot.frames[k + 1]->show_.ball_.x_ < 52.5f ) ++k;
              const float a = shot.frames[k]->show_.ball_.x_, b = shot.frames[k + 1]->show_.ball_.x_;
              const double crossing = double( k ) + ( 52.5 - a ) / ( b - a );
              QVERIFY( BallFlight::height( *s, crossing, G ) <= BallFlight::CROSSBAR - 2.0f * RADIUS );
              QVERIFY( flight.heightAtFrame( k ) > 0.0f );
              // placed: the next from the ground
              QCOMPARE( flight.heightAtFrame( shot.last() ), 0.0f );
          }

          // a shot from near the goal over a defender 3 cycles after the
          // kick: it cannot go over it and still under the crossbar at the
          // line a few cycles later; on the ground
          Log close;
          close.add( 40.0f, 0.0f );
          close.roll( 40.0f, 0.0f, 1.0f, 0.0f, 2.5f, 9 );
          close.touch( 0, 4 );
          for ( std::size_t k = 1; k <= close.last(); ++k )
          {
              if ( close.frames[k]->show_.ball_.x_ > 52.5f ) close.at( k ).pmode_ = rcss::rcg::PM_AfterGoal_Left;
          }
          close.at( 3 ).show_.player_[12].x_ = close.frames[3]->show_.ball_.x_;
          close.at( 3 ).show_.player_[12].y_ = 1.0f;
          BallFlight flight;
          flight.update( close.frames, params(), RADIUS );
          const BallFlight::Segment * s = segmentFrom( flight, 0 );
          QVERIFY( s );
          QCOMPARE( s->end, BallFlight::Touch::PlayMode );
          QCOMPARE( s->loft, BallFlight::Loft::None );
          QCOMPARE( flight.heightAtFrame( 3 ), 0.0f );
      }

    /*!
      a long ball ending where the ball was placed (a jump): as high as the
      way it went before, not the jump, asks
    */
    void placedAtEnd()
      {
          // a kick that flies to the end (not bouncing: its variety)
          unsigned int start = 0;
          while ( BallFlight::variety( start, 0, 4, 0 ) < BallFlight::BOUNCE_SHARE ) ++start;
          Log log;
          log.start = start;
          log.add( -10.0f, 0.0f );
          log.roll( -10.0f, 0.0f, 1.0f, 0.0f, 3.0f, 16 ); // about 31 m
          log.touch( 0, 4 );
          log.add( -30.0f, 0.0f ); // placed back, 50 m
          BallFlight flight;
          flight.update( log.frames, params(), RADIUS );
          const BallFlight::Segment * s = segmentFrom( flight, 0 );
          QVERIFY( s );
          QCOMPARE( s->end, BallFlight::Touch::PlayMode );
          QCOMPARE( s->loft, BallFlight::Loft::LongBall );
          QVERIFY( ! s->bounce );
          const float far = log.frames[16]->show_.ball_.x_ - log.frames[0]->show_.ball_.x_;
          const float apex = s->v * s->v / float( 2.0 * G );
          QVERIFY2( std::fabs( apex - BallFlight::APEX_PER_DISTANCE * far ) < 0.1f, qPrintable( QString::number( apex ) ) );
      }

    /*!
      the cuts: a play mode change (placed: from the ground), a jump, and a
      change of velocity without a touch (a collision)
    */
    void cuts()
      {
          Log log = ::longBall();
          // a free kick placed at frame 8, taken at once
          for ( std::size_t k = 8; k <= log.last(); ++k ) log.at( k ).pmode_ = rcss::rcg::PM_FreeKick_Right;
          BallFlight flight;
          flight.update( log.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->last, std::size_t( 8 ) );
          QCOMPARE( segmentFrom( flight, 0 )->end, BallFlight::Touch::PlayMode );
          QCOMPARE( flight.heightAtFrame( 8 ), 0.0f );

          // a jump
          log = ::longBall();
          log.at( 9 ).show_.ball_.x_ += 10.0f;
          flight.update( log.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->last, std::size_t( 9 ) );

          // a tackle's bit stays for its cycles: one touch at its start
          Log slide;
          slide.add( 0.0f, 0.0f );
          slide.roll( 0.0f, 0.0f, 1.0f, 0.0f, 2.5f, 15 );
          slide.touch( 0, 4, rcss::rcg::TACKLE );
          for ( std::size_t k = 2; k <= 10; ++k ) slide.at( k ).show_.player_[4].state_ |= rcss::rcg::TACKLE;
          flight.update( slide.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->start, BallFlight::Touch::Tackle );
          QCOMPARE( segmentFrom( flight, 0 )->last, slide.last() );

          // the ball hit a player: its bit at the frame of the collision,
          // which is the moment (the ball's move need not show it)
          Log body = ::longBall();
          body.at( 9 ).show_.player_[15].x_ = body.frames[9]->show_.ball_.x_ + 0.4f;
          body.at( 9 ).show_.player_[15].y_ = 0.0f;
          body.at( 9 ).show_.player_[15].state_ |= rcss::rcg::BALL_COLLIDE;
          flight.update( body.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->last, std::size_t( 9 ) );
          QCOMPARE( segmentFrom( flight, 0 )->end, BallFlight::Touch::Collision );
          QCOMPARE( segmentFrom( flight, 0 )->reach, BallFlight::BODY_REACH );
          QVERIFY( flight.heightAtFrame( 9 ) <= BallFlight::BODY_REACH + 1.0e-4f );
          QVERIFY( segmentFrom( flight, 9 ) );
          QCOMPARE( segmentFrom( flight, 9 )->start, BallFlight::Touch::Collision );
          // a trap: the collision followed by a kick of the same player 2
          // cycles later; the ball low enough for a foot at the collision
          body = ::longBall();
          body.at( 13 ).show_.player_[8].state_ |= rcss::rcg::BALL_COLLIDE;
          flight.update( body.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->last, std::size_t( 13 ) );
          QCOMPARE( segmentFrom( flight, 0 )->end, BallFlight::Touch::Collision );
          QCOMPARE( segmentFrom( flight, 0 )->reach, BallFlight::FOOT_REACH );
          QVERIFY( flight.heightAtFrame( 13 ) <= BallFlight::FOOT_REACH + 1.0e-4f );
          // a kick long after the collision: the body's reach
          body = ::longBall();
          body.at( 10 ).show_.player_[8].state_ |= rcss::rcg::BALL_COLLIDE;
          flight.update( body.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->reach, BallFlight::BODY_REACH );

          // a kick after the collision: a kick
          body = ::longBall();
          body.at( 15 ).show_.player_[8].state_ |= rcss::rcg::BALL_COLLIDE;
          flight.update( body.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->end, BallFlight::Touch::Kick );
          QCOMPARE( segmentFrom( flight, 15 )->start, BallFlight::Touch::Kick );

          // the ball back from a body, without a bit (a post; an old log)
          Log hit;
          hit.add( 0.0f, 0.0f );
          hit.roll( 0.0f, 0.0f, 1.0f, 0.0f, 2.5f, 8 );
          hit.roll( hit.frames.back()->show_.ball_.x_, 0.0f, -1.0f, 0.0f, 0.3f, 6 );
          hit.touch( 0, 4 );
          flight.update( hit.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->end, BallFlight::Touch::Collision );
          QCOMPARE( segmentFrom( flight, 0 )->last, std::size_t( 8 ) );
      }

    /*!
      a log with heights of its own (a 2.5D server): not active, those are
      used; the same frames: not found again; other frames: found again
    */
    void frames()
      {
          Log log = ::longBall();
          BallFlight flight;
          flight.update( log.frames, params(), RADIUS );
          QVERIFY( flight.active() );
          const std::vector< BallFlight::Segment > first = flight.segments();
          flight.update( log.frames, params(), RADIUS );
          QCOMPARE( flight.segments().size(), first.size() );
          // more frames
          log.add( 0.0f, 0.0f );
          flight.update( log.frames, params(), RADIUS );
          QCOMPARE( segmentFrom( flight, 0 )->v, first[0].v );
          QVERIFY( flight.heightAtFrame( 5 ) > 0.0f );

          Log real = ::longBall();
          real.at( 5 ).show_.ball_.z_ = 1.0f;
          flight.update( real.frames, params(), RADIUS );
          QVERIFY( ! flight.active() );
          QCOMPARE( flight.heightAtFrame( 5 ), 0.0f );
          flight.update( DispCont(), params(), RADIUS );
          QVERIFY( ! flight.active() );
          QCOMPARE( flight.heightAt( 0, 0.5 ), 0.0f );
      }

    /*!
      what reads the heights: the goalie's catch at the ball's height, and
      the trail through the ball's centers in the air
    */
    void readers()
      {
          const Log log = ::longBall( rcss::rcg::CATCH, 11 );
          BallFlight flight;
          flight.update( log.frames, params(), RADIUS );
          const rcss::rcg::ServerParamT & sp = params();
          const FigureActions with = FigureActionLog::read( log.frames, 11, 16, 17, 0.0,
                                                            log.frames[16]->show_.player_[11], sp, RADIUS, &flight );
          QVERIFY( with.caught.on );
          QCOMPARE( with.caught.height, flight.heightAtFrame( 15 ) );
          const FigureActions without = FigureActionLog::read( log.frames, 11, 16, 17, 0.0,
                                                               log.frames[16]->show_.player_[11], sp, RADIUS );
          QCOMPARE( without.caught.height, 0.0f );

          const std::vector< BallAids3D::TrailPoint > points = BallAids3D::trailPoints( log.frames, 8, 9, 0.5, 5, RADIUS, &flight );
          QVERIFY( points.size() > 3 );
          QVERIFY( std::fabs( points[0].at.y() - ( flight.heightAt( 8, 0.5 ) + RADIUS ) ) < 1.0e-5f );
          QVERIFY( std::fabs( points[1].at.y() - ( flight.heightAtFrame( 8 ) + RADIUS ) ) < 1.0e-5f );
          QVERIFY( std::fabs( points[2].at.y() - ( flight.heightAtFrame( 7 ) + RADIUS ) ) < 1.0e-5f );
          float top = 0.0f;
          for ( const BallAids3D::TrailPoint & p : points ) top = std::max( top, p.at.y() );
          QVERIFY( top > RADIUS + 0.2f ); // in the air
          const std::vector< BallAids3D::TrailPoint > flat = BallAids3D::trailPoints( log.frames, 8, 9, 0.5, 5, RADIUS );
          QCOMPARE( flat[1].at.y(), RADIUS );
      }
};

QTEST_MAIN( TestBallFlight3D )

#include "test_ball_flight_3d.moc"
