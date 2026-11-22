// -*-c++-*-

/*!
  \file test_ui_icon.cpp
  \brief unit test of the icons that follow the color scheme (UIIcon)
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
#include "ui_icon.h"
#include "ui_palette.h"

#include <QAction>
#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QPixmap>

/*
  Options reads and writes ~/.rcssmonitor.conf; CTest runs this test with
  HOME in the build tree (test/CMakeLists.txt).
*/

namespace {

//! the icons of the tool bar made from icons/svg/
const char * const NAMES[] = {
    "open", "connect", "disconnect", "kick_off",
    "live_mode", "live_delay", "play", "stop", "step_back", "step_forward",
    "slower", "faster", "prev_score", "next_score", "loop",
};

//! an embedded image (":/icons/light/16/play.png"), through QPixmap as the icons' images
QImage
resourceImage( const QString & scheme,
               const int size,
               const QString & name )
{
    UIIcon::initResources();
    return QPixmap( QStringLiteral( ":/icons/%1/%2/%3.png" ).arg( scheme ).arg( size ).arg( name ) )
        .toImage().convertToFormat( QImage::Format_ARGB32_Premultiplied );
}

//! an icon painted in a square of a size (device independent pixels) at a device pixel ratio
QImage
painted( const QIcon & icon,
         const int size,
         const qreal ratio )
{
    const int device = qRound( size * ratio );
    QImage image( device, device, QImage::Format_ARGB32_Premultiplied );
    image.setDevicePixelRatio( ratio );
    image.fill( Qt::transparent );
    QPainter painter( &image );
    icon.paint( &painter, QRect( 0, 0, size, size ) );
    painter.end();
    image.setDevicePixelRatio( 1.0 ); // compared in device pixels
    return image;
}

//! the action of a text
QAction *
actionOf( const MainWindow & window,
          const QString & text )
{
    for ( QAction * act : window.findChildren< QAction * >() )
    {
        if ( act->text() == text ) return act;
    }
    return nullptr;
}

}

/*!
  \brief unit test of UIIcon
*/
class TestUIIcon
    : public QObject {
    Q_OBJECT

private slots:

    void cleanup();
    void icons();
    void scheme();
    void sizes();
    void devicePixelRatio();
    void disabled();
    void missing();
    void appIcon();
    void mainWindow();
};

/*-------------------------------------------------------------------*/
/*!
  back to the light palette after each test
*/
void
TestUIIcon::cleanup()
{
    QApplication::setPalette( UIPalette::light() );
}

/*-------------------------------------------------------------------*/
/*!
  Each icon has its images of the four sizes, different in the light and
  the dark color schemes.
*/
void
TestUIIcon::icons()
{
    for ( const char * name : NAMES )
    {
        const QIcon icon = UIIcon::fromName( name );
        QVERIFY2( ! icon.isNull(), name );
        QList< QSize > sizes;
        for ( const int s : UIIcon::SIZES ) sizes << QSize( s, s );
        QCOMPARE( icon.availableSizes(), sizes );
        for ( const int s : UIIcon::SIZES )
        {
            const QImage light = resourceImage( "light", s, name );
            const QImage dark = resourceImage( "dark", s, name );
            QCOMPARE( light.size(), QSize( s, s ) );
            QCOMPARE( dark.size(), QSize( s, s ) );
            QVERIFY2( light != dark, name );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!
  The images of the scheme of the application's palette, chosen whenever the
  icon is drawn: the same icon follows a change of the palette.
*/
void
TestUIIcon::scheme()
{
    const QIcon icon = UIIcon::fromName( "play" );

    QApplication::setPalette( UIPalette::light() );
    QCOMPARE( icon.pixmap( 16, 16 ).toImage().convertToFormat( QImage::Format_ARGB32_Premultiplied ),
              resourceImage( "light", 16, "play" ) );

    QApplication::setPalette( UIPalette::dark() );
    QCOMPARE( icon.pixmap( 16, 16 ).toImage().convertToFormat( QImage::Format_ARGB32_Premultiplied ),
              resourceImage( "dark", 16, "play" ) );
    QCOMPARE( painted( icon, 16, 1.0 ), resourceImage( "dark", 16, "play" ) );

    // a dark palette of the desktop (not UIPalette's) is dark too
    QApplication::setPalette( QPalette( QColor( 70, 60, 90 ), QColor( 30, 30, 40 ) ) );
    QCOMPARE( painted( icon, 16, 1.0 ), resourceImage( "dark", 16, "play" ) );
}

/*-------------------------------------------------------------------*/
/*!
  Drawn in the largest size that fits, from the image of that size: 24 for
  the Live Mode and Play buttons; never larger than the largest image.
*/
void
TestUIIcon::sizes()
{
    const QIcon icon = UIIcon::fromName( "live_mode" );
    QCOMPARE( icon.actualSize( QSize( 16, 16 ) ), QSize( 16, 16 ) );
    QCOMPARE( icon.actualSize( QSize( 24, 24 ) ), QSize( 24, 24 ) );
    QCOMPARE( icon.actualSize( QSize( 30, 30 ) ), QSize( 24, 24 ) );
    QCOMPARE( icon.actualSize( QSize( 64, 64 ) ), QSize( 48, 48 ) );
    QCOMPARE( icon.actualSize( QSize( 12, 12 ) ), QSize( 12, 12 ) ); // smaller than the smallest: scaled

    for ( const int s : UIIcon::SIZES )
    {
        QCOMPARE( painted( icon, s, 1.0 ), resourceImage( "light", s, "live_mode" ) );
    }
}

/*-------------------------------------------------------------------*/
/*!
  On a display of a device pixel ratio, the image of the size times the
  ratio: 32 for 16 and 48 for 24 at 2, 24 for 16 at 1.5.
*/
void
TestUIIcon::devicePixelRatio()
{
    const QIcon icon = UIIcon::fromName( "play" );
    QCOMPARE( painted( icon, 16, 2.0 ), resourceImage( "light", 32, "play" ) );
    QCOMPARE( painted( icon, 24, 2.0 ), resourceImage( "light", 48, "play" ) );
    QCOMPARE( painted( icon, 16, 1.5 ), resourceImage( "light", 24, "play" ) );

    // no image of the size: the nearest larger one, scaled
    const QImage scaled = painted( icon, 16, 1.25 );
    QCOMPARE( scaled.size(), QSize( 20, 20 ) );
    QVERIFY( scaled != QImage() );
}

/*-------------------------------------------------------------------*/
/*!
  The disabled icon is made by the style from the scheme's image.
*/
void
TestUIIcon::disabled()
{
    const QIcon icon = UIIcon::fromName( "play" );
    const QImage normal = icon.pixmap( 16, 16, QIcon::Normal ).toImage();
    const QImage disabled = icon.pixmap( 16, 16, QIcon::Disabled ).toImage();
    QCOMPARE( disabled.size(), QSize( 16, 16 ) );
    QVERIFY( normal != disabled );
}

/*-------------------------------------------------------------------*/
/*!
  An unknown name: a null icon, with a warning.
*/
void
TestUIIcon::missing()
{
    QTest::ignoreMessage( QtWarningMsg, QRegularExpression( "UIIcon: the icon \"no_such_icon\" is not found" ) );
    QVERIFY( UIIcon::fromName( "no_such_icon" ).isNull() );
}

/*-------------------------------------------------------------------*/
/*!
  The application icon: the images of its six sizes (icons/app/), the same
  in both color schemes; the main window's icon.
*/
void
TestUIIcon::appIcon()
{
    UIIcon::initResources();
    const QIcon icon = UIIcon::appIcon( "rcssmonitor" );
    QVERIFY( ! icon.isNull() );
    QList< QSize > sizes;
    for ( const int s : UIIcon::APP_SIZES ) sizes << QSize( s, s );
    QCOMPARE( icon.availableSizes(), sizes );
    for ( const int s : UIIcon::APP_SIZES )
    {
        const QImage expected = QPixmap( QStringLiteral( ":/icons/app/%1/rcssmonitor.png" ).arg( s ) )
            .toImage().convertToFormat( QImage::Format_ARGB32_Premultiplied );
        QCOMPARE( expected.size(), QSize( s, s ) );
        for ( const QPalette & palette : { UIPalette::light(), UIPalette::dark() } )
        {
            QApplication::setPalette( palette );
            QCOMPARE( icon.pixmap( s, s ).toImage().convertToFormat( QImage::Format_ARGB32_Premultiplied ), expected );
        }
    }

    QTest::ignoreMessage( QtWarningMsg, QRegularExpression( "UIIcon: the application icon \"no_such_app\" is not found" ) );
    QVERIFY( UIIcon::appIcon( "no_such_app" ).isNull() );

    MainWindow window;
    QCOMPARE( window.windowIcon().availableSizes(), sizes );
}

/*-------------------------------------------------------------------*/
/*!
  The buttons of the main window have the icons of icons/svg/, in the color
  scheme chosen; Play/Stop shows stop while playing.
*/
void
TestUIIcon::mainWindow()
{
    MainWindow window;
    const struct {
        const char * text;
        const char * icon;
    } table[] = {
        { "Open Game Log", "open" },
        { "&Connect", "connect" },
        { "Connect &to ...", "connect" },
        { "&Disconnect", "disconnect" },
        { "&KickOff", "kick_off" },
        { "Live Mode", "live_mode" },
        { "Live Delay", "live_delay" },
        { "Play/Stop", "play" },
        { "Step Backward", "step_back" },
        { "Step Forward", "step_forward" },
        { "Decelerate", "slower" },
        { "Accelerate", "faster" },
        { "Previous score", "prev_score" },
        { "Next score", "next_score" },
        { "Loop Playback", "loop" },
    };
    for ( const char * scheme : { "light", "dark" } )
    {
        QApplication::setPalette( QString( scheme ) == "dark" ? UIPalette::dark() : UIPalette::light() );
        for ( const auto & t : table )
        {
            QAction * act = actionOf( window, t.text );
            QVERIFY2( act, t.text );
            QCOMPARE( painted( act->icon(), 16, 1.0 ), resourceImage( scheme, 16, t.icon ) );
        }
    }

    QAction * play = actionOf( window, "Play/Stop" );
    play->setChecked( true );
    QCOMPARE( painted( play->icon(), 16, 1.0 ), resourceImage( "dark", 16, "stop" ) );
    play->setChecked( false );
    QCOMPARE( painted( play->icon(), 16, 1.0 ), resourceImage( "dark", 16, "play" ) );
}

QTEST_MAIN( TestUIIcon )
#include "test_ui_icon.moc"
