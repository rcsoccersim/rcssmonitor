// -*-c++-*-

/*!
  \file test_archive_log_source.cpp
  \brief unit test of ArchiveLogSource, with an HTTP server on the local
  machine for h5ai
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
  docs/plan/2026-11-22-02_remote-logs.md 4.3. test/data/h5ai_2023.json and
  h5ai_finals.json are the answers of archive.robocup.info (2026-11-24) for
  /Soccer/Simulation/2D/logs/RoboCup/2023/ and its Finals/ (4 games of its
  25 kept): the directories up to the root and those beside them, and the
  entries of the directory.
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
            const qint64 age_secs )
{
    const QString path = source.cachePath( dir );
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
      }

    void defaults();
    void parseUrl_data();
    void parseUrl();
    void parseDirectories();
    void parseFiles();
    void parseNames();
    void parseNotH5ai();
    void listAndCache();
    void reload();
    void severalDirectories();
    void failures();
    void aborted();
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
    QVERIFY( source.cachePath( "" ).startsWith( "/cache/archive.robocup.info/" ) );
    QVERIFY( source.cachePath( "" ).endsWith( ".json" ) );
    QVERIFY( source.cachePath( "" ) != source.cachePath( "RoboCup" ) );
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
    QVERIFY( QFile::exists( source.cachePath( "RoboCup" ) ) );

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
    server.route( "/logs/page/", HttpTestServer::ok( "<html><body>Index of /logs/page</body></html>" ) );
    HttpTestServer::Response error;
    error.status = 500;
    error.reason = "Internal Server Error";
    server.route( "/logs/broken/", error );
    QTemporaryDir cache;
    ArchiveLogSource source( server.url( "/logs/" ), cache.path() );
    Told told( source );

    source.list( "none", false );
    QVERIFY( told.waitFor( 1 ) );
    QVERIFY2( told.failures.last().startsWith( "none: " ), qPrintable( told.failures.last() ) );
    QVERIFY2( told.failures.last().contains( "/logs/none/ is not found on the server" ), qPrintable( told.failures.last() ) );

    source.list( "page", false );
    QVERIFY( told.waitFor( 2 ) );
    QVERIFY2( told.failures.last().contains( "did not answer with a list of files (h5ai)" ), qPrintable( told.failures.last() ) );
    QVERIFY( ! QFile::exists( source.cachePath( "page" ) ) );

    source.list( "broken", false );
    QVERIFY( told.waitFor( 3 ) );
    QVERIFY2( told.failures.last().contains( "HTTP 500 Internal Server Error" ), qPrintable( told.failures.last() ) );

    // an old cache
    QVERIFY( writeCache( source, "broken", answer( { "/logs/broken/kept.rcg" } ), 30 * 24 * 3600 ) );
    source.list( "broken", false );
    QVERIFY( told.waitFor( 4 ) );
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

QTEST_GUILESS_MAIN( TestArchiveLogSource )
#include "test_archive_log_source.moc"
