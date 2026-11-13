// -*-c++-*-

/*!
  \file test_overlay_builder.cpp
  \brief unit test of OverlayBuilder, the overlays on the ground of the 3D view
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

#include "camera_3d.h"
#include "disp_holder.h"
#include "options.h"
#include "overlay_builder.h"
#include "overlay_math.h"

#include <rcss/rcg/types.h>

#include <cmath>

namespace {

constexpr double DEG2RAD = M_PI / 180.0;

//! one connected player of the left team at (x, y), the others not in the show
rcss::rcg::ShowInfoT
makeShow( const float x,
          const float y,
          const float body,
          const float neck = rcss::rcg::SHOWINFO_SCALE2F,
          const float view_width = rcss::rcg::SHOWINFO_SCALE2F )
{
    rcss::rcg::ShowInfoT show;
    rcss::rcg::PlayerT & p = show.player_[0];
    p.side_ = 'l';
    p.unum_ = 1;
    p.type_ = 0;
    p.state_ = rcss::rcg::STAND;
    p.x_ = x;
    p.y_ = y;
    p.body_ = body;
    p.neck_ = neck;
    p.view_width_ = view_width;
    return show;
}

//! the field point of a vertex
QPointF
field( const ColorVertex3D & v )
{
    return QPointF( v.position.x(), v.position.z() );
}

bool
near( const QPointF & a,
      const QPointF & b,
      const double eps = 1.0e-4 )
{
    return std::hypot( a.x() - b.x(), a.y() - b.y() ) < eps;
}

QVector4D
rgba( const QColor & c,
      const float opacity = 1.0f )
{
    return QVector4D( float( c.redF() ), float( c.greenF() ), float( c.blueF() ), opacity );
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

}

class TestOverlayBuilder
    : public QObject {
    Q_OBJECT

private:
    DispHolder M_holder;

    double reach() const
      {
          const rcss::rcg::PlayerTypeT & type = M_holder.playerType( 0 );
          return type.player_size_ + type.kickable_margin_ + M_holder.serverParam().ball_size_;
      }

private slots:

    void init()
      {
          // each test from the same options, whatever the one before left
          Options::instance().unselectPlayer();
          setShow( &Options::showPlayer, &Options::toggleShowPlayer, true );
          setShow( &Options::showBall, &Options::toggleShowBall, true );
          setShow( &Options::showViewArea, &Options::toggleShowViewArea, false );
          setShow( &Options::showFocusPoint, &Options::toggleShowFocusPoint, false );
          setShow( &Options::showCatchArea, &Options::toggleShowCatchArea, false );
          setShow( &Options::showTackleArea, &Options::toggleShowTackleArea, false );
          setShow( &Options::showKickAccelArea, &Options::toggleShowKickAccelArea, false );
          setShow( &Options::showPointto, &Options::toggleShowPointto, false );
          setShow( &Options::showOffsideLine, &Options::toggleShowOffsideLine, false );
      }

    void line()
      {
          std::vector< ColorVertex3D > list;
          OverlayBuilder::addLine( list, QPointF( 1, 2 ), QPointF( 4, 6 ), Qt::red );
          QCOMPARE( list.size(), size_t( 2 ) );
          QCOMPARE( list[0].position, QVector3D( 1, OverlayBuilder::LINE_Y, 2 ) );
          QCOMPARE( list[1].position, QVector3D( 4, OverlayBuilder::LINE_Y, 6 ) );
          QCOMPARE( list[0].color, QVector4D( 1, 0, 0, 1 ) );
          // the distances along it, for the dashes
          QCOMPARE( list[0].distance, 0.0f );
          QCOMPARE( list[1].distance, 5.0f );
      }

    void arc()
      {
          std::vector< ColorVertex3D > list;
          OverlayBuilder::addArc( list, QPointF( 10, -5 ), 2.0, Qt::blue );
          QCOMPARE( list.size(), size_t( OverlayBuilder::CIRCLE_SEGMENTS * 2 ) );
          // on the circle, joined end to end, the distance going on along it
          for ( std::size_t i = 0; i < list.size(); ++i )
          {
              QVERIFY( std::fabs( std::hypot( list[i].position.x() - 10.0, list[i].position.z() + 5.0 ) - 2.0 ) < 1.0e-5 );
              if ( i >= 2 && i % 2 == 0 )
              {
                  QCOMPARE( list[i].position, list[i - 1].position );
                  QVERIFY( std::fabs( list[i].distance - list[i - 1].distance ) < 1.0e-5f );
              }
          }
          QVERIFY( std::fabs( list.back().distance - 2.0 * 2.0 * M_PI ) < 1.0e-4 );

          // a quarter, from +x toward +y (clockwise on the 2D screen)
          list.clear();
          OverlayBuilder::addArc( list, QPointF( 0, 0 ), 1.0, Qt::blue, 0.0, 90.0 );
          QCOMPARE( list.size(), size_t( OverlayBuilder::CIRCLE_SEGMENTS / 4 * 2 ) );
          QVERIFY( near( field( list.front() ), QPointF( 1, 0 ) ) );
          QVERIFY( near( field( list.back() ), QPointF( 0, 1 ) ) );
      }

    void directions()
      {
          const Options & opt = Options::instance();
          SceneContent c;
          OverlayBuilder::addDirections( c, makeShow( -10, 5, 90.0f, -90.0f ), M_holder, opt );
          QCOMPARE( c.lines.size(), size_t( 4 ) );
          // the body: from the center to the kickable area's edge, the player pen's color
          QVERIFY( near( field( c.lines[0] ), QPointF( -10, 5 ) ) );
          QVERIFY( near( field( c.lines[1] ), QPointF( -10, 5 + reach() ) ) );
          QCOMPARE( c.lines[0].color, rgba( opt.playerPen().color() ) );
          // the face: body + neck, the neck's color
          QVERIFY( near( field( c.lines[3] ), QPointF( -10 + reach(), 5 ) ) );
          QCOMPARE( c.lines[2].color, rgba( opt.neckPen().color() ) );

          // without the neck, the body only
          c.clear();
          OverlayBuilder::addDirections( c, makeShow( 0, 0, 0.0f ), M_holder, opt );
          QCOMPARE( c.lines.size(), size_t( 2 ) );

          // not connected: nothing
          c.clear();
          rcss::rcg::ShowInfoT show = makeShow( 0, 0, 0.0f, 0.0f );
          show.player_[0].state_ = 0;
          OverlayBuilder::addDirections( c, show, M_holder, opt );
          QCOMPARE( c.lines.size(), size_t( 0 ) );
      }

    void directionsWithViewAreas()
      {
          // the view area shows the face: the body line only
          Options & opt = Options::instance();
          SceneContent c;
          opt.toggleShowViewArea();
          OverlayBuilder::addDirections( c, makeShow( 0, 0, 0.0f, 30.0f, 90.0f ), M_holder, opt );
          opt.toggleShowViewArea();
          QCOMPARE( c.lines.size(), size_t( 2 ) );
          QCOMPARE( c.lines[0].color, rgba( opt.playerPen().color() ) );

          // no players: nothing
          c.clear();
          opt.toggleShowPlayer();
          OverlayBuilder::addDirections( c, makeShow( 0, 0, 0.0f, 30.0f ), M_holder, opt );
          opt.toggleShowPlayer();
          QCOMPARE( c.lines.size(), size_t( 0 ) );
      }

    void viewArea()
      {
          Options & opt = Options::instance();
          const double visible = M_holder.serverParam().visible_distance_;
          const rcss::rcg::ShowInfoT show = makeShow( 3, 4, 20.0f, 10.0f, 90.0f ); // the face at 30 degrees

          // not asked: nothing
          SceneContent c;
          OverlayBuilder::addViewAreas( c, show, M_holder, opt );
          QVERIFY( c.sectors.empty() && c.lines.empty() );

          opt.toggleShowViewArea();
          OverlayBuilder::addViewAreas( c, show, M_holder, opt );
          opt.toggleShowViewArea();
          // a fan of triangles from the player to visible_distance, 30 +- 45 degrees, translucent
          QCOMPARE( c.sectors.size(), size_t( OverlayBuilder::SECTOR_SEGMENTS * 3 ) );
          QVERIFY( c.lines.empty() );
          double lo = 1000.0, hi = -1000.0;
          for ( std::size_t i = 0; i < c.sectors.size(); i += 3 )
          {
              QVERIFY( near( field( c.sectors[i] ), QPointF( 3, 4 ) ) );
              for ( std::size_t k = 1; k < 3; ++k )
              {
                  const QPointF d = field( c.sectors[i + k] ) - QPointF( 3, 4 );
                  QVERIFY( std::fabs( std::hypot( d.x(), d.y() ) - visible ) < 1.0e-4 );
                  const double a = std::atan2( d.y(), d.x() ) / DEG2RAD;
                  lo = std::min( lo, a );
                  hi = std::max( hi, a );
              }
          }
          QVERIFY( std::fabs( lo - ( 30.0 - 45.0 ) ) < 1.0e-3 );
          QVERIFY( std::fabs( hi - ( 30.0 + 45.0 ) ) < 1.0e-3 );
          QCOMPARE( c.sectors[0].position.y(), OverlayBuilder::SECTOR_Y );
          QCOMPARE( c.sectors[0].color, rgba( opt.viewAreaPen().color(), OverlayBuilder::SECTOR_OPACITY ) );

          // without the view width: nothing
          c.clear();
          opt.toggleShowViewArea();
          OverlayBuilder::addViewAreas( c, makeShow( 3, 4, 20.0f, 10.0f ), M_holder, opt );
          opt.toggleShowViewArea();
          QVERIFY( c.sectors.empty() );
      }

    void overlaysOff()
      {
          // none of them asked: nothing (the ball's path is on by default: off here)
          Options & opt = Options::instance();
          const int vel = opt.ballVelCycle();
          opt.setBallVelCycle( 0 );
          rcss::rcg::ShowInfoT show = makeShow( 0, 0, 0.0f, 0.0f, 90.0f );
          show.player_[0].state_ |= rcss::rcg::GOALIE;
          show.player_[0].point_x_ = 5.0f;
          show.player_[0].point_y_ = 5.0f;
          SceneContent c;
          OverlayBuilder::addOverlays( c, show, M_holder, opt );
          opt.setBallVelCycle( vel );
          QVERIFY( c.lines.empty() && c.dotted_lines.empty() && c.points.empty() && c.texts.empty() );
      }

    void focusPoint()
      {
          Options & opt = Options::instance();
          rcss::rcg::ShowInfoT show = makeShow( 0, 0, 0.0f, 0.0f, 90.0f );
          show.player_[0].focus_dist_ = 5.0f;
          SceneContent c;
          opt.toggleShowFocusPoint();
          OverlayBuilder::addOverlays( c, show, M_holder, opt ); // not selected
          QVERIFY( c.lines.empty() );
          opt.setSelectedNumber( rcss::rcg::LEFT, 1 );
          OverlayBuilder::addOverlays( c, show, M_holder, opt );
          opt.toggleShowFocusPoint();
          // a line to it, and a circle around it
          QCOMPARE( c.lines.size(), size_t( 2 + OverlayBuilder::CIRCLE_SEGMENTS * 2 ) );
          QVERIFY( near( field( c.lines[1] ), QPointF( 5, 0 ) ) );
          QCOMPARE( c.lines[0].color, rgba( opt.focusPointPen().color() ) );
      }

    void catchArea()
      {
          Options & opt = Options::instance();
          rcss::rcg::ShowInfoT show = makeShow( -50, 0, 0.0f );
          SceneContent c;
          opt.toggleShowCatchArea();
          OverlayBuilder::addOverlays( c, show, M_holder, opt ); // not a goalie
          QVERIFY( c.lines.empty() );
          show.player_[0].state_ |= rcss::rcg::GOALIE;
          OverlayBuilder::addOverlays( c, show, M_holder, opt );
          opt.toggleShowCatchArea();
          // the catchable circle, in the goalie's color
          const double r = OverlayMath::catchArea( M_holder.serverParam(), M_holder.playerType( 0 ) ).catchable_;
          QCOMPARE( c.lines.size(), size_t( OverlayBuilder::CIRCLE_SEGMENTS * 2 ) );
          QVERIFY( near( field( c.lines[0] ), QPointF( -50 + r, 0 ) ) );
          QCOMPARE( c.lines[0].color, rgba( opt.leftGoaliePen().color() ) );
          QVERIFY( c.dotted_lines.empty() ); // the default type has no stretch
      }

    /*
      From rcssserver 17.0.0, the goalie cannot catch behind it (catch
      angles -90 to +90): the arc in front, closed by a side half the catch
      box's width behind; the outlines of a stretched type dotted, their
      dashes going on from the arc to the side
    */
    void catchAreaLimited()
      {
          Options & opt = Options::instance();
          rcss::rcg::ServerParamT sp;
          sp.max_catch_angle_ = 90.0;
          sp.min_catch_angle_ = -90.0;
          DispHolder holder;
          holder.handleServerParam( sp );
          rcss::rcg::PlayerTypeT type;
          type.id_ = 0;
          type.catchable_area_l_stretch_ = 1.3;
          holder.handlePlayerType( type );

          rcss::rcg::ShowInfoT show = makeShow( -50, 0, 0.0f ); // facing +x
          show.player_[0].state_ |= rcss::rcg::GOALIE;
          SceneContent c;
          opt.toggleShowCatchArea();
          OverlayBuilder::addOverlays( c, show, holder, opt );
          opt.toggleShowCatchArea();

          const double half_width = sp.catchable_area_w_ * 0.5;
          auto check = [&]( const std::vector< ColorVertex3D > & list,
                            const double length ) {
              QVERIFY( list.size() >= 8 );
              // closed: the side ends where the arc starts
              QVERIFY( near( field( list.back() ), field( list.front() ) ) );
              // nothing further behind than half the width; the side there
              const ColorVertex3D & a = list[list.size() - 2];
              const ColorVertex3D & b = list.back();
              QVERIFY( std::fabs( a.position.x() - ( -50 - half_width ) ) < 1.0e-4 );
              QVERIFY( std::fabs( b.position.x() - ( -50 - half_width ) ) < 1.0e-4 );
              QVERIFY( std::fabs( a.position.z() - length ) < 1.0e-4 );
              QVERIFY( std::fabs( b.position.z() + length ) < 1.0e-4 );
              for ( const ColorVertex3D & v : list )
              {
                  QVERIFY( v.position.x() > -50 - half_width - 1.0e-4 );
              }
              // the dashes go on from the arc
              QVERIFY( std::fabs( a.distance - list[list.size() - 3].distance ) < 1.0e-4f );
              QVERIFY( std::fabs( b.distance - a.distance - 2.0 * length ) < 1.0e-4 );
          };
          const OverlayMath::CatchArea area = OverlayMath::catchArea( sp, type );
          check( c.lines, area.catchable_length_ );
          QCOMPARE( c.lines[0].color, rgba( opt.leftGoaliePen().color() ) );

          // the largest and the smallest, one after the other: the second
          // starts its dashes again
          size_t second = 1;
          while ( second < c.dotted_lines.size()
                  && c.dotted_lines[second].distance != 0.0f )
          {
              ++second;
          }
          QVERIFY( second < c.dotted_lines.size() );
          check( std::vector< ColorVertex3D >( c.dotted_lines.begin(), c.dotted_lines.begin() + second ),
                 area.max_length_ );
          check( std::vector< ColorVertex3D >( c.dotted_lines.begin() + second, c.dotted_lines.end() ),
                 area.min_length_ );
          QCOMPARE( c.dotted_lines[0].color, rgba( opt.leftGoalieStretchPen().color() ) );
      }

    void tackleArea()
      {
          Options & opt = Options::instance();
          const rcss::rcg::ServerParamT & sp = M_holder.serverParam();
          rcss::rcg::ShowInfoT show = makeShow( 0, 0, 90.0f ); // facing +y
          show.ball_.x_ = 0.0f;
          show.ball_.y_ = 1.0f;
          SceneContent c;
          opt.toggleShowTackleArea();
          OverlayBuilder::addOverlays( c, show, M_holder, opt );
          // the box turned with the body: 4 sides, and the chances at the player
          QCOMPARE( c.lines.size(), size_t( 8 ) );
          QVERIFY( near( field( c.lines[2] ), QPointF( sp.tackle_width_, sp.tackle_dist_ ) ) );
          QVERIFY( near( field( c.lines[4] ), QPointF( - sp.tackle_width_, sp.tackle_dist_ ) ) );
          QCOMPARE( c.texts.size(), size_t( 1 ) );
          QVERIFY( c.texts[0].text.startsWith( "T=" ) );
          QCOMPARE( c.texts[0].at, QVector3D( 0, OverlayBuilder::LINE_Y, 0 ) );
          QCOMPARE( c.texts[0].color, opt.tacklePen().color() );

          // out of reach: nothing
          c.clear();
          show.ball_.y_ = 5.0f;
          OverlayBuilder::addOverlays( c, show, M_holder, opt );
          opt.toggleShowTackleArea();
          QVERIFY( c.lines.empty() && c.texts.empty() );
      }

    void kickAccelArea()
      {
          Options & opt = Options::instance();
          rcss::rcg::ShowInfoT show = makeShow( 0, 0, 0.0f );
          show.ball_.x_ = 0.6f;
          show.ball_.vx_ = 1.5f;
          show.ball_.vy_ = 0.0f;
          SceneContent c;
          opt.toggleShowKickAccelArea();
          OverlayBuilder::addOverlays( c, show, M_holder, opt ); // not selected
          QVERIFY( c.lines.empty() );
          opt.setSelectedNumber( rcss::rcg::LEFT, 1 );
          OverlayBuilder::addOverlays( c, show, M_holder, opt );
          opt.toggleShowKickAccelArea();
          // the line to the next place, the arcs, and the largest acceleration at the next place
          QVERIFY( c.lines.size() > 2 + 8 );
          QCOMPARE( c.texts.size(), size_t( 1 ) );
          QVERIFY( near( field( c.lines[0] ), QPointF( 0.6, 0 ) ) );
          QVERIFY( near( field( c.lines[1] ), QPointF( 2.1, 0 ) ) );
          QVERIFY( c.lines.size() > 2 + 8 );
          QCOMPARE( c.texts.size(), size_t( 1 ) );
          QVERIFY( c.texts[0].text.startsWith( "MaxAccel=" ) );
          QVERIFY( near( QPointF( c.texts[0].at.x(), c.texts[0].at.z() ), QPointF( 2.1, 0 ) ) );
      }

    void pointtoAndOffside()
      {
          Options & opt = Options::instance();
          rcss::rcg::ShowInfoT show = makeShow( 0, 0, 0.0f );
          show.player_[0].point_x_ = 10.0f;
          show.player_[0].point_y_ = -5.0f;
          SceneContent c;
          opt.toggleShowPointto();
          OverlayBuilder::addOverlays( c, show, M_holder, opt );
          opt.toggleShowPointto();
          // a line to the point and a cross there
          QCOMPARE( c.lines.size(), size_t( 6 ) );
          QVERIFY( near( field( c.lines[1] ), QPointF( 10, -5 ) ) );
          QVERIFY( near( field( c.lines[2] ), QPointF( 10 - OverlayBuilder::POINTTO_CROSS, -5 - OverlayBuilder::POINTTO_CROSS ) ) );

          // the offside lines across the pitch
          c.clear();
          opt.toggleShowOffsideLine();
          OverlayBuilder::addOverlays( c, show, M_holder, opt );
          opt.toggleShowOffsideLine();
          const OverlayMath::OffsideLines lines = OverlayMath::offsideLines( show );
          QCOMPARE( c.lines.size(), size_t( 4 ) );
          QVERIFY( near( field( c.lines[0] ), QPointF( lines.left_, -Options::PITCH_HALF_WIDTH ) ) );
          QVERIFY( near( field( c.lines[1] ), QPointF( lines.left_, Options::PITCH_HALF_WIDTH ) ) );
          QCOMPARE( c.lines[2].color, rgba( opt.rightTeamPen().color() ) );
      }

    void ballPath()
      {
          Options & opt = Options::instance();
          const int vel = opt.ballVelCycle();
          opt.setBallVelCycle( 5 );
          rcss::rcg::ShowInfoT show = makeShow( 0, 0, 0.0f );
          show.ball_.x_ = 1.0f;
          show.ball_.vx_ = 2.0f;
          show.ball_.vy_ = 0.0f;
          SceneContent c;
          OverlayBuilder::addOverlays( c, show, M_holder, opt );
          opt.setBallVelCycle( vel );
          // a dot after each cycle, and a line to the last
          QCOMPARE( c.points.size(), size_t( 5 ) );
          QVERIFY( near( field( c.points[0] ), QPointF( 3, 0 ) ) );
          QCOMPARE( c.lines.size(), size_t( 2 ) );
          QVERIFY( near( field( c.lines[1] ), field( c.points.back() ) ) );
          QCOMPARE( c.points[0].color, rgba( opt.ballVelPen().color() ) );
      }

    void placeTexts()
      {
          // a camera straight above the origin, without perspective: 1 m = 10 pixels
          CameraPose camera;
          camera.position = QVector3D( 0, 100, 0 );
          camera.target = QVector3D( 0, 0, 0 );
          camera.up = QVector3D( 0, 0, -1 );
          camera.ortho_half_height = 20.0f;
          const QSize size( 400, 400 );
          const QMatrix4x4 m = camera.projection( 1.0f ) * camera.view();
          const QFontMetrics metrics( QFont( "Sans Serif", 9 ) );

          std::vector< OverlayText > texts = {
              { QVector3D( 0, 0, 0 ), "a", Qt::red },
              { QVector3D( 0.1f, 0, 0 ), "b", Qt::blue }, // covering the first: a line down
              { QVector3D( -10, 0, -10 ), "c", Qt::green },
              { QVector3D( 30, 0, 0 ), "off", Qt::green }, // off the screen
          };
          const std::vector< OverlayBuilder::PlacedText > placed = OverlayBuilder::placeTexts( texts, m, size, metrics );
          QCOMPARE( placed.size(), size_t( 3 ) );
          // 10 pixels right of and 2 below the point, at the baseline
          QCOMPARE( placed[0].baseline, QPoint( 200 + 10, 200 + 2 + metrics.ascent() ) );
          QCOMPARE( placed[0].color, QColor( Qt::red ) );
          QCOMPARE( placed[1].baseline, QPoint( 201 + 10, 200 + 2 + metrics.ascent() + metrics.height() ) );
          QCOMPARE( placed[2].baseline, QPoint( 100 + 10, 100 + 2 + metrics.ascent() ) );

          // right behind a perspective camera: left out (projected, it would
          // be at the center of the screen)
          camera.ortho_half_height = 0.0f;
          camera.position = QVector3D( 0, 10, 10 );
          camera.target = QVector3D( 0, 0, 20 );
          camera.up = QVector3D( 0, 1, 0 );
          const QVector3D behind = camera.position - 5.0f * ( camera.target - camera.position ).normalized();
          const QVector3D ahead = camera.position + 5.0f * ( camera.target - camera.position ).normalized();
          const QMatrix4x4 perspective = camera.projection( 1.0f ) * camera.view();
          QVERIFY( OverlayBuilder::placeTexts( { { behind, "x", Qt::red } }, perspective, size, metrics ).empty() );
          QCOMPARE( OverlayBuilder::placeTexts( { { ahead, "x", Qt::red } }, perspective, size, metrics ).size(), size_t( 1 ) );
      }

    void largeViewArea()
      {
          // the selected player: the lines of its large view instead
          Options & opt = Options::instance();
          opt.setSelectedNumber( rcss::rcg::LEFT, 1 );
          SceneContent c;
          opt.toggleShowViewArea();
          OverlayBuilder::addViewAreas( c, makeShow( 0, 0, 0.0f, 0.0f, 120.0f ), M_holder, opt );
          opt.toggleShowViewArea();
          QVERIFY( c.sectors.empty() );

          const int arc = int( std::ceil( OverlayBuilder::CIRCLE_SEGMENTS * 120.0 / 360.0 ) );
          QCOMPARE( c.lines.size(), size_t( ( 3 * arc + 2 + OverlayBuilder::CIRCLE_SEGMENTS ) * 2 ) );
          QCOMPARE( c.lines[0].color, rgba( opt.largeViewAreaPen().color() ) );
          QCOMPARE( c.lines[0].position.y(), OverlayBuilder::LARGE_VIEW_Y );
          // the edges reach the farthest arc at -60 and +60 degrees
          auto has = [&]( const QPointF & p ) {
              return std::any_of( c.lines.begin(), c.lines.end(), [&]( const ColorVertex3D & v ) { return near( field( v ), p, 1.0e-3 ); } );
          };
          QVERIFY( has( QPointF( 60.0 * std::cos( -60.0 * DEG2RAD ), 60.0 * std::sin( -60.0 * DEG2RAD ) ) ) );
          QVERIFY( has( QPointF( 60.0 * std::cos( 60.0 * DEG2RAD ), 60.0 * std::sin( 60.0 * DEG2RAD ) ) ) );
          QVERIFY( has( QPointF( 20.0, 0.0 ) ) );
          QVERIFY( has( QPointF( 40.0, 0.0 ) ) );
          // the circle that is felt
          QVERIFY( has( QPointF( -M_holder.serverParam().visible_distance_, 0.0 ) ) );
      }
};

QTEST_MAIN( TestOverlayBuilder )
#include "test_overlay_builder.moc"
