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

#include "ball_3d.h"
#include "ball_aids_3d.h"
#include "frame_builder_3d.h"
#include "cameras_3d.h"
#include "disp_holder.h"
#include "field_painter.h"
#include "field_view_3d.h"
#include "ground_texture.h"
#include "label_atlas.h"
#include "overlay_builder.h"
#include "figure_uniform.h"
#include "figures_3d.h"
#include "options.h"
#include "scene_3d.h"
#include "scene_builder.h"
#include "stadium_3d.h"
#include "stands_3d.h"
#include "grass_3d.h"
#include "scenery_3d.h"
#include "sun_3d.h"

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_3_3_Core>

#include <cmath>
#include <functional>
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
    void batches();
    void panels();
    void figures();
    void humanFigure();
    void shadows();
    void blobShadows();
    void sky();
    void realisticGrass();
    void stands();
    void ball();
    void ballAids();
    void topSwitch();
    void cleanupTestCase();
};

/*-------------------------------------------------------------------*/
void
TestScene3D::initTestCase()
{
    // the ground plain: its color is the field color (the stripes are
    // checked by test ground_texture)
    Options::instance().setGrass3D( GrassMode::Plain );

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
/*!
  The vertices made in every frame (VertexBatch), from 10 m above: a solid
  line drawn all along, a dotted one in dashes, two dots, and a translucent
  triangle over the grass. The same in every frame.
*/
void
TestScene3D::batches()
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

    // away from the lines of the field: around (20, 20)
    const QPointF o( 20.0, 20.0 );
    SceneContent content;
    OverlayBuilder::addLine( content.lines, o + QPointF( -4, -2 ), o + QPointF( 4, -2 ), Qt::red );
    OverlayBuilder::addLine( content.dotted_lines, o + QPointF( -4, 2 ), o + QPointF( 4, 2 ), Qt::blue );
    for ( const double x : { -3.0, 3.0 } )
    {
        content.points.push_back( ColorVertex3D{ QVector3D( float( o.x() + x ), OverlayBuilder::LINE_Y, float( o.y() ) ), QVector4D( 1, 1, 0, 1 ), 0.0f } );
    }
    for ( const QPointF & p : { QPointF( -1.5, 0.6 ), QPointF( 0.5, 0.6 ), QPointF( -0.5, 1.6 ) } )
    {
        content.sectors.push_back( ColorVertex3D{ QVector3D( float( o.x() + p.x() ), OverlayBuilder::SECTOR_Y, float( o.y() + p.y() ) ), QVector4D( 1, 0, 1, 0.5f ), 0.0f } );
    }

    CameraPose camera;
    camera.position = QVector3D( float( o.x() ), 10.0f, float( o.y() ) );
    camera.target = QVector3D( float( o.x() ), 0.0f, float( o.y() ) );
    camera.up = QVector3D( 0.0f, 0.0f, -1.0f );
    scene.render( fbo.handle(), w, h, camera, content );
    const QImage image = fbo.toImage();
    scene.render( fbo.handle(), w, h, camera, content );
    QCOMPARE( image, fbo.toImage() );

    const double px_per_m = h / ( 2 * 10 * std::tan( 17.5 * M_PI / 180 ) ); // at the ground
    auto screen = [&]( const double x, const double y ) {
        return QPoint( int( std::lround( w / 2 + x * px_per_m ) ), int( std::lround( h / 2 + y * px_per_m ) ) );
    };
    // whether a pixel near the point (2 pixels up or down) has the color
    auto drawn = [&]( const double x, const double y, const QColor & color ) {
        const QPoint s = screen( x, y );
        for ( int dy = -2; dy <= 2; ++dy )
        {
            if ( distance( image.pixelColor( s.x(), s.y() + dy ), color ) < 60 ) return true;
        }
        return false;
    };

    int solid = 0, dotted = 0, columns = 0;
    for ( double x = -3.0; x <= 3.0; x += 1.0 / px_per_m, ++columns )
    {
        solid += ( drawn( x, -2.0, Qt::red ) ? 1 : 0 );
        dotted += ( drawn( x, 2.0, Qt::blue ) ? 1 : 0 );
    }
    QVERIFY2( solid == columns, qPrintable( QString( "solid %1 of %2" ).arg( solid ).arg( columns ) ) );
    // 0.3 m dashes and 0.3 m gaps: about half
    QVERIFY2( dotted > columns * 0.35 && dotted < columns * 0.65, qPrintable( QString( "dotted %1 of %2" ).arg( dotted ).arg( columns ) ) );
    QVERIFY( drawn( -3.0, 0.0, Qt::yellow ) );
    QVERIFY( drawn( 3.0, 0.0, Qt::yellow ) );
    QVERIFY( ! drawn( 0.0, 0.0, Qt::yellow ) );

    // the triangle: the grass tinted, not covered (the view is 3.1 m from its center to its top and bottom)
    const QColor field = Options::instance().fieldBrush().color();
    const QPoint inside = screen( -0.5, 0.95 ), outside = screen( 2.0, 1.0 );
    QVERIFY( image.rect().contains( inside ) && image.rect().contains( outside ) );
    const QColor tinted = image.pixelColor( inside );
    QVERIFY2( distance( tinted, field ) > 60 && distance( tinted, QColor( Qt::magenta ) ) > 60, qPrintable( tinted.name() ) );
    QVERIFY2( distance( image.pixelColor( outside ), field ) < 12, qPrintable( image.pixelColor( outside ).name() ) );

    scene.release();
}

/*-------------------------------------------------------------------*/
/*!
  A panel (Stadium3D): its image on the front, upright (the image's top at
  the top) and repeated once in every BOARD_TILE m of a board; its back
  plain in the instance's color; both drawn (no face left out)
*/
void
TestScene3D::panels()
{
    DispHolder holder;
    FieldPainter painter( holder );
    Scene3D scene;
    QVERIFY( scene.initialize() );
    scene.setGround( GroundTexture::draw( painter ) );

    // the board of the left end: red on the upper half, blue on the lower,
    // and on the right half of each tile a green stripe
    QImage board( 100, 20, QImage::Format_RGB32 );
    board.fill( Qt::blue );
    for ( int y = 0; y < 10; ++y ) for ( int x = 0; x < 100; ++x ) board.setPixelColor( x, y, Qt::red );
    for ( int y = 0; y < 20; ++y ) for ( int x = 60; x < 80; ++x ) board.setPixelColor( x, y, Qt::green );
    QImage banner( 10, 10, QImage::Format_RGB32 );
    banner.fill( Qt::white );
    scene.setStadium( 0, board, banner );

    const int w = 400, h = 200;
    QOpenGLFramebufferObjectFormat fbo_format;
    fbo_format.setAttachment( QOpenGLFramebufferObject::Depth );
    QOpenGLFramebufferObject fbo( w, h, fbo_format );

    // a board 20 m wide and 4 m high, standing 10 m in front of the camera,
    // facing it (+z); the camera 30 m away, so that it fills the middle
    SceneContent content;
    QMatrix4x4 m;
    m.translate( 0.0f, 2.0f, 0.0f );
    m.scale( 2.0f * Stadium3D::BOARD_TILE, 4.0f, 1.0f );
    content.boards[0].push_back( Instance3D{ m, QVector4D( 1.0f, 1.0f, 0.0f, 1.0f ) } );
    CameraPose camera;
    camera.position = QVector3D( 0.0f, 2.0f, 30.0f );
    camera.target = QVector3D( 0.0f, 2.0f, 0.0f );
    scene.render( fbo.handle(), w, h, camera, content );
    QImage front = fbo.toImage();

    // where a point of the board is on the screen
    const QMatrix4x4 vp = camera.projection( float( w ) / h ) * camera.view();
    const auto at = [&]( const float x, const float y ) {
        const QVector3D ndc = vp.map( QVector3D( x, y, 0.0f ) );
        return QPoint( int( ( ndc.x() + 1.0f ) / 2.0f * w ), int( ( 1.0f - ndc.y() ) / 2.0f * h ) );
    };
    // the top red, the bottom blue (not upside down), at the first tile's left
    QCOMPARE( front.pixelColor( at( -9.0f, 3.0f ) ), QColor( Qt::red ) );
    QCOMPARE( front.pixelColor( at( -9.0f, 1.0f ) ), QColor( Qt::blue ) );
    // the green stripe at 6 to 8 m of each tile: in both tiles
    QCOMPARE( front.pixelColor( at( -10.0f + 7.0f, 2.0f ) ), QColor( Qt::green ) );
    QCOMPARE( front.pixelColor( at( 7.0f, 2.0f ) ), QColor( Qt::green ) );
    QCOMPARE( front.pixelColor( at( 3.0f, 3.0f ) ), QColor( Qt::red ) );

    // from behind: the plain back (yellow), not the image
    camera.position = QVector3D( 0.0f, 2.0f, -30.0f );
    scene.render( fbo.handle(), w, h, camera, content );
    const QImage back = fbo.toImage();
    QCOMPARE( back.pixelColor( w / 2, h / 2 ), QColor( Qt::yellow ) );

    scene.release();
}

/*-------------------------------------------------------------------*/
/*!
  a human figure seen from its side: its jersey in its color up to about
  1.2 times a person's height, its joints in their own color, nothing
  above it; the same when drawn again, and nothing without it
*/
void
TestScene3D::figures()
{
    DispHolder holder;
    FieldPainter painter( holder );
    Figures3D figures;
    QVERIFY2( figures.load(), qPrintable( figures.error() ) );
    Scene3D scene;
    QVERIFY( scene.initialize() );
    scene.setGround( GroundTexture::draw( painter ) );
    scene.setFigureBody( 0, figures.model(), figures.paints() );

    const int w = 300, h = 400;
    QOpenGLFramebufferObjectFormat fbo_format;
    fbo_format.setAttachment( QOpenGLFramebufferObject::Depth );
    QOpenGLFramebufferObject fbo( w, h, fbo_format );

    // facing +x at the center spot, in red; seen from 6 m on its left side
    SceneContent content;
    const float height = 2.22f;
    content.figures.push_back( FigureInstance{ 0, 0.0f, 0.0f, 0.0f, height, QVector4D( 1.0f, 0.0f, 0.0f, 1.0f ) } );
    figures.pose( content, 0.0 );
    CameraPose camera;
    camera.position = QVector3D( 0.0f, 1.2f, -6.0f );
    camera.target = QVector3D( 0.0f, 1.2f, 0.0f );

    scene.render( fbo.handle(), w, h, camera, content );
    const QImage image = fbo.toImage();
    scene.render( fbo.handle(), w, h, camera, content );
    QCOMPARE( image, fbo.toImage() );

    // where a point of the world is on the image
    const QMatrix4x4 view_proj = camera.projection( float( w ) / h ) * camera.view();
    const auto pixel = [&]( const QVector3D & p ) {
        const QVector3D ndc = view_proj.map( p );
        return QPoint( int( ( ndc.x() + 1.0f ) / 2.0f * w ), int( ( 1.0f - ndc.y() ) / 2.0f * h ) );
    };
    const auto red = []( const QColor & c ) { return c.red() > c.green() + 60 && c.red() > c.blue() + 60; };
    const auto purple = []( const QColor & c ) { return c.blue() > c.red() + 30 && c.blue() > c.green() + 60; };

    // the jersey at the waist and the chest
    for ( const float y : { 1.0f, 1.4f } )
    {
        const QColor c = image.pixelColor( pixel( QVector3D( 0.0f, y, 0.0f ) ) );
        QVERIFY2( red( c ), qPrintable( QString::number( y ) + " " + c.name() ) );
    }
    // nothing of it above its height
    const QPoint top = pixel( QVector3D( 0.0f, height + 0.15f, 0.0f ) );
    for ( int x = 0; x < w; ++x )
    {
        QVERIFY2( ! red( image.pixelColor( x, top.y() ) ), qPrintable( QString::number( x ) ) );
    }
    // the head near the top
    QVERIFY( red( image.pixelColor( pixel( QVector3D( 0.0f, height - 0.2f, 0.0f ) ) ) ) );
    // the joints in their own color, somewhere on it
    int joints = 0;
    for ( int y = top.y(); y < h; ++y )
    {
        for ( int x = 0; x < w; ++x )
        {
            if ( purple( image.pixelColor( x, y ) ) ) ++joints;
        }
    }
    QVERIFY2( joints > 20, qPrintable( QString::number( joints ) ) );

    // without it: the background there
    content.figures.clear();
    figures.pose( content, 0.0 );
    scene.render( fbo.handle(), w, h, camera, content );
    QVERIFY( ! red( fbo.toImage().pixelColor( pixel( QVector3D( 0.0f, 1.4f, 0.0f ) ) ) ) );
}

/*-------------------------------------------------------------------*/
/*!
  the human body in its uniform, seen from its side: the shirt in the
  figure's color at the chest, the shorts darker at the thigh, the sock in
  its color at the shin, the shoe dark at the foot, the skin at the head
*/
void
TestScene3D::humanFigure()
{
    DispHolder holder;
    FieldPainter painter( holder );
    Figures3D figures;
    QVERIFY2( figures.load(), qPrintable( figures.error() ) );
    QVERIFY( FigurePacks::builtin( "human" ) );
    QVERIFY2( figures.setPack( *FigurePacks::builtin( "human" ) ), qPrintable( figures.error() ) );
    // the hairstyles of each body: bits of its own
    const auto bit = [&figures]( const int body, const char * material ) {
        return figures.pack().bodies[body].bitOf( material );
    };
    Scene3D scene;
    QVERIFY( scene.initialize() );
    scene.setGround( GroundTexture::draw( painter ) );
    scene.setFigureBody( 0, figures.model(), figures.paints(), figures.uniformCuts() );

    const int w = 300, h = 400;
    QOpenGLFramebufferObjectFormat fbo_format;
    fbo_format.setAttachment( QOpenGLFramebufferObject::Depth );
    QOpenGLFramebufferObject fbo( w, h, fbo_format );

    // facing +x at the center spot, in red; seen from 4 m on its left side
    SceneContent content;
    const float height = 1.85f;
    content.figures.push_back( FigureInstance{ 0, 0.0f, 0.0f, 0.0f, height, QVector4D( 1.0f, 0.0f, 0.0f, 1.0f ) } );
    figures.pose( content, 0.0 );
    CameraPose camera;
    camera.position = QVector3D( 0.0f, 1.0f, -4.0f );
    camera.target = QVector3D( 0.0f, 1.0f, 0.0f );
    scene.render( fbo.handle(), w, h, camera, content );
    const QImage image = fbo.toImage();
    // the same body all in its color, lit the same, to compare with
    std::vector< FigureUniform::Paint > all_color = figures.paints();
    for ( FigureUniform::Paint & p : all_color )
    {
        if ( p.kind == FigureUniform::Paint::Uniform ) p.kind = FigureUniform::Paint::Figure;
    }
    scene.setFigureBody( 0, figures.model(), all_color );
    scene.render( fbo.handle(), w, h, camera, content );
    const QImage plain = fbo.toImage();
    scene.setFigureBody( 0, figures.model(), figures.paints(), figures.uniformCuts() );

    // a point of the figure's bone, a little toward the camera: its pixel
    // in the uniform, and its brightness of the figure's color there
    std::vector< QMatrix4x4 > world;
    figures.worldNodes( content.figures[0], 0.0, world );
    const QMatrix4x4 view_proj = camera.projection( float( w ) / h ) * camera.view();
    QColor lit;
    const auto color = [&]( const char * from, const char * to, const float t, const QVector3D & shift ) {
        const QVector3D a = world[figures.model().nodeIndex( from )].column( 3 ).toVector3D();
        const QVector3D b = world[figures.model().nodeIndex( to )].column( 3 ).toVector3D();
        const QVector3D ndc = view_proj.map( a + ( b - a ) * t + shift );
        const QPoint p( int( ( ndc.x() + 1.0f ) / 2.0f * w ), int( ( 1.0f - ndc.y() ) / 2.0f * h ) );
        lit = plain.pixelColor( p );
        return image.pixelColor( p );
    };
    const auto describe = [&]( const char * what, const QColor & c ) {
        return qPrintable( QString( "%1 %2 (%3)" ).arg( what, c.name(), lit.name() ) ); };
    const QVector3D side( 0.0f, 0.0f, -0.05f ); // toward the camera, on the surface
    const auto red = []( const QColor & c ) { return c.red() > 60 && c.green() < 20 && c.blue() < 20; };

    // the shirt and the sock: the figure's color (the chest, or the sleeve before it)
    const QColor shirt = color( "spine_02", "spine_03", 0.5f, QVector3D( 0.06f, 0.0f, 0.0f ) );
    QVERIFY2( red( lit ) && std::abs( shirt.red() - lit.red() ) <= 2 && shirt.green() < 20, describe( "shirt", shirt ) );
    const QColor sock = color( "calf_l", "foot_l", 0.55f, side );
    QVERIFY2( red( lit ) && std::abs( sock.red() - lit.red() ) <= 2 && sock.green() < 20, describe( "sock", sock ) );
    // the shorts: darker by their shade
    const QColor shorts = color( "thigh_l", "calf_l", 0.25f, side );
    QVERIFY2( red( lit ) && std::abs( shorts.red() - FigureUniform::SHORTS_SHADE * lit.red() ) <= 3 && shorts.green() < 20,
              describe( "shorts", shorts ) );
    // the shoe: dark
    const QColor shoe = color( "foot_l", "ball_l", 0.5f, QVector3D( 0.0f, 0.0f, -0.03f ) );
    QVERIFY2( red( lit ) && shoe.red() < 40 && shoe.green() < 40 && shoe.blue() < 40, describe( "shoe", shoe ) );
    // the skin: the face, the forearm
    const QColor face = color( "Head", "Head", 0.0f, QVector3D( 0.05f, 0.05f, -0.06f ) );
    QVERIFY2( red( lit ) && face.red() > face.green() && face.green() > face.blue() && face.green() > 70, describe( "face", face ) );
    const QColor arm = color( "lowerarm_l", "hand_l", 0.5f, side );
    QVERIFY2( red( lit ) && arm.red() > arm.green() && arm.green() > arm.blue() && arm.green() > 70, describe( "forearm", arm ) );

    // from the front above: the shoulders' tops beside the neck in the
    // shirt (not torn, 2026-11-14, user), under the chin the skin
    {
        CameraPose above;
        above.position = QVector3D( 1.6f, 2.3f, 0.0f );
        above.target = QVector3D( 0.0f, 1.45f, 0.0f );
        above.fov_y = 30.0f;
        scene.render( fbo.handle(), w, h, above, content );
        const QImage uniform = fbo.toImage();
        scene.setFigureBody( 0, figures.model(), all_color );
        scene.render( fbo.handle(), w, h, above, content );
        const QImage colored = fbo.toImage();
        scene.setFigureBody( 0, figures.model(), figures.paints(), figures.uniformCuts() );
        const QMatrix4x4 above_proj = above.projection( float( w ) / h ) * above.view();
        const auto pixel = [&]( const QVector3D & p ) {
            const QVector3D ndc = above_proj.map( p );
            return QPoint( int( ( ndc.x() + 1.0f ) / 2.0f * w ), int( ( 1.0f - ndc.y() ) / 2.0f * h ) );
        };
        const auto joint = [&]( const char * name ) {
            return world[figures.model().nodeIndex( name )].column( 3 ).toVector3D();
        };
        for ( const char * arm : { "upperarm_l", "upperarm_r" } )
        {
            const QPoint at = pixel( joint( "neck_01" ) + ( joint( arm ) - joint( "neck_01" ) ) * 0.7f + QVector3D( 0.0f, 0.05f, 0.0f ) );
            const QColor c = uniform.pixelColor( at );
            QVERIFY2( red( colored.pixelColor( at ) ) && std::abs( c.red() - colored.pixelColor( at ).red() ) <= 2 && c.green() < 20,
                      qPrintable( QString( "%1's shoulder %2" ).arg( arm, c.name() ) ) );
        }
        const QPoint chin = pixel( joint( "Head" ) + QVector3D( 0.08f, -0.02f, 0.0f ) );
        const QColor c = uniform.pixelColor( chin );
        QVERIFY2( red( colored.pixelColor( chin ) ) && c.green() > 70, qPrintable( "chin " + c.name() ) );
    }

    // its looks: the skin of its own color; the hair in its color, the
    // hairstyle it wears only
    const auto count = [&]( const QImage & img, const std::function< bool ( const QColor & ) > & is ) {
        int n = 0;
        for ( int y = 0; y < h; ++y )
            for ( int x = 0; x < w; ++x )
                if ( is( img.pixelColor( x, y ) ) ) ++n;
        return n;
    };
    const auto blue = []( const QColor & c ) { return c.blue() > c.red() + 60 && c.blue() > c.green() + 60; };
    const auto green = []( const QColor & c ) { return c.green() > 60 && c.red() < 20 && c.blue() < 20; };
    QCOMPARE( count( image, blue ), 0 );
    FigureInstance & f = content.figures[0];
    f.look.hair_color = QVector4D( 0.0f, 0.0f, 1.0f, 1.0f );
    f.look.skin = QVector4D( 0.0f, 0.8f, 0.0f, 1.0f );
    figures.pose( content, 0.0 );
    scene.render( fbo.handle(), w, h, camera, content );
    const QImage parted = fbo.toImage();
    const int parted_hair = count( parted, blue );
    QVERIFY2( parted_hair > 150, qPrintable( QString::number( parted_hair ) ) );
    QVERIFY( count( parted, green ) > 300 ); // the face, the arms, the knees
    QVERIFY( ! red( parted.pixelColor( QPoint( 0, 0 ) ) ) );
    // no hairstyle: the eyebrows only
    f.look.hair = 0;
    scene.render( fbo.handle(), w, h, camera, content );
    const int eyebrows = count( fbo.toImage(), blue );
    QVERIFY2( eyebrows < parted_hair / 4, qPrintable( QString( "%1 %2" ).arg( eyebrows ).arg( parted_hair ) ) );
    // with a beard too: more
    f.look.hair = bit( 0, "Hair_SimpleParted" ) | bit( 0, "Hair_Beard" );
    scene.render( fbo.handle(), w, h, camera, content );
    QVERIFY( count( fbo.toImage(), blue ) > parted_hair + 20 );
    // a bit of no hairstyle of his body: none
    f.look.hair = 1u << 5;
    scene.render( fbo.handle(), w, h, camera, content );
    QCOMPARE( count( fbo.toImage(), blue ), eyebrows );

    // the woman's body, with her long hair
    scene.setFigureBody( 1, figures.model( 1 ), figures.paints( 1 ), figures.uniformCuts( 1 ) );
    f.look.body = 1; // the woman's body
    f.look.hair = bit( 1, "Hair_Long" );
    figures.pose( content, 0.0 );
    QCOMPARE( content.figure_bodies, std::vector< int >( { 1 } ) );
    scene.render( fbo.handle(), w, h, camera, content );
    const QImage woman = fbo.toImage();
    QVERIFY2( count( woman, blue ) > parted_hair, qPrintable( QString::number( count( woman, blue ) ) ) );
    QVERIFY( count( woman, red ) > 1000 );
    QVERIFY( count( woman, green ) > 300 );
    // and the man's not drawn
    QVERIFY( woman != parted );

    // from behind, her short hair: no scalp through its gaps (2026-11-14)
    {
        f.look.hair = bit( 1, "Hair_BuzzedFemale" );
        figures.pose( content, 0.0 );
        std::vector< QMatrix4x4 > her;
        figures.worldNodes( f, 0.0, her );
        const QVector3D head = her[figures.model( 1 ).nodeIndex( "Head" )].column( 3 ).toVector3D();
        CameraPose behind;
        behind.position = head + QVector3D( -1.0f, 0.05f, 0.0f );
        behind.target = head + QVector3D( 0.0f, 0.05f, 0.0f );
        behind.fov_y = 20.0f;
        const auto scalp = [&]() {
            scene.render( fbo.handle(), w, h, behind, content );
            const QImage img = fbo.toImage();
            // the back of the head: around its middle, above the ears
            const QMatrix4x4 vp = behind.projection( float( w ) / h ) * behind.view();
            const QVector3D ndc = vp.map( head + QVector3D( -0.05f, 0.1f, 0.0f ) );
            const QPoint at( int( ( ndc.x() + 1.0f ) / 2.0f * w ), int( ( 1.0f - ndc.y() ) / 2.0f * h ) );
            int n = 0;
            for ( int y = at.y() - 20; y <= at.y() + 20; ++y )
                for ( int x = at.x() - 25; x <= at.x() + 25; ++x )
                    if ( green( img.pixelColor( x, y ) ) ) ++n;
            return n;
        };
        QCOMPARE( scalp(), 0 );
        // without a hairstyle: the scalp
        f.look.hair = 0;
        QVERIFY2( scalp() > 500, qPrintable( QString::number( scalp() ) ) );
    }
    scene.release();
}

/*-------------------------------------------------------------------*/
/*!
  The sun's shadows seen from straight above: a player's body, a figure
  and a goal post darken the ground away from the sun, as far as the sun's
  rays through them reach it; the ground toward the sun and under the ball
  (which casts none: its disc tells its height) is as without the shadows.
  Turned off, the image is as before; turned on, the first frame is as the
  next (§11: 4).
*/
void
TestScene3D::shadows()
{
    DispHolder holder;
    FieldPainter painter( holder );
    Figures3D figures;
    QVERIFY2( figures.load(), qPrintable( figures.error() ) );
    Scene3D scene;
    QVERIFY( scene.initialize() );
    scene.setGround( GroundTexture::draw( painter ) );
    scene.setFigureBody( 0, figures.model(), figures.paints() );

    const int w = 400, h = 300;
    QOpenGLFramebufferObjectFormat fbo_format;
    fbo_format.setAttachment( QOpenGLFramebufferObject::Depth );
    QOpenGLFramebufferObject fbo( w, h, fbo_format );

    // a player at (-4, 0), the ball 2 m up at (0, -4), a figure at (4, 0)
    rcss::rcg::ShowInfoT show;
    rcss::rcg::PlayerT & p = show.player_[0];
    p.side_ = 'l';
    p.unum_ = 1;
    p.state_ = rcss::rcg::STAND;
    p.x_ = -4.0f;
    p.y_ = 0.0f;
    show.ball_.x_ = 0.0f;
    show.ball_.y_ = -4.0f;
    show.ball_.z_ = 2.0f;
    SceneContent content;
    SceneBuilder::addGoals( content );
    SceneBuilder::addShow( content, show, holder, Options::instance() );
    const float height = 1.85f;
    content.figures.push_back( FigureInstance{ 0, 4.0f, 0.0f, 0.0f, height, QVector4D( 1.0f, 0.0f, 0.0f, 1.0f ) } );
    figures.pose( content, 0.0 );

    // 30 m above the center spot
    CameraPose camera;
    camera.position = QVector3D( 0.0f, 30.0f, 0.0f );
    camera.target = QVector3D( 0.0f, 0.0f, 0.0f );
    camera.up = QVector3D( 0.0f, 0.0f, -1.0f );
    const QMatrix4x4 view_proj = camera.projection( float( w ) / h ) * camera.view();
    const auto pixel = [&]( const QVector3D & q ) {
        const QVector3D ndc = view_proj.map( q );
        return QPoint( int( ( ndc.x() + 1.0f ) / 2.0f * w ), int( ( 1.0f - ndc.y() ) / 2.0f * h ) );
    };
    // where the sun's ray through a point meets the ground, and the point
    // as far on the other side (toward the sun)
    const QVector3D sun = Sun3D::direction();
    const auto shade = [&]( const QVector3D & q ) { return q - sun * ( q.y() / sun.y() ); };
    const auto lit = [&]( const QVector3D & q ) { const QVector3D g( q.x(), 0.0f, q.z() ); return g - ( shade( q ) - g ); };

    content.shadows = false;
    scene.render( fbo.handle(), w, h, camera, content );
    const QImage plain = fbo.toImage();
    content.shadows = true;
    scene.render( fbo.handle(), w, h, camera, content );
    const QImage first = fbo.toImage();
    scene.render( fbo.handle(), w, h, camera, content );
    QCOMPARE( first, fbo.toImage() );

    const auto darker = [&]( const QVector3D & q ) {
        const QPoint at = pixel( q );
        const int with = first.pixelColor( at ).lightness();
        const int without = plain.pixelColor( at ).lightness();
        return with < without * 0.8 && with > without * 0.4; // as dark as SHADOW_OPACITY: not black
    };
    const auto same = [&]( const QVector3D & q ) {
        const QPoint at = pixel( q );
        return first.pixelColor( at ) == plain.pixelColor( at );
    };

    // the player's body, halfway up, and the figure's chest
    const QVector3D body( -4.0f, 0.8f, 0.0f );
    const QVector3D chest( 4.0f, 1.3f, 0.0f );
    for ( const QVector3D & q : { body, chest } )
    {
        QVERIFY2( darker( shade( q ) ), qPrintable( QString( "%1 %2: %3 %4" )
                                                    .arg( q.x() ).arg( q.y() )
                                                    .arg( first.pixelColor( pixel( shade( q ) ) ).name() )
                                                    .arg( plain.pixelColor( pixel( shade( q ) ) ).name() ) ) );
        QVERIFY( same( lit( q ) ) );
    }
    // beyond the shadow of the figure's head: lit
    QVERIFY( same( shade( QVector3D( 4.0f, height + 0.4f, 0.0f ) ) ) );
    // the ball casts none
    const float r = float( SceneBuilder::ballRadius( Options::instance(), holder.serverParam().ball_size_ ) );
    QVERIFY( same( shade( QVector3D( 0.0f, 2.0f + r, -4.0f ) ) ) );

    // a goal post seen from 10 m above, its shadow on the ground in front of
    // the goal: thin (the post is 12 cm), softened by the edges, lighter
    CameraPose goal = camera;
    goal.position = QVector3D( -52.5f, 10.0f, 7.0f );
    goal.target = QVector3D( -52.5f, 0.0f, 7.0f );
    const QMatrix4x4 goal_view_proj = goal.projection( float( w ) / h ) * goal.view();
    const QVector3D post( -52.5f, 1.2f, 7.01f ); // the right post (the field's +y), halfway up
    const QVector3D ndc = goal_view_proj.map( shade( post ) );
    const QPoint at( int( ( ndc.x() + 1.0f ) / 2.0f * w ), int( ( 1.0f - ndc.y() ) / 2.0f * h ) );
    content.shadows = false;
    scene.render( fbo.handle(), w, h, goal, content );
    const int goal_plain = fbo.toImage().pixelColor( at ).lightness();
    content.shadows = true;
    scene.render( fbo.handle(), w, h, goal, content );
    const int goal_shade = fbo.toImage().pixelColor( at ).lightness();
    QVERIFY2( goal_shade < goal_plain * 0.9, qPrintable( QString( "%1 %2" ).arg( goal_shade ).arg( goal_plain ) ) );

    // without the figure: no shadow of it left in the map
    SceneContent alone = content;
    alone.figures.clear();
    alone.figure_joints.clear();
    alone.figure_bodies.clear();
    scene.render( fbo.handle(), w, h, camera, alone );
    QCOMPARE( fbo.toImage().pixelColor( pixel( shade( chest ) ) ), plain.pixelColor( pixel( shade( chest ) ) ) );

    // off again: as before
    content.shadows = false;
    scene.render( fbo.handle(), w, h, camera, content );
    QCOMPARE( fbo.toImage(), plain );

    scene.release();
}

/*-------------------------------------------------------------------*/
/*!
  The simple shadows seen from straight above: a soft ellipse from the
  player's feet away from the sun, as dark as Sun3D::SHADOW_OPACITY in its
  middle, fading toward its edge, nothing beyond it nor toward the sun;
  the same in every frame.
*/
void
TestScene3D::blobShadows()
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
    show.ball_.x_ = 100.0f; // out of the view
    SceneContent content;
    SceneBuilder::addShow( content, show, holder, Options::instance() );
    content.discs.clear(); // without the circle under the player, to see the shadow alone
    QCOMPARE( content.picks.size(), size_t( 1 ) );

    CameraPose camera;
    camera.position = QVector3D( 0.0f, 20.0f, 0.0f );
    camera.target = QVector3D( 0.0f, 0.0f, 0.0f );
    camera.up = QVector3D( 0.0f, 0.0f, -1.0f );
    const QMatrix4x4 view_proj = camera.projection( float( w ) / h ) * camera.view();
    const auto pixel = [&]( const QVector3D & q ) {
        const QVector3D ndc = view_proj.map( q );
        return QPoint( int( ( ndc.x() + 1.0f ) / 2.0f * w ), int( ( 1.0f - ndc.y() ) / 2.0f * h ) );
    };

    scene.render( fbo.handle(), w, h, camera, content );
    const QImage plain = fbo.toImage();
    SceneBuilder::addBlobShadows( content );
    QCOMPARE( content.blob_shadows.size(), size_t( 1 ) );
    scene.render( fbo.handle(), w, h, camera, content );
    const QImage first = fbo.toImage();
    scene.render( fbo.handle(), w, h, camera, content );
    QCOMPARE( first, fbo.toImage() );

    // points of the ellipse (the unit disc of its model matrix)
    const QMatrix4x4 & m = content.blob_shadows[0].model;
    const auto ratio = [&]( const QVector3D & disc ) {
        const QPoint at = pixel( m.map( disc ) );
        return double( first.pixelColor( at ).lightness() ) / std::max( 1, plain.pixelColor( at ).lightness() );
    };
    // the middle, a meter past the feet: as dark as the shadows (0.6 of the light)
    const double middle = ratio( QVector3D( 0.2f, 0.0f, 0.0f ) );
    QVERIFY2( middle > 0.5 && middle < 0.7, qPrintable( QString::number( middle ) ) );
    // toward the edge, lighter; beyond it and toward the sun, as without
    const double fading = ratio( QVector3D( 0.0f, 0.0f, 0.8f ) );
    QVERIFY2( fading > middle + 0.1 && fading < 0.98, qPrintable( QString::number( fading ) ) );
    for ( const QVector3D & out : { QVector3D( 1.3f, 0.0f, 0.0f ), QVector3D( -1.6f, 0.0f, 0.0f ), QVector3D( 0.0f, 0.0f, 1.5f ) } )
    {
        const QPoint at = pixel( m.map( out ) );
        QCOMPARE( first.pixelColor( at ), plain.pixelColor( at ) );
    }

    scene.release();
}

/*-------------------------------------------------------------------*/
/*!
  The sky and the ground beyond, seen level from 50 m up: above the
  horizon the sky's colors (Scenery3D::skyColor()), below it the ground
  beyond in its color nearer than the haze, toward the horizon's color in
  the haze; without the sky, the plain background and no haze. The same in
  every frame.
*/
void
TestScene3D::sky()
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

    CameraPose camera;
    camera.position = QVector3D( 0.0f, 50.0f, 0.0f );
    camera.target = QVector3D( 100.0f, 50.0f, 0.0f ); // level along +x
    SceneContent content;
    content.sky = true;
    content.far_ground = Scenery3D::farGroundColor( Options::instance().fieldBrush().color() );

    scene.render( fbo.handle(), w, h, camera, content );
    const QImage image = fbo.toImage();
    scene.render( fbo.handle(), w, h, camera, content );
    QCOMPARE( image, fbo.toImage() );

    // the row of the middle column seen at an angle above (+) or below (-) level
    const double f = ( h / 2.0 ) / std::tan( camera.fov_y / 2.0 * M_PI / 180.0 );
    const auto row = [&]( const double degree ) { return int( std::lround( h / 2.0 - std::tan( degree * M_PI / 180.0 ) * f ) ); };
    const auto color = [&]( const QVector4D & c ) { return QColor::fromRgbF( c.x(), c.y(), c.z() ); };
    const auto mix = []( const QColor & a, const QColor & b, const double t ) {
        return QColor::fromRgbF( a.redF() * ( 1 - t ) + b.redF() * t, a.greenF() * ( 1 - t ) + b.greenF() * t,
                                 a.blueF() * ( 1 - t ) + b.blueF() * t );
    };
    const QColor far = color( content.far_ground );

    // the sky above
    for ( const double up : { 2.0, 8.0, 16.0 } )
    {
        const double a = up * M_PI / 180.0;
        const QColor expected = Scenery3D::skyColor( QVector3D( float( std::cos( a ) ), float( std::sin( a ) ), 0.0f ) );
        const QColor c = image.pixelColor( w / 2, row( up ) );
        QVERIFY2( distance( c, expected ) < 10, qPrintable( QString( "%1: %2 %3" ).arg( up ).arg( c.name(), expected.name() ) ) );
    }
    // the ground beyond, 300 m on: its color; 1000 m on, in the haze
    const double at300 = std::atan( 50.0 / 300.0 ) * 180.0 / M_PI;
    const double at1000 = std::atan( 50.0 / 1000.0 ) * 180.0 / M_PI;
    QVERIFY2( distance( image.pixelColor( w / 2, row( -at300 ) ), far ) < 8,
              qPrintable( image.pixelColor( w / 2, row( -at300 ) ).name() ) );
    const QColor hazy = mix( far, Scenery3D::SKY_HORIZON, Scenery3D::haze( std::hypot( 1000.0f, 50.0f ) ) );
    QVERIFY2( distance( image.pixelColor( w / 2, row( -at1000 ) ), hazy ) < 24, // a pixel near the horizon spans many meters
              qPrintable( image.pixelColor( w / 2, row( -at1000 ) ).name() + " " + hazy.name() ) );
    QVERIFY( distance( hazy, far ) > 100 ); // the haze shows

    // without the sky: the plain background above, no haze below
    content.sky = false;
    scene.render( fbo.handle(), w, h, camera, content );
    const QImage plain = fbo.toImage();
    QVERIFY( distance( plain.pixelColor( w / 2, row( 8.0 ) ), Scenery3D::CLEAR ) <= 3 );
    QVERIFY2( distance( plain.pixelColor( w / 2, row( -at1000 ) ), far ) < 8,
              qPrintable( plain.pixelColor( w / 2, row( -at1000 ) ).name() ) );

    scene.release();
}

/*-------------------------------------------------------------------*/
/*!
  The realistic grass of the ground shader (the patches and the wear are
  painted into the ground image: tests grass_3d, ground_texture): the
  stripes by the way the camera looks (a point of an even stripe lighter
  from the near side than from the far side, and of an odd one darker),
  and the blades near the camera only.
*/
void
TestScene3D::realisticGrass()
{
    DispHolder holder;
    FieldPainter painter( holder );
    Scene3D scene;
    QVERIFY( scene.initialize() );
    scene.setGround( GroundTexture::draw( painter ) ); // plain (initTestCase)

    const int w = 400, h = 300;
    QOpenGLFramebufferObjectFormat fbo_format;
    fbo_format.setAttachment( QOpenGLFramebufferObject::Depth );
    QOpenGLFramebufferObject fbo( w, h, fbo_format );

    const auto shot = [&]( const CameraPose & camera, const bool realistic ) {
        SceneContent content;
        content.realistic_grass = realistic;
        scene.render( fbo.handle(), w, h, camera, content );
        return fbo.toImage();
    };
    const auto pixel = [&]( const CameraPose & camera, const QVector3D & q ) {
        const QVector3D ndc = ( camera.projection( float( w ) / h ) * camera.view() ).map( q );
        return QPoint( int( ( ndc.x() + 1.0f ) / 2.0f * w ), int( ( 1.0f - ndc.y() ) / 2.0f * h ) );
    };
    // the mean lightness around a point of the ground, realistic over plain
    const auto ratio = [&]( const CameraPose & camera, const QVector3D & q ) {
        const QImage real = shot( camera, true );
        const QImage plain = shot( camera, false );
        const QPoint at = pixel( camera, q );
        double a = 0.0, b = 0.0;
        for ( int dy = -1; dy <= 1; ++dy )
        {
            for ( int dx = -1; dx <= 1; ++dx )
            {
                a += real.pixelColor( at + QPoint( dx, dy ) ).lightness();
                b += plain.pixelColor( at + QPoint( dx, dy ) ).lightness();
            }
        }
        return a / b;
    };

    // the stripes: from the near side (+z) and the far side, mirrored
    CameraPose near, far;
    near.position = QVector3D( 0.0f, 30.0f, 60.0f );
    far.position = QVector3D( 0.0f, 30.0f, -60.0f );
    near.target = far.target = QVector3D( 0.0f, 0.0f, 0.0f );
    const float sw = float( FieldPainter::GRASS_STRIPE_WIDTH );
    const QVector3D even( 2.5f * sw, 0.0f, 0.0f ), odd( 3.5f * sw, 0.0f, 0.0f );
    const double even_near = ratio( near, even ), even_far = ratio( far, even );
    const double odd_near = ratio( near, odd ), odd_far = ratio( far, odd );
    QVERIFY2( even_near > 1.03 && odd_near < 0.97, qPrintable( QString( "%1 %2" ).arg( even_near ).arg( odd_near ) ) );
    QVERIFY2( even_far < 0.995 && odd_far > 1.005, qPrintable( QString( "%1 %2" ).arg( even_far ).arg( odd_far ) ) );

    // the blades: within a stripe, seen straight down from near (8 m) they
    // show, from far (40 m) they do not (the plain ground is one color)
    const auto spread = [&]( const float height ) {
        CameraPose above;
        above.position = QVector3D( 4.5f * sw, height, -20.0f );
        above.target = QVector3D( 4.5f * sw, 0.0f, -20.0f );
        above.up = QVector3D( 0.0f, 0.0f, -1.0f );
        const QImage image = shot( above, true );
        double mean = 0.0, sq = 0.0;
        int n = 0;
        for ( int y = h / 2 - 30; y < h / 2 + 30; y += 2 )
        {
            for ( int x = w / 2 - 30; x < w / 2 + 30; x += 2 )
            {
                const double l = image.pixelColor( x, y ).lightness();
                mean += l; sq += l * l; ++n;
            }
        }
        mean /= n;
        return std::sqrt( std::max( 0.0, sq / n - mean * mean ) );
    };
    const double near_spread = spread( 8.0f );
    const double far_spread = spread( 40.0f );
    QVERIFY2( near_spread > 1.5, qPrintable( QString::number( near_spread ) ) );
    QVERIFY2( far_spread < 0.8, qPrintable( QString::number( far_spread ) ) );
    // faded out toward BLADES_FAR, not cut there
    const double fading_spread = spread( Grass3D::BLADES_FAR - 2.0f );
    QVERIFY2( fading_spread < 0.8, qPrintable( QString::number( fading_spread ) ) );

    scene.release();
}

/*-------------------------------------------------------------------*/
/*!
  The stands: drawn beyond the far touch line seen from the pitch; a stand
  with the camera inside it is not drawn (the center spot as without the
  stands); from behind a stand, its back wall (not see-through).
*/
void
TestScene3D::stands()
{
    DispHolder holder;
    FieldPainter painter( holder );
    Scene3D scene;
    QVERIFY( scene.initialize() );
    scene.setGround( GroundTexture::draw( painter ) );
    Stadium3D::TeamLook left, right;
    left.color = Qt::red;
    left.name = "L";
    right.color = Qt::blue;
    right.name = "R";
    for ( int i = 0; i < Stands3D::STAND_COUNT; ++i )
    {
        scene.setStands( i, Stands3D::image( i, left, right ) );
    }

    const int w = 400, h = 300;
    QOpenGLFramebufferObjectFormat fbo_format;
    fbo_format.setAttachment( QOpenGLFramebufferObject::Depth );
    QOpenGLFramebufferObject fbo( w, h, fbo_format );
    const auto shot = [&]( const CameraPose & camera, const bool stands ) {
        SceneContent content;
        content.stands = stands;
        content.sky = true;
        scene.render( fbo.handle(), w, h, camera, content );
        return fbo.toImage();
    };

    // from the pitch, the far stand ahead
    CameraPose look;
    look.position = QVector3D( 0.0f, 3.0f, 0.0f );
    look.target = QVector3D( 0.0f, 5.0f, -( Stands3D::stand( 0 ).front + 6.0f ) );
    const QImage with = shot( look, true );
    QCOMPARE( shot( look, true ), with );
    const QColor seen = with.pixelColor( w / 2, h / 2 );
    QVERIFY2( distance( seen, shot( look, false ).pixelColor( w / 2, h / 2 ) ) > 30, qPrintable( seen.name() ) );

    // inside the near stand, under its rows: from there the treads of the
    // lower rows face up to the camera (through the risers, seen from
    // behind); the stand is not drawn, the pitch beyond as without it
    const float front = Stands3D::stand( 1 ).front;
    CameraPose inside;
    inside.position = QVector3D( 0.0f, 2.4f, front + 8.0f );
    inside.target = QVector3D( 0.0f, Stands3D::WALL + Stands3D::RISER, front + 1.3f ); // the second row's tread
    QVERIFY( Stands3D::insideStand( 1, inside.position ) );
    QCOMPARE( shot( inside, true ).pixelColor( w / 2, h / 2 ), shot( inside, false ).pixelColor( w / 2, h / 2 ) );

    // behind the near stand, low: its back wall in the way (not see-through)
    CameraPose behind;
    behind.position = QVector3D( 0.0f, 3.0f, Stands3D::stand( 1 ).front + Stands3D::depth() + 15.0f );
    behind.target = QVector3D( 0.0f, 0.0f, 0.0f );
    QVERIFY( ! Stands3D::insideStand( 1, behind.position ) );
    const QColor wall = shot( behind, true ).pixelColor( w / 2, h / 2 );
    QVERIFY2( distance( wall, shot( behind, false ).pixelColor( w / 2, h / 2 ) ) > 30, qPrintable( wall.name() ) );
    QVERIFY( wall.saturation() < 60 ); // the concrete, not the grass

    scene.release();
}

/*-------------------------------------------------------------------*/
/*!
  The ball's pattern, seen from straight above without perspective: each
  pixel of the ball dark where Ball3D::patternAt() says, at the direction
  on the ball itself (turned back by how it rolled), and nowhere dark
  without the pattern; the same in every frame.
*/
void
TestScene3D::ball()
{
    DispHolder holder;
    FieldPainter painter( holder );
    Scene3D scene;
    QVERIFY( scene.initialize() );
    scene.setGround( GroundTexture::draw( painter ) );

    const int w = 240, h = 240;
    QOpenGLFramebufferObjectFormat fbo_format;
    fbo_format.setAttachment( QOpenGLFramebufferObject::Depth );
    QOpenGLFramebufferObject fbo( w, h, fbo_format );

    const float r = 0.5f; // a large ball, for many pixels
    CameraPose camera;
    camera.position = QVector3D( 0.0f, 10.0f, 0.0f );
    camera.target = QVector3D( 0.0f, 0.0f, 0.0f );
    camera.up = QVector3D( 0.0f, 0.0f, -1.0f ); // the screen's right +x, its down +z
    camera.ortho_half_height = 1.2f * r;

    const auto shot = [&]( const QQuaternion & turn, const bool pattern ) {
        SceneContent content;
        Instance3D ball;
        ball.model.translate( 0.0f, r, 0.0f );
        ball.model.rotate( turn );
        ball.model.scale( r );
        ball.color = QVector4D( 1.0f, 1.0f, 1.0f, 1.0f );
        content.balls.push_back( ball );
        content.ball_pattern = pattern;
        scene.render( fbo.handle(), w, h, camera, content );
        return fbo.toImage();
    };

    // the share of the pixels of the ball (within 0.85 of its radius) dark
    // where patternAt() says, and the share dark at all
    const auto agreement = [&]( const QImage & image, const QQuaternion & turn, double * dark ) {
        int agree = 0, count = 0, darks = 0;
        for ( int y = 0; y < h; ++y )
        {
            for ( int x = 0; x < w; ++x )
            {
                const float wx = ( x + 0.5f - w * 0.5f ) / ( h * 0.5f ) * camera.ortho_half_height / r;
                const float wz = ( y + 0.5f - h * 0.5f ) / ( h * 0.5f ) * camera.ortho_half_height / r;
                if ( wx * wx + wz * wz > 0.85f * 0.85f ) continue;
                const QVector3D world( wx, std::sqrt( 1.0f - wx * wx - wz * wz ), wz );
                const bool expected = Ball3D::patternAt( turn.conjugated().rotatedVector( world ) ) < 0.5f;
                const bool seen = image.pixelColor( x, y ).lightness() < 100;
                if ( seen == expected ) ++agree;
                if ( seen ) ++darks;
                ++count;
            }
        }
        *dark = double( darks ) / count;
        return double( agree ) / count;
    };

    const QQuaternion turn = QQuaternion::fromAxisAndAngle( 1.0f, 2.0f, 3.0f, 50.0f );
    for ( const QQuaternion & q : { QQuaternion(), turn } )
    {
        const QImage patterned = shot( q, true );
        QCOMPARE( shot( q, true ), patterned );
        double dark = 0.0;
        const double agree = agreement( patterned, q, &dark );
        QVERIFY2( agree > 0.97, qPrintable( QString::number( agree ) ) );
        QVERIFY2( dark > 0.15 && dark < 0.5, qPrintable( QString::number( dark ) ) );
        // plain: nothing dark on it
        agreement( shot( q, false ), q, &dark );
        QCOMPARE( dark, 0.0 );
    }
    // the turn shows: the turned ball does not look as the one not turned
    double dark = 0.0;
    QVERIFY( agreement( shot( turn, true ), QQuaternion(), &dark ) < 0.9 );

    scene.release();
}

/*-------------------------------------------------------------------*/
/*!
  The ring and the trail seen from straight above without perspective: the
  ring's line light, its fill faint, nothing outside it; the trail light
  along its middle, fainter to its edges and to its tail, nothing beside
  it; the same in every frame.
*/
void
TestScene3D::ballAids()
{
    DispHolder holder;
    FieldPainter painter( holder );
    Scene3D scene;
    QVERIFY( scene.initialize() );
    scene.setGround( GroundTexture::draw( painter ) );

    const int w = 200, h = 200;
    QOpenGLFramebufferObjectFormat fbo_format;
    fbo_format.setAttachment( QOpenGLFramebufferObject::Depth );
    QOpenGLFramebufferObject fbo( w, h, fbo_format );

    // 4 m across the screen around (20, 20), off the lines: 0.02 m a pixel
    const QVector3D at( 20.0f, 0.0f, 20.0f );
    CameraPose camera;
    camera.position = at + QVector3D( 0.0f, 10.0f, 0.0f );
    camera.target = at;
    camera.up = QVector3D( 0.0f, 0.0f, -1.0f ); // the screen's right +x, its down +z
    camera.ortho_half_height = 2.0f;
    const float mpp = BallAids3D::metersPerPixel( camera, at, float( h ) );
    QVERIFY( std::fabs( mpp - 0.02f ) < 1.0e-6f );
    const auto pixel = [&]( const QImage & image, const float dx, const float dz ) {
        return image.pixelColor( int( w * 0.5f + dx / mpp ), int( h * 0.5f + dz / mpp ) );
    };
    const auto shot = [&]( const SceneContent & content ) {
        scene.render( fbo.handle(), w, h, camera, content );
        return fbo.toImage();
    };
    const QImage bare = shot( SceneContent() );

    // the ring, 1 m: its line 0.06 m (3 pixels) inside the radius
    SceneContent ring;
    BallAids3D::addRing( ring, at, 1.0f, mpp, Qt::white );
    const QImage with_ring = shot( ring );
    QCOMPARE( shot( ring ), with_ring );
    for ( const QPointF & way : { QPointF( 1, 0 ), QPointF( 0, -1 ), QPointF( -0.6, 0.8 ) } )
    {
        const QColor line = pixel( with_ring, 0.97f * way.x(), 0.97f * way.y() );
        QVERIFY2( line.lightness() > pixel( bare, 0.97f * way.x(), 0.97f * way.y() ).lightness() + 100, qPrintable( line.name() ) );
        const QColor fill = pixel( with_ring, 0.5f * way.x(), 0.5f * way.y() );
        const int d = distance( fill, pixel( bare, 0.5f * way.x(), 0.5f * way.y() ) );
        QVERIFY2( d > 5 && d < 90, qPrintable( fill.name() ) );
        QCOMPARE( pixel( with_ring, 1.15f * way.x(), 1.15f * way.y() ), pixel( bare, 1.15f * way.x(), 1.15f * way.y() ) );
    }

    // the trail along x: its head at +1.5 m, 0.8 m wide there, 5 cycles to -1.5 m
    std::vector< BallAids3D::TrailPoint > points;
    for ( int k = 0; k <= 5; ++k )
    {
        points.push_back( BallAids3D::TrailPoint{ at + QVector3D( 1.5f - 0.6f * k, 0.1f, 0.0f ), float( k ) } );
    }
    SceneContent trail;
    BallAids3D::addTrail( trail, points, camera.position, 0.8f, 5, Qt::white );
    const QImage with_trail = shot( trail );
    QCOMPARE( shot( trail ), with_trail );
    const auto lift = [&]( const float dx, const float dz ) {
        return pixel( with_trail, dx, dz ).lightness() - pixel( bare, dx, dz ).lightness();
    };
    QVERIFY2( lift( 1.2f, 0.0f ) > 60, qPrintable( QString::number( lift( 1.2f, 0.0f ) ) ) );
    QVERIFY( lift( 1.2f, 0.0f ) > lift( 1.2f, 0.25f ) ); // fainter to the edges
    QVERIFY( lift( 1.2f, 0.25f ) > 0 );
    QVERIFY( lift( 1.2f, 0.0f ) > lift( -0.6f, 0.0f ) + 20 ); // and to the tail
    QCOMPARE( pixel( with_trail, 1.2f, 0.6f ), pixel( bare, 1.2f, 0.6f ) ); // nothing beside it
    QCOMPARE( pixel( with_trail, 1.8f, 0.0f ), pixel( bare, 1.8f, 0.0f ) ); // nor ahead

    scene.release();
}

/*-------------------------------------------------------------------*/
/*!
  The first frame after switching to the top camera is the same as the
  next one (docs/PLAN_3D-VIEW.md §11: 4; in the web version, the first one
  drew the cylinders at the origin), and so after switching back, with
  everything of Phase 8 on: frames of a log built by FrameBuilder3D.
*/
void
TestScene3D::topSwitch()
{
    DispHolder holder;
    for ( int f = 0; f < 10; ++f )
    {
        rcss::rcg::ShowInfoT show;
        show.time_ = f;
        show.ball_.x_ = 0.5f * f;
        show.ball_.y_ = 1.0f;
        for ( int i = 0; i < rcss::rcg::MAX_PLAYER * 2; ++i )
        {
            rcss::rcg::PlayerT & p = show.player_[i];
            const bool left = ( i < rcss::rcg::MAX_PLAYER );
            p.side_ = ( left ? 'l' : 'r' );
            p.unum_ = static_cast< rcss::rcg::Int16 >( i % rcss::rcg::MAX_PLAYER + 1 );
            p.state_ = rcss::rcg::STAND;
            p.x_ = ( left ? -1.0f : 1.0f ) * ( 5.0f + 4.0f * float( i % rcss::rcg::MAX_PLAYER ) ) + 0.3f * f;
            p.y_ = -20.0f + 3.5f * float( i % rcss::rcg::MAX_PLAYER );
            p.body_ = ( left ? 0.0f : 180.0f );
            p.stamina_ = 8000.0f;
        }
        holder.handleShow( show );
    }
    Options & opt = Options::instance();
    opt.setPlayerModel3D( PLAYER_MODEL_MANNEQUIN );
    opt.setShadows3D( ShadowMode::Detailed );
    opt.setGrass3D( GrassMode::Realistic );

    FieldPainter painter( holder );
    Scene3D scene;
    QVERIFY( scene.initialize() );
    scene.setGround( GroundTexture::draw( painter ) );

    const int w = 320, h = 240;
    QOpenGLFramebufferObjectFormat fbo_format;
    fbo_format.setAttachment( QOpenGLFramebufferObject::Depth );
    QOpenGLFramebufferObject fbo( w, h, fbo_format );

    FrameBuilder3D builder;
    Interpolate::Position p;
    p.index = 5;
    p.next = 6;
    p.alpha = 0.4;
    const auto shot = [&]( const CameraMode mode, const CameraPose & camera ) {
        SceneContent content;
        builder.build( holder, opt, p, mode, content );
        builder.addBallAids( holder, opt, camera, float( h ), content );
        if ( ! content.figures.empty() )
        {
            const Figures3D & figures = builder.figures();
            for ( int k = 0; k < figures.bodyCount(); ++k )
            {
                scene.setFigureBody( k, figures.model( k ), figures.paints( k ), figures.uniformCuts( k ) );
            }
        }
        scene.render( fbo.handle(), w, h, camera, content );
        return fbo.toImage();
    };

    const CameraPose broadcast = Cameras3D::broadcast( float( w ) / h, 2.5f, 1.0f );
    const CameraPose top = Cameras3D::top( float( w ) / h );
    const QImage before = shot( CameraMode::Broadcast, broadcast );
    QCOMPARE( shot( CameraMode::Broadcast, broadcast ), before );
    const QImage first = shot( CameraMode::Top, top );
    QCOMPARE( shot( CameraMode::Top, top ), first );
    QVERIFY( first != before );
    QCOMPARE( shot( CameraMode::Broadcast, broadcast ), before );

    opt.setPlayerModel3D( Options::PLAYER_MODEL_3D );
    opt.setShadows3D( Options::SHADOWS_3D );
    opt.setGrass3D( GrassMode::Plain );
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
