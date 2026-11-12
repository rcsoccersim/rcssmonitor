// -*-c++-*-

/*!
  \file test_overlay_math.cpp
  \brief unit test of OverlayMath
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

#include "overlay_math.h"

#include <cmath>

#include <vector>

namespace {

/*!
  \brief make show data with the given players. The other players are
  not connected (state 0).
  \param left_x x of the left players from unum 1
  \param right_x x of the right players from unum 1
  \param ball_x x of the ball
*/
rcss::rcg::ShowInfoT
makeShow( const std::vector< float > & left_x,
          const std::vector< float > & right_x,
          const float ball_x )
{
    rcss::rcg::ShowInfoT show;
    show.ball_.x_ = ball_x;
    for ( int i = 0; i < rcss::rcg::MAX_PLAYER; ++i )
    {
        rcss::rcg::PlayerT & l = show.player_[i];
        l.side_ = 'l';
        l.unum_ = static_cast< rcss::rcg::Int16 >( i + 1 );
        if ( i < static_cast< int >( left_x.size() ) )
        {
            l.state_ = 1;
            l.x_ = left_x[i];
        }

        rcss::rcg::PlayerT & r = show.player_[rcss::rcg::MAX_PLAYER + i];
        r.side_ = 'r';
        r.unum_ = static_cast< rcss::rcg::Int16 >( i + 1 );
        if ( i < static_cast< int >( right_x.size() ) )
        {
            r.state_ = 1;
            r.x_ = right_x[i];
        }
    }
    return show;
}

}

/*!
  \brief unit test of OverlayMath
*/
class TestOverlayMath
    : public QObject {
    Q_OBJECT

private slots:

    void offsideAtSecondLastPlayer();
    void offsideAtBall();
    void offsideAtHalfwayLine();
    void offsideWithinPitch();
    void offsideIgnoresDisconnected();
    void focusPoint();
    void catchArea();
    void tackleChances();
    void tackleText();
    void kickAccelNotKickable();
    void kickAccelArcs();
    void kickAccelCircle();
    void ballPath();
};

namespace {

//! a player of the left team at (x, y), facing body
rcss::rcg::PlayerT
makePlayer( const float x,
            const float y,
            const float body )
{
    rcss::rcg::PlayerT p;
    p.side_ = 'l';
    p.unum_ = 1;
    p.state_ = rcss::rcg::STAND;
    p.x_ = x;
    p.y_ = y;
    p.body_ = body;
    return p;
}

rcss::rcg::BallT
makeBall( const float x,
          const float y,
          const float vx,
          const float vy )
{
    rcss::rcg::BallT b;
    b.x_ = x;
    b.y_ = y;
    b.vx_ = vx;
    b.vy_ = vy;
    return b;
}

}

/*-------------------------------------------------------------------*/
/*!
  The line is at the second last player of each team. The lines used to
  stay at x = 0 (between x = 0 and the ball), as no player matched the
  side check.
*/
void
TestOverlayMath::offsideAtSecondLastPlayer()
{
    const rcss::rcg::ShowInfoT show = makeShow( { -50.0f, -20.0f, -30.0f, -10.0f, 5.0f },
                                                { 50.0f, 20.0f, 35.0f, 10.0f, -5.0f },
                                                0.0f );
    const OverlayMath::OffsideLines lines = OverlayMath::offsideLines( show );
    QCOMPARE( lines.left_, -30.0f );
    QCOMPARE( lines.right_, 35.0f );
}

/*-------------------------------------------------------------------*/
/*!
  The ball nearer to the goal line than the second last player sets the line.
*/
void
TestOverlayMath::offsideAtBall()
{
    const OverlayMath::OffsideLines left
        = OverlayMath::offsideLines( makeShow( { -50.0f, -30.0f }, { 50.0f, 35.0f }, -40.0f ) );
    QCOMPARE( left.left_, -40.0f );
    QCOMPARE( left.right_, 35.0f );

    const OverlayMath::OffsideLines right
        = OverlayMath::offsideLines( makeShow( { -50.0f, -30.0f }, { 50.0f, 35.0f }, 45.0f ) );
    QCOMPARE( right.left_, -30.0f );
    QCOMPARE( right.right_, 45.0f );
}

/*-------------------------------------------------------------------*/
/*!
  With fewer than two players in their own half, the line is at x = 0.
*/
void
TestOverlayMath::offsideAtHalfwayLine()
{
    const OverlayMath::OffsideLines lines
        = OverlayMath::offsideLines( makeShow( { -50.0f, 10.0f, 20.0f }, { 50.0f, -10.0f }, 0.0f ) );
    QCOMPARE( lines.left_, 0.0f );
    QCOMPARE( lines.right_, 0.0f );
}

/*-------------------------------------------------------------------*/
/*!
  The lines do not go beyond the goal lines, even with the ball out.
*/
void
TestOverlayMath::offsideWithinPitch()
{
    const OverlayMath::OffsideLines left
        = OverlayMath::offsideLines( makeShow( { -50.0f, -30.0f }, { 50.0f, 35.0f }, -60.0f ) );
    QCOMPARE( left.left_, -52.5f );

    const OverlayMath::OffsideLines right
        = OverlayMath::offsideLines( makeShow( { -50.0f, -30.0f }, { 50.0f, 35.0f }, 60.0f ) );
    QCOMPARE( right.right_, 52.5f );
}

/*-------------------------------------------------------------------*/
/*!
  Players not connected (state 0) do not count.
*/
void
TestOverlayMath::offsideIgnoresDisconnected()
{
    rcss::rcg::ShowInfoT show = makeShow( { -50.0f, -20.0f }, { 50.0f, 20.0f }, 0.0f );
    rcss::rcg::PlayerT & left = show.player_[5];
    left.state_ = 0;
    left.x_ = -45.0f;
    rcss::rcg::PlayerT & right = show.player_[rcss::rcg::MAX_PLAYER + 5];
    right.state_ = 0;
    right.x_ = 45.0f;

    const OverlayMath::OffsideLines lines = OverlayMath::offsideLines( show );
    QCOMPARE( lines.left_, -20.0f );
    QCOMPARE( lines.right_, 20.0f );
}

/*-------------------------------------------------------------------*/
/*!
  The focus point: along body + neck + focus direction, at the focus distance
*/
void
TestOverlayMath::focusPoint()
{
    rcss::rcg::PlayerT p = makePlayer( 1.0f, 2.0f, 30.0f );
    p.neck_ = 30.0f;
    p.focus_dir_ = 30.0f;
    Vector2D point;
    QVERIFY( ! OverlayMath::focusPoint( p, &point ) ); // no focus distance

    p.focus_dist_ = 4.0f;
    QVERIFY( OverlayMath::focusPoint( p, &point ) );
    QVERIFY( std::fabs( point.x - 1.0 ) < 1.0e-5 );
    QVERIFY( std::fabs( point.y - 6.0 ) < 1.0e-5 );
}

/*-------------------------------------------------------------------*/
/*!
  The catchable radius reaches the far corners of the catch box; a stretch
  over 1 gives the largest and the smallest
*/
void
TestOverlayMath::catchArea()
{
    rcss::rcg::ServerParamT sp;
    rcss::rcg::PlayerTypeT type;
    OverlayMath::CatchArea area = OverlayMath::catchArea( sp, type );
    QVERIFY( std::fabs( area.catchable_ - std::hypot( sp.catchable_area_w_ / 2, sp.catchable_area_l_ ) ) < 1.0e-9 );
    QVERIFY( ! area.stretched_ );

    type.catchable_area_l_stretch_ = 1.3;
    area = OverlayMath::catchArea( sp, type );
    QVERIFY( area.stretched_ );
    QVERIFY( std::fabs( area.max_ - std::hypot( sp.catchable_area_w_ / 2, sp.catchable_area_l_ * 1.3 ) ) < 1.0e-9 );
    QVERIFY( std::fabs( area.min_ - std::hypot( sp.catchable_area_w_ / 2, sp.catchable_area_l_ * 0.7 ) ) < 1.0e-9 );
}

/*-------------------------------------------------------------------*/
/*!
  The chances of a tackle and a foul: 1 - (x / dist)^e - (y / width)^e,
  relative to the body
*/
void
TestOverlayMath::tackleChances()
{
    rcss::rcg::ServerParamT sp;
    // facing +y, the ball 1 m ahead: half the tackle distance
    const rcss::rcg::PlayerT p = makePlayer( 0.0f, 0.0f, 90.0f );
    OverlayMath::TackleChances c = OverlayMath::tackleChances( sp, p, makeBall( 0.0f, 1.0f, 0.0f, 0.0f ) );
    QVERIFY( c.tackle_ && c.foul_ );
    QVERIFY( std::fabs( c.tackle_chance_ - ( 1.0 - std::pow( 0.5, sp.tackle_exponent_ ) ) ) < 1.0e-6 );
    QVERIFY( std::fabs( c.foul_chance_ - ( 1.0 - std::pow( 0.5, sp.foul_exponent_ ) ) ) < 1.0e-6 );

    // too far ahead
    c = OverlayMath::tackleChances( sp, p, makeBall( 0.0f, 2.5f, 0.0f, 0.0f ) );
    QVERIFY( ! c.any() );
    // behind, with no tackle distance behind (the default 0)
    c = OverlayMath::tackleChances( sp, p, makeBall( 0.0f, -0.5f, 0.0f, 0.0f ) );
    QVERIFY( ! c.any() );
    // ahead and aside, 9/10 of the way to each edge: only the foul (its
    // larger exponent makes it reach farther)
    c = OverlayMath::tackleChances( sp, p, makeBall( float( -0.9 * sp.tackle_width_ ), float( 0.9 * sp.tackle_dist_ ), 0.0f, 0.0f ) );
    QVERIFY( ! c.tackle_ );
    QVERIFY( c.foul_ );
    QVERIFY( std::fabs( c.foul_chance_ - ( 1.0 - 2.0 * std::pow( 0.9, sp.foul_exponent_ ) ) ) < 1.0e-5 );
}

/*-------------------------------------------------------------------*/
/*!
  The texts as the 2D view writes them
*/
void
TestOverlayMath::tackleText()
{
    QCOMPARE( OverlayMath::tackleText( { true, 0.5, true, 0.8126 } ), QString( "T=0.5,F=0.813" ) );
    QCOMPARE( OverlayMath::tackleText( { true, 0.25, false, 0.0 } ), QString( "Tackle=0.25" ) );
    QCOMPARE( OverlayMath::tackleText( { false, 0.0, true, 0.999999 } ), QString( "Foul=1" ) );
    QCOMPARE( OverlayMath::tackleText( { false, 0.0, false, 0.0 } ), QString() );
}

/*-------------------------------------------------------------------*/
void
TestOverlayMath::kickAccelNotKickable()
{
    rcss::rcg::ServerParamT sp;
    rcss::rcg::PlayerTypeT type;
    const rcss::rcg::PlayerT p = makePlayer( 0.0f, 0.0f, 0.0f );
    // out of the kickable area
    QVERIFY( ! OverlayMath::kickAccel( sp, type, p, makeBall( 2.0f, 0.0f, 0.0f, 0.0f ) ).kickable_ );
    // no velocity in the show
    rcss::rcg::BallT ball = makeBall( 0.5f, 0.0f, 0.0f, 0.0f );
    ball.vx_ = rcss::rcg::SHOWINFO_SCALE2F;
    QVERIFY( ! OverlayMath::kickAccel( sp, type, p, ball ).kickable_ );
}

/*-------------------------------------------------------------------*/
/*!
  A ball at the player's feet: the largest acceleration by the distance
  and the direction, and two arcs that meet at the crossings of the circle
  of the fastest ball and the circle of the largest acceleration
*/
void
TestOverlayMath::kickAccelArcs()
{
    rcss::rcg::ServerParamT sp;
    rcss::rcg::PlayerTypeT type;
    const rcss::rcg::PlayerT p = makePlayer( 0.0f, 0.0f, 0.0f );
    const rcss::rcg::BallT ball = makeBall( 0.6f, 0.0f, 1.5f, 0.0f ); // ahead, rolling ahead fast
    const OverlayMath::KickAccel kick = OverlayMath::kickAccel( sp, type, p, ball );
    QVERIFY( kick.kickable_ );
    const double expected = std::min( sp.max_power_ * type.kick_power_rate_
                                      * ( 1.0 - 0.25 * ( 0.6 - type.player_size_ - sp.ball_size_ ) / type.kickable_margin_ ),
                                      sp.ball_accel_max_ );
    QVERIFY( std::fabs( kick.max_accel_ - expected ) < 1.0e-5 );
    QVERIFY( std::fabs( kick.next_.x - 2.1 ) < 1.0e-5 );
    QCOMPARE( kick.arcs_.size(), size_t( 2 ) );

    // both arcs end at the same two points, on both circles
    auto end_points = []( const OverlayMath::Arc & a ) {
        return std::make_pair( a.center_ + Vector2D::from_polar( a.radius_, a.start_ ),
                               a.center_ + Vector2D::from_polar( a.radius_, a.start_ + a.span_ ) );
    };
    const auto s = end_points( kick.arcs_[0] ), n = end_points( kick.arcs_[1] );
    QVERIFY( s.first.dist( n.second ) < 1.0e-4 || s.first.dist( n.first ) < 1.0e-4 );
    QVERIFY( s.second.dist( n.first ) < 1.0e-4 || s.second.dist( n.second ) < 1.0e-4 );
    QVERIFY( std::fabs( s.first.dist( kick.next_ ) - kick.max_accel_ ) < 1.0e-4 );
    QVERIFY( std::fabs( n.first.dist( Vector2D( ball.x_, ball.y_ ) ) - sp.ball_speed_max_ ) < 1.0e-4 );
    // the arc of the fastest ball is on the side of the next place: its middle is ahead
    const OverlayMath::Arc & a = kick.arcs_[0];
    QVERIFY( ( a.center_ + Vector2D::from_polar( a.radius_, a.start_ + a.span_ / 2 ) ).x > ball.x_ );
    QVERIFY( a.span_ > 0.0 && a.span_ < 360.0 );
}

/*-------------------------------------------------------------------*/
/*!
  A still ball: the circles do not cross, the circle of the largest
  acceleration around the next place
*/
void
TestOverlayMath::kickAccelCircle()
{
    rcss::rcg::ServerParamT sp;
    rcss::rcg::PlayerTypeT type;
    const OverlayMath::KickAccel kick = OverlayMath::kickAccel( sp, type, makePlayer( 0.0f, 0.0f, 0.0f ), makeBall( 0.5f, 0.0f, 0.0f, 0.0f ) );
    QVERIFY( kick.kickable_ );
    QCOMPARE( kick.arcs_.size(), size_t( 1 ) );
    QCOMPARE( kick.arcs_[0].span_, 360.0 );
    QCOMPARE( kick.arcs_[0].radius_, kick.max_accel_ );
}

/*-------------------------------------------------------------------*/
/*!
  The ball's path: decaying, until it is slower than the step, at most the cycles (and 100)
*/
void
TestOverlayMath::ballPath()
{
    rcss::rcg::ServerParamT sp;
    std::vector< Vector2D > path = OverlayMath::ballPath( sp, makeBall( 0.0f, 0.0f, 2.0f, 0.0f ), 3, 0.01 );
    QCOMPARE( path.size(), size_t( 3 ) );
    QVERIFY( std::fabs( path[0].x - 2.0 ) < 1.0e-6 );
    QVERIFY( std::fabs( path[1].x - ( 2.0 + 2.0 * sp.ball_decay_ ) ) < 1.0e-6 );

    path = OverlayMath::ballPath( sp, makeBall( 0.0f, 0.0f, 2.0f, 0.0f ), 1000, 0.01 );
    QVERIFY( path.size() < 100 );
    QVERIFY( 2.0 * std::pow( sp.ball_decay_, double( path.size() ) ) < 0.01 );
    QVERIFY( 2.0 * std::pow( sp.ball_decay_, double( path.size() - 1 ) ) >= 0.01 );
    path = OverlayMath::ballPath( sp, makeBall( 0.0f, 0.0f, 2.0f, 0.0f ), 1000, 0.0 );
    QCOMPARE( path.size(), size_t( 100 ) );

    // still, or no velocity: none
    QVERIFY( OverlayMath::ballPath( sp, makeBall( 0.0f, 0.0f, 0.0f, 0.0f ), 10, 0.01 ).empty() );
}

QTEST_APPLESS_MAIN( TestOverlayMath )

#include "test_overlay_math.moc"
