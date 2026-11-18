// -*-c++-*-

/*!
  \file test_rcg_types.cpp
  \brief unit test of the parameter types of rcss/rcg
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

#include <cmath>
#include <sstream>
#include <string>

/*!
  \brief unit test of the parameter types of rcss/rcg
*/
class TestRcgTypes
    : public QObject {
    Q_OBJECT

private slots:

    void serverParamDefaults();
    void playerParamDefaults();
    void playerTypeDefaults();
    void newServerParams();
    void playerTypeNoiseRates();
};

/*-------------------------------------------------------------------*/
/*!
  The defaults are those of rcssserver 19.0.0 (B7), used when a log or a
  server does not have a parameter. Some of those that changed.
*/
void
TestRcgTypes::serverParamDefaults()
{
    const rcss::rcg::ServerParamT param;
    QCOMPARE( param.player_speed_max_, 1.05 );
    QCOMPARE( param.stamina_max_, 8000.0 );
    QCOMPARE( param.stamina_capacity_, 130600.0 );
    QCOMPARE( param.extra_stamina_, 50.0 );
    QCOMPARE( param.max_dash_angle_, 180.0 );
    QCOMPARE( param.min_dash_angle_, -180.0 );
    QCOMPARE( param.dash_angle_step_, 1.0 );
    QCOMPARE( param.side_dash_rate_, 0.4 );
    QCOMPARE( param.back_dash_rate_, 0.7 );
    QCOMPARE( param.min_dash_power_, 0.0 );
    QCOMPARE( param.tackle_width_, 1.25 );
    QCOMPARE( param.tackle_back_dist_, 0.0 );
    QCOMPARE( param.max_catch_angle_, 90.0 );
    QCOMPARE( param.min_catch_angle_, -90.0 );
    QCOMPARE( param.drop_ball_time_, 100 );
    QCOMPARE( param.extra_half_time_, 100 );
    QCOMPARE( param.game_log_version_, 6 );
    QCOMPARE( param.golden_goal_, false );
    QCOMPARE( param.effort_inc_, 0.01 );
    QCOMPARE( param.ball_size_, 0.085 );
}

/*-------------------------------------------------------------------*/
void
TestRcgTypes::playerParamDefaults()
{
    const rcss::rcg::PlayerParamT param;
    QCOMPARE( param.player_types_, 18 );
    QCOMPARE( param.player_decay_delta_min_, -0.1 );
    QCOMPARE( param.extra_stamina_delta_max_, 50.0 );
    QCOMPARE( param.effort_max_delta_factor_, -0.004 );
    QCOMPARE( param.new_dash_power_rate_delta_min_, -0.0012 );
    QCOMPARE( param.new_dash_power_rate_delta_max_, 0.0008 );
    QCOMPARE( param.catchable_area_l_stretch_min_, 1.0 );
    QCOMPARE( param.catchable_area_l_stretch_max_, 1.3 );
}

/*-------------------------------------------------------------------*/
/*!
  The default player type is the type 0 of rcssserver: dash_power_rate was
  0.06 (a typo of 0.006), and team_far_length and team_too_far_length are
  the diagonal of the pitch since 2022.
*/
void
TestRcgTypes::playerTypeDefaults()
{
    const rcss::rcg::PlayerTypeT type;
    QCOMPARE( type.player_speed_max_, 1.05 );
    QCOMPARE( type.dash_power_rate_, 0.006 );
    QCOMPARE( type.extra_stamina_, 50.0 );
    QVERIFY( std::abs( type.team_far_length_ - 125.096 ) < 0.001 );
    QVERIFY( std::abs( type.team_too_far_length_ - 125.096 ) < 0.001 );
    QCOMPARE( type.unum_far_length_, 20.0 );
}

/*-------------------------------------------------------------------*/
/*!
  The parameters that the servers accepting TCP and WebSocket connections
  send are known.
*/
void
TestRcgTypes::newServerParams()
{
    rcss::rcg::ServerParamT param;
    QVERIFY( param.setValue( "tcp_port", "6100" ) );
    QVERIFY( param.setValue( "tcp_coach_port", "6101" ) );
    QVERIFY( param.setValue( "tcp_olcoach_port", "6102" ) );
    QVERIFY( param.setValue( "ws_port", "6200" ) );
    QVERIFY( param.setValue( "ws_allowed_origins", "\"http://localhost\"" ) );
    QCOMPARE( param.tcp_port_, 6100 );
    QCOMPARE( param.tcp_coach_port_, 6101 );
    QCOMPARE( param.tcp_olcoach_port_, 6102 );
    QCOMPARE( param.ws_port_, 6200 );
    QCOMPARE( param.ws_allowed_origins_, std::string( "http://localhost" ) );

    rcss::rcg::ServerParamT copy;
    copy.copyFrom( param );
    QCOMPARE( copy.ws_port_, 6200 );
}

/*-------------------------------------------------------------------*/
/*!
  The four noise rates of a player type (v19) are written with their own
  values; all four were written with dist_noise_rate.
*/
void
TestRcgTypes::playerTypeNoiseRates()
{
    rcss::rcg::PlayerTypeT type;
    QVERIFY( type.setValue( "dist_noise_rate", "0.1" ) );
    QVERIFY( type.setValue( "focus_dist_noise_rate", "0.2" ) );
    QVERIFY( type.setValue( "land_dist_noise_rate", "0.3" ) );
    QVERIFY( type.setValue( "land_focus_dist_noise_rate", "0.4" ) );

    std::ostringstream sexp;
    type.toServerString( sexp );
    QVERIFY( sexp.str().find( "(dist_noise_rate 0.1)" ) != std::string::npos );
    QVERIFY( sexp.str().find( "(focus_dist_noise_rate 0.2)" ) != std::string::npos );
    QVERIFY( sexp.str().find( "(land_dist_noise_rate 0.3)" ) != std::string::npos );
    QVERIFY( sexp.str().find( "(land_focus_dist_noise_rate 0.4)" ) != std::string::npos );

    std::ostringstream json;
    type.toJSON( json );
    QVERIFY( json.str().find( "\"focus_dist_noise_rate\":0.2" ) != std::string::npos );
    QVERIFY( json.str().find( "\"land_dist_noise_rate\":0.3" ) != std::string::npos );
    QVERIFY( json.str().find( "\"land_focus_dist_noise_rate\":0.4" ) != std::string::npos );
}

QTEST_APPLESS_MAIN( TestRcgTypes )

#include "test_rcg_types.moc"
