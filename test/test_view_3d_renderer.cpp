// -*-c++-*-

/*!
  \file test_view_3d_renderer.cpp
  \brief unit test of View3DRenderer: a frame of the 3D view drawn apart from its window
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

#include "view_3d_renderer.h"

#include "cameras_3d.h"
#include "field_view_3d.h"

#include "disp_holder.h"
#include "options.h"

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QPainter>

namespace {

constexpr int FRAMES = 10;
const QSize VIEW( 320, 180 );

//! a frame: the 22 players in two lines, the ball moving along x
rcss::rcg::ShowInfoT
show( const int frame )
{
    rcss::rcg::ShowInfoT s;
    s.time_ = frame;
    s.ball_.x_ = 0.5f * float( frame );
    s.ball_.y_ = 0.0f;
    for ( int i = 0; i < rcss::rcg::MAX_PLAYER * 2; ++i )
    {
        rcss::rcg::PlayerT & p = s.player_[i];
        const bool left = ( i < rcss::rcg::MAX_PLAYER );
        p.side_ = ( left ? 'l' : 'r' );
        p.unum_ = static_cast< rcss::rcg::Int16 >( i % rcss::rcg::MAX_PLAYER + 1 );
        p.state_ = rcss::rcg::STAND;
        p.type_ = 0;
        p.x_ = ( left ? -1.0f : 1.0f ) * ( 5.0f + 4.0f * float( i % rcss::rcg::MAX_PLAYER ) );
        p.y_ = ( left ? -10.0f : 10.0f );
        p.body_ = ( left ? 0.0f : 180.0f );
        p.stamina_ = 8000.0f;
        p.effort_ = 1.0f;
        p.recovery_ = 1.0f;
    }
    return s;
}

//! at frame index, not between two
Interpolate::Position
at( const std::size_t index )
{
    Interpolate::Position p;
    p.index = index;
    p.next = index;
    p.alpha = 0.0;
    return p;
}

//! the pixels that differ between two images in a rectangle
int
differing( const QImage & a,
           const QImage & b,
           const QRect & rect )
{
    int count = 0;
    for ( int y = rect.top(); y <= rect.bottom(); ++y )
    {
        for ( int x = rect.left(); x <= rect.right(); ++x )
        {
            if ( a.pixel( x, y ) != b.pixel( x, y ) ) ++count;
        }
    }
    return count;
}

}

/*!
  \brief View3DRenderer draws a frame as the 3D view does, into any
  framebuffer of the current context (docs/plan/2026-11-22-03_frame-export.md 4.2)
*/
class TestView3DRenderer
    : public QObject {
    Q_OBJECT

    QOpenGLContext M_context;
    QOffscreenSurface M_surface;

private slots:

    void initTestCase()
      {
          QSurfaceFormat format = FieldView3D::surfaceFormat();
          format.setSamples( 0 );
          M_context.setFormat( format );
          M_surface.setFormat( format );
          M_surface.create();
          if ( ! M_context.create()
               || M_context.format().version() < qMakePair( 3, 3 )
               || ! M_context.makeCurrent( &M_surface ) )
          {
              QSKIP( "no OpenGL 3.3 context" );
          }
      }

    /*
      The steps of a frame: build(), the camera after subject(), render().
      The same frame drawn twice is the same image, and the state is of
      what was drawn.
    */
    void render()
      {
          DispHolder holder;
          for ( int f = 0; f < FRAMES; ++f ) holder.handleShow( show( f ) );
          holder.setIndexFirst();

          View3DRenderer renderer( holder );
          QVERIFY( ! renderer.ready() );
          QVERIFY( renderer.initialize() );
          QVERIFY( renderer.ready() );

          QOpenGLFramebufferObjectFormat fbo_format;
          fbo_format.setAttachment( QOpenGLFramebufferObject::CombinedDepthStencil );
          QOpenGLFramebufferObject fbo( VIEW, fbo_format );

          CameraController camera;
          const float aspect = float( VIEW.width() ) / float( VIEW.height() );
          renderer.build( at( 4 ), false, 0, camera.mode() );
          // the broadcast camera after the ball of the frame
          QCOMPARE( renderer.subject().ball_x, 2.0f );
          const CameraPose pose = camera.update( 0.0, aspect, renderer.subject() );
          renderer.render( fbo.handle(), pose, VIEW, 1.0, 0 );
          const QImage first = fbo.toImage();
          QVERIFY( renderer.buildTime() >= 0.0 );
          QVERIFY( renderer.submitTime() >= 0.0 );

          const View3DState state = renderer.state( pose );
          QCOMPARE( state.players, 22 );
          QCOMPARE( state.at.index, std::size_t( 4 ) );
          QVERIFY( state.labels > 0 );

          // the field's color in the middle of the view: the ground was sent
          const QColor field = Options::instance().fieldBrush().color();
          int grass = 0;
          for ( int x = 0; x < VIEW.width(); x += 8 )
          {
              const QColor c = first.pixelColor( x, VIEW.height() * 3 / 4 );
              if ( std::abs( c.green() - field.green() ) < 40 && c.green() > c.red() ) ++grass;
          }
          QVERIFY2( grass > 10, qPrintable( QString::number( grass ) ) );

          renderer.build( at( 4 ), false, 0, camera.mode() );
          renderer.render( fbo.handle(), camera.update( 0.0, aspect, renderer.subject() ), VIEW, 1.0, 0 );
          QCOMPARE( fbo.toImage(), first );

          // another frame: another image
          renderer.build( at( 8 ), false, 0, camera.mode() );
          renderer.render( fbo.handle(), camera.update( 0.0, aspect, renderer.subject() ), VIEW, 1.0, 0 );
          QVERIFY( fbo.toImage() != first );

          renderer.release();
      }

    /*
      drawHud() draws over the scene in the logical size: the score board
      at the bottom of the view.
    */
    void hud()
      {
          DispHolder holder;
          for ( int f = 0; f < FRAMES; ++f ) holder.handleShow( show( f ) );
          holder.setIndexFirst();

          View3DRenderer renderer( holder );
          QVERIFY( renderer.initialize() );
          QOpenGLFramebufferObjectFormat fbo_format;
          fbo_format.setAttachment( QOpenGLFramebufferObject::CombinedDepthStencil );
          QOpenGLFramebufferObject fbo( VIEW, fbo_format );

          CameraController camera;
          renderer.build( at( 0 ), false, 0, camera.mode() );
          const CameraPose pose = camera.update( 0.0, float( VIEW.width() ) / float( VIEW.height() ), renderer.subject() );
          renderer.render( fbo.handle(), pose, VIEW, 1.0, 0 );
          const QImage scene = fbo.toImage().convertToFormat( QImage::Format_RGB32 );

          QImage image = scene;
          {
              QPainter painter( &image );
              const QRect logical( QPoint( 0, 0 ), VIEW );
              painter.setViewport( logical );
              painter.setWindow( logical );
              renderer.drawHud( painter, pose, VIEW, 1.0, camera.mode() );
          }
          QVERIFY( Options::instance().showScoreBoard() );
          const QRect bottom( 0, VIEW.height() - 10, VIEW.width(), 10 );
          const QRect middle( 0, VIEW.height() / 2 - 5, VIEW.width(), 10 );
          QVERIFY( differing( scene, image, bottom ) > VIEW.width() );
          QCOMPARE( differing( scene, image, middle ), 0 );

          renderer.release();
      }

    /*
      The team logos are on the boards from the first frame: the ends are
      drawn after TeamGraphicPainter::update(), not after the HUD drew the
      logos (the first frame of an export has them too). The first frame
      is the same as the second.
    */
    void logosFirstFrame()
      {
          DispHolder holder;
          rcss::rcg::TeamT team_l, team_r;
          team_l.name_ = "A";
          team_r.name_ = "B";
          holder.handleTeam( 0, team_l, team_r );
          std::vector< std::string > xpm;
          xpm.push_back( "8 8 1 1" );
          xpm.push_back( "a c #ff00ff" );
          for ( int i = 0; i < 8; ++i ) xpm.push_back( "aaaaaaaa" );
          for ( int x = 0; x < 32; ++x )
          {
              for ( int y = 0; y < 8; ++y )
              {
                  holder.handleTeamGraphic( 'l', x, y, xpm );
                  holder.handleTeamGraphic( 'r', x, y, xpm );
              }
          }
          for ( int f = 0; f < FRAMES; ++f ) holder.handleShow( show( f ) );
          holder.setIndexFirst();
          QVERIFY( Options::instance().showTeamGraphic() );

          View3DRenderer renderer( holder );
          QVERIFY( renderer.initialize() );
          QOpenGLFramebufferObjectFormat fbo_format;
          fbo_format.setAttachment( QOpenGLFramebufferObject::CombinedDepthStencil );
          QOpenGLFramebufferObject fbo( VIEW, fbo_format );

          // behind the right goal: the left end's board, with its logo, in sight
          CameraController camera;
          camera.setMode( CameraMode::GoalRight );
          const float aspect = float( VIEW.width() ) / float( VIEW.height() );
          renderer.build( at( 0 ), false, 0, camera.mode() );
          renderer.render( fbo.handle(), camera.update( 0.0, aspect, renderer.subject() ), VIEW, 1.0, 0 );
          const QImage first = fbo.toImage();
          // the HUD, as the view draws it after the scene (it draws the logos)
          {
              QImage hud( VIEW, QImage::Format_RGB32 );
              QPainter painter( &hud );
              renderer.drawHud( painter, camera.update( 0.0, aspect, renderer.subject() ), VIEW, 1.0, camera.mode() );
          }
          renderer.build( at( 0 ), false, 0, camera.mode() );
          renderer.render( fbo.handle(), camera.update( 0.0, aspect, renderer.subject() ), VIEW, 1.0, 0 );
          QCOMPARE( first, fbo.toImage() );

          renderer.release();
      }

    void cleanupTestCase()
      {
          M_context.doneCurrent();
      }
};

QTEST_MAIN( TestView3DRenderer )
#include "test_view_3d_renderer.moc"
