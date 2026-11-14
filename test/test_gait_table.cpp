// -*-c++-*-

/*!
  \file test_gait_table.cpp
  \brief unit test of GaitTable (how far the players have walked)
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

#include "gait_table.h"

#include <cmath>
#include <memory>

namespace {

//! the strides of the test: 1 m a cycle at 1 m/s, 4 m at 5 m/s
std::vector< GaitStride >
strides()
{
    return { { 1.0f, 1.0f }, { 5.0f, 4.0f } };
}

//! a frame with players where told (the others not connected)
struct Player {
    int index;
    float x;
    float y;
    float body;
};

DispConstPtr
frame( const std::vector< Player > & players )
{
    auto d = std::make_shared< rcss::rcg::DispInfoT >();
    for ( const Player & p : players )
    {
        rcss::rcg::PlayerT & t = d->show_.player_[p.index];
        t.side_ = ( p.index < rcss::rcg::MAX_PLAYER ? 'l' : 'r' );
        t.unum_ = static_cast< rcss::rcg::Int16 >( p.index % rcss::rcg::MAX_PLAYER + 1 );
        t.state_ = 1;
        t.x_ = p.x;
        t.y_ = p.y;
        t.body_ = p.body;
    }
    return d;
}

//! player 0 going along x at a speed (m/s, 0.1 s a cycle), facing a direction
DispCont
straight( const int count,
          const float speed,
          const float body = 0.0f )
{
    DispCont frames;
    for ( int i = 0; i < count; ++i ) frames.push_back( frame( { { 0, speed * 0.1f * i, 0.0f, body } } ) );
    return frames;
}

bool
near( const double a,
      const double b,
      const double tolerance = 1.0e-4 )
{
    return std::fabs( a - b ) <= tolerance;
}

}

/*!
  \brief unit test of GaitTable
*/
class TestGaitTable
    : public QObject {
    Q_OBJECT

private slots:

    //! the stride between the clips' speeds, held beyond them
    void strideAt()
      {
          GaitTable t;
          QCOMPARE( t.strideAt( 3.0f ), 1.0f ); // none
          t.setStrides( strides() );
          QCOMPARE( t.strideAt( 0.2f ), 1.0f );
          QCOMPARE( t.strideAt( 1.0f ), 1.0f );
          QVERIFY( near( t.strideAt( 2.0f ), 1.75 ) );
          QCOMPARE( t.strideAt( 5.0f ), 4.0f );
          QCOMPARE( t.strideAt( 9.0f ), 4.0f );
      }

    /*!
      the cycles walked: as far as walked over the stride at the speed,
      from each player's own start; between the frames by the fraction
    */
    void walked()
      {
          GaitTable t;
          t.setStrides( strides() );
          const DispCont frames = straight( 10, 1.0f ); // 0.1 m a cycle: 0.1 cycles
          t.update( frames, 0.1 );
          QCOMPARE( t.frameCount(), std::size_t( 10 ) );
          QVERIFY( near( t.motion( 0, 0, 1, 0.0 ).gait, 0.0 ) );
          QVERIFY( near( t.motion( 0, 4, 5, 0.0 ).gait, 0.4 ) );
          QVERIFY( near( t.motion( 0, 4, 5, 0.5 ).gait, 0.45 ) );
          // not moving toward a next frame: at the frame
          QVERIFY( near( t.motion( 0, 4, 4, 0.5 ).gait, 0.4 ) );
          // the last frame
          QVERIFY( near( t.motion( 0, 9, 9, 0.7 ).gait, 0.9 ) );

          // faster: a longer stride (5 m/s: 0.5 m a cycle over 4 m)
          t.update( straight( 10, 5.0f ), 0.1 );
          QVERIFY( near( t.motion( 0, 4, 5, 0.0 ).gait, 4 * 0.5 / 4.0 ) );
          // the same moves with a longer step (0.2 s): 2.5 m/s, a stride of 2.125 m
          t.update( straight( 10, 5.0f ), 0.2 );
          QVERIFY( near( t.motion( 0, 4, 5, 0.0 ).gait, 4 * 0.5 / 2.125 ) );

          // each player from its own point, still while standing
          t.update( straight( 10, 1.0f ), 0.1 );
          DispCont two;
          for ( int i = 0; i < 3; ++i ) two.push_back( frame( { { 0, 0.0f, 0.0f, 0.0f }, { 5, 0.0f, 0.0f, 0.0f } } ) );
          t.update( two, 0.1 );
          QVERIFY( t.motion( 0, 2, 2, 0.0 ).gait != t.motion( 5, 2, 2, 0.0 ).gait );
          QCOMPARE( t.motion( 5, 0, 1, 0.0 ).gait, t.motion( 5, 2, 2, 0.0 ).gait );

          // nothing outside
          QCOMPARE( t.motion( 0, 3, 3, 0.0 ).gait, 0.0 );
          QCOMPARE( t.motion( 22, 0, 1, 0.0 ).gait, 0.0 );
      }

    /*!
      backward (away from the body direction) takes the cycles back;
      sideways is still forward
    */
    void backward()
      {
          GaitTable t;
          t.setStrides( strides() );
          t.update( straight( 5, 1.0f, 180.0f ), 0.1 );
          QVERIFY( near( t.motion( 0, 4, 4, 0.0 ).gait, -0.4 ) );
          t.update( straight( 5, 1.0f, 100.0f ), 0.1 ); // the body 100 degrees from the way
          QVERIFY( near( t.motion( 0, 4, 4, 0.0 ).gait, -0.4 ) );
          t.update( straight( 5, 1.0f, 80.0f ), 0.1 );
          QVERIFY( near( t.motion( 0, 4, 4, 0.0 ).gait, 0.4 ) );
      }

    //! a move farther than TELEPORT, and a player not connected, are not walked
    void notWalked()
      {
          GaitTable t;
          t.setStrides( strides() );
          DispCont frames = straight( 3, 1.0f );
          frames.push_back( frame( { { 0, 30.0f, 0.0f, 0.0f } } ) ); // placed for a free kick
          frames.push_back( frame( { { 0, 30.1f, 0.0f, 0.0f } } ) );
          frames.push_back( frame( {} ) ); // gone
          frames.push_back( frame( { { 0, 0.0f, 0.0f, 0.0f } } ) ); // back
          t.update( frames, 0.1 );
          QVERIFY( near( t.motion( 0, 2, 2, 0.0 ).gait, 0.2 ) );
          QVERIFY( near( t.motion( 0, 3, 3, 0.0 ).gait, 0.2 ) );
          QVERIFY( near( t.motion( 0, 4, 4, 0.0 ).gait, 0.3 ) );
          QVERIFY( near( t.motion( 0, 6, 6, 0.0 ).gait, 0.3 ) );
          // the placing has no velocity: halfway between it (0) and the next move (1 m/s)
          QVERIFY( near( t.motion( 0, 3, 3, 0.0 ).vx, 0.5 ) );
      }

    /*!
      the velocity, smoothed through the moves; the acceleration over the
      cycles around: none going steadily, forward when speeding up
    */
    void velocity()
      {
          GaitTable t;
          t.setStrides( strides() );
          t.update( straight( 20, 3.0f ), 0.1 );
          const FigureMotion m = t.motion( 0, 10, 11, 0.3 );
          QVERIFY( near( m.vx, 3.0 ) );
          QVERIFY( near( m.vy, 0.0 ) );
          QVERIFY( near( m.ax, 0.0 ) );
          QVERIFY( near( m.ay, 0.0 ) );

          // standing for 10 frames, then going along +y at 4 m/s
          DispCont frames;
          for ( int i = 0; i < 20; ++i )
          {
              frames.push_back( frame( { { 0, 0.0f, ( i < 10 ? 0.0f : 0.4f * ( i - 9 ) ), 90.0f } } ) );
          }
          t.update( frames, 0.1 );
          // the move into frame 10 (4 m/s) is at 9.5, the one before (0) at 8.5
          QVERIFY( near( t.motion( 0, 9, 10, 0.25 ).vy, 3.0 ) );
          QVERIFY( near( t.motion( 0, 8, 9, 0.0 ).vy, 0.0 ) );
          // 4 m/s gained over the 0.4 s around
          const FigureMotion a = t.motion( 0, 9, 10, 0.5 );
          QVERIFY( near( a.ay, 10.0 ) );
          QVERIFY( near( a.ax, 0.0 ) );
          QVERIFY( near( t.motion( 0, 15, 16, 0.0 ).ay, 0.0 ) );
          // before the first frame and after the last, held
          QVERIFY( near( t.motion( 0, 19, 19, 0.0 ).vy, 4.0 ) );
          QVERIFY( near( t.motion( 0, 0, 1, 0.0 ).ay, 0.0 ) );

          // going along -x at 4 m/s, stopping at frame 10: slowing down, +x
          frames.clear();
          for ( int i = 0; i < 20; ++i )
          {
              frames.push_back( frame( { { 0, -0.4f * std::min( i, 10 ), 0.0f, 180.0f } } ) );
          }
          t.update( frames, 0.1 );
          const FigureMotion stop = t.motion( 0, 10, 11, 0.0 );
          QVERIFY( near( stop.vx, -2.0 ) );
          QVERIFY( near( stop.ax, 4.0 / 0.4 ) );
          QVERIFY( near( stop.ay, 0.0 ) );
          QVERIFY( near( t.motion( 0, 4, 5, 0.0 ).vx, -4.0 ) );
      }

    /*!
      a live game: the new frames are walked on from where they were, and
      when the oldest are dropped, the others keep their cycles; other
      frames, other strides and another step start again
    */
    void live()
      {
          GaitTable t;
          t.setStrides( strides() );
          DispCont frames = straight( 30, 1.0f );
          DispCont part( frames.begin(), frames.begin() + 12 );
          t.update( part, 0.1 );
          part.insert( part.end(), frames.begin() + 12, frames.end() );
          t.update( part, 0.1 );
          GaitTable whole;
          whole.setStrides( strides() );
          whole.update( frames, 0.1 );
          QCOMPARE( t.frameCount(), whole.frameCount() );
          for ( std::size_t i = 0; i < frames.size(); ++i )
          {
              QCOMPARE( t.motion( 0, i, i + 1, 0.5 ).gait, whole.motion( 0, i, i + 1, 0.5 ).gait );
          }

          // the oldest 10 dropped, and 3 more come
          const double at_20 = t.motion( 0, 20, 21, 0.0 ).gait;
          DispCont live( frames.begin() + 10, frames.end() );
          for ( int i = 30; i < 33; ++i ) live.push_back( frame( { { 0, 0.1f * i, 0.0f, 0.0f } } ) );
          t.update( live, 0.1 );
          QCOMPARE( t.frameCount(), std::size_t( 23 ) );
          QCOMPARE( t.motion( 0, 10, 11, 0.0 ).gait, at_20 );
          QVERIFY( near( t.motion( 0, 22, 22, 0.0 ).gait, at_20 + 1.2 ) );

          // all dropped but the newest: still found
          DispCont newest( live.end() - 1, live.end() );
          t.update( newest, 0.1 );
          QCOMPARE( t.frameCount(), std::size_t( 1 ) );
          QVERIFY( near( t.motion( 0, 0, 0, 0.0 ).gait, at_20 + 1.2 ) );

          // other frames: from the start
          t.update( straight( 5, 1.0f ), 0.1 );
          QVERIFY( near( t.motion( 0, 4, 4, 0.0 ).gait, 0.4 ) );
          // another step, other strides: again
          t.update( straight( 5, 1.0f ), 0.1 );
          t.update( straight( 5, 1.0f ), 0.1 );
          const DispCont same = straight( 5, 1.0f );
          t.update( same, 0.1 );
          t.update( same, 0.2 );
          QVERIFY( near( t.motion( 0, 4, 4, 0.0 ).gait, 0.4 ) ); // 0.1 m over 1 m, at 0.5 m/s
          t.setStrides( { { 1.0f, 2.0f } } );
          t.update( same, 0.2 );
          QVERIFY( near( t.motion( 0, 4, 4, 0.0 ).gait, 0.2 ) );
          t.setStrides( { { 1.0f, 2.0f } } ); // the same: kept
          QCOMPARE( t.frameCount(), std::size_t( 5 ) );
      }
};

QTEST_MAIN( TestGaitTable )

#include "test_gait_table.moc"
