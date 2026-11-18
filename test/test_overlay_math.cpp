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
};

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

QTEST_APPLESS_MAIN( TestOverlayMath )

#include "test_overlay_math.moc"
