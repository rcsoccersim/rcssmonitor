// -*-c++-*-

/*!
  \file test_ground_texture.cpp
  \brief unit test of the ground of the 3D view
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
#include "ground_texture.h"
#include "options.h"

#include <rcss/rcg/types.h>

#include <cmath>

/*
  Options reads and writes ~/.rcssmonitor.conf; CTest runs this test with
  HOME in the build tree (test/CMakeLists.txt).
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
  \brief unit test of the ground of the 3D view
*/
class TestGroundTexture
    : public QObject {
    Q_OBJECT

private slots:

    void image();
    void stripes();
    void key();
};

/*-------------------------------------------------------------------*/
/*!
  The pitch and its margins at 24 px/m, the pitch lines 3 px wide, no
  goals, and the mapping of the 2D view kept.
*/
void
TestGroundTexture::image()
{
    DispHolder holder;
    FieldPainter painter( holder );
    Options & opt = Options::instance();
    const Options::FieldTransform before = opt.fieldTransform();

    opt.setGrass3D( GrassMode::Plain ); // the plain field color
    const QImage image = GroundTexture::draw( painter );
    opt.setGrass3D( Options::GRASS_3D );
    QCOMPARE( image.size(), QSize( 115 * 24, 78 * 24 ) );

    const Options::FieldTransform after = opt.fieldTransform();
    QCOMPARE( after.scale, before.scale );
    QCOMPARE( after.center, before.center );

    const QColor field = opt.fieldBrush().color();
    const QColor line = opt.linePen().color();
    const auto at = [&]( double x, double y ) // field coordinates
        {
            return image.pixelColor( int( std::lround( image.width() / 2 + x * 24 ) ),
                                     int( std::lround( image.height() / 2 + y * 24 ) ) );
        };

    // the grass, the center line, and where the left goal would be
    QVERIFY( distance( at( 10.0, -10.0 ), field ) < 6 );
    QVERIFY( distance( at( 0.0, -20.0 ), line ) < 60 );
    QVERIFY( distance( at( -53.5, 0.0 ), field ) < 6 );

    // the width of the center line: about 3 px
    const int row = image.height() / 2 - 20 * 24;
    int wide = 0;
    for ( int x = image.width() / 2 - 10; x <= image.width() / 2 + 10; ++x )
    {
        if ( distance( image.pixelColor( x, row ), field ) > distance( field, line ) / 2 ) ++wide;
    }
    QVERIFY2( 2 <= wide && wide <= 4, qPrintable( QString::number( wide ) ) );
}

/*-------------------------------------------------------------------*/
/*!
  The grass mowed in stripes by the option: 5.25 m wide across the pitch,
  light from the halfway line to the right, dark next, and so on both
  ways, over the margins too; the lines over them; not on the minimap.
*/
void
TestGroundTexture::stripes()
{
    DispHolder holder;
    FieldPainter painter( holder );
    Options & opt = Options::instance();
    QCOMPARE( opt.grass3D(), GrassMode::Stripes ); // by default
    const QImage image = GroundTexture::draw( painter );
    const QColor field = opt.fieldBrush().color();
    const auto times = [&]( const double k ) {
        return QColor( int( std::lround( field.red() * k ) ), int( std::lround( field.green() * k ) ),
                       int( std::lround( field.blue() * k ) ) );
    };
    const QColor light = times( FieldPainter::GRASS_LIGHT );
    const QColor dark = times( FieldPainter::GRASS_DARK );
    const auto at = [&]( double x, double y ) // field coordinates
        {
            return image.pixelColor( int( std::lround( image.width() / 2 + x * 24 ) ),
                                     int( std::lround( image.height() / 2 + y * 24 ) ) );
        };
    const double w = FieldPainter::GRASS_STRIPE_WIDTH;
    QCOMPARE( w, 5.25 );
    // the middles of the stripes (away from the lines), to the right and the left
    for ( int k = -10; k < 10; ++k )
    {
        const QColor expected = ( k % 2 != 0 ? dark : light );
        const QColor c = at( ( k + 0.5 ) * w, -25.0 );
        QVERIFY2( distance( c, expected ) <= 3, qPrintable( QString( "%1: %2" ).arg( k ).arg( c.name() ) ) );
    }
    // in the margins too: beyond the touch line, and beyond the goal line
    QVERIFY( distance( at( 0.5 * w, -36.0 ), light ) <= 3 );
    QVERIFY( distance( at( -55.0, 20.0 ), dark ) <= 3 ); // k = -11
    // the edges: the light stripe right of the halfway line ends 5.25 m on
    QVERIFY( distance( at( w - 0.1, -25.0 ), light ) <= 3 );
    QVERIFY( distance( at( w + 0.1, -25.0 ), dark ) <= 3 );
    // the lines over them: the center line, the touch line
    const QColor line = opt.linePen().color();
    QVERIFY( distance( at( 0.0, -20.0 ), line ) < 60 );
    QVERIFY( distance( at( 3.0, -34.0 ), line ) < 60 );

    // plain, and realistic (whose stripes the ground shader draws): the plain field color
    for ( const GrassMode mode : { GrassMode::Plain, GrassMode::Realistic } )
    {
        opt.setGrass3D( mode );
        QVERIFY( distance( GroundTexture::draw( painter ).pixelColor( image.width() / 2 + int( 1.5 * w * 24 ), 10 ), field ) <= 3 );
    }
    opt.setGrass3D( GrassMode::Stripes );
    // realistic: the patches and the wear painted in (Grass3D::paintGround()):
    // a goal mouth redder, the lines still light
    opt.setGrass3D( GrassMode::Realistic );
    const QImage real = GroundTexture::draw( painter );
    opt.setGrass3D( GrassMode::Stripes );
    const auto real_at = [&]( double x, double y ) {
        return real.pixelColor( int( std::lround( real.width() / 2 + x * 24 ) ), int( std::lround( real.height() / 2 + y * 24 ) ) );
    };
    QVERIFY2( real_at( -49.5, 2.0 ).red() > field.red() + 15, qPrintable( real_at( -49.5, 2.0 ).name() ) );
    QVERIFY( distance( real_at( 0.0, -20.0 ), line ) < 90 );
    // the minimap: no stripes
    const QImage minimap = GroundTexture::draw( painter, 4.0, 1.0 );
    QVERIFY( distance( minimap.pixelColor( minimap.width() / 2 + int( 1.5 * w * 4 ), 6 ), field ) <= 3 );
}

/*-------------------------------------------------------------------*/
/*!
  The image is drawn again when the colors or the keepaway area change.
*/
void
TestGroundTexture::key()
{
    rcss::rcg::ServerParamT param;
    Options & opt = Options::instance();
    const QString first = GroundTexture::key( param );
    QCOMPARE( GroundTexture::key( param ), first );

    const QColor line = opt.linePen().color();
    opt.setLineColor( Qt::red );
    QVERIFY( GroundTexture::key( param ) != first );
    opt.setLineColor( line );
    QCOMPARE( GroundTexture::key( param ), first );

    for ( const GrassMode mode : { GrassMode::Plain, GrassMode::Realistic } )
    {
        opt.setGrass3D( mode );
        QVERIFY( GroundTexture::key( param ) != first );
    }
    opt.setGrass3D( Options::GRASS_3D );
    QCOMPARE( GroundTexture::key( param ), first );

    param.keepaway_mode_ = ! param.keepaway_mode_;
    QVERIFY( GroundTexture::key( param ) != first );
}

QTEST_MAIN( TestGroundTexture )

#include "test_ground_texture.moc"
