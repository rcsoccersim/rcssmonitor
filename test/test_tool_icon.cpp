// -*-c++-*-

/*!
  \file test_tool_icon.cpp
  \brief unit test of the toolbar icons
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

#include "main_window.h"
#include "tool_icon.h"

#include <QAction>

/*
  Options reads and writes ~/.rcssmonitor.conf; CTest runs this test with
  HOME in the build tree (test/CMakeLists.txt).
*/

namespace {

/*!
  \brief the colors of the pixels of an icon, in a set of names with alpha.
*/
QSet< QString >
iconColors( const QIcon & icon )
{
    const QImage image = icon.pixmap( 16, 16 ).toImage().convertToFormat( QImage::Format_ARGB32 );
    QSet< QString > colors;
    for ( int y = 0; y < image.height(); ++y )
    {
        for ( int x = 0; x < image.width(); ++x )
        {
            const QRgb p = image.pixel( x, y );
            if ( qAlpha( p ) == 255 )
            {
                colors.insert( QColor( p ).name() );
            }
        }
    }
    return colors;
}

}

/*!
  \brief unit test of the toolbar icons
*/
class TestToolIcon
    : public QObject {
    Q_OBJECT

private slots:

    void tint();
    void followPalette();
};

/*-------------------------------------------------------------------*/
/*!
  Black becomes the color, white transparent, gray the color half
  transparent, and transparent stays transparent.
*/
void
TestToolIcon::tint()
{
    QImage image( 4, 1, QImage::Format_ARGB32 );
    image.setPixel( 0, 0, qRgba( 0, 0, 0, 255 ) );
    image.setPixel( 1, 0, qRgba( 255, 255, 255, 255 ) );
    image.setPixel( 2, 0, qRgba( 128, 128, 128, 255 ) );
    image.setPixel( 3, 0, qRgba( 0, 0, 0, 0 ) );

    const QColor color( 240, 230, 220 );
    const QImage tinted = tintIcon( image, color );

    QCOMPARE( tinted.pixel( 0, 0 ), qRgba( 240, 230, 220, 255 ) );
    QCOMPARE( qAlpha( tinted.pixel( 1, 0 ) ), 0 );
    QCOMPARE( qRed( tinted.pixel( 2, 0 ) ), 240 );
    QVERIFY( qAbs( qAlpha( tinted.pixel( 2, 0 ) ) - 127 ) <= 1 );
    QCOMPARE( qAlpha( tinted.pixel( 3, 0 ) ), 0 );
}

/*-------------------------------------------------------------------*/
/*!
  The toolbar icons are drawn in the text color of the palette, and again
  when the palette changes (e.g. to a dark one).
*/
void
TestToolIcon::followPalette()
{
    MainWindow window;

    QAction * live = nullptr;
    for ( QAction * act : window.findChildren< QAction * >() )
    {
        if ( act->text() == "Live Mode" ) live = act;
    }
    QVERIFY( live );

    QPalette palette = window.palette();
    palette.setColor( QPalette::ButtonText, QColor( 10, 20, 30 ) );
    window.setPalette( palette );
    QCOMPARE( iconColors( live->icon() ), QSet< QString >( { "#0a141e" } ) );

    palette.setColor( QPalette::ButtonText, QColor( 240, 230, 220 ) );
    window.setPalette( palette );
    QCOMPARE( iconColors( live->icon() ), QSet< QString >( { "#f0e6dc" } ) );
}

QTEST_MAIN( TestToolIcon )

#include "test_tool_icon.moc"
