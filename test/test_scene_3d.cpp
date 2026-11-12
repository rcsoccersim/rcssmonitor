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

/*!
  \brief how white each cell of a grid over a rectangle of an image is
  (the fraction of its pixels lighter than 230), row by row
*/
std::vector< double >
whiteGrid( const QImage & image,
           const QRectF & rect,
           const int columns,
           const int rows )
{
    std::vector< double > grid( std::size_t( columns * rows ), 0.0 );
    std::vector< int > count( grid.size(), 0 );
    for ( int y = int( std::ceil( rect.top() ) ); y < int( rect.bottom() ); ++y )
    {
        for ( int x = int( std::ceil( rect.left() ) ); x < int( rect.right() ); ++x )
        {
            const int c = std::min( columns - 1, int( ( x - rect.left() ) / rect.width() * columns ) );
            const int r = std::min( rows - 1, int( ( y - rect.top() ) / rect.height() * rows ) );
            const std::size_t i = std::size_t( r * columns + c );
            ++count[i];
            if ( image.pixelColor( x, y ).lightness() > 230 && image.pixelColor( x, y ).alpha() > 200 ) grid[i] += 1.0;
        }
    }
    for ( std::size_t i = 0; i < grid.size(); ++i ) grid[i] /= std::max( 1, count[i] );
    return grid;
}

//! the correlation of two grids, b read upside down or mirrored if asked
double
correlation( const std::vector< double > & a,
             const std::vector< double > & b,
             const int columns,
             const int rows,
             const bool flip_rows,
             const bool flip_columns )
{
    double ma = 0.0, mb = 0.0;
    for ( std::size_t i = 0; i < a.size(); ++i ) { ma += a[i]; mb += b[i]; }
    ma /= a.size();
    mb /= b.size();
    double ab = 0.0, aa = 0.0, bb = 0.0;
    for ( int r = 0; r < rows; ++r )
    {
        for ( int c = 0; c < columns; ++c )
        {
            const double va = a[std::size_t( r * columns + c )] - ma;
            const int rb = ( flip_rows ? rows - 1 - r : r ), cb = ( flip_columns ? columns - 1 - c : c );
            const double vb = b[std::size_t( rb * columns + cb )] - mb;
            ab += va * vb;
            aa += va * va;
            bb += vb * vb;
        }
    }
    return ab / std::sqrt( std::max( 1.0e-12, aa * bb ) );
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
  (as its image in the atlas, whatever the font; "7,t0" with the player
  type, which no font makes symmetric); nothing to its left. The same in
  every frame.
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
    // away from the lines: the label's width depends on the font of the machine
    p.x_ = 20.0f;
    p.y_ = 20.0f;
    show.ball_.x_ = 100.0f; // out of the view

    const CameraPose camera = Cameras3D::top( float( w ) / h );
    LabelAtlas atlas;
    SceneContent content;
    SceneBuilder::addShow( content, show, holder, Options::instance() );
    Options::instance().toggleShowPlayerType();
    SceneBuilder::addLabels( content, show, holder, Options::instance(), atlas,
                             SceneBuilder::LabelView{ camera, float( h ), 30.0 } );
    Options::instance().toggleShowPlayerType();
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
    for ( int y = int( cy - text_h / 2 ); y <= int( cy + text_h / 2 ); ++y )
    {
        for ( int x = int( cx - text_w / 2 ); x <= int( cx + text_w / 2 ); ++x )
        {
            const QColor c = image.pixelColor( x, y );
            if ( c.lightness() < 25 ) ++black;
            if ( c.lightness() > 230 ) ++white;
        }
    }
    QVERIFY2( white > 10 && black > 10, qPrintable( QString( "white %1 black %2" ).arg( white ).arg( black ) ) );

    // upright and not mirrored: the white of the label on the screen is
    // like that of its image in the atlas, more than upside down or mirrored
    const int columns = 24, rows = 8;
    const QRectF on_screen( cx - label.size.x() * px_per_m / 2, cy - label.size.y() * px_per_m / 2,
                            label.size.x() * px_per_m, label.size.y() * px_per_m );
    const QRectF uv = atlas.uv( 6 );
    const QRectF in_atlas( uv.left() * atlas.image().width(), uv.top() * atlas.image().height(),
                           uv.width() * atlas.image().width(), uv.height() * atlas.image().height() );
    const std::vector< double > drawn = whiteGrid( image, on_screen, columns, rows );
    const std::vector< double > source = whiteGrid( atlas.image(), in_atlas, columns, rows );
    const double upright = correlation( drawn, source, columns, rows, false, false );
    const double upside_down = correlation( drawn, source, columns, rows, true, false );
    const double mirrored = correlation( drawn, source, columns, rows, false, true );
    QVERIFY2( upright > 0.5 && upright > upside_down + 0.1 && upright > mirrored + 0.1,
              qPrintable( QString( "upright %1, upside down %2, mirrored %3" ).arg( upright ).arg( upside_down ).arg( mirrored ) ) );

    // nothing on the other side of the player
    const QColor field = Options::instance().fieldBrush().color();
    const int mirror = int( std::lround( w / 2 + ( 2.0 * p.x_ - label.center.x() ) * px_per_m ) );
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
