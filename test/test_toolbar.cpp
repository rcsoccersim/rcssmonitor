// -*-c++-*-

/*!
  \file test_toolbar.cpp
  \brief unit test of the tool bar: the icons drawn in code, the buttons, the labels and the cycle field
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
#include "options.h"
#include "toolbar_icons.h"

#include <QAction>
#include <QFile>
#include <QTemporaryDir>
#include <QLabel>
#include <QLineEdit>
#include <QToolBar>
#include <QToolButton>

/*
  Options reads and writes ~/.rcssmonitor.conf; CTest runs this test with
  HOME in the build tree (test/CMakeLists.txt).
*/

namespace {

constexpr int SIZE = 32; //!< the icons drawn at twice the web version's 16 x 16

//! the opacity of an icon at a point of the web version's 16 x 16 square
int
alphaAt( const QImage & image,
         const double x,
         const double y )
{
    return qAlpha( image.pixel( int( x * SIZE / 16.0 ), int( y * SIZE / 16.0 ) ) );
}

//! the tool bar's button of an action, if any
QToolButton *
buttonOf( const MainWindow & window,
          const QString & text )
{
    for ( QToolButton * b : window.findChildren< QToolButton * >() )
    {
        if ( b->defaultAction() && b->defaultAction()->text() == text ) return b;
    }
    return nullptr;
}

//! the colors of the opaque pixels of an icon
QSet< QString >
iconColors( const QIcon & icon )
{
    const QImage image = icon.pixmap( 16, 16 ).toImage().convertToFormat( QImage::Format_ARGB32 );
    QSet< QString > colors;
    for ( int y = 0; y < image.height(); ++y )
    {
        for ( int x = 0; x < image.width(); ++x )
        {
            if ( qAlpha( image.pixel( x, y ) ) == 255 ) colors.insert( QColor( image.pixel( x, y ) ).name() );
        }
    }
    return colors;
}

}

/*!
  \brief unit test of the tool bar
*/
class TestToolbar
    : public QObject {
    Q_OBJECT

private slots:

    void icons();
    void buttons();
    void labels();
    void stoppedTime();
    void rows();
};

/*-------------------------------------------------------------------*/
/*!
  The icons drawn in code have the web version's shapes: drawn where the
  shapes are, clear where they are not, in the color asked; each its own.
*/
void
TestToolbar::icons()
{
    const QColor color( 200, 100, 50 );
    const QImage open = drawnIconImage( DrawnIcon::Open, color, SIZE );
    QCOMPARE( open.size(), QSize( SIZE, SIZE ) );
    QVERIFY( alphaAt( open, 1.5, 8.0 ) > 128 ); // the folder's left side
    QVERIFY( alphaAt( open, 8.0, 12.5 ) > 128 ); // its bottom
    QCOMPARE( alphaAt( open, 8.0, 9.5 ), 0 ); // inside it
    QCOMPARE( alphaAt( open, 15.5, 0.5 ), 0 );

    const QImage connect = drawnIconImage( DrawnIcon::Connect, color, SIZE );
    QVERIFY( alphaAt( connect, 5.0, 8.0 ) > 128 ); // the plug
    QVERIFY( alphaAt( connect, 7.0, 8.0 ) > 128 );
    QVERIFY( alphaAt( connect, 14.5, 8.0 ) > 128 ); // the socket's back
    QCOMPARE( alphaAt( connect, 12.5, 8.0 ), 0 ); // in the socket

    const QImage disconnect = drawnIconImage( DrawnIcon::Disconnect, color, SIZE );
    QVERIFY( alphaAt( disconnect, 4.0, 8.0 ) > 128 );
    QCOMPARE( alphaAt( disconnect, 7.5, 8.0 ), 0 ); // pulled out: between the prongs
    QVERIFY( alphaAt( disconnect, 7.5, 6.5 ) > 128 ); // a prong

    const QImage kick_off = drawnIconImage( DrawnIcon::KickOff, color, SIZE );
    QVERIFY( alphaAt( kick_off, 8.0, 8.0 ) > 128 ); // the pentagon
    QVERIFY( alphaAt( kick_off, 14.0, 8.0 ) > 128 ); // the ball's outline
    QCOMPARE( alphaAt( kick_off, 11.0, 4.5 ), 0 );
    QCOMPARE( alphaAt( kick_off, 0.5, 0.5 ), 0 );

    const QImage loop = drawnIconImage( DrawnIcon::Loop, color, SIZE );
    QVERIFY( alphaAt( loop, 8.0, 3.0 ) > 128 ); // the upper arc over the top
    QVERIFY( alphaAt( loop, 8.0, 13.0 ) > 128 ); // the lower one under the bottom
    QVERIFY( alphaAt( loop, 12.8, 3.5 ) > 128 ); // an arrow's head
    QCOMPARE( alphaAt( loop, 8.0, 8.0 ), 0 );

    // in the color asked; each its own
    for ( const QImage & image : { open, connect, disconnect, kick_off, loop } )
    {
        for ( int y = 0; y < SIZE; ++y )
        {
            for ( int x = 0; x < SIZE; ++x )
            {
                const QRgb p = image.convertToFormat( QImage::Format_ARGB32 ).pixel( x, y );
                if ( qAlpha( p ) == 255 ) QCOMPARE( QColor( p ).name(), color.name() );
            }
        }
    }
    QVERIFY( open != connect && connect != disconnect && disconnect != kick_off && kick_off != loop );
}

/*-------------------------------------------------------------------*/
/*!
  The buttons of the web version's tool bar: Open, Connect, Disconnect and
  Kick Off before the player's buttons, Loop after them; Disconnect and
  Kick Off only when connected; Loop sets the option; the icons drawn in
  the text color of the palette, and again when it changes.
*/
void
TestToolbar::buttons()
{
    MainWindow window;
    window.resize( 1280, 720 ); // the whole tool bar in the window
    for ( QToolBar * bar : window.findChildren< QToolBar * >() ) bar->setVisible( true ); // hidden by default
    window.show();
    QVERIFY( QTest::qWaitForWindowExposed( &window ) ); // laid out
    QToolButton * open = buttonOf( window, "Open Game Log" );
    QToolButton * connect = buttonOf( window, "&Connect" );
    QToolButton * disconnect = buttonOf( window, "&Disconnect" );
    QToolButton * kick_off = buttonOf( window, "&KickOff" );
    QToolButton * loop = buttonOf( window, "Loop Playback" );
    QVERIFY( open && connect && disconnect && kick_off && loop );
    QVERIFY( open->isEnabled() && connect->isEnabled() );
    QVERIFY( ! disconnect->isEnabled() && ! kick_off->isEnabled() );
    for ( QToolButton * b : { open, connect, disconnect, kick_off, loop } )
    {
        QVERIFY( ! b->defaultAction()->icon().isNull() );
        QVERIFY( ! b->defaultAction()->toolTip().isEmpty() );
    }
    QVERIFY( open->defaultAction()->toolTip().contains( "Ctrl+O" ) );
    QVERIFY( kick_off->defaultAction()->toolTip().contains( "Ctrl+K" ) );
    // in the order of the web version's: the file and the server, then the player
    const auto x = [&]( const QToolButton * b ) { return b->mapTo( &window, QPoint() ).x(); };
    QVERIFY( x( open ) < x( connect ) && x( connect ) < x( disconnect ) && x( disconnect ) < x( kick_off ) );
    QVERIFY( x( kick_off ) < x( buttonOf( window, "Live Mode" ) ) );
    QVERIFY( x( buttonOf( window, "Next score" ) ) < x( loop ) );

    // loop playback, the option auto_loop_mode
    Options & opt = Options::instance();
    opt.setAutoLoopMode( false );
    QVERIFY( loop->defaultAction()->isCheckable() );
    loop->defaultAction()->setChecked( false );
    loop->click();
    QVERIFY( opt.autoLoopMode() );
    loop->click();
    QVERIFY( ! opt.autoLoopMode() );

    // the palette's text color
    QPalette palette = window.palette();
    palette.setColor( QPalette::ButtonText, QColor( 10, 20, 30 ) );
    window.setPalette( palette );
    QCOMPARE( iconColors( loop->defaultAction()->icon() ), QSet< QString >( { "#0a141e" } ) );
    palette.setColor( QPalette::ButtonText, QColor( 240, 230, 220 ) );
    window.setPalette( palette );
    QCOMPARE( iconColors( open->defaultAction()->icon() ), QSet< QString >( { "#f0e6dc" } ) );
}

/*-------------------------------------------------------------------*/
/*!
  After the slider: "cycle (frame / frames)", the time per frame, and the
  cycle field, which goes to the first frame from a cycle with Enter, turns
  red after the last one (back with typing), and gives the keys back with Esc.
*/
void
TestToolbar::labels()
{
    Options & opt = Options::instance();
    opt.setGameLogFile( std::string( TEST_DATA_DIR ) + "/head_v5.rcg" ); // the shows of cycles 1, 2 and 4
    MainWindow window;
    QLabel * frame = window.findChild< QLabel * >( "frame_label" );
    QLabel * interval = window.findChild< QLabel * >( "interval_label" );
    QLineEdit * cycle = window.findChild< QLineEdit * >( "cycle_edit" );
    QVERIFY( frame && interval && cycle );
    // no log yet
    QVERIFY( frame->text().isEmpty() );
    QVERIFY( ! cycle->isEnabled() );

    window.show();
    window.init();
    QTRY_VERIFY( ! frame->text().isEmpty() );
    QVERIFY( frame->text().endsWith( " / 3)" ) );
    QCOMPARE( interval->text(), QString( "%1 ms" ).arg( opt.timerInterval() ) );
    QVERIFY( cycle->isEnabled() );

    // to cycle 4: the last of the three frames
    const QColor base = cycle->palette().color( QPalette::Base );
    cycle->setFocus();
    QTest::keyClicks( cycle, "4" );
    QTest::keyClick( cycle, Qt::Key_Return );
    QCOMPARE( frame->text(), QString( "4 (3 / 3)" ) );
    QVERIFY( cycle->text().isEmpty() );
    // to cycle 2
    QTest::keyClicks( cycle, "2" );
    QTest::keyClick( cycle, Qt::Key_Return );
    QCOMPARE( frame->text(), QString( "2 (2 / 3)" ) );
    // no frame of cycle 3: the first one after it (as the web version and the slider)
    QTest::keyClicks( cycle, "3" );
    QTest::keyClick( cycle, Qt::Key_Return );
    QCOMPARE( frame->text(), QString( "4 (3 / 3)" ) );
    QTest::keyClicks( cycle, "2" );
    QTest::keyClick( cycle, Qt::Key_Return );
    QCOMPARE( frame->text(), QString( "2 (2 / 3)" ) );
    // after the last one: red, and stays
    QTest::keyClicks( cycle, "9" );
    QTest::keyClick( cycle, Qt::Key_Return );
    QCOMPARE( frame->text(), QString( "2 (2 / 3)" ) );
    QCOMPARE( cycle->text(), QString( "9" ) );
    QVERIFY( cycle->palette().color( QPalette::Base ) != base );
    QVERIFY( cycle->palette().color( QPalette::Base ).red() > cycle->palette().color( QPalette::Base ).green() );
    // typing again takes the red off
    QTest::keyClick( cycle, Qt::Key_Backspace );
    QCOMPARE( cycle->palette().color( QPalette::Base ), base );
    // Esc: cleared
    QTest::keyClicks( cycle, "1" );
    QTest::keyClick( cycle, Qt::Key_Escape );
    QVERIFY( cycle->text().isEmpty() );
    QCOMPARE( frame->text(), QString( "2 (2 / 3)" ) );
    // only digits
    QTest::keyClicks( cycle, "a-1" );
    QCOMPARE( cycle->text(), QString( "1" ) );

    opt.setGameLogFile( std::string() );
}

/*-------------------------------------------------------------------*/
/*!
  The cycle with the stopped time after a dot, as the web version's.
*/
void
TestToolbar::stoppedTime()
{
    // head_json.rcg (the shows of cycles 1, 2 and 5) with the last one in a stopped time
    QFile in( QString( TEST_DATA_DIR ) + "/head_json.rcg" );
    QVERIFY( in.open( QIODevice::ReadOnly ) );
    QByteArray log = in.readAll();
    QCOMPARE( log.count( "\"show\":{\"time\":5," ), 1 );
    log.replace( "\"show\":{\"time\":5,", "\"show\":{\"time\":5,\"stime\":3," );
    QTemporaryDir dir;
    QVERIFY( dir.isValid() );
    const QString path = dir.filePath( "stopped.rcg" );
    QFile out( path );
    QVERIFY( out.open( QIODevice::WriteOnly ) );
    out.write( log );
    out.close();

    Options & opt = Options::instance();
    opt.setGameLogFile( path.toStdString() );
    MainWindow window;
    QLabel * frame = window.findChild< QLabel * >( "frame_label" );
    QLineEdit * cycle = window.findChild< QLineEdit * >( "cycle_edit" );
    window.show();
    window.init();
    QTRY_VERIFY( ! frame->text().isEmpty() );
    QTest::keyClicks( cycle, "5" );
    QTest::keyClick( cycle, Qt::Key_Return );
    QCOMPARE( frame->text(), QString( "5.3 (3 / 3)" ) );
    QTest::keyClicks( cycle, "2" );
    QTest::keyClick( cycle, Qt::Key_Return );
    QCOMPARE( frame->text(), QString( "2 (2 / 3)" ) );
    opt.setGameLogFile( std::string() );
}

/*-------------------------------------------------------------------*/
/*!
  The buttons and the position (the slider, the frame label, the cycle
  field, the time per frame) in one row in a wide window, in two rows in a
  narrow one, the position all in the window; one again when it is wide.
*/
void
TestToolbar::rows()
{
    MainWindow window;
    const QList< QToolBar * > bars = window.findChildren< QToolBar * >();
    QCOMPARE( bars.size(), 2 );
    for ( QToolBar * bar : bars ) bar->setVisible( true ); // hidden by default
    QToolBar * buttons = nullptr;
    for ( QToolBar * bar : bars )
    {
        if ( bar->isAncestorOf( buttonOf( window, "Live Mode" ) ) ) buttons = bar;
    }
    QToolBar * position = ( bars[0] == buttons ? bars[1] : bars[0] );
    QVERIFY( buttons && position != buttons );
    QLabel * interval = window.findChild< QLabel * >( "interval_label" );
    QVERIFY( position->isAncestorOf( interval ) );
    QVERIFY( position->isAncestorOf( window.findChild< QLineEdit * >( "cycle_edit" ) ) );

    window.resize( 1600, 720 );
    window.show();
    QVERIFY( QTest::qWaitForWindowExposed( &window ) );
    QTRY_COMPARE( position->y(), buttons->y() ); // one row
    QVERIFY( ! window.toolBarBreak( position ) );

    window.resize( 700, 720 );
    QTRY_VERIFY( position->y() > buttons->y() ); // two rows
    QVERIFY( window.toolBarBreak( position ) );
    // the time per frame, the last, in the window
    QTRY_VERIFY( interval->isVisible() );
    QVERIFY( interval->mapTo( &window, QPoint( interval->width(), 0 ) ).x() <= window.width() );

    window.resize( 1600, 720 );
    QTRY_COMPARE( position->y(), buttons->y() );
    QVERIFY( ! window.toolBarBreak( position ) );

    // hidden and shown again by View > Tool Bar
    QAction * toggle = nullptr;
    for ( QAction * act : window.findChildren< QAction * >() )
    {
        if ( act->text() == "Tool Bar" ) toggle = act;
    }
    QVERIFY( toggle );
    toggle->setChecked( true );
    toggle->trigger(); // off
    QVERIFY( buttons->isHidden() && position->isHidden() );
    toggle->trigger(); // on
    QVERIFY( ! buttons->isHidden() && ! position->isHidden() );
}

QTEST_MAIN( TestToolbar )

#include "test_toolbar.moc"
