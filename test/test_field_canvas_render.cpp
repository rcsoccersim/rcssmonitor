// -*-c++-*-

/*!
  \file test_field_canvas_render.cpp
  \brief unit test of FieldCanvas::renderFrame(), FrameScope and File >
  Save Image of This Frame
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
#include "field_canvas.h"
#include "frame_scope.h"
#include "game_log_file.h"
#include "main_window.h"
#include "options.h"
#include "rcg_handler.h"

#include <QAction>
#include <QImageReader>
#include <QMenu>
#include <QMenuBar>
#include <QTemporaryDir>

#include <cstdlib>

/*
  docs/plan/2026-11-22-03_frame-export.md 4.1, 4.6, §6, D5.
  test/data/head_v5.rcg has 3 frames: the cycles 1, 2 and 4. CTest runs
  this test with HOME in the build tree and the offscreen platform (device
  pixel ratio 1).
*/

namespace {

const QString LOG = QStringLiteral( TEST_DATA_DIR "/head_v5.rcg" );

//! the image as RGB32, to compare
QImage
rgb32( const QImage & image )
{
    return image.convertToFormat( QImage::Format_RGB32 );
}

/*!
  the view drawn by its paintEvent() into an image (QWidget::render()). The
  texts drawn for a widget may have subpixel antialiasing (by the font
  settings), which Qt uses only for widgets: compared with renderFrame()
  without texts (showTexts()).
*/
QImage
drawn( QWidget & widget )
{
    const qreal dpr = widget.devicePixelRatioF();
    QImage image( widget.size() * dpr, QImage::Format_RGB32 );
    image.setDevicePixelRatio( dpr );
    widget.render( &image );
    return image;
}

//! the player numbers and types, and the score board: the texts of the 2D view
void
showTexts( const bool number,
           const bool type,
           const bool board )
{
    Options & opt = Options::instance();
    if ( opt.showPlayerNumber() != number ) opt.toggleShowPlayerNumber();
    if ( opt.showPlayerType() != type ) opt.toggleShowPlayerType();
    if ( opt.showScoreBoard() != board ) opt.toggleShowScoreBoard();
}

void
showTexts( const bool on )
{
    showTexts( on, on, on );
}

//! the state of the view, to compare
QString
textOf( const Options::ViewState & s )
{
    return QString( "scale %1 center (%2 %3) canvas %4x%5 (%6 %7) focus (%8 %9) selected %10" )
        .arg( s.field_scale ).arg( s.field_center.x() ).arg( s.field_center.y() )
        .arg( s.canvas_width ).arg( s.canvas_height )
        .arg( s.sized_with_team_graphic ).arg( s.sized_with_score_board )
        .arg( s.focus_point.x() ).arg( s.focus_point.y() )
        .arg( s.selected_number );
}

//! the leftmost pixel of a row near the color of the lines (the left goal line), or -1
int
leftLine( const QImage & image,
          const int y )
{
    const QColor line = Options::instance().linePen().color();
    for ( int x = 0; x < image.width(); ++x )
    {
        const QColor c = image.pixelColor( x, y );
        if ( std::abs( c.red() - line.red() ) + std::abs( c.green() - line.green() ) + std::abs( c.blue() - line.blue() ) < 60 )
        {
            return x;
        }
    }
    return -1;
}

}

/*!
  \brief unit test of drawing a frame into an image
*/
class TestFieldCanvasRender
    : public QObject {
    Q_OBJECT

private:

    DispHolder M_holder;
    //! the texts shown by default: set back for each test
    bool M_number = true;
    bool M_type = false;
    bool M_board = true;

private slots:

    void initTestCase()
      {
          RCGHandler handler( nullptr, M_holder );
          QVERIFY( loadGameLogFile( LOG.toStdString(), handler ) );
          QCOMPARE( M_holder.dispCont().size(), std::size_t( 3 ) );
          const Options & opt = Options::instance();
          M_number = opt.showPlayerNumber();
          M_type = opt.showPlayerType();
          M_board = opt.showScoreBoard();
      }

    void init()
      {
          Options & opt = Options::instance();
          opt.fitToScreen();
          opt.setFocusType( Options::FOCUS_POINT );
          opt.setFocusPointReal( 0.0, 0.0 );
          opt.setPlayerSelectType( Options::SELECT_FIX );
          showTexts( M_number, M_type, M_board );
          M_holder.setIndex( 1 );
      }

    void cleanup()
      {
          showTexts( M_number, M_type, M_board );
      }

    void sameAsView();
    void otherFrame();
    void stateKept();
    void indexInvalid();
    void devicePixelRatio();
    void zoomed();
    void emptySize();
    void saveImage();
};

/*-------------------------------------------------------------------*/
/*!
  The frame shown, as large as the view: the same image as the view draws.
  The texts are not compared with the view's: Qt draws them with subpixel
  antialiasing only for widgets (by the font settings), and then their
  advances may differ a little too (Qt 5).
  The score board hidden after the view was drawn: the scale fit again
  for the image, and the view's set back (the items shown when the scale
  was fit are a part of Options::ViewState).
*/
void
TestFieldCanvasRender::sameAsView()
{
    FieldCanvas canvas( M_holder );
    canvas.resize( 640, 480 );
    canvas.show();
    QVERIFY( QTest::qWaitForWindowExposed( &canvas ) );

    showTexts( false );
    const QImage image = canvas.renderFrame( 1, canvas.size(), 1.0 );
    QCOMPARE( image.size(), QSize( 640, 480 ) );
    QCOMPARE( image.format(), QImage::Format_RGB32 );
    QVERIFY( image == drawn( canvas ) );
    QVERIFY( image == rgb32( canvas.grab().toImage() ) );

    // the texts drawn, the same each time
    showTexts( true );
    const QImage texts = canvas.renderFrame( 1, canvas.size(), 1.0 );
    QVERIFY( texts != image );
    QVERIFY( canvas.renderFrame( 1, canvas.size(), 1.0 ) == texts );
}

/*-------------------------------------------------------------------*/
//! another frame: as the view draws it when it is shown; the frame shown stays
void
TestFieldCanvasRender::otherFrame()
{
    showTexts( false );
    FieldCanvas canvas( M_holder );
    canvas.resize( 640, 480 );
    canvas.show();
    QVERIFY( QTest::qWaitForWindowExposed( &canvas ) );

    const QImage shown = drawn( canvas );
    const QImage other = canvas.renderFrame( 2, canvas.size(), 1.0 );
    QCOMPARE( M_holder.currentIndex(), std::size_t( 1 ) );
    QVERIFY( other != shown );
    QVERIFY( drawn( canvas ) == shown );

    M_holder.setIndex( 2 );
    QVERIFY( drawn( canvas ) == other );

    // an index out of the frames: the frame shown
    QVERIFY( canvas.renderFrame( 100, canvas.size(), 1.0 ) == other );
}

/*-------------------------------------------------------------------*/
/*!
  The state of the view after drawing other frames in other sizes: the
  focus on the ball, the zoom and the player selected automatically.
*/
void
TestFieldCanvasRender::stateKept()
{
    Options & opt = Options::instance();
    opt.setFocusType( Options::FOCUS_BALL );
    opt.setPlayerSelectType( Options::SELECT_AUTO_ALL );
    opt.zoomIn();
    showTexts( false );

    FieldCanvas canvas( M_holder );
    canvas.resize( 640, 480 );
    canvas.show();
    QVERIFY( QTest::qWaitForWindowExposed( &canvas ) );
    const QImage view = drawn( canvas );
    const QString state = textOf( opt.viewState() );

    QVERIFY( ! canvas.renderFrame( 0, QSize( 320, 240 ), 1.0 ).isNull() );
    QVERIFY( ! canvas.renderFrame( 2, QSize( 1920, 1080 ), 1.0 ).isNull() );
    QVERIFY( ! canvas.renderFrame( 2, QSize( 1280, 960 ), 2.0 ).isNull() );

    QCOMPARE( textOf( opt.viewState() ), state );
    QCOMPARE( M_holder.currentIndex(), std::size_t( 1 ) );
    QVERIFY( opt.zoomed() );
    QCOMPARE( opt.focusType(), Options::FOCUS_BALL );
    QCOMPARE( opt.playerSelectType(), Options::SELECT_AUTO_ALL );
    QVERIFY( drawn( canvas ) == view );

    // FrameScope alone
    {
        FrameScope scope( M_holder, 0 );
        QCOMPARE( M_holder.currentIndex(), std::size_t( 0 ) );
        opt.updateFieldSize( 100, 100 );
        opt.setFocusPointReal( 10.0, 10.0 );
        opt.setSelectedNumber( rcss::rcg::LEFT, 3 );
    }
    QCOMPARE( M_holder.currentIndex(), std::size_t( 1 ) );
    QCOMPARE( textOf( opt.viewState() ), state );
}

/*-------------------------------------------------------------------*/
//! no frame chosen (INVALID_INDEX: the newest one drawn): set back as it was
void
TestFieldCanvasRender::indexInvalid()
{
    FieldCanvas canvas( M_holder );
    canvas.resize( 320, 240 );
    M_holder.restoreIndex( DispHolder::INVALID_INDEX );

    const QImage newest = canvas.renderFrame( DispHolder::INVALID_INDEX, canvas.size(), 1.0 );
    QCOMPARE( M_holder.currentIndex(), DispHolder::INVALID_INDEX );
    QVERIFY( canvas.renderFrame( 0, canvas.size(), 1.0 ) != newest );
    QCOMPARE( M_holder.currentIndex(), DispHolder::INVALID_INDEX );
    QVERIFY( canvas.renderFrame( 2, canvas.size(), 1.0 ) == newest );
}

/*-------------------------------------------------------------------*/
/*!
  A device pixel ratio of 2: the frame laid out in the logical size (the
  score board at the bottom), drawn with twice the pixels. Of 0.5 (an image
  smaller than the view, the texts as large as in it): the same layout,
  half as large (QPainter takes no ratio below 1: the window is mapped
  onto the pixels).
*/
void
TestFieldCanvasRender::devicePixelRatio()
{
    Options & opt = Options::instance();
    QVERIFY( opt.showScoreBoard() );

    FieldCanvas canvas( M_holder );
    canvas.resize( 640, 480 );
    const QImage one = canvas.renderFrame( 1, QSize( 640, 480 ), 1.0 );
    const QImage two = canvas.renderFrame( 1, QSize( 1280, 960 ), 2.0 );
    QCOMPARE( two.size(), QSize( 1280, 960 ) );
    QCOMPARE( two.devicePixelRatio(), 2.0 );

    // the score board at the bottom left of both
    const QColor board = opt.scoreBoardBrush().color();
    QCOMPARE( one.pixelColor( 2, 478 ), board );
    QCOMPARE( two.pixelColor( 4, 957 ), board );

    // the same layout: the left goal line twice as far
    const int y1 = 240 + 60;
    const int x1 = leftLine( one, y1 );
    const int x2 = leftLine( two, y1 * 2 );
    QVERIFY( x1 > 0 );
    QVERIFY2( std::abs( x2 - 2 * x1 ) <= 2, qPrintable( QString( "%1 %2" ).arg( x1 ).arg( x2 ) ) );

    const QImage half = canvas.renderFrame( 1, QSize( 320, 240 ), 0.5 );
    QCOMPARE( half.size(), QSize( 320, 240 ) );
    QCOMPARE( half.pixelColor( 1, 239 ), board );
    const int x_half = leftLine( half, y1 / 2 );
    QVERIFY2( std::abs( 2 * x_half - x1 ) <= 3, qPrintable( QString( "%1 %2" ).arg( x1 ).arg( x_half ) ) );
}

/*-------------------------------------------------------------------*/
//! zoomed: an image of another size shows the same range as the view (the scale by the heights)
void
TestFieldCanvasRender::zoomed()
{
    Options & opt = Options::instance();
    opt.zoomOut(); // the whole field, smaller

    FieldCanvas canvas( M_holder );
    canvas.resize( 640, 480 );
    canvas.show();
    QVERIFY( QTest::qWaitForWindowExposed( &canvas ) );
    const QImage view = drawn( canvas );
    const QImage twice = canvas.renderFrame( 1, QSize( 1280, 960 ), 1.0 );

    const int y = 240 + 60;
    const int x1 = leftLine( view, y );
    const int x2 = leftLine( twice, y * 2 );
    QVERIFY( x1 > 0 );
    QVERIFY2( std::abs( x2 - 2 * x1 ) <= 3, qPrintable( QString( "%1 %2" ).arg( x1 ).arg( x2 ) ) );
    QVERIFY( opt.zoomed() );
}

/*-------------------------------------------------------------------*/
void
TestFieldCanvasRender::emptySize()
{
    FieldCanvas canvas( M_holder );
    QVERIFY( canvas.renderFrame( 1, QSize( 0, 100 ), 1.0 ).isNull() );
    QVERIFY( canvas.renderFrame( 1, QSize(), 1.0 ).isNull() );
}

/*-------------------------------------------------------------------*/
/*!
  File > Save Image of This Frame (Ctrl+Shift+I): the frame shown, as large as the view;
  the name of the log and the cycle; PNG without an extension; no frame.
*/
void
TestFieldCanvasRender::saveImage()
{
    MainWindow window;
    window.resize( 800, 600 );
    window.show();
    QVERIFY( QTest::qWaitForWindowExposed( &window ) );

    QAction * act = window.findChild< QAction * >( "save_image" );
    QVERIFY( act );
    QCOMPARE( act->shortcut(), QKeySequence( Qt::CTRL | Qt::SHIFT | Qt::Key_I ) );
    QVERIFY( act->isEnabled() );
    bool in_file_menu = false;
    for ( QAction * menu : window.menuBar()->actions() )
    {
        if ( menu->menu() && menu->text() == "&File" )
        {
            in_file_menu = menu->menu()->actions().contains( act );
        }
    }
    QVERIFY( in_file_menu );

    QTemporaryDir dir;
    QVERIFY( dir.isValid() );
    QString error;
    QVERIFY( ! window.saveImage( dir.filePath( "none.png" ), &error ) );
    QCOMPARE( error, QString( "There is no frame to save." ) );
    QVERIFY( ! QFile::exists( dir.filePath( "none.png" ) ) );

    window.openGameLogFile( LOG );
    QCOMPARE( window.defaultImageName(), QString( "head_v5-1.png" ) );

    FieldCanvas * canvas = window.findChild< FieldCanvas * >();
    QVERIFY( canvas );
    showTexts( false );
    QVERIFY( window.saveImage( dir.filePath( "frame.png" ), &error ) );
    QVERIFY( error.isEmpty() );
    const QImage saved( dir.filePath( "frame.png" ) );
    QCOMPARE( saved.size(), canvas->size() * canvas->devicePixelRatioF() );
    QVERIFY( rgb32( saved ) == drawn( *canvas ) );

    // no extension: PNG
    QVERIFY( window.saveImage( dir.filePath( "frame" ), &error ) );
    QCOMPARE( QImageReader( dir.filePath( "frame.png" ) ).format(), QByteArray( "png" ) );
    QVERIFY( window.saveImage( dir.filePath( "frame.jpg" ), &error ) );
    QCOMPARE( QImageReader( dir.filePath( "frame.jpg" ) ).format(), QByteArray( "jpeg" ) );

    // a directory not there
    QVERIFY( ! window.saveImage( dir.filePath( "no/such/dir/frame.png" ), &error ) );
    QVERIFY2( error.contains( "Could not write the image" ), qPrintable( error ) );
}

QTEST_MAIN( TestFieldCanvasRender )
#include "test_field_canvas_render.moc"
