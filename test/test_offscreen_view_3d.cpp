// -*-c++-*-

/*!
  \file test_offscreen_view_3d.cpp
  \brief unit test of OffscreenView3D: the 3D view drawn into images
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

#include "offscreen_view_3d.h"

#include "cameras_3d.h"
#include "export_job.h"
#include "export_timing.h"

#include "disp_holder.h"
#include "image_sequence_sink.h"
#include "options.h"

#include <QTemporaryDir>

#include <algorithm>
#include <cstdlib>

namespace {

constexpr int FRAMES = 20;
const QSize VIEW( 320, 180 );
const double DT = 1.0 / 30.0;

//! a frame: the 22 players in two lines, the ball moving along x 1 m a frame
rcss::rcg::ShowInfoT
show( const int frame )
{
    rcss::rcg::ShowInfoT s;
    s.time_ = frame;
    s.ball_.x_ = 1.0f * float( frame );
    s.ball_.y_ = 0.0f;
    for ( int i = 0; i < rcss::rcg::MAX_PLAYER * 2; ++i )
    {
        rcss::rcg::PlayerT & p = s.player_[i];
        const bool left = ( i < rcss::rcg::MAX_PLAYER );
        p.side_ = ( left ? 'l' : 'r' );
        p.unum_ = static_cast< rcss::rcg::Int16 >( i % rcss::rcg::MAX_PLAYER + 1 );
        p.state_ = rcss::rcg::STAND;
        p.type_ = 0;
        p.x_ = ( left ? -1.0f : 1.0f ) * ( 5.0f + 4.0f * float( i % rcss::rcg::MAX_PLAYER ) );
        p.y_ = ( left ? -10.0f : 10.0f );
        p.body_ = ( left ? 0.0f : 180.0f );
        p.stamina_ = 8000.0f;
        p.effort_ = 1.0f;
        p.recovery_ = 1.0f;
    }
    return s;
}

void
fill( DispHolder & holder )
{
    holder.handlePlayMode( 0, rcss::rcg::PM_PlayOn );
    for ( int f = 0; f < FRAMES; ++f ) holder.handleShow( show( f ) );
    holder.setIndex( 2 );
}

bool
near( const QVector3D & a,
      const QVector3D & b )
{
    return ( a - b ).length() < 1.0e-3f;
}

/*!
  \brief whether two images are the same picture: their colors differ by
  2 at most (the GPU's rounding, e.g. of the shadows after other frames, or
  of two contexts of a software renderer)
  \param why set to how they differ (the pixels, the largest difference
  and where), and told when they are not the same to the pixel
*/
bool
samePicture( const QImage & a,
             const QImage & b,
             QString * why )
{
    if ( a.size() != b.size() )
    {
        *why = QString( "the sizes %1x%2 and %3x%4" ).arg( a.width() ).arg( a.height() ).arg( b.width() ).arg( b.height() );
        return false;
    }
    int count = 0;
    int largest = 0;
    QRect where;
    for ( int y = 0; y < a.height(); ++y )
    {
        for ( int x = 0; x < a.width(); ++x )
        {
            const QRgb p = a.pixel( x, y ), q = b.pixel( x, y );
            const int d = std::max( { std::abs( qRed( p ) - qRed( q ) ),
                                      std::abs( qGreen( p ) - qGreen( q ) ),
                                      std::abs( qBlue( p ) - qBlue( q ) ) } );
            if ( d == 0 ) continue;
            ++count;
            largest = std::max( largest, d );
            where |= QRect( x, y, 1, 1 );
        }
    }
    *why = QString( "%1 pixels differ, by %2 at most, in (%3, %4) %5x%6" )
        .arg( count ).arg( largest )
        .arg( where.x() ).arg( where.y() ).arg( where.width() ).arg( where.height() );
    if ( count > 0 )
    {
        qInfo( "%s", qPrintable( *why ) );
    }
    return largest <= 2;
}

}

/*!
  \brief OffscreenView3D draws the 3D view into images, as the export and
  Save Image of This Frame do (docs/plan/2026-11-22-03_frame-export.md 4.2,
  4.15)
*/
class TestOffscreenView3D
    : public QObject {
    Q_OBJECT

private slots:

    void initTestCase()
      {
          const QString reason = OffscreenView3D::unavailableReason();
          if ( ! reason.isEmpty() )
          {
              QSKIP( qPrintable( reason ) );
          }
      }

    //! nothing drawn before initialize(), nor without frames
    void notReady()
      {
          DispHolder holder;
          fill( holder );
          OffscreenView3D view( holder );
          QVERIFY( ! view.ready() );
          QVERIFY( view.renderAt( 1.0, VIEW, 1.0, DT ).isNull() );

          DispHolder empty;
          OffscreenView3D none( empty );
          QString error;
          QVERIFY( none.initialize( &error ) );
          QVERIFY( error.isEmpty() );
          QVERIFY( none.ready() );
          QVERIFY( none.renderAt( 0.0, VIEW, 1.0, DT ).isNull() );
          QVERIFY( none.state().drawn == false );
      }

    /*
      A frame between two: the image of the size, the HUD laid out in the
      logical size (the score board at the bottom, with a device pixel
      ratio of 1, 2 or 0.5), and the frame shown and the state of the view
      set back.
    */
    void render()
      {
          DispHolder holder;
          fill( holder );
          OffscreenView3D view( holder );
          QVERIFY( view.initialize() );
          view.start( CameraController(), 30.0 );
          view.startRange();

          Options & opt = Options::instance();
          QVERIFY( opt.showScoreBoard() );
          const Options::ViewState state = opt.viewState();

          const QImage image = view.renderAt( 4.5, VIEW, 1.0, DT );
          QCOMPARE( image.size(), VIEW );
          QCOMPARE( image.format(), QImage::Format_RGB32 );
          const View3DState drawn = view.state();
          QVERIFY( drawn.drawn );
          QCOMPARE( drawn.at.index, std::size_t( 4 ) );
          QCOMPARE( drawn.at.next, std::size_t( 5 ) );
          QCOMPARE( drawn.at.alpha, 0.5 );
          QCOMPARE( drawn.players, 22 );

          QCOMPARE( holder.currentIndex(), std::size_t( 2 ) );
          QCOMPARE( opt.viewState().canvas_width, state.canvas_width );
          QCOMPARE( opt.viewState().canvas_height, state.canvas_height );
          QCOMPARE( opt.viewState().selected_number, state.selected_number );

          const QColor board = opt.scoreBoardBrush().color();
          QCOMPARE( image.pixelColor( 2, VIEW.height() - 2 ), board );
          const QImage twice = view.renderAt( 4.5, VIEW * 2, 2.0, 0.0 );
          QCOMPARE( twice.size(), VIEW * 2 );
          QCOMPARE( twice.pixelColor( 4, VIEW.height() * 2 - 3 ), board );
          const QImage half = view.renderAt( 4.5, VIEW / 2, 0.5, 0.0 );
          QCOMPARE( half.size(), VIEW / 2 );
          QCOMPARE( half.pixelColor( 1, VIEW.height() / 2 - 1 ), board );
          // the score board as high as the view's: a quarter of the twice's in pixels
          int top_twice = VIEW.height() * 2 - 1;
          while ( top_twice > 0 && twice.pixelColor( 4, top_twice - 1 ) == board ) --top_twice;
          int top_half = VIEW.height() / 2 - 1;
          while ( top_half > 0 && half.pixelColor( 1, top_half - 1 ) == board ) --top_half;
          QVERIFY2( std::abs( ( VIEW.height() * 2 - top_twice ) - 4 * ( VIEW.height() / 2 - top_half ) ) <= 4,
                    qPrintable( QString( "%1 %2" ).arg( top_twice ).arg( top_half ) ) );
      }

    /*
      The same input, the same images: two views from the same camera, the
      same positions and the same starts of the ranges (the same picture:
      on a software renderer, two contexts may round a few colors by 1 or 2).
    */
    void sameInput()
      {
          DispHolder holder;
          fill( holder );
          OffscreenView3D a( holder );
          OffscreenView3D b( holder );
          QVERIFY( a.initialize() );
          QVERIFY( b.initialize() );
          a.start( CameraController(), 30.0 );
          b.start( CameraController(), 30.0 );
          const std::vector< double > positions = ExportTiming::positions3D( 2, 6, 1.0 / 3.0 );
          for ( std::size_t i = 0; i < positions.size(); ++i )
          {
              if ( i == 0 || i == 6 )
              {
                  a.startRange();
                  b.startRange();
              }
              const QImage ia = a.renderAt( positions[i], VIEW, 1.0, DT );
              const QImage ib = b.renderAt( positions[i], VIEW, 1.0, DT );
              QVERIFY( ! ia.isNull() );
              QString why;
              QVERIFY2( samePicture( ia, ib, &why ), qPrintable( QString( "image %1: %2" ).arg( i ).arg( why ) ) );
          }
      }

    /*
      The camera glides after the ball from image to image, and jumps at
      the start of a range (CameraController::snap()): as a camera that
      starts there.
    */
    void rangeStart()
      {
          DispHolder holder;
          fill( holder );
          OffscreenView3D view( holder );
          QVERIFY( view.initialize() );
          view.start( CameraController(), 30.0 );
          view.startRange();
          view.renderAt( 0.0, VIEW, 1.0, DT );
          QVERIFY( view.camera().settled() );

          // far away: gliding
          view.renderAt( 15.0, VIEW, 1.0, DT );
          QVERIFY( ! view.camera().settled() );

          // a new range: there at once, as a view that starts there
          view.startRange();
          const QImage jumped = view.renderAt( 15.0, VIEW, 1.0, DT );
          QVERIFY( view.camera().settled() );
          OffscreenView3D fresh( holder );
          QVERIFY( fresh.initialize() );
          fresh.start( CameraController(), 30.0 );
          fresh.startRange();
          const QImage there = fresh.renderAt( 15.0, VIEW, 1.0, DT );
          QCOMPARE( view.state().camera_pose.target, fresh.state().camera_pose.target );
          QCOMPARE( view.state().camera_pose.position, fresh.state().camera_pose.position );
          // the same picture: a few colors may differ by 1 or 2 (the GPU's
          // rounding of the shadows, after other frames)
          QString why;
          QVERIFY2( samePicture( jumped, there, &why ), qPrintable( why ) );
      }

    /*
      Save Image of This Frame: the frame with a camera as it is (the
      view's), not moved; the export's camera not changed.
    */
    void still()
      {
          DispHolder holder;
          fill( holder );
          OffscreenView3D view( holder );
          QVERIFY( view.initialize() );

          CameraController camera;
          camera.setMode( CameraMode::Free );
          camera.viewFrom( Viewpoint::LeftEnd );
          const CameraPose pose = camera.update( 0.0, float( VIEW.width() ) / VIEW.height(), 0.0f, 0.0f );
          const QImage image = view.renderStill( 3, camera, VIEW, 1.0 );
          QCOMPARE( image.size(), VIEW );
          QCOMPARE( view.state().at.index, std::size_t( 3 ) );
          QCOMPARE( view.state().at.alpha, 0.0 );
          QVERIFY( near( view.state().camera_pose.position, pose.position ) );
          QVERIFY( near( view.state().camera_pose.target, pose.target ) );
          QVERIFY( view.renderStill( 3, camera, VIEW, 1.0 ) == image );
          QCOMPARE( view.camera().mode(), CameraController().mode() );
      }

    /*
      An export by ExportJob: an image for each position, written by the sink.
    */
    void job()
      {
          DispHolder holder;
          fill( holder );
          OffscreenView3D view( holder );
          QVERIFY( view.initialize() );
          view.start( CameraController(), 30.0 );

          QTemporaryDir tmp;
          ExportPart part{ new ImageSequenceSink( tmp.path(), { "0.png", "1.png", "2.png", "3.png" }, "png" ),
                           { 2, 2, 2, 3 } };
          part.positions = { 2.0, 2.0 + 1.0 / 3.0, 2.0 + 2.0 / 3.0, 3.0 };
          bool ok = false;
          QString message;
          int finished = 0;
          ExportJob job( view, { part }, VIEW, 1.0, 30.0 );
          QObject::connect( &job, &ExportJob::finished,
                            [&]( bool o, const QString & m ) { ++finished; ok = o; message = m; } );
          job.start();
          QTRY_COMPARE( finished, 1 );
          QVERIFY2( ok, qPrintable( message ) );
          QCOMPARE( view.state().at.index, std::size_t( 3 ) );
          const QImage first( tmp.filePath( "0.png" ) );
          const QImage second( tmp.filePath( "1.png" ) );
          QCOMPARE( first.size(), VIEW );
          QVERIFY( first != second );
          QCOMPARE( holder.currentIndex(), std::size_t( 2 ) );
      }
};

QTEST_MAIN( TestOffscreenView3D )
#include "test_offscreen_view_3d.moc"
