// -*-c++-*-

/*!
  \file test_scene_builder.cpp
  \brief unit test of SceneBuilder, what the 3D view draws and where
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

#include "disp_holder.h"
#include "options.h"
#include "scene_builder.h"

#include <rcss/rcg/types.h>

namespace {

//! all players standing in two lines, the ball at the center spot
rcss::rcg::ShowInfoT
makeShow()
{
    rcss::rcg::ShowInfoT show;
    for ( int i = 0; i < rcss::rcg::MAX_PLAYER * 2; ++i )
    {
        rcss::rcg::PlayerT & p = show.player_[i];
        const bool left = ( i < rcss::rcg::MAX_PLAYER );
        p.side_ = ( left ? 'l' : 'r' );
        p.unum_ = static_cast< rcss::rcg::Int16 >( i % rcss::rcg::MAX_PLAYER + 1 );
        p.state_ = rcss::rcg::STAND;
        p.type_ = 0;
        p.x_ = ( left ? -10.0f : 10.0f );
        p.y_ = -30.0f + 5.0f * ( i % rcss::rcg::MAX_PLAYER );
    }
    return show;
}

//! where the model matrix puts the point p of the shape
QVector3D
at( const Instance3D & i,
    const QVector3D & p = QVector3D() )
{
    return i.model.map( p );
}

bool
near( const QVector3D & a,
      const QVector3D & b )
{
    return ( a - b ).length() < 1.0e-4f;
}

}

class TestSceneBuilder
    : public QObject {
    Q_OBJECT

private:
    DispHolder M_holder;

private slots:

    void init()
      {
          Options::instance().unselectPlayer();
      }

    void goals()
      {
          SceneContent c;
          SceneBuilder::addGoals( c );
          // 2 posts and a crossbar, and the back, 2 sides, the roof and the floor of the net
          QCOMPARE( c.cylinders.size(), size_t( 6 ) );
          QCOMPARE( c.quads.size(), size_t( 10 ) );

          // the posts stand on the goal lines, up to the crossbar
          const float x = float( Options::PITCH_HALF_LENGTH );
          const float z = float( Options::GOAL_HALF_WIDTH );
          QVERIFY( near( at( c.cylinders[0], QVector3D( 0, -0.5f, 0 ) ), QVector3D( -x, 0, -z ) ) );
          QVERIFY( near( at( c.cylinders[1], QVector3D( 0, 0.5f, 0 ) ), QVector3D( -x, SceneBuilder::GOAL_HEIGHT, z ) ) );
          // the crossbar from post to post
          QVERIFY( near( at( c.cylinders[2], QVector3D( 0, 0.5f, 0 ) ), QVector3D( -x, SceneBuilder::GOAL_HEIGHT, z ) ) );
          QVERIFY( near( at( c.cylinders[2], QVector3D( 0, -0.5f, 0 ) ), QVector3D( -x, SceneBuilder::GOAL_HEIGHT, -z ) ) );
          QVERIFY( near( at( c.cylinders[3] ), QVector3D( x, SceneBuilder::GOAL_HEIGHT / 2, -z ) ) );
          // the back of the net behind the goal line
          QVERIFY( near( at( c.quads[0] ), QVector3D( -x - float( Options::GOAL_DEPTH ), SceneBuilder::GOAL_HEIGHT / 2, 0 ) ) );
          QCOMPARE( c.quads[0].color.w(), SceneBuilder::NET_OPACITY );
      }

    void players()
      {
          SceneContent c;
          rcss::rcg::ShowInfoT show = makeShow();
          show.player_[3].side_ = 'n'; // not in the show
          SceneBuilder::addShow( c, show, M_holder, Options::instance() );

          const size_t n = rcss::rcg::MAX_PLAYER * 2 - 1;
          QCOMPARE( c.cylinders.size(), n );
          QCOMPARE( c.spheres.size(), n + 1 ); // the heads and the ball
          QCOMPARE( c.cones.size(), n * 2 );
          QCOMPARE( c.discs.size(), n + 1 ); // the circles and the ball's shadow
          QCOMPARE( c.rings.size(), size_t( 0 ) );
          QCOMPARE( c.decay_rings.size(), size_t( 0 ) );

          // the body on the ground, of the player type's size
          const rcss::rcg::PlayerTypeT & type = M_holder.playerType( 0 );
          const rcss::rcg::ServerParamT & sp = M_holder.serverParam();
          const Instance3D & body = c.cylinders[0];
          QVERIFY( near( at( body, QVector3D( 0, -0.5f, 0 ) ), QVector3D( -10, 0, -30 ) ) );
          QVERIFY( near( at( body, QVector3D( 1, 0.5f, 0 ) ), QVector3D( -10 + float( type.player_size_ ), SceneBuilder::BODY_HEIGHT, -30 ) ) );
          // the head on it
          QVERIFY( near( at( c.spheres[0] ), QVector3D( -10, SceneBuilder::HEAD_HEIGHT, -30 ) ) );
          // the circle of the kickable area, in the team color
          const float kickable = float( type.player_size_ + type.kickable_margin_ + sp.ball_size_ );
          QVERIFY( near( at( c.discs[0], QVector3D( 1, 0, 0 ) ), QVector3D( -10 + kickable, SceneBuilder::DISC_Y, -30 ) ) );
          const QColor team = Options::instance().leftTeamBrush().color();
          QCOMPARE( c.discs[0].color, QVector4D( team.redF(), team.greenF(), team.blueF(), SceneBuilder::DISC_OPACITY ) );
          QCOMPARE( c.cylinders[0].color.toVector3D(), QVector3D( team.redF(), team.greenF(), team.blueF() ) );
      }

    void playerSizeOption()
      {
          Options & opt = Options::instance();
          const double old = opt.playerSize();
          opt.setPlayerSize( 2.0 );

          SceneContent c;
          SceneBuilder::addShow( c, makeShow(), M_holder, opt );
          opt.setPlayerSize( old );

          QVERIFY( near( at( c.discs[0], QVector3D( 1, 0, 0 ) ), QVector3D( -8, SceneBuilder::DISC_Y, -30 ) ) );
      }

    void directions()
      {
          rcss::rcg::ShowInfoT show = makeShow();
          rcss::rcg::PlayerT & p = show.player_[0];
          p.body_ = 90.0f; // toward the field's +y, the world's +z
          p.neck_ = -90.0f; // the face toward +x

          SceneContent c;
          SceneBuilder::addShow( c, show, M_holder, Options::instance() );

          const Instance3D & nose = c.cones[0];
          const QVector3D nose_dir = ( at( nose, QVector3D( 1, 0, 0 ) ) - at( nose ) ).normalized();
          QVERIFY( near( nose_dir, QVector3D( 0, 0, 1 ) ) );
          QVERIFY( at( nose ).z() > -30.0f ); // in front of the body
          QCOMPARE( nose.color, QVector4D( 0, 0, 0, 1 ) );

          const Instance3D & face = c.cones[1];
          const QVector3D face_dir = ( at( face, QVector3D( 1, 0, 0 ) ) - at( face ) ).normalized();
          QVERIFY( near( face_dir, QVector3D( 1, 0, 0 ) ) );
          QCOMPARE( face.color, QVector4D( 1, 0, 0, 1 ) );
      }

    void highlightAndDecay()
      {
          const Options & opt = Options::instance();
          rcss::rcg::ShowInfoT show = makeShow();
          show.player_[0].state_ |= rcss::rcg::KICK;
          rcss::rcg::PlayerT & tired = show.player_[1];
          tired.stamina_ = 1000.0f;
          tired.effort_ = 0.5f;
          tired.recovery_ = 1.0f;

          SceneContent c;
          SceneBuilder::addShow( c, show, M_holder, opt );

          QCOMPARE( c.rings.size(), size_t( 1 ) );
          const QColor kick = opt.kickPen().color();
          QCOMPARE( c.rings[0].color, QVector4D( kick.redF(), kick.greenF(), kick.blueF(), 1.0f ) );
          QVERIFY( near( at( c.rings[0] ), QVector3D( -10, SceneBuilder::RING_Y, -30 ) ) );

          QCOMPARE( c.decay_rings.size(), size_t( 1 ) );
          QVERIFY( near( at( c.decay_rings[0] ), QVector3D( -10, SceneBuilder::RING_Y, -25 ) ) );
          // and the body darker
          QVERIFY( c.cylinders[1].color.toVector3D().length() < c.cylinders[2].color.toVector3D().length() );
      }

    void ball()
      {
          const Options & opt = Options::instance();
          const double r = SceneBuilder::ballRadius( opt, M_holder.serverParam().ball_size_ );
          rcss::rcg::ShowInfoT show = makeShow();
          show.ball_.x_ = 3.0f;
          show.ball_.y_ = -4.0f;

          SceneContent c;
          SceneBuilder::addShow( c, show, M_holder, opt );
          const Instance3D & ball = c.spheres.back();
          const Instance3D & shadow = c.discs.back();
          // on the ground
          QVERIFY( near( at( ball, QVector3D( 0, -1, 0 ) ), QVector3D( 3, 0, -4 ) ) );
          QVERIFY( near( at( shadow ), QVector3D( 3, SceneBuilder::BALL_SHADOW_Y, -4 ) ) );
          QCOMPARE( shadow.color.w(), 0.45f );

          // in the air (z, which the server of the 2.5D simulation may send)
          show.ball_.z_ = 2.0f;
          c.clear();
          SceneBuilder::addShow( c, show, M_holder, opt );
          QVERIFY( near( at( c.spheres.back() ), QVector3D( 3, 2.0f + float( r ), -4 ) ) );
          QVERIFY( near( at( c.discs.back() ), QVector3D( 3, SceneBuilder::BALL_SHADOW_Y, -4 ) ) );
          QCOMPARE( c.discs.back().color.w(), 0.3f );
      }

    void hidden()
      {
          Options & opt = Options::instance();
          SceneContent c;

          opt.toggleShowPlayer();
          SceneBuilder::addShow( c, makeShow(), M_holder, opt );
          opt.toggleShowPlayer();
          QCOMPARE( c.cylinders.size(), size_t( 0 ) );
          QCOMPARE( c.spheres.size(), size_t( 1 ) );

          c.clear();
          opt.toggleShowBall();
          SceneBuilder::addShow( c, makeShow(), M_holder, opt );
          opt.toggleShowBall();
          QCOMPARE( c.spheres.size(), size_t( rcss::rcg::MAX_PLAYER * 2 ) );
          QCOMPARE( c.discs.size(), size_t( rcss::rcg::MAX_PLAYER * 2 ) );
      }
};

QTEST_MAIN( TestSceneBuilder )
#include "test_scene_builder.moc"
