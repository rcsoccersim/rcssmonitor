// -*-c++-*-

/*!
  \file test_export_dialog.cpp
  \brief unit test of File > Export Images / Video: ExportDialog
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

#include "disp_holder.h"
#include "export_dialog.h"
#include "field_canvas.h"
#include "main_window.h"
#include "options.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QTemporaryDir>

/*
  docs/plan/2026-11-22-03_frame-export.md 4.4, 4.6, S1, S7, S8, D6, §6.
  test/data/head_v5.rcg has 3 frames: the cycles 1, 2 and 4. CTest runs
  this test with HOME in the build tree and the offscreen platform.
*/

namespace {

const QString LOG = QStringLiteral( TEST_DATA_DIR "/head_v5.rcg" );

template < typename T >
T *
child( const QObject & parent,
       const char * name )
{
    return parent.findChild< T * >( QString::fromLatin1( name ) );
}

//! the dialog, with the images small, in a directory
ExportDialog *
openDialog( MainWindow & window,
            const QString & dir )
{
    QAction * act = window.findChild< QAction * >( "export" );
    if ( ! act ) return nullptr;
    act->trigger();
    ExportDialog * dialog = window.exportDialog();
    if ( ! dialog ) return nullptr;
    QComboBox * size = child< QComboBox >( *dialog, "size" );
    size->setCurrentIndex( size->findData( "custom" ) );
    child< QSpinBox >( *dialog, "width" )->setValue( 160 );
    child< QSpinBox >( *dialog, "height" )->setValue( 120 );
    child< QCheckBox >( *dialog, "scale_texts" )->setChecked( false );
    child< QDoubleSpinBox >( *dialog, "fps" )->setValue( 10.0 );
    child< QDoubleSpinBox >( *dialog, "speed" )->setValue( 1.0 );
    child< QCheckBox >( *dialog, "name_by_cycle" )->setChecked( false );
    child< QLineEdit >( *dialog, "prefix" )->setText( "img-" );
    dialog->setDirectory( dir );
    dialog->setAll();
    return dialog;
}

//! export and wait for the end: ok and the message, or "not started"
QString
exportNow( ExportDialog & dialog,
           const bool ask = false )
{
    QSignalSpy finished( &dialog, &ExportDialog::exportFinished );
    if ( ! dialog.startExport( ask ) ) return QStringLiteral( "not started" );
    if ( ! finished.wait( 10000 ) ) return QStringLiteral( "no end" );
    return ( finished.first().at( 0 ).toBool() ? QStringLiteral( "ok: " ) : QStringLiteral( "failed: " ) )
        + finished.first().at( 1 ).toString();
}

}

/*!
  \brief unit test of the export dialog
*/
class TestExportDialog
    : public QObject {
    Q_OBJECT

private slots:

    void action();
    void range();
    void exportImages();
    void namesByCycle();
    void sizes();
    void filesThere();
    void connected();
};

/*-------------------------------------------------------------------*/
/*!
  File > Export Images / Video (Ctrl+Shift+E): in the File menu after Save
  Image of This Frame; enabled with a game log; a modeless dialog, once.
*/
void
TestExportDialog::action()
{
    MainWindow window;
    window.show();
    QVERIFY( QTest::qWaitForWindowExposed( &window ) );

    QAction * act = window.findChild< QAction * >( "export" );
    QVERIFY( act );
    QCOMPARE( act->shortcut(), QKeySequence( Qt::CTRL | Qt::SHIFT | Qt::Key_E ) );
    QVERIFY( ! act->isEnabled() );
    QAction * save_image = window.findChild< QAction * >( "save_image" );
    QVERIFY( save_image );
    QCOMPARE( save_image->text(), QString( "Save &Image of This Frame..." ) );
    for ( QAction * menu : window.menuBar()->actions() )
    {
        if ( menu->menu() && menu->text() == "&File" )
        {
            const QList< QAction * > items = menu->menu()->actions();
            QVERIFY( items.contains( act ) );
            QCOMPARE( items.indexOf( act ), items.indexOf( save_image ) + 1 );
        }
    }

    window.openGameLogFile( LOG );
    QVERIFY( act->isEnabled() );
    QVERIFY( ! window.exportDialog() );
    act->trigger();
    ExportDialog * dialog = window.exportDialog();
    QVERIFY( dialog );
    QVERIFY( dialog->isVisible() );
    QVERIFY( ! dialog->isModal() );
    act->trigger();
    QCOMPARE( window.exportDialog(), dialog );
    QCOMPARE( window.findChildren< ExportDialog * >().size(), 1 );
}

/*-------------------------------------------------------------------*/
/*!
  The range: empty fields for the whole log, the positions now, the
  reasons of the wrong ones (the export disabled).
*/
void
TestExportDialog::range()
{
    MainWindow window;
    window.show();
    QVERIFY( QTest::qWaitForWindowExposed( &window ) );
    window.openGameLogFile( LOG );
    QTemporaryDir tmp;
    ExportDialog * dialog = openDialog( window, tmp.path() );
    QVERIFY( dialog );
    QPushButton * button = child< QPushButton >( *dialog, "export" );

    QString error;
    ExportRange r = dialog->range( &error );
    QCOMPARE( r.first, std::size_t( 0 ) );
    QCOMPARE( r.last, std::size_t( 2 ) );
    QVERIFY2( dialog->summaryText().startsWith( "3 images of 3 frames" ), qPrintable( dialog->summaryText() ) );
    QVERIFY( button->isEnabled() );

    // the positions now: the first frame shown after opening
    child< QPushButton >( *dialog, "start_now" )->click();
    QCOMPARE( child< QLineEdit >( *dialog, "start" )->text(), QString( "1" ) );
    QTest::keyClick( &window, Qt::Key_Right );
    child< QPushButton >( *dialog, "end_now" )->click();
    QCOMPARE( child< QLineEdit >( *dialog, "end" )->text(), QString( "2" ) );
    r = dialog->range( &error );
    QCOMPARE( r.first, std::size_t( 0 ) );
    QCOMPARE( r.last, std::size_t( 1 ) );
    QVERIFY2( dialog->summaryText().startsWith( "2 images of 2 frames" ), qPrintable( dialog->summaryText() ) );

    // a cycle missing: the next frame
    dialog->setStart( "3" );
    dialog->setEnd( "4" );
    r = dialog->range( &error );
    QCOMPARE( r.first, std::size_t( 2 ) );
    QVERIFY( dialog->findChildren< QLabel * >().size() > 0 );
    QVERIFY2( dialog->summaryText().startsWith( "1 image of 1 frame" ), qPrintable( dialog->summaryText() ) );

    // wrong ones
    dialog->setStart( "5" );
    QVERIFY( ! dialog->range( &error ).isValid() );
    QVERIFY2( error.contains( "after the last frame" ), qPrintable( error ) );
    QVERIFY( ! button->isEnabled() );
    dialog->setStart( "4" );
    dialog->setEnd( "2" );
    QVERIFY( ! dialog->range( &error ).isValid() );
    QCOMPARE( error, QString( "The start is after the end." ) );
    QCOMPARE( dialog->summaryText(), error );
    QVERIFY( ! button->isEnabled() );
    dialog->setStart( "abc" );
    QVERIFY( ! dialog->range( &error ).isValid() );
    QVERIFY2( error.contains( "Not a position" ), qPrintable( error ) );
    QVERIFY( ! dialog->startExport( false ) );
    QVERIFY2( dialog->messageText().contains( "Not a position" ), qPrintable( dialog->messageText() ) );

    dialog->setAll();
    QVERIFY( dialog->range( &error ).isValid() );
    QVERIFY( button->isEnabled() );
}

/*-------------------------------------------------------------------*/
/*!
  The images of the range, named by their numbers; the playback stopped;
  the frame shown and the view unchanged; the settings kept.
*/
void
TestExportDialog::exportImages()
{
    MainWindow window;
    window.show();
    QVERIFY( QTest::qWaitForWindowExposed( &window ) );
    window.openGameLogFile( LOG );
    QTemporaryDir tmp;
    ExportDialog * dialog = openDialog( window, tmp.filePath( "images" ) );
    QVERIFY( dialog );
    FieldCanvas * canvas = window.findChild< FieldCanvas * >();
    QVERIFY( canvas );

    QTest::keyClick( &window, Qt::Key_Right ); // the frame shown: 2
    QTest::qWait( 200 ); // the rows of the tool bars laid out again after opening the log
    const QSize view_size = canvas->size();
    const QImage view = canvas->grab().toImage();
    const Options::ViewState state = Options::instance().viewState();

    QSignalSpy stop( dialog, &ExportDialog::stopRequested );
    const QString result = exportNow( *dialog );
    QVERIFY2( result.startsWith( "ok: 3 images in " ), qPrintable( result ) );
    QCOMPARE( stop.count(), 1 );
    QVERIFY2( dialog->messageText().startsWith( "Exported 3 images" ), qPrintable( dialog->messageText() ) );
    QVERIFY2( window.statusBar()->currentMessage().startsWith( "Exported 3 images" ), qPrintable( window.statusBar()->currentMessage() ) );

    const QDir dir( tmp.filePath( "images" ) );
    QCOMPARE( dir.entryList( QDir::Files ), QStringList( { "img-00000.png", "img-00001.png", "img-00002.png" } ) );
    QCOMPARE( QImage( dir.filePath( "img-00002.png" ) ).size(), QSize( 160, 120 ) );
    QVERIFY( QImage( dir.filePath( "img-00000.png" ) ) != QImage( dir.filePath( "img-00002.png" ) ) );

    // nothing changed in the view
    QCOMPARE( canvas->size(), view_size );
    QVERIFY( canvas->grab().toImage() == view );
    QCOMPARE( Options::instance().viewState().canvas_width, state.canvas_width );
    QCOMPARE( Options::instance().viewState().field_scale, state.field_scale );

    // the settings written
    QCOMPARE( Options::instance().exportSettings().prefix, QString( "img-" ) );
    QCOMPARE( Options::instance().exportSettings().size, QString( "custom" ) );
    QCOMPARE( Options::instance().exportSettings().width, 160 );
    QCOMPARE( Options::instance().exportSettings().directory, QDir::cleanPath( tmp.filePath( "images" ) ) );
}

/*-------------------------------------------------------------------*/
//! named by the cycles when an image is a frame; the frames twice at half the speed
void
TestExportDialog::namesByCycle()
{
    MainWindow window;
    window.show();
    QVERIFY( QTest::qWaitForWindowExposed( &window ) );
    window.openGameLogFile( LOG );
    QTemporaryDir tmp;
    ExportDialog * dialog = openDialog( window, tmp.path() );
    QVERIFY( dialog );
    QCheckBox * by_cycle = child< QCheckBox >( *dialog, "name_by_cycle" );

    QVERIFY( dialog->canNameByCycle() );
    QVERIFY( by_cycle->isEnabled() );
    by_cycle->setChecked( true );
    QString error;
    QCOMPARE( dialog->imageNames( dialog->imageFrames( dialog->range( &error ) ) ),
              QStringList( { "img-c00001.00.png", "img-c00002.00.png", "img-c00004.00.png" } ) );

    // half the speed: 2 images a frame, by the numbers
    child< QDoubleSpinBox >( *dialog, "speed" )->setValue( 0.5 );
    QVERIFY( ! dialog->canNameByCycle() );
    QVERIFY( ! by_cycle->isEnabled() );
    const std::vector< std::size_t > frames = dialog->imageFrames( dialog->range( &error ) );
    QCOMPARE( frames.size(), std::size_t( 5 ) );
    QCOMPARE( dialog->imageNames( frames ).last(), QString( "img-00004.png" ) );
    QVERIFY2( dialog->summaryText().startsWith( "5 images of 3 frames" ), qPrintable( dialog->summaryText() ) );

    // the same speed at 20 images/s: one frame an image again
    child< QDoubleSpinBox >( *dialog, "fps" )->setValue( 20.0 );
    child< QDoubleSpinBox >( *dialog, "speed" )->setValue( 2.0 );
    QVERIFY( dialog->canNameByCycle() );
    const QString result = exportNow( *dialog );
    QVERIFY2( result.startsWith( "ok: " ), qPrintable( result ) );
    QCOMPARE( QDir( tmp.path() ).entryList( QDir::Files ),
              QStringList( { "img-c00001.00.png", "img-c00002.00.png", "img-c00004.00.png" } ) );
}

/*-------------------------------------------------------------------*/
//! the sizes: as the view, a fixed one; the texts scaled with the image or not
void
TestExportDialog::sizes()
{
    MainWindow window;
    window.resize( 800, 600 );
    window.show();
    QVERIFY( QTest::qWaitForWindowExposed( &window ) );
    window.openGameLogFile( LOG );
    QTemporaryDir tmp;
    ExportDialog * dialog = openDialog( window, tmp.path() );
    QVERIFY( dialog );
    FieldCanvas * canvas = window.findChild< FieldCanvas * >();
    QComboBox * size = child< QComboBox >( *dialog, "size" );
    QCheckBox * scale = child< QCheckBox >( *dialog, "scale_texts" );

    size->setCurrentIndex( size->findData( "view" ) );
    QCOMPARE( dialog->imageSize(), canvas->size() * canvas->devicePixelRatioF() );
    QVERIFY( ! child< QSpinBox >( *dialog, "width" )->isEnabled() );
    QCOMPARE( child< QSpinBox >( *dialog, "width" )->value(), dialog->imageSize().width() );
    scale->setChecked( true );
    QCOMPARE( dialog->imageRatio(), canvas->devicePixelRatioF() );

    size->setCurrentIndex( size->findData( "1920x1080" ) );
    QCOMPARE( dialog->imageSize(), QSize( 1920, 1080 ) );
    QCOMPARE( dialog->imageRatio(), 1080.0 / canvas->height() );
    scale->setChecked( false );
    QCOMPARE( dialog->imageRatio(), 1.0 );
    QVERIFY2( dialog->summaryText().contains( "1920 x 1080 pixels" ), qPrintable( dialog->summaryText() ) );
}

/*-------------------------------------------------------------------*/
//! the files there: asked once (cancel: not exported); without asking, overwritten
void
TestExportDialog::filesThere()
{
    MainWindow window;
    window.show();
    QVERIFY( QTest::qWaitForWindowExposed( &window ) );
    window.openGameLogFile( LOG );
    QTemporaryDir tmp;
    ExportDialog * dialog = openDialog( window, tmp.path() );
    QVERIFY( dialog );
    QVERIFY( exportNow( *dialog ).startsWith( "ok: " ) );
    const QDateTime written = QFileInfo( tmp.filePath( "img-00000.png" ) ).lastModified();

    int asked = 0;
    QString question;
    QTimer::singleShot( 200, [&]() {
        QMessageBox * box = qobject_cast< QMessageBox * >( QApplication::activeModalWidget() );
        if ( ! box ) return;
        ++asked;
        question = box->text();
        box->button( QMessageBox::Cancel )->click();
    } );
    QCOMPARE( exportNow( *dialog, true ), QString( "not started" ) );
    QCOMPARE( asked, 1 );
    QVERIFY2( question.startsWith( "3 of the 3 files are already in " ), qPrintable( question ) );
    QCOMPARE( QFileInfo( tmp.filePath( "img-00000.png" ) ).lastModified(), written );

    QTest::qWait( 20 );
    QVERIFY( exportNow( *dialog ).startsWith( "ok: " ) );
}

/*-------------------------------------------------------------------*/
//! connected to a server: no export (S1)
void
TestExportDialog::connected()
{
    MainWindow window;
    window.show();
    QVERIFY( QTest::qWaitForWindowExposed( &window ) );
    window.openGameLogFile( LOG );
    QTemporaryDir tmp;
    ExportDialog * dialog = openDialog( window, tmp.path() );
    QVERIFY( dialog );

    dialog->setExportAllowed( false );
    QVERIFY( ! child< QPushButton >( *dialog, "export" )->isEnabled() );
    QVERIFY( ! dialog->startExport( false ) );
    QVERIFY2( dialog->messageText().contains( "connected to a server" ), qPrintable( dialog->messageText() ) );
    QVERIFY( QDir( tmp.path() ).entryList( QDir::Files ).isEmpty() );

    dialog->setExportAllowed( true );
    QVERIFY( child< QPushButton >( *dialog, "export" )->isEnabled() );
}

QTEST_MAIN( TestExportDialog )
#include "test_export_dialog.moc"
