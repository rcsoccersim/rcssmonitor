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

#include <QVector2D>

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

    /*!
      the ways mixed: forward only (the classic way), whichever way the
      forward stride; with backward clips only, forward and backward mixed
      in a band about sideways; with sideways clips, the two ways around
      the way gone, by weights that move a foot on the ground along it
    */
    void waysMixed()
      {
          const auto rad = []( const float degree ) { return degree * float( M_PI ) / 180.0f; };
          GaitWays ways;
          ways.strides[GaitWays::Forward] = strides();
          QVERIFY( ! ways.directional() );
          QVERIFY( ! ways.sideways() );
          for ( const float d : { 0.0f, 45.0f, 90.0f, 180.0f, -120.0f } )
          {
              const GaitWays::Mix m = ways.mixAt( 3.0f, rad( d ) );
              QCOMPARE( m.a, int( GaitWays::Forward ) );
              QCOMPARE( m.b, int( GaitWays::Forward ) );
              QCOMPARE( m.t, 0.0f );
              QVERIFY( near( m.stride, 2.5 ) );
          }
          QCOMPARE( GaitWays::strideAt( {}, 2.0f ), 1.0f );
          QVERIFY( near( GaitWays::strideAt( strides(), 3.0f ), 2.5 ) );

          // backward only: forward, the band (smoothly), backward
          ways.strides[GaitWays::Backward] = { { 0.5f, 0.5f }, { 2.0f, 1.5f } };
          QVERIFY( ways.directional() );
          QVERIFY( ! ways.sideways() );
          const float band = GaitWays::BACK_BAND;
          for ( const float side : { 1.0f, -1.0f } )
          {
              QCOMPARE( ways.mixAt( 1.0f, rad( side * ( 90.0f - band - 1.0f ) ) ).t, 0.0f );
              QCOMPARE( ways.mixAt( 1.0f, rad( side * ( 90.0f + band + 1.0f ) ) ).t, 1.0f );
              const GaitWays::Mix m = ways.mixAt( 1.0f, rad( side * 90.0f ) );
              QCOMPARE( m.a, int( GaitWays::Forward ) );
              QCOMPARE( m.b, int( GaitWays::Backward ) );
              QVERIFY( near( m.t, 0.5 ) );
              QVERIFY( near( m.stride, 0.5 * ( 1.0 + ( 0.5 + 1.0 / 3.0 ) ) ) ); // halfway between the strides
              const float quarter = ways.mixAt( 1.0f, rad( side * ( 90.0f - 0.5f * band ) ) ).t;
              QVERIFY( near( quarter, 0.15625 ) ); // smoothstep(1/4)
          }
          QVERIFY( near( ways.mixAt( 1.0f, rad( 180.0f ) ).stride, 0.5 + 0.5 / 1.5 ) );

          // sideways too
          ways.strides[GaitWays::Right] = { { 0.5f, 0.4f }, { 1.5f, 1.0f } };
          ways.strides[GaitWays::Left] = { { 0.5f, 0.4f }, { 1.5f, 1.0f } };
          QVERIFY( ways.sideways() );
          QVERIFY( ways.directional() );
          const struct { float d; int a; int b; } around[] = {
              { 0.0f, GaitWays::Forward, GaitWays::Right },
              { 30.0f, GaitWays::Forward, GaitWays::Right },
              { 89.0f, GaitWays::Forward, GaitWays::Right },
              { 91.0f, GaitWays::Right, GaitWays::Backward },
              { 170.0f, GaitWays::Right, GaitWays::Backward },
              { -30.0f, GaitWays::Forward, GaitWays::Left },
              { -120.0f, GaitWays::Left, GaitWays::Backward },
              { 330.0f, GaitWays::Forward, GaitWays::Left }, // -30
          };
          for ( const float speed : { 0.3f, 1.0f, 4.0f } )
          {
              for ( const auto & w : around )
              {
                  const GaitWays::Mix m = ways.mixAt( speed, rad( w.d ) );
                  QCOMPARE( m.a, w.a );
                  QCOMPARE( m.b, w.b );
                  // a foot on the ground goes against the way gone: the
                  // ways' feet by their weights and strides, along it, as
                  // far as the stride of the mix
                  const auto unit = []( const int way ) {
                      static const QVector2D u[4] = { { 1.0f, 0.0f }, { 0.0f, 1.0f }, { -1.0f, 0.0f }, { 0.0f, -1.0f } };
                      return u[way]; // forward, to the right (+y on the field), ...
                  };
                  const auto strideOf = [&]( const int way ) {
                      return GaitWays::strideAt( ways.strides[way], speed );
                  };
                  const QVector2D foot = unit( m.a ) * ( ( 1.0f - m.t ) * strideOf( m.a ) )
                      + unit( m.b ) * ( m.t * strideOf( m.b ) );
                  // (the clips' way from the hips turned)
                  const float clip = GaitWays::clipWay( rad( w.d ) );
                  const QVector2D way( std::cos( clip ), std::sin( clip ) );
                  QVERIFY2( std::fabs( foot.x() * way.y() - foot.y() * way.x() ) < 1.0e-4f,
                            qPrintable( QString( "%1 at %2" ).arg( w.d ).arg( speed ) ) );
                  QVERIFY( near( QVector2D::dotProduct( foot, way ), m.stride ) );
                  QVERIFY( m.t >= 0.0f && m.t <= 1.0f );
              }
              // straight each way: that way alone, its stride; left and right alike
              QCOMPARE( ways.mixAt( speed, 0.0f ).t, 0.0f );
              QVERIFY( near( ways.mixAt( speed, 0.0f ).stride, GaitWays::strideAt( ways.strides[0], speed ) ) );
              for ( const float side : { 1.0f, -1.0f } )
              {
                  const GaitWays::Mix m = ways.mixAt( speed, rad( side * 90.0f ) );
                  QVERIFY( ( m.b == ( side > 0.0f ? GaitWays::Right : GaitWays::Left ) && near( m.t, 1.0 ) )
                           || ( m.a == ( side > 0.0f ? GaitWays::Right : GaitWays::Left ) && near( m.t, 0.0 ) ) );
                  QVERIFY( near( m.stride, GaitWays::strideAt( ways.strides[GaitWays::Right], speed ) ) );
                  const GaitWays::Mix l = ways.mixAt( speed, rad( side * 45.0f ) );
                  const GaitWays::Mix r = ways.mixAt( speed, rad( -side * 45.0f ) );
                  QVERIFY( near( l.t, r.t ) && near( l.stride, r.stride ) );
              }
              QVERIFY( near( ways.mixAt( speed, rad( 180.0f ) ).t, 1.0 ) );
              QVERIFY( near( ways.mixAt( speed, rad( 180.0f ) ).stride, GaitWays::strideAt( ways.strides[GaitWays::Backward], speed ) ) );
          }
          // without backward ones, sideways mixed with the forward ones (played backward)
          GaitWays side_only = ways;
          side_only.strides[GaitWays::Backward].clear();
          QVERIFY( side_only.directional() );
          QVERIFY( near( side_only.mixAt( 3.0f, rad( 180.0f ) ).stride, 2.5 ) );
          // one side only: not sideways
          GaitWays one_side;
          one_side.strides[GaitWays::Forward] = strides();
          one_side.strides[GaitWays::Left] = ways.strides[GaitWays::Left];
          QVERIFY( ! one_side.sideways() );
          QVERIFY( ! one_side.directional() );
      }

    /*!
      with sideways clips, the hips turn toward the way gone as far as the
      way from forward or backward up to TWIST_FULL, then less to none
      sideways; the clips go the rest of the way from the hips; without
      them, no turn
    */
    void twist()
      {
          const auto rad = []( const float degree ) { return degree * float( M_PI ) / 180.0f; };
          const auto deg = []( const float radian ) { return radian * 180.0f / float( M_PI ); };
          GaitWays ways;
          ways.strides[GaitWays::Forward] = strides();
          QCOMPARE( ways.twistAt( rad( 20.0f ) ), 0.0f );
          ways.strides[GaitWays::Backward] = { { 0.5f, 0.5f } };
          QCOMPARE( ways.twistAt( rad( 20.0f ) ), 0.0f );
          ways.strides[GaitWays::Right] = ways.strides[GaitWays::Left] = { { 0.5f, 0.4f } };
          QCOMPARE( GaitWays::TWIST_FULL, 30.0f );
          const struct { float toward; float twist; float clip; } cases[] = {
              { 0.0f, 0.0f, 0.0f }, { 20.0f, 20.0f, 0.0f }, { 30.0f, 30.0f, 0.0f }, { 60.0f, 15.0f, 45.0f },
              { 90.0f, 0.0f, 90.0f }, { 120.0f, -15.0f, 135.0f }, { 160.0f, -20.0f, 180.0f }, { 180.0f, 0.0f, 180.0f },
              { -20.0f, -20.0f, 0.0f }, { -60.0f, -15.0f, -45.0f }, { -120.0f, 15.0f, -135.0f }, { 340.0f, -20.0f, 0.0f },
          };
          for ( const auto & c : cases )
          {
              const float twist = deg( ways.twistAt( rad( c.toward ) ) );
              const float clip = deg( GaitWays::clipWay( rad( c.toward ) ) );
              QVERIFY2( std::fabs( twist - c.twist ) < 1.0e-3f, qPrintable( QString( "%1: %2" ).arg( c.toward ).arg( twist ) ) );
              QVERIFY2( std::fabs( std::remainder( clip - c.clip, 360.0f ) ) < 1.0e-3f, qPrintable( QString( "%1: %2" ).arg( c.toward ).arg( clip ) ) );
              // the hips' turn and the clips' way: the way gone
              QVERIFY( std::fabs( std::remainder( twist + clip - c.toward, 360.0f ) ) < 1.0e-3f );
          }
          // straight forward: the forward clips alone at once (a figure running a little off its body direction)
          QCOMPARE( ways.mixAt( 4.0f, rad( 25.0f ) ).t, 0.0f );
          QVERIFY( near( ways.mixAt( 4.0f, rad( 25.0f ) ).stride, GaitWays::strideAt( strides(), 4.0f ) ) );
      }

    /*!
      with clips of other ways, the gait goes on whichever way the player
      goes (forward only, backward takes it back), each move by the stride
      of the ways mixed for it; other ways: the table again
    */
    void waysWalked()
      {
          GaitWays ways;
          ways.strides[GaitWays::Forward] = strides();
          ways.strides[GaitWays::Backward] = { { 0.5f, 0.5f }, { 2.0f, 1.5f } };
          ways.strides[GaitWays::Right] = { { 0.5f, 0.4f }, { 1.5f, 1.0f } };
          ways.strides[GaitWays::Left] = { { 0.5f, 0.4f }, { 1.5f, 1.0f } };
          // backward at 2 m/s: 0.2 m a cycle, over 1.5 m strides
          GaitTable t;
          t.setWays( ways );
          QVERIFY( t.ways() == ways );
          t.update( straight( 6, -2.0f ), 0.1 );
          const double start = t.motion( 0, 0, 0, 0.0 ).gait;
          QVERIFY( near( t.motion( 0, 5, 5, 0.0 ).gait - start, 5 * 0.2 / 1.5 ) );
          // sideways (facing +y, going +x: to the left) at 1 m/s
          t.update( straight( 6, 1.0f, 90.0f ), 0.1 );
          const GaitWays::Mix left = ways.mixAt( 1.0f, float( -0.5 * M_PI ) );
          QCOMPARE( left.b, int( GaitWays::Left ) );
          QVERIFY( near( t.motion( 0, 5, 5, 0.0 ).gait - t.motion( 0, 0, 0, 0.0 ).gait, 5 * 0.1 / left.stride ) );
          // diagonally back and to the right at 1.5 m/s
          DispCont diagonal;
          for ( int i = 0; i < 6; ++i ) diagonal.push_back( frame( { { 0, -0.15f * i * float( M_SQRT1_2 ), 0.15f * i * float( M_SQRT1_2 ), 0.0f } } ) );
          t.update( diagonal, 0.1 );
          const GaitWays::Mix back_right = ways.mixAt( 1.5f, float( 0.75 * M_PI ) );
          QCOMPARE( back_right.a, int( GaitWays::Right ) );
          QCOMPARE( back_right.b, int( GaitWays::Backward ) );
          QVERIFY( near( t.motion( 0, 5, 5, 0.0 ).gait - t.motion( 0, 0, 0, 0.0 ).gait, 5 * 0.15 / back_right.stride ) );
          // forward only: backward takes it back (the classic way)
          t.setStrides( strides() );
          QCOMPARE( t.frameCount(), std::size_t( 0 ) );
          t.update( straight( 6, -2.0f ), 0.1 );
          QVERIFY( near( t.motion( 0, 5, 5, 0.0 ).gait - t.motion( 0, 0, 0, 0.0 ).gait, -5 * 0.2 / 1.75 ) );
          QVERIFY( ! t.ways().directional() );
          QCOMPARE( t.strides().size(), strides().size() );
          // the same ways: kept; others: again
          t.setWays( ways );
          t.update( straight( 6, -2.0f ), 0.1 );
          t.setWays( ways );
          QCOMPARE( t.frameCount(), std::size_t( 6 ) );
          GaitWays other = ways;
          other.strides[GaitWays::Left][0].stride = 0.3f;
          QVERIFY( other != ways );
          t.setWays( other );
          QCOMPARE( t.frameCount(), std::size_t( 0 ) );
      }

    /*!
      the legs following the body direction: the body turned at once, the
      legs its direction averaged over TURN_LAG cycles before; how far they
      lag behind it, how fast they turn (to the right positive), the steps
      taken as they turn either way (a step each TURN_STEP_ANGLE, at most
      TURN_STEPS_MAX a cycle, however fast); across +-180 the short way;
      starting again after a move and out of the game
    */
    void legs()
      {
          // standing, the body turned 90 degrees to the right between frames 9 and 10
          const auto turning = []( const float to,
                                   const float jump_x = 0.0f ) {
              DispCont frames;
              for ( int i = 0; i < 20; ++i )
              {
                  frames.push_back( frame( { { 0, i >= 10 ? jump_x : 0.0f, 0.0f, i >= 10 ? to : 0.0f } } ) );
              }
              return frames;
          };
          GaitTable t;
          t.setStrides( strides() );
          t.update( turning( 90.0f ), 0.1 );
          // halfway through the turn: the body at 45, the legs at 3.75 (11.25 / 3)
          FigureMotion m = t.motion( 0, 9, 10, 0.5 );
          QVERIFY2( near( m.lag, 45.0 - 3.75 ), qPrintable( QString::number( m.lag ) ) );
          // a cycle after: the legs halfway (0, 45 and 90 averaged)
          m = t.motion( 0, 11, 12, 0.0 );
          QVERIFY( near( m.lag, 45.0 ) );
          QVERIFY2( near( m.turn_rate, 300.0, 1.0 ), qPrintable( QString::number( m.turn_rate ) ) );
          // the steps: the legs at 15 (a sixth of a step), then 45 (a third more)
          QVERIFY2( near( m.steps, 0.5 * 90.0 / GaitTable::TURN_STEP_ANGLE ), qPrintable( QString::number( m.steps ) ) );
          // between the frames: in a line (a third more at 12)
          QVERIFY( near( t.motion( 0, 11, 12, 0.5 ).steps, ( 45.0 + 15.0 ) / GaitTable::TURN_STEP_ANGLE ) );
          // TURN_LAG after the turn: caught up, still; a step each TURN_STEP_ANGLE
          m = t.motion( 0, 13, 14, 0.0 );
          QVERIFY( near( m.lag, 0.0 ) );
          QVERIFY( near( m.steps, 90.0 / GaitTable::TURN_STEP_ANGLE ) );
          m = t.motion( 0, 15, 16, 0.5 );
          QVERIFY( near( m.turn_rate, 0.0 ) );
          QVERIFY( near( m.steps, 90.0 / GaitTable::TURN_STEP_ANGLE ) );
          // before the turn: nothing
          m = t.motion( 0, 5, 6, 0.5 );
          QVERIFY( near( m.lag, 0.0 ) && near( m.turn_rate, 0.0 ) && near( m.steps, 0.0 ) );
          // however fast: at most TURN_STEPS_MAX a cycle (spinning, 170 a cycle twice:
          // the legs at 28.3, 113.3, 226.7, 311.7, 340, the three in the middle held)
          DispCont spinning;
          for ( int i = 0; i < 20; ++i ) spinning.push_back( frame( { { 0, 0.0f, 0.0f, i >= 11 ? -20.0f : i >= 10 ? 170.0f : 0.0f } } ) );
          t.update( spinning, 0.1 );
          for ( std::size_t i = 0; i + 1 < t.frameCount(); ++i )
          {
              const double d = t.motion( 0, i + 1, i + 2, 0.0 ).steps - t.motion( 0, i, i + 1, 0.0 ).steps;
              QVERIFY2( d >= 0.0 && d <= GaitTable::TURN_STEPS_MAX + 1.0e-9, qPrintable( QString( "%1: %2" ).arg( i ).arg( d ) ) );
          }
          const double ends = 2.0 * ( 85.0 / 3.0 ) / GaitTable::TURN_STEP_ANGLE;
          QVERIFY2( near( t.motion( 0, 15, 16, 0.0 ).steps, ends + 3.0 * GaitTable::TURN_STEPS_MAX ),
                    qPrintable( QString::number( t.motion( 0, 15, 16, 0.0 ).steps ) ) );
          // a turn of 170 is not held: about 1.9 steps
          t.update( turning( 170.0f ), 0.1 );
          QVERIFY( near( t.motion( 0, 15, 16, 0.0 ).steps, 170.0 / GaitTable::TURN_STEP_ANGLE ) );

          // to the left: the lag and the rate the other way, the turn counted the same
          t.update( turning( -90.0f ), 0.1 );
          m = t.motion( 0, 11, 12, 0.0 );
          QVERIFY( near( m.lag, -45.0 ) );
          QVERIFY( near( m.turn_rate, -300.0, 1.0 ) );
          QVERIFY( near( m.steps, 45.0 / GaitTable::TURN_STEP_ANGLE ) );

          // across 180: from 170 to -170 is 20 degrees to the right
          DispCont across;
          for ( int i = 0; i < 20; ++i ) across.push_back( frame( { { 0, 0.0f, 0.0f, i >= 10 ? -170.0f : 170.0f } } ) );
          t.update( across, 0.1 );
          m = t.motion( 0, 11, 12, 0.0 );
          QVERIFY2( near( m.lag, 10.0 ), qPrintable( QString::number( m.lag ) ) );
          QVERIFY( m.turn_rate > 0.0f );
          QVERIFY( near( t.motion( 0, 15, 16, 0.0 ).steps, 20.0 / GaitTable::TURN_STEP_ANGLE ) );

          // a move (a free kick's placing): the legs start again in the body direction
          t.update( turning( 90.0f, 5.0f ), 0.1 );
          m = t.motion( 0, 11, 12, 0.0 );
          QVERIFY( near( m.lag, 0.0 ) );
          QVERIFY( near( m.steps, 0.0 ) );
          QVERIFY( near( m.turn_rate, 0.0 ) );
          // out of the game for a frame: the same
          DispCont gone = turning( 90.0f );
          gone[10] = frame( {} );
          t.update( gone, 0.1 );
          m = t.motion( 0, 12, 13, 0.0 );
          QVERIFY( near( m.lag, 0.0 ) );
          QVERIFY( near( m.steps, 0.0 ) );

          // live: frames added one by one, and the oldest dropped, give the same
          const DispCont all = turning( 90.0f );
          GaitTable once;
          once.setStrides( strides() );
          once.update( all, 0.1 );
          GaitTable live;
          live.setStrides( strides() );
          DispCont some;
          for ( const DispConstPtr & f : all )
          {
              some.push_back( f );
              live.update( some, 0.1 );
          }
          for ( std::size_t i = 0; i + 1 < all.size(); ++i )
          {
              const FigureMotion a = once.motion( 0, i, i + 1, 0.25 );
              const FigureMotion b = live.motion( 0, i, i + 1, 0.25 );
              QVERIFY( near( a.lag, b.lag ) && near( a.steps, b.steps ) && near( a.turn_rate, b.turn_rate ) );
          }
          some.erase( some.begin(), some.begin() + 4 );
          live.update( some, 0.1 );
          const FigureMotion a = once.motion( 0, 12, 13, 0.5 );
          const FigureMotion b = live.motion( 0, 8, 9, 0.5 );
          QVERIFY( near( a.lag, b.lag ) && near( a.steps, b.steps ) && near( a.turn_rate, b.turn_rate ) );
          // the legs started again after a move (frame 8), the oldest frames dropped: the same
          DispCont moved;
          for ( int i = 0; i < 20; ++i ) moved.push_back( frame( { { 0, i >= 8 ? 5.0f : 0.0f, 0.0f, i >= 10 ? 90.0f : 0.0f } } ) );
          GaitTable whole;
          whole.update( moved, 0.1 );
          GaitTable dropped;
          dropped.update( moved, 0.1 );
          moved.erase( moved.begin(), moved.begin() + 4 );
          dropped.update( moved, 0.1 );
          const FigureMotion c = whole.motion( 0, 10, 11, 0.5 );
          const FigureMotion d = dropped.motion( 0, 6, 7, 0.5 );
          QVERIFY2( near( c.lag, d.lag ) && c.lag > 20.0f, qPrintable( QString( "%1 %2" ).arg( c.lag ).arg( d.lag ) ) );
      }
};

QTEST_MAIN( TestGaitTable )

#include "test_gait_table.moc"
