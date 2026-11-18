// -*-c++-*-

/*!
  \file test_rcg_util.cpp
  \brief unit test of the rcg utilities
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

#include "rcss/rcg/types.h"
#include "rcss/rcg/util.h"

/*!
  \brief unit test of the rcg utilities
*/
class TestRcgUtil
    : public QObject {
    Q_OBJECT

private slots:

    void playModeNames();
};

/*-------------------------------------------------------------------*/
/*!
  Every play mode name in PLAYMODE_STRINGS converts back to its play mode.
  "foul_push_r" was mapped as "foul_push_l" and became PM_Null.
*/
void
TestRcgUtil::playModeNames()
{
    static const char * names[] = PLAYMODE_STRINGS;
    // PLAYMODE_STRINGS ends with two empty strings after the last mode.
    QVERIFY( int( sizeof( names ) / sizeof( names[0] ) ) >= int( rcss::rcg::PM_MAX ) );

    for ( int i = 1; i < rcss::rcg::PM_MAX; ++i )
    {
        const rcss::rcg::PlayMode pm = rcss::rcg::to_playmode_enum( names[i] );
        QVERIFY2( pm == i,
                  qPrintable( QString( "%1 -> %2, expected %3" )
                              .arg( names[i] ).arg( int( pm ) ).arg( i ) ) );
    }

    QCOMPARE( rcss::rcg::to_playmode_enum( "foul_push_r" ), rcss::rcg::PM_Foul_Push_Right );
    QCOMPARE( rcss::rcg::to_playmode_enum( "no_such_mode" ), rcss::rcg::PM_Null );
}

QTEST_APPLESS_MAIN( TestRcgUtil )

#include "test_rcg_util.moc"
