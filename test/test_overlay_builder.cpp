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

#include "disp_holder.h"
#include "options.h"
#include "overlay_builder.h"

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
          Options::instance().unselectPlayer();
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
