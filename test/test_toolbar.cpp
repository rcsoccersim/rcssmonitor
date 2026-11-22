// -*-c++-*-

/*!
  \file test_toolbar.cpp
  \brief unit test of the tool bar: the buttons, the labels and the cycle field
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
#include "connect_dialog.h"
#include "ui_icon.h"
#include "ui_palette.h"

#include <QAction>
#include <QApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QLabel>
#include <QLineEdit>
#include <QToolBar>
#include <QToolButton>
#include <QMenuBar>
#include <QMenu>

/*
  Options reads and writes ~/.rcssmonitor.conf; CTest runs this test with
  HOME in the build tree (test/CMakeLists.txt).
*/

namespace {

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

//! an icon at 16 pixels, and an embedded image of the icons (icons/; UIIcon), in the same format
QImage
iconImage( const QIcon & icon )
{
    return icon.pixmap( 16, 16 ).toImage().convertToFormat( QImage::Format_ARGB32_Premultiplied );
}
QImage
embeddedImage( const QString & scheme,
               const QString & name )
{
    UIIcon::initResources();
    return QPixmap( QStringLiteral( ":/icons/%1/16/%2.png" ).arg( scheme, name ) )
        .toImage().convertToFormat( QImage::Format_ARGB32_Premultiplied );
}

}

/*!
  \brief unit test of the tool bar
*/
class TestToolbar
    : public QObject {
    Q_OBJECT

private slots:

    void buttons();
    void labels();
    void stoppedTime();
    void rows();
    void liveDelay();
};

/*-------------------------------------------------------------------*/
/*!
  The buttons of the web version's tool bar: Open, Connect, Disconnect and
  Kick Off before the player's buttons, Loop after them; Disconnect and
  Kick Off only when connected; Connect opens the dialog of the server
  (Ctrl+C connects again, as the web version's); Loop sets the option; the
  icons of icons/svg/, in the images of the color scheme (UIIcon).
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
    QToolButton * connect = buttonOf( window, "Connect &to ..." );
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
    QVERIFY( connect->defaultAction()->toolTip().contains( "Ctrl+C: connect again" ) );
    QVERIFY( ! buttonOf( window, "&Connect" ) ); // connecting again: the key and the menu

    // Connect opens the dialog of the server (closed here without connecting)
    bool dialog_shown = false;
    QTimer::singleShot( 0, &window, [&]()
                        {
                            ConnectDialog * dialog = qobject_cast< ConnectDialog * >( QApplication::activeModalWidget() );
                            dialog_shown = ( dialog != nullptr );
                            if ( dialog ) dialog->reject();
                        } );
    connect->click();
    QVERIFY( dialog_shown );
    QVERIFY( connect->isEnabled() ); // not connected
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

    // the images of the color scheme of the application's palette
    const struct {
        QToolButton * button;
        const char * icon;
    } icons[] = {
        { open, "open" }, { connect, "connect" }, { disconnect, "disconnect" },
        { kick_off, "kick_off" }, { loop, "loop" },
    };
    const QPalette original = QApplication::palette();
    for ( const char * scheme : { "dark", "light" } )
    {
        QApplication::setPalette( QString( scheme ) == "dark" ? UIPalette::dark() : UIPalette::light() );
        for ( const auto & i : icons )
        {
            QCOMPARE( iconImage( i.button->defaultAction()->icon() ), embeddedImage( scheme, i.icon ) );
        }
    }
    QApplication::setPalette( original );
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

/*-------------------------------------------------------------------*/
/*!
  The delayed live view (docs/plan/2026-11-11_3d-view.md §5.10): a checkable button
  after Live Mode and an item after it in the Monitor menu, which set the
  option live_delay and tell the seconds; live and delayed, the label after
  the cycle tells how far behind the newest frame.
*/
void
TestToolbar::liveDelay()
{
    Options & opt = Options::instance();
    opt.setLiveDelay( false );
    opt.setLiveDelaySeconds( Options::DEFAULT_LIVE_DELAY_SECONDS );
    opt.setGameLogFile( std::string( TEST_DATA_DIR ) + "/head_v5.rcg" ); // the shows of cycles 1, 2 and 4

    MainWindow window;
    window.resize( 1280, 720 ); // the whole tool bar in the window
    for ( QToolBar * bar : window.findChildren< QToolBar * >() ) bar->setVisible( true ); // hidden by default
    window.show();
    window.init();
    QVERIFY( QTest::qWaitForWindowExposed( &window ) );
    QLabel * interval = window.findChild< QLabel * >( "interval_label" );
    QTRY_VERIFY( interval && ! interval->text().isEmpty() );

    QToolButton * live_mode = buttonOf( window, "Live Mode" );
    QToolButton * live_delay = buttonOf( window, "Live Delay" );
    QVERIFY( live_mode && live_delay );
    const auto x = [&]( const QToolButton * b ) { return b->mapTo( &window, QPoint() ).x(); };
    QVERIFY( x( live_mode ) < x( live_delay ) && x( live_delay ) < x( buttonOf( window, "Play/Stop" ) ) );
    QAction * act = live_delay->defaultAction();
    QVERIFY( act->isCheckable() && ! act->isChecked() );
    QVERIFY( act->isEnabled() ); // set at any time, used when live
    QVERIFY( ! act->icon().isNull() );
    QVERIFY( act->toolTip().contains( "3.0 s" ) );
    QVERIFY( act->statusTip().contains( "referee" ) );

    // in the Monitor menu, after Live Mode (found by its title: Qt 6.2 has
    // no QAction::menu())
    QMenu * monitor = nullptr;
    for ( QMenu * m : window.menuBar()->findChildren< QMenu * >() )
    {
        if ( m->title() == "&Monitor" ) monitor = m;
    }
    QVERIFY( monitor );
    const QList< QAction * > items = monitor->actions();
    const int live_mode_at = items.indexOf( live_mode->defaultAction() );
    QVERIFY( live_mode_at >= 0 );
    QCOMPARE( items.value( live_mode_at + 1 ), act );

    // the option
    live_delay->click();
    QVERIFY( opt.liveDelay() );
    QVERIFY( act->isChecked() );
    // not live (a log): the time per frame
    QCOMPARE( interval->text(), QString( "%1 ms" ).arg( opt.timerInterval() ) );

    // live and delayed: how far behind; not delayed: the time per frame again
    QVERIFY( QMetaObject::invokeMethod( &window, "setLiveMode" ) );
    live_delay->click();
    QVERIFY( ! opt.liveDelay() );
    QCOMPARE( interval->text(), QString( "%1 ms" ).arg( opt.timerInterval() ) );
    live_delay->click();
    QCOMPARE( interval->text(), QString( "LIVE -3.0 s" ) );

    // the seconds changed in the settings: the tips follow
    opt.setLiveDelaySeconds( 4.5 );
    QVERIFY( QMetaObject::invokeMethod( &window, "applyLiveDelay" ) );
    QVERIFY( act->toolTip().contains( "4.5 s" ) );
    QCOMPARE( interval->text(), QString( "LIVE -4.5 s" ) );
    // turned off in the settings: the button follows
    opt.setLiveDelay( false );
    QVERIFY( QMetaObject::invokeMethod( &window, "applyLiveDelay" ) );
    QVERIFY( ! act->isChecked() );

    opt.setLiveDelaySeconds( Options::DEFAULT_LIVE_DELAY_SECONDS );
    opt.setGameLogFile( std::string() );
}

QTEST_MAIN( TestToolbar )

#include "test_toolbar.moc"
