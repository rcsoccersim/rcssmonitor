// -*-c++-*-

/*!
  \file test_frame_builder_3d.cpp
  \brief unit test of FrameBuilder3D
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

#include "frame_builder_3d.h"
#include "scenery_3d.h"

#include "camera_mode.h"
#include "disp_holder.h"
#include "options.h"

#include <cmath>

namespace {

//! the frames of the log
constexpr int FRAMES = 120;
//! the kick of player 0: its bit at this frame
constexpr int KICK = 40;
//! the tackle of player 12 (the right team's 2): its bit over these frames
constexpr int TACKLE = 60;
constexpr int TACKLE_CYCLES = 10;

/*!
  \brief a show of a log: the 22 players, each running along x at its own
  speed (0 to 1.05 m a cycle, some backward) and turning a little, player
  0 kicking at KICK, player 12 tackling from TACKLE
*/
rcss::rcg::ShowInfoT
show( const int frame )
{
    rcss::rcg::ShowInfoT s;
    s.time_ = frame;
    s.ball_.x_ = 0.0f;
    s.ball_.y_ = 0.3f;
    if ( frame >= KICK )
    {
        // kicked away at 2 m a cycle, slowing down
        const int c = frame - KICK + 1;
        s.ball_.x_ = 0.6f + 2.0f * ( 1.0f - std::pow( 0.94f, float( c ) ) ) / 0.06f;
    }
    for ( int i = 0; i < rcss::rcg::MAX_PLAYER * 2; ++i )
    {
        rcss::rcg::PlayerT & p = s.player_[i];
        const bool left = ( i < rcss::rcg::MAX_PLAYER );
        p.side_ = ( left ? 'l' : 'r' );
        p.unum_ = static_cast< rcss::rcg::Int16 >( i % rcss::rcg::MAX_PLAYER + 1 );
        p.state_ = rcss::rcg::STAND;
        p.type_ = 0;
        const float speed = 0.05f * float( i ); // m a cycle
        const float way = ( i % 3 == 2 ? -1.0f : 1.0f ); // some backward
        p.x_ = -40.0f + 4.0f * float( i % 11 ) + way * speed * float( frame );
        p.y_ = ( left ? -20.0f : 20.0f ) + 0.02f * float( frame % 30 );
        p.body_ = ( left ? 0.0f : 180.0f ) + 10.0f * std::sin( 0.1f * float( frame ) );
        if ( ! left ) p.x_ = -p.x_;
        p.neck_ = 30.0f * std::sin( 0.2f * float( frame + i ) );
        p.stamina_ = 8000.0f;
        p.effort_ = 1.0f;
        p.recovery_ = 1.0f;
    }
    // player 0 standing at the ball for its kick
    rcss::rcg::PlayerT & kicker = s.player_[0];
    kicker.x_ = 0.0f;
    kicker.y_ = 0.0f;
    kicker.body_ = 0.0f;
    if ( frame == KICK ) kicker.state_ |= rcss::rcg::KICK;
    rcss::rcg::PlayerT & tackler = s.player_[12];
    if ( frame >= TACKLE && frame < TACKLE + TACKLE_CYCLES ) tackler.state_ |= rcss::rcg::TACKLE;
    return s;
}

//! the frames [0, count) of the log into a holder
void
fill( DispHolder & holder,
      const int from,
      const int count )
{
    for ( int f = from; f < from + count; ++f ) holder.handleShow( show( f ) );
}

//! at frame index, a fraction alpha to the next one
Interpolate::Position
at( const DispHolder & holder,
    const std::size_t index,
    const double alpha = 0.0 )
{
    Interpolate::Position p;
    p.index = index;
    p.next = std::min( index + 1, holder.dispCont().size() - 1 );
    p.alpha = ( p.next != p.index ? alpha : 0.0 );
    return p;
}

}

/*!
  \brief unit test of FrameBuilder3D, as the 3D view builds the frames of
  a log, with its inspection state (View3DState)
*/
class TestFrameBuilder3D
    : public QObject {
    Q_OBJECT

    DispHolder M_holder;

    View3DState build( FrameBuilder3D & builder,
                       const DispHolder & holder,
                       const Interpolate::Position & p,
                       const CameraMode camera = CameraMode::Broadcast )
      {
          SceneContent content;
          builder.build( holder, Options::instance(), p, camera, content );
          return builder.state( content, CameraPose() );
      }

private slots:

    void initTestCase()
      {
          fill( M_holder, 0, FRAMES );
          QCOMPARE( M_holder.dispCont().size(), std::size_t( FRAMES ) );
      }

    void init()
      {
          Options & opt = Options::instance();
          opt.setPlayerModel3D( PlayerModel::Mannequin );
          opt.setFigureScale3D( Options::FIGURE_SCALE_3D );
          opt.unselectPlayer();
      }

    void cleanup()
      {
          Options::instance().setPlayerModel3D( Options::PLAYER_MODEL_3D );
      }

    /*!
      the players drawn: as figures by the option (cylinders from the top
      camera and with the simple model), with the frames, the counts and
      a probe of each figure
    */
    void counts()
      {
          Options & opt = Options::instance();
          FrameBuilder3D builder;
          View3DState s = build( builder, M_holder, at( M_holder, 10, 0.5 ) );
          QVERIFY( s.drawn );
          QVERIFY( s.figures_ready );
          QCOMPARE( s.at.index, std::size_t( 10 ) );
          QCOMPARE( s.at.next, std::size_t( 11 ) );
          QCOMPARE( s.at.alpha, 0.5 );
          QCOMPARE( s.players, 22 );
          QCOMPARE( s.figures, 22 );
          QCOMPARE( s.cylinders, 0 );
          QCOMPARE( s.model, PlayerModel::Mannequin );
          QCOMPARE( int( s.probes.size() ), 22 );
          QCOMPARE( s.figure_joints.size(), std::size_t( 22 * builder.figures().model().jointCount() * 16 ) );
          QCOMPARE( builder.time(), 10.5 * 0.1 );
          // the show half way between the frames
          QCOMPARE( builder.show().player_[5].x_, ( show( 10 ).player_[5].x_ + show( 11 ).player_[5].x_ ) * 0.5f );
          // the probes: the left knee near the player, up from the ground
          for ( const FigureProbe & p : s.probes )
          {
              const rcss::rcg::PlayerT & player = builder.show().player_[p.player];
              QVERIFY( std::hypot( p.left_knee.x() - player.x_, p.left_knee.z() - player.y_ ) < 0.5f );
              QVERIFY( p.left_knee.y() > 0.2f && p.left_knee.y() < 0.8f );
          }

          // from the top camera: cylinders
          s = build( builder, M_holder, at( M_holder, 10 ), CameraMode::Top );
          QCOMPARE( s.figures, 0 );
          QCOMPARE( s.cylinders, 22 );
          QCOMPARE( s.model, PlayerModel::Simple );
          QVERIFY( s.probes.empty() );
          QVERIFY( s.figure_joints.empty() );

          // the simple model: cylinders
          opt.setPlayerModel3D( PlayerModel::Simple );
          s = build( builder, M_holder, at( M_holder, 10 ) );
          QCOMPARE( s.figures, 0 );
          QCOMPARE( s.cylinders, 22 );
          QCOMPARE( s.model, PlayerModel::Simple );

          // no players shown
          opt.setPlayerModel3D( PlayerModel::Mannequin );
          opt.toggleShowPlayer();
          s = build( builder, M_holder, at( M_holder, 10 ) );
          opt.toggleShowPlayer();
          QCOMPARE( s.players, 0 );
          QCOMPARE( s.figures, 0 );

          // no frames: nothing but the goals
          DispHolder empty;
          FrameBuilder3D other;
          SceneContent content;
          other.build( empty, opt, Interpolate::Position(), CameraMode::Broadcast, content );
          QVERIFY( ! other.hasShow() );
          QVERIFY( content.picks.empty() );
          QVERIFY( ! content.cylinders.empty() ); // the goal posts
      }

    /*!
      the sun's shadows: detailed (the shadow map) or simple (an ellipse
      under each player drawn) by the option, neither from the top camera
      nor when off; reset with the content
    */
    void shadows()
      {
          Options & opt = Options::instance();
          FrameBuilder3D builder;
          opt.setShadows3D( ShadowMode::Detailed );
          View3DState s = build( builder, M_holder, at( M_holder, 10 ) );
          QVERIFY( s.shadows );
          QCOMPARE( s.blob_shadows, 0 );
          QVERIFY( build( builder, M_holder, at( M_holder, 10 ), CameraMode::Player ).shadows );
          QVERIFY( ! build( builder, M_holder, at( M_holder, 10 ), CameraMode::Top ).shadows );
          QVERIFY( build( builder, M_holder, at( M_holder, 10 ), CameraMode::Free ).shadows );

          opt.setShadows3D( ShadowMode::Simple );
          s = build( builder, M_holder, at( M_holder, 10 ) );
          QVERIFY( ! s.shadows );
          QCOMPARE( s.blob_shadows, s.players ); // one each, none of the goal frames
          QCOMPARE( s.blob_shadows, 22 );
          s = build( builder, M_holder, at( M_holder, 10 ), CameraMode::Top );
          QCOMPARE( s.blob_shadows, 0 );
          opt.toggleShowPlayer();
          s = build( builder, M_holder, at( M_holder, 10 ) );
          opt.toggleShowPlayer();
          QCOMPARE( s.blob_shadows, 0 ); // no players drawn

          opt.setShadows3D( ShadowMode::Off );
          s = build( builder, M_holder, at( M_holder, 10 ) );
          QVERIFY( ! s.shadows );
          QCOMPARE( s.blob_shadows, 0 );
          opt.setShadows3D( Options::SHADOWS_3D );

          SceneContent content;
          content.shadows = true;
          content.blob_shadows.push_back( Instance3D() );
          content.clear();
          QVERIFY( ! content.shadows );
          QVERIFY( content.blob_shadows.empty() );
      }

    /*!
      the sky by the option, from every camera; the ground beyond in the
      shade of the field color
    */
    void scenery()
      {
          Options & opt = Options::instance();
          FrameBuilder3D builder;
          QVERIFY( opt.sky3D() ); // on by default
          QVERIFY( build( builder, M_holder, at( M_holder, 10 ) ).sky );
          QVERIFY( build( builder, M_holder, at( M_holder, 10 ), CameraMode::Top ).sky );
          opt.setSky3D( false );
          QVERIFY( ! build( builder, M_holder, at( M_holder, 10 ) ).sky );
          opt.setSky3D( Options::SKY_3D );

          SceneContent content;
          builder.build( M_holder, opt, at( M_holder, 10 ), CameraMode::Broadcast, content );
          QCOMPARE( content.far_ground, Scenery3D::farGroundColor( opt.fieldBrush().color() ) );
          content.clear();
          QVERIFY( ! content.sky );
      }

    /*!
      the human bodies: of the option, men and women mixed (both bodies
      drawn) or one of them; the same poses however reached
    */
    void humanBodies()
      {
          Options & opt = Options::instance();
          opt.setPlayerModel3D( PlayerModel::Human );
          FrameBuilder3D builder;
          for ( const FigureBodies bodies : { FigureBodies::Mixed, FigureBodies::Male, FigureBodies::Female } )
          {
              opt.setFigureBodies3D( bodies );
              const View3DState s = build( builder, M_holder, at( M_holder, 30, 0.5 ) );
              QCOMPARE( s.model, PlayerModel::Human );
              QCOMPARE( s.figures, 22 );
              QCOMPARE( int( s.figure_bodies.size() ), 22 );
              int women = 0;
              for ( const int b : s.figure_bodies ) if ( b == FigureLooks::FEMALE ) ++women;
              if ( bodies == FigureBodies::Mixed ) QVERIFY( women > 0 && women < 22 );
              if ( bodies == FigureBodies::Male ) QCOMPARE( women, 0 );
              if ( bodies == FigureBodies::Female ) QCOMPARE( women, 22 );
              // the knees where the players are
              for ( const FigureProbe & p : s.probes )
              {
                  QVERIFY( p.left_knee.y() > 0.2f && p.left_knee.y() < 0.8f );
              }
          }
          opt.setFigureBodies3D( FigureBodies::Mixed );
          const View3DState first = build( builder, M_holder, at( M_holder, 38, 0.25 ) );
          build( builder, M_holder, at( M_holder, 100 ) );
          QCOMPARE( build( builder, M_holder, at( M_holder, 38, 0.25 ) ).figure_joints, first.figure_joints );
          opt.setPlayerModel3D( PlayerModel::Mannequin );
      }

    /*!
      the actions counted: the kick around its moment, the tackle while its
      bit lasts, none long after
    */
    void actions()
      {
          FrameBuilder3D builder;
          View3DState s = build( builder, M_holder, at( M_holder, KICK - 1 ) );
          QCOMPARE( s.kicks, 1 );
          QCOMPARE( s.tackles, 0 );
          s = build( builder, M_holder, at( M_holder, TACKLE + 3 ) );
          QCOMPARE( s.kicks, 0 );
          QCOMPARE( s.tackles, 1 );
          s = build( builder, M_holder, at( M_holder, 20 ) );
          QCOMPARE( s.kicks + s.tackles + s.fouled + s.catches, 0 );
      }

    /*!
      the same pose for a frame however it is reached (docs/PLAN_3D-VIEW.md
      §5.2, Phase 7): drawn again and again, stepped forward and back,
      jumped far and back, by another builder that did not see the frames
      between, and in a live game before and after more frames came
    */
    void samePose()
      {
          const Interpolate::Position frame = at( M_holder, 38, 0.25 ); // around the kick
          FrameBuilder3D builder;
          const View3DState first = build( builder, M_holder, frame );
          QVERIFY( ! first.figure_joints.empty() );
          QCOMPARE( first.kicks, 1 );

          // again and again: the neck and the legs do not change
          for ( int k = 0; k < 3; ++k )
          {
              QCOMPARE( build( builder, M_holder, frame ).figure_joints, first.figure_joints );
          }
          // stepped forward and back
          for ( std::size_t i = 39; i <= 44; ++i ) build( builder, M_holder, at( M_holder, i, 0.5 ) );
          for ( std::size_t i = 44; i >= 39; --i ) build( builder, M_holder, at( M_holder, i ) );
          View3DState back = build( builder, M_holder, frame );
          QCOMPARE( back.figure_joints, first.figure_joints );
          // jumped far and back
          build( builder, M_holder, at( M_holder, 110 ) );
          build( builder, M_holder, at( M_holder, 2 ) );
          back = build( builder, M_holder, frame );
          QCOMPARE( back.figure_joints, first.figure_joints );
          QCOMPARE( back.probes.size(), first.probes.size() );
          for ( std::size_t k = 0; k < back.probes.size(); ++k )
          {
              QCOMPARE( back.probes[k].left_knee, first.probes[k].left_knee );
              QCOMPARE( back.probes[k].head, first.probes[k].head );
          }
          // another builder, straight to the frame
          FrameBuilder3D other;
          QCOMPARE( build( other, M_holder, frame ).figure_joints, first.figure_joints );

          // live: the frames up to a few after it, then more
          DispHolder live;
          fill( live, 0, 42 );
          FrameBuilder3D live_builder;
          const View3DState before = build( live_builder, live, at( live, 38, 0.25 ) );
          fill( live, 42, 30 );
          const View3DState after = build( live_builder, live, at( live, 38, 0.25 ) );
          QCOMPARE( before.figure_joints, first.figure_joints );
          QCOMPARE( after.figure_joints, first.figure_joints );
          // the frames that came later, as in the whole log
          QCOMPARE( build( live_builder, live, at( live, 60, 0.5 ) ).figure_joints,
                    build( builder, M_holder, at( M_holder, 60, 0.5 ) ).figure_joints );

          // and other frames have other poses (the figures move)
          QVERIFY( build( builder, M_holder, at( M_holder, 30 ) ).figure_joints != first.figure_joints );
          QVERIFY( build( builder, M_holder, at( M_holder, 38, 0.5 ) ).figure_joints != first.figure_joints );
      }

    /*!
      what the cameras follow: the ball, and the selected player as tall
      as it is drawn
    */
    void subject()
      {
          Options & opt = Options::instance();
          FrameBuilder3D builder;
          SceneContent content;
          builder.build( M_holder, opt, at( M_holder, KICK + 5 ), CameraMode::Broadcast, content );
          QCOMPARE( builder.subject().ball_x, builder.show().ball_.x_ );
          QVERIFY( builder.subject().ball_x > 5.0f );
          QVERIFY( ! builder.subject().has_player );
          QVERIFY( ! builder.seen() );

          opt.setSelectedNumber( rcss::rcg::RIGHT, 3 ); // index 13
          opt.setFigureScale3D( 1.2 );
          builder.build( M_holder, opt, at( M_holder, 20 ), CameraMode::Player, content );
          QVERIFY( builder.subject().has_player );
          QCOMPARE( builder.subject().player, &builder.show().player_[13] );
          QCOMPARE( builder.subject().player_x, builder.show().player_[13].x_ );
          QVERIFY( std::fabs( builder.subject().player_scale - 1.2f ) < 1.0e-5f );
          // the player camera draws what its player perceives
          QVERIFY( builder.seen() );
          QVERIFY( builder.seen()->active );
          // as cylinders: of the default scale
          opt.setPlayerModel3D( PlayerModel::Simple );
          builder.build( M_holder, opt, at( M_holder, 20 ), CameraMode::Player, content );
          QVERIFY( std::fabs( builder.subject().player_scale - 1.0f ) < 1.0e-5f );
          opt.unselectPlayer();
      }
};

QTEST_MAIN( TestFrameBuilder3D )
#include "test_frame_builder_3d.moc"
