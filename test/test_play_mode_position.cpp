// -*-c++-*-

/*!
  \file test_play_mode_position.cpp
  \brief unit test of playModeBallPosition()
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

#include "play_mode_position.h"

/*!
  \brief unit test of playModeBallPosition()
*/
class TestPlayModePosition
    : public QObject {
    Q_OBJECT

private slots:

    void backPass();
    void otherModes();
    void withinPitch();
    void unsupported();
};

/*-------------------------------------------------------------------*/
/*!
  back_pass_l / _r are offered, and the ball goes into the penalty area of
  the team that made the back pass. Change Playmode used to send nothing
  for them.
*/
void
TestPlayModePosition::backPass()
{
    // left: the penalty area is x <= -36, |y| <= 20.16
    double x = 10.0, y = 30.0;
    QVERIFY( playModeBallPosition( rcss::rcg::PM_Back_Pass_Left, &x, &y ) );
    QCOMPARE( x, -36.0 );
    QCOMPARE( y, 20.16 );

    x = -40.0; y = -5.0;
    QVERIFY( playModeBallPosition( rcss::rcg::PM_Back_Pass_Left, &x, &y ) );
    QCOMPARE( x, -40.0 );
    QCOMPARE( y, -5.0 );

    // right: the penalty area is x >= 36
    x = -10.0; y = -30.0;
    QVERIFY( playModeBallPosition( rcss::rcg::PM_Back_Pass_Right, &x, &y ) );
    QCOMPARE( x, 36.0 );
    QCOMPARE( y, -20.16 );
}

/*-------------------------------------------------------------------*/
/*!
  The positions of the other modes, as before.
*/
void
TestPlayModePosition::otherModes()
{
    double x = 10.0, y = 5.0;
    QVERIFY( playModeBallPosition( rcss::rcg::PM_PlayOn, &x, &y ) );
    QCOMPARE( x, 10.0 );
    QCOMPARE( y, 5.0 );

    x = 10.0; y = 5.0;
    QVERIFY( playModeBallPosition( rcss::rcg::PM_BeforeKickOff, &x, &y ) );
    QCOMPARE( x, 0.0 );
    QCOMPARE( y, 0.0 );

    x = 10.0; y = -5.0;
    QVERIFY( playModeBallPosition( rcss::rcg::PM_KickIn_Right, &x, &y ) );
    QCOMPARE( x, 10.0 );
    QCOMPARE( y, -34.0 );

    x = 10.0; y = 5.0;
    QVERIFY( playModeBallPosition( rcss::rcg::PM_CornerKick_Left, &x, &y ) );
    QCOMPARE( x, 51.5 );
    QCOMPARE( y, 33.0 );

    x = 10.0; y = -5.0;
    QVERIFY( playModeBallPosition( rcss::rcg::PM_GoalKick_Left, &x, &y ) );
    QCOMPARE( x, -47.0 );
    QCOMPARE( y, -9.16 );

    // an indirect free kick of the left team goes out of the right goal area
    x = 50.0; y = 3.0;
    QVERIFY( playModeBallPosition( rcss::rcg::PM_IndFreeKick_Left, &x, &y ) );
    QCOMPARE( x, 47.0 );
    QCOMPARE( y, 3.0 );

    x = 10.0; y = 5.0;
    QVERIFY( playModeBallPosition( rcss::rcg::PM_Foul_Push_Right, &x, &y ) );
    QCOMPARE( x, 10.0 );
    QCOMPARE( y, 5.0 );

    x = 10.0; y = 5.0;
    QVERIFY( playModeBallPosition( rcss::rcg::PM_Illegal_Defense_Right, &x, &y ) );
    QCOMPARE( x, 41.5 );
    QCOMPARE( y, 0.0 );
}

/*-------------------------------------------------------------------*/
/*!
  A point outside the pitch is first brought onto it.
*/
void
TestPlayModePosition::withinPitch()
{
    double x = 60.0, y = -40.0;
    QVERIFY( playModeBallPosition( rcss::rcg::PM_PlayOn, &x, &y ) );
    QCOMPARE( x, 52.5 );
    QCOMPARE( y, -34.0 );
}

/*-------------------------------------------------------------------*/
/*!
  A mode that Change Playmode does not offer is refused, and the point is
  left as it is.
*/
void
TestPlayModePosition::unsupported()
{
    double x = 10.0, y = 5.0;
    QVERIFY( ! playModeBallPosition( rcss::rcg::PM_TimeOver, &x, &y ) );
    QVERIFY( ! playModeBallPosition( rcss::rcg::PM_AfterGoal_Left, &x, &y ) );
    QCOMPARE( x, 10.0 );
    QCOMPARE( y, 5.0 );
}

QTEST_APPLESS_MAIN( TestPlayModePosition )

#include "test_play_mode_position.moc"
