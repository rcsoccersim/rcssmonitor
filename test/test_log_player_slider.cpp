// -*-c++-*-

/*!
  \file test_log_player_slider.cpp
  \brief unit test of LogPlayerSlider: the bands of the ranges and the goals
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

#include "log_player_slider.h"
#include "ui_palette.h"

/*
  docs/plan/2026-11-22-03_frame-export.md 4.4, D9. CTest runs this test
  with the offscreen platform.
*/

namespace {

//! whether a pixel is near a color (the bands are translucent)
bool
near( const QColor & a,
      const QColor & b,
      const int margin )
{
    return std::abs( a.red() - b.red() ) <= margin
        && std::abs( a.green() - b.green() ) <= margin
        && std::abs( a.blue() - b.blue() ) <= margin;
}

}

/*!
  \brief unit test of the slider's marks
*/
class TestLogPlayerSlider
    : public QObject {
    Q_OBJECT

private slots:

    void positions();
    void marks();
};

/*-------------------------------------------------------------------*/
//! the positions of the frames along the groove: in order, the ends at the ends
void
TestLogPlayerSlider::positions()
{
    LogPlayerSlider slider;
    slider.resize( 400, 16 );
    slider.updateIndex( 0, 101 );
    QCOMPARE( slider.maximum(), 100 );
    const int a = slider.positionOf( 0 );
    const int m = slider.positionOf( 50 );
    const int b = slider.positionOf( 100 );
    QVERIFY( 0 <= a );
    QVERIFY( a < m );
    QVERIFY( m < b );
    QVERIFY( b < 400 );
    QVERIFY( std::abs( ( a + b ) / 2 - m ) <= 1 );
    QCOMPARE( slider.positionOf( 1000 ), b ); // after the last frame: at the end
}

/*-------------------------------------------------------------------*/
//! the bands (the current one darker) and the ticks of the goals in the groove
void
TestLogPlayerSlider::marks()
{
    LogPlayerSlider slider;
    slider.setPalette( UIPalette::light() );
    slider.resize( 400, 16 );
    slider.updateIndex( 0, 101 );
    slider.show();
    QVERIFY( QTest::qWaitForWindowExposed( &slider ) );
    const QImage plain = slider.grab().toImage();

    slider.setBands( { LogPlayerSlider::Band{ 10, 30, false }, LogPlayerSlider::Band{ 60, 80, true } } );
    slider.setGoals( { 50 } );
    QCOMPARE( slider.bands().size(), std::size_t( 2 ) );
    QCOMPARE( slider.goals().size(), std::size_t( 1 ) );
    const QImage marked = slider.grab().toImage();
    const int y = marked.height() / 2;

    // the goal: the orange of the light palette
    QVERIFY( near( marked.pixelColor( slider.positionOf( 50 ), y ), slider.goalColor(), 40 ) );
    QCOMPARE( slider.goalColor(), QColor( 211, 84, 0 ) );

    // the bands: changed from the plain groove, the current one more
    const QColor plain_band = plain.pixelColor( slider.positionOf( 20 ), y );
    const QColor band = marked.pixelColor( slider.positionOf( 20 ), y );
    const QColor current = marked.pixelColor( slider.positionOf( 70 ) + 1, y );
    QVERIFY( band != plain_band );
    const QColor highlight = slider.palette().color( QPalette::Highlight );
    const auto distance = [&highlight]( const QColor & c ) {
        return std::abs( c.red() - highlight.red() ) + std::abs( c.green() - highlight.green() ) + std::abs( c.blue() - highlight.blue() );
    };
    QVERIFY( distance( current ) < distance( band ) );

    // outside the bands: as before
    QCOMPARE( marked.pixelColor( slider.positionOf( 45 ), y ), plain.pixelColor( slider.positionOf( 45 ), y ) );

    // none again: as before
    slider.setBands( {} );
    slider.setGoals( {} );
    QVERIFY( slider.grab().toImage() == plain );

    // the dark palette: a light orange
    slider.setPalette( UIPalette::dark() );
    QCOMPARE( slider.goalColor(), QColor( 255, 183, 77 ) );
}

QTEST_MAIN( TestLogPlayerSlider )
#include "test_log_player_slider.moc"
