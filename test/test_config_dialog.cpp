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
#include <QGroupBox>

#include "config.h"

#include "config_dialog.h"
#include "disp_holder.h"
#include "options.h"

#ifdef HAVE_3D_VIEW
#include "figure_pack.h"
#endif

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTimer>
#include <QStandardItemModel>
#include <QTemporaryDir>
#include <QToolButton>
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
    void figurePacks();
    void trashFigurePacks();
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
    Options::instance().setFigureFolder3D( QString() );
}

//! write a file of a test's figure pack
bool
writeFile( const QString & path,
           const QByteArray & data )
{
    QFile f( path );
    return f.open( QIODevice::WriteOnly ) && f.write( data ) == data.size();
}

/*!
  HOME and the XDG data folder in a folder of the test's, while it lives:
  the user's figure packs and the trash there, whoever runs the test
*/
struct HomeIn {
    QByteArray home;
    QByteArray data;
    explicit HomeIn( const QString & dir )
        : home( qgetenv( "HOME" ) ),
          data( qgetenv( "XDG_DATA_HOME" ) )
      {
          QDir().mkpath( dir + "/share" );
          qputenv( "HOME", QFile::encodeName( dir ) );
          qputenv( "XDG_DATA_HOME", QFile::encodeName( dir + "/share" ) );
      }
    ~HomeIn()
      {
          qputenv( "HOME", home );
          qputenv( "XDG_DATA_HOME", data );
      }
};


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
    for ( const char * name : { "closeup_distance_3d", "closeup_elevation_3d", "figure_scale_3d",
                                "player_camera_time_constant", "player_camera_max_turn",
                                "player_camera_fixed_fov" } )
    {
        QDoubleSpinBox * spin = child< QDoubleSpinBox >( dialog, name );
        QVERIFY( spin );
        QVERIFY2( spin->toolTip().contains( "Range: " ), name );
    }
    {
        QComboBox * model = child< QComboBox >( dialog, "player_model_3d" );
        QVERIFY( model );
        for ( int i = 0; i < model->count(); ++i )
        {
            QVERIFY2( ! model->itemData( i, Qt::ToolTipRole ).toString().isEmpty(), qPrintable( model->itemText( i ) ) );
        }
        QVERIFY( ! child< QToolButton >( dialog, "figure_folder_3d" )->toolTip().isEmpty() );
    }
    for ( const char * name : { "compass_3d", "minimap_3d", "figure_bodies_3d", "shadows_3d", "grass_3d", "player_camera_position",
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
    // the items with a tooltip marked by a dotted underline (TooltipMarker)
    {
        QLabel * compass = nullptr;
        for ( QLabel * l : dialog.findChildren< QLabel * >() )
        {
            if ( l->text() == "Compass:" ) compass = l;
        }
        QVERIFY( compass && ! compass->toolTip().isEmpty() );
        dialog.show();
        const QImage with = compass->grab().toImage();
        const QString tip = compass->toolTip();
        compass->setToolTip( QString() );
        const QImage without = compass->grab().toImage();
        compass->setToolTip( tip );
        QVERIFY( with != without );
    }
    // the notes of the groups in their tooltips, not below them (compact)
    for ( const char * title : { "Close-up Camera", "Guides", "Players", "Ball", "Stadium", "Player Sight" } )
    {
        bool found = false;
        for ( const QGroupBox * g : dialog.findChildren< QGroupBox * >() )
        {
            if ( g->title() == title )
            {
                found = true;
                QVERIFY2( ! g->toolTip().isEmpty(), title );
            }
        }
        QVERIFY2( found, title );
    }
    QVERIFY( ! child< QCheckBox >( dialog, "boards_3d" )->toolTip().isEmpty() );
    QVERIFY( ! child< QCheckBox >( dialog, "banners_3d" )->toolTip().isEmpty() );
    QVERIFY( ! child< QCheckBox >( dialog, "sky_3d" )->toolTip().isEmpty() );
    QVERIFY( ! child< QCheckBox >( dialog, "stands_3d" )->toolTip().isEmpty() );
    QVERIFY( ! child< QCheckBox >( dialog, "ball_pattern_3d" )->toolTip().isEmpty() );
    QVERIFY( child< QPushButton >( dialog, "default_3d" )->toolTip().contains( "Ball Pattern On, Flight Off, Ring On (0.7 m), Trail On (5 cycles)," ) );
    QVERIFY( child< QPushButton >( dialog, "default_3d" )->toolTip().contains( "Minimap Off, FPS Off," ) );
    QVERIFY( ! child< QCheckBox >( dialog, "fps_3d" )->toolTip().isEmpty() );
    for ( const char * name : { "ball_ring_3d", "ball_trail_3d" } )
    {
        QVERIFY2( ! child< QCheckBox >( dialog, name )->toolTip().isEmpty(), name );
    }
    for ( const char * name : { "ball_ring_radius_3d", "ball_trail_cycles_3d" } )
    {
        QVERIFY2( child< QDoubleSpinBox >( dialog, name )->toolTip().contains( "Range:" ), name );
    }
    QVERIFY( child< QPushButton >( dialog, "default_3d" )->toolTip().contains( "Stands On, Grass Stripes, Sky On" ) );
    QVERIFY( child< QPushButton >( dialog, "default_3d" )->toolTip().contains( "Close-up 12 m, 20" ) );
    QVERIFY( child< QPushButton >( dialog, "default_3d" )->toolTip().contains( "Model Simple, Figure Scale 1, Bodies Mixed, Shadows Simple," ) );
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
    QButtonGroup * grass = child< QButtonGroup >( dialog, "grass_3d" );
    QCheckBox * sky = child< QCheckBox >( dialog, "sky_3d" );
    QCheckBox * stands = child< QCheckBox >( dialog, "stands_3d" );
    QCheckBox * pattern = child< QCheckBox >( dialog, "ball_pattern_3d" );
    QCheckBox * fps = child< QCheckBox >( dialog, "fps_3d" );
    QVERIFY( fps );
    QVERIFY( ! fps->isChecked() );
    QCheckBox * flight = child< QCheckBox >( dialog, "ball_flight_3d" );
    QVERIFY( flight );
    QVERIFY( ! flight->isChecked() );
    QVERIFY( ! flight->toolTip().isEmpty() );
    QCheckBox * ring = child< QCheckBox >( dialog, "ball_ring_3d" );
    QCheckBox * trail = child< QCheckBox >( dialog, "ball_trail_3d" );
    QDoubleSpinBox * ring_radius = child< QDoubleSpinBox >( dialog, "ball_ring_radius_3d" );
    QDoubleSpinBox * trail_cycles = child< QDoubleSpinBox >( dialog, "ball_trail_cycles_3d" );
    QVERIFY( grass && sky && stands && pattern && ring && trail && ring_radius && trail_cycles );
    QPushButton * defaults = child< QPushButton >( dialog, "default_3d" );
    QComboBox * model = child< QComboBox >( dialog, "player_model_3d" );
    QLabel * credits = child< QLabel >( dialog, "figure_credits_3d" );
    QDoubleSpinBox * scale = child< QDoubleSpinBox >( dialog, "figure_scale_3d" );
    QButtonGroup * bodies = child< QButtonGroup >( dialog, "figure_bodies_3d" );
    QButtonGroup * shadows = child< QButtonGroup >( dialog, "shadows_3d" );
    QVERIFY( distance && elevation && compass && minimap && boards && banners && defaults && model && credits && scale && bodies && shadows );

    // the options at first
    QCOMPARE( distance->value(), Options::CLOSEUP_DISTANCE_3D );
    QCOMPARE( elevation->value(), Options::CLOSEUP_ELEVATION_3D );
    QCOMPARE( compass->checkedButton()->text(), QString( "Off" ) );
    QCOMPARE( minimap->checkedButton()->text(), QString( "Off" ) );
    QVERIFY( boards->isChecked() );
    QVERIFY( banners->isChecked() );
    QCOMPARE( grass->buttons().size(), 3 );
    QCOMPARE( grass->checkedButton()->text(), QString( "Stripes" ) );
    QVERIFY( sky->isChecked() );
    QVERIFY( stands->isChecked() );
    QVERIFY( pattern->isChecked() );
    QVERIFY( ring->isChecked() );
    QVERIFY( trail->isChecked() );
    QCOMPARE( ring_radius->value(), Options::BALL_RING_RADIUS_3D );
    QCOMPARE( trail_cycles->value(), double( Options::BALL_TRAIL_CYCLES_3D ) );
    QCOMPARE( trail_cycles->decimals(), 0 );
    QVERIFY( ring_radius->isEnabled() && trail_cycles->isEnabled() );
    QCOMPARE( model->currentText(), QString( "Simple" ) );
    QCOMPARE( model->currentData().toString(), QString( "simple" ) );
    QVERIFY( credits->text().isEmpty() ); // no credits of the cylinders
    QCOMPARE( scale->value(), Options::FIGURE_SCALE_3D );
    // the scale is not for the cylinders, the bodies only for the human figures
    QVERIFY( ! scale->isEnabled() );
    QCOMPARE( bodies->checkedButton()->text(), QString( "Mixed" ) );
    QVERIFY( ! bodies->button( 0 )->isEnabled() );
    QCOMPARE( shadows->buttons().size(), 3 );
    QCOMPARE( shadows->checkedButton()->text(), QString( "Simple" ) );
    for ( QAbstractButton * b : shadows->buttons() )
    {
        QVERIFY( b->isEnabled() ); // the cylinders cast them too
    }

    // the players' model (the cylinders, then the bundled packs, then
    // those of the user's if any), and the figures' scale (with a figure)
    QVERIFY( model->count() >= 3 );
    QCOMPARE( model->itemText( 1 ), QString( "Mannequin" ) );
    QCOMPARE( model->itemData( 1 ).toString(), QString( "mannequin" ) );
    QCOMPARE( model->itemText( 2 ), QString( "Human" ) );
    QCOMPARE( model->itemData( 2 ).toString(), QString( "human" ) );
    model->setCurrentIndex( 2 );
    emit model->activated( 2 );
    QCOMPARE( opt.playerModel3D(), QString( "human" ) );
    // its credits and its motion's (the Mannequin's), in short, in full in the tooltip
    QCOMPARE( credits->text(), QString( "Credits: Quaternius (CC0 1.0)" ) );
    QVERIFY( credits->toolTip().contains( "Universal Base Characters [Standard] by Quaternius (CC0 1.0)" ) );
    QVERIFY( credits->toolTip().contains( "Universal Animation Library [Standard] by Quaternius (CC0 1.0)" ) );
    QVERIFY( credits->toolTip().contains( "https://quaternius.com/" ) );
    QVERIFY( scale->isEnabled() );
    QVERIFY( bodies->button( 2 )->isEnabled() );
    QCOMPARE( bodies->button( 2 )->text(), QString( "Female" ) );
    bodies->button( 2 )->click();
    QCOMPARE( opt.figureBodies3D(), FigureBodies::Female );
    bodies->button( 1 )->click();
    QCOMPARE( opt.figureBodies3D(), FigureBodies::Male );
    model->setCurrentIndex( 1 );
    emit model->activated( 1 );
    QCOMPARE( opt.playerModel3D(), QString( "mannequin" ) );
    QVERIFY( ! bodies->button( 1 )->isEnabled() ); // one body of anyone's
    QCOMPARE( credits->text(), QString( "Credits: Quaternius (CC0 1.0)" ) );
    QVERIFY( ! credits->toolTip().contains( "Universal Base Characters" ) );
    QCOMPARE( spy.count(), 4 );
    scale->setValue( 1.25 );
    QCOMPARE( opt.figureScale3D(), 1.25 );
    QCOMPARE( spy.count(), 5 );
    spy.clear();

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
    QCOMPARE( shadows->button( 2 )->text(), QString( "Detailed" ) );
    shadows->button( 2 )->click();
    QCOMPARE( opt.shadows3D(), ShadowMode::Detailed );
    shadows->button( 0 )->click();
    QCOMPARE( opt.shadows3D(), ShadowMode::Off );
    QCOMPARE( grass->button( 2 )->text(), QString( "Realistic" ) );
    grass->button( 2 )->click();
    QCOMPARE( opt.grass3D(), GrassMode::Realistic );
    grass->button( 0 )->click();
    QCOMPARE( opt.grass3D(), GrassMode::Plain );
    sky->click();
    QVERIFY( ! opt.sky3D() );
    stands->click();
    QVERIFY( ! opt.stands3D() );
    pattern->click();
    QVERIFY( ! opt.ballPattern3D() );
    QCOMPARE( spy.count(), 13 );
    ring_radius->setValue( 2.5 );
    QCOMPARE( opt.ballRingRadius3D(), 2.5 );
    trail_cycles->setValue( 12.0 );
    QCOMPARE( opt.ballTrailCycles3D(), 12 );
    ring->click();
    QVERIFY( ! opt.ballRing3D() );
    QVERIFY( ! ring_radius->isEnabled() ); // the radius only with the ring
    QVERIFY( trail_cycles->isEnabled() );
    trail->click();
    QVERIFY( ! opt.ballTrail3D() );
    QVERIFY( ! trail_cycles->isEnabled() );
    fps->click();
    QVERIFY( opt.fps3D() );
    flight->click();
    QVERIFY( opt.ballFlight3D() );
    QCOMPARE( spy.count(), 19 );

    // Default: the options and the fields back
    defaults->click();
    QCOMPARE( spy.count(), 20 );
    QVERIFY( ! opt.ballFlight3D() );
    QVERIFY( ! flight->isChecked() );
    QVERIFY( ! opt.fps3D() );
    QVERIFY( ! fps->isChecked() );
    QVERIFY( opt.ballRing3D() && opt.ballTrail3D() );
    QVERIFY( ring->isChecked() && trail->isChecked() );
    QCOMPARE( opt.ballRingRadius3D(), Options::BALL_RING_RADIUS_3D );
    QCOMPARE( opt.ballTrailCycles3D(), Options::BALL_TRAIL_CYCLES_3D );
    QCOMPARE( ring_radius->value(), Options::BALL_RING_RADIUS_3D );
    QCOMPARE( trail_cycles->value(), double( Options::BALL_TRAIL_CYCLES_3D ) );
    QVERIFY( ring_radius->isEnabled() && trail_cycles->isEnabled() );
    QVERIFY( opt.stands3D() );
    QVERIFY( stands->isChecked() );
    QVERIFY( opt.ballPattern3D() );
    QVERIFY( pattern->isChecked() );
    QCOMPARE( opt.grass3D(), GrassMode::Stripes );
    QVERIFY( opt.sky3D() );
    QCOMPARE( grass->checkedId(), 1 );
    QVERIFY( sky->isChecked() );
    QCOMPARE( opt.closeupDistance3D(), Options::CLOSEUP_DISTANCE_3D );
    QCOMPARE( opt.closeupElevation3D(), Options::CLOSEUP_ELEVATION_3D );
    QCOMPARE( opt.compass3D(), GuideMode::Off );
    QCOMPARE( opt.minimap3D(), GuideMode::Off );
    QVERIFY( opt.boards3D() );
    QVERIFY( opt.banners3D() );
    QCOMPARE( opt.shadows3D(), ShadowMode::Simple );
    QCOMPARE( distance->value(), Options::CLOSEUP_DISTANCE_3D );
    QCOMPARE( elevation->value(), Options::CLOSEUP_ELEVATION_3D );
    QCOMPARE( compass->checkedId(), 0 );
    QCOMPARE( minimap->checkedId(), 0 );
    QVERIFY( boards->isChecked() );
    QVERIFY( banners->isChecked() );
    QCOMPARE( shadows->checkedId(), 1 );
    QCOMPARE( opt.playerModel3D(), QString( Options::PLAYER_MODEL_3D ) );
    QCOMPARE( opt.figureScale3D(), Options::FIGURE_SCALE_3D );
    QCOMPARE( opt.figureBodies3D(), Options::FIGURE_BODIES_3D );
    QCOMPARE( bodies->checkedId(), 0 );
    QCOMPARE( model->currentIndex(), 0 );
    QCOMPARE( scale->value(), Options::FIGURE_SCALE_3D );
    QVERIFY( ! scale->isEnabled() );
    // the fields set back by Default do not set the options again
    QCOMPARE( spy.count(), 20 );
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
  the figure packs in the list of the models: those of a folder of the
  user's choice after the bundled ones, its name the folder's; one that
  cannot be read told but not chosen, why in its tooltip; one that could
  not be drawn told, and chosen to try it again; the credits of the one
  chosen, or why it is wrong; a name of no pack; the bodies only for a
  pack with men's and women's; no folder again
*/
void
TestConfigDialog::figurePacks()
{
#ifndef HAVE_3D_VIEW
    QSKIP( "without the 3D view" );
#else
    Options & opt = Options::instance();
    resetView3D();

    // a folder of two packs: a robot of the Mannequin's model and motion, and a broken one
    QTemporaryDir folder;
    QVERIFY( folder.isValid() );
    QDir dir( folder.path() );
    QVERIFY( dir.mkpath( "robot" ) && dir.mkpath( "broken" ) );
    QVERIFY( QFile::copy( ":/figures/mannequin/Mannequin.glb", dir.filePath( "robot/Robot.glb" ) ) );
    QVERIFY( writeFile( dir.filePath( "robot/figure.json" ),
                        "{ \"format\": 1, \"name\": \"Robot\", \"description\": \"A robot.\",\n"
                        "  \"credits\": [ { \"title\": \"Robot\", \"author\": \"Someone\", \"license\": \"CC BY 4.0\" } ],\n"
                        "  \"motion\": \"mannequin\",\n"
                        "  \"bodies\": [ { \"model\": \"Robot.glb\", \"paint\": { \"M_Main\": \"team\" } } ] }\n" ) );
    QVERIFY( writeFile( dir.filePath( "broken/figure.json" ), "{ \"format\": 1,\n  \"name\": }\n" ) );
    // and a pack of many credits
    QVERIFY( dir.mkpath( "wordy" ) );
    QVERIFY( QFile::copy( ":/figures/mannequin/Mannequin.glb", dir.filePath( "wordy/Robot.glb" ) ) );
    {
        QByteArray credits;
        for ( int i = 0; i < 12; ++i )
        {
            credits += QByteArray( i == 0 ? "" : ", " ) + "{ \"author\": \"Someone With A Long Name " + QByteArray::number( i )
                + "\", \"license\": \"CC BY 4.0\" }";
        }
        QVERIFY( writeFile( dir.filePath( "wordy/figure.json" ),
                            "{ \"format\": 1, \"name\": \"Wordy\", \"credits\": [ " + credits + " ], \"motion\": \"mannequin\",\n"
                            "  \"bodies\": [ { \"model\": \"Robot.glb\" } ] }\n" ) );
    }

    DispHolder holder;
    ConfigDialog dialog( nullptr, holder );
    dialog.show();
    QSignalSpy spy( &dialog, SIGNAL( configured() ) );
    QComboBox * model = child< QComboBox >( dialog, "player_model_3d" );
    QLabel * credits = child< QLabel >( dialog, "figure_credits_3d" );
    QToolButton * packs = child< QToolButton >( dialog, "figure_folder_3d" );
    QButtonGroup * bodies = child< QButtonGroup >( dialog, "figure_bodies_3d" );
    QVERIFY( model && credits && packs && bodies );
    QVERIFY( packs->menu() );
    QAction * no_folder = packs->menu()->findChild< QAction * >( "no_figure_folder_3d" );
    QVERIFY( no_folder );
    QVERIFY( ! no_folder->isEnabled() ); // no folder yet
    QVERIFY( model->findData( "robot" ) < 0 );
    const auto enabled = [model]( const int i ) {
        const QStandardItemModel * items = qobject_cast< const QStandardItemModel * >( model->model() );
        return items && items->item( i )->isEnabled();
    };

    // the folder: its packs after the bundled ones
    dialog.setFigureFolder( folder.path() );
    QCOMPARE( spy.count(), 1 );
    QCOMPARE( opt.figureFolder3D(), folder.path() );
    QVERIFY( no_folder->isEnabled() );
    QVERIFY( packs->toolTip().contains( folder.path() ) );
    int robot = model->findData( "robot" );
    const int broken = model->findData( "broken" );
    QVERIFY( robot > 2 && broken > 2 );
    QCOMPARE( model->itemText( robot ), QString( "Robot" ) );
    QVERIFY( enabled( robot ) );
    QVERIFY( model->itemData( robot, Qt::ToolTipRole ).toString().contains( "A robot." ) );
    QVERIFY( model->itemData( robot, Qt::ToolTipRole ).toString().contains( "Robot by Someone (CC BY 4.0)" ) );
    QVERIFY( model->itemData( robot, Qt::ToolTipRole ).toString().contains( dir.filePath( "robot" ) ) );
    QCOMPARE( model->itemText( broken ), QString( "broken (cannot be read)" ) );
    QVERIFY( ! enabled( broken ) );
    QVERIFY2( model->itemData( broken, Qt::ToolTipRole ).toString().contains( "figure.json" )
              && model->itemData( broken, Qt::ToolTipRole ).toString().contains( "line 2" ),
              qPrintable( model->itemData( broken, Qt::ToolTipRole ).toString() ) );

    // a pack put while the dialog is open: looked for when it shows again
    QVERIFY( dir.mkpath( "late" ) );
    QVERIFY( QFile::copy( ":/figures/mannequin/Mannequin.glb", dir.filePath( "late/Robot.glb" ) ) );
    QVERIFY( QFile::copy( dir.filePath( "robot/figure.json" ), dir.filePath( "late/figure.json" ) ) );
    dialog.update3D();
    QVERIFY( model->findData( "late" ) < 0 );
    dialog.hide();
    dialog.show();
    QVERIFY( model->findData( "late" ) > 2 );
    robot = model->findData( "robot" );

    // chosen: its name is the folder's; its credits and its motion's
    model->setCurrentIndex( robot );
    emit model->activated( robot );
    QCOMPARE( opt.playerModel3D(), QString( "robot" ) );
    QCOMPARE( spy.count(), 2 );
    QCOMPARE( credits->text(), QString( "Credits: Someone (CC BY 4.0); Quaternius (CC0 1.0)" ) );
    QVERIFY( credits->styleSheet().isEmpty() );
    QVERIFY( ! bodies->button( 0 )->isEnabled() ); // no men's and women's bodies

    // a long line of credits: as a whole (elided when drawn, as wide as the
    // dialog is), not widening the dialog
    // (the 3D tab shown: its layout of a fixed size settles in the event loop)
    QTabWidget * tabs = dialog.findChild< QTabWidget * >();
    QVERIFY( tabs );
    tabs->setCurrentIndex( tabNames( dialog ).indexOf( "3D" ) );
    const auto width = [&dialog, credits]() {
        for ( int i = 0; i < 5; ++i ) QCoreApplication::processEvents(); // a layout, then its parent's
        return std::make_pair( dialog.sizeHint().width(), credits->parentWidget()->parentWidget()->width() );
    };
    const std::pair< int, int > before = width();
    const int dialog_width = before.first;
    const int wordy = model->findData( "wordy" );
    QVERIFY( wordy > 2 );
    model->setCurrentIndex( wordy );
    emit model->activated( wordy );
    QVERIFY( credits->text().startsWith( "Credits: Someone With A Long Name 0 (CC BY 4.0); " ) );
    QVERIFY( credits->text().endsWith( "Someone With A Long Name 11 (CC BY 4.0); Quaternius (CC0 1.0)" ) ); // and its motion's
    QVERIFY( credits->fontMetrics().horizontalAdvance( credits->text() ) > 2 * dialog_width );
    QVERIFY( width() == before );
    model->setCurrentIndex( model->findData( "robot" ) );
    emit model->activated( model->findData( "robot" ) );
    QCOMPARE( opt.playerModel3D(), QString( "robot" ) );

    // one that could not be drawn: told, and it can be chosen again
    FigurePacks::noteError( "robot", "Robot: Robot.glb: no clip \"Run\"" );
    dialog.update3D();
    QCOMPARE( model->itemText( model->currentIndex() ), QString( "Robot (cannot be drawn)" ) );
    QVERIFY( enabled( model->currentIndex() ) );
    QVERIFY( model->itemData( model->currentIndex(), Qt::ToolTipRole ).toString().startsWith( "Robot: Robot.glb: no clip" ) );
    QVERIFY( credits->text().startsWith( "Cannot be drawn: Robot: Robot.glb" ) );
    QVERIFY( ! credits->styleSheet().isEmpty() );
    FigurePacks::noteError( "robot", QString() );
    dialog.update3D();
    QCOMPARE( model->itemText( model->currentIndex() ), QString( "Robot" ) );

    // no folder again: the name of no pack, told, the players as cylinders
    no_folder->trigger();
    QVERIFY( opt.figureFolder3D().isEmpty() );
    QCOMPARE( opt.playerModel3D(), QString( "robot" ) );
    QVERIFY( model->findData( "broken" ) < 0 );
    QCOMPARE( model->currentText(), QString( "robot (not found)" ) );
    QVERIFY( ! enabled( model->currentIndex() ) );
    QVERIFY( credits->text().startsWith( "No figure pack \"robot\"" ) );
    QVERIFY( ! credits->styleSheet().isEmpty() );

    // the Human: the bodies of men and women
    model->setCurrentIndex( 2 );
    emit model->activated( 2 );
    QCOMPARE( opt.playerModel3D(), QString( "human" ) );
    QVERIFY( bodies->button( 0 )->isEnabled() );
    resetView3D();
    FigurePacks::refresh();
#endif
}

/*-------------------------------------------------------------------*/
/*!
  a figure pack of the user's moved to the trash from the Packs menu: the
  user's packs listed (not the bundled ones), the folder told where two
  have the same name, one that cannot be read too; asked first, Cancel
  keeps it; Yes moves its folder to the trash, with its files; the one
  chosen moved: the players as cylinders; the folder chosen, a pack
  itself, moved: no folder; none of the user's left: the item off
*/
void
TestConfigDialog::trashFigurePacks()
{
#ifndef HAVE_3D_VIEW
    QSKIP( "without the 3D view" );
#else
    Options & opt = Options::instance();
    resetView3D();
    QTemporaryDir home;
    QVERIFY( home.isValid() );
    const HomeIn in( home.path() );
    const QString user = FigurePacks::userFolders().value( 0 );
    const QString trash = QStandardPaths::writableLocation( QStandardPaths::GenericDataLocation ) + "/Trash";
    QCOMPARE( user, home.filePath( "share/rcssmonitor/figures" ) );
    QCOMPARE( trash, home.filePath( "share/Trash" ) );

    const auto pack = []( const QString & dir, const QByteArray & name ) {
        return QDir().mkpath( dir )
            && QFile::copy( ":/figures/mannequin/Mannequin.glb", dir + "/Robot.glb" )
            && writeFile( dir + "/figure.json", "{ \"format\": 1, \"name\": \"" + name + "\", \"motion\": \"mannequin\",\n"
                          "  \"bodies\": [ { \"model\": \"Robot.glb\" } ] }\n" );
    };
    // the user's: a pack, another of the same name, a broken one; a folder of the user's choice, a pack itself
    QVERIFY( pack( user + "/doomed", "Doomed" ) );
    QVERIFY( pack( user + "/twin", "Doomed" ) );
    QVERIFY( QDir().mkpath( user + "/broken" ) && writeFile( user + "/broken/figure.json", "{ \"format\": 1,\n  \"name\": }\n" ) );
    const QString chosen = home.filePath( "chosen" );
    QVERIFY( pack( chosen, "Chosen" ) );
    FigurePacks::refresh();

    DispHolder holder;
    ConfigDialog dialog( nullptr, holder );
    dialog.show();
    QSignalSpy spy( &dialog, SIGNAL( configured() ) );
    QComboBox * model = child< QComboBox >( dialog, "player_model_3d" );
    QToolButton * packs = child< QToolButton >( dialog, "figure_folder_3d" );
    QVERIFY( model && packs && packs->menu() );
    QMenu * menu = packs->menu()->findChild< QMenu * >( "trash_figure_pack_3d" );
    QVERIFY( menu );
    dialog.setFigureFolder( chosen );
    const auto items = [menu]() {
        QStringList list;
        for ( QAction * a : menu->actions() ) list << a->text();
        return list;
    };
    const auto actionOf = [menu]( const QString & dir ) -> QAction * {
        for ( QAction * a : menu->actions() )
        {
            if ( a->data().toString() == dir ) return a;
        }
        return nullptr;
    };
    // the question answered when it shows (it is asked after the menu is done with the action)
    QString asked;
    const auto answer = [&asked]( const QMessageBox::StandardButton button ) {
        asked.clear();
        QTimer * t = new QTimer;
        t->setInterval( 10 );
        QObject::connect( t, &QTimer::timeout, [t, button, &asked]() {
            QMessageBox * box = qobject_cast< QMessageBox * >( QApplication::activeModalWidget() );
            if ( ! box || box->objectName() != "trash_figure_pack_3d_question" ) return;
            asked = box->text() + "\n" + box->informativeText();
            if ( box->defaultButton() != box->button( QMessageBox::Cancel ) ) asked += "\n(not Cancel by default)";
            t->stop();
            t->deleteLater();
            box->button( button )->click();
        } );
        t->start();
    };
    const auto choose = [&]( const QString & id ) {
        const int i = model->findData( id );
        model->setCurrentIndex( i );
        emit model->activated( i );
    };

    // the user's listed (by the folders where the names are the same), not the bundled ones
    QVERIFY( menu->menuAction()->isEnabled() );
    QCOMPARE( items(), QStringList( { "Chosen", "broken (cannot be read)",
                                      "Doomed (" + user + "/doomed)", "Doomed (" + user + "/twin)" } ) );
    QCOMPARE( actionOf( user + "/doomed" )->toolTip(), user + "/doomed" );
    choose( "doomed" );
    QCOMPARE( opt.playerModel3D(), QString( "doomed" ) );

    // asked first: Cancel keeps it
    const int count = spy.count();
    answer( QMessageBox::Cancel );
    actionOf( user + "/doomed" )->trigger();
    QTRY_VERIFY( ! asked.isEmpty() );
    QVERIFY2( asked.contains( "\"Doomed\"" ) && asked.contains( user + "/doomed" ), qPrintable( asked ) );
    QVERIFY2( ! asked.contains( "not Cancel" ), qPrintable( asked ) ); // Enter does not move it
    QVERIFY( QFileInfo( user + "/doomed/figure.json" ).isFile() );
    QCOMPARE( opt.playerModel3D(), QString( "doomed" ) );
    QCOMPARE( spy.count(), count );

    // Yes: its folder with its files in the trash (restorable); it was chosen: the players as cylinders
    answer( QMessageBox::Yes );
    actionOf( user + "/doomed" )->trigger();
    QTRY_VERIFY( ! asked.isEmpty() );
    QVERIFY( ! QFileInfo::exists( user + "/doomed" ) );
    QVERIFY( QFileInfo( trash + "/files/doomed/figure.json" ).isFile() );
    QVERIFY( QFileInfo( trash + "/files/doomed/Robot.glb" ).isFile() );
    QVERIFY( QFileInfo( trash + "/info/doomed.trashinfo" ).isFile() );
    QCOMPARE( opt.playerModel3D(), QString( PLAYER_MODEL_SIMPLE ) );
    QCOMPARE( model->currentIndex(), 0 );
    QVERIFY( model->findData( "doomed" ) < 0 );
    QVERIFY( spy.count() > count );
    QCOMPARE( items(), QStringList( { "Chosen", "broken (cannot be read)", "Doomed" } ) ); // the other one alone now

    // one not chosen (that cannot be read): the model stays
    choose( "twin" );
    answer( QMessageBox::Yes );
    actionOf( user + "/broken" )->trigger();
    QTRY_VERIFY( ! asked.isEmpty() );
    QVERIFY( ! QFileInfo::exists( user + "/broken" ) );
    QCOMPARE( opt.playerModel3D(), QString( "twin" ) );

    // the folder chosen, a pack itself: no folder any more
    answer( QMessageBox::Yes );
    actionOf( chosen )->trigger();
    QTRY_VERIFY( ! asked.isEmpty() );
    QVERIFY( ! QFileInfo::exists( chosen ) );
    QVERIFY( opt.figureFolder3D().isEmpty() );
    QCOMPARE( opt.playerModel3D(), QString( "twin" ) );

    // the last of the user's: none left, the item off
    answer( QMessageBox::Yes );
    actionOf( user + "/twin" )->trigger();
    QTRY_VERIFY( ! asked.isEmpty() );
    QCOMPARE( opt.playerModel3D(), QString( PLAYER_MODEL_SIMPLE ) );
    QVERIFY( menu->actions().isEmpty() );
    QVERIFY( ! menu->menuAction()->isEnabled() );
    QVERIFY( QDir( user ).exists() ); // the user's folder of packs stays

    resetView3D();
    FigurePacks::refresh();
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

    // compact: no line below a row (the summaries are in the tooltips)
    for ( const QButtonGroup * group : { position, direction, motion, fov, sight } )
    {
        for ( const QAbstractButton * b : group->buttons() )
        {
            const QString summary = b->property( "summary" ).toString();
            QVERIFY( ! summary.isEmpty() );
            for ( const QLabel * l : dialog.findChildren< QLabel * >() )
            {
                QVERIFY2( l->text() != summary, qPrintable( summary ) );
            }
        }
    }
    QVERIFY( position->button( 1 )->toolTip().contains( "6 m behind the player and 3 m high" ) );
    QVERIFY( position->button( 0 )->toolTip().startsWith( "At the eyes" ) );

    position->button( 0 )->click();
    QCOMPARE( opt.playerCamera().position, Position::Eye );
    direction->button( 2 )->click();
    QCOMPARE( opt.playerCamera().direction, Direction::Middle );
    motion->button( 0 )->click();
    QCOMPARE( opt.playerCamera().motion, Motion::Instant );
    fov->button( 1 )->click();
    QCOMPARE( opt.playerCamera().fov, Fov::Eased );
    sight->button( 0 )->click();
    QCOMPARE( opt.playerCamera().sight, Sight::All );
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
