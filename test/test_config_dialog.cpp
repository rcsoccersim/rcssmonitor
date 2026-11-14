// -*-c++-*-

/*!
  \file test_config_dialog.cpp
  \brief unit test of ConfigDialog
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

/////////////////////////////////////////////////////////////////////

#include <QtTest>

#include "config.h"

#include "config_dialog.h"
#include "disp_holder.h"
#include "options.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QTabWidget>

/*
  Options reads and writes ~/.rcssmonitor.conf; CTest runs this test with
  HOME in the build tree (test/CMakeLists.txt).
*/

/*!
  \brief unit test of ConfigDialog
*/
class TestConfigDialog
    : public QObject {
    Q_OBJECT

private slots:

    void selectFix();
    void illegalDefenseColor();
    void allFocusPoints();

    void tabs3D();
    void view3DTab();
    void numberRange();
    void update3D();
    void playerCameraTab();
    void playerCameraDim();
};

namespace {

QStringList
tabNames( const ConfigDialog & dialog )
{
    QStringList names;
    const QTabWidget * tabs = dialog.findChild< QTabWidget * >();
    for ( int i = 0; tabs && i < tabs->count(); ++i )
    {
        names << tabs->tabText( i );
    }
    return names;
}

template < typename T >
T *
child( const ConfigDialog & dialog,
       const char * name )
{
    T * w = dialog.findChild< T * >( QString::fromLatin1( name ) );
    if ( ! w ) qWarning( "no %s", name );
    return w;
}

//! the settings of the 3D tabs back to their defaults
void
resetView3D()
{
    Options::instance().setView3DDefaults();
    Options::instance().setPlayerCamera( PlayerCameraOptions() );
}

//! the line below a row of choices
QString
noteOf( const QButtonGroup * group )
{
    // the note is the label in the same grid, below the buttons: the one whose
    // text is the checked button's summary
    return group->checkedButton()->property( "summary" ).toString();
}

}

/*-------------------------------------------------------------------*/
/*!
  The "Fix" button of the player selection sets SELECT_FIX. It used to set
  SELECT_AUTO_RIGHT.
*/
void
TestConfigDialog::selectFix()
{
    DispHolder holder;
    ConfigDialog dialog( nullptr, holder );

    Options::instance().setPlayerSelectType( Options::SELECT_AUTO_LEFT );
    QSignalSpy spy( &dialog, SIGNAL( configured() ) );

    QVERIFY( QMetaObject::invokeMethod( &dialog, "clickSelectFix" ) );
    QCOMPARE( Options::instance().playerSelectType(), Options::SELECT_FIX );
    QCOMPARE( spy.count(), 1 );

    // already Fix: nothing changes
    QVERIFY( QMetaObject::invokeMethod( &dialog, "clickSelectFix" ) );
    QCOMPARE( Options::instance().playerSelectType(), Options::SELECT_FIX );
    QCOMPARE( spy.count(), 1 );
}

/*-------------------------------------------------------------------*/
/*!
  The "Illegal Defense" item of the color list starts from the illegal
  defense color. It used to start from the pointto color.
*/
void
TestConfigDialog::illegalDefenseColor()
{
    DispHolder holder;
    ConfigDialog dialog( nullptr, holder );

    const QColor illegal_defense = Options::instance().illegalDefensePen().color();
    const QColor pointto = Options::instance().pointtoPen().color();
    QVERIFY( illegal_defense != pointto );

    const ColorItem * item = nullptr;
    for ( const QListWidget * list : dialog.findChildren< QListWidget * >() )
    {
        for ( int i = 0; i < list->count(); ++i )
        {
            if ( list->item( i )->text() == "Illegal Defense" )
            {
                item = dynamic_cast< const ColorItem * >( list->item( i ) );
            }
        }
    }

    QVERIFY( item );
    QCOMPARE( item->newColor(), illegal_defense );
}

/*-------------------------------------------------------------------*/
/*!
  "All Focus Points" sets its option, and works only with "Focus Point"
*/
void
TestConfigDialog::allFocusPoints()
{
    Options & opt = Options::instance();
    if ( opt.showFocusPoint() ) opt.toggleShowFocusPoint();
    if ( opt.showAllFocusPoints() ) opt.toggleShowAllFocusPoints();

    DispHolder holder;
    ConfigDialog dialog( nullptr, holder );
    QCheckBox * focus = nullptr;
    QCheckBox * all = nullptr;
    for ( QCheckBox * cb : dialog.findChildren< QCheckBox * >() )
    {
        if ( cb->text() == "Focus Point" ) focus = cb;
        if ( cb->text() == "All Focus Points" ) all = cb;
    }
    QVERIFY( focus );
    QVERIFY( all );
    QVERIFY( ! all->isEnabled() );

    focus->click();
    QVERIFY( opt.showFocusPoint() );
    QVERIFY( all->isEnabled() );
    QSignalSpy spy( &dialog, SIGNAL( configured() ) );
    all->click();
    QVERIFY( opt.showAllFocusPoints() );
    QCOMPARE( spy.count(), 1 );

    // the key of Focus Point (toggleShowFocusPoint()) too
    QVERIFY( QMetaObject::invokeMethod( &dialog, "toggleShowFocusPoint" ) );
    QVERIFY( ! opt.showFocusPoint() );
    QVERIFY( ! all->isEnabled() );
    QVERIFY( all->isChecked() ); // kept for the next time

    opt.toggleShowAllFocusPoints();
}

/*-------------------------------------------------------------------*/
/*!
  the 3D tab and the Player Camera tab, only in a build with the 3D view
*/
void
TestConfigDialog::tabs3D()
{
    DispHolder holder;
    ConfigDialog dialog( nullptr, holder );

#ifdef HAVE_3D_VIEW
    QCOMPARE( tabNames( dialog ),
              QStringList( { "Show", "Canvas", "Color", "Font", "3D", "Player Camera" } ) );
    // every item has a tooltip
    for ( const char * name : { "closeup_distance_3d", "closeup_elevation_3d",
                                "player_camera_time_constant", "player_camera_max_turn",
                                "player_camera_fixed_fov" } )
    {
        QDoubleSpinBox * spin = child< QDoubleSpinBox >( dialog, name );
        QVERIFY( spin );
        QVERIFY2( spin->toolTip().contains( "Range: " ), name );
    }
    for ( const char * name : { "compass_3d", "minimap_3d", "player_camera_position",
                                "player_camera_direction", "player_camera_motion",
                                "player_camera_fov", "player_camera_sight" } )
    {
        QButtonGroup * group = child< QButtonGroup >( dialog, name );
        QVERIFY( group );
        for ( QAbstractButton * b : group->buttons() )
        {
            QVERIFY2( ! b->toolTip().isEmpty(), qPrintable( b->text() ) );
        }
    }
    QVERIFY( ! child< QCheckBox >( dialog, "boards_3d" )->toolTip().isEmpty() );
    QVERIFY( ! child< QCheckBox >( dialog, "banners_3d" )->toolTip().isEmpty() );
    QVERIFY( child< QPushButton >( dialog, "default_3d" )->toolTip().contains( "Close-up 12 m, 20" ) );
    QVERIFY( child< QPushButton >( dialog, "default_player_camera" )->toolTip()
             .contains( "Over the Shoulder, Head, Smooth, Fixed, Server Rules" ) );
#else
    QCOMPARE( tabNames( dialog ), QStringList( { "Show", "Canvas", "Color", "Font" } ) );
    QVERIFY( ! dialog.findChild< QDoubleSpinBox * >( "closeup_distance_3d" ) );
    // and nothing to update
    dialog.update3D();
#endif
}

/*-------------------------------------------------------------------*/
/*!
  the 3D tab: the close-up cameras, the compass, the minimap, the boards
  and the banners set their options, and Default sets them back
*/
void
TestConfigDialog::view3DTab()
{
#ifndef HAVE_3D_VIEW
    QSKIP( "without the 3D view" );
#else
    Options & opt = Options::instance();
    resetView3D();

    DispHolder holder;
    ConfigDialog dialog( nullptr, holder );
    dialog.show();
    QSignalSpy spy( &dialog, SIGNAL( configured() ) );

    QDoubleSpinBox * distance = child< QDoubleSpinBox >( dialog, "closeup_distance_3d" );
    QDoubleSpinBox * elevation = child< QDoubleSpinBox >( dialog, "closeup_elevation_3d" );
    QButtonGroup * compass = child< QButtonGroup >( dialog, "compass_3d" );
    QButtonGroup * minimap = child< QButtonGroup >( dialog, "minimap_3d" );
    QCheckBox * boards = child< QCheckBox >( dialog, "boards_3d" );
    QCheckBox * banners = child< QCheckBox >( dialog, "banners_3d" );
    QPushButton * defaults = child< QPushButton >( dialog, "default_3d" );
    QVERIFY( distance && elevation && compass && minimap && boards && banners && defaults );

    // the options at first
    QCOMPARE( distance->value(), Options::CLOSEUP_DISTANCE_3D );
    QCOMPARE( elevation->value(), Options::CLOSEUP_ELEVATION_3D );
    QCOMPARE( compass->checkedButton()->text(), QString( "Off" ) );
    QCOMPARE( minimap->checkedButton()->text(), QString( "Off" ) );
    QVERIFY( boards->isChecked() );
    QVERIFY( banners->isChecked() );

    distance->setValue( 30.0 );
    QCOMPARE( opt.closeupDistance3D(), 30.0 );
    QCOMPARE( spy.count(), 1 );
    elevation->setValue( 45.0 );
    QCOMPARE( opt.closeupElevation3D(), 45.0 );
    QCOMPARE( spy.count(), 2 );

    compass->button( 1 )->click();
    QCOMPARE( opt.compass3D(), GuideMode::Player );
    minimap->button( 2 )->click();
    QCOMPARE( opt.minimap3D(), GuideMode::All );
    QCOMPARE( compass->button( 1 )->text(), QString( "Player Camera" ) );
    QCOMPARE( minimap->button( 2 )->text(), QString( "All Cameras" ) );
    boards->click();
    QVERIFY( ! opt.boards3D() );
    banners->click();
    QVERIFY( ! opt.banners3D() );
    QCOMPARE( spy.count(), 6 );

    // Default: the options and the fields back
    defaults->click();
    QCOMPARE( spy.count(), 7 );
    QCOMPARE( opt.closeupDistance3D(), Options::CLOSEUP_DISTANCE_3D );
    QCOMPARE( opt.closeupElevation3D(), Options::CLOSEUP_ELEVATION_3D );
    QCOMPARE( opt.compass3D(), GuideMode::Off );
    QCOMPARE( opt.minimap3D(), GuideMode::Off );
    QVERIFY( opt.boards3D() );
    QVERIFY( opt.banners3D() );
    QCOMPARE( distance->value(), Options::CLOSEUP_DISTANCE_3D );
    QCOMPARE( elevation->value(), Options::CLOSEUP_ELEVATION_3D );
    QCOMPARE( compass->checkedId(), 0 );
    QCOMPARE( minimap->checkedId(), 0 );
    QVERIFY( boards->isChecked() );
    QVERIFY( banners->isChecked() );
    // the fields set back by Default do not set the options again
    QCOMPARE( spy.count(), 7 );
#endif
}

/*-------------------------------------------------------------------*/
/*!
  a number field: the error color while what is typed is out of the range,
  and the nearest end with Enter
*/
void
TestConfigDialog::numberRange()
{
#ifndef HAVE_3D_VIEW
    QSKIP( "without the 3D view" );
#else
    Options & opt = Options::instance();
    resetView3D();

    DispHolder holder;
    ConfigDialog dialog( nullptr, holder );
    dialog.show();
    dialog.findChild< QTabWidget * >()->setCurrentIndex( 4 );
    QVERIFY( QTest::qWaitForWindowExposed( &dialog ) );

    QDoubleSpinBox * distance = child< QDoubleSpinBox >( dialog, "closeup_distance_3d" );
    QVERIFY( distance );
    const QColor normal = distance->palette().color( QPalette::Base );

    distance->setFocus();
    distance->selectAll();
    QTest::keyClicks( distance, "1" ); // under 3 m
    QVERIFY( ! distance->hasAcceptableInput() );
    QVERIFY( distance->palette().color( QPalette::Base ) != normal );
    // not applied while typing
    QCOMPARE( opt.closeupDistance3D(), Options::CLOSEUP_DISTANCE_3D );

    QTest::keyClick( distance, Qt::Key_Return );
    QCOMPARE( distance->value(), Options::MIN_CLOSEUP_DISTANCE_3D );
    QCOMPARE( opt.closeupDistance3D(), Options::MIN_CLOSEUP_DISTANCE_3D );
    QCOMPARE( distance->palette().color( QPalette::Base ), normal );

    // within the range: as it is
    distance->selectAll();
    QTest::keyClicks( distance, "25" );
    QCOMPARE( distance->palette().color( QPalette::Base ), normal );
    QCOMPARE( opt.closeupDistance3D(), Options::MIN_CLOSEUP_DISTANCE_3D ); // with Enter
    QTest::keyClick( distance, Qt::Key_Return );
    QCOMPARE( opt.closeupDistance3D(), 25.0 );

    // over the range: the extra digit is not taken
    distance->selectAll();
    QTest::keyClicks( distance, "70" );
    QTest::keyClick( distance, Qt::Key_Return );
    QVERIFY( opt.closeupDistance3D() <= Options::MAX_CLOSEUP_DISTANCE_3D );
#endif
}

/*-------------------------------------------------------------------*/
/*!
  update3D(): the fields show the options changed elsewhere (Z and X change
  the close-up distance), without writing the shown values back
*/
void
TestConfigDialog::update3D()
{
#ifndef HAVE_3D_VIEW
    QSKIP( "without the 3D view" );
#else
    Options & opt = Options::instance();
    resetView3D();

    DispHolder holder;
    ConfigDialog dialog( nullptr, holder );
    QSignalSpy spy( &dialog, SIGNAL( configured() ) );

    opt.setCloseupDistance3D( 12.0 / 1.5 / 1.5 ); // 5.333...
    opt.setCompass3D( GuideMode::All );
    PlayerCameraOptions pc;
    pc.sight = PlayerCameraStyle::Sight::View;
    pc.max_turn = 180.0;
    opt.setPlayerCamera( pc );
    dialog.update3D();

    QDoubleSpinBox * distance = child< QDoubleSpinBox >( dialog, "closeup_distance_3d" );
    QCOMPARE( distance->value(), 5.3 );
    QCOMPARE( child< QButtonGroup >( dialog, "compass_3d" )->checkedId(), 2 );
    QCOMPARE( child< QButtonGroup >( dialog, "player_camera_sight" )->checkedButton()->text(),
              QString( "View Cone" ) );
    QCOMPARE( child< QDoubleSpinBox >( dialog, "player_camera_max_turn" )->value(), 180.0 );

    QCOMPARE( spy.count(), 0 );
    QCOMPARE( opt.closeupDistance3D(), 12.0 / 1.5 / 1.5 ); // not rounded

    resetView3D();
#endif
}

/*-------------------------------------------------------------------*/
/*!
  the Player Camera tab: every style and number sets its option, the line
  below a row sums up the chosen one, and Default sets them back
*/
void
TestConfigDialog::playerCameraTab()
{
#ifndef HAVE_3D_VIEW
    QSKIP( "without the 3D view" );
#else
    using namespace PlayerCameraStyle;
    Options & opt = Options::instance();
    resetView3D();

    DispHolder holder;
    ConfigDialog dialog( nullptr, holder );
    dialog.show();
    QSignalSpy spy( &dialog, SIGNAL( configured() ) );

    QButtonGroup * position = child< QButtonGroup >( dialog, "player_camera_position" );
    QButtonGroup * direction = child< QButtonGroup >( dialog, "player_camera_direction" );
    QButtonGroup * motion = child< QButtonGroup >( dialog, "player_camera_motion" );
    QButtonGroup * fov = child< QButtonGroup >( dialog, "player_camera_fov" );
    QButtonGroup * sight = child< QButtonGroup >( dialog, "player_camera_sight" );
    QDoubleSpinBox * time_constant = child< QDoubleSpinBox >( dialog, "player_camera_time_constant" );
    QDoubleSpinBox * max_turn = child< QDoubleSpinBox >( dialog, "player_camera_max_turn" );
    QDoubleSpinBox * fixed_fov = child< QDoubleSpinBox >( dialog, "player_camera_fixed_fov" );
    QPushButton * defaults = child< QPushButton >( dialog, "default_player_camera" );
    QVERIFY( position && direction && motion && fov && sight
             && time_constant && max_turn && fixed_fov && defaults );

    // the buttons in the order of the options' lists, with their labels
    QCOMPARE( position->buttons().size(), int( positions().size() ) );
    QCOMPARE( sight->button( 2 )->text(), QString( "Server Rules" ) );

    // the defaults at first
    QCOMPARE( position->checkedButton()->text(), QString( "Over the Shoulder" ) );
    QCOMPARE( direction->checkedButton()->text(), QString( "Head" ) );
    QCOMPARE( motion->checkedButton()->text(), QString( "Smooth" ) );
    QCOMPARE( fov->checkedButton()->text(), QString( "Fixed" ) );
    QCOMPARE( sight->checkedButton()->text(), QString( "Server Rules" ) );
    QCOMPARE( time_constant->value(), TIME_CONSTANT );
    QCOMPARE( max_turn->value(), MAX_TURN );
    QCOMPARE( fixed_fov->value(), FIXED_FOV );

    // the note below the row: the summary of the chosen one
    const auto note_shown = [&dialog]( const QButtonGroup * group ) {
        for ( const QLabel * l : dialog.findChildren< QLabel * >() )
        {
            if ( l->text() == noteOf( group ) ) return true;
        }
        return false;
    };
    QVERIFY( note_shown( position ) );
    QCOMPARE( noteOf( position ), QString( "6 m behind the player and 3 m high." ) );

    position->button( 0 )->click();
    QCOMPARE( opt.playerCamera().position, Position::Eye );
    QVERIFY( noteOf( position ).startsWith( "At the eyes" ) );
    QVERIFY( note_shown( position ) );
    direction->button( 2 )->click();
    QCOMPARE( opt.playerCamera().direction, Direction::Middle );
    QVERIFY( note_shown( direction ) );
    motion->button( 0 )->click();
    QCOMPARE( opt.playerCamera().motion, Motion::Instant );
    fov->button( 1 )->click();
    QCOMPARE( opt.playerCamera().fov, Fov::Eased );
    QVERIFY( note_shown( fov ) );
    sight->button( 0 )->click();
    QCOMPARE( opt.playerCamera().sight, Sight::All );
    QVERIFY( note_shown( sight ) );
    QCOMPARE( spy.count(), 5 );

    time_constant->setValue( 1.25 );
    QCOMPARE( opt.playerCamera().time_constant, 1.25 );
    max_turn->setValue( 720.0 );
    QCOMPARE( opt.playerCamera().max_turn, 720.0 );
    fixed_fov->setValue( 60.0 );
    QCOMPARE( opt.playerCamera().fixed_fov, 60.0 );
    QCOMPARE( spy.count(), 8 );
    // one style changes, the others stay
    QCOMPARE( opt.playerCamera().position, Position::Eye );
    QCOMPARE( opt.playerCamera().sight, Sight::All );

    defaults->click();
    QCOMPARE( spy.count(), 9 );
    QVERIFY( opt.playerCamera() == PlayerCameraOptions() );
    QCOMPARE( position->checkedButton()->text(), QString( "Over the Shoulder" ) );
    QCOMPARE( sight->checkedButton()->text(), QString( "Server Rules" ) );
    QCOMPARE( max_turn->value(), MAX_TURN );
    QVERIFY( note_shown( position ) );
    QCOMPARE( spy.count(), 9 );
#endif
}

/*-------------------------------------------------------------------*/
/*!
  the numbers that the chosen styles do not use are dim: the time constant
  with Smooth or Eased, the largest turn with Smooth, the fixed field of
  view with Fixed
*/
void
TestConfigDialog::playerCameraDim()
{
#ifndef HAVE_3D_VIEW
    QSKIP( "without the 3D view" );
#else
    using namespace PlayerCameraStyle;
    resetView3D();

    DispHolder holder;
    ConfigDialog dialog( nullptr, holder );
    QButtonGroup * motion = child< QButtonGroup >( dialog, "player_camera_motion" );
    QButtonGroup * fov = child< QButtonGroup >( dialog, "player_camera_fov" );
    QDoubleSpinBox * time_constant = child< QDoubleSpinBox >( dialog, "player_camera_time_constant" );
    QDoubleSpinBox * max_turn = child< QDoubleSpinBox >( dialog, "player_camera_max_turn" );
    QDoubleSpinBox * fixed_fov = child< QDoubleSpinBox >( dialog, "player_camera_fixed_fov" );
    QVERIFY( motion && fov && time_constant && max_turn && fixed_fov );

    struct Case {
        int motion; // instant, smooth
        int fov; // view, eased, fixed
        bool time_constant;
        bool max_turn;
        bool fixed_fov;
    };
    const Case cases[] = {
        { 1, 2, true, true, true }, // the defaults
        { 0, 2, false, false, true },
        { 0, 0, false, false, false },
        { 0, 1, true, false, false },
        { 1, 0, true, true, false },
    };
    for ( const Case & c : cases )
    {
        motion->button( c.motion )->click();
        fov->button( c.fov )->click();
        QCOMPARE( time_constant->isEnabled(), c.time_constant );
        QCOMPARE( max_turn->isEnabled(), c.max_turn );
        QCOMPARE( fixed_fov->isEnabled(), c.fixed_fov );
        // the labels too
        const QLabel * label = nullptr;
        for ( const QLabel * l : dialog.findChildren< QLabel * >() )
        {
            if ( l->buddy() == max_turn ) label = l;
        }
        QVERIFY( label );
        QCOMPARE( label->isEnabled(), c.max_turn );
    }

    resetView3D();
#endif
}

QTEST_MAIN( TestConfigDialog )

#include "test_config_dialog.moc"
