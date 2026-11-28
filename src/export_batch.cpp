// -*-c++-*-

/*!
  \file export_batch.cpp
  \brief the export from the command line, without a window. Source File.
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

#include "export_batch.h"

#include "export_job.h"
#include "export_range_list.h"
#include "export_timing.h"
#include "ffmpeg_sink.h"
#include "field_canvas.h"
#include "game_log_file.h"
#include "image_sequence_sink.h"
#include "rcg_handler.h"

#ifdef HAVE_3D_VIEW
#include "cameras_3d.h"
#include "offscreen_view_3d.h"
#endif

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>
#include <memory>
#include <set>

namespace {

//! the extensions of the videos: their formats (D14)
const char * const VIDEO_EXTENSIONS[] = { "mp4", "mov", "mkv" };

//! the options of the export (addOptions()), to tell those given without an export
const char * const OPTION_NAMES[] = {
    "export-view", "export-range", "export-join", "export-fps", "export-speed", "export-size",
    "export-scale", "export-image-format", "export-image-prefix", "export-crf", "export-preset", "ffmpeg",
};

//! the names of the ranges in the names of files: safe, and made unique (name, name-2, ...)
QStringList
fileNames( const std::vector< ExportBatch::NamedRange > & ranges )
{
    QStringList names;
    std::set< QString > used;
    for ( const ExportBatch::NamedRange & r : ranges )
    {
        const QString base = ExportRangeList::safeName( r.name );
        QString name = base;
        for ( int n = 2; used.count( name ) > 0; ++n )
        {
            name = base + QString( "-%1" ).arg( n );
        }
        used.insert( name );
        names << name;
    }
    return names;
}

//! a number of an option in its range
bool
readNumber( const QCommandLineParser & parser,
            const char * name,
            const double min,
            const double max,
            double * value,
            QString * error )
{
    if ( ! parser.isSet( QLatin1String( name ) ) )
    {
        return true;
    }
    bool ok = false;
    const double v = parser.value( QLatin1String( name ) ).toDouble( &ok );
    if ( ! ok || v < min || max < v )
    {
        *error = QObject::tr( "--%1 %2: a number from %3 to %4 is expected." )
            .arg( QLatin1String( name ), parser.value( QLatin1String( name ) ) ).arg( min ).arg( max );
        return false;
    }
    *value = v;
    return true;
}

}

/*-------------------------------------------------------------------*/
void
ExportBatch::addOptions( QCommandLineParser & parser )
{
    parser.addOption( QCommandLineOption( "export-video",
                                          "Export the game log as a video FILE (.mp4, .mov or .mkv) by ffmpeg, without a window, and quit.",
                                          "FILE" ) );
    parser.addOption( QCommandLineOption( "export-images",
                                          "Export the game log as images into DIR, without a window, and quit.",
                                          "DIR" ) );
    parser.addOption( QCommandLineOption( "export-view",
                                          "The view that draws the exported images: 2d, or 3d (it needs OpenGL 3.3 and an X display, e.g. under xvfb-run). (Default=2d)",
                                          "2d|3d" ) );
    parser.addOption( QCommandLineOption( "export-range",
                                          "A range to export: START-END (cycles, or 1234.3 for a stopped frame; an end may be empty), all, first-half,"
                                          " second-half, half-N, extra-N, penalties, game, or goals[:BEFORE:AFTER] (a range for each goal, from BEFORE"
                                          " seconds before it to AFTER seconds after it, the seconds of the playback, the stopped frames too;"
                                          " 10 and 5 by default). Given again for more ranges. (Default=all)",
                                          "RANGE" ) );
    parser.addOption( QCommandLineOption( "export-join",
                                          "Export the ranges in one video (or one sequence of images), not each in its own (FILE-NAME.ext, or DIR/NAME)." ) );
    parser.addOption( QCommandLineOption( "export-fps",
                                          "The images a second. (Default=10 for 2d, 30 for 3d)",
                                          "double" ) );
    parser.addOption( QCommandLineOption( "export-speed",
                                          "How many times as fast as the game is played. (Default=1)",
                                          "double" ) );
    parser.addOption( QCommandLineOption( "export-size",
                                          "The size of the images in pixels (even for a video). (Default=1280x720)",
                                          "WxH" ) );
    parser.addOption( QCommandLineOption( "export-scale",
                                          "The device pixel ratio R: the images look as a view of their size / R, enlarged R times (the texts, the logos and the lines)."
                                          " (Default=the height / 720)",
                                          "R" ) );
    parser.addOption( QCommandLineOption( "export-image-format",
                                          "The format of the images: png or jpg. (Default=png)",
                                          "png|jpg" ) );
    parser.addOption( QCommandLineOption( "export-image-prefix",
                                          "The start of the names of the images. (Default=image-)",
                                          "str" ) );
    parser.addOption( QCommandLineOption( "export-crf",
                                          "The quality of the video (CRF): 0 to 51, smaller is better and larger. (Default=18)",
                                          "int" ) );
    parser.addOption( QCommandLineOption( "export-preset",
                                          "The encoding speed of the video: " + FfmpegSink::PRESETS.join( ", " ) + ". (Default=medium)",
                                          "str" ) );
    parser.addOption( QCommandLineOption( "ffmpeg",
                                          "The path of ffmpeg for --export-video. (Default=ffmpeg in the PATH)",
                                          "path" ) );
}

/*-------------------------------------------------------------------*/
bool
ExportBatch::readOptions( const QCommandLineParser & parser,
                          Options::BatchExportSettings * settings,
                          QString * error )
{
    Options::BatchExportSettings & s = *settings;
    if ( parser.isSet( QStringLiteral( "export-video" ) ) ) s.video = parser.value( QStringLiteral( "export-video" ) );
    if ( parser.isSet( QStringLiteral( "export-images" ) ) ) s.images = parser.value( QStringLiteral( "export-images" ) );
    if ( ! s.video.isEmpty() && ! s.images.isEmpty() )
    {
        *error = QObject::tr( "Give --export-video or --export-images, not both." );
        return false;
    }
    if ( ! s.enabled() )
    {
        for ( const char * name : OPTION_NAMES )
        {
            if ( parser.isSet( QLatin1String( name ) ) )
            {
                *error = QObject::tr( "--%1 is an option of the export: give --export-video FILE or --export-images DIR too." ).arg( QLatin1String( name ) );
                return false;
            }
        }
        return true;
    }

    if ( parser.isSet( QStringLiteral( "export-view" ) ) )
    {
        const QString view = parser.value( QStringLiteral( "export-view" ) ).trimmed().toLower();
        if ( view != QLatin1String( "2d" ) && view != QLatin1String( "3d" ) )
        {
            *error = QObject::tr( "--export-view %1: 2d or 3d is expected." ).arg( view );
            return false;
        }
        s.view_3d = ( view == QLatin1String( "3d" ) );
#ifndef HAVE_3D_VIEW
        if ( s.view_3d )
        {
            *error = QObject::tr( "--export-view 3d: this rcssmonitor is built without the 3D view." );
            return false;
        }
#endif
    }
    if ( parser.isSet( QStringLiteral( "export-range" ) ) ) s.ranges = parser.values( QStringLiteral( "export-range" ) );
    if ( parser.isSet( QStringLiteral( "export-join" ) ) ) s.join = true;
    if ( ! readNumber( parser, "export-fps", 0.1, 240.0, &s.fps, error )
         || ! readNumber( parser, "export-speed", 0.01, 64.0, &s.speed, error )
         || ! readNumber( parser, "export-scale", 0.1, 16.0, &s.scale, error ) )
    {
        return false;
    }
    if ( parser.isSet( QStringLiteral( "export-size" ) ) )
    {
        static const QRegularExpression size( QStringLiteral( "^\\s*(\\d+)\\s*[xX]\\s*(\\d+)\\s*$" ) );
        const QRegularExpressionMatch m = size.match( parser.value( QStringLiteral( "export-size" ) ) );
        const int w = ( m.hasMatch() ? m.captured( 1 ).toInt() : 0 );
        const int h = ( m.hasMatch() ? m.captured( 2 ).toInt() : 0 );
        if ( w < 16 || 7680 < w || h < 16 || 4320 < h )
        {
            *error = QObject::tr( "--export-size %1: WxH from 16x16 to 7680x4320 is expected." ).arg( parser.value( QStringLiteral( "export-size" ) ) );
            return false;
        }
        s.width = w;
        s.height = h;
    }
    if ( parser.isSet( QStringLiteral( "export-image-format" ) ) )
    {
        const QString format = parser.value( QStringLiteral( "export-image-format" ) ).trimmed().toLower();
        if ( format != QLatin1String( "png" ) && format != QLatin1String( "jpg" ) && format != QLatin1String( "jpeg" ) )
        {
            *error = QObject::tr( "--export-image-format %1: png or jpg is expected." ).arg( format );
            return false;
        }
        s.image_format = ( format == QLatin1String( "png" ) ? QStringLiteral( "png" ) : QStringLiteral( "jpg" ) );
    }
    if ( parser.isSet( QStringLiteral( "export-image-prefix" ) ) ) s.image_prefix = parser.value( QStringLiteral( "export-image-prefix" ) );
    if ( parser.isSet( QStringLiteral( "export-crf" ) ) )
    {
        bool ok = false;
        const int crf = parser.value( QStringLiteral( "export-crf" ) ).toInt( &ok );
        if ( ! ok || crf < 0 || 51 < crf )
        {
            *error = QObject::tr( "--export-crf %1: 0 to 51 is expected." ).arg( parser.value( QStringLiteral( "export-crf" ) ) );
            return false;
        }
        s.crf = crf;
    }
    if ( parser.isSet( QStringLiteral( "export-preset" ) ) )
    {
        const QString preset = parser.value( QStringLiteral( "export-preset" ) ).trimmed();
        if ( ! FfmpegSink::PRESETS.contains( preset ) )
        {
            *error = QObject::tr( "--export-preset %1: one of %2 is expected." ).arg( preset, FfmpegSink::PRESETS.join( ", " ) );
            return false;
        }
        s.preset = preset;
    }
    if ( parser.isSet( QStringLiteral( "ffmpeg" ) ) ) s.ffmpeg = parser.value( QStringLiteral( "ffmpeg" ) );
    return true;
}

/*-------------------------------------------------------------------*/
bool
ExportBatch::resolveRanges( const QStringList & texts,
                            const DispHolder & holder,
                            std::vector< NamedRange > * ranges,
                            QString * error )
{
    ranges->clear();
    const DispCont & frames = holder.dispCont();
    if ( frames.empty() )
    {
        *error = QObject::tr( "The game log has no frame." );
        return false;
    }
    const int step = std::max( 1, holder.serverParam().simulator_step_ );

    const QStringList list = ( texts.isEmpty() ? QStringList( QStringLiteral( "all" ) ) : texts );
    bool goals_asked = false;
    for ( const QString & given : list )
    {
        const QString text = given.trimmed();
        if ( text == QLatin1String( "all" ) )
        {
            NamedRange r;
            r.range.first = 0;
            r.range.last = frames.size() - 1;
            r.name = text;
            ranges->push_back( r );
            continue;
        }

        // goals[:BEFORE:AFTER]: a range for each goal (D8: 10 s before to 5 s
        // after), the seconds of the playback around the frame of the goal
        // (docs/plan/2026-11-28-00_goal-scene-length.md D1)
        if ( text == QLatin1String( "goals" )
             || text.startsWith( QLatin1String( "goals:" ) ) )
        {
            double before = 10.0;
            double after = 5.0;
            if ( text != QLatin1String( "goals" ) )
            {
                const QStringList parts = text.split( QLatin1Char( ':' ) );
                bool ok_before = false, ok_after = false;
                if ( parts.size() == 3 )
                {
                    before = parts[1].toDouble( &ok_before );
                    after = parts[2].toDouble( &ok_after );
                }
                if ( ! ok_before || ! ok_after || before < 0.0 || after < 0.0 )
                {
                    *error = QObject::tr( "The range %1: goals or goals:BEFORE:AFTER (seconds) is expected." ).arg( text );
                    return false;
                }
            }
            goals_asked = true;
            const std::vector< ExportGoal > goals = ExportRange::goals( frames, holder.scoreChangedIndex() );
            for ( std::size_t i = 0; i < goals.size(); ++i )
            {
                NamedRange r;
                r.range = ExportRange::around( frames, goals[i].frame, before, after, step );
                r.name = QString( "goal%1-c%2" ).arg( i + 1 ).arg( goals[i].cycle );
                if ( r.range.isValid() ) ranges->push_back( r );
            }
            continue;
        }

        // a part of the game
        bool found = false;
        for ( const ExportPeriod & p : ExportRange::periods( frames, holder.serverParam() ) )
        {
            if ( p.key == text )
            {
                NamedRange r;
                r.range.first = p.first;
                r.range.last = p.last;
                r.name = p.key;
                ranges->push_back( r );
                found = true;
                break;
            }
        }
        if ( found )
        {
            continue;
        }
        static const QRegularExpression period( QStringLiteral( "^(first-half|second-half|half-\\d+|extra-\\d+|penalties|game)$" ) );
        if ( period.match( text ).hasMatch() )
        {
            *error = QObject::tr( "The range %1: the game log has no such part." ).arg( text );
            return false;
        }

        // START-END
        const int dash = text.indexOf( QLatin1Char( '-' ) );
        if ( dash < 0 )
        {
            *error = QObject::tr( "The range %1: START-END, all, first-half, second-half, half-N, extra-N, penalties, game or goals[:BEFORE:AFTER] is expected." ).arg( text );
            return false;
        }
        ExportRangeSpec spec;
        spec.start = text.left( dash ).trimmed();
        spec.end = text.mid( dash + 1 ).trimmed();
        spec.name = text;
        QString why;
        NamedRange r;
        r.range = ExportRangeList::resolve( spec, frames, &why );
        if ( ! r.range.isValid() )
        {
            *error = QObject::tr( "The range %1: %2" ).arg( text, why );
            return false;
        }
        r.name = text;
        ranges->push_back( r );
    }

    if ( ranges->empty() )
    {
        *error = ( goals_asked
                   ? QObject::tr( "There is no goal in the game log." )
                   : QObject::tr( "There is no frame to export." ) );
        return false;
    }
    return true;
}

/*-------------------------------------------------------------------*/
ExportBatch::ExportBatch( const Options::BatchExportSettings & settings,
                          std::ostream & log )
    : M_settings( settings ),
      M_log( log )
{

}

/*-------------------------------------------------------------------*/
int
ExportBatch::fail( const QString & message )
{
    M_log << "rcssmonitor: " << message.toStdString() << std::endl;
    return 1;
}

/*-------------------------------------------------------------------*/
QStringList
ExportBatch::videoFiles( const std::vector< NamedRange > & ranges ) const
{
    QString file = QDir::cleanPath( M_settings.video );
    if ( QFileInfo( file ).suffix().isEmpty() )
    {
        file += QStringLiteral( ".mp4" );
    }
    if ( ranges.size() <= 1
         || M_settings.join )
    {
        return QStringList( file );
    }

    // FILE-NAME.ext: a video for each range, by its name (as the export dialog)
    const QFileInfo info( file );
    const QString base = QDir::cleanPath( QDir( info.path() ).filePath( info.completeBaseName() ) );
    QStringList files;
    for ( const QString & name : fileNames( ranges ) )
    {
        files << base + QLatin1Char( '-' ) + name + QLatin1Char( '.' ) + info.suffix();
    }
    return files;
}

/*-------------------------------------------------------------------*/
int
ExportBatch::run( const QString & game_log )
{
    const Options::BatchExportSettings & s = M_settings;
    if ( ! s.enabled() )
    {
        return fail( QObject::tr( "Nothing to export: give --export-video FILE or --export-images DIR." ) );
    }
    if ( game_log.isEmpty() )
    {
        return fail( QObject::tr( "No game log: rcssmonitor --export-video FILE [OPTIONS] GAME_LOG" ) );
    }

    // the game log, without a window (no message box)
    M_disp_holder.clear();
    {
        RCGHandler handler( nullptr, M_disp_holder );
        if ( ! loadGameLogFile( QFile::encodeName( game_log ).toStdString(), handler ) )
        {
            return fail( QObject::tr( "Could not read the game log %1." ).arg( game_log ) );
        }
    }
    M_disp_holder.setIndexFirst();

    // the ranges, and the frames (2D) or the positions (3D) of their images
    std::vector< NamedRange > ranges;
    QString error;
    if ( ! resolveRanges( s.ranges, M_disp_holder, &ranges, &error ) )
    {
        return fail( error );
    }
    const int step = std::max( 1, M_disp_holder.serverParam().simulator_step_ );
    const double fps = ( s.fps > 0.0
                         ? s.fps
                         : s.view_3d ? ExportTiming::DEFAULT_FPS_3D : ExportTiming::DEFAULT_FPS_2D );
    const double per_image = ExportTiming::framesPerImage( s.speed, step, fps );
    std::vector< std::vector< std::size_t > > frames;
    std::vector< std::vector< double > > positions;
    std::size_t total = 0;
    std::size_t frame_count = 0;
    for ( const NamedRange & r : ranges )
    {
        frames.push_back( ExportTiming::frames2D( r.range.first, r.range.last, per_image ) );
        positions.push_back( s.view_3d
                             ? ExportTiming::positions3D( r.range.first, r.range.last, per_image )
                             : std::vector< double >() );
        total += frames.back().size();
        frame_count += r.range.size();
    }
    if ( total == 0 )
    {
        return fail( QObject::tr( "There is no frame to export." ) );
    }

    // the size: even for a video (yuv420p); the layout as a view of the height / the scale
    const bool video = ! s.video.isEmpty();
    QSize pixels( s.width, s.height );
    if ( video )
    {
        pixels = QSize( std::max( 2, pixels.width() & ~1 ), std::max( 2, pixels.height() & ~1 ) );
    }
    const qreal dpr = ( s.scale > 0.0
                        ? s.scale
                        : std::max( 0.1, pixels.height() / LAYOUT_HEIGHT ) );

    // what draws: the 3D view first (nothing written without it)
    std::unique_ptr< FieldCanvas > canvas;
#ifdef HAVE_3D_VIEW
    std::unique_ptr< OffscreenView3D > view_3d;
#endif
    if ( s.view_3d )
    {
#ifdef HAVE_3D_VIEW
        if ( pixels.width() > OffscreenView3D::MAX_WIDTH
             || pixels.height() > OffscreenView3D::MAX_HEIGHT )
        {
            return fail( QObject::tr( "A 3D image is %1 x %2 pixels at most." ).arg( OffscreenView3D::MAX_WIDTH ).arg( OffscreenView3D::MAX_HEIGHT ) );
        }
        view_3d.reset( new OffscreenView3D( M_disp_holder ) );
        if ( ! view_3d->initialize( &error ) )
        {
            return fail( error + QObject::tr( " The 3D view needs OpenGL 3.3 and an X display: on a machine without one, run it under xvfb-run." ) );
        }
        // the camera of the options (--camera-3d), jumping at the first image of each range
        CameraController camera;
        camera.setMode( Options::instance().camera3D() );
        view_3d->start( camera, fps );
#else
        return fail( QObject::tr( "This rcssmonitor is built without the 3D view." ) );
#endif
    }
    else
    {
        canvas.reset( new FieldCanvas( M_disp_holder ) );
    }

    // where they go
    const bool separate = ( ranges.size() > 1 && ! s.join );
    std::vector< ExportPart > parts;
    const auto all_in_one = [&]( FrameSink * sink ) {
        ExportPart part{ sink, {} };
        for ( std::size_t r = 0; r < ranges.size(); ++r )
        {
            part.range_starts.push_back( part.frames.size() );
            part.frames.insert( part.frames.end(), frames[r].begin(), frames[r].end() );
            part.positions.insert( part.positions.end(), positions[r].begin(), positions[r].end() );
        }
        return part;
    };
    QString target;
    if ( video )
    {
        const QString suffix = QFileInfo( videoFiles( ranges ).front() ).suffix().toLower();
        if ( std::find_if( std::begin( VIDEO_EXTENSIONS ), std::end( VIDEO_EXTENSIONS ),
                           [&]( const char * e ) { return suffix == QLatin1String( e ); } ) == std::end( VIDEO_EXTENSIONS ) )
        {
            return fail( QObject::tr( "A video is a .mp4, .mov or .mkv file: not .%1." ).arg( suffix ) );
        }
        const FfmpegSink::Info info = FfmpegSink::probe( s.ffmpeg );
        if ( ! info.found )
        {
            return fail( info.error + QObject::tr( " A video needs ffmpeg: install it, or give --ffmpeg PATH." ) );
        }
        // the version without its copyright, as the export dialog shows it
        QString version = info.version;
        const int copyright = version.indexOf( QLatin1String( " Copyright" ) );
        if ( copyright > 0 ) version.truncate( copyright );
        M_log << version.toStdString() << "; H.264 by " << info.encoder.toStdString() << std::endl;
        // a video of the 2D view: flat colors (-tune animation); of the 3D view: as a film
        const QStringList files = videoFiles( ranges );
        const auto sink = [&]( const QString & file ) {
            return new FfmpegSink( info.program, file, info.encoder, s.crf, s.preset, ! s.view_3d );
        };
        if ( separate )
        {
            for ( std::size_t r = 0; r < ranges.size(); ++r )
            {
                parts.push_back( ExportPart{ sink( files[static_cast< int >( r )] ), frames[r], positions[r] } );
            }
            target = QObject::tr( "%1 videos (%2, ...)" ).arg( files.size() ).arg( files.front() );
        }
        else
        {
            parts.push_back( all_in_one( sink( files.front() ) ) );
            target = files.front();
        }
    }
    else
    {
        // each range in its own directory by its name, or the numbers going on over them
        QStringList names;
        if ( separate )
        {
            const QStringList dirs = fileNames( ranges );
            for ( std::size_t r = 0; r < ranges.size(); ++r )
            {
                const int digits = ExportTiming::nameDigits( frames[r].size() );
                for ( std::size_t n = 0; n < frames[r].size(); ++n )
                {
                    names << dirs[static_cast< int >( r )] + QLatin1Char( '/' )
                        + ExportTiming::imageName( s.image_prefix, n, digits, s.image_format );
                }
            }
        }
        else
        {
            const int digits = ExportTiming::nameDigits( total );
            for ( std::size_t n = 0; n < total; ++n )
            {
                names << ExportTiming::imageName( s.image_prefix, n, digits, s.image_format );
            }
        }
        parts.push_back( all_in_one( new ImageSequenceSink( s.images, names, s.image_format.toLatin1() ) ) );
        target = s.images;
    }

    M_log << "Exporting " << total << " images of " << frame_count << " frames"
          << ( ranges.size() > 1 ? QString( " in %1 ranges" ).arg( ranges.size() ).toStdString() : std::string() )
          << ", " << pixels.width() << " x " << pixels.height() << " pixels, " << fps << " images/s, by the "
          << ( s.view_3d ? "3D" : "2D" ) << " view, to " << target.toStdString() << std::endl;

    std::unique_ptr< ExportJob > job;
#ifdef HAVE_3D_VIEW
    if ( view_3d )
    {
        job.reset( new ExportJob( *view_3d, parts, pixels, dpr, fps ) );
    }
#endif
    if ( ! job )
    {
        job.reset( new ExportJob( *canvas, parts, pixels, dpr, fps ) );
    }

    // the progress at most every second, and every tenth
    QEventLoop loop;
    bool ok = false;
    QString message;
    QElapsedTimer clock;
    clock.start();
    qint64 told_at = 0;
    std::size_t told_tenth = 0;
    QObject::connect( job.get(), &ExportJob::progressed,
                      [&]( std::size_t done, std::size_t all ) {
                          const std::size_t tenth = ( all > 0 ? done * 10 / all : 10 );
                          if ( done < all
                               && tenth == told_tenth
                               && clock.elapsed() - told_at < 1000 )
                          {
                              return;
                          }
                          told_tenth = tenth;
                          told_at = clock.elapsed();
                          M_log << ( done < all
                                     ? QString( "Exported %1 of %2 images (%3%)" ).arg( done ).arg( all ).arg( all > 0 ? done * 100 / all : 100 )
                                     : video ? QString( "Exported %1 images; finishing the video" ).arg( all )
                                     : QString( "Exported %1 images" ).arg( all ) ).toStdString() << std::endl;
                      } );
    QObject::connect( job.get(), &ExportJob::finished,
                      [&]( bool o, const QString & m ) {
                          ok = o;
                          message = m;
                          loop.quit();
                      } );
    job->start();
    loop.exec();

    if ( ! ok )
    {
        return fail( message );
    }
    for ( const QString & line : message.split( QLatin1Char( '\n' ), Qt::SkipEmptyParts ) )
    {
        M_log << "Exported " << line.toStdString() << std::endl;
    }
    M_log << "Finished in " << clock.elapsed() / 1000.0 << " s" << std::endl;
    return 0;
}
