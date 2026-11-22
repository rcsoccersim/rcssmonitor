// -*-c++-*-

/*!
  \file test_ui_color_scheme.cpp
  \brief unit test of the color schemes of the widgets (View > UI Color
  Scheme, --ui-color-scheme) and of the styles (View > Window Style)
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
#include "config_dialog.h"
#include "main_window.h"
#include "options.h"
#include "ui_palette.h"
#include "ui_style.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QProcess>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QTemporaryDir>
#include <QToolBar>
#include <QToolButton>

#include <cmath>
#include <cstdlib>
#include <iostream>

/*
  Options reads and writes ~/.rcssmonitor.conf; CTest runs this test with
  HOME in the build tree (test/CMakeLists.txt).

  The offscreen platform cannot make the desktop's palette dark, so the
  desktop's palette is a made-up one, set to the application before any
  window is made (initTestCase()).

  Options reads the settings file once, when it is made, and writes it when
  the program ends. To see that, this test runs itself again as a child
  process with CHILD_ENV set (main()): the child parses its command line as
  rcssmonitor does, prints the color scheme, and ends.
*/

namespace {

//! the environment variable that makes this program the child (main())
const char * const CHILD_ENV = "RCSSMONITOR_TEST_UI_COLOR_SCHEME_CHILD";

//! the line of the child's output that tells the color scheme
const QString CHILD_PREFIX = QStringLiteral( "ui_color_scheme=" );

/*!
  \brief the differences of two palettes in the main roles
  \return the differences, one in a line; empty if none
*/
QString
paletteDifference( const QPalette & actual,
                   const QPalette & expected )
{
    const QPalette::ColorRole roles[] = {
        QPalette::Window, QPalette::WindowText, QPalette::Base, QPalette::Text,
        QPalette::Button, QPalette::ButtonText, QPalette::Light, QPalette::Midlight,
        QPalette::Mid, QPalette::Dark, QPalette::Shadow,
        QPalette::Highlight, QPalette::HighlightedText,
    };
    const QPalette::ColorGroup groups[] = { QPalette::Active, QPalette::Disabled };

    QStringList diffs;
    for ( const QPalette::ColorGroup g : groups )
    {
        for ( const QPalette::ColorRole r : roles )
        {
            const QColor a = actual.color( g, r );
            const QColor e = expected.color( g, r );
            if ( a != e )
            {
                diffs << QString( "group=%1 role=%2 actual=%3 expected=%4" )
                    .arg( int( g ) ).arg( int( r ) )
                    .arg( a.name( QColor::HexArgb ), e.name( QColor::HexArgb ) );
            }
        }
    }
    return diffs.join( QLatin1Char( '\n' ) );
}

//! the contrast ratio of two colors (WCAG 2)
double
contrast( const QColor & a,
          const QColor & b )
{
    const auto luminance = []( const QColor & c ) {
        const auto lin = []( const double v ) {
            return v <= 0.03928 ? v / 12.92 : std::pow( ( v + 0.055 ) / 1.055, 2.4 );
        };
        return 0.2126 * lin( c.redF() ) + 0.7152 * lin( c.greenF() ) + 0.0722 * lin( c.blueF() );
    };
    const double la = luminance( a );
    const double lb = luminance( b );
    return ( std::max( la, lb ) + 0.05 ) / ( std::min( la, lb ) + 0.05 );
}

//! the number of the outermost pixels of an image near a color
int
borderPixelsOf( const QImage & image,
                const QColor & color )
{
    int count = 0;
    for ( int y = 0; y < image.height(); ++y )
    {
        for ( int x = 0; x < image.width(); ++x )
        {
            if ( x != 0 && y != 0
                 && x != image.width() - 1 && y != image.height() - 1 )
            {
                continue;
            }
            const QColor c = image.pixelColor( x, y );
            if ( std::abs( c.red() - color.red() ) <= 8
                 && std::abs( c.green() - color.green() ) <= 8
                 && std::abs( c.blue() - color.blue() ) <= 8 )
            {
                ++count;
            }
        }
    }
    return count;
}

//! the action of View > UI Color Scheme of a scheme
QAction *
schemeAction( const MainWindow & window,
              const UIColorScheme scheme )
{
    return window.findChild< QAction * >( "ui_color_scheme_" + uiColorSchemeName( scheme ) );
}

//! the action of View > Window Style of a style (by its key, any case)
QAction *
styleAction( const MainWindow & window,
             const QString & key )
{
    for ( QAction * act : window.findChildren< QAction * >() )
    {
        if ( act->data().toString().compare( key, Qt::CaseInsensitive ) == 0
             && QStyleFactory::keys().contains( act->data().toString() ) )
        {
            return act;
        }
    }
    return nullptr;
}

//! the checked actions of View > UI Color Scheme
QStringList
checkedSchemes( const MainWindow & window )
{
    QStringList names;
    for ( const auto & s : uiColorSchemes() )
    {
        QAction * act = schemeAction( window, s.first );
        if ( act && act->isChecked() ) names << QString::fromLatin1( s.second );
    }
    return names;
}

//! a value saved in the settings file (~/.rcssmonitor.conf)
QString
savedValue( const QString & key )
{
    QSettings settings( Options::CONF_FILE, QSettings::IniFormat );
    return settings.value( key ).toString();
}

/*!
  \brief show a window and start it as rcssmonitor does (MainWindow::init()),
  with a game log (not to connect to a server), and wait until the log is
  open (init() opens it a moment later)
*/
bool
start( MainWindow & window )
{
    Options::instance().setGameLogFile( std::string( TEST_DATA_DIR ) + "/head_v5.rcg" );
    window.show();
    window.init();
    QLabel * frame = window.findChild< QLabel * >( "frame_label" );
    const bool opened = frame && QTest::qWaitFor( [frame]() { return ! frame->text().isEmpty(); } );
    Options::instance().setGameLogFile( std::string() );
    return opened;
}

/*!
  \brief choose a color scheme in the menu. Qt6 sets the application's
  palette to the widgets when the events are processed (Qt5 at once), as
  the event loop does in rcssmonitor.
*/
void
choose( const MainWindow & window,
        const UIColorScheme scheme )
{
    schemeAction( window, scheme )->trigger();
    QCoreApplication::processEvents();
}

/*!
  \brief choose a style in the menu, as choose()
*/
void
chooseStyle( const MainWindow & window,
             const QString & key )
{
    styleAction( window, key )->trigger();
    QCoreApplication::processEvents();
}

}

/*!
  \brief unit test of the color schemes of the widgets
*/
class TestUIColorScheme
    : public QObject {
    Q_OBJECT

private:

    QPalette M_desktop; //!< the made-up palette of the desktop

private slots:

    void initTestCase();
    void init();
    void palettes();
    void names();
    void defaultIsSystem();
    void menu();
    void styleChange();
    void savedStyle();
    void checkedToolButton();
    void wrongMarks();
    void configDialogFollows();
    void commandLine_data();
    void commandLine();
};

/*-------------------------------------------------------------------*/
/*!
  a dark desktop, neither the light nor the dark palette
*/
void
TestUIColorScheme::initTestCase()
{
    QApplication::setPalette( QPalette( QColor( 70, 60, 90 ), QColor( 30, 30, 40 ) ) );
    M_desktop = QApplication::palette();
    QVERIFY( UIPalette::isDark( M_desktop ) );
    QVERIFY( ! paletteDifference( M_desktop, UIPalette::dark() ).isEmpty() );
}

/*-------------------------------------------------------------------*/
/*!
  every test starts on the desktop's palette, as rcssmonitor does (a window
  of a test before may have left another one)
*/
void
TestUIColorScheme::init()
{
    QApplication::setPalette( M_desktop );
    QCoreApplication::processEvents();
    Options::instance().setUIColorScheme( UIColorScheme::System );
}

/*-------------------------------------------------------------------*/
/*!
  The light and dark palettes of the Fusion style, and the colors of the
  errors legible on both.
*/
void
TestUIColorScheme::palettes()
{
    const QPalette light = UIPalette::light();
    QCOMPARE( light.color( QPalette::Window ), QColor( 239, 239, 239 ) );
    QCOMPARE( light.color( QPalette::WindowText ), QColor( Qt::black ) );
    QCOMPARE( light.color( QPalette::Base ), QColor( Qt::white ) );
    QCOMPARE( light.color( QPalette::Highlight ), QColor( 48, 140, 198 ) );
    QVERIFY( ! UIPalette::isDark( light ) );

    const QPalette dark = UIPalette::dark();
    QCOMPARE( dark.color( QPalette::Window ), QColor( 50, 50, 50 ) );
    QCOMPARE( dark.color( QPalette::WindowText ), QColor( 240, 240, 240 ) );
    QVERIFY( dark.color( QPalette::Base ).lightness() < dark.color( QPalette::Window ).lightness() );
    QCOMPARE( dark.color( QPalette::Highlight ), QColor( 48, 140, 198 ) );
    QVERIFY( UIPalette::isDark( dark ) );

    // System: the desktop's
    QVERIFY( paletteDifference( UIPalette::of( UIColorScheme::System, M_desktop ), M_desktop ).isEmpty() );
    QVERIFY( paletteDifference( UIPalette::of( UIColorScheme::Light, M_desktop ), light ).isEmpty() );
    QVERIFY( paletteDifference( UIPalette::of( UIColorScheme::Dark, M_desktop ), dark ).isEmpty() );

    // the text of an error on the window, and the text on a wrong field:
    // at least 4.5:1 (WCAG AA)
    for ( const QPalette & p : { light, dark } )
    {
        QVERIFY2( contrast( UIPalette::errorText( p ), p.color( QPalette::Window ) ) >= 4.5,
                  qPrintable( UIPalette::errorText( p ).name() ) );
        const QColor base = UIPalette::errorBase( p );
        QVERIFY( base.red() > base.green() + 40 && base.red() > base.blue() + 40 );
        QVERIFY2( contrast( base, p.color( QPalette::Text ) ) >= 4.5,
                  qPrintable( base.name() ) );
    }
}

/*-------------------------------------------------------------------*/
/*!
  The names in the options and the command line.
*/
void
TestUIColorScheme::names()
{
    QCOMPARE( uiColorSchemes().size(), std::size_t( 3 ) );
    for ( const auto & s : uiColorSchemes() )
    {
        UIColorScheme scheme = ( s.first == UIColorScheme::Dark
                                 ? UIColorScheme::Light
                                 : UIColorScheme::Dark );
        QVERIFY( uiColorSchemeFromName( uiColorSchemeName( s.first ), &scheme ) );
        QCOMPARE( scheme, s.first );
    }
    QCOMPARE( uiColorSchemeName( UIColorScheme::System ), QString( "system" ) );
    QCOMPARE( uiColorSchemeName( UIColorScheme::Light ), QString( "light" ) );
    QCOMPARE( uiColorSchemeName( UIColorScheme::Dark ), QString( "dark" ) );

    // an unknown name leaves the scheme as it was
    UIColorScheme scheme = UIColorScheme::Light;
    QVERIFY( ! uiColorSchemeFromName( "Dark", &scheme ) );
    QVERIFY( ! uiColorSchemeFromName( "", &scheme ) );
    QCOMPARE( scheme, UIColorScheme::Light );
}

/*-------------------------------------------------------------------*/
/*!
  By default (no settings file), the desktop's palette, checked in the menu,
  and the style wrapped by UIStyle.
*/
void
TestUIColorScheme::defaultIsSystem()
{
    QCOMPARE( Options::instance().uiColorScheme(), UIColorScheme::System );
    QCOMPARE( savedValue( "MainWindow/ui_color_scheme" ), QString( "system" ) ); // the file made with the defaults

    MainWindow window;
    QVERIFY( start( window ) );

    const QString diff = paletteDifference( QApplication::palette(), M_desktop );
    QVERIFY2( diff.isEmpty(), qPrintable( diff ) );
    QCOMPARE( checkedSchemes( window ), QStringList( "system" ) );
    QVERIFY( dynamic_cast< UIStyle * >( QApplication::style() ) );
    QVERIFY( ! QApplication::style()->objectName().isEmpty() );
}

/*-------------------------------------------------------------------*/
/*!
  The menu: one scheme checked, its palette set to the application, and
  saved at once.
*/
void
TestUIColorScheme::menu()
{
    MainWindow window;
    QVERIFY( start( window ) );
    QAction * system = schemeAction( window, UIColorScheme::System );
    QAction * light = schemeAction( window, UIColorScheme::Light );
    QAction * dark = schemeAction( window, UIColorScheme::Dark );
    QVERIFY( system && light && dark );
    QVERIFY( system->actionGroup() && system->actionGroup()->isExclusive() );
    QCOMPARE( system->actionGroup(), dark->actionGroup() );
    QCOMPARE( system->actionGroup(), light->actionGroup() );

    const struct {
        QAction * action;
        UIColorScheme scheme;
        QPalette palette;
    } table[] = {
        { dark, UIColorScheme::Dark, UIPalette::dark() },
        { system, UIColorScheme::System, M_desktop },
        { light, UIColorScheme::Light, UIPalette::light() },
        { dark, UIColorScheme::Dark, UIPalette::dark() },
    };
    for ( const auto & t : table )
    {
        t.action->trigger();
        QCoreApplication::processEvents();
        QCOMPARE( checkedSchemes( window ), QStringList( uiColorSchemeName( t.scheme ) ) );
        QCOMPARE( Options::instance().uiColorScheme(), t.scheme );
        const QString diff = paletteDifference( QApplication::palette(), t.palette );
        QVERIFY2( diff.isEmpty(), qPrintable( diff ) );
        // the window and its widgets follow
        QCOMPARE( window.palette().color( QPalette::Window ), t.palette.color( QPalette::Window ) );
        QCOMPARE( savedValue( "MainWindow/ui_color_scheme" ), uiColorSchemeName( t.scheme ) );
    }

    // back to the default for the other tests
    choose( window, UIColorScheme::System );
}

/*-------------------------------------------------------------------*/
/*!
  View > Window Style: the scheme is kept (setStyle() may set the style's
  palette), the style is wrapped by UIStyle, and the style chosen is used
  by the next window (the next start).
*/
void
TestUIColorScheme::styleChange()
{
    const QString original = QApplication::style()->objectName();
    {
        MainWindow window;
        QVERIFY( start( window ) );
        QAction * windows = styleAction( window, "Windows" );
        QAction * back = styleAction( window, original );
        if ( ! windows || ! back )
        {
            QSKIP( "the styles Windows and the original one are not both here" );
        }

        choose( window, UIColorScheme::Dark );
        chooseStyle( window, "Windows" );
        QCOMPARE( QApplication::style()->objectName().toLower(), QString( "windows" ) );
        QVERIFY( dynamic_cast< UIStyle * >( QApplication::style() ) );
        QVERIFY( windows->isChecked() );
        {
            const QString diff = paletteDifference( QApplication::palette(), UIPalette::dark() );
            QVERIFY2( diff.isEmpty(), qPrintable( diff ) );
        }

        choose( window, UIColorScheme::Light );
        {
            const QString diff = paletteDifference( QApplication::palette(), UIPalette::light() );
            QVERIFY2( diff.isEmpty(), qPrintable( diff ) );
        }
    } // saved when the window closes
    QCOMPARE( savedValue( "MainWindow/window_style" ).toLower(), QString( "windows" ) );

    {
        // the next start: in the style chosen
        QApplication::setStyle( QStyleFactory::create( original ) );
        // the desktop's palette, which the style may have changed (the
        // scheme chosen is kept in the options)
        QApplication::setPalette( M_desktop );
        QCoreApplication::processEvents();
        MainWindow window;
        QVERIFY( start( window ) );
        QCOMPARE( QApplication::style()->objectName().toLower(), QString( "windows" ) );
        QVERIFY( styleAction( window, "Windows" )->isChecked() );
        // and in the scheme chosen
        QCOMPARE( checkedSchemes( window ), QStringList( "light" ) );

        // back to the original style and the default scheme
        chooseStyle( window, original );
        QCOMPARE( QApplication::style()->objectName().toLower(), original.toLower() );
        choose( window, UIColorScheme::System );
        const QString diff = paletteDifference( QApplication::palette(), M_desktop );
        QVERIFY2( diff.isEmpty(), qPrintable( diff ) );
    }
    QCOMPARE( savedValue( "MainWindow/window_style" ).toLower(), original.toLower() );
}

/*-------------------------------------------------------------------*/
/*!
  A style saved that is unknown (e.g. "plastique" by an older version,
  which Qt5 and Qt6 do not have): the platform's style, and not saved again.
*/
void
TestUIColorScheme::savedStyle()
{
    const QString original = QApplication::style()->objectName();
    {
        QSettings settings( Options::CONF_FILE, QSettings::IniFormat );
        settings.setValue( "MainWindow/window_style", "plastique" );
    }
    {
        MainWindow window;
        QVERIFY( start( window ) );
        QCOMPARE( QApplication::style()->objectName().toLower(), original.toLower() );
        QVERIFY( dynamic_cast< UIStyle * >( QApplication::style() ) );
        QVERIFY( styleAction( window, original )->isChecked() );
    }
    QCOMPARE( savedValue( "MainWindow/window_style" ), QString() );
}

/*-------------------------------------------------------------------*/
/*!
  UIStyle: the checked buttons of the tool bars with a border of the
  highlight color, of the same size as the others: a button of a tool bar,
  and the main window's (in a frame in the tool bar). Not the buttons out
  of the tool bars.
*/
void
TestUIColorScheme::checkedToolButton()
{
    MainWindow window;
    window.resize( 1280, 720 ); // the whole tool bar in the window
    for ( QToolBar * bar : window.findChildren< QToolBar * >() ) bar->setVisible( true ); // hidden by default
    QVERIFY( start( window ) );
    QVERIFY( dynamic_cast< UIStyle * >( QApplication::style() ) );

    QToolButton * loop = nullptr;
    for ( QToolButton * b : window.findChildren< QToolButton * >() )
    {
        if ( b->defaultAction() && b->defaultAction()->text() == "Loop Playback" ) loop = b;
    }
    QVERIFY( loop && loop->defaultAction()->isCheckable() );
    QVERIFY( ! qobject_cast< QToolBar * >( loop->parentWidget() ) ); // in a frame in the tool bar

    for ( const UIColorScheme scheme : { UIColorScheme::Light, UIColorScheme::Dark } )
    {
        choose( window, scheme );

        QToolBar bar;
        QAction * act = bar.addAction( QStringLiteral( "Checked" ) );
        act->setCheckable( true );
        bar.show();
        QVERIFY( QTest::qWaitForWindowExposed( &bar ) );

        QWidget * outside = new QWidget();
        QToolButton * button_outside = new QToolButton( outside );
        button_outside->setCheckable( true );
        button_outside->setText( QStringLiteral( "Checked" ) );
        outside->show();
        QVERIFY( QTest::qWaitForWindowExposed( outside ) );

        const QColor highlight = QApplication::palette().color( QPalette::Highlight );
        const struct {
            QWidget * button;
            QAction * action;
        } buttons[] = {
            { bar.widgetForAction( act ), act },
            { loop, loop->defaultAction() },
        };
        for ( const auto & b : buttons )
        {
            QVERIFY( b.button );
            b.action->setChecked( false );
            QCoreApplication::processEvents();
            const QSize unchecked_size = b.button->size();
            QCOMPARE( borderPixelsOf( b.button->grab().toImage(), highlight ), 0 );

            b.action->setChecked( true );
            QCoreApplication::processEvents();
            QCOMPARE( b.button->size(), unchecked_size );
            const QImage checked = b.button->grab().toImage();
            // most of the outermost pixels (but the round corners)
            const int border_length = 2 * ( checked.width() + checked.height() ) - 4;
            QVERIFY2( borderPixelsOf( checked, highlight ) > border_length / 2,
                      qPrintable( QString( "%1 / %2" ).arg( borderPixelsOf( checked, highlight ) ).arg( border_length ) ) );
            b.action->setChecked( false );
        }

        // out of the tool bars: as the style draws it
        button_outside->setChecked( true );
        QCoreApplication::processEvents();
        QCOMPARE( borderPixelsOf( button_outside->grab().toImage(), highlight ), 0 );
        delete outside;
    }

    choose( window, UIColorScheme::System );
}

/*-------------------------------------------------------------------*/
/*!
  The marks of the wrong fields and texts: only their color set, so that
  they follow the scheme in the others and in all when not wrong; marked
  again in the colors of a new scheme.
*/
void
TestUIColorScheme::wrongMarks()
{
    MainWindow window;
    QVERIFY( start( window ) );
    choose( window, UIColorScheme::Light );
    const QPalette light = UIPalette::light();
    const QPalette dark = UIPalette::dark();

    // the tool bar's cycle field: red when the log has no such cycle
    QLineEdit * cycle = window.findChild< QLineEdit * >( "cycle_edit" );
    QVERIFY( cycle );
    QTest::keyClicks( cycle, "9" );
    QTest::keyClick( cycle, Qt::Key_Return );
    QCOMPARE( cycle->palette().color( QPalette::Base ), UIPalette::errorBase( light ) );
    QCOMPARE( cycle->palette().color( QPalette::Text ), light.color( QPalette::Text ) );

    QLabel * label = new QLabel( "wrong", &window );
    UIPalette::markWrongText( label, true );
    QCOMPARE( label->palette().color( QPalette::WindowText ), UIPalette::errorText( light ) );

    QLineEdit * right = new QLineEdit( &window );
    UIPalette::markWrongField( right, false );

    // another scheme: marked again in its colors
    choose( window, UIColorScheme::Dark );
    QCOMPARE( cycle->palette().color( QPalette::Base ), UIPalette::errorBase( dark ) );
    QCOMPARE( cycle->palette().color( QPalette::Text ), dark.color( QPalette::Text ) );
    QCOMPARE( label->palette().color( QPalette::WindowText ), UIPalette::errorText( dark ) );
    QCOMPARE( label->palette().color( QPalette::Window ), dark.color( QPalette::Window ) );
    QCOMPARE( right->palette().color( QPalette::Base ), dark.color( QPalette::Base ) );

    // typing again takes the red off: the scheme's color
    QTest::keyClick( cycle, Qt::Key_Backspace );
    QCOMPARE( cycle->palette().color( QPalette::Base ), dark.color( QPalette::Base ) );
    UIPalette::markWrongText( label, false );
    QCOMPARE( label->palette().color( QPalette::WindowText ), dark.color( QPalette::WindowText ) );

    // and they follow the next scheme
    choose( window, UIColorScheme::Light );
    QCOMPARE( cycle->palette().color( QPalette::Base ), light.color( QPalette::Base ) );
    QCOMPARE( label->palette().color( QPalette::WindowText ), light.color( QPalette::WindowText ) );

    choose( window, UIColorScheme::System );
}

/*-------------------------------------------------------------------*/
/*!
  The number fields of the settings dialog (marked when the dialog shows
  the options) follow the scheme.
*/
void
TestUIColorScheme::configDialogFollows()
{
    MainWindow window;
    QVERIFY( start( window ) );
    const QList< QDoubleSpinBox * > spins = window.findChildren< QDoubleSpinBox * >();
    QVERIFY( ! spins.isEmpty() );

    for ( const UIColorScheme scheme : { UIColorScheme::Light, UIColorScheme::Dark, UIColorScheme::Light } )
    {
        choose( window, scheme );
        const QPalette expected = UIPalette::of( scheme, M_desktop );
        for ( QDoubleSpinBox * spin : spins )
        {
            QCOMPARE( spin->palette().color( QPalette::Base ), expected.color( QPalette::Base ) );
            QCOMPARE( spin->palette().color( QPalette::Text ), expected.color( QPalette::Text ) );
        }
    }

    // the swatches of the Color tab: with a gray border (white and black are seen on both)
    ConfigDialog * dialog = window.findChild< ConfigDialog * >();
    QVERIFY( dialog );
    int swatches = 0;
    for ( QListWidget * list : dialog->findChildren< QListWidget * >() )
    {
        for ( int i = 0; i < list->count(); ++i )
        {
            ColorItem * item = dynamic_cast< ColorItem * >( list->item( i ) );
            if ( ! item ) continue;
            const QImage image = item->icon().pixmap( 16, 16 ).toImage();
            QCOMPARE( image.size(), QSize( 16, 16 ) );
            QCOMPARE( image.pixelColor( 0, 0 ), QColor( 128, 128, 128 ) );
            QCOMPARE( image.pixelColor( 15, 8 ), QColor( 128, 128, 128 ) );
            QCOMPARE( image.pixelColor( 8, 8 ), item->newColor() );
            ++swatches;
        }
    }
    QVERIFY( swatches > 10 );

    choose( window, UIColorScheme::System );
}

/*-------------------------------------------------------------------*/
void
TestUIColorScheme::commandLine_data()
{
    QTest::addColumn< QString >( "saved" ); // in the settings file; empty: no file
    QTest::addColumn< QStringList >( "args" );
    QTest::addColumn< QString >( "used" );
    QTest::addColumn< QString >( "savedAfter" );

    QTest::newRow( "no file" ) << QString() << QStringList() << "system" << "system";
    QTest::newRow( "saved" ) << "dark" << QStringList() << "dark" << "dark";
    QTest::newRow( "command line" ) << "dark" << QStringList{ "--ui-color-scheme", "light" } << "light" << "dark";
    QTest::newRow( "command line, =" ) << "light" << QStringList{ "--ui-color-scheme=system" } << "system" << "light";
    QTest::newRow( "unknown" ) << "dark" << QStringList{ "--ui-color-scheme", "Light" } << "dark" << "dark";
    QTest::newRow( "unknown saved" ) << "sepia" << QStringList() << "system" << "sepia";
}

/*-------------------------------------------------------------------*/
/*!
  The scheme saved is read when the program starts, the command line's is
  used over it, and the command line's is not saved when the program ends
  (the child process: main()).
*/
void
TestUIColorScheme::commandLine()
{
    QFETCH( QString, saved );
    QFETCH( QStringList, args );
    QFETCH( QString, used );
    QFETCH( QString, savedAfter );

    QTemporaryDir home;
    QVERIFY( home.isValid() );
    const QString conf = home.filePath( ".rcssmonitor.conf" );
    if ( ! saved.isEmpty() )
    {
        QSettings settings( conf, QSettings::IniFormat );
        settings.setValue( "MainWindow/ui_color_scheme", saved );
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert( CHILD_ENV, "1" );
    env.insert( "HOME", home.path() );
    env.remove( "RCSSMONITOR_TEST_HOME" ); // the settings file kept (fresh_settings.cpp)
    env.insert( "QT_QPA_PLATFORM", "offscreen" );
    QProcess child;
    child.setProcessEnvironment( env );
    child.start( QCoreApplication::applicationFilePath(), args );
    QVERIFY( child.waitForFinished( 30000 ) );
    const QString out = QString::fromLocal8Bit( child.readAllStandardOutput() );
    const QString err = QString::fromLocal8Bit( child.readAllStandardError() );
    QVERIFY2( child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0,
              qPrintable( out + err ) );

    QString printed;
    for ( const QString & line : out.split( '\n' ) )
    {
        if ( line.startsWith( CHILD_PREFIX ) ) printed = line.mid( CHILD_PREFIX.size() ).trimmed();
    }
    QCOMPARE( printed, used );
    if ( args.size() == 2 && args.at( 1 ) == "Light" )
    {
        QVERIFY2( err.contains( "ui-color-scheme" ), qPrintable( err ) );
    }

    QVERIFY( QFile::exists( conf ) );
    QSettings settings( conf, QSettings::IniFormat );
    QCOMPARE( settings.value( "MainWindow/ui_color_scheme" ).toString(), savedAfter );
}

/*-------------------------------------------------------------------*/
/*!
  The test, or the child of commandLine(): it parses its command line as
  rcssmonitor does (Options reads the settings file first), prints the
  scheme, and ends (Options writes the settings file).
*/
int
main( int argc,
      char ** argv )
{
    if ( qEnvironmentVariableIsSet( CHILD_ENV ) )
    {
        QApplication app( argc, argv );
        if ( ! Options::instance().parseCmdLine( argc, argv ) )
        {
            return 1;
        }
        std::cout << CHILD_PREFIX.toStdString()
                  << uiColorSchemeName( Options::instance().uiColorScheme() ).toStdString()
                  << std::endl;
        return 0;
    }

    QApplication app( argc, argv );
    app.setAttribute( Qt::AA_Use96Dpi, true );
    TestUIColorScheme test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec( &test, argc, argv );
}

#include "test_ui_color_scheme.moc"
