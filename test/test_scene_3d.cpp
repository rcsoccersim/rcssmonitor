// -*-c++-*-

/*!
  \file test_scene_3d.cpp
  \brief test of drawing the scene of the 3D view offscreen
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

#include <QtTest>

#include "disp_holder.h"
#include "field_painter.h"
#include "field_view_3d.h"
#include "ground_texture.h"
#include "options.h"
#include "scene_3d.h"
#include "scene_builder.h"

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>

#include <cmath>
#include <memory>

/*
  The scene is drawn offscreen (docs/PLAN_3D-VIEW.md §10.2): the test is
  skipped where an OpenGL 3.3 context cannot be made. With Mesa,
  LIBGL_ALWAYS_SOFTWARE=1 draws with llvmpipe, without a GPU.
*/

namespace {

int
distance( const QColor & a,
          const QColor & b )
{
    return std::abs( a.red() - b.red() ) + std::abs( a.green() - b.green() ) + std::abs( a.blue() - b.blue() );
}

}

/*!
  \brief test of drawing the scene of the 3D view offscreen
*/
class TestScene3D
    : public QObject {
    Q_OBJECT

private:
    QOpenGLContext M_context;
    QOffscreenSurface M_surface;

private slots:

    void initTestCase();
    void ground();
    void content();
    void cleanupTestCase();
};

/*-------------------------------------------------------------------*/
void
TestScene3D::initTestCase()
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

/*-------------------------------------------------------------------*/
/*!
  The ground seen from straight above: the center line in the middle and
  the grass beside it, the same in every frame.
*/
void
TestScene3D::ground()
{
    DispHolder holder;
    FieldPainter painter( holder );
    Scene3D scene;
    QVERIFY( scene.initialize() );
    scene.setGround( GroundTexture::draw( painter ) );

    const int w = 400, h = 300;
    QOpenGLFramebufferObjectFormat fbo_format;
    fbo_format.setAttachment( QOpenGLFramebufferObject::Depth );
    QOpenGLFramebufferObject fbo( w, h, fbo_format );

    // 10 m above the center spot: about 13 m of the field across
    CameraPose camera;
    camera.position = QVector3D( 0.0f, 10.0f, 0.0f );
    camera.target = QVector3D( 0.0f, 0.0f, 0.0f );
    camera.up = QVector3D( 0.0f, 0.0f, -1.0f ); // the field's -y is up on the screen

    scene.render( fbo.handle(), w, h, camera, SceneContent() );
    const QImage first = fbo.toImage();
    scene.render( fbo.handle(), w, h, camera, SceneContent() );
    const QImage second = fbo.toImage();
    QCOMPARE( first, second );

    const QColor field = Options::instance().fieldBrush().color();
    const QColor line = Options::instance().linePen().color();
    // the center line (x = 0) at the top of the view, and the grass 3 m to its right
    QVERIFY2( distance( first.pixelColor( w / 2, h / 8 ), line ) < 90,
              qPrintable( first.pixelColor( w / 2, h / 8 ).name() ) );
    const int px_per_m = int( h / ( 2 * 10 * std::tan( 17.5 * M_PI / 180 ) ) );
    QVERIFY2( distance( first.pixelColor( w / 2 + 3 * px_per_m, h / 8 ), field ) < 12,
              qPrintable( first.pixelColor( w / 2 + 3 * px_per_m, h / 8 ).name() ) );

    scene.release();
}

/*-------------------------------------------------------------------*/
/*!
  A player and the ball seen from straight above: the head of the player at
  the center, and the ball to the right in its color, with its shadow on
  the grass; the same in every frame.
*/
void
TestScene3D::content()
{
    DispHolder holder;
    FieldPainter painter( holder );
    Scene3D scene;
    QVERIFY( scene.initialize() );
    scene.setGround( GroundTexture::draw( painter ) );

    const int w = 400, h = 300;
    QOpenGLFramebufferObjectFormat fbo_format;
    fbo_format.setAttachment( QOpenGLFramebufferObject::Depth );
    QOpenGLFramebufferObject fbo( w, h, fbo_format );

    rcss::rcg::ShowInfoT show;
    rcss::rcg::PlayerT & p = show.player_[0];
    p.side_ = 'l';
    p.unum_ = 1;
    p.state_ = rcss::rcg::STAND;
    p.x_ = 0.0f;
    p.y_ = 0.0f;
    p.body_ = 90.0f; // the nose downward on the screen, away from the ball
    show.ball_.x_ = 3.0f;
    show.ball_.y_ = 0.0f;
    show.ball_.z_ = 2.0f; // in the air: the shadow is not hidden by the ball
    SceneContent content;
    SceneBuilder::addGoals( content );
    SceneBuilder::addShow( content, show, holder, Options::instance() );

    // 10 m above the center spot, as in ground()
    CameraPose camera;
    camera.position = QVector3D( 0.0f, 10.0f, 0.0f );
    camera.target = QVector3D( 0.0f, 0.0f, 0.0f );
    camera.up = QVector3D( 0.0f, 0.0f, -1.0f );

    scene.render( fbo.handle(), w, h, camera, content );
    const QImage first = fbo.toImage();
    scene.render( fbo.handle(), w, h, camera, content );
    QCOMPARE( first, fbo.toImage() );

    const QColor field = Options::instance().fieldBrush().color();
    const QColor team = Options::instance().leftTeamBrush().color();
    const QColor ball = Options::instance().ballBrush().color();
    // the top of the head, lit from above
    const QColor head = first.pixelColor( w / 2, h / 2 );
    QVERIFY2( distance( head, team ) < 100, qPrintable( head.name() ) );
    // the ball, 3 m to the right and 2 m up, nearer to the camera
    const double px_per_m = h / ( 2 * 10 * std::tan( 17.5 * M_PI / 180 ) ); // on the ground
    const double ball_y = 2.0 + SceneBuilder::ballRadius( Options::instance(), holder.serverParam().ball_size_ );
    const QColor ball_px = first.pixelColor( w / 2 + int( 3 * px_per_m * 10.0 / ( 10.0 - ball_y ) ), h / 2 );
    QVERIFY2( distance( ball_px, ball ) < 120, qPrintable( ball_px.name() ) );
    // the circle under the player, and the shadow of the ball: darker or
    // tinted grass, not the bare grass
    const QColor disc = first.pixelColor( w / 2 - int( 0.7 * px_per_m ), h / 2 );
    QVERIFY2( distance( disc, field ) > 20, qPrintable( disc.name() ) );
    const QColor shadow = first.pixelColor( w / 2 + int( 2.9 * px_per_m ), h / 2 );
    QVERIFY2( shadow.lightness() < field.lightness() - 10, qPrintable( shadow.name() ) );
    // the grass far from them
    const QColor grass = first.pixelColor( w / 2 - int( 3 * px_per_m ), h / 4 );
    QVERIFY2( distance( grass, field ) < 12, qPrintable( grass.name() ) );

    scene.release();
}

/*-------------------------------------------------------------------*/
void
TestScene3D::cleanupTestCase()
{
    M_context.doneCurrent();
}

QTEST_MAIN( TestScene3D )

#include "test_scene_3d.moc"
