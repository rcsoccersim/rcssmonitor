// -*-c++-*-

/*!
  \file test_image_sequence_sink.cpp
  \brief unit test of ImageSequenceSink and ExportJob: the images exported
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
#include "export_job.h"
#include "field_canvas.h"
#include "game_log_file.h"
#include "image_sequence_sink.h"
#include "options.h"
#include "rcg_handler.h"

#include <QImageReader>
#include <QTemporaryDir>

/*
  docs/plan/2026-11-22-03_frame-export.md 4.5, §6. test/data/head_v5.rcg
  has 3 frames (the cycles 1, 2 and 4).
*/

namespace {

QImage
colored( const QColor & color )
{
    QImage image( 32, 24, QImage::Format_RGB32 );
    image.fill( color );
    return image;
}

//! what a job told
struct Told {
    QStringList progress;
    int finished = 0;
    bool ok = false;
    QString message;

    explicit Told( ExportJob & job )
      {
          QObject::connect( &job, &ExportJob::progressed,
                            [this]( std::size_t done, std::size_t total ) {
                                progress << QString( "%1/%2" ).arg( done ).arg( total );
                            } );
          QObject::connect( &job, &ExportJob::finished,
                            [this]( bool o, const QString & m ) { ++finished; ok = o; message = m; } );
      }
};

/*!
  \brief draws an image of a red by the position (3D) or the frame (2D),
  and tells the starts of the ranges and the positions drawn
*/
class FakeRenderer
    : public ExportRenderer {
public:
    QStringList calls;
    int fail_at = -1; //!< the image of a part not drawn (a null image)

    void startRange() override
      {
          calls << QStringLiteral( "start" );
      }

    QImage render( const ExportPart & part,
                   const std::size_t i,
                   const QSize & pixels,
                   const qreal ) override
      {
          const double position = ( i < part.positions.size() ? part.positions[i] : double( part.frames[i] ) );
          calls << QString::number( position );
          if ( static_cast< int >( i ) == fail_at )
          {
              return QImage();
          }
          QImage image( pixels, QImage::Format_RGB32 );
          image.fill( QColor( int( position * 20 ) % 256, 0, 0 ) );
          return image;
      }
};

}

/*!
  \brief unit test of the images exported
*/
class TestImageSequenceSink
    : public QObject {
    Q_OBJECT

private:

    DispHolder M_holder;

private slots:

    void initTestCase()
      {
          RCGHandler handler( nullptr, M_holder );
          QVERIFY( loadGameLogFile( QStringLiteral( TEST_DATA_DIR "/head_v5.rcg" ).toStdString(), handler ) );
          QCOMPARE( M_holder.dispCont().size(), std::size_t( 3 ) );
          M_holder.setIndex( 1 );
      }

    void formats();
    void writeImages();
    void notWritable();
    void tooMany();
    void job();
    void jobAborted();
    void jobFailed();
    void jobNothing();
    void jobRenderer();
};

/*-------------------------------------------------------------------*/
void
TestImageSequenceSink::formats()
{
    const QList< QByteArray > f = ImageSequenceSink::formats();
    QVERIFY( ! f.isEmpty() );
    QCOMPARE( f.first(), QByteArray( "png" ) );
}

/*-------------------------------------------------------------------*/
//! the images in the files of the names given; the directory made
void
TestImageSequenceSink::writeImages()
{
    QTemporaryDir tmp;
    QVERIFY( tmp.isValid() );
    const QString dir = tmp.filePath( "new/dir" );

    ImageSequenceSink sink( dir, { "a.png", "b.png" }, "png" );
    QSignalSpy finished( &sink, &FrameSink::finished );
    QVERIFY( sink.open( QSize( 32, 24 ), 10.0 ) );
    QCOMPARE( sink.write( colored( Qt::red ) ), FrameSink::ACCEPTED );
    QCOMPARE( sink.write( colored( Qt::blue ) ), FrameSink::ACCEPTED );
    QCOMPARE( sink.written(), 2 );
    sink.finish();
    QCOMPARE( finished.count(), 1 );
    QCOMPARE( finished.first().at( 0 ).toBool(), true );
    QVERIFY2( finished.first().at( 1 ).toString().startsWith( "2 images in " ), qPrintable( finished.first().at( 1 ).toString() ) );

    QCOMPARE( QImage( dir + "/a.png" ).pixelColor( 0, 0 ), QColor( Qt::red ) );
    QCOMPARE( QImage( dir + "/b.png" ).pixelColor( 0, 0 ), QColor( Qt::blue ) );
    QCOMPARE( QImageReader( dir + "/b.png" ).format(), QByteArray( "png" ) );

    // JPEG, one image
    ImageSequenceSink jpeg( dir, { "c.jpg" }, "jpg" );
    QSignalSpy jpeg_finished( &jpeg, &FrameSink::finished );
    QVERIFY( jpeg.open( QSize( 32, 24 ), 10.0 ) );
    QCOMPARE( jpeg.write( colored( Qt::green ) ), FrameSink::ACCEPTED );
    jpeg.finish();
    QVERIFY( jpeg_finished.first().at( 1 ).toString().startsWith( "1 image in " ) );
    QCOMPARE( QImageReader( dir + "/c.jpg" ).format(), QByteArray( "jpeg" ) );
}

/*-------------------------------------------------------------------*/
void
TestImageSequenceSink::notWritable()
{
    QTemporaryDir tmp;
    QFile file( tmp.filePath( "file" ) );
    QVERIFY( file.open( QIODevice::WriteOnly ) );
    file.close();

    // a directory where a file is
    ImageSequenceSink sink( tmp.filePath( "file/dir" ), { "a.png" }, "png" );
    QVERIFY( ! sink.open( QSize( 32, 24 ), 10.0 ) );
    QVERIFY2( sink.errorString().contains( "Could not make the directory" ), qPrintable( sink.errorString() ) );
}

/*-------------------------------------------------------------------*/
void
TestImageSequenceSink::tooMany()
{
    QTemporaryDir tmp;
    ImageSequenceSink sink( tmp.path(), { "a.png" }, "png" );
    QVERIFY( sink.open( QSize( 32, 24 ), 10.0 ) );
    QCOMPARE( sink.write( colored( Qt::red ) ), FrameSink::ACCEPTED );
    QCOMPARE( sink.write( colored( Qt::red ) ), FrameSink::FAILED );
    QVERIFY( ! sink.errorString().isEmpty() );
}

/*-------------------------------------------------------------------*/
/*!
  A job: the frames drawn by the 2D view into the files, an image at a time
  after start() returns; the view and the frame shown unchanged.
*/
void
TestImageSequenceSink::job()
{
    FieldCanvas canvas( M_holder );
    canvas.resize( 320, 240 );
    const Options::ViewState state = Options::instance().viewState();

    QTemporaryDir tmp;
    const std::vector< std::size_t > frames = { 0, 1, 2, 2 };
    ExportJob job( canvas,
                   new ImageSequenceSink( tmp.path(), { "0.png", "1.png", "2.png", "3.png" }, "png" ),
                   frames, QSize( 640, 480 ), 2.0, 10.0 );
    Told told( job );
    QCOMPARE( job.total(), std::size_t( 4 ) );
    job.start();
    QVERIFY( job.isRunning() );
    QCOMPARE( told.progress.size(), 0 ); // not before start() returns
    QTRY_COMPARE( told.finished, 1 );
    QVERIFY2( told.ok, qPrintable( told.message ) );
    QCOMPARE( told.progress, QStringList( { "1/4", "2/4", "3/4", "4/4" } ) );
    QVERIFY( ! job.isRunning() );
    QCOMPARE( job.done(), std::size_t( 4 ) );

    for ( int i = 0; i < 4; ++i )
    {
        const QImage image( tmp.filePath( QString( "%1.png" ).arg( i ) ) );
        QCOMPARE( image.size(), QSize( 640, 480 ) );
        const QImage expected = canvas.renderFrame( frames[i], QSize( 640, 480 ), 2.0 );
        QVERIFY( image.convertToFormat( QImage::Format_RGB32 ) == expected );
    }
    QCOMPARE( M_holder.currentIndex(), std::size_t( 1 ) );
    QCOMPARE( Options::instance().viewState().canvas_width, state.canvas_width );
    QCOMPARE( Options::instance().viewState().field_scale, state.field_scale );
}

/*-------------------------------------------------------------------*/
//! aborted: told at once, the images written so far kept
void
TestImageSequenceSink::jobAborted()
{
    FieldCanvas canvas( M_holder );
    QTemporaryDir tmp;
    ExportJob job( canvas,
                   new ImageSequenceSink( tmp.path(), { "0.png", "1.png", "2.png" }, "png" ),
                   { 0, 1, 2 }, QSize( 64, 48 ), 1.0, 10.0 );
    Told told( job );
    QObject::connect( &job, &ExportJob::progressed,
                      [&job]( std::size_t done, std::size_t ) { if ( done == 1 ) job.abort(); } );
    job.start();
    QTRY_COMPARE( told.finished, 1 );
    QVERIFY( ! told.ok );
    QCOMPARE( told.message, QString( "Stopped after 1 of 3 images." ) );
    QTest::qWait( 50 );
    QCOMPARE( told.finished, 1 );
    QCOMPARE( told.progress, QStringList( { "1/3" } ) );
    QVERIFY( QFile::exists( tmp.filePath( "0.png" ) ) );
    QVERIFY( ! QFile::exists( tmp.filePath( "1.png" ) ) );

    // abort() before the end is told once; after the end, nothing
    job.abort();
    QCOMPARE( told.finished, 1 );
}

/*-------------------------------------------------------------------*/
//! an error of the sink: told with its reason
void
TestImageSequenceSink::jobFailed()
{
    FieldCanvas canvas( M_holder );
    QTemporaryDir tmp;
    QFile file( tmp.filePath( "file" ) );
    QVERIFY( file.open( QIODevice::WriteOnly ) );
    file.close();

    {
        ExportJob job( canvas, new ImageSequenceSink( tmp.filePath( "file/dir" ), { "0.png" }, "png" ),
                       { 0 }, QSize( 64, 48 ), 1.0, 10.0 );
        Told told( job );
        job.start();
        QTRY_COMPARE( told.finished, 1 );
        QVERIFY( ! told.ok );
        QVERIFY2( told.message.contains( "Could not make the directory" ), qPrintable( told.message ) );
    }
    {
        // more images than names
        ExportJob job( canvas, new ImageSequenceSink( tmp.path(), { "0.png" }, "png" ),
                       { 0, 1 }, QSize( 64, 48 ), 1.0, 10.0 );
        Told told( job );
        job.start();
        QTRY_COMPARE( told.finished, 1 );
        QVERIFY( ! told.ok );
        QCOMPARE( told.progress, QStringList( { "1/2" } ) );
    }
}

/*-------------------------------------------------------------------*/
void
TestImageSequenceSink::jobNothing()
{
    FieldCanvas canvas( M_holder );
    QTemporaryDir tmp;
    ExportJob job( canvas, new ImageSequenceSink( tmp.path(), {}, "png" ),
                   {}, QSize( 64, 48 ), 1.0, 10.0 );
    Told told( job );
    job.start();
    QTRY_COMPARE( told.finished, 1 );
    QVERIFY( ! told.ok );
    QCOMPARE( told.message, QString( "There is no frame to export." ) );
}

/*-------------------------------------------------------------------*/
/*!
  A job drawn by another renderer (the 3D view drawn offscreen): the
  positions of the images, and the starts of the ranges told before their
  first images (the first image of each part too); an image not drawn
  stops the job.
*/
void
TestImageSequenceSink::jobRenderer()
{
    QTemporaryDir tmp;
    {
        FakeRenderer renderer;
        ExportPart part{ new ImageSequenceSink( tmp.filePath( "one" ), { "0.png", "1.png", "2.png", "3.png", "4.png" }, "png" ),
                         { 0, 0, 1, 7, 7 } };
        part.positions = { 0.0, 0.5, 1.0, 7.0, 7.5 };
        part.range_starts = { 0, 3 };
        QVERIFY( part.startsRange( 0 ) );
        QVERIFY( ! part.startsRange( 1 ) );
        QVERIFY( part.startsRange( 3 ) );
        ExportJob job( renderer, { part }, QSize( 32, 24 ), 1.0, 30.0 );
        Told told( job );
        QCOMPARE( job.total(), std::size_t( 5 ) );
        job.start();
        QTRY_COMPARE( told.finished, 1 );
        QVERIFY2( told.ok, qPrintable( told.message ) );
        QCOMPARE( renderer.calls.join( ' ' ), QString( "start 0 0.5 1 start 7 7.5" ) );
        QCOMPARE( QImage( tmp.filePath( "one/4.png" ) ).size(), QSize( 32, 24 ) );
    }
    {
        // a part for each range: each starts one
        FakeRenderer renderer;
        ExportPart a{ new ImageSequenceSink( tmp.filePath( "a" ), { "0.png", "1.png" }, "png" ), { 0, 1 } };
        ExportPart b{ new ImageSequenceSink( tmp.filePath( "b" ), { "0.png" }, "png" ), { 5 } };
        ExportJob job( renderer, { a, b }, QSize( 32, 24 ), 1.0, 10.0 );
        Told told( job );
        job.start();
        QTRY_COMPARE( told.finished, 1 );
        QVERIFY2( told.ok, qPrintable( told.message ) );
        QCOMPARE( renderer.calls.join( ' ' ), QString( "start 0 1 start 5" ) );
    }
    {
        // not drawn: stopped, told
        FakeRenderer renderer;
        renderer.fail_at = 1;
        ExportJob job( renderer, { ExportPart{ new ImageSequenceSink( tmp.filePath( "c" ), { "0.png", "1.png", "2.png" }, "png" ), { 0, 1, 2 } } },
                       QSize( 32, 24 ), 1.0, 10.0 );
        Told told( job );
        job.start();
        QTRY_COMPARE( told.finished, 1 );
        QVERIFY( ! told.ok );
        QCOMPARE( told.message, QString( "Could not draw the image 2." ) );
        QCOMPARE( told.progress, QStringList( { "1/3" } ) );
        QVERIFY( ! job.isRunning() );
    }
}

QTEST_MAIN( TestImageSequenceSink )
#include "test_image_sequence_sink.moc"
