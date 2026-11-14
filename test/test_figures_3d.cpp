// -*-c++-*-

/*!
  \file test_figures_3d.cpp
  \brief unit test of Figures3D (the players as human figures)
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

#include "camera_mode.h"
#include "figures_3d.h"
#include "gait_table.h"
#include "options.h"
#include "scene_builder.h"

#include <cmath>
#include <memory>

namespace {

//! where a node of a figure is in the world
QVector3D
nodeAt( const Figures3D & figures,
        const FigureInstance & figure,
        const double time,
        const QString & name )
{
    std::vector< NodeTransform > local;
    figures.localPose( figure, time, local );
    std::vector< QMatrix4x4 > global;
    figures.model().globals( local, global );
    return ( figures.modelMatrix( figure ) * global[figures.model().nodeIndex( name )] ).column( 3 ).toVector3D();
}

//! where the left foot is in the world: between the ankle and the toes, as analyzeGait() reads it
QVector3D
leftFoot( const Figures3D & figures,
          const FigureInstance & figure,
          const double time = 0.0 )
{
    return ( nodeAt( figures, figure, time, "foot_l" ) + nodeAt( figures, figure, time, "ball_l" ) ) * 0.5f;
}

//! the field angle (degree) that a pair of left and right nodes faces
float
facing( const QVector3D & left,
        const QVector3D & right )
{
    // facing a, the left is (sin a, -cos a) on the field, whose y is the world's z
    const QVector3D l = left - right;
    return std::atan2( l.x(), -l.z() ) * 180.0f / float( M_PI );
}

float
angleBetween( const float a,
              const float b )
{
    return std::remainder( a - b, 360.0f );
}

//! a figure 2.22 m high (1.2 times a person's height) unless told
FigureInstance
figureAt( const int player,
          const float x,
          const float y,
          const float body,
          const float height = 2.22f )
{
    return FigureInstance{ player, x, y, body, height, QVector4D( 1.0f, 0.0f, 0.0f, 1.0f ) };
}

}

/*!
  \brief unit test of Figures3D
*/
class TestFigures3D
    : public QObject {
    Q_OBJECT

private:
    Figures3D M_figures;

private slots:

    void initTestCase()
      {
          QVERIFY2( M_figures.load(), qPrintable( M_figures.error() ) );
          QVERIFY( M_figures.ready() );
          QVERIFY( M_figures.load() ); // once
      }

    //! the option of the player model: simple by default, read by its names
    void option()
      {
          QCOMPARE( Options::PLAYER_MODEL_3D, PlayerModel::Simple );
          PlayerModel m = PlayerModel::Simple;
          QVERIFY( playerModelFromName( "figure", &m ) );
          QCOMPARE( m, PlayerModel::Figure );
          QVERIFY( ! playerModelFromName( "robot", &m ) );
          QCOMPARE( m, PlayerModel::Figure ); // unchanged
          QCOMPARE( playerModelName( PlayerModel::Simple ), QString( "simple" ) );
          QCOMPARE( Figures3D::jerseyMaterials(), QStringList( { "M_Main" } ) );

          // the scale of the figures: life size by default, kept within its range
          Options & opt = Options::instance();
          QCOMPARE( Options::FIGURE_SCALE_3D, 1.0 );
          opt.setFigureScale3D( 3.0 );
          QCOMPARE( opt.figureScale3D(), Options::MAX_FIGURE_SCALE_3D );
          opt.setFigureScale3D( 0.1 );
          QCOMPARE( opt.figureScale3D(), Options::MIN_FIGURE_SCALE_3D );
          opt.setFigureScale3D( Options::FIGURE_SCALE_3D );
      }

    /*!
      a figure stands on the ground at its player, 1.2 times a person's
      height, facing the body direction
    */
    void placed()
      {
          const FigureInstance f = figureAt( 3, 10.0f, -5.0f, 30.0f );
          // the lowest point of the model at rest on the ground at the player
          const QVector3D foot = M_figures.modelMatrix( f ).map( QVector3D( 0.0f, M_figures.model().min_y, 0.0f ) );
          QVERIFY( ( foot - QVector3D( 10.0f, 0.0f, -5.0f ) ).length() < 1.0e-4f );
          // as high as the figure's height, each its own
          for ( const float height : { 1.85f, 2.22f, 2.5f } )
          {
              const FigureInstance g = figureAt( 3, 10.0f, -5.0f, 30.0f, height );
              const QVector3D top = M_figures.modelMatrix( g ).map( QVector3D( 0.0f, M_figures.model().max_y, 0.0f ) );
              QVERIFY( std::fabs( top.y() - height ) < 1.0e-3f );
              const QVector3D low = M_figures.modelMatrix( g ).map( QVector3D( 0.0f, M_figures.model().min_y, 0.0f ) );
              QVERIFY( std::fabs( low.y() ) < 1.0e-4f );
          }

          // standing: the head high, the feet low
          const QVector3D head = nodeAt( M_figures, f, 0.0, "Head" );
          QVERIFY2( head.y() > 1.7f && head.y() < 2.2f, qPrintable( QString::number( head.y() ) ) );
          QVERIFY( nodeAt( M_figures, f, 0.0, "foot_l" ).y() < 0.3f );

          // facing the body direction (from +x toward +y on the field, which
          // is the world's +z): the toes ahead of the ankles, the left foot
          // to the left (toward -90 degrees from the body direction)
          for ( const float body : { 0.0f, 30.0f, 135.0f, -100.0f } )
          {
              const FigureInstance g = figureAt( 3, 10.0f, -5.0f, body );
              const float rad = body * float( M_PI ) / 180.0f;
              const QVector3D ahead( std::cos( rad ), 0.0f, std::sin( rad ) );
              const QVector3D left( std::sin( rad ), 0.0f, -std::cos( rad ) );
              const QVector3D toes = nodeAt( M_figures, g, 0.0, "ball_l" ) - nodeAt( M_figures, g, 0.0, "foot_l" );
              QVERIFY2( QVector3D::dotProduct( toes, ahead ) > 0.05f, qPrintable( QString::number( body ) ) );
              const QVector3D side = nodeAt( M_figures, g, 0.0, "thigh_l" ) - nodeAt( M_figures, g, 0.0, "thigh_r" );
              QVERIFY2( QVector3D::dotProduct( side, left ) > 0.1f, qPrintable( QString::number( body ) ) );
          }
      }

    /*!
      the skin matrices: as many as the joints of each figure, putting a
      joint's own point where its node is; the same for the same time, and
      the players at their own points of the idle clip
    */
    void pose()
      {
          SceneContent content;
          content.figures.push_back( figureAt( 0, 0.0f, 0.0f, 0.0f ) );
          content.figures.push_back( figureAt( 5, 20.0f, 3.0f, 90.0f ) );
          M_figures.pose( content, 12.3 );
          const std::size_t floats = std::size_t( M_figures.model().jointCount() ) * 16;
          QCOMPARE( content.figure_joints.size(), 2 * floats );

          // the joint of the head: its skin matrix takes the joint's point
          // at bind (the inverse of its inverse bind matrix) to the head
          const GltfModel & m = M_figures.model();
          const int head = m.nodeIndex( "Head" );
          const int joint = int( std::find( m.joint_nodes.begin(), m.joint_nodes.end(), head ) - m.joint_nodes.begin() );
          for ( int k = 0; k < 2; ++k )
          {
              const QMatrix4x4 skin = QMatrix4x4( &content.figure_joints[k * floats + joint * 16] ).transposed();
              const QVector3D bind = m.inverse_bind[joint].inverted().column( 3 ).toVector3D();
              QVERIFY( ( skin.map( bind ) - nodeAt( M_figures, content.figures[k], 12.3, "Head" ) ).length() < 1.0e-3f );
          }

          // the same time, the same pose (the log decides it, not the draws)
          const std::vector< float > first = content.figure_joints;
          M_figures.pose( content, 12.3 );
          QVERIFY( content.figure_joints == first );

          // the idle clip moves with the time, each player at its own point of it
          const FigureInstance a = figureAt( 0, 0.0f, 0.0f, 0.0f );
          const FigureInstance b = figureAt( 1, 0.0f, 0.0f, 0.0f );
          const QVector3D hand_a = nodeAt( M_figures, a, 1.0, "hand_l" );
          QVERIFY( ( hand_a - nodeAt( M_figures, a, 1.7, "hand_l" ) ).length() > 1.0e-4f );
          QVERIFY( ( hand_a - nodeAt( M_figures, b, 1.0, "hand_l" ) ).length() > 1.0e-4f );
          QVERIFY( ( hand_a - nodeAt( M_figures, b, 1.0 - Figures3D::IDLE_OFFSET, "hand_l" ) ).length() < 1.0e-4f );

          // standing still (no motion): the idle clip, as above
          FigureInstance still = figureAt( 0, 0.0f, 0.0f, 0.0f );
          still.motion.gait = 0.37; // walked, but not moving now
          QVERIFY( ( nodeAt( M_figures, still, 1.0, "hand_l" ) - hand_a ).length() < 1.0e-5f );

          // no figures, no matrices
          content.figures.clear();
          M_figures.pose( content, 1.0 );
          QVERIFY( content.figure_joints.empty() );
      }

    /*!
      the clips of walking and running, read from their motions: slowest
      first, the left foot down at the start of the gait and the right one
      half a cycle later; not the idle clip
    */
    void gaitClips()
      {
          const std::vector< GaitClip > & gaits = M_figures.gaitClips();
          QCOMPARE( gaits.size(), std::size_t( 3 ) );
          const GltfModel & m = M_figures.model();
          QCOMPARE( m.clips[gaits[0].clip].name, QString( "Walk_Loop" ) );
          QCOMPARE( m.clips[gaits[1].clip].name, QString( "Jog_Fwd_Loop" ) );
          QCOMPARE( m.clips[gaits[2].clip].name, QString( "Sprint_Loop" ) );
          // a person's walk, and a sprint
          QVERIFY2( gaits[0].speed > 0.7f && gaits[0].speed < 1.3f, qPrintable( QString::number( gaits[0].speed ) ) );
          QVERIFY2( gaits[2].speed > 6.0f && gaits[2].speed < 10.0f, qPrintable( QString::number( gaits[2].speed ) ) );
          QVERIFY( gaits[0].speed < gaits[1].speed && gaits[1].speed < gaits[2].speed );

          const std::vector< GaitStride > strides = M_figures.strides();
          QCOMPARE( strides.size(), gaits.size() );
          for ( std::size_t k = 0; k < gaits.size(); ++k )
          {
              const GaitClip & g = gaits[k];
              QCOMPARE( g.duration, m.clips[g.clip].duration );
              QVERIFY( g.contact >= 0.0f && g.contact < g.duration );
              QVERIFY( g.down > 0.0f && g.down < g.duration );
              QCOMPARE( strides[k].speed, g.speed );
              QCOMPARE( strides[k].stride, g.speed * g.duration );
              // in the middle of its time down, the left foot is lower than
              // the right one; half a cycle later, the other way
              for ( const float half : { 0.0f, 0.5f } )
              {
                  std::vector< NodeTransform > local = m.rest;
                  m.sample( m.clips[g.clip], g.contact + 0.5f * g.down + half * g.duration, local );
                  std::vector< QMatrix4x4 > global;
                  m.globals( local, global );
                  const float left = global[m.nodeIndex( "foot_l" )].column( 3 ).y();
                  const float right = global[m.nodeIndex( "foot_r" )].column( 3 ).y();
                  QVERIFY2( half == 0.0f ? left < right : right < left,
                            qPrintable( m.clips[g.clip].name + " " + QString::number( half ) ) );
              }
          }

          // the time of a phase: from the left foot coming down
          GaitClip clip;
          clip.contact = 0.3f;
          clip.duration = 1.2f;
          QCOMPARE( clip.timeAt( 0.0f ), 0.3f );
          QVERIFY( std::fabs( clip.timeAt( 0.5f ) - 0.9f ) < 1.0e-6f );

          // a walk starting elsewhere in its cycle (its keys turned round by
          // 0.4 s): the foot comes down 0.4 s earlier, as fast
          {
              GltfModel turned = m;
              GltfModel::Clip & walk = turned.clips[gaits[0].clip];
              const float shift = 0.4f;
              for ( GltfModel::NodeChannels & node : walk.nodes )
              {
                  for ( GltfModel::Channel * ch : { &node.t, &node.r, &node.s } )
                  {
                      for ( float & time : ch->times )
                      {
                          time = std::fmod( time - shift + walk.duration, walk.duration );
                      }
                      // the keys in time order again
                      const int width = ( ch == &node.r ? 4 : 3 );
                      std::vector< std::pair< float, std::vector< float > > > keys;
                      for ( std::size_t k = 0; k < ch->times.size(); ++k )
                      {
                          keys.push_back( { ch->times[k], std::vector< float >( ch->values.begin() + k * width,
                                                                               ch->values.begin() + ( k + 1 ) * width ) } );
                      }
                      std::stable_sort( keys.begin(), keys.end(),
                                        []( const auto & a, const auto & b ) { return a.first < b.first; } );
                      ch->times.clear();
                      ch->values.clear();
                      for ( const auto & key : keys )
                      {
                          ch->times.push_back( key.first );
                          ch->values.insert( ch->values.end(), key.second.begin(), key.second.end() );
                      }
                  }
              }
              HumanoidRig rig;
              QVERIFY( rig.build( turned, quaterniusBoneNames() ) );
              GaitClip later;
              QVERIFY( Figures3D::analyzeGait( turned, rig, gaits[0].clip, &later ) );
              const float moved = std::remainder( gaits[0].contact - later.contact - shift, walk.duration );
              QVERIFY2( std::fabs( moved ) < 0.05f, qPrintable( QString::number( later.contact ) ) );
              QVERIFY( std::fabs( later.speed - gaits[0].speed ) < 0.05f * gaits[0].speed );
          }

          // standing does not walk
          GaitClip idle;
          QVERIFY( ! Figures3D::analyzeGait( m, M_figures.rig(), m.clipIndex( "Idle_Loop" ), &idle ) );
          QVERIFY( ! Figures3D::analyzeGait( m, M_figures.rig(), -1, &idle ) );
      }

    //! how the clips are mixed by the speed
    void mixAt()
      {
          const std::vector< GaitClip > & g = M_figures.gaitClips();
          Figures3D::Mix mix = M_figures.mixAt( 0.0f );
          QCOMPARE( mix.moving, 0.0f );
          mix = M_figures.mixAt( 0.4f * g[0].speed ); // half of MOVING_FULL
          QVERIFY( std::fabs( mix.moving - 0.5f ) < 1.0e-5f );
          QCOMPARE( mix.from, 0 );
          QCOMPARE( mix.to, 0 );
          mix = M_figures.mixAt( g[0].speed );
          QCOMPARE( mix.moving, 1.0f );
          QCOMPARE( mix.from, 0 );
          QCOMPARE( mix.t, 0.0f );
          mix = M_figures.mixAt( 0.25f * g[1].speed + 0.75f * g[2].speed );
          QCOMPARE( mix.from, 1 );
          QCOMPARE( mix.to, 2 );
          QVERIFY( std::fabs( mix.t - 0.75f ) < 1.0e-4f );
          mix = M_figures.mixAt( g[2].speed + 3.0f );
          QCOMPARE( mix.from, 2 );
          QCOMPARE( mix.to, 2 );
      }

    /*!
      the poses between: below a walk, between standing and walking; between
      two clips' speeds, between their poses (the hand somewhere else than
      in either)
    */
    void mixed()
      {
          const std::vector< GaitClip > & g = M_figures.gaitClips();
          auto hand = [&]( const float speed )
              {
                  FigureInstance f = figureAt( 0, 0.0f, 0.0f, 0.0f );
                  f.motion.gait = 0.2;
                  f.motion.vx = speed;
                  return nodeAt( M_figures, f, 1.0, "hand_l" );
              };
          for ( const float between : { 0.5f * Figures3D::MOVING_FULL * g[0].speed, 0.5f * ( g[1].speed + g[2].speed ) } )
          {
              const float slower = ( between < g[0].speed ? 0.0f : g[1].speed );
              const float faster = ( between < g[0].speed ? g[0].speed : g[2].speed );
              const QVector3D h = hand( between );
              QVERIFY2( ( h - hand( slower ) ).length() > 0.02f, qPrintable( QString::number( between ) ) );
              QVERIFY2( ( h - hand( faster ) ).length() > 0.02f, qPrintable( QString::number( between ) ) );
              // nearer to the two than they are to each other
              QVERIFY( ( h - hand( slower ) ).length() < ( hand( faster ) - hand( slower ) ).length() );
          }
      }

    /*!
      the feet do not slide: going at a clip's speed, the left foot stays
      where it came down in the middle half of its time down, for a figure
      of a person's height and a taller one (longer strides), forward and
      backward
    */
    void feetStay()
      {
          for ( const GaitClip & g : M_figures.gaitClips() )
          {
              const QString name = M_figures.model().clips[g.clip].name;
              for ( const float height : { SceneBuilder::PERSON_HEIGHT, 2.22f } )
              {
                  for ( const float way : { 1.0f, -1.0f } )
                  {
                      // a stride for the figure's height, the phase from the gait
                      const float scale = height / SceneBuilder::PERSON_HEIGHT;
                      float low = 1.0e9f, high = -1.0e9f;
                      for ( int k = 0; k <= 10; ++k )
                      {
                          const float phase = ( 0.25f + 0.05f * k ) * g.down / g.duration; // within the time down
                          const double gait = ( way > 0.0f ? phase : phase - 1.0f ) * scale; // backward: from the end
                          const float along = float( gait ) * g.stride();
                          FigureInstance f = figureAt( 0, 3.0f + along * 0.6f, -2.0f + along * 0.8f, 53.13f, height );
                          f.motion.gait = gait;
                          f.motion.vx = way * g.speed * 0.6f;
                          f.motion.vy = way * g.speed * 0.8f;
                          if ( way < 0.0f ) f.body = 53.13f; // still facing the same way
                          const QVector3D foot = leftFoot( M_figures, f );
                          const float d = foot.x() * 0.6f + foot.z() * 0.8f; // along the way
                          low = std::min( low, d );
                          high = std::max( high, d );
                      }
                      QVERIFY2( high - low < 0.06f * scale,
                                qPrintable( name + QString( " height %1 way %2: %3 m" ).arg( height ).arg( way ).arg( high - low ) ) );
                  }
              }
          }
      }

    /*!
      the same through the gait table: a player going steadily at the
      jog's speed, drawn between the frames; and backward at the walk's
    */
    void feetStayOnTheLog()
      {
          const std::vector< GaitClip > & gaits = M_figures.gaitClips();
          for ( const int which : { 1, 0 } )
          {
              const GaitClip & g = gaits[which];
              const float way = ( which == 0 ? -1.0f : 1.0f );
              DispCont frames;
              for ( int i = 0; i < 60; ++i )
              {
                  auto d = std::make_shared< rcss::rcg::DispInfoT >();
                  rcss::rcg::PlayerT & p = d->show_.player_[0];
                  p.side_ = 'l';
                  p.unum_ = 1;
                  p.state_ = 1;
                  p.x_ = way * g.speed * 0.1f * i;
                  p.body_ = 0.0f;
                  frames.push_back( d );
              }
              GaitTable table;
              table.setStrides( M_figures.strides() );
              table.update( frames, 0.1 );

              // drawn 20 times a cycle; the first time the left foot is in
              // the middle of its time down
              float low = 1.0e9f, high = -1.0e9f;
              int down = 0;
              for ( int k = 200; k < 800; ++k )
              {
                  const std::size_t index = k / 20;
                  const double alpha = ( k % 20 ) / 20.0;
                  FigureInstance f = figureAt( 0, way * g.speed * 0.1f * float( index + alpha ), 0.0f, 0.0f, SceneBuilder::PERSON_HEIGHT );
                  f.motion = table.motion( 0, index, index + 1, alpha );
                  const double phase = ( f.motion.gait - std::floor( f.motion.gait ) ) * g.duration;
                  const bool in = ( phase > 0.3 * g.down && phase < 0.7 * g.down );
                  if ( ! in )
                  {
                      if ( down > 0 ) break;
                      continue;
                  }
                  ++down;
                  const float x = leftFoot( M_figures, f ).x();
                  low = std::min( low, x );
                  high = std::max( high, x );
              }
              QVERIFY( down > 3 );
              QVERIFY2( high - low < 0.06f, qPrintable( QString::number( high - low ) ) );
          }
      }

    /*!
      going diagonally, the hips turn toward the way, the chest keeps the
      body direction; straight forward or sideways, no turn
    */
    void twist()
      {
          const float speed = M_figures.gaitClips()[1].speed;
          auto facings = [&]( const float way, float * hips, float * chest )
              {
                  FigureInstance f = figureAt( 0, 0.0f, 0.0f, 20.0f );
                  f.motion.gait = 0.3;
                  const float rad = ( 20.0f + way ) * float( M_PI ) / 180.0f;
                  f.motion.vx = speed * std::cos( rad );
                  f.motion.vy = speed * std::sin( rad );
                  *hips = facing( nodeAt( M_figures, f, 0.0, "thigh_l" ), nodeAt( M_figures, f, 0.0, "thigh_r" ) );
                  *chest = facing( nodeAt( M_figures, f, 0.0, "upperarm_l" ), nodeAt( M_figures, f, 0.0, "upperarm_r" ) );
                  return M_figures.twistOf( f );
              };
          float hips0, chest0;
          QCOMPARE( facings( 0.0f, &hips0, &chest0 ), 0.0f );
          for ( const float way : { 45.0f, -45.0f, 135.0f, 90.0f } )
          {
              float hips, chest;
              const float twist = facings( way, &hips, &chest );
              const float expected = Figures3D::TWIST_MAX * std::sin( 2.0f * way * float( M_PI ) / 180.0f );
              QVERIFY2( std::fabs( twist - expected ) < 1.0e-3f, qPrintable( QString::number( twist ) ) );
              QVERIFY2( std::fabs( angleBetween( hips, hips0 ) - expected ) < 1.0f,
                        qPrintable( QString( "%1: hips %2" ).arg( way ).arg( angleBetween( hips, hips0 ) ) ) );
              QVERIFY2( std::fabs( angleBetween( chest, chest0 ) ) < 1.0f,
                        qPrintable( QString( "%1: chest %2" ).arg( way ).arg( angleBetween( chest, chest0 ) ) ) );
          }
          // slower than a walk, less
          FigureInstance slow = figureAt( 0, 0.0f, 0.0f, 0.0f );
          slow.motion.vx = slow.motion.vy = 0.1f;
          QVERIFY( M_figures.twistOf( slow ) < 0.5f * Figures3D::TWIST_MAX );
      }

    /*!
      leaning toward the acceleration about the feet: forward when speeding
      up, back when slowing down, to the side when turning; upright going
      steadily
    */
    void lean()
      {
          FigureInstance upright = figureAt( 0, 5.0f, 5.0f, 90.0f ); // facing the field's +y, the world's +z
          const QVector3D head0 = nodeAt( M_figures, upright, 0.0, "Head" );
          const QVector3D foot0 = M_figures.modelMatrix( upright ).map( QVector3D( 0.0f, M_figures.model().min_y, 0.0f ) );
          const float most = Figures3D::TILT_MAX * std::tanh( 1.0f ) * float( M_PI ) / 180.0f;
          struct Case {
              float ax, ay; //!< on the field
              QVector3D toward; //!< the head goes (world)
          };
          for ( const Case & c : { Case{ 0.0f, 15.0f, QVector3D( 0.0f, 0.0f, 1.0f ) }, // speeding up
                                   Case{ 0.0f, -15.0f, QVector3D( 0.0f, 0.0f, -1.0f ) }, // slowing down
                                   Case{ 15.0f, 0.0f, QVector3D( 1.0f, 0.0f, 0.0f ) } } ) // turning to the right
          {
              FigureInstance f = upright;
              f.motion.ax = c.ax;
              f.motion.ay = c.ay;
              const QVector3D moved = nodeAt( M_figures, f, 0.0, "Head" ) - head0;
              // as far as the head's height leaned by the angle, toward the acceleration
              const float expected = ( head0.y() ) * std::sin( most );
              QVERIFY2( std::fabs( QVector3D::dotProduct( moved, c.toward ) - expected ) < 0.02f,
                        qPrintable( QString( "%1 %2: %3" ).arg( c.ax ).arg( c.ay ).arg( QVector3D::dotProduct( moved, c.toward ) ) ) );
              // the feet stay on the ground where they were
              const QVector3D foot = M_figures.modelMatrix( f ).map( QVector3D( 0.0f, M_figures.model().min_y, 0.0f ) );
              QVERIFY( ( foot - foot0 ).length() < 1.0e-4f );
          }
          // more acceleration, more lean, up to TILT_MAX
          FigureInstance f = upright;
          f.motion.ay = 1000.0f;
          const float y = ( nodeAt( M_figures, f, 0.0, "Head" ) - foot0 ).y();
          QVERIFY( std::fabs( y - head0.y() * std::cos( Figures3D::TILT_MAX * float( M_PI ) / 180.0f ) ) < 0.01f );
      }
};

QTEST_MAIN( TestFigures3D )

#include "test_figures_3d.moc"
