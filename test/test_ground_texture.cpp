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

    const QImage image = GroundTexture::draw( painter );
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

    param.keepaway_mode_ = ! param.keepaway_mode_;
    QVERIFY( GroundTexture::key( param ) != first );
}

QTEST_MAIN( TestGroundTexture )

#include "test_ground_texture.moc"
