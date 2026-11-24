// -*-c++-*-

/*!
  \file ffmpeg_sink.cpp
  \brief the exported frames as a video, by an external ffmpeg. Source File.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ffmpeg_sink.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QSysInfo>

namespace {

//! the end of the standard error kept
const int STDERR_KEPT = 16 * 1024;

//! the end shown with an error
const int STDERR_SHOWN = 1500;

}

const QStringList FfmpegSink::PRESETS = {
    QStringLiteral( "ultrafast" ), QStringLiteral( "superfast" ), QStringLiteral( "veryfast" ),
    QStringLiteral( "faster" ), QStringLiteral( "fast" ), QStringLiteral( "medium" ),
    QStringLiteral( "slow" ), QStringLiteral( "slower" ), QStringLiteral( "veryslow" ),
};

/*-------------------------------------------------------------------*/
FfmpegSink::FfmpegSink( const QString & program,
                        const QString & file,
                        const QString & encoder,
                        const int crf,
                        const QString & preset,
                        const bool animation,
                        QObject * parent )
    : FrameSink( parent ),
      M_program( program ),
      M_file( file ),
      M_encoder( encoder ),
      M_crf( crf ),
      M_preset( preset ),
      M_animation( animation ),
      M_process( nullptr ),
      M_frame_bytes( 0 ),
      M_busy( false ),
      M_starting( false ),
      M_finishing( false ),
      M_ended( false )
{

}

/*-------------------------------------------------------------------*/
FfmpegSink::~FfmpegSink()
{
    if ( M_process
         && M_process->state() != QProcess::NotRunning )
    {
        M_process->disconnect( this );
        M_process->kill();
        M_process->waitForFinished( 2000 );
    }
}

/*-------------------------------------------------------------------*/
QString
FfmpegSink::pixelFormat()
{
    // Format_RGB32 is 0xffRRGGBB in a 32-bit word
    return QSysInfo::ByteOrder == QSysInfo::LittleEndian
        ? QStringLiteral( "bgr0" )
        : QStringLiteral( "0rgb" );
}

/*-------------------------------------------------------------------*/
QStringList
FfmpegSink::arguments( const QSize & size,
                       const double fps,
                       const QString & file,
                       const QString & encoder,
                       const int crf,
                       const QString & preset,
                       const bool animation )
{
    QStringList args = {
        QStringLiteral( "-hide_banner" ), QStringLiteral( "-loglevel" ), QStringLiteral( "error" ), QStringLiteral( "-nostats" ),
        QStringLiteral( "-f" ), QStringLiteral( "rawvideo" ),
        QStringLiteral( "-pixel_format" ), pixelFormat(),
        QStringLiteral( "-video_size" ), QString( "%1x%2" ).arg( size.width() ).arg( size.height() ),
        QStringLiteral( "-framerate" ), QString::number( fps, 'g', 10 ),
        QStringLiteral( "-i" ), QStringLiteral( "pipe:0" ),
        QStringLiteral( "-an" ),
        QStringLiteral( "-c:v" ), encoder,
    };

    if ( encoder == QLatin1String( "libx264" ) )
    {
        args << QStringLiteral( "-preset" ) << preset
             << QStringLiteral( "-crf" ) << QString::number( crf );
        if ( animation )
        {
            args << QStringLiteral( "-tune" ) << QStringLiteral( "animation" );
        }
    }
    else if ( encoder == QLatin1String( "mpeg4" ) )
    {
        args << QStringLiteral( "-q:v" ) << QStringLiteral( "2" );
    }
    else
    {
        args << QStringLiteral( "-b:v" ) << QStringLiteral( "10M" );
    }

    // the colors of the screens (bt709), 4:2:0 for the players
    args << QStringLiteral( "-vf" ) << QStringLiteral( "scale=out_color_matrix=bt709:out_range=tv,format=yuv420p" )
         << QStringLiteral( "-colorspace" ) << QStringLiteral( "bt709" )
         << QStringLiteral( "-color_primaries" ) << QStringLiteral( "bt709" )
         << QStringLiteral( "-color_trc" ) << QStringLiteral( "bt709" );

    // played while it is read (a browser)
    const QString ext = QFileInfo( file ).suffix().toLower();
    if ( ext == QLatin1String( "mp4" ) || ext == QLatin1String( "mov" ) )
    {
        args << QStringLiteral( "-movflags" ) << QStringLiteral( "+faststart" );
    }

    args << QStringLiteral( "-y" ) << file;
    return args;
}

/*-------------------------------------------------------------------*/
QString
FfmpegSink::findProgram( const QString & path )
{
    const QString p = path.trimmed();
    if ( p.isEmpty() )
    {
        return QStandardPaths::findExecutable( QStringLiteral( "ffmpeg" ) );
    }
    const QFileInfo info( p );
    if ( info.isFile() && info.isExecutable() )
    {
        return info.absoluteFilePath();
    }
    // a name in the PATH
    return p.contains( QLatin1Char( '/' ) ) ? QString() : QStandardPaths::findExecutable( p );
}

/*-------------------------------------------------------------------*/
FfmpegSink::Info
FfmpegSink::probe( const QString & path )
{
    Info info;
    info.program = findProgram( path );
    if ( info.program.isEmpty() )
    {
        info.error = ( path.trimmed().isEmpty()
                       ? tr( "ffmpeg is not found in the PATH." )
                       : tr( "%1 is not found, or cannot be run." ).arg( QDir::toNativeSeparators( path.trimmed() ) ) );
        return info;
    }

    // its output, or an empty text
    const auto run = [&info]( const QStringList & args, QString * error ) {
        QProcess process;
        process.start( info.program, args );
        if ( ! process.waitForFinished( 5000 )
             || process.exitStatus() != QProcess::NormalExit
             || process.exitCode() != 0 )
        {
            *error = tr( "Could not run %1: %2" ).arg( QDir::toNativeSeparators( info.program ), process.errorString() );
            process.kill();
            process.waitForFinished( 1000 );
            return QString();
        }
        return QString::fromLocal8Bit( process.readAllStandardOutput() );
    };

    const QString version = run( { QStringLiteral( "-hide_banner" ), QStringLiteral( "-version" ) }, &info.error );
    if ( ! info.error.isEmpty() )
    {
        return info;
    }
    info.version = version.section( QLatin1Char( '\n' ), 0, 0 ).trimmed();

    const QString encoders = run( { QStringLiteral( "-hide_banner" ), QStringLiteral( "-encoders" ) }, &info.error );
    if ( ! info.error.isEmpty() )
    {
        return info;
    }
    // " V....D libx264    libx264 H.264 / AVC ...": the second word of a line
    QStringList names;
    for ( const QString & line : encoders.split( QLatin1Char( '\n' ) ) )
    {
        const QStringList words = line.simplified().split( QLatin1Char( ' ' ) );
        if ( words.size() >= 2 && words[0].startsWith( QLatin1Char( 'V' ) ) )
        {
            names << words[1];
        }
    }
    for ( const char * name : { "libx264", "libopenh264", "mpeg4" } )
    {
        if ( names.contains( QLatin1String( name ) ) )
        {
            info.encoder = QLatin1String( name );
            break;
        }
    }
    if ( info.encoder.isEmpty() )
    {
        info.error = tr( "%1 has no encoder of H.264 or MPEG-4." ).arg( QDir::toNativeSeparators( info.program ) );
        return info;
    }

    info.found = true;
    return info;
}

/*-------------------------------------------------------------------*/
bool
FfmpegSink::open( const QSize & pixels,
                  const double fps )
{
    M_error.clear();
    M_stderr.clear();
    M_busy = false;
    M_finishing = false;
    M_ended = false;

    if ( pixels.width() % 2 != 0 || pixels.height() % 2 != 0 )
    {
        M_error = tr( "The size of a video must be even: %1 x %2." ).arg( pixels.width() ).arg( pixels.height() );
        return false;
    }
    if ( ! QDir().mkpath( QFileInfo( M_file ).absolutePath() ) )
    {
        M_error = tr( "Could not make the directory %1." ).arg( QDir::toNativeSeparators( QFileInfo( M_file ).absolutePath() ) );
        return false;
    }

    M_size = pixels;
    M_frame_bytes = static_cast< qint64 >( pixels.width() ) * pixels.height() * 4;

    M_process = new QProcess( this );
    M_process->setProcessChannelMode( QProcess::SeparateChannels );
    connect( M_process, &QProcess::readyReadStandardError,
             this, &FfmpegSink::readStandardError );
    connect( M_process, &QProcess::bytesWritten,
             this, &FfmpegSink::handleBytesWritten );
    connect( M_process, QOverload< int, QProcess::ExitStatus >::of( &QProcess::finished ),
             this, [this]( int code, QProcess::ExitStatus ) { handleFinished( code ); } );
    connect( M_process, &QProcess::errorOccurred,
             this, &FfmpegSink::handleError );

    M_starting = true;
    M_process->start( M_program, arguments( pixels, fps, M_file, M_encoder, M_crf, M_preset, M_animation ) );
    const bool started = M_process->waitForStarted( 5000 );
    M_starting = false;
    if ( ! started )
    {
        M_error = tr( "Could not start %1: %2" ).arg( QDir::toNativeSeparators( M_program ), M_process->errorString() );
        M_ended = true;
        return false;
    }
    return true;
}

/*-------------------------------------------------------------------*/
FrameSink::WriteResult
FfmpegSink::write( const QImage & image )
{
    if ( M_ended
         || ! M_process
         || M_process->state() != QProcess::Running )
    {
        if ( M_error.isEmpty() ) M_error = withDetail( tr( "ffmpeg ended before the end of the video." ) );
        return FAILED;
    }

    // not more than two frames waiting in the pipe
    if ( M_process->bytesToWrite() >= 2 * M_frame_bytes )
    {
        M_busy = true;
        return BUSY;
    }

    const QImage frame = ( image.format() == QImage::Format_RGB32
                           ? image
                           : image.convertToFormat( QImage::Format_RGB32 ) );
    if ( frame.width() != M_size.width()
         || frame.height() != M_size.height() )
    {
        M_error = tr( "An image of %1 x %2 for a video of %3 x %4." )
            .arg( frame.width() ).arg( frame.height() ).arg( M_size.width() ).arg( M_size.height() );
        return FAILED;
    }
    // the lines of RGB32 have no padding: the rows at once
    const qint64 bytes = static_cast< qint64 >( frame.bytesPerLine() ) * frame.height();
    if ( M_process->write( reinterpret_cast< const char * >( frame.constBits() ), bytes ) != bytes )
    {
        M_error = withDetail( tr( "Could not give the frame to ffmpeg: %1" ).arg( M_process->errorString() ) );
        return FAILED;
    }
    return ACCEPTED;
}

/*-------------------------------------------------------------------*/
void
FfmpegSink::finish()
{
    if ( M_ended || ! M_process )
    {
        return;
    }
    // ffmpeg ends when its input ends, after the file is written
    M_finishing = true;
    M_process->closeWriteChannel();
}

/*-------------------------------------------------------------------*/
void
FfmpegSink::abort()
{
    // stopped: no end told, and the file not written to the end removed (D13)
    M_ended = true;
    if ( M_process )
    {
        M_process->disconnect( this );
        if ( M_process->state() != QProcess::NotRunning )
        {
            M_process->kill();
            M_process->waitForFinished( 2000 );
        }
    }
    QFile::remove( M_file );
}

/*-------------------------------------------------------------------*/
void
FfmpegSink::readStandardError()
{
    if ( ! M_process ) return;
    M_stderr += M_process->readAllStandardError();
    if ( M_stderr.size() > STDERR_KEPT )
    {
        M_stderr = M_stderr.right( STDERR_KEPT );
    }
}

/*-------------------------------------------------------------------*/
void
FfmpegSink::handleBytesWritten()
{
    if ( M_busy
         && M_process
         && M_process->bytesToWrite() < 2 * M_frame_bytes )
    {
        M_busy = false;
        emit readyForFrame();
    }
}

/*-------------------------------------------------------------------*/
void
FfmpegSink::handleFinished( int exit_code )
{
    readStandardError();
    if ( M_ended )
    {
        return;
    }
    if ( M_finishing
         && M_process->exitStatus() == QProcess::NormalExit
         && exit_code == 0 )
    {
        end( true, QDir::toNativeSeparators( M_file ) );
        return;
    }

    // an error, or ended before the input: what was written is not a video
    QFile::remove( M_file );
    M_error = withDetail( M_process->exitStatus() == QProcess::NormalExit
                          ? tr( "ffmpeg ended with the code %1." ).arg( exit_code )
                          : tr( "ffmpeg crashed." ) );
    end( false, M_error );
}

/*-------------------------------------------------------------------*/
void
FfmpegSink::handleError()
{
    // after it started: a crash and a broken pipe end in handleFinished()
    if ( M_ended
         || M_starting
         || ! M_process
         || M_process->error() != QProcess::FailedToStart )
    {
        return;
    }
    M_error = tr( "Could not start %1: %2" ).arg( QDir::toNativeSeparators( M_program ), M_process->errorString() );
    end( false, M_error );
}

/*-------------------------------------------------------------------*/
QString
FfmpegSink::withDetail( const QString & reason ) const
{
    const QString detail = QString::fromLocal8Bit( M_stderr ).trimmed().right( STDERR_SHOWN );
    return detail.isEmpty()
        ? reason
        : reason + QLatin1Char( '\n' ) + detail;
}

/*-------------------------------------------------------------------*/
void
FfmpegSink::end( const bool ok,
                 const QString & message )
{
    if ( M_ended )
    {
        return;
    }
    M_ended = true;
    emit finished( ok, message );
}
