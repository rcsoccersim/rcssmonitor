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

#include "cameras_3d.h"
#include "disp_holder.h"
#include "field_painter.h"
#include "field_view_3d.h"
#include "ground_texture.h"
#include "label_atlas.h"
#include "options.h"
#include "scene_3d.h"
#include "scene_builder.h"

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_3_3_Core>

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
    void topCamera();
    void labels();
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

    // the same after what QPainter (the HUD) may leave: clipping, masks
    {
        QOpenGLFunctions_3_3_Core gl;
        QVERIFY( gl.initializeOpenGLFunctions() );
        gl.glEnable( GL_SCISSOR_TEST );
        gl.glScissor( 0, 0, 10, 10 );
        gl.glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
        gl.glDepthMask( GL_FALSE );
        gl.glDepthFunc( GL_GREATER );
    }
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
/*!
  The top camera, without perspective, as the 2D view: the center line
  across the middle, the goals at the left and the right, and a player at
  the field's y = -20 above the center (the field's -y up on the screen).
*/
void
TestScene3D::topCamera()
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
    p.y_ = -20.0f;
    show.ball_.x_ = 100.0f; // out of the view
    SceneContent content;
    SceneBuilder::addGoals( content );
    SceneBuilder::addShow( content, show, holder, Options::instance() );

    const CameraPose camera = Cameras3D::top( float( w ) / h );
    scene.render( fbo.handle(), w, h, camera, content );
    const QImage image = fbo.toImage();

    const double px_per_m = h / 2.0 / camera.ortho_half_height;
    const QColor field = Options::instance().fieldBrush().color();
    auto at = [&]( const double x, const double y ) {
        return image.pixelColor( int( std::lround( w / 2 + x * px_per_m ) ), int( std::lround( h / 2 + y * px_per_m ) ) );
    };

    // the center line, from top to bottom through the middle: thinner than a
    // pixel here (12.5 cm), so lighter than the grass rather than white
    QVERIFY2( at( 0.0, 10.0 ).lightness() > field.lightness() + 15, qPrintable( at( 0.0, 10.0 ).name() ) );
    QVERIFY2( at( 0.0, -30.0 ).lightness() > field.lightness() + 15, qPrintable( at( 0.0, -30.0 ).name() ) );
    QVERIFY2( distance( at( 10.0, 10.0 ), field ) < 12, qPrintable( at( 10.0, 10.0 ).name() ) );
    // the goals, left and right of the pitch: the dark floor under the white
    // roof of the net, grayer than the grass
    const double goal_x = Options::PITCH_HALF_LENGTH + Options::GOAL_DEPTH / 2;
    QVERIFY2( distance( at( -goal_x, 0.0 ), field ) > 40, qPrintable( at( -goal_x, 0.0 ).name() ) );
    QVERIFY2( distance( at( goal_x, 0.0 ), field ) > 40, qPrintable( at( goal_x, 0.0 ).name() ) );
    QVERIFY2( distance( at( -goal_x, 10.0 ), field ) < 12, qPrintable( at( -goal_x, 10.0 ).name() ) );
    // the player above the center, not below
    QVERIFY2( distance( at( 0.4, -20.0 ), field ) > 20, qPrintable( at( 0.4, -20.0 ).name() ) );
    QVERIFY2( distance( at( 0.4, 20.0 ), field ) < 12, qPrintable( at( 0.4, 20.0 ).name() ) );

    scene.release();
}

/*-------------------------------------------------------------------*/
/*!
  The number label of a player from the top camera, over the ground to the
  right of the player, white in a black outline, upright and not mirrored
  (a 7: its bar at the top, its foot at the left); nothing to its left. The
  same in every frame.
*/
void
TestScene3D::labels()
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
    p.unum_ = 7;
    p.state_ = rcss::rcg::STAND;
    p.x_ = 0.0f;
    p.y_ = 10.0f;
    show.ball_.x_ = 100.0f; // out of the view

    const CameraPose camera = Cameras3D::top( float( w ) / h );
    LabelAtlas atlas;
    SceneContent content;
    SceneBuilder::addShow( content, show, holder, Options::instance() );
    SceneBuilder::addLabels( content, show, holder, Options::instance(), atlas,
                             SceneBuilder::LabelView{ camera, float( h ), 30.0 } );
    QCOMPARE( content.labels.size(), size_t( 1 ) );
    scene.setLabels( atlas.image() );

    scene.render( fbo.handle(), w, h, camera, content );
    const QImage image = fbo.toImage();
    scene.render( fbo.handle(), w, h, camera, content );
    QCOMPARE( image, fbo.toImage() );

    // the label's area on the screen
    const double px_per_m = h / 2.0 / camera.ortho_half_height;
    const Billboard3D & label = content.labels[0];
    const double text_w = atlas.textWidth( 6 ) * label.size.y() / LabelAtlas::LABEL_SCALE * px_per_m;
    const double text_h = label.size.y() * LabelAtlas::LABEL_FONT / LabelAtlas::LABEL_HEIGHT * px_per_m;
    const int cx = int( std::lround( w / 2 + label.center.x() * px_per_m ) );
    const int cy = int( std::lround( h / 2 + label.center.z() * px_per_m ) );
    int white = 0, black = 0;
    int top = 0, bottom = 0, bottom_left = 0, bottom_right = 0; // white pixels
    for ( int y = int( cy - text_h / 2 ); y <= int( cy + text_h / 2 ); ++y )
    {
        for ( int x = int( cx - text_w / 2 ); x <= int( cx + text_w / 2 ); ++x )
        {
            const QColor c = image.pixelColor( x, y );
            if ( c.lightness() < 25 ) ++black;
            if ( c.lightness() <= 230 ) continue;
            ++white;
            if ( y < cy - text_h / 4 ) ++top;
            if ( y > cy + text_h / 4 )
            {
                ++bottom;
                ( x < cx ? bottom_left : bottom_right ) += 1;
            }
        }
    }
    QVERIFY2( white > 10 && black > 10, qPrintable( QString( "white %1 black %2" ).arg( white ).arg( black ) ) );
    QVERIFY2( top > bottom, qPrintable( QString( "top %1 bottom %2" ).arg( top ).arg( bottom ) ) );
    QVERIFY2( bottom_left > bottom_right, qPrintable( QString( "left %1 right %2" ).arg( bottom_left ).arg( bottom_right ) ) );

    // nothing on the other side of the player
    const QColor field = Options::instance().fieldBrush().color();
    const int mirror = int( std::lround( w / 2 - label.center.x() * px_per_m ) );
    QVERIFY2( distance( image.pixelColor( mirror, cy ), field ) < 12, qPrintable( image.pixelColor( mirror, cy ).name() ) );

    // over everything: the same under the ground (the top camera has no perspective)
    SceneContent buried = content;
    buried.labels[0].center.setY( -5.0f );
    scene.render( fbo.handle(), w, h, camera, buried );
    QCOMPARE( fbo.toImage(), image );

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
