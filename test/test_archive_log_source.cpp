// -*-c++-*-

/*!
  \file test_archive_log_source.cpp
  \brief unit test of ArchiveLogSource, with an HTTP server on the local
  machine for h5ai and for pages of links
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

#include "archive_log_source.h"
#include "http_test_server.h"
#include "options.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkProxy>
#include <QTemporaryDir>

/*
  docs/plan/2026-11-22-02_remote-logs.md 4.3, 4.9. test/data/h5ai_2023.json
  and h5ai_finals.json are the answers of archive.robocup.info (2026-11-24)
  for /Soccer/Simulation/2D/logs/RoboCup/2023/ and its Finals/ (4 games of
  its 25 kept): the directories up to the root and those beside them, and
  the entries of the directory. h5ai_finals.html is the page of the same
  Finals/ (4 games kept), with its links shown without JavaScript.

  test/data/index_apache_2025.html and index_apache_finals.html are pages of
  the autoindex of Apache 2.4 (FancyIndexing, HTMLTable) of a server of game
  logs (2026-11-24; 4 games of 9 kept), the host and the paths changed:
  /logs/RoboCup2025/ and its Finals/.
*/

namespace {

const QString ARCHIVE = "https://archive.robocup.info/Soccer/Simulation/2D/logs/";

QByteArray
dataOf( const QString & name )
{
    QFile file( QStringLiteral( TEST_DATA_DIR "/" ) + name );
    return file.open( QIODevice::ReadOnly ) ? file.readAll() : QByteArray();
}

//! an answer of h5ai with these hrefs (and the directories up to the root)
QByteArray
answer( const QStringList & hrefs )
{
    QByteArray json = "{\"items\":[{\"href\":\"/\",\"time\":0,\"size\":1,\"managed\":true,\"fetched\":true}";
    for ( const QString & href : hrefs )
    {
        json += ",{\"href\":\"" + href.toUtf8() + "\",\"time\":1688889143000,\"size\":"
            + QByteArray( href.endsWith( '/' ) ? "100" : "5123351" ) + "}";
    }
    return json + "]}";
}

//! the body of the request for a directory
QByteArray
request( const QByteArray & path )
{
    return "action=get&items[href]=" + QUrl::toPercentEncoding( path ) + "&items[what]=1";
}

/*!
  \struct Told
  \brief what a source told
*/
struct Told {
    QStringList dirs; //!< listed
    QHash< QString, QVector< LogSourceEntry > > entries;
    QStringList failures; //!< "DIR: MESSAGE"
    QStringList notes;

    explicit Told( LogSource & source )
      {
          QObject::connect( &source, &LogSource::listed,
                            [this]( const QString & d, const QVector< LogSourceEntry > & e ) {
                                dirs << d;
                                entries[d] = e;
                            } );
          QObject::connect( &source, &LogSource::failed,
                            [this]( const QString & d, const QString & message ) { failures << d + ": " + message; } );
          QObject::connect( &source, &LogSource::noted,
                            [this]( const QString & text ) { notes << text; } );
      }

    int count() const { return dirs.size() + failures.size(); }

    bool waitFor( const int n )
      {
          return QTest::qWaitFor( [this, n]() { return count() >= n; }, 10000 );
      }
};

//! the paths of entries, a directory with '/' at its end
QStringList
pathsOf( const QVector< LogSourceEntry > & entries )
{
    QStringList list;
    for ( const LogSourceEntry & e : entries ) list << e.path + ( e.directory ? "/" : "" );
    return list;
}

//! make the cache file of a directory, of an age
bool
writeCache( const ArchiveLogSource & source,
            const QString & dir,
            const QByteArray & json,
            const qint64 age_secs,
            const ArchiveLogSource::Listing listing = ArchiveLogSource::H5AI_LISTING )
{
    const QString path = source.cachePath( dir, listing );
    if ( ! QDir().mkpath( QFileInfo( path ).absolutePath() ) ) return false;
    QFile file( path );
    if ( ! file.open( QIODevice::WriteOnly ) ) return false;
    file.write( json );
    file.flush(); // not written by close() after the time
    return file.setFileTime( QDateTime::currentDateTime().addSecs( -age_secs ), QFileDevice::FileModificationTime );
}

}

/*!
  \brief unit test of ArchiveLogSource
*/
class TestArchiveLogSource
    : public QObject {
    Q_OBJECT

private slots:

    void initTestCase()
      {
          QNetworkProxy::setApplicationProxy( QNetworkProxy::NoProxy );
          QVERIFY( ! dataOf( "h5ai_2023.json" ).isEmpty() );
          QVERIFY( ! dataOf( "h5ai_finals.json" ).isEmpty() );
          QVERIFY( ! dataOf( "h5ai_finals.html" ).isEmpty() );
          QVERIFY( ! dataOf( "index_apache_2025.html" ).isEmpty() );
          QVERIFY( ! dataOf( "index_apache_finals.html" ).isEmpty() );
      }

    void defaults();
    void parseUrl_data();
    void parseUrl();
    void parseDirectories();
    void parseFiles();
    void parseNames();
    void parseNotH5ai();
    void parseApache();
    void parseIndex_data();
    void parseIndex();
    void parseLinks();
    void parseNotIndex();
    void parseSize_data();
    void parseSize();
    void listAndCache();
    void reload();
    void severalDirectories();
    void failures();
    void aborted();
    void indexListing();
    void findListing();
};

/*-------------------------------------------------------------------*/
void
TestArchiveLogSource::defaults()
{
    QCOMPARE( ArchiveLogSource::CACHE_SECONDS, 24 * 3600 );
    QVERIFY( ArchiveLogSource::defaultCacheDir().endsWith( "/archive" ) );

    // the default source, as text() gives it
    QVERIFY( Options::DEFAULT_LOG_SOURCES.contains( ARCHIVE ) );
    QString error;
    const QUrl url = ArchiveLogSource::parseUrl( ARCHIVE, &error );
    QVERIFY( error.isEmpty() );
    const ArchiveLogSource source( url, "/cache" );
    QCOMPARE( source.text(), ARCHIVE );
    QCOMPARE( source.name(), QString( "archive.robocup.info/Soccer/Simulation/2D/logs" ) );
    QVERIFY( source.description().contains( ARCHIVE ) );
    QCOMPARE( source.dirUrl( "" ).toString(), ARCHIVE );
    QCOMPARE( source.dirUrl( "RoboCup/2023" ).toString(), ARCHIVE + "RoboCup/2023/" );
    QCOMPARE( source.dirUrl( "a b/c%" ).toString( QUrl::FullyEncoded ), ARCHIVE + "a%20b/c%25/" );
    QCOMPARE( source.listing(), ArchiveLogSource::UNKNOWN_LISTING );
    const auto H5AI = ArchiveLogSource::H5AI_LISTING;
    const auto INDEX = ArchiveLogSource::INDEX_LISTING;
    QVERIFY( source.cachePath( "", H5AI ).startsWith( "/cache/archive.robocup.info/" ) );
    QVERIFY( source.cachePath( "", H5AI ).endsWith( ".json" ) );
    QVERIFY( source.cachePath( "", H5AI ) != source.cachePath( "RoboCup", H5AI ) );
    QVERIFY( source.cachePath( "", INDEX ).endsWith( ".html" ) );
    QCOMPARE( source.cachePath( "", INDEX ).chopped( 5 ), source.cachePath( "", H5AI ).chopped( 5 ) );
}

/*-------------------------------------------------------------------*/
void
TestArchiveLogSource::parseUrl_data()
{
    QTest::addColumn< QString >( "text" );
    QTest::addColumn< QString >( "url" ); // empty: not one

    QTest::newRow( "the archive" ) << ARCHIVE << ARCHIVE;
    QTest::newRow( "no '/' at the end" ) << " https://archive.robocup.info/Soccer/Simulation/2D/logs/RoboCup/2023 "
                                         << ARCHIVE + "RoboCup/2023/";
    QTest::newRow( "http, no path" ) << "http://example.org" << "http://example.org/";
    QTest::newRow( "a port" ) << "http://127.0.0.1:8080/logs" << "http://127.0.0.1:8080/logs/";
    QTest::newRow( "encoded" ) << "https://example.org/a%20b" << "https://example.org/a%20b/";
    QTest::newRow( "a query and a fragment" ) << "https://example.org/logs/?x=1#y" << "https://example.org/logs/";
    QTest::newRow( "ftp" ) << "ftp://example.org/logs/" << "";
    QTest::newRow( "no scheme" ) << "archive.robocup.info/Soccer/" << "";
    QTest::newRow( "a user" ) << "https://user:pass@example.org/logs/" << "";
    QTest::newRow( "GitHub" ) << "https://github.com/rcsoccersim/RoboCup2026" << "";
    QTest::newRow( "no host" ) << "https:///logs/" << "";
    QTest::newRow( "nothing" ) << "" << "";
}

/*-------------------------------------------------------------------*/
void
TestArchiveLogSource::parseUrl()
{
    QFETCH( QString, text );
    QFETCH( QString, url );

    QString error = "x";
    const QUrl u = ArchiveLogSource::parseUrl( text, &error );
    if ( url.isEmpty() )
    {
        QVERIFY( ! u.isValid() );
        QVERIFY( ! error.isEmpty() );
        QVERIFY2( error.contains( text.trimmed() ), qPrintable( error ) );
    }
    else
    {
        QCOMPARE( u.toString( QUrl::FullyEncoded ), url );
        QVERIFY( error.isEmpty() );
    }
}

/*-------------------------------------------------------------------*/
//! the directories right under a directory: not those up to the root, nor beside them
void
TestArchiveLogSource::parseDirectories()
{
    QString error = "x";
    const QVector< LogSourceEntry > entries
        = ArchiveLogSource::parseItems( dataOf( "h5ai_2023.json" ),
                                        QUrl( ARCHIVE + "RoboCup/2023/" ),
                                        "RoboCup/2023",
                                        &error );
    QVERIFY( error.isEmpty() );
    QCOMPARE( pathsOf( entries ),
              QStringList( { "RoboCup/2023/Finals/", "RoboCup/2023/MainRound/", "RoboCup/2023/Round1/",
                             "RoboCup/2023/Round2/", "RoboCup/2023/Round3/", "RoboCup/2023/SeedingRound/" } ) );
    for ( const LogSourceEntry & e : entries )
    {
        QVERIFY( e.directory );
        QVERIFY( ! e.listed );
        QVERIFY( ! e.url.isValid() );
    }

    // the root of the 2D logs, from the same answer
    const QVector< LogSourceEntry > root
        = ArchiveLogSource::parseItems( dataOf( "h5ai_2023.json" ), QUrl( ARCHIVE ), "", &error );
    QCOMPARE( pathsOf( root ), QStringList( { "RCAP/", "RoboCup/" } ) );
}

/*-------------------------------------------------------------------*/
//! the game logs (not .rcl.gz), their sizes and URLs
void
TestArchiveLogSource::parseFiles()
{
    QString error;
    const QVector< LogSourceEntry > entries
        = ArchiveLogSource::parseItems( dataOf( "h5ai_finals.json" ),
                                        QUrl( ARCHIVE + "RoboCup/2023/Finals/" ),
                                        "RoboCup/2023/Finals",
                                        &error );
    QVERIFY( error.isEmpty() );
    QCOMPARE( entries.size(), 4 );
    const LogSourceEntry & e = entries.first();
    QCOMPARE( e.path, QString( "RoboCup/2023/Finals/20230709095223-Hermes2D_3-vs-R3CESBU_3.rcg.gz" ) );
    QVERIFY( ! e.directory );
    QCOMPARE( e.size, qint64( 5123351 ) );
    QCOMPARE( e.url.toString(), ARCHIVE + "RoboCup/2023/Finals/20230709095223-Hermes2D_3-vs-R3CESBU_3.rcg.gz" );
    for ( const LogSourceEntry & f : entries )
    {
        QVERIFY( f.path.endsWith( ".rcg.gz" ) );
    }
}

/*-------------------------------------------------------------------*/
//! names encoded by h5ai, files of other kinds, entries deeper
void
TestArchiveLogSource::parseNames()
{
    const QByteArray json = answer( { "/x/", "/x/logs/", "/x/logs/a%20b/", "/x/logs/g%C3%A9.rcg",
                                      "/x/logs/readme.txt", "/x/logs/game.rcl.gz", "/x/logs/old.log.gz",
                                      "/x/logs/sub/deep.rcg.gz", "/x/other.rcg" } );
    QString error;
    const QVector< LogSourceEntry > entries
        = ArchiveLogSource::parseItems( json, QUrl( "http://example.org/x/logs/" ), "", &error );
    QVERIFY( error.isEmpty() );
    QCOMPARE( pathsOf( entries ), QStringList( { QString( "a b/" ), QString::fromUtf8( "g\xc3\xa9.rcg" ) } ) );
    QCOMPARE( entries.at( 1 ).url.toString( QUrl::FullyEncoded ), QString( "http://example.org/x/logs/g%C3%A9.rcg" ) );

    // a directory with a space
    const QVector< LogSourceEntry > sub
        = ArchiveLogSource::parseItems( answer( { "/x/logs/a%20b/", "/x/logs/a%20b/1.rcg" } ),
                                        QUrl( "http://example.org/x/logs/a%20b/" ), "a b", &error );
    QCOMPARE( pathsOf( sub ), QStringList( { "a b/1.rcg" } ) );
    QCOMPARE( sub.first().url.toString( QUrl::FullyEncoded ), QString( "http://example.org/x/logs/a%20b/1.rcg" ) );
}

/*-------------------------------------------------------------------*/
//! not an answer of h5ai (e.g. a page of the directory): an error
void
TestArchiveLogSource::parseNotH5ai()
{
    for ( const QByteArray & json : { QByteArray( "<html><body>Index of /logs</body></html>" ),
                                      QByteArray( "{\"items\":3}" ), QByteArray() } )
    {
        QString error;
        QVERIFY( ArchiveLogSource::parseItems( json, QUrl( ARCHIVE ), "", &error ).isEmpty() );
        QVERIFY2( error.contains( "h5ai" ), qPrintable( error ) );
    }
}

/*-------------------------------------------------------------------*/
//! the pages of Apache: the directories, then the game logs with their (rounded) sizes
void
TestArchiveLogSource::parseApache()
{
    QString error = "x";
    const QVector< LogSourceEntry > dirs
        = ArchiveLogSource::parseIndex( dataOf( "index_apache_2025.html" ),
                                        QUrl( "https://example.org/logs/RoboCup2025/" ),
                                        "RoboCup2025",
                                        &error );
    QVERIFY( error.isEmpty() );
    QCOMPARE( pathsOf( dirs ),
              QStringList( { "RoboCup2025/Finals/", "RoboCup2025/GroupA/", "RoboCup2025/GroupB/", "RoboCup2025/GroupC/",
                             "RoboCup2025/GroupD/", "RoboCup2025/GroupE/", "RoboCup2025/GroupF/", "RoboCup2025/GroupG/",
                             "RoboCup2025/GroupH/", "RoboCup2025/GroupI/", "RoboCup2025/GroupJ/", "RoboCup2025/GroupK/",
                             "RoboCup2025/GroupL/" } ) );
    for ( const LogSourceEntry & e : dirs )
    {
        QVERIFY( e.directory );
        QVERIFY( ! e.listed );
        QVERIFY( ! e.url.isValid() );
        QCOMPARE( e.size, qint64( -1 ) );
    }

    const QVector< LogSourceEntry > logs
        = ArchiveLogSource::parseIndex( dataOf( "index_apache_finals.html" ),
                                        QUrl( "https://example.org/logs/RoboCup2025/Finals/" ),
                                        "RoboCup2025/Finals",
                                        &error );
    QVERIFY( error.isEmpty() );
    QCOMPARE( pathsOf( logs ),
              QStringList( { "RoboCup2025/Finals/202507201242-SIRLab_5-vs-ITAndroids_0.rcg.gz",
                             "RoboCup2025/Finals/202507201255-SIRLab_5-vs-RoboTech_3.rcg.gz",
                             "RoboCup2025/Finals/202507201308-SIRLab_0-vs-SRBIAU2D_1.rcg.gz",
                             "RoboCup2025/Finals/202507201321-SRBIAU2D_4-vs-YuShan2025_6.rcg.gz" } ) );
    QCOMPARE( logs.at( 0 ).size, qint64( 5242880 ) ); // 5.0M
    QCOMPARE( logs.at( 3 ).size, qint64( 6920602 ) ); // 6.6M
    QCOMPARE( logs.at( 0 ).url.toString(),
              QString( "https://example.org/logs/RoboCup2025/Finals/202507201242-SIRLab_5-vs-ITAndroids_0.rcg.gz" ) );
}

/*-------------------------------------------------------------------*/
void
TestArchiveLogSource::parseIndex_data()
{
    QTest::addColumn< QByteArray >( "html" );
    QTest::addColumn< QString >( "url" );
    QTest::addColumn< QStringList >( "paths" );
    QTest::addColumn< QString >( "sizes" );

    const QString game = "202507201242-SIRLab_5-vs-ITAndroids_0.rcg.gz";

    // IndexOptions without HTMLTable: the names cut
    QTest::newRow( "Apache, preformatted" )
        << QByteArray( "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
                       "<html>\n <head>\n  <title>Index of /logs</title>\n </head>\n <body>\n<h1>Index of /logs</h1>\n"
                       "<pre><img src=\"/icons/blank.gif\" alt=\"Icon \"> <a href=\"?C=N;O=D\">Name</a>"
                       "                    <a href=\"?C=M;O=A\">Last modified</a>      <a href=\"?C=S;O=A\">Size</a>"
                       "  <a href=\"?C=D;O=A\">Description</a><hr><img src=\"/icons/back.gif\" alt=\"[PARENTDIR]\">"
                       " <a href=\"/\">Parent Directory</a>                             -   \n"
                       "<img src=\"/icons/folder.gif\" alt=\"[DIR]\"> <a href=\"Finals/\">Finals/</a>"
                       "                 2025-07-21 07:06    -   \n"
                       "<img src=\"/icons/compressed.gif\" alt=\"[   ]\"> <a href=\"202507201242-SIRLab_5-vs-ITAndroids_0.rcg.gz\">"
                       "202507201242-SIRLab_5-vs-ITAnd..&gt;</a> 2025-07-20 21:55  5.0M  \n"
                       "<img src=\"/icons/compressed.gif\" alt=\"[   ]\"> <a href=\"202507201242-SIRLab_5-vs-ITAndroids_0.rcl.gz\">"
                       "202507201242-SIRLab_5-vs-ITAnd..&gt;</a> 2025-07-20 21:55  1.4M  \n"
                       "<img src=\"/icons/unknown.gif\" alt=\"[   ]\"> <a href=\"small.rcg\">small.rcg</a>"
                       "               2025-07-20 21:55  512   \n"
                       "<hr></pre>\n<address>Apache/2.4.58 (Ubuntu) Server at example.org Port 80</address>\n</body></html>\n" )
        << "http://example.org/logs/"
        << QStringList( { "Finals/", game, "small.rcg" } )
        << "-1 5242880 512";

    // IndexOptions IconsAreLinks: two links to a file
    QTest::newRow( "Apache, icons as links" )
        << QByteArray( "<html><body><pre>"
                       "<a href=\"small.rcg\"><img src=\"/icons/unknown.gif\" alt=\"[   ]\"></a> <a href=\"small.rcg\">small.rcg</a>"
                       "  2025-07-20 21:55  2.5K  \n</pre></body></html>" )
        << "http://example.org/logs/"
        << QStringList( { "small.rcg" } )
        << "2560";

    // the sizes in bytes (autoindex_exact_size on, the default)
    QTest::newRow( "nginx" )
        << QByteArray( "<html>\n<head><title>Index of /logs/</title></head>\n<body>\n<h1>Index of /logs/</h1><hr><pre>"
                       "<a href=\"../\">../</a>\n"
                       "<a href=\"Finals/\">Finals/</a>                                            21-Jul-2025 07:06                   -\n"
                       "<a href=\"202507201242-SIRLab_5-vs-ITAndroids_0.rcg.gz\">202507201242-SIRLab_5-vs-ITAndroids_0.rcg.gz</a>"
                       "       20-Jul-2025 21:55             5242880\n"
                       "<a href=\"a%20b.rcg\">a b.rcg</a>                                          20-Jul-2025 21:55                1200\n"
                       "</pre><hr></body>\n</html>\n" )
        << "http://example.org/logs/"
        << QStringList( { "Finals/", game, "a b.rcg" } )
        << "-1 5242880 1200";

    // autoindex_exact_size off
    QTest::newRow( "nginx, rounded" )
        << QByteArray( "<html><body><pre><a href=\"../\">../</a>\n"
                       "<a href=\"202507201242-SIRLab_5-vs-ITAndroids_0.rcg.gz\">202507201242-SIRLab_5-vs-ITAndroids_0.rcg.gz</a>"
                       "       20-Jul-2025 21:55                  5M\n"
                       "<a href=\"small.rcg\">small.rcg</a>      20-Jul-2025 21:55                 12K\n"
                       "</pre></body></html>" )
        << "http://example.org/logs/"
        << QStringList( { game, "small.rcg" } )
        << "5242880 12288";

    // python3 -m http.server (3.12): no sizes
    QTest::newRow( "Python's http.server" )
        << QByteArray( "<!DOCTYPE HTML>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
                       "<title>Directory listing for /logs/</title>\n</head>\n<body>\n"
                       "<h1>Directory listing for /logs/</h1>\n<hr>\n<ul>\n"
                       "<li><a href=\"202507201242-SIRLab_5-vs-ITAndroids_0.rcg.gz\">202507201242-SIRLab_5-vs-ITAndroids_0.rcg.gz</a></li>\n"
                       "<li><a href=\"202507201242-SIRLab_5-vs-ITAndroids_0.rcl.gz\">202507201242-SIRLab_5-vs-ITAndroids_0.rcl.gz</a></li>\n"
                       "<li><a href=\"a%26b.rcg\">a&amp;b.rcg</a></li>\n"
                       "<li><a href=\"Finals/\">Finals/</a></li>\n"
                       "<li><a href=\"Group%20A/\">Group A/</a></li>\n"
                       "</ul>\n<hr>\n</body>\n</html>\n" )
        << "http://127.0.0.1:8000/logs/"
        << QStringList( { game, "a&b.rcg", "Finals/", "Group A/" } )
        << "-1 -1 -1 -1";

    // the page of h5ai (its links without JavaScript): absolute paths, "KB"
    QTest::newRow( "h5ai's page" )
        << dataOf( "h5ai_finals.html" )
        << ARCHIVE + "RoboCup/2023/Finals/"
        << QStringList( { "20230709095223-Hermes2D_3-vs-R3CESBU_3.rcg.gz", "20230709095303-Hermes2D_2-vs-R3CESBU_6.rcg.gz",
                          "20230709100741-Hades2D_2-vs-R3CESBU_2.rcg.gz", "20230709100754-Hades2D_2-vs-R3CESBU_3.rcg.gz" } )
        << "5245952 5390336 5327872 5260288";

    QTest::newRow( "an empty directory" )
        << QByteArray( "<html><body><h1>Index of /logs</h1><table><tr><td><a href=\"/\">Parent Directory</a></td></tr></table></body></html>" )
        << "http://example.org/logs/"
        << QStringList()
        << "";
}

/*-------------------------------------------------------------------*/
void
TestArchiveLogSource::parseIndex()
{
    QFETCH( QByteArray, html );
    QFETCH( QString, url );
    QFETCH( QStringList, paths );
    QFETCH( QString, sizes );

    QString error = "x";
    const QVector< LogSourceEntry > entries = ArchiveLogSource::parseIndex( html, QUrl( url ), "", &error );
    QVERIFY2( error.isEmpty(), qPrintable( error ) );
    QCOMPARE( pathsOf( entries ), paths );
    QStringList s;
    for ( const LogSourceEntry & e : entries )
    {
        s << QString::number( e.size );
        if ( ! e.directory )
        {
            QCOMPARE( e.url.adjusted( QUrl::RemovePath ), QUrl( url ).adjusted( QUrl::RemovePath ) );
            QCOMPARE( e.url.path( QUrl::FullyDecoded ), QUrl( url ).path() + e.path );
        }
    }
    QCOMPARE( s.join( ' ' ), sizes );
}

/*-------------------------------------------------------------------*/
//! the links taken: right under the directory, on the same server, each once
void
TestArchiveLogSource::parseLinks()
{
    const QByteArray html = "<html><body>\n"
        "<a href=\"?C=N;O=D\">Name</a> <a href=\"/x/\">Parent</a> <a href=\"../\">up</a> <a href=\"./\">here</a>"
        " <a href=\"#top\">top</a> <a href=\"\">empty</a>\n"
        "<A HREF=\"1.rcg\">upper case</A>\n"
        "<a class=\"x\" href='2.rcg'>quoted once</a>\n"
        "<a href=3.rcg>not quoted</a>\n"
        "<a href=\"a&amp;b.rcg\">a reference</a>\n"
        "<a href=\"g%C3%A9.rcg\">encoded</a>\n"
        "<a href=\"/x/logs/4.rcg\">an absolute path</a>\n"
        "<a href=\"http://example.org/x/logs/5.rcg\">a full URL</a>\n"
        "<a href=\"http://EXAMPLE.org:80/x/logs/6.rcg\">the default port</a>\n"
        "<a href=\"https://example.org/x/logs/7.rcg\">another scheme</a>\n"
        "<a href=\"http://other.org/x/logs/8.rcg\">another host</a>\n"
        "<a href=\"http://example.org:8080/x/logs/9.rcg\">another port</a>\n"
        "<a href=\"/x/other/10.rcg\">another directory</a>\n"
        "<a href=\"sub/deep.rcg\">deeper</a>\n"
        "<a href=\"11.rcg?x=1\">a query</a>\n"
        "<a href=\"1.rcg\"><img src=\"icon.gif\"></a>\n"
        "<a href=\"game.rcl.gz\">a command log</a> <a href=\"readme.txt\">readme</a>\n"
        "<a href=\"dir/\">dir/</a> <a href=\"./dir/\">again</a> <a href=\"sub/../dir2/\">dir2</a>\n"
        "<a data-href=\"12.rcg\" href=\"13.rcg\">data-href first</a>\n"
        "</body></html>\n";
    QString error = "x";
    const QVector< LogSourceEntry > entries
        = ArchiveLogSource::parseIndex( html, QUrl( "http://example.org/x/logs/" ), "logs", &error );
    QVERIFY( error.isEmpty() );
    QCOMPARE( pathsOf( entries ),
              QStringList( { "logs/1.rcg", "logs/2.rcg", "logs/3.rcg", "logs/a&b.rcg", QString::fromUtf8( "logs/g\xc3\xa9.rcg" ),
                             "logs/4.rcg", "logs/5.rcg", "logs/6.rcg", "logs/dir/", "logs/dir2/", "logs/13.rcg" } ) );
    QCOMPARE( entries.at( 4 ).url.toString( QUrl::FullyEncoded ), QString( "http://example.org/x/logs/g%C3%A9.rcg" ) );
    QCOMPARE( entries.at( 5 ).url.toString(), QString( "http://example.org/x/logs/4.rcg" ) );
}

/*-------------------------------------------------------------------*/
//! not a page (e.g. an answer of h5ai): an error
void
TestArchiveLogSource::parseNotIndex()
{
    for ( const QByteArray & page : { QByteArray( "{\"items\":[{\"href\":\"/logs/1.rcg\"}]}" ),
                                      QByteArray( "Not Found" ), QByteArray() } )
    {
        QString error;
        QVERIFY( ArchiveLogSource::parseIndex( page, QUrl( ARCHIVE ), "", &error ).isEmpty() );
        QVERIFY2( error.contains( "did not answer with a list of files" ), qPrintable( error ) );
    }
}

/*-------------------------------------------------------------------*/
void
TestArchiveLogSource::parseSize_data()
{
    QTest::addColumn< QString >( "text" );
    QTest::addColumn< qint64 >( "size" );

    QTest::newRow( "Apache" ) << " 2025-07-20 21:55  5.0M  " << qint64( 5242880 );
    QTest::newRow( "Apache, K" ) << "2025-07-20 21:55  512K" << qint64( 524288 );
    QTest::newRow( "Apache, bytes" ) << "2025-07-20 21:55  512 " << qint64( 512 );
    QTest::newRow( "Apache, a directory" ) << "2025-07-20 21:55    -   " << qint64( -1 );
    QTest::newRow( "nginx" ) << "20-Jul-2025 21:55             5242880" << qint64( 5242880 );
    QTest::newRow( "h5ai" ) << " 2023-07-09 07:52 5123 KB " << qint64( 5245952 );
    QTest::newRow( "lighttpd" ) << "2025-Jul-20 21:55:00 5.0M application/x-gzip" << qint64( 5242880 );
    QTest::newRow( "MiB" ) << "1.5 MiB" << qint64( 1572864 );
    QTest::newRow( "GB" ) << "2GB" << qint64( 2147483648LL );
    QTest::newRow( "T" ) << "1T" << qint64( 1099511627776LL );
    QTest::newRow( "lower case" ) << "1.4m" << qint64( 1468006 );
    QTest::newRow( "bytes" ) << "100 bytes" << qint64( 100 );
    QTest::newRow( "B" ) << "12 B" << qint64( 12 );
    QTest::newRow( "a date and a time only" ) << "2025-07-20 21:55" << qint64( -1 );
    QTest::newRow( "a unit unknown" ) << "5X" << qint64( -1 );
    QTest::newRow( "a number and a word" ) << "5 X" << qint64( 5 );
    QTest::newRow( "nothing" ) << "" << qint64( -1 );
}

/*-------------------------------------------------------------------*/
void
TestArchiveLogSource::parseSize()
{
    QFETCH( QString, text );
    QFETCH( qint64, size );

    QCOMPARE( ArchiveLogSource::parseSize( text ), size );
}

/*-------------------------------------------------------------------*/
/*!
  A directory at a time, by a POST to it: its list kept in the cache for a
  day (no request), then asked again.
*/
void
TestArchiveLogSource::listAndCache()
{
    HttpTestServer server;
    server.route( "/logs/", HttpTestServer::ok( answer( { "/logs/", "/logs/RoboCup/", "/logs/top.rcg.gz" } ) ) );
    server.route( "/logs/RoboCup/", HttpTestServer::ok( answer( { "/logs/", "/logs/RoboCup/", "/logs/RoboCup/2023/" } ) ) );
    QTemporaryDir cache;
    QVERIFY( cache.isValid() );
    ArchiveLogSource source( server.url( "/logs/" ), cache.path() );
    Told told( source );

    source.list( QString(), false );
    QCOMPARE( told.count(), 0 ); // not before list() returns
    QVERIFY( told.waitFor( 1 ) );
    QVERIFY2( told.failures.isEmpty(), qPrintable( told.failures.join( ' ' ) ) );
    QCOMPARE( told.dirs, QStringList( { "" } ) );
    QCOMPARE( pathsOf( told.entries[""] ), QStringList( { "RoboCup/", "top.rcg.gz" } ) );
    QCOMPARE( told.entries[""].at( 1 ).url, server.url( "/logs/top.rcg.gz" ) );
    QCOMPARE( server.requests( "/logs/" ), 1 );
    QCOMPARE( server.body( "/logs/" ), request( "/logs/" ) );
    QVERIFY2( server.userAgent().startsWith( "rcssmonitor/" ), server.userAgent().constData() );
    QVERIFY( told.notes.isEmpty() );

    source.list( "RoboCup", false );
    QVERIFY( told.waitFor( 2 ) );
    QCOMPARE( told.dirs.last(), QString( "RoboCup" ) );
    QCOMPARE( pathsOf( told.entries["RoboCup"] ), QStringList( { "RoboCup/2023/" } ) );
    QCOMPARE( server.body( "/logs/RoboCup/" ), request( "/logs/RoboCup/" ) );
    QVERIFY( QFile::exists( source.cachePath( "RoboCup", ArchiveLogSource::H5AI_LISTING ) ) );
    // h5ai: no page asked for
    QCOMPARE( source.listing(), ArchiveLogSource::H5AI_LISTING );
    QCOMPARE( server.requests( "GET", "/logs/" ), 0 );
    QCOMPARE( server.requests( "GET", "/logs/RoboCup/" ), 0 );

    // from the cache, with a note
    source.list( "RoboCup", false );
    QVERIFY( told.waitFor( 3 ) );
    QCOMPARE( server.requests( "/logs/RoboCup/" ), 1 );
    QCOMPARE( pathsOf( told.entries["RoboCup"] ), QStringList( { "RoboCup/2023/" } ) );
    QCOMPARE( told.notes.size(), 1 );
    QVERIFY2( told.notes.last().contains( "The list read at" ), qPrintable( told.notes.last() ) );
    QVERIFY( told.notes.last().contains( "a day later" ) );

    // a day old: asked again
    QVERIFY( writeCache( source, "RoboCup", answer( { "/logs/RoboCup/old/" } ), 25 * 3600 ) );
    source.list( "RoboCup", false );
    QVERIFY( told.waitFor( 4 ) );
    QCOMPARE( server.requests( "/logs/RoboCup/" ), 2 );
    QCOMPARE( pathsOf( told.entries["RoboCup"] ), QStringList( { "RoboCup/2023/" } ) );
}

/*-------------------------------------------------------------------*/
//! Reload: the root asked again, and the directories opened after it too
void
TestArchiveLogSource::reload()
{
    HttpTestServer server;
    server.route( "/logs/", HttpTestServer::ok( answer( { "/logs/RoboCup/" } ) ) );
    server.route( "/logs/RoboCup/", HttpTestServer::ok( answer( { "/logs/RoboCup/2023/" } ) ) );
    QTemporaryDir cache;
    ArchiveLogSource source( server.url( "/logs/" ), cache.path() );
    Told told( source );

    source.list( QString(), false );
    QVERIFY( told.waitFor( 1 ) );
    source.list( "RoboCup", false );
    QVERIFY( told.waitFor( 2 ) );
    source.list( "RoboCup", false );
    QVERIFY( told.waitFor( 3 ) );
    QCOMPARE( server.requests( "/logs/RoboCup/" ), 1 );

    source.list( QString(), true );
    QVERIFY( told.waitFor( 4 ) );
    QCOMPARE( server.requests( "/logs/" ), 2 );
    source.list( "RoboCup", false );
    QVERIFY( told.waitFor( 5 ) );
    QCOMPARE( server.requests( "/logs/RoboCup/" ), 2 );
    // the new cache: used
    source.list( "RoboCup", false );
    QVERIFY( told.waitFor( 6 ) );
    QCOMPARE( server.requests( "/logs/RoboCup/" ), 2 );

    // a directory reloaded: only it
    source.list( "RoboCup", true );
    QVERIFY( told.waitFor( 7 ) );
    QCOMPARE( server.requests( "/logs/RoboCup/" ), 3 );
    source.list( QString(), false );
    QVERIFY( told.waitFor( 8 ) );
    QCOMPARE( server.requests( "/logs/" ), 2 );
}

/*-------------------------------------------------------------------*/
//! directories read at once (branches opened one after another)
void
TestArchiveLogSource::severalDirectories()
{
    HttpTestServer server;
    HttpTestServer::Response slow = HttpTestServer::ok( answer( { "/logs/a/1.rcg.gz" } ) );
    slow.split_delay = 200;
    server.route( "/logs/a/", slow );
    server.route( "/logs/b/", HttpTestServer::ok( answer( { "/logs/b/2.rcg.gz" } ) ) );
    QTemporaryDir cache;
    ArchiveLogSource source( server.url( "/logs/" ), cache.path() );
    Told told( source );

    source.list( "a", false );
    source.list( "b", false );
    QVERIFY( told.waitFor( 2 ) );
    QCOMPARE( told.dirs, QStringList( { "b", "a" } ) );
    QCOMPARE( pathsOf( told.entries["a"] ), QStringList( { "a/1.rcg.gz" } ) );
    QCOMPARE( pathsOf( told.entries["b"] ), QStringList( { "b/2.rcg.gz" } ) );

    // the same directory again: the first request dropped
    QTest::qWait( 100 );
    told.dirs.clear();
    source.list( "a", true );
    source.list( "a", true );
    QVERIFY( told.waitFor( 1 ) );
    QTest::qWait( 400 );
    QCOMPARE( told.dirs, QStringList( { "a" } ) );
}

/*-------------------------------------------------------------------*/
/*!
  A directory not on the server, a server without h5ai, a server error:
  told; the list in the cache, however old, shown with the reason.
*/
void
TestArchiveLogSource::failures()
{
    HttpTestServer server;
    server.route( "/logs/", HttpTestServer::ok( answer( { "/logs/page/" } ) ) );
    server.route( "/logs/page/", HttpTestServer::ok( "<html><body>Index of /logs/page</body></html>" ) );
    server.route( "/logs/broken/", HttpTestServer::error( 500, "Internal Server Error" ) );
    QTemporaryDir cache;
    ArchiveLogSource source( server.url( "/logs/" ), cache.path() );
    Told told( source );

    // not found out yet: the page asked for too
    source.list( "none", false );
    QVERIFY( told.waitFor( 1 ) );
    QVERIFY2( told.failures.last().startsWith( "none: " ), qPrintable( told.failures.last() ) );
    QVERIFY2( told.failures.last().contains( "/logs/none/ is not found on the server" ), qPrintable( told.failures.last() ) );
    QCOMPARE( server.requests( "POST", "/logs/none/" ), 1 );
    QCOMPARE( server.requests( "GET", "/logs/none/" ), 1 );
    QCOMPARE( source.listing(), ArchiveLogSource::UNKNOWN_LISTING );

    // h5ai: an answer of another kind is an error
    source.list( "", false );
    QVERIFY( told.waitFor( 2 ) );
    QCOMPARE( source.listing(), ArchiveLogSource::H5AI_LISTING );
    source.list( "page", false );
    QVERIFY( told.waitFor( 3 ) );
    QVERIFY2( told.failures.last().contains( "did not answer with a list of files (h5ai)" ), qPrintable( told.failures.last() ) );
    QCOMPARE( server.requests( "GET", "/logs/page/" ), 0 );
    QVERIFY( ! QFile::exists( source.cachePath( "page", ArchiveLogSource::H5AI_LISTING ) ) );

    source.list( "broken", false );
    QVERIFY( told.waitFor( 4 ) );
    QVERIFY2( told.failures.last().contains( "HTTP 500 Internal Server Error" ), qPrintable( told.failures.last() ) );

    // an old cache
    QVERIFY( writeCache( source, "broken", answer( { "/logs/broken/kept.rcg" } ), 30 * 24 * 3600 ) );
    source.list( "broken", false );
    QVERIFY( told.waitFor( 5 ) );
    QCOMPARE( told.failures.size(), 3 );
    QCOMPARE( pathsOf( told.entries["broken"] ), QStringList( { "broken/kept.rcg" } ) );
    QCOMPARE( told.notes.size(), 1 );
    QVERIFY2( told.notes.last().startsWith( "HTTP 500 Internal Server Error The list read at" ), qPrintable( told.notes.last() ) );
}

/*-------------------------------------------------------------------*/
//! abort(): nothing told, also for a list from the cache
void
TestArchiveLogSource::aborted()
{
    HttpTestServer server;
    HttpTestServer::Response slow = HttpTestServer::ok( answer( { "/logs/1.rcg" } ) );
    slow.hang = true;
    server.route( "/logs/", slow );
    QTemporaryDir cache;
    ArchiveLogSource source( server.url( "/logs/" ), cache.path() );
    Told told( source );

    source.list( QString(), false );
    QTRY_COMPARE( server.requests( "/logs/" ), 1 );
    source.abort();
    QTest::qWait( 300 );
    QCOMPARE( told.count(), 0 );

    QVERIFY( writeCache( source, "a", answer( { "/logs/a/1.rcg" } ), 0 ) );
    source.list( "a", false );
    source.abort();
    QTest::qWait( 100 );
    QCOMPARE( told.count(), 0 );
}

/*-------------------------------------------------------------------*/
/*!
  A server of pages of links (Apache: a POST answered with 404): the page
  read after the POST of the first directory, the others by GET only, kept
  in the cache as .html; Reload finds it out again.
*/
void
TestArchiveLogSource::indexListing()
{
    HttpTestServer server;
    server.route( "POST", "/logs/RoboCup2025/", HttpTestServer::error( 404, "Not Found" ) );
    server.route( "GET", "/logs/RoboCup2025/", HttpTestServer::ok( dataOf( "index_apache_2025.html" ) ) );
    server.route( "GET", "/logs/RoboCup2025/Finals/", HttpTestServer::ok( dataOf( "index_apache_finals.html" ) ) );
    QTemporaryDir cache;
    QVERIFY( cache.isValid() );
    ArchiveLogSource source( server.url( "/logs/RoboCup2025/" ), cache.path() );
    Told told( source );

    source.list( QString(), false );
    QVERIFY( told.waitFor( 1 ) );
    QVERIFY2( told.failures.isEmpty(), qPrintable( told.failures.join( ' ' ) ) );
    QCOMPARE( told.entries[""].size(), 13 );
    QCOMPARE( pathsOf( told.entries[""] ).first(), QString( "Finals/" ) );
    QCOMPARE( source.listing(), ArchiveLogSource::INDEX_LISTING );
    QCOMPARE( server.requests( "POST", "/logs/RoboCup2025/" ), 1 );
    QCOMPARE( server.requests( "GET", "/logs/RoboCup2025/" ), 1 );
    QVERIFY2( server.userAgent().startsWith( "rcssmonitor/" ), server.userAgent().constData() );
    QVERIFY( QFile::exists( source.cachePath( "", ArchiveLogSource::INDEX_LISTING ) ) );
    QVERIFY( ! QFile::exists( source.cachePath( "", ArchiveLogSource::H5AI_LISTING ) ) );

    source.list( "Finals", false );
    QVERIFY( told.waitFor( 2 ) );
    QCOMPARE( told.entries["Finals"].size(), 4 );
    QCOMPARE( told.entries["Finals"].first().path, QString( "Finals/202507201242-SIRLab_5-vs-ITAndroids_0.rcg.gz" ) );
    QCOMPARE( told.entries["Finals"].first().size, qint64( 5242880 ) );
    QCOMPARE( told.entries["Finals"].first().url,
              server.url( "/logs/RoboCup2025/Finals/202507201242-SIRLab_5-vs-ITAndroids_0.rcg.gz" ) );
    QCOMPARE( server.requests( "POST", "/logs/RoboCup2025/Finals/" ), 0 );
    QCOMPARE( server.requests( "GET", "/logs/RoboCup2025/Finals/" ), 1 );

    // from the cache
    source.list( "Finals", false );
    QVERIFY( told.waitFor( 3 ) );
    QCOMPARE( server.requests( "/logs/RoboCup2025/Finals/" ), 1 );
    QCOMPARE( told.entries["Finals"].size(), 4 );
    QVERIFY2( told.notes.last().contains( "The list read at" ), qPrintable( told.notes.last() ) );

    // another source of the same cache: the kind from the cache
    ArchiveLogSource again( server.url( "/logs/RoboCup2025/" ), cache.path() );
    Told told_again( again );
    again.list( QString(), false );
    QVERIFY( told_again.waitFor( 1 ) );
    QCOMPARE( told_again.entries[""].size(), 13 );
    QCOMPARE( again.listing(), ArchiveLogSource::INDEX_LISTING );
    QCOMPARE( server.requests( "/logs/RoboCup2025/" ), 2 );

    // Reload: found out again
    source.list( QString(), true );
    QVERIFY( told.waitFor( 4 ) );
    QCOMPARE( server.requests( "POST", "/logs/RoboCup2025/" ), 2 );
    QCOMPARE( server.requests( "GET", "/logs/RoboCup2025/" ), 2 );
    QCOMPARE( source.listing(), ArchiveLogSource::INDEX_LISTING );
    source.list( "Finals", false );
    QVERIFY( told.waitFor( 5 ) );
    QCOMPARE( server.requests( "GET", "/logs/RoboCup2025/Finals/" ), 2 );

    // a directory not on the server: no POST
    source.list( "none", false );
    QVERIFY( told.waitFor( 6 ) );
    QVERIFY2( told.failures.last().contains( "/logs/RoboCup2025/none/ is not found on the server" ),
              qPrintable( told.failures.last() ) );
    QCOMPARE( server.requests( "POST", "/logs/RoboCup2025/none/" ), 0 );
}

/*-------------------------------------------------------------------*/
/*!
  How a server lists a directory: a POST answered with an error (Python's
  http.server: 501) or with a page (not of h5ai) is followed by a GET; a
  server not reached is not asked again.
*/
void
TestArchiveLogSource::findListing()
{
    const QByteArray page = "<html><body><a href=\"1.rcg\">1.rcg</a></body></html>";
    HttpTestServer server;
    server.route( "POST", "/python/", HttpTestServer::error( 501, "Unsupported method ('POST')" ) );
    server.route( "GET", "/python/", HttpTestServer::ok( page ) );
    server.route( "/any/", HttpTestServer::ok( page ) ); // a POST answered as a GET
    QTemporaryDir cache;

    for ( const QByteArray & path : { QByteArray( "/python/" ), QByteArray( "/any/" ) } )
    {
        ArchiveLogSource source( server.url( path ), cache.path() );
        Told told( source );
        source.list( QString(), false );
        QVERIFY( told.waitFor( 1 ) );
        QVERIFY2( told.failures.isEmpty(), qPrintable( told.failures.join( ' ' ) ) );
        QCOMPARE( pathsOf( told.entries[""] ), QStringList( { "1.rcg" } ) );
        QCOMPARE( source.listing(), ArchiveLogSource::INDEX_LISTING );
        QCOMPARE( server.requests( "POST", path ), 1 );
        QCOMPARE( server.requests( "GET", path ), 1 );
    }

    // not reached: told at once, nothing found out
    QTcpServer closed;
    QVERIFY( closed.listen( QHostAddress::LocalHost, 0 ) );
    const quint16 port = closed.serverPort();
    closed.close();
    ArchiveLogSource source( QUrl( QStringLiteral( "http://127.0.0.1:%1/logs/" ).arg( port ) ), cache.path() );
    Told told( source );
    source.list( QString(), false );
    QVERIFY( told.waitFor( 1 ) );
    QCOMPARE( told.failures.size(), 1 );
    QCOMPARE( source.listing(), ArchiveLogSource::UNKNOWN_LISTING );
    QTest::qWait( 200 );
    QCOMPARE( told.count(), 1 );
}

QTEST_GUILESS_MAIN( TestArchiveLogSource )
#include "test_archive_log_source.moc"
