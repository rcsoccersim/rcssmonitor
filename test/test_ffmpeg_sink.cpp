// -*-c++-*-

/*!
  \file test_ffmpeg_sink.cpp
  \brief unit test of FfmpegSink: the videos written by ffmpeg
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
#include "ffmpeg_sink.h"
#include "field_canvas.h"
#include "game_log_file.h"
#include "rcg_handler.h"

#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>

/*
  docs/plan/2026-11-22-03_frame-export.md 4.5, D13 to D15, §6. The sink is
  tested with a fake ffmpeg (a shell script: it answers -version and
  -encoders, writes its arguments to OUT.args, and its input to OUT), and
  with the real one when it is installed (skipped if not; failed if not
  and RCSSMONITOR_REQUIRE_FFMPEG=1, as in the CI).
  test/data/head_v5.rcg has 3 frames.
*/

namespace {

const char * const ENCODERS_X264 =
    "Encoders:\n"
    " V..... = Video\n"
    " ------\n"
    " V....D libx264              libx264 H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10 (codec h264)\n"
    " V....D mpeg4                MPEG-4 part 2\n"
    " A....D aac                  AAC (Advanced Audio Coding)\n";

const char * const ENCODERS_MPEG4 =
    "Encoders:\n"
    " V....D mpeg4                MPEG-4 part 2\n"
    " A....D libx264              not a video encoder\n";

const char * const ENCODERS_OPENH264 =
    "Encoders:\n"
    " V....D mpeg4                MPEG-4 part 2\n"
    " V....D libopenh264          OpenH264 H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10 (codec h264)\n";

const char * const ENCODERS_NONE =
    "Encoders:\n"
    " A....D aac                  AAC (Advanced Audio Coding)\n";

//! the input written to the file
const char * const BODY_OK = "exec cat > \"$last\"\n";

/*!
  \brief a fake ffmpeg in the directory
  \param encoders the output of -encoders
  \param body what it does with the video (after OUT.args is written)
*/
QString
fakeFfmpeg( const QTemporaryDir & dir,
            const QString & name,
            const char * encoders,
            const char * body )
{
    const QString path = dir.filePath( name );
    QFile file( path );
    if ( ! file.open( QIODevice::WriteOnly | QIODevice::Text ) ) return QString();
    file.write( QByteArray( "#!/bin/sh\n"
                            "case \"$1 $2\" in\n"
                            "  \"-hide_banner -version\")\n"
                            "    echo 'ffmpeg version 9.9-fake Copyright (c) the tests'\n"
                            "    echo 'built for the tests'\n"
                            "    exit 0 ;;\n"
                            "  \"-hide_banner -encoders\")\n"
                            "    printf '%s' '" ) + encoders + "'\n"
                "    exit 0 ;;\n"
                "esac\n"
                "for a in \"$@\"; do last=\"$a\"; done\n"
                "printf '%s\\n' \"$@\" > \"$last.args\"\n"
                + body );
    file.close();
    file.setPermissions( file.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeUser );
    return path;
}

QImage
colored( const QSize & size,
         const QColor & color )
{
    QImage image( size, QImage::Format_RGB32 );
    image.fill( color );
    return image;
}

//! the arguments the fake was run with
QStringList
argsOf( const QString & video )
{
    QFile file( video + ".args" );
    if ( ! file.open( QIODevice::ReadOnly | QIODevice::Text ) ) return QStringList();
    QStringList args = QString::fromUtf8( file.readAll() ).split( '\n' );
    if ( ! args.isEmpty() && args.last().isEmpty() ) args.removeLast();
    return args;
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

//! ffprobe of a video: "key=value" lines of the stream and the format
QMap< QString, QString >
probeVideo( const QString & ffprobe,
            const QString & file )
{
    QProcess process;
    process.start( ffprobe, { "-v", "error", "-select_streams", "v:0", "-count_frames",
                              "-show_entries", "stream=codec_name,width,height,pix_fmt,nb_read_frames,color_space",
                              "-show_entries", "format=duration",
                              "-of", "default=noprint_wrappers=1", file } );
    QMap< QString, QString > values;
    if ( ! process.waitForFinished( 30000 ) ) return values;
    for ( const QString & line : QString::fromUtf8( process.readAllStandardOutput() ).split( '\n' ) )
    {
        const int eq = line.indexOf( '=' );
        if ( eq > 0 ) values[line.left( eq )] = line.mid( eq + 1 ).trimmed();
    }
    return values;
}

}

/*!
  \brief unit test of the videos written by ffmpeg
*/
class TestFfmpegSink
    : public QObject {
    Q_OBJECT

private:

    DispHolder M_holder;

private slots:

    void initTestCase()
      {
#ifdef Q_OS_WIN
          QSKIP( "the fake ffmpeg is a shell script" );
#endif
          RCGHandler handler( nullptr, M_holder );
          QVERIFY( loadGameLogFile( QStringLiteral( TEST_DATA_DIR "/head_v5.rcg" ).toStdString(), handler ) );
          QCOMPARE( M_holder.dispCont().size(), std::size_t( 3 ) );
      }

    void arguments();
    void findProgram();
    void probe();
    void write();
    void busy();
    void cannotOpen();
    void failed();
    void aborted();
    void severalVideos();
    void realFfmpeg();
};

/*-------------------------------------------------------------------*/
//! the arguments (4.5): raw frames in; H.264 at a quality and a speed, or another encoder; the colors of the screens
void
TestFfmpegSink::arguments()
{
    QCOMPARE( FfmpegSink::pixelFormat(),
              QString( QSysInfo::ByteOrder == QSysInfo::LittleEndian ? "bgr0" : "0rgb" ) );
    const QString pix = FfmpegSink::pixelFormat();

    QCOMPARE( FfmpegSink::arguments( QSize( 1920, 1080 ), 10.0, "/v/a.mp4", "libx264", 18, "medium", true ),
              QStringList( { "-hide_banner", "-loglevel", "error", "-nostats",
                             "-f", "rawvideo", "-pixel_format", pix, "-video_size", "1920x1080", "-framerate", "10",
                             "-i", "pipe:0", "-an", "-c:v", "libx264",
                             "-preset", "medium", "-crf", "18", "-tune", "animation",
                             "-vf", "scale=out_color_matrix=bt709:out_range=tv,format=yuv420p",
                             "-colorspace", "bt709", "-color_primaries", "bt709", "-color_trc", "bt709",
                             "-movflags", "+faststart", "-y", "/v/a.mp4" } ) );

    // not tuned; .mkv: no faststart; 12.5 images/s
    QStringList args = FfmpegSink::arguments( QSize( 64, 48 ), 12.5, "/v/b.MKV", "libx264", 23, "slow", false );
    QVERIFY( ! args.contains( "-tune" ) );
    QVERIFY( ! args.contains( "-movflags" ) );
    QCOMPARE( args.at( args.indexOf( "-framerate" ) + 1 ), QString( "12.5" ) );
    QCOMPARE( args.at( args.indexOf( "-crf" ) + 1 ), QString( "23" ) );
    QCOMPARE( args.at( args.indexOf( "-preset" ) + 1 ), QString( "slow" ) );

    // other encoders: no CRF nor preset of libx264
    args = FfmpegSink::arguments( QSize( 64, 48 ), 10.0, "/v/c.mov", "mpeg4", 18, "medium", true );
    QCOMPARE( args.at( args.indexOf( "-c:v" ) + 1 ), QString( "mpeg4" ) );
    QCOMPARE( args.at( args.indexOf( "-q:v" ) + 1 ), QString( "2" ) );
    QVERIFY( ! args.contains( "-crf" ) );
    QVERIFY( ! args.contains( "-preset" ) );
    QVERIFY( args.contains( "-movflags" ) );
    args = FfmpegSink::arguments( QSize( 64, 48 ), 10.0, "/v/d.mp4", "libopenh264", 18, "medium", true );
    QCOMPARE( args.at( args.indexOf( "-b:v" ) + 1 ), QString( "10M" ) );
    QVERIFY( ! args.contains( "-crf" ) );
}

/*-------------------------------------------------------------------*/
//! the path given, a name in the PATH, or none
void
TestFfmpegSink::findProgram()
{
    QTemporaryDir tmp;
    const QString fake = fakeFfmpeg( tmp, "ffmpeg-fake", ENCODERS_X264, BODY_OK );
    QCOMPARE( FfmpegSink::findProgram( fake ), QFileInfo( fake ).absoluteFilePath() );
    QCOMPARE( FfmpegSink::findProgram( "  " + fake + " " ), QFileInfo( fake ).absoluteFilePath() );

    QFile plain( tmp.filePath( "plain" ) );
    QVERIFY( plain.open( QIODevice::WriteOnly ) );
    plain.close();
    QCOMPARE( FfmpegSink::findProgram( plain.fileName() ), QString() );
    QCOMPARE( FfmpegSink::findProgram( tmp.filePath( "missing" ) ), QString() );
    QCOMPARE( FfmpegSink::findProgram( tmp.path() ), QString() );
    QCOMPARE( FfmpegSink::findProgram( "rcssmonitor-no-such-ffmpeg" ), QString() );
    QCOMPARE( FfmpegSink::findProgram( QString() ), QStandardPaths::findExecutable( "ffmpeg" ) );
}

/*-------------------------------------------------------------------*/
//! its version and its encoder: libx264, or libopenh264, or mpeg4; or why not
void
TestFfmpegSink::probe()
{
    QTemporaryDir tmp;
    FfmpegSink::Info info = FfmpegSink::probe( fakeFfmpeg( tmp, "x264", ENCODERS_X264, BODY_OK ) );
    QVERIFY2( info.found, qPrintable( info.error ) );
    QCOMPARE( info.version, QString( "ffmpeg version 9.9-fake Copyright (c) the tests" ) );
    QCOMPARE( info.encoder, QString( "libx264" ) );
    QCOMPARE( info.program, tmp.filePath( "x264" ) );
    QVERIFY( info.error.isEmpty() );

    info = FfmpegSink::probe( fakeFfmpeg( tmp, "mpeg4", ENCODERS_MPEG4, BODY_OK ) );
    QVERIFY( info.found );
    QCOMPARE( info.encoder, QString( "mpeg4" ) ); // an audio "libx264" is not taken

    info = FfmpegSink::probe( fakeFfmpeg( tmp, "openh264", ENCODERS_OPENH264, BODY_OK ) );
    QVERIFY( info.found );
    QCOMPARE( info.encoder, QString( "libopenh264" ) );

    info = FfmpegSink::probe( fakeFfmpeg( tmp, "none", ENCODERS_NONE, BODY_OK ) );
    QVERIFY( ! info.found );
    QVERIFY2( info.error.endsWith( "has no encoder of H.264 or MPEG-4." ), qPrintable( info.error ) );

    // it fails
    QFile broken( tmp.filePath( "broken" ) );
    QVERIFY( broken.open( QIODevice::WriteOnly | QIODevice::Text ) );
    broken.write( "#!/bin/sh\necho 'cannot load a library' >&2\nexit 127\n" );
    broken.close();
    broken.setPermissions( broken.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeUser );
    info = FfmpegSink::probe( broken.fileName() );
    QVERIFY( ! info.found );
    QVERIFY2( info.error.startsWith( "Could not run " ), qPrintable( info.error ) );

    // not there
    info = FfmpegSink::probe( tmp.filePath( "missing" ) );
    QVERIFY( ! info.found );
    QVERIFY2( info.error.endsWith( "is not found, or cannot be run." ), qPrintable( info.error ) );
    QVERIFY( info.program.isEmpty() );
}

/*-------------------------------------------------------------------*/
//! the frames in its input as raw RGB32; the end told when it has ended
void
TestFfmpegSink::write()
{
    QTemporaryDir tmp;
    const QString program = fakeFfmpeg( tmp, "ffmpeg", ENCODERS_X264, BODY_OK );
    const QString video = tmp.filePath( "new/dir/a.mp4" );
    const QSize size( 64, 48 );

    FfmpegSink sink( program, video, "libx264", 20, "fast", true );
    QSignalSpy finished( &sink, &FrameSink::finished );
    QSignalSpy ready( &sink, &FrameSink::readyForFrame );
    QVERIFY2( sink.open( size, 12.5 ), qPrintable( sink.errorString() ) );
    const QColor colors[] = { Qt::red, Qt::green, Qt::blue };
    for ( const QColor & c : colors )
    {
        FrameSink::WriteResult r = sink.write( colored( size, c ) );
        if ( r == FrameSink::BUSY )
        {
            QVERIFY( ready.wait( 5000 ) );
            r = sink.write( colored( size, c ) );
        }
        QCOMPARE( r, FrameSink::ACCEPTED );
    }
    sink.finish();
    QVERIFY( finished.wait( 5000 ) );
    QCOMPARE( finished.count(), 1 );
    QCOMPARE( finished.first().at( 0 ).toBool(), true );
    QCOMPARE( finished.first().at( 1 ).toString(), QDir::toNativeSeparators( video ) );

    // the arguments, and the frames: 3 x 64 x 48 x 4 bytes, the red one first
    QCOMPARE( argsOf( video ), FfmpegSink::arguments( size, 12.5, video, "libx264", 20, "fast", true ) );
    QFile file( video );
    QVERIFY( file.open( QIODevice::ReadOnly ) );
    const QByteArray bytes = file.readAll();
    QCOMPARE( bytes.size(), 3 * 64 * 48 * 4 );
    const QImage first( reinterpret_cast< const uchar * >( bytes.constData() ), 64, 48, QImage::Format_RGB32 );
    QCOMPARE( first.pixelColor( 10, 10 ), QColor( Qt::red ) );
    const QImage last( reinterpret_cast< const uchar * >( bytes.constData() ) + 2 * 64 * 48 * 4, 64, 48, QImage::Format_RGB32 );
    QCOMPARE( last.pixelColor( 10, 10 ), QColor( Qt::blue ) );
}

/*-------------------------------------------------------------------*/
//! not more than two frames waiting in the pipe: BUSY, then readyForFrame()
void
TestFfmpegSink::busy()
{
    QTemporaryDir tmp;
    const QString program = fakeFfmpeg( tmp, "ffmpeg", ENCODERS_X264, BODY_OK );
    const QString video = tmp.filePath( "busy.mp4" );
    const QSize size( 640, 480 );

    FfmpegSink sink( program, video, "libx264", 18, "medium", true );
    QSignalSpy finished( &sink, &FrameSink::finished );
    QSignalSpy ready( &sink, &FrameSink::readyForFrame );
    QVERIFY( sink.open( size, 10.0 ) );
    // no event loop yet: the frames wait in the pipe
    QCOMPARE( sink.write( colored( size, Qt::red ) ), FrameSink::ACCEPTED );
    QCOMPARE( sink.write( colored( size, Qt::red ) ), FrameSink::ACCEPTED );
    QCOMPARE( sink.write( colored( size, Qt::red ) ), FrameSink::BUSY );
    QCOMPARE( ready.count(), 0 );
    QVERIFY( ready.wait( 5000 ) );
    QCOMPARE( sink.write( colored( size, Qt::red ) ), FrameSink::ACCEPTED );
    sink.finish();
    QVERIFY( finished.wait( 5000 ) );
    QVERIFY( finished.first().at( 0 ).toBool() );
    QCOMPARE( QFileInfo( video ).size(), qint64( 3 ) * 640 * 480 * 4 );

    // an image of another size
    FfmpegSink other( program, tmp.filePath( "other.mp4" ), "libx264", 18, "medium", true );
    QVERIFY( other.open( size, 10.0 ) );
    QCOMPARE( other.write( colored( QSize( 64, 48 ), Qt::red ) ), FrameSink::FAILED );
    QVERIFY2( other.errorString().startsWith( "An image of 64 x 48 for a video of 640 x 480" ), qPrintable( other.errorString() ) );
    other.abort();
}

/*-------------------------------------------------------------------*/
//! not opened: odd sizes, no directory, no program
void
TestFfmpegSink::cannotOpen()
{
    QTemporaryDir tmp;
    const QString program = fakeFfmpeg( tmp, "ffmpeg", ENCODERS_X264, BODY_OK );

    FfmpegSink odd( program, tmp.filePath( "odd.mp4" ), "libx264", 18, "medium", true );
    QVERIFY( ! odd.open( QSize( 65, 48 ), 10.0 ) );
    QCOMPARE( odd.errorString(), QString( "The size of a video must be even: 65 x 48." ) );

    QFile file( tmp.filePath( "file" ) );
    QVERIFY( file.open( QIODevice::WriteOnly ) );
    file.close();
    FfmpegSink no_dir( program, tmp.filePath( "file/a.mp4" ), "libx264", 18, "medium", true );
    QVERIFY( ! no_dir.open( QSize( 64, 48 ), 10.0 ) );
    QVERIFY2( no_dir.errorString().startsWith( "Could not make the directory" ), qPrintable( no_dir.errorString() ) );

    // not started: told by open() only, and a file there kept
    QFile there( tmp.filePath( "there.mp4" ) );
    QVERIFY( there.open( QIODevice::WriteOnly ) );
    there.write( "old" );
    there.close();
    FfmpegSink missing( tmp.filePath( "missing" ), there.fileName(), "libx264", 18, "medium", true );
    QSignalSpy finished( &missing, &FrameSink::finished );
    QVERIFY( ! missing.open( QSize( 64, 48 ), 10.0 ) );
    QVERIFY2( missing.errorString().startsWith( "Could not start " ), qPrintable( missing.errorString() ) );
    QTest::qWait( 50 );
    QCOMPARE( finished.count(), 0 );
    QVERIFY( QFile::exists( there.fileName() ) );
}

/*-------------------------------------------------------------------*/
//! ffmpeg ended with an error, at once or later: the end of its standard error told, the file removed
void
TestFfmpegSink::failed()
{
    FieldCanvas canvas( M_holder );
    QTemporaryDir tmp;

    {
        const QString program = fakeFfmpeg( tmp, "at-once", ENCODERS_X264,
                                            "echo 'some lines first' >&2\n"
                                            "echo \"Unknown encoder 'libx264'\" >&2\n"
                                            ": > \"$last\"\n"
                                            "exit 1\n" );
        const QString video = tmp.filePath( "at-once.mp4" );
        ExportJob job( canvas, new FfmpegSink( program, video, "libx264", 18, "medium", true ),
                       { 0, 1, 2 }, QSize( 64, 48 ), 1.0, 10.0 );
        Told told( job );
        job.start();
        QTRY_COMPARE_WITH_TIMEOUT( told.finished, 1, 5000 );
        QVERIFY( ! told.ok );
        QVERIFY2( told.message.startsWith( "ffmpeg ended with the code 1.\n" ), qPrintable( told.message ) );
        QVERIFY2( told.message.endsWith( "Unknown encoder 'libx264'" ), qPrintable( told.message ) );
        QVERIFY( ! QFile::exists( video ) );
    }
    {
        // after a part of the input
        const QString program = fakeFfmpeg( tmp, "later", ENCODERS_X264,
                                            "head -c 20000 > \"$last\"\n"
                                            "echo 'Conversion failed!' >&2\n"
                                            "exit 3\n" );
        const QString video = tmp.filePath( "later.mp4" );
        ExportJob job( canvas, new FfmpegSink( program, video, "libx264", 18, "medium", true ),
                       { 0, 1, 2, 0, 1, 2 }, QSize( 64, 48 ), 1.0, 10.0 );
        Told told( job );
        job.start();
        QTRY_COMPARE_WITH_TIMEOUT( told.finished, 1, 5000 );
        QVERIFY( ! told.ok );
        QVERIFY2( told.message.startsWith( "ffmpeg ended with the code 3.\n" ), qPrintable( told.message ) );
        QVERIFY2( told.message.endsWith( "Conversion failed!" ), qPrintable( told.message ) );
        QVERIFY( ! QFile::exists( video ) );
    }
}

/*-------------------------------------------------------------------*/
//! stopped: ffmpeg killed, the file removed (D13), nothing more told
void
TestFfmpegSink::aborted()
{
    FieldCanvas canvas( M_holder );
    QTemporaryDir tmp;
    const QString program = fakeFfmpeg( tmp, "ffmpeg", ENCODERS_X264, BODY_OK );
    const QString video = tmp.filePath( "stopped.mp4" );

    FfmpegSink * sink = new FfmpegSink( program, video, "libx264", 18, "medium", true );
    QSignalSpy sink_finished( sink, &FrameSink::finished );
    ExportJob job( canvas, sink, { 0, 1, 2 }, QSize( 64, 48 ), 1.0, 10.0 );
    Told told( job );
    QObject::connect( &job, &ExportJob::progressed,
                      [&job]( std::size_t done, std::size_t ) { if ( done == 2 ) job.abort(); } );
    job.start();
    QTRY_COMPARE( told.finished, 1 );
    QVERIFY( ! told.ok );
    QCOMPARE( told.message, QString( "Stopped after 2 of 3 images." ) );
    QVERIFY( ! QFile::exists( video ) );
    QTest::qWait( 100 );
    QCOMPARE( told.finished, 1 );
    QCOMPARE( sink_finished.count(), 0 );
}

/*-------------------------------------------------------------------*/
//! a video for each part, one after another; the files of the parts told
void
TestFfmpegSink::severalVideos()
{
    FieldCanvas canvas( M_holder );
    QTemporaryDir tmp;
    const QString program = fakeFfmpeg( tmp, "ffmpeg", ENCODERS_X264, BODY_OK );
    const QString a = tmp.filePath( "v-a.mp4" );
    const QString b = tmp.filePath( "v-b.mp4" );

    ExportJob job( canvas,
                   { ExportPart{ new FfmpegSink( program, a, "libx264", 18, "medium", true ), { 0, 1 } },
                     ExportPart{ new FfmpegSink( program, b, "libx264", 18, "medium", true ), { 2 } } },
                   QSize( 64, 48 ), 1.0, 10.0 );
    Told told( job );
    QCOMPARE( job.total(), std::size_t( 3 ) );
    job.start();
    QTRY_COMPARE_WITH_TIMEOUT( told.finished, 1, 5000 );
    QVERIFY2( told.ok, qPrintable( told.message ) );
    QCOMPARE( told.message, QDir::toNativeSeparators( a ) + "\n" + QDir::toNativeSeparators( b ) );
    QCOMPARE( told.progress, QStringList( { "1/3", "2/3", "3/3" } ) );
    QCOMPARE( QFileInfo( a ).size(), qint64( 2 ) * 64 * 48 * 4 );
    QCOMPARE( QFileInfo( b ).size(), qint64( 1 ) * 64 * 48 * 4 );

    // the second stopped: the first kept (D13)
    QFile::remove( a );
    QFile::remove( b );
    ExportJob stopped( canvas,
                       { ExportPart{ new FfmpegSink( program, a, "libx264", 18, "medium", true ), { 0 } },
                         ExportPart{ new FfmpegSink( program, b, "libx264", 18, "medium", true ), { 1, 2 } } },
                       QSize( 64, 48 ), 1.0, 10.0 );
    Told stopped_told( stopped );
    QObject::connect( &stopped, &ExportJob::progressed,
                      [&stopped]( std::size_t done, std::size_t ) { if ( done == 2 ) stopped.abort(); } );
    stopped.start();
    QTRY_COMPARE_WITH_TIMEOUT( stopped_told.finished, 1, 5000 );
    QCOMPARE( stopped_told.message, QString( "Stopped after 2 of 3 images." ) );
    QCOMPARE( QFileInfo( a ).size(), qint64( 1 ) * 64 * 48 * 4 );
    QVERIFY( ! QFile::exists( b ) );
}

/*-------------------------------------------------------------------*/
/*!
  The real ffmpeg, if installed: a video of the frames that plays (ffprobe:
  the codec, the size, the frames, the length N / fps); a wrong preset told
  with its error.
*/
void
TestFfmpegSink::realFfmpeg()
{
    const FfmpegSink::Info info = FfmpegSink::probe( QString() );
    const QString ffprobe = QStandardPaths::findExecutable( "ffprobe" );
    if ( ! info.found || ffprobe.isEmpty() )
    {
        // the CI that has ffmpeg (.github/workflows/ci.yml) must run this
        if ( qEnvironmentVariableIntValue( "RCSSMONITOR_REQUIRE_FFMPEG" ) == 1 )
        {
            QFAIL( "ffmpeg or ffprobe is not installed, and RCSSMONITOR_REQUIRE_FFMPEG=1" );
        }
        QSKIP( "ffmpeg or ffprobe is not installed" );
    }

    FieldCanvas canvas( M_holder );
    QTemporaryDir tmp;
    for ( const QString name : { "real.mp4", "real.mkv" } )
    {
        const QString video = tmp.filePath( name );
        ExportJob job( canvas, new FfmpegSink( info.program, video, info.encoder, 18, "medium", true ),
                       { 0, 1, 2, 2 }, QSize( 160, 120 ), 1.0, 10.0 );
        Told told( job );
        job.start();
        QTRY_COMPARE_WITH_TIMEOUT( told.finished, 1, 30000 );
        QVERIFY2( told.ok, qPrintable( told.message ) );
        QCOMPARE( told.progress.last(), QString( "4/4" ) );

        const QMap< QString, QString > values = probeVideo( ffprobe, video );
        QCOMPARE( values.value( "codec_name" ), QString( info.encoder == "mpeg4" ? "mpeg4" : "h264" ) );
        QCOMPARE( values.value( "width" ), QString( "160" ) );
        QCOMPARE( values.value( "height" ), QString( "120" ) );
        QCOMPARE( values.value( "pix_fmt" ), QString( "yuv420p" ) );
        QCOMPARE( values.value( "nb_read_frames" ), QString( "4" ) );
        QVERIFY2( std::fabs( values.value( "duration" ).toDouble() - 0.4 ) < 0.02, qPrintable( values.value( "duration" ) ) );
    }

    if ( info.encoder == "libx264" )
    {
        const QString video = tmp.filePath( "wrong.mp4" );
        ExportJob job( canvas, new FfmpegSink( info.program, video, info.encoder, 18, "no-such-preset", true ),
                       { 0, 1, 2 }, QSize( 160, 120 ), 1.0, 10.0 );
        Told told( job );
        job.start();
        QTRY_COMPARE_WITH_TIMEOUT( told.finished, 1, 30000 );
        QVERIFY( ! told.ok );
        QVERIFY2( told.message.startsWith( "ffmpeg ended with the code " ), qPrintable( told.message ) );
        QVERIFY2( told.message.contains( '\n' ), qPrintable( told.message ) ); // with its standard error
        QVERIFY( ! QFile::exists( video ) );
    }
}

QTEST_MAIN( TestFfmpegSink )
#include "test_ffmpeg_sink.moc"
