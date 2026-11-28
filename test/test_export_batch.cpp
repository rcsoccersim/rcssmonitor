// -*-c++-*-

/*!
  \file test_export_batch.cpp
  \brief unit test of ExportBatch: the export from the command line
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

#include <QtTest>

#include "export_batch.h"

#include "disp_holder.h"
#include "options.h"

#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>

#include <sstream>
#include <string>

namespace {

const QString LOG = QStringLiteral( TEST_DATA_DIR "/head_v5.rcg" );

//! the options read from a command line (the program's name first)
bool
readArgs( const QStringList & args,
          Options::BatchExportSettings * settings,
          QString * error )
{
    QCommandLineParser parser;
    parser.addPositionalArgument( "FILE", "the game log" );
    ExportBatch::addOptions( parser );
    if ( ! parser.parse( QStringList( "rcssmonitor" ) + args ) )
    {
        *error = parser.errorText();
        return false;
    }
    return ExportBatch::readOptions( parser, settings, error );
}

/*!
  a game in live frames: the cycles 0 to 6000, a goal of the left at 100
  and of the right at 3100 (as test_export_dialog)
*/
void
addGame( DispHolder & holder )
{
    const std::vector< std::pair< int, const char * > > frames = {
        { 0, "before_kick_off" }, { 50, "play_on" }, { 100, "goal_l" }, { 101, "kick_off_r" }, { 140, "play_on" },
        { 160, "play_on" }, { 2990, "play_on" }, { 3000, "play_on" }, { 3001, "kick_off_r" }, { 3100, "goal_r" },
        { 3101, "kick_off_l" }, { 3200, "play_on" }, { 6000, "time_over" } };
    for ( const auto & f : frames )
    {
        const int left = ( f.first >= 100 ? 1 : 0 );
        const int right = ( f.first >= 3100 ? 1 : 0 );
        const std::string msg = "{\"show\":{\"time\":" + std::to_string( f.first )
            + ",\"mode\":\"" + f.second + "\""
            + ",\"team\":{\"l\":{\"name\":\"LEFT\",\"score\":" + std::to_string( left ) + "},"
            + "\"r\":{\"name\":\"RIGHT\",\"score\":" + std::to_string( right ) + "}}"
            + ",\"ball\":{\"x\":0,\"y\":0,\"vx\":0,\"vy\":0},\"players\":[]}}";
        QVERIFY( holder.addJSON( msg.c_str() ) );
    }
}

/*!
  a game of a frame a cycle, 0 to 300: a goal of the left at 200, and the
  frames stopped after it (200.1 to 200.50) before the kick-off at 201
  (as test_export_dialog; docs/plan/2026-11-28-00_goal-scene-length.md)
*/
void
addGoalGame( DispHolder & holder )
{
    for ( int c = 0; c <= 300; ++c )
    {
        for ( int s = 0; s <= ( c == 200 ? 50 : 0 ); ++s )
        {
            const char * mode = ( c == 0 ? "before_kick_off" : c == 200 ? "goal_l" : c == 201 ? "kick_off_r" : "play_on" );
            const std::string msg = "{\"show\":{\"time\":" + std::to_string( c ) + ",\"stime\":" + std::to_string( s )
                + ",\"mode\":\"" + mode + "\""
                + ",\"team\":{\"l\":{\"name\":\"LEFT\",\"score\":" + std::to_string( c >= 200 ? 1 : 0 ) + "},"
                + "\"r\":{\"name\":\"RIGHT\",\"score\":0}}"
                + ",\"ball\":{\"x\":0,\"y\":0,\"vx\":0,\"vy\":0},\"players\":[]}}";
            QVERIFY( holder.addJSON( msg.c_str() ) );
        }
    }
}

//! the ranges as "NAME:FIRST-LAST" (frame indices), a space between
QString
textOf( const std::vector< ExportBatch::NamedRange > & ranges )
{
    QStringList list;
    for ( const ExportBatch::NamedRange & r : ranges )
    {
        list << QString( "%1:%2-%3" ).arg( r.name ).arg( r.range.first ).arg( r.range.last );
    }
    return list.join( ' ' );
}

//! a fake ffmpeg: its version and encoders, its arguments into FILE.args, the frames into FILE
QString
fakeFfmpeg( const QTemporaryDir & dir )
{
    const QString path = dir.filePath( "ffmpeg" );
    QFile file( path );
    if ( ! file.open( QIODevice::WriteOnly | QIODevice::Text ) ) return QString();
    file.write( "#!/bin/sh\n"
                "case \"$1 $2\" in\n"
                "  \"-hide_banner -version\") echo 'ffmpeg version 9.9-fake Copyright (c) the tests'; exit 0 ;;\n"
                "  \"-hide_banner -encoders\") printf '%s\\n' 'Encoders:' ' V....D libx264    the encoder libx264'; exit 0 ;;\n"
                "esac\n"
                "for a in \"$@\"; do last=\"$a\"; done\n"
                "printf '%s\\n' \"$@\" > \"$last.args\"\n"
                "exec cat > \"$last\"\n" );
    file.close();
    file.setPermissions( file.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeUser );
    return path;
}

}

/*!
  \brief the export from the command line (docs/plan/2026-11-22-03_frame-export.md
  4.7, 4.16)
*/
class TestExportBatch
    : public QObject {
    Q_OBJECT

private slots:

    //! the options: the defaults, all of them, and the wrong ones
    void options()
      {
          Options::BatchExportSettings s;
          QString error;
          QVERIFY2( readArgs( { "game.rcg" }, &s, &error ), qPrintable( error ) );
          QVERIFY( ! s.enabled() );

          s = Options::BatchExportSettings();
          QVERIFY2( readArgs( { "--export-video", "out.mp4", "game.rcg" }, &s, &error ), qPrintable( error ) );
          QVERIFY( s.enabled() );
          QCOMPARE( s.video, QString( "out.mp4" ) );
          QVERIFY( ! s.view_3d );
          QVERIFY( s.ranges.isEmpty() );
          QVERIFY( ! s.join );
          QCOMPARE( s.fps, 0.0 );
          QCOMPARE( s.speed, 1.0 );
          QCOMPARE( s.width, 1280 );
          QCOMPARE( s.height, 720 );
          QCOMPARE( s.scale, 0.0 );
          QCOMPARE( s.crf, 18 );
          QCOMPARE( s.preset, QString( "medium" ) );
          QCOMPARE( s.image_format, QString( "png" ) );
          QCOMPARE( s.image_prefix, QString( "image-" ) );

          s = Options::BatchExportSettings();
          QVERIFY2( readArgs( { "--export-images", "dir", "--export-view", "3D", "--export-range", "goals:5:3",
                                "--export-range", "100-200", "--export-join", "--export-fps", "60", "--export-speed", "0.5",
                                "--export-size", "1920x1080", "--export-scale", "2", "--export-image-format", "JPEG",
                                "--export-image-prefix", "f-", "--export-crf", "23", "--export-preset", "slow",
                                "--ffmpeg", "/opt/ffmpeg", "game.rcg" }, &s, &error ),
                    qPrintable( error ) );
          QCOMPARE( s.images, QString( "dir" ) );
#ifdef HAVE_3D_VIEW
          QVERIFY( s.view_3d );
#endif
          QCOMPARE( s.ranges, QStringList( { "goals:5:3", "100-200" } ) );
          QVERIFY( s.join );
          QCOMPARE( s.fps, 60.0 );
          QCOMPARE( s.speed, 0.5 );
          QCOMPARE( s.width, 1920 );
          QCOMPARE( s.height, 1080 );
          QCOMPARE( s.scale, 2.0 );
          QCOMPARE( s.image_format, QString( "jpg" ) );
          QCOMPARE( s.image_prefix, QString( "f-" ) );
          QCOMPARE( s.crf, 23 );
          QCOMPARE( s.preset, QString( "slow" ) );
          QCOMPARE( s.ffmpeg, QString( "/opt/ffmpeg" ) );

          const QList< QPair< QStringList, QString > > wrong = {
              { { "--export-video", "a.mp4", "--export-images", "d" }, "not both" },
              { { "--export-fps", "30" }, "--export-fps is an option of the export" },
              { { "--export-images", "d", "--export-view", "4d" }, "2d or 3d" },
              { { "--export-images", "d", "--export-fps", "0" }, "--export-fps 0" },
              { { "--export-images", "d", "--export-size", "1920" }, "WxH" },
              { { "--export-images", "d", "--export-size", "8x8" }, "WxH" },
              { { "--export-images", "d", "--export-image-format", "gif" }, "png or jpg" },
              { { "--export-video", "a.mp4", "--export-crf", "52" }, "0 to 51" },
              { { "--export-video", "a.mp4", "--export-preset", "fastest" }, "fastest" },
          };
          for ( const auto & w : wrong )
          {
              s = Options::BatchExportSettings();
              error.clear();
              QVERIFY2( ! readArgs( w.first, &s, &error ), qPrintable( w.first.join( ' ' ) ) );
              QVERIFY2( error.contains( w.second ), qPrintable( error ) );
          }
      }

    //! the ranges: positions, all, the parts of the game, the goals; the wrong ones
    void ranges()
      {
          DispHolder holder;
          addGame( holder );
          const std::size_t last = holder.dispCont().size() - 1;
          std::vector< ExportBatch::NamedRange > ranges;
          QString error;

          QVERIFY2( ExportBatch::resolveRanges( {}, holder, &ranges, &error ), qPrintable( error ) );
          QCOMPARE( textOf( ranges ), QString( "all:0-%1" ).arg( last ) );

          QVERIFY2( ExportBatch::resolveRanges( { "50-140", " -100 ", "3001-", "first-half" }, holder, &ranges, &error ),
                    qPrintable( error ) );
          QCOMPARE( ranges.size(), std::size_t( 4 ) );
          QCOMPARE( textOf( { ranges[0], ranges[1], ranges[2] } ),
                    QString( "50-140:1-4 -100:0-2 3001-:8-%1" ).arg( last ) );
          QCOMPARE( ranges[3].name, QString( "first-half" ) );
          QCOMPARE( ranges[3].range.first, std::size_t( 0 ) );

          // a range for each goal, named as the export dialog's
          QVERIFY2( ExportBatch::resolveRanges( { "goals" }, holder, &ranges, &error ), qPrintable( error ) );
          QCOMPARE( ranges.size(), std::size_t( 2 ) );
          QCOMPARE( ranges[0].name, QString( "goal1-c100" ) );
          QCOMPARE( ranges[1].name, QString( "goal2-c3100" ) );
          // the seconds as frames (a frame a step): the frames of this live
          // game are far apart (docs/plan/2026-11-28-00_goal-scene-length.md D1)
          QCOMPARE( textOf( ranges ), QString( "goal1-c100:0-%1 goal2-c3100:0-%1" ).arg( last ) );
          QVERIFY2( ExportBatch::resolveRanges( { "goals:0:0" }, holder, &ranges, &error ), qPrintable( error ) );
          QCOMPARE( textOf( ranges ), QString( "goal1-c100:2-2 goal2-c3100:9-9" ) );
          QVERIFY2( ExportBatch::resolveRanges( { "goals:0.2:0.1" }, holder, &ranges, &error ), qPrintable( error ) );
          QCOMPARE( textOf( ranges ), QString( "goal1-c100:0-3 goal2-c3100:7-10" ) );

          // the frames stopped after a goal are counted: the goal scene by
          // default is 151 frames, to the last of them (200.50, D2)
          DispHolder stopped;
          addGoalGame( stopped );
          QCOMPARE( stopped.dispCont().size(), std::size_t( 351 ) );
          QVERIFY2( ExportBatch::resolveRanges( { "goals" }, stopped, &ranges, &error ), qPrintable( error ) );
          QCOMPARE( textOf( ranges ), QString( "goal1-c200:100-250" ) );
          QCOMPARE( ranges[0].range.size(), std::size_t( 151 ) );
          QCOMPARE( ExportRange::positionText( stopped.dispCont(), ranges[0].range.last ), QString( "200.50" ) );
          QVERIFY2( ExportBatch::resolveRanges( { "goals:2:1" }, stopped, &ranges, &error ), qPrintable( error ) );
          QCOMPARE( textOf( ranges ), QString( "goal1-c200:180-210" ) );

          const QList< QPair< QString, QString > > wrong = {
              { "100", "START-END" },
              { "goals:5", "goals:BEFORE:AFTER" },
              { "goals:-1:5", "goals:BEFORE:AFTER" },
              { "extra-1", "no such part" },
              { "9000-9100", "after the last frame" },
              { "half", "START-END" },
          };
          for ( const auto & w : wrong )
          {
              error.clear();
              QVERIFY2( ! ExportBatch::resolveRanges( { w.first }, holder, &ranges, &error ), qPrintable( w.first ) );
              QVERIFY2( error.contains( w.second ), qPrintable( error ) );
          }

          // no goal, no frame
          DispHolder none;
          QVERIFY( ! ExportBatch::resolveRanges( {}, none, &ranges, &error ) );
          QCOMPARE( error, QString( "The game log has no frame." ) );
          DispHolder scoreless;
          QVERIFY( scoreless.addJSON( "{\"show\":{\"time\":1,\"ball\":{\"x\":0,\"y\":0,\"vx\":0,\"vy\":0},\"players\":[]}}" ) );
          QVERIFY( ! ExportBatch::resolveRanges( { "goals" }, scoreless, &ranges, &error ) );
          QCOMPARE( error, QString( "There is no goal in the game log." ) );
      }

    //! images of the game log: the files, the progress and the end told, the exit code 0
    void images()
      {
          QTemporaryDir tmp;
          Options::BatchExportSettings s;
          s.images = tmp.filePath( "out" );
          s.width = 320;
          s.height = 240;
          std::ostringstream log;
          ExportBatch batch( s, log );
          QCOMPARE( batch.run( LOG ), 0 );
          const QString told = QString::fromStdString( log.str() );
          QVERIFY2( told.contains( "Exporting 3 images of 3 frames, 320 x 240 pixels, 10 images/s, by the 2D view" ), qPrintable( told ) );
          QVERIFY2( told.contains( "Exported 3 images in " ), qPrintable( told ) );
          QVERIFY2( told.contains( "Finished in " ), qPrintable( told ) );
          QDir dir( s.images );
          QCOMPARE( dir.entryList( QDir::Files ), QStringList( { "image-00000.png", "image-00001.png", "image-00002.png" } ) );
          QCOMPARE( QImage( dir.filePath( "image-00000.png" ) ).size(), QSize( 320, 240 ) );
          QCOMPARE( batch.dispHolder().dispCont().size(), std::size_t( 3 ) );

          // two ranges, each in its own directory; joined, the numbers going on
          s.images = tmp.filePath( "two" );
          s.ranges = QStringList( { "-1", "1-" } );
          s.image_prefix = "f-";
          std::ostringstream log2;
          QCOMPARE( ExportBatch( s, log2 ).run( LOG ), 0 );
          QCOMPARE( QDir( s.images ).entryList( QDir::Dirs | QDir::NoDotAndDotDot ), QStringList( { "-1", "1-" } ) );
          // the cycles 1, 2 and 4: -1 is the first frame, 1- all three
          QCOMPARE( QDir( s.images + "/-1" ).entryList( QDir::Files ).size(), 1 );
          QCOMPARE( QDir( s.images + "/1-" ).entryList( QDir::Files ).size(), 3 );
          s.images = tmp.filePath( "joined" );
          s.join = true;
          std::ostringstream log3;
          QCOMPARE( ExportBatch( s, log3 ).run( LOG ), 0 );
          QCOMPARE( QDir( s.images ).entryList( QDir::Files ).size(), 4 );
          QVERIFY( QFile::exists( s.images + "/f-00003.png" ) );
      }

    //! a video by ffmpeg: one file, the arguments of the 2D view
    void video()
      {
#ifdef Q_OS_WIN
          QSKIP( "the fake ffmpeg is a shell script" );
#endif
          QTemporaryDir tmp;
          Options::BatchExportSettings s;
          s.video = tmp.filePath( "game" );
          s.width = 161;
          s.height = 121;
          s.ffmpeg = fakeFfmpeg( tmp );
          s.crf = 21;
          std::ostringstream log;
          QCOMPARE( ExportBatch( s, log ).run( LOG ), 0 );
          const QString told = QString::fromStdString( log.str() );
          QVERIFY2( told.contains( "ffmpeg version 9.9-fake; H.264 by libx264" ), qPrintable( told ) );
          QVERIFY2( told.contains( "160 x 120 pixels" ), qPrintable( told ) );
          const QString file = tmp.filePath( "game.mp4" );
          QCOMPARE( QFileInfo( file ).size(), qint64( 160 * 120 * 4 * 3 ) );
          QFile args( file + ".args" );
          QVERIFY( args.open( QIODevice::ReadOnly ) );
          const QString text = QString::fromUtf8( args.readAll() );
          QVERIFY2( text.contains( "animation" ), qPrintable( text ) );
          QVERIFY2( text.contains( "\n21\n" ), qPrintable( text ) );
      }

    //! what cannot be exported: told, the exit code 1
    void failures()
      {
          QTemporaryDir tmp;
          Options::BatchExportSettings s;
          s.images = tmp.filePath( "out" );
          const auto failure = [&]( const QString & game_log, const QString & expected ) {
              std::ostringstream log;
              const int code = ExportBatch( s, log ).run( game_log );
              const QString told = QString::fromStdString( log.str() );
              return code == 1
                  && told.contains( "rcssmonitor: " )
                  && told.contains( expected );
          };
          QVERIFY( failure( QString(), "No game log" ) );
          QVERIFY( failure( tmp.filePath( "none.rcg" ), "Could not read the game log" ) );
          s.ranges = QStringList( "100" );
          QVERIFY( failure( LOG, "The range 100: " ) );
          s.ranges.clear();
          s.images.clear();
          QVERIFY( failure( LOG, "Nothing to export" ) );
          s.video = tmp.filePath( "a.avi" );
          QVERIFY( failure( LOG, "A video is a .mp4, .mov or .mkv file: not .avi." ) );
          s.video = tmp.filePath( "a.mp4" );
          s.ffmpeg = tmp.filePath( "no-ffmpeg" );
          QVERIFY( failure( LOG, "A video needs ffmpeg" ) );
          QVERIFY( ! QFile::exists( tmp.filePath( "a.mp4" ) ) );
      }

    //! the 3D view: drawn between the frames (30 images/s), or why not
    void view3D()
      {
          QTemporaryDir tmp;
          Options::BatchExportSettings s;
          s.images = tmp.filePath( "out" );
          s.width = 160;
          s.height = 90;
          s.view_3d = true;
          std::ostringstream log;
          const int code = ExportBatch( s, log ).run( LOG );
          const QString told = QString::fromStdString( log.str() );
          if ( code != 0
               && ( told.contains( "OpenGL" ) || told.contains( "built without the 3D view" ) ) )
          {
              QSKIP( qPrintable( "the 3D view cannot be drawn here: " + told.trimmed().split( '\n' ).last() ) );
          }
          QCOMPARE( code, 0 );
          QVERIFY2( told.contains( "Exporting 7 images of 3 frames, 160 x 90 pixels, 30 images/s, by the 3D view" ), qPrintable( told ) );
          QCOMPARE( QDir( s.images ).entryList( QDir::Files ).size(), 7 );
          QCOMPARE( QImage( s.images + "/image-00000.png" ).size(), QSize( 160, 90 ) );
      }
};

QTEST_MAIN( TestExportBatch )
#include "test_export_batch.moc"
