// -*-c++-*-

/*!
  \file test_guides_3d.cpp
  \brief unit test of the compass and the minimap over the 3D view
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

#include "ground_texture.h"
#include "guides_3d.h"
#include "options.h"

#include <rcss/rcg/types.h>

#include <QPainter>

#include <cmath>

namespace {

int
diff( const QColor & a,
      const QColor & b )
{
    return std::abs( a.red() - b.red() ) + std::abs( a.green() - b.green() ) + std::abs( a.blue() - b.blue() );
}

}

/*!
  \brief unit test of Guides3D
*/
class TestGuides3D
    : public QObject {
    Q_OBJECT

private slots:

    //! never with the top camera; with the player camera only, or with all
    void shown()
      {
          for ( const CameraModeInfo & m : cameraModes() )
          {
              QVERIFY( ! Guides3D::shown( GuideMode::Off, m.mode ) );
              QCOMPARE( Guides3D::shown( GuideMode::Player, m.mode ), m.mode == CameraMode::Player );
              QCOMPARE( Guides3D::shown( GuideMode::All, m.mode ), m.mode != CameraMode::Top );
          }
          // the names in the options
          GuideMode mode = GuideMode::Off;
          QVERIFY( guideModeFromName( "player", &mode ) );
          QCOMPARE( mode, GuideMode::Player );
          QVERIFY( ! guideModeFromName( "some", &mode ) );
          QCOMPARE( mode, GuideMode::Player );
          QCOMPARE( guideModeName( GuideMode::All ), QString( "all" ) );
          QCOMPARE( Options::instance().compass3D(), GuideMode::Off );
          QCOMPARE( Options::instance().minimap3D(), GuideMode::Off );
      }

    //! where the camera is and looks, and how wide across
    void guide()
      {
          CameraPose pose;
          pose.position = QVector3D( 10.0f, 30.0f, 60.0f );
          pose.target = QVector3D( 10.0f, 0.0f, 0.0f );
          pose.fov_y = 35.0f;
          const float aspect = 16.0f / 9.0f;
          const Guides3D::CameraGuide g = Guides3D::guideOf( pose, aspect );
          QCOMPARE( g.x, 10.0f );
          QCOMPARE( g.y, 60.0f );
          QVERIFY( std::fabs( g.direction - ( -90.0f ) ) < 1.0e-4f ); // toward -y of the field
          const float fov = 2.0f * std::atan( std::tan( 17.5f * float( M_PI ) / 180.0f ) * aspect ) * 180.0f / float( M_PI );
          QVERIFY( std::fabs( g.fov - fov ) < 1.0e-3f );

          QCOMPARE( Guides3D::bearing( QPointF( 0, 0 ), QPointF( 0, 10 ) ), 90.0f );
          QCOMPARE( Guides3D::compassOffset( 90.0f, 0.0f ), 0.5f );
          QCOMPARE( Guides3D::compassOffset( -90.0f, 0.0f ), -0.5f );
          // the short way round
          QVERIFY( std::fabs( Guides3D::compassOffset( 170.0f, -170.0f ) - ( -20.0f / 180.0f ) ) < 1.0e-5f );
      }

    //! the compass at the top in the middle, the minimap at the bottom right
    void rects()
      {
          QCOMPARE( Guides3D::compassRect( 1280 ), QRectF( 400, 6, 480, 22 ) );
          QCOMPARE( Guides3D::compassRect( 600 ).width(), 360.0 );
          double scale = 0.0;
          const QRectF r = Guides3D::minimapRect( 1280, 720, &scale );
          QCOMPARE( r.width(), 300.0 );
          QCOMPARE( r.height(), std::round( 300.0 * GroundTexture::width() / GroundTexture::length() ) );
          QCOMPARE( r.right(), 1280.0 - 8.0 );
          QCOMPARE( r.bottom(), 720.0 - 8.0 );
          QCOMPARE( scale, 300.0 / GroundTexture::length() );
          QCOMPARE( Guides3D::minimapPoint( r, scale, QPointF( 0, 0 ) ), r.center() );
          QCOMPARE( Guides3D::minimapPoint( r, scale, QPointF( GroundTexture::length() / 2, 0 ) ).x(), r.right() );
          // small views: at least 140 wide
          QCOMPARE( Guides3D::minimapRect( 400, 300, &scale ).width(), 140.0 );
      }

    /*!
      The compass: the goal straight ahead at the middle, the one behind at
      an end, the ball to the right when it is clockwise; the marks in the
      team and ball colors
    */
    void compass()
      {
          QImage image( 640, 40, QImage::Format_ARGB32 );
          image.fill( Qt::black );
          const Options & opt = Options::instance();
          const QRectF r = Guides3D::compassRect( image.width() );
          const Guides3D::CameraGuide g{ 0.0f, 0.0f, 0.0f, 60.0f }; // at the center, toward the right goal
          const QPointF ball( 0.0, 10.0 ); // 90 degrees clockwise
          Guides3D::CompassMarks marks;
          {
              QPainter painter( &image );
              marks = Guides3D::drawCompass( painter, r, g, &ball, opt );
          }
          QCOMPARE( marks.right, r.center().x() );
          QVERIFY( std::fabs( marks.left - r.left() ) < 1.0e-6 || std::fabs( marks.left - r.right() ) < 1.0e-6 );
          QCOMPARE( marks.ball, r.center().x() + r.width() / 4.0 );
          // the colors, beside the letter
          const int middle = int( r.center().y() );
          QVERIFY( diff( image.pixelColor( int( marks.right ) - 5, middle ), opt.rightTeamBrush().color() ) < 30 );
          QVERIFY( diff( image.pixelColor( int( marks.ball ), middle ), opt.ballBrush().color() ) < 30 );
          // without the ball: no mark
          QImage none( 640, 40, QImage::Format_ARGB32 );
          none.fill( Qt::black );
          {
              QPainter painter( &none );
              marks = Guides3D::drawCompass( painter, r, g, nullptr, opt );
          }
          QVERIFY( marks.ball < 0.0 );
          QVERIFY( diff( none.pixelColor( int( r.center().x() + r.width() / 4.0 ), middle ), opt.ballBrush().color() ) > 60 );
      }

    /*!
      The minimap: the pitch, the players in their colors, the selected one
      ringed, the ball, the camera, and what it sees brighter
    */
    void minimap()
      {
          QImage image( 1280, 720, QImage::Format_ARGB32 );
          image.fill( Qt::black );
          Options & opt = Options::instance();
          double scale = 0.0;
          const QRectF r = Guides3D::minimapRect( image.width(), image.height(), &scale );
          QImage field( int( r.width() ), int( r.height() ), QImage::Format_ARGB32 );
          field.fill( QColor( 0, 100, 0 ) );

          rcss::rcg::ShowInfoT show;
          rcss::rcg::PlayerT & p = show.player_[0];
          p.side_ = 'l';
          p.unum_ = 1;
          p.state_ = rcss::rcg::STAND;
          p.x_ = -20.0f;
          p.y_ = -20.0f;
          rcss::rcg::PlayerT & q = show.player_[11];
          q.side_ = 'r';
          q.unum_ = 1;
          q.state_ = rcss::rcg::STAND | rcss::rcg::GOALIE;
          q.x_ = 40.0f;
          q.y_ = 20.0f;
          show.ball_.x_ = 10.0f;
          show.ball_.y_ = -25.0f;
          const Guides3D::CameraGuide g{ 0.0f, 30.0f, -90.0f, 60.0f }; // from the near touch line, toward -y
          // the same without a selected player, to see the ring
          QImage unselected = image;
          {
              QPainter painter( &unselected );
              Guides3D::drawMinimap( painter, r, scale, g, &show, field, opt );
          }
          opt.setSelectedNumber( rcss::rcg::LEFT, 1 );
          {
              QPainter painter( &image );
              Guides3D::drawMinimap( painter, r, scale, g, &show, field, opt );
          }
          opt.unselectPlayer();

          const auto px = [&]( const double x, const double y ) {
              return image.pixelColor( Guides3D::minimapPoint( r, scale, QPointF( x, y ) ).toPoint() );
          };
          QVERIFY( diff( px( -20, -20 ), opt.leftTeamBrush().color() ) < 40 );
          QVERIFY( diff( px( 40, 20 ), opt.rightGoalieBrush().color() ) < 40 );
          QVERIFY( diff( px( 10, -25 ), opt.ballBrush().color() ) < 40 );
          QCOMPARE( px( 0, 30 ), QColor( Qt::white ) ); // the camera
          // the ring around the selected player: lighter around it than without the selection
          const QPointF sel = Guides3D::minimapPoint( r, scale, QPointF( -20, -20 ) );
          const double radius = std::max( 2.5, scale * 1.2 ) + 2.0;
          int with_ring = 0, without = 0;
          for ( int i = 0; i < 16; ++i )
          {
              const QPoint at = ( sel + QPointF( radius * std::cos( i * M_PI / 8 ), radius * std::sin( i * M_PI / 8 ) ) ).toPoint();
              with_ring += image.pixelColor( at ).lightness();
              without += unselected.pixelColor( at ).lightness();
          }
          QVERIFY2( with_ring > without + 16 * 40, qPrintable( QString( "%1 %2" ).arg( with_ring ).arg( without ) ) );
          // what the camera sees (ahead, toward -y) brighter than behind it
          QVERIFY( px( 0, 0 ).lightness() > px( 0, 36 ).lightness() + 10 );
          // inside the 60 degrees (about -72 from the camera) and outside (about -10)
          QVERIFY( px( 10, 0 ).lightness() > px( 30, 25 ).lightness() + 10 );
          // nothing outside the minimap: not the camera's view beyond it either (above it, where it looks)
          QCOMPARE( image.pixelColor( int( r.center().x() ), int( r.top() ) - 20 ), QColor( Qt::black ) );
          QCOMPARE( image.pixelColor( 10, 10 ), QColor( Qt::black ) );
      }
};

QTEST_MAIN( TestGuides3D )
#include "test_guides_3d.moc"
