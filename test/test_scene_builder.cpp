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

#include "cameras_3d.h"
#include "disp_holder.h"
#include "figure_pack.h"
#include "label_atlas.h"
#include "options.h"
#include "player_style.h"
#include "scene_builder.h"
#include "sun_3d.h"

#include <rcss/rcg/types.h>

namespace {

//! the bodies of the bundled Human pack, from which the figures' looks are drawn
const FigureCast &
humanCast()
{
    static const FigureCast cast = FigurePacks::builtin( "human" )->cast();
    return cast;
}

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

bool
near( const float a,
      const float b )
{
    return std::fabs( a - b ) < 1.0e-4f;
}

//! a camera at the position, looking at the target
SceneBuilder::LabelView
labelView( const QVector3D & position,
           const QVector3D & target )
{
    SceneBuilder::LabelView view;
    view.camera.position = position;
    view.camera.target = target;
    view.camera.fov_y = 35.0f;
    view.height = 720.0f;
    view.font_pixels = 12.0;
    return view;
}

//! the height (m) of a label's text: 40 of its 64 pixels
float
textHeight( const Billboard3D & label )
{
    return label.size.y() * LabelAtlas::LABEL_FONT / LabelAtlas::LABEL_HEIGHT;
}

//! set an option of the Show tab, which Options only toggles
void
setShow( bool ( Options::*shown )() const,
         void ( Options::*toggle )(),
         const bool on )
{
    Options & opt = Options::instance();
    if ( ( opt.*shown )() != on ) ( opt.*toggle )();
}

QVector4D
rgba( const QColor & c )
{
    return QVector4D( float( c.redF() ), float( c.greenF() ), float( c.blueF() ), 1.0f );
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
          // each test from the same options, whatever the one before left
          Options::instance().unselectPlayer();
          setShow( &Options::showPlayer, &Options::toggleShowPlayer, true );
          setShow( &Options::showBall, &Options::toggleShowBall, true );
          setShow( &Options::showPlayerNumber, &Options::toggleShowPlayerNumber, true );
          setShow( &Options::showPlayerType, &Options::toggleShowPlayerType, false );
          setShow( &Options::showCard, &Options::toggleShowCard, true );
          setShow( &Options::showIllegalDefense, &Options::toggleShowIllegalDefense, false );
          setShow( &Options::showStamina, &Options::toggleShowStamina, false );
          setShow( &Options::showStaminaCapacity, &Options::toggleShowStaminaCapacity, false );
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

          // the nets in the colors of the teams that keep the goals: the
          // left team the left goal (x < 0); the floors stay dark
          c.clear();
          SceneBuilder::addGoals( c, QColor( 255, 0, 0 ), QColor( 0, 0, 255 ) );
          int left_nets = 0, right_nets = 0;
          for ( const Instance3D & q : c.quads )
          {
              if ( q.color.w() != SceneBuilder::NET_OPACITY ) continue;
              const bool left = ( at( q ).x() < 0.0f );
              QCOMPARE( q.color.toVector3D(), left ? QVector3D( 1, 0, 0 ) : QVector3D( 0, 0, 1 ) );
              ++( left ? left_nets : right_nets );
          }
          QCOMPARE( left_nets, 4 );
          QCOMPARE( right_nets, 4 );
      }

    void players()
      {
          SceneContent c;
          rcss::rcg::ShowInfoT show = makeShow();
          show.player_[3].side_ = 'n'; // not in the show
          SceneBuilder::addShow( c, show, M_holder, Options::instance() );

          const size_t n = rcss::rcg::MAX_PLAYER * 2 - 1;
          QCOMPARE( c.cylinders.size(), n );
          QCOMPARE( c.spheres.size(), n ); // the heads
          QCOMPARE( c.balls.size(), size_t( 1 ) );
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

          // the players drawn, for a right click to aim at (Picking)
          QCOMPARE( c.picks.size(), n );
          QCOMPARE( c.picks[0].side, rcss::rcg::LEFT );
          QCOMPARE( c.picks[0].unum, 1 );
          QCOMPARE( c.picks[0].x, -10.0f );
          QCOMPARE( c.picks[0].y, -30.0f );
          QCOMPARE( c.picks[0].radius, float( type.player_size_ ) );
          // the one not in the show is skipped
          QCOMPARE( c.picks[3].unum, 5 );
          QCOMPARE( c.picks.back().side, rcss::rcg::RIGHT );
          QCOMPARE( c.picks.back().unum, 11 );
          c.clear();
          QVERIFY( c.picks.empty() );
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
          QCOMPARE( c.balls.size(), size_t( 1 ) );
          const Instance3D & ball = c.balls.back();
          const Instance3D & shadow = c.discs.back();
          // on the ground
          QVERIFY( near( at( ball, QVector3D( 0, -1, 0 ) ), QVector3D( 3, 0, -4 ) ) );
          QVERIFY( near( at( shadow ), QVector3D( 3, SceneBuilder::BALL_SHADOW_Y, -4 ) ) );
          QCOMPARE( shadow.color.w(), 0.45f );

          // in the air (z, which the server of the 2.5D simulation may send)
          show.ball_.z_ = 2.0f;
          c.clear();
          SceneBuilder::addShow( c, show, M_holder, opt );
          QVERIFY( near( at( c.balls.back() ), QVector3D( 3, 2.0f + float( r ), -4 ) ) );
          QVERIFY( near( at( c.discs.back() ), QVector3D( 3, SceneBuilder::BALL_SHADOW_Y, -4 ) ) );
          QCOMPARE( c.discs.back().color.w(), 0.3f );

          // turned as it rolled, about its center, still of radius r
          const QQuaternion turn = QQuaternion::fromAxisAndAngle( 1.0f, 2.0f, 3.0f, 70.0f );
          c.clear();
          SceneBuilder::addShow( c, show, M_holder, opt, nullptr, nullptr, turn );
          const QVector3D center( 3, 2.0f + float( r ), -4 );
          QVERIFY( near( at( c.balls.back() ), center ) );
          for ( const QVector3D & v : { QVector3D( 1, 0, 0 ), QVector3D( 0, 1, 0 ), QVector3D( 0, 0, 1 ) } )
          {
              QVERIFY( near( at( c.balls.back(), v ), center + turn.rotatedVector( v ) * float( r ) ) );
          }
      }

    /*!
      the ball's size in the 3D view: its own option, apart from the 2D
      view's; the real one (the server's ball_size) below 0.01
    */
    void ballSize()
      {
          Options & opt = Options::instance();
          const double size_2d = opt.ballSize();
          const double real = M_holder.serverParam().ball_size_;
          rcss::rcg::ShowInfoT show = makeShow();
          show.ball_.x_ = 3.0f;
          show.ball_.y_ = -4.0f;
          show.ball_.z_ = 2.0f;
          const auto radius = [&]() {
              SceneContent c;
              SceneBuilder::addShow( c, show, M_holder, opt );
              return ( at( c.balls.back() ) - at( c.balls.back(), QVector3D( 0, -1, 0 ) ) ).length();
          };

          // the real size by default, whatever the 2D view's
          QCOMPARE( opt.ballSize3D(), Options::BALL_SIZE_3D );
          QCOMPARE( SceneBuilder::ballRadius( opt, real ), real );
          QVERIFY( near( radius(), float( real ) ) );
          opt.setBallSize( 0.8 );
          QCOMPARE( SceneBuilder::ballRadius( opt, real ), real );
          QVERIFY( near( radius(), float( real ) ) );

          // its own
          opt.setBallSize3D( 0.2 );
          QCOMPARE( SceneBuilder::ballRadius( opt, real ), 0.2 );
          QVERIFY( near( radius(), 0.2f ) );
          // in the air by its radius
          {
              SceneContent c;
              SceneBuilder::addShow( c, show, M_holder, opt );
              QVERIFY( near( at( c.balls.back() ), QVector3D( 3, 2.2f, -4 ) ) );
          }
          // below 0.01, the real one
          opt.setBallSize3D( 0.005 );
          QCOMPARE( SceneBuilder::ballRadius( opt, real ), real );

          opt.setBallSize3D( Options::BALL_SIZE_3D );
          opt.setBallSize( size_2d );
      }

    void labelsFar()
      {
          LabelAtlas atlas;
          SceneContent c;
          rcss::rcg::ShowInfoT show = makeShow();
          show.player_[3].side_ = 'n'; // not in the show
          SceneBuilder::addLabels( c, show, M_holder, Options::instance(), atlas,
                                   labelView( QVector3D( 0, 40, 60 ), QVector3D( 0, 0, 0 ) ) );
          QCOMPARE( c.labels.size(), size_t( rcss::rcg::MAX_PLAYER * 2 - 1 ) );

          // far from the camera: above the head, in meters (1.3 m high, 3:1)
          auto it = std::find_if( c.labels.begin(), c.labels.end(), []( const Billboard3D & b ) {
                  return near( b.center, QVector3D( -10, SceneBuilder::NUMBER_HEIGHT, -30 ) ); } );
          QVERIFY( it != c.labels.end() );
          QVERIFY( near( it->size.y(), float( LabelAtlas::LABEL_SCALE ) ) );
          QVERIFY( near( it->size.x(), float( LabelAtlas::LABEL_SCALE ) * 3.0f ) );
          QCOMPARE( it->offset, QVector2D( 0, 0 ) );
          // with its text in its slot, white
          QCOMPARE( it->uv, atlas.uv( 0 ) );
          QCOMPARE( it->color, QVector4D( 1, 1, 1, 1 ) );
          QVERIFY( atlas.textWidth( 0 ) > 0.0 );

          // from far to near: the first is the farthest from the camera
          const QVector3D eye( 0, 40, 60 );
          const QVector3D forward = -eye.normalized();
          for ( std::size_t i = 1; i < c.labels.size(); ++i )
          {
              QVERIFY( QVector3D::dotProduct( c.labels[i - 1].center - eye, forward )
                       >= QVector3D::dotProduct( c.labels[i].center - eye, forward ) - 1.0e-4f );
          }
      }

    void labelsNear()
      {
          // 5 m in front of the camera: the text would be 185 pixels high;
          // it is 2 x 12 pixels at most, smaller toward the head
          LabelAtlas atlas;
          SceneContent c;
          rcss::rcg::ShowInfoT show = makeShow();
          for ( auto & p : show.player_ ) p.state_ = 0, p.side_ = 'n';
          show.player_[0].side_ = 'l';
          show.player_[0].state_ = rcss::rcg::STAND;
          show.player_[0].x_ = 0.0f;
          show.player_[0].y_ = 0.0f;
          const SceneBuilder::LabelView view = labelView( QVector3D( 0, SceneBuilder::NUMBER_HEIGHT, 5 ),
                                                          QVector3D( 0, SceneBuilder::NUMBER_HEIGHT, 0 ) );
          SceneBuilder::addLabels( c, show, M_holder, Options::instance(), atlas, view );
          QCOMPARE( c.labels.size(), size_t( 1 ) );
          const Billboard3D & label = c.labels[0];

          const float focal = 720.0f / ( 2.0f * std::tan( 17.5f * float( M_PI ) / 180.0f ) );
          const float depth = 5.0f; // the camera looks level
          const float pixels = textHeight( label ) * focal / depth;
          QVERIFY2( std::fabs( pixels - 24.0f ) < 0.5f, qPrintable( QString::number( pixels ) ) );
          const float k = label.size.y() / float( LabelAtlas::LABEL_SCALE );
          QVERIFY( k < 0.2f );
          QVERIFY( near( label.center.y(), SceneBuilder::LABEL_BASE + k * ( SceneBuilder::NUMBER_HEIGHT - SceneBuilder::LABEL_BASE ) ) );
      }

    void labelsTop()
      {
          // from the top camera: to the right of the circle on the ground,
          // the foot of the text at the player's center, as high as the font
          LabelAtlas atlas;
          SceneContent c;
          SceneBuilder::LabelView view = labelView( QVector3D(), QVector3D() );
          view.camera = Cameras3D::top( 16.0f / 9.0f );
          SceneBuilder::addLabels( c, makeShow(), M_holder, Options::instance(), atlas, view );

          const rcss::rcg::PlayerTypeT & type = M_holder.playerType( 0 );
          const float ground = float( type.player_size_ + type.kickable_margin_ + M_holder.serverParam().ball_size_ );
          const float pixels_per_meter = 720.0f / ( 2.0f * view.camera.ortho_half_height );
          auto it = std::find_if( c.labels.begin(), c.labels.end(), []( const Billboard3D & b ) {
                  return std::fabs( b.center.z() + 30.0f ) < 1.0f && b.center.x() < 0.0f; } );
          QVERIFY( it != c.labels.end() );
          QVERIFY( std::fabs( textHeight( *it ) * pixels_per_meter - 12.0f ) < 1.0e-3f );
          const float k = it->size.y() / float( LabelAtlas::LABEL_SCALE );
          const float width = float( atlas.textWidth( 0 ) ) * k;
          QVERIFY( near( it->center.x(), -10.0f + ground + SceneBuilder::LABEL_GAP * k + width / 2.0f ) );
          QVERIFY( near( it->center.z(), -30.0f - textHeight( *it ) / 2.0f ) );
      }

    void labelTexts()
      {
          Options & opt = Options::instance();
          LabelAtlas atlas;
          SceneContent c;
          const SceneBuilder::LabelView view = labelView( QVector3D( 0, 40, 60 ), QVector3D( 0, 0, 0 ) );

          // the number and the type: a wider text
          SceneBuilder::addLabels( c, makeShow(), M_holder, opt, atlas, view );
          const double number = atlas.textWidth( 0 );
          opt.toggleShowPlayerType();
          SceneBuilder::addLabels( c, makeShow(), M_holder, opt, atlas, view );
          opt.toggleShowPlayerType();
          QVERIFY( atlas.textWidth( 0 ) > number );

          // neither: no labels
          c.clear();
          opt.toggleShowPlayerNumber();
          SceneBuilder::addLabels( c, makeShow(), M_holder, opt, atlas, view );
          opt.toggleShowPlayerNumber();
          QCOMPARE( c.labels.size(), size_t( 0 ) );

          // illegal defense, in its color when it is shown
          rcss::rcg::ShowInfoT show = makeShow();
          show.player_[0].state_ |= rcss::rcg::ILLEGAL_DEFENSE;
          c.clear();
          opt.toggleShowIllegalDefense();
          SceneBuilder::addLabels( c, show, M_holder, opt, atlas, view );
          opt.toggleShowIllegalDefense();
          auto it = std::find_if( c.labels.begin(), c.labels.end(), [&]( const Billboard3D & b ) { return b.uv == atlas.uv( 0 ); } );
          QVERIFY( it != c.labels.end() );
          QCOMPARE( it->color, rgba( opt.illegalDefensePen().color() ) );
          c.clear();
          SceneBuilder::addLabels( c, show, M_holder, opt, atlas, view );
          it = std::find_if( c.labels.begin(), c.labels.end(), [&]( const Billboard3D & b ) { return b.uv == atlas.uv( 0 ); } );
          QCOMPARE( it->color, QVector4D( 1, 1, 1, 1 ) );
      }

    void cards()
      {
          Options & opt = Options::instance();
          LabelAtlas atlas;
          SceneContent c;
          const SceneBuilder::LabelView view = labelView( QVector3D( 0, 40, 60 ), QVector3D( 0, 0, 0 ) );
          rcss::rcg::ShowInfoT show = makeShow();
          show.player_[0].state_ |= rcss::rcg::YELLOW_CARD;
          show.player_[1].state_ |= rcss::rcg::RED_CARD;
          SceneBuilder::addLabels( c, show, M_holder, opt, atlas, view );
          QCOMPARE( c.labels.size(), size_t( rcss::rcg::MAX_PLAYER * 2 + 2 ) );

          // to the right of the number, in the white slot tinted yellow or red
          std::vector< Billboard3D > cards;
          std::copy_if( c.labels.begin(), c.labels.end(), std::back_inserter( cards ), [&]( const Billboard3D & b ) {
                  return b.uv == atlas.uv( LabelAtlas::WHITE_SLOT ); } );
          QCOMPARE( cards.size(), size_t( 2 ) );
          const Billboard3D & yellow = ( cards[0].center.z() < cards[1].center.z() ? cards[0] : cards[1] );
          const Billboard3D & red = ( cards[0].center.z() < cards[1].center.z() ? cards[1] : cards[0] );
          QCOMPARE( yellow.color, QVector4D( 1, 1, 0, 1 ) );
          QCOMPARE( red.color, QVector4D( 1, 0, 0, 1 ) );
          QVERIFY( near( yellow.center, QVector3D( -10, SceneBuilder::NUMBER_HEIGHT, -30 ) ) );
          QVERIFY( near( yellow.size.x(), SceneBuilder::CARD_WIDTH ) );
          QVERIFY( near( yellow.size.y(), SceneBuilder::CARD_HEIGHT ) );
          const float right = float( atlas.textWidth( 0 ) ) / 2.0f + SceneBuilder::CARD_GAP + SceneBuilder::CARD_WIDTH / 2.0f;
          QVERIFY( near( yellow.offset.x(), right ) );
          QCOMPARE( yellow.offset.y(), 0.0f );

          // not shown: no cards
          c.clear();
          opt.toggleShowCard();
          SceneBuilder::addLabels( c, show, M_holder, opt, atlas, view );
          opt.toggleShowCard();
          QCOMPARE( c.labels.size(), size_t( rcss::rcg::MAX_PLAYER * 2 ) );
      }

    void staminaRates()
      {
          const rcss::rcg::ServerParamT & sp = M_holder.serverParam();
          rcss::rcg::PlayerT p = makeShow().player_[0];
          p.stamina_ = float( sp.stamina_max_ / 4 );
          QVERIFY( near( float( SceneBuilder::staminaRate( p, sp ) ), 0.25f ) );
          p.stamina_ = float( sp.stamina_max_ * 2 );
          QCOMPARE( SceneBuilder::staminaRate( p, sp ), 1.0 );

          // without the capacity (old logs): none
          QCOMPARE( SceneBuilder::capacityRate( p, sp ), -1.0 );
          p.stamina_capacity_ = float( sp.stamina_capacity_ / 2 );
          QVERIFY( near( float( SceneBuilder::capacityRate( p, sp ) ), 0.5f ) );

          // red at or below the thresholds of the recovery and the effort, green when full
          const double threshold = std::max( sp.recover_dec_thr_, sp.effort_dec_thr_ );
          QCOMPARE( SceneBuilder::staminaHue( threshold, sp ), 0.0 );
          QCOMPARE( SceneBuilder::staminaHue( threshold / 2, sp ), 0.0 );
          QCOMPARE( SceneBuilder::staminaHue( 1.0, sp ), 120.0 );
          QVERIFY( near( float( SceneBuilder::staminaHue( ( 1.0 + threshold ) / 2, sp ) ), 60.0f ) );

          // the higher of the two thresholds, whichever
          rcss::rcg::ServerParamT other;
          other.recover_dec_thr_ = 0.3;
          other.effort_dec_thr_ = 0.5;
          QCOMPARE( SceneBuilder::staminaHue( 0.5, other ), 0.0 );
          std::swap( other.recover_dec_thr_, other.effort_dec_thr_ );
          QCOMPARE( SceneBuilder::staminaHue( 0.5, other ), 0.0 );
          QVERIFY( near( float( SceneBuilder::staminaHue( 0.75, other ) ), 60.0f ) );
      }

    void staminaBars()
      {
          Options & opt = Options::instance();
          const rcss::rcg::ServerParamT & sp = M_holder.serverParam();
          LabelAtlas atlas;
          SceneContent c;
          rcss::rcg::ShowInfoT show = makeShow();
          for ( auto & p : show.player_ ) p.side_ = 'n';
          rcss::rcg::PlayerT & p = show.player_[0];
          p.side_ = 'l';
          p.stamina_ = float( sp.stamina_max_ * 0.75 );
          p.stamina_capacity_ = float( sp.stamina_capacity_ * 0.5 );

          // none asked: the number only, at its height
          const SceneBuilder::LabelView view = labelView( QVector3D( 0, 40, 60 ), QVector3D( 0, 0, 0 ) );
          SceneBuilder::addLabels( c, show, M_holder, opt, atlas, view );
          QCOMPARE( c.labels.size(), size_t( 1 ) );
          QCOMPARE( c.labels[0].center.y(), SceneBuilder::NUMBER_HEIGHT );

          // both: the number up by BAR_LIFT, and under it two bars, each a background and a fill
          c.clear();
          opt.toggleShowStamina();
          opt.toggleShowStaminaCapacity();
          SceneBuilder::addLabels( c, show, M_holder, opt, atlas, view );
          QCOMPARE( c.labels.size(), size_t( 5 ) );
          const Billboard3D & number = c.labels[0];
          QVERIFY( near( number.center.y(), SceneBuilder::NUMBER_HEIGHT + SceneBuilder::BAR_LIFT ) );
          const Billboard3D & back = c.labels[1];
          const Billboard3D & fill = c.labels[2];
          const Billboard3D & capacity_back = c.labels[3];
          const Billboard3D & capacity_fill = c.labels[4];
          for ( const Billboard3D & b : c.labels ) QCOMPARE( b.center, number.center );
          // the stamina bar under the text: dark, and the fill from the left end
          const float text_half = textHeight( number ) / 2.0f;
          QVERIFY( near( back.offset.y(), -( text_half + SceneBuilder::BAR_GAP + SceneBuilder::BAR_STAMINA / 2.0f ) ) );
          QVERIFY( near( back.size.x(), SceneBuilder::BAR_WIDTH + 2.0f * SceneBuilder::BAR_FRAME ) );
          QCOMPARE( back.color, QVector4D( 0, 0, 0, SceneBuilder::BAR_BACK_OPACITY ) );
          QVERIFY( near( fill.size.x(), SceneBuilder::BAR_WIDTH * 0.75f ) );
          QVERIFY( near( fill.offset.x(), -0.25f * SceneBuilder::BAR_WIDTH / 2.0f ) );
          QVERIFY( near( fill.offset.y(), back.offset.y() ) );
          const QColor hue = QColor::fromHslF( SceneBuilder::staminaHue( 0.75, sp ) / 360.0, 0.85, 0.5 );
          QCOMPARE( fill.color, rgba( hue ) );
          // the capacity bar under it, thinner, blue
          QVERIFY( near( capacity_back.offset.y(), -( text_half + SceneBuilder::BAR_GAP + SceneBuilder::BAR_STAMINA
                                                      + SceneBuilder::BAR_GAP + SceneBuilder::BAR_CAPACITY / 2.0f ) ) );
          QVERIFY( near( capacity_fill.size.x(), SceneBuilder::BAR_WIDTH * 0.5f ) );
          QVERIFY( near( capacity_fill.size.y(), SceneBuilder::BAR_CAPACITY ) );
          QCOMPARE( capacity_fill.color, rgba( SceneBuilder::CAPACITY_COLOR ) );

          // no stamina left: the background only; no stamina in the show: no bar
          c.clear();
          p.stamina_ = 0.0f;
          opt.toggleShowStaminaCapacity();
          SceneBuilder::addLabels( c, show, M_holder, opt, atlas, view );
          QCOMPARE( c.labels.size(), size_t( 2 ) );
          c.clear();
          p.stamina_ = rcss::rcg::SHOWINFO_SCALE2F;
          SceneBuilder::addLabels( c, show, M_holder, opt, atlas, view );
          opt.toggleShowStamina();
          QCOMPARE( c.labels.size(), size_t( 1 ) );
          QCOMPARE( c.labels[0].center.y(), SceneBuilder::NUMBER_HEIGHT );
      }

    void staminaBarsTop()
      {
          // from the top camera: from the left end of the number, as the 2D view writes them
          Options & opt = Options::instance();
          LabelAtlas atlas;
          SceneContent c;
          rcss::rcg::ShowInfoT show = makeShow();
          for ( auto & p : show.player_ ) p.side_ = 'n';
          show.player_[0].side_ = 'l';
          show.player_[0].stamina_ = float( M_holder.serverParam().stamina_max_ );
          SceneBuilder::LabelView view = labelView( QVector3D(), QVector3D() );
          view.camera = Cameras3D::top( 16.0f / 9.0f );
          opt.toggleShowStamina();
          SceneBuilder::addLabels( c, show, M_holder, opt, atlas, view );
          opt.toggleShowStamina();
          QCOMPARE( c.labels.size(), size_t( 3 ) );
          const float k = c.labels[0].size.y() / float( LabelAtlas::LABEL_SCALE );
          const float number_left = c.labels[0].center.x() - float( atlas.textWidth( 0 ) ) * k / 2.0f;
          const float bar_left = c.labels[0].center.x() + c.labels[1].offset.x() - c.labels[1].size.x() / 2.0f;
          QVERIFY( near( bar_left, number_left ) );
      }

    /*!
      With the player camera's sight: the players its player does not see
      are not drawn, nor the ball; one whose team it may not know fades to
      gray, without the highlights; one whose directions it does not know
      has no direction marks; its own is not drawn at its eyes
    */
    /*!
      the players as human figures: where their directions are known, a
      figure in place of the cylinder, the head and the direction marks,
      in the body's color; the circles, the rings and the picks as before
    */
    void figures()
      {
          Options & opt = Options::instance();
          rcss::rcg::ShowInfoT show = makeShow();
          show.player_[3].body_ = 45.0f;
          opt.setSelectedNumber( rcss::rcg::LEFT, 4 ); // index 3: highlighted

          SceneContent simple;
          SceneBuilder::addShow( simple, show, M_holder, opt );
          SceneContent c;
          SceneBuilder::addShow( c, show, M_holder, opt, nullptr, &humanCast() );
          opt.unselectPlayer();
          const size_t n = rcss::rcg::MAX_PLAYER * 2;
          QCOMPARE( c.figures.size(), n );
          QCOMPARE( c.cylinders.size(), size_t( 0 ) );
          QCOMPARE( c.spheres.size(), size_t( 0 ) ); // no heads
          QCOMPARE( c.balls.size(), size_t( 1 ) );
          QCOMPARE( c.cones.size(), size_t( 0 ) );
          QCOMPARE( c.discs.size(), simple.discs.size() );
          QCOMPARE( c.rings.size(), size_t( 1 ) );
          QCOMPARE( c.picks.size(), n );
          // picked as wide as the figure: wider than the cylinder
          QVERIFY( near( c.picks[3].radius, simple.picks[3].radius * SceneBuilder::FIGURE_PICK_SCALE * float( opt.figureScale3D() ) ) );
          QVERIFY( c.picks[3].radius > simple.picks[3].radius );
          const FigureInstance & f = c.figures[3];
          QCOMPARE( f.player, 3 );
          QCOMPARE( f.x, show.player_[3].x_ );
          QCOMPARE( f.y, show.player_[3].y_ );
          QCOMPARE( f.body, 45.0f );
          QCOMPARE( f.height, SceneBuilder::PERSON_HEIGHT * float( opt.figureScale3D() ) );
          QCOMPARE( f.color, simple.cylinders[3].color ); // the body's color
          QVERIFY( c.figure_joints.empty() ); // Figures3D poses them

          // without the figures: none
          QVERIFY( simple.figures.empty() );

          // a player whose directions the player camera does not know: a cylinder
          Perceptions seen;
          seen.active = true;
          seen.players.fill( PlayerSight::FULL );
          seen.players[2] = PlayerSight::FELT;
          c.clear();
          SceneBuilder::addShow( c, show, M_holder, opt, &seen, &humanCast() );
          QCOMPARE( c.figures.size(), n - 1 );
          QCOMPARE( c.cylinders.size(), size_t( 1 ) );
          QVERIFY( near( at( c.cylinders[0] ).x(), show.player_[2].x_ ) );
          for ( const FigureInstance & g : c.figures ) QVERIFY( g.player != 2 );
      }

    /*!
      the simple shadows: one for each player drawn, as tall as it is drawn
      and as wide as its body (of the cylinder, or the figure's, not the
      wider pick), black at Sun3D::SHADOW_OPACITY
    */
    void blobShadows()
      {
          Options & opt = Options::instance();
          rcss::rcg::ShowInfoT show = makeShow();
          show.player_[3].side_ = 'n'; // not in the show: no shadow
          const auto same = []( const QMatrix4x4 & a, const QMatrix4x4 & b ) {
              for ( int i = 0; i < 16; ++i )
              {
                  if ( std::fabs( a.constData()[i] - b.constData()[i] ) > 1.0e-5f ) return false;
              }
              return true;
          };
          for ( const bool figures : { false, true } )
          {
              SceneContent c;
              SceneBuilder::addShow( c, show, M_holder, opt, nullptr, figures ? &humanCast() : nullptr );
              SceneBuilder::addBlobShadows( c );
              QCOMPARE( c.picks.size(), size_t( rcss::rcg::MAX_PLAYER * 2 - 1 ) );
              QCOMPARE( c.blob_shadows.size(), c.picks.size() );
              for ( size_t i = 0; i < c.picks.size(); ++i )
              {
                  const PlayerPick & p = c.picks[i];
                  QCOMPARE( p.figure, figures );
                  const rcss::rcg::PlayerT * player = nullptr;
                  for ( const rcss::rcg::PlayerT & q : show.player_ )
                  {
                      if ( q.side() == p.side && q.unum_ == p.unum ) player = &q;
                  }
                  QVERIFY( player );
                  const float body = float( M_holder.playerType( player->type_ ).player_size_ );
                  const float top = p.head_height + SceneBuilder::HEAD_RADIUS;
                  // a figure as wide as its body times its height over a person's
                  const float width = 2.0f * body * ( figures ? top / SceneBuilder::PERSON_HEIGHT : 1.0f );
                  QVERIFY2( same( c.blob_shadows[i].model, Sun3D::blobShadow( p.x, p.y, top, width ) ),
                            qPrintable( QString( "%1 %2" ).arg( i ).arg( figures ) ) );
                  QCOMPARE( c.blob_shadows[i].color, QVector4D( 0.0f, 0.0f, 0.0f, Sun3D::SHADOW_OPACITY ) );
              }
          }

          // no players drawn: no shadows
          SceneContent c;
          opt.toggleShowPlayer();
          SceneBuilder::addShow( c, show, M_holder, opt );
          opt.toggleShowPlayer();
          SceneBuilder::addBlobShadows( c );
          QVERIFY( c.blob_shadows.empty() );
      }

    /*!
      the figures' looks (FigureLooks): each player's own by its team, its
      side and its number, of the bodies of the option; as far as the
      player camera's player knows who it is, toward the standard ones
    */
    void looks()
      {
          Options & opt = Options::instance();
          rcss::rcg::ShowInfoT show = makeShow();
          const DispConstPtr disp = M_holder.currentDisp();
          const std::string names[2] = { ( disp ? disp->team_[0].name_ : std::string() ),
                                         ( disp ? disp->team_[1].name_ : std::string() ) };
          const std::string & team = names[0];
          for ( const FigureBodies bodies : { FigureBodies::Mixed, FigureBodies::Male, FigureBodies::Female } )
          {
              opt.setFigureBodies3D( bodies );
              SceneContent c;
              SceneBuilder::addShow( c, show, M_holder, opt, nullptr, &humanCast() );
              QCOMPARE( c.figures.size(), size_t( rcss::rcg::MAX_PLAYER * 2 ) );
              for ( const FigureInstance & f : c.figures )
              {
                  const rcss::rcg::PlayerT & p = show.player_[f.player];
                  const FigureLook own = FigureLooks::of( p.side(), p.unum_, names[p.side() == rcss::rcg::LEFT ? 0 : 1], bodies, humanCast() );
                  QCOMPARE( f.look.body, own.body );
                  QCOMPARE( f.look.hair, own.hair );
                  QCOMPARE( f.look.skin, own.skin );
                  QCOMPARE( f.look.hair_color, own.hair_color );
              }
          }
          opt.setFigureBodies3D( FigureBodies::Mixed );
          // men and women
          SceneContent c;
          SceneBuilder::addShow( c, show, M_holder, opt, nullptr, &humanCast() );
          int women = 0;
          for ( const FigureInstance & f : c.figures ) if ( f.look.body == 1 ) ++women; // the Human pack's woman
          QVERIFY( women > 0 && women < int( c.figures.size() ) );

          // the player camera half sure of a number: the colors half way, the standard body and hair
          Perceptions seen;
          seen.active = true;
          seen.players.fill( PlayerSight::FULL );
          seen.players[3] = Perception{ true, 1.0f, 0.5f, true };
          c.clear();
          SceneBuilder::addShow( c, show, M_holder, opt, &seen, &humanCast() );
          const FigureLook own = FigureLooks::of( show.player_[3].side(), show.player_[3].unum_, team, FigureBodies::Mixed, humanCast() );
          const FigureLook plain = FigureLooks::standard( FigureBodies::Mixed, humanCast() );
          const FigureInstance & f = c.figures[3];
          QCOMPARE( f.look.body, plain.body );
          QCOMPARE( f.look.hair, plain.hair );
          QVERIFY( ( f.look.skin - ( own.skin + plain.skin ) * 0.5f ).length() < 1.0e-5f );
          QVERIFY( ( f.look.hair_color - ( own.hair_color + plain.hair_color ) * 0.5f ).length() < 1.0e-5f );
          QCOMPARE( c.figures[2].look.skin, FigureLooks::of( show.player_[2].side(), show.player_[2].unum_, team, FigureBodies::Mixed, humanCast() ).skin );
      }

    /*!
      the players' heights by their types (the reach, player_size +
      kickable_margin, of the default type's): the cylinder and its head,
      the direction marks, the picks, the label and the figure; of the
      default height where the player camera does not know the number for sure
    */
    void heights()
      {
          Options & opt = Options::instance();
          const rcss::rcg::ServerParamT & sp = M_holder.serverParam();
          rcss::rcg::PlayerTypeT type = M_holder.playerType( 0 );
          QVERIFY( near( SceneBuilder::heightRatio( type, sp ), 1.0f ) );
          type.kickable_margin_ = sp.kickable_margin_ + 0.1;
          const float ratio = float( ( sp.player_size_ + sp.kickable_margin_ + 0.1 ) / ( sp.player_size_ + sp.kickable_margin_ ) );
          QVERIFY( near( SceneBuilder::heightRatio( type, sp ), ratio ) );
          QVERIFY( ratio > 1.05f );
          type.kickable_margin_ = sp.kickable_margin_ - 0.1;
          QVERIFY( SceneBuilder::heightRatio( type, sp ) < 0.95f );
          rcss::rcg::PlayerTypeT broken = type;
          broken.player_size_ = 0.0;
          broken.kickable_margin_ = 0.0;
          QCOMPARE( SceneBuilder::heightRatio( broken, sp ), 1.0f );

          // a holder with a longer reach for the type 1
          DispHolder holder;
          holder.handleServerParam( sp );
          rcss::rcg::PlayerTypeT longer = M_holder.playerType( 0 );
          longer.id_ = 1;
          longer.kickable_margin_ = sp.kickable_margin_ + 0.1;
          holder.handlePlayerType( longer );
          rcss::rcg::ShowInfoT show = makeShow();
          show.player_[3].type_ = 1;

          SceneContent c;
          SceneBuilder::addShow( c, show, holder, opt );
          const float body = SceneBuilder::BODY_HEIGHT * ratio;
          const float head = body + SceneBuilder::HEAD_GAP + SceneBuilder::HEAD_RADIUS;
          // the cylinder from the ground to its height, the head on it
          QVERIFY( near( at( c.cylinders[3], QVector3D( 0, -0.5f, 0 ) ).y(), 0.0f ) );
          QVERIFY( near( at( c.cylinders[3], QVector3D( 0, 0.5f, 0 ) ).y(), body ) );
          QVERIFY( near( at( c.spheres[3] ).y(), head ) );
          QVERIFY( near( at( c.spheres[3], QVector3D( 0, 1, 0 ) ).y() - at( c.spheres[3] ).y(), SceneBuilder::HEAD_RADIUS ) );
          // the others of the default height
          QVERIFY( near( at( c.cylinders[2], QVector3D( 0, 0.5f, 0 ) ).y(), SceneBuilder::BODY_HEIGHT ) );
          QVERIFY( near( at( c.spheres[2] ).y(), SceneBuilder::HEAD_HEIGHT ) );
          // the direction marks: at the chest and the head
          QVERIFY( near( at( c.cones[6] ).y(), body * 0.7f ) );
          QVERIFY( near( at( c.cones[7] ).y(), head ) );
          QVERIFY( near( at( c.cones[4] ).y(), SceneBuilder::BODY_HEIGHT * 0.7f ) );
          // the picks
          QVERIFY( near( c.picks[3].body_height, body ) );
          QVERIFY( near( c.picks[3].head_height, head ) );
          QVERIFY( near( c.picks[2].body_height, SceneBuilder::BODY_HEIGHT ) );
          // the label above its head
          LabelAtlas atlas;
          SceneBuilder::addLabels( c, show, holder, opt, atlas,
                                   labelView( QVector3D( -10, 20, 80 ), QVector3D( -10, 0, -15 ) ) );
          const float y = -30.0f + 5.0f * 3;
          const auto label = std::find_if( c.labels.begin(), c.labels.end(), [y]( const Billboard3D & b ) {
              return std::fabs( b.center.x() + 10.0f ) < 1.0e-3f && std::fabs( b.center.z() - y ) < 1.0e-3f; } );
          QVERIFY( label != c.labels.end() );
          QVERIFY( near( label->center.y(), head + SceneBuilder::HEAD_RADIUS + SceneBuilder::LABEL_ABOVE ) );

          // a figure: a person's height times the ratio times the scale
          opt.setFigureScale3D( 1.2 );
          c.clear();
          SceneBuilder::addShow( c, show, holder, opt, nullptr, &humanCast() );
          QVERIFY( near( c.figures[3].height, SceneBuilder::PERSON_HEIGHT * ratio * 1.2f ) );
          QVERIFY( near( c.figures[2].height, SceneBuilder::PERSON_HEIGHT * 1.2f ) );
          // picked as tall: the head's top at the figure's top
          QVERIFY( near( c.picks[3].head_height + SceneBuilder::HEAD_RADIUS, c.figures[3].height ) );
          QVERIFY( c.picks[3].body_height < c.picks[3].head_height - SceneBuilder::HEAD_RADIUS );
          // and as wide: scaled with the figure
          const float wide = float( type.player_size_ ) * SceneBuilder::FIGURE_PICK_SCALE;
          QVERIFY( near( c.picks[3].radius, wide * ratio * 1.2f ) );
          QVERIFY( near( c.picks[2].radius, wide * 1.2f ) );
          opt.setFigureScale3D( Options::FIGURE_SCALE_3D );

          // the player camera not sure of the number: of the default height
          Perceptions seen;
          seen.active = true;
          seen.players.fill( PlayerSight::FULL );
          // (the heights between the default one and the type's, by how sure it is)
          seen.players[3] = Perception{ true, 1.0f, 0.5f, true }; // the team known, the number half
          const float half = 1.0f + ( ratio - 1.0f ) * 0.5f;
          c.clear();
          SceneBuilder::addShow( c, show, holder, opt, &seen );
          QVERIFY( near( at( c.cylinders[3], QVector3D( 0, 0.5f, 0 ) ).y(), SceneBuilder::BODY_HEIGHT * half ) );
          QVERIFY( near( c.picks[3].head_height, SceneBuilder::BODY_HEIGHT * half + SceneBuilder::HEAD_GAP + SceneBuilder::HEAD_RADIUS ) );
          c.clear();
          SceneBuilder::addShow( c, show, holder, opt, &seen, &humanCast() );
          QVERIFY( near( c.figures[3].height, SceneBuilder::PERSON_HEIGHT * half * float( opt.figureScale3D() ) ) );
          // not at all sure: of the default height
          seen.players[3] = Perception{ true, 1.0f, 0.0f, true };
          c.clear();
          SceneBuilder::addShow( c, show, holder, opt, &seen, &humanCast() );
          QVERIFY( near( c.figures[3].height, SceneBuilder::PERSON_HEIGHT * float( opt.figureScale3D() ) ) );
          // a felt one (gray, a cylinder even with figures): of the default height
          seen.players[3] = PlayerSight::FELT;
          c.clear();
          SceneBuilder::addShow( c, show, holder, opt, &seen, &humanCast() );
          QCOMPARE( c.cylinders.size(), size_t( 1 ) );
          QVERIFY( near( at( c.cylinders[0], QVector3D( 0, 0.5f, 0 ) ).y(), SceneBuilder::BODY_HEIGHT ) );
          // sure of the number: by its type
          seen.players[3] = PlayerSight::FULL;
          c.clear();
          SceneBuilder::addShow( c, show, holder, opt, &seen );
          QVERIFY( near( at( c.cylinders[3], QVector3D( 0, 0.5f, 0 ) ).y(), body ) );
      }

    /*!
      the height of the figures by the option, and the number above the
      top of a figure's head as above a cylinder's, shrinking toward it
    */
    void figureLabels()
      {
          Options & opt = Options::instance();
          rcss::rcg::ShowInfoT show = makeShow();
          const rcss::rcg::PlayerTypeT & type = M_holder.playerType( 0 );
          for ( const double scale : { 1.0, 1.2, 1.5 } )
          {
              opt.setFigureScale3D( scale );
              QCOMPARE( SceneBuilder::figureHeight( SceneBuilder::heightRatio( type, M_holder.serverParam() ), opt ), SceneBuilder::PERSON_HEIGHT * float( scale ) );

              SceneContent c;
              SceneBuilder::addShow( c, show, M_holder, opt, nullptr, &humanCast() );
              const float head_top = c.figures[0].height;
              QCOMPARE( head_top, SceneBuilder::PERSON_HEIGHT * float( scale ) );

              // far: full size, LABEL_ABOVE above the head, its text clear of it
              LabelAtlas atlas;
              SceneBuilder::addLabels( c, show, M_holder, opt, atlas,
                                       labelView( QVector3D( -10, 20, 60 ), QVector3D( -10, 0, -30 ) ) );
              const auto found = std::find_if( c.labels.begin(), c.labels.end(), []( const Billboard3D & b ) {
                  return std::fabs( b.center.x() + 10.0f ) < 1.0e-3f && std::fabs( b.center.z() + 30.0f ) < 1.0e-3f; } );
              QVERIFY( found != c.labels.end() );
              const Billboard3D * label = &*found;
              const Billboard3D far = *found; // kept: the list is made again below
              QVERIFY( near( label->center.y(), head_top + SceneBuilder::LABEL_ABOVE ) );
              QVERIFY( label->center.y() - textHeight( *label ) / 2.0f > head_top + 0.05f );

              // near: smaller, toward the top of the head, still above it
              c.labels.clear();
              SceneBuilder::addLabels( c, show, M_holder, opt, atlas,
                                       labelView( QVector3D( -10, head_top, -26 ), QVector3D( -10, head_top, -30 ) ) );
              const auto near_label = std::find_if( c.labels.begin(), c.labels.end(), []( const Billboard3D & b ) {
                  return std::fabs( b.center.x() + 10.0f ) < 1.0e-3f && std::fabs( b.center.z() + 30.0f ) < 1.0e-3f; } );
              QVERIFY( near_label != c.labels.end() );
              QVERIFY( textHeight( *near_label ) < textHeight( far ) - 1.0e-3f );
              QVERIFY( near_label->center.y() < far.center.y() );
              QVERIFY( near_label->center.y() - textHeight( *near_label ) / 2.0f > head_top );
          }
          opt.setFigureScale3D( Options::FIGURE_SCALE_3D );

          // a cylinder's number as before
          SceneContent c;
          SceneBuilder::addShow( c, show, M_holder, opt );
          LabelAtlas atlas;
          SceneBuilder::addLabels( c, show, M_holder, opt, atlas,
                                   labelView( QVector3D( -10, 20, 60 ), QVector3D( -10, 0, -30 ) ) );
          QVERIFY( std::any_of( c.labels.begin(), c.labels.end(), []( const Billboard3D & b ) {
              return near( b.center, QVector3D( -10, SceneBuilder::NUMBER_HEIGHT, -30 ) ); } ) );
          QVERIFY( near( SceneBuilder::NUMBER_HEIGHT, 2.4f ) );
      }

    void perceptions()
      {
          Options & opt = Options::instance();
          rcss::rcg::ShowInfoT show = makeShow();
          Perceptions seen;
          seen.active = true;
          seen.players.fill( PlayerSight::FULL );
          seen.players[1] = PlayerSight::UNSEEN;
          seen.players[2] = Perception{ true, 0.5f, 0.0f, false };
          seen.players[3] = PlayerSight::FELT;
          seen.ball = false;
          seen.hidden = 4;
          opt.setSelectedNumber( rcss::rcg::LEFT, 4 ); // the 4th (index 3): selected, but felt only

          SceneContent c;
          SceneBuilder::addShow( c, show, M_holder, opt, &seen );
          opt.unselectPlayer();
          const size_t n = rcss::rcg::MAX_PLAYER * 2 - 2; // without the unseen and the hidden
          QCOMPARE( c.cylinders.size(), n );
          QCOMPARE( c.spheres.size(), n );
          QCOMPARE( c.balls.size(), size_t( 0 ) ); // no ball
          QCOMPARE( c.discs.size(), n ); // nor its shadow
          QCOMPARE( c.cones.size(), ( n - 2 ) * 2 ); // not of the two without directions
          QCOMPARE( c.rings.size(), size_t( 0 ) ); // the selected one's team is not known
          QCOMPARE( c.picks.size(), n );
          for ( const PlayerPick & p : c.picks )
          {
              QVERIFY( p.side != rcss::rcg::LEFT || ( p.unum != 2 && p.unum != 5 ) ); // index 1 and the hidden index 4
          }

          // the colors: the team half known halfway to the gray, unknown the gray
          const rcss::rcg::PlayerTypeT & type = M_holder.playerType( 0 );
          const PlayerStyle full = playerStyle( show.player_[0], type, M_holder.serverParam(), opt );
          QCOMPARE( c.cylinders[0].color, rgba( full.body.color() ) );
          const PlayerStyle half = playerStyle( show.player_[2], type, M_holder.serverParam(), opt );
          QCOMPARE( c.cylinders[1].color, rgba( fadeTeam( half.body.color(), 0.5f ) ) );
          QCOMPARE( c.cylinders[2].color.toVector3D(), rgba( QColor( 0x9a, 0x9a, 0x9a ) ).toVector3D() );

          // a highlight when the team is known
          opt.setSelectedNumber( rcss::rcg::LEFT, 3 ); // index 2, half known
          c.clear();
          SceneBuilder::addShow( c, show, M_holder, opt, &seen );
          opt.unselectPlayer();
          QCOMPARE( c.rings.size(), size_t( 1 ) );

          // only its own hidden, everything else drawn (the sight is everything)
          Perceptions eyes;
          eyes.hidden = 0;
          c.clear();
          SceneBuilder::addShow( c, show, M_holder, opt, &eyes );
          QCOMPARE( c.cylinders.size(), size_t( rcss::rcg::MAX_PLAYER * 2 - 1 ) );
          QCOMPARE( c.spheres.size(), size_t( rcss::rcg::MAX_PLAYER * 2 - 1 ) );
          QCOMPARE( c.balls.size(), size_t( 1 ) ); // with the ball
      }

    /*!
      A goalie with the player camera's sight: in its goalie color as far as
      its number is known (rcssserver tells a goalie only with its number),
      in its team's field color without it; the body, the circle on the
      ground and a human figure alike
    */
    void perceivedGoalie()
      {
          Options & opt = Options::instance();
          rcss::rcg::ShowInfoT show = makeShow();
          show.player_[0].state_ |= rcss::rcg::GOALIE;
          const rcss::rcg::PlayerTypeT & type = M_holder.playerType( 0 );
          const QColor field = playerStyle( show.player_[1], type, M_holder.serverParam(), opt ).body.color();
          const QColor keeper = playerStyle( show.player_[0], type, M_holder.serverParam(), opt ).body.color();
          QVERIFY( field != keeper );
          Perceptions seen;
          seen.active = true;
          seen.players.fill( PlayerSight::FULL );
          for ( const float unum : { 1.0f, 0.5f, 0.0f } )
          {
              seen.players[0] = Perception{ true, 1.0f, unum, unum > 0.0f };
              SceneContent c;
              SceneBuilder::addShow( c, show, M_holder, opt, &seen );
              const QColor expected = playerStyle( show.player_[0], type, M_holder.serverParam(), opt, unum ).body.color();
              QCOMPARE( c.cylinders[0].color, rgba( expected ) );
              QCOMPARE( c.spheres[0].color, rgba( expected ) );
              QCOMPARE( c.discs[0].color.toVector3D(), rgba( expected ).toVector3D() );
              if ( unum == 1.0f ) QCOMPARE( c.cylinders[0].color, rgba( keeper ) );
              if ( unum == 0.0f ) QCOMPARE( c.cylinders[0].color.toVector3D(), rgba( field ).toVector3D() );
          }
          // a figure: its color too
          FigureCast cast;
          cast.bodies.push_back( FigureCast::Body() );
          seen.players[0] = Perception{ true, 1.0f, 0.0f, true };
          SceneContent c;
          SceneBuilder::addShow( c, show, M_holder, opt, &seen, &cast );
          QVERIFY( ! c.figures.empty() );
          QCOMPARE( c.figures[0].player, 0 );
          QCOMPARE( c.figures[0].color.toVector3D(), rgba( field ).toVector3D() );
          // without the player camera: the goalie color
          c.clear();
          SceneBuilder::addShow( c, show, M_holder, opt );
          QCOMPARE( c.cylinders[0].color, rgba( keeper ) );
      }

    /*!
      The labels with the player camera's sight: the number as likely as its
      player knows it, the cards of the players whose team it may know, and
      the bars of its own only
    */
    void perceptionLabels()
      {
          Options & opt = Options::instance();
          setShow( &Options::showStamina, &Options::toggleShowStamina, true );
          LabelAtlas atlas;
          SceneContent c;
          const SceneBuilder::LabelView view = labelView( QVector3D( 0, 40, 60 ), QVector3D( 0, 0, 0 ) );
          rcss::rcg::ShowInfoT show = makeShow();
          for ( auto & p : show.player_ ) p.side_ = 'n';
          for ( int i = 0; i < 4; ++i )
          {
              show.player_[i].side_ = 'l';
              show.player_[i].state_ |= rcss::rcg::YELLOW_CARD;
              show.player_[i].stamina_ = float( M_holder.serverParam().stamina_max_ );
          }
          Perceptions seen;
          seen.active = true;
          seen.players.fill( PlayerSight::FULL );
          seen.players[1] = Perception{ true, 1.0f, 0.5f, true };
          seen.players[2] = Perception{ true, 0.5f, 0.0f, false };
          seen.players[3] = PlayerSight::FELT;
          opt.setSelectedNumber( rcss::rcg::LEFT, 1 );
          SceneBuilder::addLabels( c, show, M_holder, opt, atlas, view, &seen );
          opt.unselectPlayer();
          setShow( &Options::showStamina, &Options::toggleShowStamina, false );

          const QRectF white = atlas.uv( LabelAtlas::WHITE_SLOT );
          std::vector< float > numbers; // the opacities of the numbers
          int cards = 0, bars = 0;
          for ( const Billboard3D & b : c.labels )
          {
              if ( b.uv != white ) numbers.push_back( b.color.w() );
              else if ( b.color.toVector3D() == QVector3D( 1, 1, 0 ) ) ++cards;
              else ++bars;
          }
          // the numbers of the first two: one known, one half
          std::sort( numbers.begin(), numbers.end() );
          QCOMPARE( numbers.size(), size_t( 2 ) );
          QCOMPARE( numbers[0], 0.5f );
          QCOMPARE( numbers[1], 1.0f );
          // the cards of the three whose team is known at all
          QCOMPARE( cards, 3 );
          // the bars (a background and a fill) of the selected one only
          QCOMPARE( bars, 2 );
      }

    void hidden()
      {
          Options & opt = Options::instance();
          SceneContent c;

          opt.toggleShowPlayer();
          SceneBuilder::addShow( c, makeShow(), M_holder, opt );
          opt.toggleShowPlayer();
          QCOMPARE( c.cylinders.size(), size_t( 0 ) );
          QCOMPARE( c.spheres.size(), size_t( 0 ) );
          QCOMPARE( c.balls.size(), size_t( 1 ) );
          QCOMPARE( c.picks.size(), size_t( 0 ) ); // not drawn: not aimed at

          c.clear();
          opt.toggleShowBall();
          SceneBuilder::addShow( c, makeShow(), M_holder, opt );
          opt.toggleShowBall();
          QCOMPARE( c.spheres.size(), size_t( rcss::rcg::MAX_PLAYER * 2 ) );
          QCOMPARE( c.balls.size(), size_t( 0 ) );
          QCOMPARE( c.discs.size(), size_t( rcss::rcg::MAX_PLAYER * 2 ) );

          // no players: no labels
          LabelAtlas atlas;
          c.clear();
          opt.toggleShowPlayer();
          SceneBuilder::addLabels( c, makeShow(), M_holder, opt, atlas, labelView( QVector3D( 0, 40, 60 ), QVector3D() ) );
          opt.toggleShowPlayer();
          QCOMPARE( c.labels.size(), size_t( 0 ) );
      }
};

QTEST_MAIN( TestSceneBuilder )
#include "test_scene_builder.moc"
