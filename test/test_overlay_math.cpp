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

#include <algorithm>
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

//! the server's catch: a direction within [min, max] of the body puts the
//! point in the catch box of the length (rcssserver Player::goalieCatch)
bool
serverCanCatch( const rcss::rcg::ServerParamT & sp,
                const rcss::rcg::PlayerT & p,
                const double length,
                const Vector2D & point )
{
    const Vector2D rel = point - Vector2D( p.x(), p.y() );
    for ( double dir = sp.min_catch_angle_; dir <= sp.max_catch_angle_ + 1.0e-9; dir += 0.05 )
    {
        Vector2D v = rel;
        v.rotate( - ( p.body_ + dir ) );
        if ( 0.0 <= v.x && v.x <= length
             && std::fabs( v.y ) <= sp.catchable_area_w_ * 0.5 )
        {
            return true;
        }
    }
    return false;
}

//! the outline as a closed polygon: the arc in fine steps, then the edges
std::vector< Vector2D >
outlinePolygon( const OverlayMath::CatchOutline & outline )
{
    std::vector< Vector2D > poly;
    for ( size_t a = 0; a < outline.arcs_.size(); ++a )
    {
        const OverlayMath::Arc & arc = outline.arcs_[a];
        for ( int i = 0; i <= 720; ++i )
        {
            poly.push_back( arc.center_ + Vector2D::from_polar( arc.radius_, arc.start_ + arc.span_ * i / 720.0 ) );
        }
        if ( a == 0 )
        {
            poly.insert( poly.end(), outline.between_.begin(), outline.between_.end() );
        }
    }
    poly.insert( poly.end(), outline.edges_.begin(), outline.edges_.end() );
    return poly;
}

//! the point is within the polygon (even-odd)
bool
inPolygon( const std::vector< Vector2D > & poly,
           const Vector2D & point )
{
    bool in = false;
    for ( size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++ )
    {
        const Vector2D & a = poly[i];
        const Vector2D & b = poly[j];
        if ( ( a.y > point.y ) != ( b.y > point.y )
             && point.x < ( b.x - a.x ) * ( point.y - a.y ) / ( b.y - a.y ) + a.x )
        {
            in = ! in;
        }
    }
    return in;
}

//! the distance from the point to the polygon's sides
double
distToPolygon( const std::vector< Vector2D > & poly,
               const Vector2D & point )
{
    double d = 1.0e9;
    for ( size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++ )
    {
        const Vector2D ab = poly[i] - poly[j];
        const double len2 = ab.innerProduct( ab );
        double t = ( len2 > 0.0 ? ( point - poly[j] ).innerProduct( ab ) / len2 : 0.0 );
        t = std::min( 1.0, std::max( 0.0, t ) );
        d = std::min( d, point.dist( poly[j] + ab * t ) );
    }
    return d;
}

bool
nearPoint( const Vector2D & a,
           const Vector2D & b )
{
    return a.dist( b ) < 1.0e-6;
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
    void catchOutlineAllAround();
    void catchOutlineHalf();
    void catchOutlineNarrow();
    void catchOutlineWide();
    void catchOutlineMatchesServer();
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
  Before rcssserver 17.0.0 (and by default, when a log has no catch
  angles), the goalie catches all around: a circle
*/
void
TestOverlayMath::catchOutlineAllAround()
{
    rcss::rcg::ServerParamT sp;
    const rcss::rcg::PlayerT p = makePlayer( 1.0f, 2.0f, 30.0f );
    const OverlayMath::CatchOutline o = OverlayMath::catchOutline( sp, p, sp.catchable_area_l_ );
    QVERIFY( o.circle_ );
    QCOMPARE( o.arcs_.size(), size_t( 1 ) );
    QVERIFY( o.edges_.empty() && o.between_.empty() );
    QVERIFY( nearPoint( o.arcs_[0].center_, Vector2D( 1.0, 2.0 ) ) );
    QVERIFY( std::fabs( o.arcs_[0].radius_ - std::hypot( 0.5, 1.2 ) ) < 1.0e-9 );
    QCOMPARE( o.arcs_[0].span_, 360.0 );
}

/*-------------------------------------------------------------------*/
/*!
  From rcssserver 17.0.0 (-90 to +90): the arc in front, and a straight
  side half the catch box's width behind the goalie
*/
void
TestOverlayMath::catchOutlineHalf()
{
    rcss::rcg::ServerParamT sp;
    sp.max_catch_angle_ = 90.0;
    sp.min_catch_angle_ = -90.0;
    const rcss::rcg::PlayerT p = makePlayer( 1.0f, 2.0f, 30.0f );
    const double l = 1.4;
    const OverlayMath::CatchOutline o = OverlayMath::catchOutline( sp, p, l );
    QVERIFY( ! o.circle_ );
    QCOMPARE( o.arcs_.size(), size_t( 1 ) );
    QVERIFY( o.between_.empty() );
    const double diagonal = std::atan2( 0.5, l ) * 180.0 / M_PI;
    QVERIFY( std::fabs( o.arcs_[0].radius_ - std::hypot( 0.5, l ) ) < 1.0e-9 );
    QVERIFY( std::fabs( o.arcs_[0].start_ - ( 30.0 - 90.0 - diagonal ) ) < 1.0e-9 );
    QVERIFY( std::fabs( o.arcs_[0].span_ - ( 180.0 + diagonal * 2.0 ) ) < 1.0e-9 );

    // from the arc's end to its start: the far corners of the boxes at +90
    // and -90, both half the width behind
    const Vector2D c( 1.0, 2.0 );
    QCOMPARE( o.edges_.size(), size_t( 2 ) );
    QVERIFY( nearPoint( o.edges_.front(), c + Vector2D::from_polar( l, 120.0 ) + Vector2D::from_polar( 0.5, 210.0 ) ) );
    QVERIFY( nearPoint( o.edges_.back(), c + Vector2D::from_polar( l, -60.0 ) + Vector2D::from_polar( 0.5, -150.0 ) ) );
    QVERIFY( nearPoint( o.edges_.front(), c + Vector2D::from_polar( o.arcs_[0].radius_, o.arcs_[0].start_ + o.arcs_[0].span_ ) ) );
    QVERIFY( nearPoint( o.edges_.back(), c + Vector2D::from_polar( o.arcs_[0].radius_, o.arcs_[0].start_ ) ) );
}

/*-------------------------------------------------------------------*/
/*!
  Directions less than 180 degrees apart leave a gap behind the goalie:
  the sides go to the near corners and meet at the goalie
*/
void
TestOverlayMath::catchOutlineNarrow()
{
    rcss::rcg::ServerParamT sp;
    sp.max_catch_angle_ = 45.0;
    sp.min_catch_angle_ = -45.0;
    const rcss::rcg::PlayerT p = makePlayer( 0.0f, 0.0f, 0.0f );
    const OverlayMath::CatchOutline o = OverlayMath::catchOutline( sp, p, 1.2 );
    QCOMPARE( o.edges_.size(), size_t( 5 ) );
    QVERIFY( nearPoint( o.edges_[1], Vector2D::from_polar( 0.5, 135.0 ) ) );
    QVERIFY( nearPoint( o.edges_[2], Vector2D( 0.0, 0.0 ) ) );
    QVERIFY( nearPoint( o.edges_[3], Vector2D::from_polar( 0.5, -135.0 ) ) );

    // closer than twice the angle of a far corner (22.6 degrees): two arcs
    // of the corners on each side, joined by the far sides of the boxes at
    // the ends, which cross on the body direction
    sp.max_catch_angle_ = 10.0;
    sp.min_catch_angle_ = -10.0;
    const OverlayMath::CatchOutline two = OverlayMath::catchOutline( sp, p, 1.2 );
    QCOMPARE( two.arcs_.size(), size_t( 2 ) );
    QVERIFY( std::fabs( two.arcs_[0].span_ - 20.0 ) < 1.0e-9 );
    QCOMPARE( two.between_.size(), size_t( 3 ) );
    QVERIFY( nearPoint( two.between_[1], Vector2D( 1.2 / std::cos( 10.0 * M_PI / 180.0 ), 0.0 ) ) );
}

/*-------------------------------------------------------------------*/
/*!
  Directions more than 180 degrees apart: the sides of the boxes at the
  ends cross behind the goalie
*/
void
TestOverlayMath::catchOutlineWide()
{
    rcss::rcg::ServerParamT sp;
    sp.max_catch_angle_ = 135.0;
    sp.min_catch_angle_ = -135.0;
    const rcss::rcg::PlayerT p = makePlayer( 0.0f, 0.0f, 0.0f );
    const OverlayMath::CatchOutline o = OverlayMath::catchOutline( sp, p, 1.2 );
    QCOMPARE( o.edges_.size(), size_t( 3 ) );
    const Vector2D & x = o.edges_[1];
    QVERIFY( std::fabs( x.y ) < 1.0e-6 ); // on the line of the body, behind
    QVERIFY( x.x < 0.0 );
    // on the side of the box at +135
    QVERIFY( std::fabs( ( x - o.edges_[0] ).outerProduct( Vector2D::from_polar( 1.0, 135.0 ) ) ) < 1.0e-6 );

    // the arc closes: a circle
    sp.max_catch_angle_ = 170.0;
    sp.min_catch_angle_ = -170.0;
    QVERIFY( OverlayMath::catchOutline( sp, p, 1.2 ).circle_ );
}

/*-------------------------------------------------------------------*/
/*!
  Within the outline exactly where the server can catch (a grid of points,
  except those within 2 cm of the outline), for several catch angles,
  bodies and lengths
*/
void
TestOverlayMath::catchOutlineMatchesServer()
{
    struct Case { double min_; double max_; float body_; double length_; };
    const Case cases[] = {
        { -90.0, 90.0, 30.0f, 1.2 },
        { -45.0, 45.0, -100.0f, 1.2 },
        { -135.0, 135.0, 170.0f, 1.5 },
        { -30.0, 100.0, 60.0f, 1.0 },
        { -120.0, 100.0, -20.0f, 1.2 },
        { -150.0, 150.0, 0.0f, 2.0 },
        { 0.0, 0.0, 45.0f, 1.2 },
        { -5.0, 25.0, -60.0f, 1.2 },
    };
    for ( const Case & k : cases )
    {
        rcss::rcg::ServerParamT sp;
        sp.min_catch_angle_ = k.min_;
        sp.max_catch_angle_ = k.max_;
        const rcss::rcg::PlayerT p = makePlayer( 3.0f, -2.0f, k.body_ );
        const OverlayMath::CatchOutline o = OverlayMath::catchOutline( sp, p, k.length_ );
        QVERIFY( ! o.circle_ );
        const std::vector< Vector2D > poly = outlinePolygon( o );
        int checked = 0;
        for ( double x = -2.6; x <= 2.6; x += 0.1 )
        {
            for ( double y = -2.6; y <= 2.6; y += 0.1 )
            {
                const Vector2D point( 3.0 + x, -2.0 + y );
                if ( distToPolygon( poly, point ) < 0.02 )
                {
                    continue;
                }
                ++checked;
                if ( inPolygon( poly, point ) != serverCanCatch( sp, p, k.length_, point ) )
                {
                    QFAIL( qPrintable( QString( "min %1 max %2 body %3: (%4, %5)" )
                                       .arg( k.min_ ).arg( k.max_ ).arg( k.body_ ).arg( x ).arg( y ) ) );
                }
            }
        }
        QVERIFY( checked > 2000 );
    }
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
