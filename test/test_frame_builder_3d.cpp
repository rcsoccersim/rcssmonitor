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

#include "ball_aids_3d.h"
#include "frame_builder_3d.h"
#include "scene_builder.h"
#include "scenery_3d.h"

#include "camera_mode.h"
#include "disp_holder.h"
#include "options.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

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
          opt.setPlayerModel3D( QString( PLAYER_MODEL_MANNEQUIN ) );
          opt.setFigureScale3D( Options::FIGURE_SCALE_3D );
          opt.unselectPlayer();
      }

    void cleanup()
      {
          Options::instance().setPlayerModel3D( Options::PLAYER_MODEL_3D );
          Options::instance().setFigureFolder3D( QString() );
      }

    /*!
      the players as the figures of a pack of the user's, in the folder of
      the option; a pack of no name, or one that cannot be drawn, as
      cylinders, told (FigurePacks::drawError()), tried again after the
      packs are looked for again; the bundled ones again
    */
    void packs()
      {
          Options & opt = Options::instance();
          QTemporaryDir folder;
          QDir dir( folder.path() );
          QVERIFY( dir.mkpath( "robot" ) && dir.mkpath( "broken" ) );
          QVERIFY( QFile::copy( ":/figures/mannequin/Mannequin.glb", dir.filePath( "robot/Robot.glb" ) ) );
          const QByteArray robot = "{ \"format\": 1, \"motion\": \"mannequin\",\n"
              "  \"bodies\": [ { \"model\": \"Robot.glb\", \"paint\": { \"M_Main\": \"team\" } } ] }\n";
          const auto write = [&dir]( const char * path, const QByteArray & data ) {
              QFile f( dir.filePath( path ) );
              return f.open( QIODevice::WriteOnly ) && f.write( data ) == data.size();
          };
          QVERIFY( write( "robot/figure.json", robot ) );
          // read, but its model is not one
          QVERIFY( write( "broken/figure.json", robot ) );
          QVERIFY( write( "broken/Robot.glb", "not a model" ) );
          opt.setFigureFolder3D( folder.path() );

          FrameBuilder3D builder;
          opt.setPlayerModel3D( "robot" );
          View3DState s = build( builder, M_holder, at( M_holder, 10 ) );
          QCOMPARE( s.model, QString( "robot" ) );
          QCOMPARE( s.figures, 22 );
          QCOMPARE( builder.figures().pack().dir, dir.filePath( "robot" ) );
          QVERIFY( FigurePacks::drawError( "robot" ).isEmpty() );

          // no such pack: cylinders, told
          opt.setPlayerModel3D( "nobody" );
          s = build( builder, M_holder, at( M_holder, 10 ) );
          QCOMPARE( s.model, QString( PLAYER_MODEL_SIMPLE ) );
          QCOMPARE( s.figures, 0 );
          QCOMPARE( s.cylinders, 22 );
          QCOMPARE( FigurePacks::drawError( "nobody" ), QString( "no figure pack \"nobody\"" ) );

          // a model that cannot be read: cylinders, told; tried once only
          opt.setPlayerModel3D( "broken" );
          s = build( builder, M_holder, at( M_holder, 10 ) );
          QCOMPARE( s.figures, 0 );
          QVERIFY2( FigurePacks::drawError( "broken" ).startsWith( "broken: Robot.glb: " ),
                    qPrintable( FigurePacks::drawError( "broken" ) ) );
          FigurePacks::noteError( "broken", "not tried again" );
          build( builder, M_holder, at( M_holder, 11 ) );
          QCOMPARE( FigurePacks::drawError( "broken" ), QString( "not tried again" ) );

          // mended, and the packs looked for again: drawn
          QVERIFY( QFile::remove( dir.filePath( "broken/Robot.glb" ) ) );
          QVERIFY( QFile::copy( ":/figures/mannequin/Mannequin.glb", dir.filePath( "broken/Robot.glb" ) ) );
          FigurePacks::refresh();
          s = build( builder, M_holder, at( M_holder, 10 ) );
          QCOMPARE( s.model, QString( "broken" ) );
          QCOMPARE( s.figures, 22 );
          QVERIFY( FigurePacks::drawError( "broken" ).isEmpty() );

          // the bundled ones again
          opt.setPlayerModel3D( QString( PLAYER_MODEL_HUMAN ) );
          s = build( builder, M_holder, at( M_holder, 10 ) );
          QCOMPARE( s.model, QString( PLAYER_MODEL_HUMAN ) );
          QCOMPARE( s.figures, 22 );
          opt.setFigureFolder3D( QString() );
          FigurePacks::refresh();
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
          QCOMPARE( s.model, QString( PLAYER_MODEL_MANNEQUIN ) );
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
          QCOMPARE( s.model, QString( PLAYER_MODEL_SIMPLE ) );
          QVERIFY( s.probes.empty() );
          QVERIFY( s.figure_joints.empty() );

          // the simple model: cylinders
          opt.setPlayerModel3D( QString( PLAYER_MODEL_SIMPLE ) );
          s = build( builder, M_holder, at( M_holder, 10 ) );
          QCOMPARE( s.figures, 0 );
          QCOMPARE( s.cylinders, 22 );
          QCOMPARE( s.model, QString( PLAYER_MODEL_SIMPLE ) );

          // no players shown
          opt.setPlayerModel3D( QString( PLAYER_MODEL_MANNEQUIN ) );
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
          QVERIFY( ! content.realistic_grass ); // stripes by default
          opt.setGrass3D( GrassMode::Realistic );
          QVERIFY( build( builder, M_holder, at( M_holder, 10 ) ).realistic_grass );
          opt.setGrass3D( GrassMode::Plain );
          QVERIFY( ! build( builder, M_holder, at( M_holder, 10 ) ).realistic_grass );
          opt.setGrass3D( Options::GRASS_3D );
          QVERIFY( content.stands ); // on by default
          opt.setStands3D( false );
          QVERIFY( ! build( builder, M_holder, at( M_holder, 10 ) ).stands );
          opt.setStands3D( Options::STANDS_3D );
          QVERIFY( content.ball_pattern ); // on by default
          opt.setBallPattern3D( false );
          QVERIFY( ! build( builder, M_holder, at( M_holder, 10 ) ).ball_pattern );
          opt.setBallPattern3D( Options::BALL_PATTERN_3D );
          content.realistic_grass = true;
          content.clear();
          QVERIFY( ! content.stands );
          QVERIFY( ! content.ball_pattern );
          QVERIFY( ! content.sky );
          QVERIFY( ! content.realistic_grass );
      }

    /*!
      the human bodies: of the option, men and women mixed (both bodies
      drawn) or one of them; the same poses however reached
    */
    void humanBodies()
      {
          Options & opt = Options::instance();
          opt.setPlayerModel3D( QString( PLAYER_MODEL_HUMAN ) );
          FrameBuilder3D builder;
          for ( const FigureBodies bodies : { FigureBodies::Mixed, FigureBodies::Male, FigureBodies::Female } )
          {
              opt.setFigureBodies3D( bodies );
              const View3DState s = build( builder, M_holder, at( M_holder, 30, 0.5 ) );
              QCOMPARE( s.model, QString( PLAYER_MODEL_HUMAN ) );
              QCOMPARE( s.figures, 22 );
              QCOMPARE( int( s.figure_bodies.size() ), 22 );
              int women = 0;
              for ( const int b : s.figure_bodies ) if ( b == 1 ) ++women; // the Human pack's woman
              if ( bodies == FigureBodies::Mixed ) QVERIFY( women > 0 && women < 22 );
              if ( bodies == FigureBodies::Male ) QCOMPARE( women, 0 );
              if ( bodies == FigureBodies::Female ) QCOMPARE( women, 22 );
              // the knees where the players are
              for ( std::size_t k = 0; k < s.probes.size(); ++k )
              {
                  const FigureProbe & p = s.probes[k];
                  QVERIFY2( p.left_knee.y() > 0.2f && p.left_knee.y() < 0.9f, // (up to a sprint's knee)
                            qPrintable( QString( "%1: %2" ).arg( k ).arg( p.left_knee.y() ) ) );
              }
          }
          opt.setFigureBodies3D( FigureBodies::Mixed );
          const View3DState first = build( builder, M_holder, at( M_holder, 38, 0.25 ) );
          build( builder, M_holder, at( M_holder, 100 ) );
          QCOMPARE( build( builder, M_holder, at( M_holder, 38, 0.25 ) ).figure_joints, first.figure_joints );
          opt.setPlayerModel3D( QString( PLAYER_MODEL_MANNEQUIN ) );
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
      the same pose for a frame however it is reached (docs/plan/2026-11-11_3d-view.md
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
          QCOMPARE( back.ball_turn, first.ball_turn );
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
      the ball rolls from the kick on (not before: it stays), turned in its
      instance; the same turn for a frame however it is reached
    */
    void ballSpin()
      {
          FrameBuilder3D builder;
          SceneContent content;
          // still before the kick
          QCOMPARE( build( builder, M_holder, at( M_holder, 10 ) ).ball_turn, QQuaternion() );
          QCOMPARE( build( builder, M_holder, at( M_holder, 38, 0.5 ) ).ball_turn, QQuaternion() );
          // on the way after it, more and more
          const Interpolate::Position frame = at( M_holder, 50, 0.3 );
          builder.build( M_holder, Options::instance(), frame, CameraMode::Broadcast, content );
          const View3DState s = builder.state( content, CameraPose() );
          QVERIFY( s.ball_turn != QQuaternion() );
          QVERIFY( build( builder, M_holder, at( M_holder, 50, 0.6 ) ).ball_turn != s.ball_turn );
          // the instance turned so
          QCOMPARE( content.balls.size(), std::size_t( 1 ) );
          const float r = float( SceneBuilder::ballRadius( Options::instance(),
                                                           M_holder.serverParam().ball_size_ ) );
          const QVector3D center = content.balls[0].model.map( QVector3D() );
          const QVector3D up = content.balls[0].model.map( QVector3D( 0, 1, 0 ) ) - center;
          QVERIFY( ( up - s.ball_turn.rotatedVector( QVector3D( 0, 1, 0 ) ) * r ).length() < 1.0e-4f );
          // again, after others, and by another builder
          build( builder, M_holder, at( M_holder, 100 ) );
          build( builder, M_holder, at( M_holder, 3 ) );
          QCOMPARE( build( builder, M_holder, frame ).ball_turn, s.ball_turn );
          FrameBuilder3D other;
          QCOMPARE( build( other, M_holder, frame ).ball_turn, s.ball_turn );
          // live, the frames coming after
          DispHolder live;
          fill( live, 0, 52 );
          FrameBuilder3D live_builder;
          QCOMPARE( build( live_builder, live, at( live, 50, 0.3 ) ).ball_turn, s.ball_turn );
          fill( live, 52, 20 );
          QCOMPARE( build( live_builder, live, at( live, 50, 0.3 ) ).ball_turn, s.ball_turn );
      }

    /*!
      the ball flying virtually (the option ball_flight_3d): the long ball
      of the kick in the air, the ball drawn at its height; on the ground
      without the option and live; the same height however the frame is
      reached
    */
    void ballFlight()
      {
          Options & opt = Options::instance();
          QVERIFY( ! opt.ballFlight3D() ); // off by default
          FrameBuilder3D builder;
          const Interpolate::Position frame = at( M_holder, 46, 0.5 ); // the ball in the air after the kick
          View3DState s = build( builder, M_holder, frame );
          QVERIFY( ! s.ball_flight );
          QCOMPARE( s.ball_height, 0.0f );

          opt.setBallFlight3D( true );
          SceneContent content;
          builder.build( M_holder, opt, frame, CameraMode::Broadcast, content );
          s = builder.state( content, CameraPose() );
          QVERIFY( s.ball_flight );
          QVERIFY( s.ball_height > 0.5f );
          QVERIFY( builder.flight() != nullptr );
          QCOMPARE( s.ball_height, builder.flight()->heightAt( 46, 0.5 ) );
          // the ball drawn at its height
          const float r = float( SceneBuilder::ballRadius( opt, M_holder.serverParam().ball_size_ ) );
          QVERIFY( std::fabs( content.balls[0].model.map( QVector3D() ).y() - ( s.ball_height + r ) ) < 1.0e-4f );
          // still before the kick: on the ground
          QCOMPARE( build( builder, M_holder, at( M_holder, 20 ) ).ball_height, 0.0f );
          // the same however reached, and by another builder
          build( builder, M_holder, at( M_holder, 100 ) );
          QCOMPARE( build( builder, M_holder, frame ).ball_height, s.ball_height );
          FrameBuilder3D other;
          QCOMPARE( build( other, M_holder, frame ).ball_height, s.ball_height );
          // live: on the ground
          builder.setLive( true );
          s = build( builder, M_holder, frame );
          QVERIFY( ! s.ball_flight );
          QCOMPARE( s.ball_height, 0.0f );
          QVERIFY( builder.flight() == nullptr );
          // the delayed live view (§5.10): the kick ended before the frame
          // drawn, so it flies as in log replay
          builder.setLive( true, 30 );
          s = build( builder, M_holder, frame );
          QVERIFY( s.ball_flight );
          QVERIFY( s.ball_height > 0.5f );
          QVERIFY( builder.flight() != nullptr );
          builder.setLive( false );
          opt.setBallFlight3D( Options::BALL_FLIGHT_3D );
      }

    /*!
      the ring and the trail by the options, where the ball is drawn; the
      ring's line wider far away; the trail as long as the cycles, from the
      log (the same however the frame is reached); none without the ball
    */
    void ballAids()
      {
          Options & opt = Options::instance();
          QVERIFY( opt.ballRing3D() && opt.ballTrail3D() ); // on by default
          QCOMPARE( opt.ballTrailCycles3D(), 5 );
          const auto aids = [&]( FrameBuilder3D & builder, const DispHolder & holder,
                                 const Interpolate::Position & p, const CameraPose & camera ) {
              SceneContent content;
              builder.build( holder, opt, p, CameraMode::Broadcast, content );
              builder.addBallAids( holder, opt, camera, 720.0f, content );
              return std::make_pair( content, builder.state( content, camera ) );
          };
          CameraPose near_camera, far_camera;
          near_camera.position = QVector3D( 20, 5, 15 );
          near_camera.target = QVector3D( 20, 0, 0.3f );
          far_camera.position = QVector3D( 20, 60, 150 );
          far_camera.target = QVector3D( 20, 0, 0.3f );

          FrameBuilder3D builder;
          const Interpolate::Position frame = at( M_holder, 50, 0.3 ); // the ball rolling after the kick
          const auto near_aids = aids( builder, M_holder, frame, near_camera );
          const SceneContent & c = near_aids.first;
          const View3DState & s = near_aids.second;
          QVERIFY( s.ball_ring );
          QCOMPARE( c.ball_rings.size(), std::size_t( 1 ) );
          // around the ball drawn, on the ground
          const QVector3D ball = c.balls[0].model.map( QVector3D() );
          const QVector3D ring = c.ball_rings[0].model.map( QVector3D() );
          QVERIFY( std::fabs( ring.x() - ball.x() ) < 1.0e-4f && std::fabs( ring.z() - ball.z() ) < 1.0e-4f );
          QCOMPARE( c.ball_ring_radius, float( Options::BALL_RING_RADIUS_3D ) );
          QCOMPARE( s.ball_ring_width, BallAids3D::RING_WIDTH );
          // the trail: between frames, the place drawn, 5 frames and part of the way
          QCOMPARE( s.ball_trail, 7 );
          QCOMPARE( c.ball_trail.size(), std::size_t( 12 * 6 ) );
          QVERIFY( ( c.ball_trail[1].position - ball ).length() < 1.0e-4f ); // its head at the ball
          // far away: the ring's line some pixels wide
          const auto far_aids = aids( builder, M_holder, frame, far_camera );
          QVERIFY( far_aids.second.ball_ring_width > BallAids3D::RING_WIDTH );
          // the same trail however the frame is reached
          build( builder, M_holder, at( M_holder, 100 ) );
          QCOMPARE( aids( builder, M_holder, frame, near_camera ).first.ball_trail.size(), c.ball_trail.size() );
          FrameBuilder3D other;
          const SceneContent again = aids( other, M_holder, frame, near_camera ).first;
          for ( std::size_t k = 0; k < c.ball_trail.size(); ++k )
          {
              QCOMPARE( again.ball_trail[k].position, c.ball_trail[k].position );
              QCOMPARE( again.ball_trail[k].color, c.ball_trail[k].color );
          }
          // still before the kick: the ring (the trail of a still ball has no length)
          const auto still = aids( builder, M_holder, at( M_holder, 20 ), near_camera );
          QVERIFY( still.second.ball_ring );

          // the options within their ranges
          opt.setBallRingRadius3D( 10.0 );
          QCOMPARE( opt.ballRingRadius3D(), Options::MAX_BALL_RING_RADIUS_3D );
          opt.setBallRingRadius3D( 0.0 );
          QCOMPARE( opt.ballRingRadius3D(), Options::MIN_BALL_RING_RADIUS_3D );
          opt.setBallTrailCycles3D( 99 );
          QCOMPARE( opt.ballTrailCycles3D(), Options::MAX_BALL_TRAIL_CYCLES_3D );
          opt.setBallTrailCycles3D( 0 );
          QCOMPARE( opt.ballTrailCycles3D(), Options::MIN_BALL_TRAIL_CYCLES_3D );

          // by the options
          opt.setBallTrailCycles3D( 2 );
          QCOMPARE( aids( builder, M_holder, at( M_holder, 50 ), near_camera ).second.ball_trail, 3 );
          opt.setBallTrailCycles3D( Options::BALL_TRAIL_CYCLES_3D );
          opt.setBallRingRadius3D( 2.0 );
          QCOMPARE( aids( builder, M_holder, frame, near_camera ).first.ball_ring_radius, 2.0f );
          opt.setBallRingRadius3D( Options::BALL_RING_RADIUS_3D );
          opt.setBallRing3D( false );
          QVERIFY( ! aids( builder, M_holder, frame, near_camera ).second.ball_ring );
          opt.setBallRing3D( true );
          opt.setBallTrail3D( false );
          QCOMPARE( aids( builder, M_holder, frame, near_camera ).second.ball_trail, 0 );
          opt.setBallTrail3D( true );
          // without the ball drawn, neither
          opt.toggleShowBall();
          const auto none = aids( builder, M_holder, frame, near_camera );
          opt.toggleShowBall();
          QVERIFY( ! none.second.ball_ring );
          QCOMPARE( none.second.ball_trail, 0 );
          QVERIFY( none.first.ball_trail.empty() );
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
          opt.setPlayerModel3D( QString( PLAYER_MODEL_SIMPLE ) );
          builder.build( M_holder, opt, at( M_holder, 20 ), CameraMode::Player, content );
          QVERIFY( std::fabs( builder.subject().player_scale - 1.0f ) < 1.0e-5f );
          opt.unselectPlayer();
      }
};

QTEST_MAIN( TestFrameBuilder3D )
#include "test_frame_builder_3d.moc"
