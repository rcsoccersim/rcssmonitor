// -*-c++-*-

/*!
  \file test_export_timing.cpp
  \brief unit test of ExportTiming: the timing of the export
*/

/*
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
*/

/////////////////////////////////////////////////////////////////////

#include <QtTest>

#include "export_timing.h"

/*
  docs/plan/2026-11-22-03_frame-export.md 4.3, §6.
*/

/*!
  \brief unit test of ExportTiming
*/
class TestExportTiming
    : public QObject {
    Q_OBJECT

private slots:

    void framesPerImage();
    void imageCount_data();
    void imageCount();
    void frames2D_data();
    void frames2D();
    void duration();
    void names();
};

/*-------------------------------------------------------------------*/
void
TestExportTiming::framesPerImage()
{
    // 2D: 10 fps at 1x, one frame an image
    QCOMPARE( ExportTiming::framesPerImage( 1.0, 100, 10.0 ), 1.0 );
    QCOMPARE( ExportTiming::DEFAULT_FPS_2D, 10.0 );
    // 3D: 30 and 60 fps
    QCOMPARE( ExportTiming::framesPerImage( 1.0, 100, 30.0 ), 1.0 / 3.0 );
    QCOMPARE( ExportTiming::framesPerImage( 1.0, 100, 60.0 ), 1.0 / 6.0 );
    // speeds
    QCOMPARE( ExportTiming::framesPerImage( 2.0, 100, 10.0 ), 2.0 );
    QCOMPARE( ExportTiming::framesPerImage( 0.5, 100, 10.0 ), 0.5 );
    // a step other than 100 ms
    QCOMPARE( ExportTiming::framesPerImage( 1.0, 50, 10.0 ), 2.0 );
    // not positive
    QCOMPARE( ExportTiming::framesPerImage( 0.0, 100, 10.0 ), 0.0 );
    QCOMPARE( ExportTiming::framesPerImage( 1.0, 0, 10.0 ), 0.0 );
    QCOMPARE( ExportTiming::framesPerImage( 1.0, 100, 0.0 ), 0.0 );
}

/*-------------------------------------------------------------------*/
void
TestExportTiming::imageCount_data()
{
    QTest::addColumn< int >( "first" );
    QTest::addColumn< int >( "last" );
    QTest::addColumn< double >( "per_image" );
    QTest::addColumn< int >( "count" );

    QTest::newRow( "one frame an image" ) << 0 << 5999 << 1.0 << 6000;
    QTest::newRow( "one frame" ) << 10 << 10 << 1.0 << 1;
    QTest::newRow( "30 fps" ) << 0 << 10 << 1.0 / 3.0 << 31;
    QTest::newRow( "60 fps" ) << 0 << 10 << 1.0 / 6.0 << 61;
    QTest::newRow( "2x" ) << 0 << 10 << 2.0 << 6;
    QTest::newRow( "2x, odd" ) << 0 << 9 << 2.0 << 5;
    QTest::newRow( "0.5x" ) << 0 << 10 << 0.5 << 21;
    QTest::newRow( "reversed" ) << 10 << 9 << 1.0 << 0;
    QTest::newRow( "no rate" ) << 0 << 10 << 0.0 << 0;
}

/*-------------------------------------------------------------------*/
void
TestExportTiming::imageCount()
{
    QFETCH( int, first );
    QFETCH( int, last );
    QFETCH( double, per_image );
    QFETCH( int, count );

    QCOMPARE( ExportTiming::imageCount( first, last, per_image ), std::size_t( count ) );
}

/*-------------------------------------------------------------------*/
void
TestExportTiming::frames2D_data()
{
    QTest::addColumn< int >( "first" );
    QTest::addColumn< int >( "last" );
    QTest::addColumn< double >( "per_image" );
    QTest::addColumn< QString >( "frames" );

    QTest::newRow( "one frame an image" ) << 3 << 6 << 1.0 << "3 4 5 6";
    QTest::newRow( "2x" ) << 3 << 8 << 2.0 << "3 5 7";
    QTest::newRow( "0.5x: each frame twice" ) << 3 << 5 << 0.5 << "3 3 4 4 5";
    QTest::newRow( "30 fps" ) << 0 << 1 << 1.0 / 3.0 << "0 0 0 1";
    QTest::newRow( "none" ) << 5 << 4 << 1.0 << "";
}

/*-------------------------------------------------------------------*/
void
TestExportTiming::frames2D()
{
    QFETCH( int, first );
    QFETCH( int, last );
    QFETCH( double, per_image );
    QFETCH( QString, frames );

    QStringList list;
    for ( std::size_t f : ExportTiming::frames2D( first, last, per_image ) ) list << QString::number( f );
    QCOMPARE( list.join( ' ' ), frames );

    // the position from n, not added up: the last image of a long range
    const std::vector< std::size_t > game = ExportTiming::frames2D( 0, 6000, 1.0 / 3.0 );
    QCOMPARE( game.size(), std::size_t( 18001 ) );
    QCOMPARE( game.back(), std::size_t( 6000 ) );
    QCOMPARE( game[17999], std::size_t( 5999 ) );
}

/*-------------------------------------------------------------------*/
void
TestExportTiming::duration()
{
    QCOMPARE( ExportTiming::duration( 6000, 10.0 ), 600.0 );
    QCOMPARE( ExportTiming::duration( 18001, 30.0 ), 18001 / 30.0 );
    QCOMPARE( ExportTiming::duration( 10, 0.0 ), 0.0 );
}

/*-------------------------------------------------------------------*/
//! the names of the images: the numbers from 0, at least 5 digits
void
TestExportTiming::names()
{
    QCOMPARE( ExportTiming::nameDigits( 0 ), 5 );
    QCOMPARE( ExportTiming::nameDigits( 100000 ), 5 ); // 0 to 99999
    QCOMPARE( ExportTiming::nameDigits( 100001 ), 6 );
    QCOMPARE( ExportTiming::imageName( "image-", 42, 5, "png" ), QString( "image-00042.png" ) );
    QCOMPARE( ExportTiming::imageName( "", 123456, 6, "jpg" ), QString( "123456.jpg" ) );
}

QTEST_GUILESS_MAIN( TestExportTiming )
#include "test_export_timing.moc"
