// -*-c++-*-

/*!
  \file test_sight.cpp
  \brief unit test of what the selected player perceives
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

#include "sight.h"

#include <cmath>

//! the styles of the player camera's sight (the namespace PlayerSight has the calculations)
using Style = PlayerCameraStyle::Sight;

namespace {

rcss::rcg::PlayerT
makePlayer( const char side,
            const int unum,
            const float x,
            const float y,
            const float body = 0.0f,
            const float neck = 0.0f,
            const float view_width = 90.0f )
{
    rcss::rcg::PlayerT p;
    p.side_ = side;
    p.unum_ = static_cast< rcss::rcg::Int16 >( unum );
    p.state_ = rcss::rcg::STAND;
    p.x_ = x;
    p.y_ = y;
    p.body_ = body;
    p.neck_ = neck;
    p.view_width_ = view_width;
    return p;
}

bool
same( const Perception & a,
      const Perception & b )
{
    return a.seen == b.seen && std::fabs( a.team - b.team ) < 1.0e-5f
        && std::fabs( a.unum - b.unum ) < 1.0e-5f && a.facing == b.facing;
}

//! the type with the team fading from 40 to 60 m (the defaults no longer fade it)
rcss::rcg::PlayerTypeT
fadingType()
{
    rcss::rcg::PlayerTypeT type;
    type.team_far_length_ = 40.0;
    type.team_too_far_length_ = 60.0;
    return type;
}

}

/*!
  \brief unit test of Sight and Perceptions
*/
class TestSight
    : public QObject {
    Q_OBJECT

private:
    rcss::rcg::ServerParamT M_param;

private slots:

    void omitProbability()
      {
          QCOMPARE( PlayerSight::omitProbability( 10.0, 20.0, 40.0 ), 0.0 );
          QCOMPARE( PlayerSight::omitProbability( 30.0, 20.0, 40.0 ), 0.5 );
          QCOMPARE( PlayerSight::omitProbability( 50.0, 20.0, 40.0 ), 1.0 );
          // too_far not beyond far: all or nothing at far
          QCOMPARE( PlayerSight::omitProbability( 100.0, 125.0, 125.0 ), 0.0 );
          QCOMPARE( PlayerSight::omitProbability( 126.0, 125.0, 125.0 ), 1.0 );
      }

    //! everything, or what is inside the view cone around the face
    void allAndView()
      {
          // facing +y by the neck (body 0 + neck 90), a view of 90 degrees
          const rcss::rcg::PlayerT me = makePlayer( 'l', 1, 0, 0, 0.0f, 90.0f );
          const rcss::rcg::PlayerTypeT type;
          const rcss::rcg::PlayerT ahead = makePlayer( 'r', 2, 0, 30 );
          const rcss::rcg::PlayerT aside = makePlayer( 'r', 3, 30, 0 ); // where the body faces, not the face
          QVERIFY( same( PlayerSight::perceivePlayer( Style::All, me, aside, type, M_param ), PlayerSight::FULL ) );
          QVERIFY( same( PlayerSight::perceivePlayer( Style::View, me, ahead, type, M_param ), PlayerSight::FULL ) );
          QVERIFY( same( PlayerSight::perceivePlayer( Style::View, me, aside, type, M_param ), PlayerSight::UNSEEN ) );
          // the edge of the view: 45 degrees
          QVERIFY( PlayerSight::perceivePlayer( Style::View, me, makePlayer( 'r', 4, 20, 22 ), type, M_param ).seen );
          QVERIFY( ! PlayerSight::perceivePlayer( Style::View, me, makePlayer( 'r', 4, 22, 20 ), type, M_param ).seen );
          // a narrow view (60)
          rcss::rcg::PlayerT narrow = me;
          narrow.view_width_ = 60.0f;
          QVERIFY( ! PlayerSight::perceivePlayer( Style::View, narrow, makePlayer( 'r', 4, 20, 22 ), type, M_param ).seen );

      }

    /*!
      The server's rules: in the view, the number fades from unum_far_length
      to unum_too_far_length (and the directions with it), the team from
      team_far_length; beyond the observation length, not seen; outside the
      view, felt within visible_distance
    */
    void server()
      {
          const rcss::rcg::PlayerT me = makePlayer( 'l', 1, 0, 0 ); // facing +x
          const rcss::rcg::PlayerTypeT type = fadingType();

          QVERIFY( same( PlayerSight::perceivePlayer( Style::Server, me, makePlayer( 'r', 2, 10, 0 ), type, M_param ), PlayerSight::FULL ) );
          // 30 m: the number half known, the team known
          QVERIFY( same( PlayerSight::perceivePlayer( Style::Server, me, makePlayer( 'r', 2, 30, 0 ), type, M_param ),
                         Perception{ true, 1.0f, 0.5f, true } ) );
          // 45 m: no number (nor directions), the team 75% known
          QVERIFY( same( PlayerSight::perceivePlayer( Style::Server, me, makePlayer( 'r', 2, 45, 0 ), type, M_param ),
                         Perception{ true, 0.75f, 0.0f, false } ) );
          // 70 m: seen, nothing known
          QVERIFY( same( PlayerSight::perceivePlayer( Style::Server, me, makePlayer( 'r', 2, 70, 0 ), type, M_param ),
                         Perception{ true, 0.0f, 0.0f, false } ) );
          // without the team, the number is left out too: with the team
          // fading first (20 to 40 m), the number at 30 m is as likely as the team
          rcss::rcg::PlayerTypeT team_first = type;
          team_first.team_far_length_ = 20.0;
          team_first.team_too_far_length_ = 40.0;
          team_first.unum_far_length_ = 30.0;
          team_first.unum_too_far_length_ = 50.0;
          QVERIFY( same( PlayerSight::perceivePlayer( Style::Server, me, makePlayer( 'r', 2, 30, 0 ), team_first, M_param ),
                         Perception{ true, 0.5f, 0.5f, true } ) );

          // beyond the observation length
          rcss::rcg::PlayerTypeT short_sight = type;
          short_sight.player_max_observation_length_ = 50.0;
          QVERIFY( ! PlayerSight::perceivePlayer( Style::Server, me, makePlayer( 'r', 2, 55, 0 ), short_sight, M_param ).seen );
          // behind: felt within visible_distance, else not seen
          QVERIFY( same( PlayerSight::perceivePlayer( Style::Server, me, makePlayer( 'r', 2, -2, 0 ), type, M_param ), PlayerSight::FELT ) );
          QVERIFY( ! PlayerSight::perceivePlayer( Style::Server, me, makePlayer( 'r', 2, -4, 0 ), type, M_param ).seen );

          // the ball: in the view up to its observation length, or felt
          rcss::rcg::BallT ball;
          ball.x_ = 40.0f;
          QVERIFY( PlayerSight::perceiveBall( Style::Server, me, ball, type, M_param ) );
          short_sight.ball_max_observation_length_ = 30.0;
          QVERIFY( ! PlayerSight::perceiveBall( Style::Server, me, ball, short_sight, M_param ) );
          ball.x_ = -2.0f;
          QVERIFY( PlayerSight::perceiveBall( Style::Server, me, ball, type, M_param ) );
          QVERIFY( ! PlayerSight::perceiveBall( Style::View, me, ball, type, M_param ) );
          ball.x_ = -4.0f;
          QVERIFY( ! PlayerSight::perceiveBall( Style::Server, me, ball, type, M_param ) );
          QVERIFY( PlayerSight::perceiveBall( Style::All, me, ball, type, M_param ) );

          // without a view width, the server's visible_angle (90)
          rcss::rcg::PlayerT no_view = me;
          no_view.view_width_ = rcss::rcg::SHOWINFO_SCALE2F;
          QVERIFY( PlayerSight::perceivePlayer( Style::Server, no_view, makePlayer( 'r', 2, 20, 18 ), type, M_param ).seen );
          QVERIFY( ! PlayerSight::perceivePlayer( Style::Server, no_view, makePlayer( 'r', 2, 18, 20 ), type, M_param ).seen );
      }

    //! the perceptions of a show: its player fully, the others by the sight; its own hidden at the eyes
    void perceptions()
      {
          rcss::rcg::ShowInfoT show;
          show.player_[0] = makePlayer( 'l', 1, 0, 0 );
          show.player_[1] = makePlayer( 'l', 2, 30, 0 );
          show.player_[11] = makePlayer( 'r', 1, -30, 0 );
          show.ball_.x_ = 10.0f;
          const rcss::rcg::PlayerTypeT type = fadingType();

          Perceptions p = Perceptions::perceive( Style::Server, show, 0, type, M_param, false );
          QVERIFY( p.active );
          QCOMPARE( p.hidden, -1 );
          QVERIFY( same( p.of( 0 ), PlayerSight::FULL ) );
          QVERIFY( same( p.of( 1 ), Perception{ true, 1.0f, 0.5f, true } ) );
          QVERIFY( ! p.of( 11 ).seen );
          QVERIFY( p.ball );

          // its own fully, whichever way it faces (it is not "in its view")
          show.player_[0].body_ = 180.0f;
          QVERIFY( same( Perceptions::perceive( Style::Server, show, 0, type, M_param, false ).of( 0 ), PlayerSight::FULL ) );
          show.player_[0].body_ = 0.0f;

          // at the eyes: its own not drawn, still known for the overlays
          p = Perceptions::perceive( Style::Server, show, 0, type, M_param, true );
          QCOMPARE( p.hidden, 0 );
          QVERIFY( ! p.of( 0 ).seen );
          QVERIFY( p.players[0].facing );

          // everything: not active, but its own still hidden at the eyes
          p = Perceptions::perceive( Style::All, show, 0, type, M_param, true );
          QVERIFY( ! p.active );
          QVERIFY( same( p.of( 11 ), PlayerSight::FULL ) );
          QVERIFY( ! p.of( 0 ).seen );
          QVERIFY( p.ball );
      }

    //! the team colors fade to the gray
    void fade()
      {
          QCOMPARE( fadeTeam( QColor( 255, 0, 0 ), 1.0f ), QColor( 255, 0, 0 ) );
          QCOMPARE( fadeTeam( QColor( 255, 0, 0 ), 0.0f ), QColor( 0x9a, 0x9a, 0x9a ) );
          const QColor half = fadeTeam( QColor( 255, 0, 0 ), 0.5f );
          QVERIFY( std::abs( half.red() - ( 255 + 0x9a ) / 2 ) <= 1 );
          QVERIFY( std::abs( half.green() - 0x9a / 2 ) <= 1 );
      }
};

QTEST_MAIN( TestSight )
#include "test_sight.moc"
