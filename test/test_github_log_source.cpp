// -*-c++-*-

/*!
  \file test_github_log_source.cpp
  \brief unit test of GitHubLogSource, with an HTTP server on the local
  machine for the API of GitHub
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

#include "github_log_source.h"
#include "http_test_server.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QNetworkProxy>
#include <QTemporaryDir>

/*
  docs/plan/2026-11-22-02_remote-logs.md 4.2, S6. test/data/github_tree.json
  is a part of the tree of rcsoccersim/RoboCup2026 (2026-11-24): 21 logs
  in logs/Finals, logs/GroupA and logs/GroupQ, with their .rcl.gz,
  .gitattributes and README.md.
*/

namespace {

const QByteArray TREE_PATH = "/repos/o/r/git/trees/HEAD?recursive=1";

QByteArray
treeJson()
{
    QFile file( QStringLiteral( TEST_DATA_DIR "/github_tree.json" ) );
    return file.open( QIODevice::ReadOnly ) ? file.readAll() : QByteArray();
}

/*!
  \struct Told
  \brief what a source told
*/
struct Told {
    int listed = 0;
    QString dir;
    QVector< LogSourceEntry > entries;
    QStringList failures;
    QStringList notes;

    explicit Told( LogSource & source )
      {
          QObject::connect( &source, &LogSource::listed,
                            [this]( const QString & d, const QVector< LogSourceEntry > & e ) {
                                ++listed;
                                dir = d;
                                entries = e;
                            } );
          QObject::connect( &source, &LogSource::failed,
                            [this]( const QString &, const QString & message ) { failures << message; } );
          QObject::connect( &source, &LogSource::noted,
                            [this]( const QString & text ) { notes << text; } );
      }

    bool waitForEnd()
      {
          return QTest::qWaitFor( [this]() { return listed + failures.size() > 0; }, 10000 );
      }
};

}

/*!
  \brief unit test of GitHubLogSource
*/
class TestGitHubLogSource
    : public QObject {
    Q_OBJECT

private slots:

    void initTestCase()
      {
          QNetworkProxy::setApplicationProxy( QNetworkProxy::NoProxy );
          QVERIFY( ! treeJson().isEmpty() );
      }

    void cleanupTestCase()
      {
          GitHubLogSource::setApiBase( QUrl( "https://api.github.com" ) );
          GitHubLogSource::setRawBase( QUrl( "https://raw.githubusercontent.com" ) );
      }

    void defaults();
    void parseRepository_data();
    void parseRepository();
    void parseTree();
    void parseNotTree();
    void listAndCache();
    void rateLimit();
    void notFound();
    void truncated();
    void aborted();
};

/*-------------------------------------------------------------------*/
void
TestGitHubLogSource::defaults()
{
    QCOMPARE( GitHubLogSource::DEFAULT_REPOSITORIES, QStringList( { "rcsoccersim/RoboCup2026" } ) );
    QCOMPARE( GitHubLogSource::CACHE_SECONDS, 3600 );
    QCOMPARE( GitHubLogSource::apiBase(), QUrl( "https://api.github.com" ) );
    QCOMPARE( GitHubLogSource::rawBase(), QUrl( "https://raw.githubusercontent.com" ) );
    QVERIFY( GitHubLogSource::defaultCacheDir().endsWith( "/github" ) );

    const GitHubLogSource source( "rcsoccersim", "RoboCup2026", "" );
    QCOMPARE( source.name(), QString( "rcsoccersim/RoboCup2026" ) );
    QCOMPARE( source.ref(), QString( "HEAD" ) );
    QVERIFY( source.description().contains( "default branch" ) );
    QVERIFY( source.cachePath().endsWith( "/github/rcsoccersim/RoboCup2026/HEAD.json" ) );

    const GitHubLogSource branch( "o", "r", "feature/x", "/cache" );
    QCOMPARE( branch.name(), QString( "o/r@feature/x" ) );
    QCOMPARE( branch.cachePath(), QString( "/cache/o/r/feature%2Fx.json" ) );
}

/*-------------------------------------------------------------------*/
void
TestGitHubLogSource::parseRepository_data()
{
    QTest::addColumn< QString >( "text" );
    QTest::addColumn< QString >( "parsed" ); // OWNER|REPO|REF, or empty

    QTest::newRow( "owner/repo" ) << "rcsoccersim/RoboCup2026" << "rcsoccersim|RoboCup2026|HEAD";
    QTest::newRow( "spaces" ) << "  rcsoccersim/RoboCup2026 " << "rcsoccersim|RoboCup2026|HEAD";
    QTest::newRow( "a branch" ) << "o/r@main" << "o|r|main";
    QTest::newRow( "a branch with '/'" ) << "o/r@feature/x" << "o|r|feature/x";
    QTest::newRow( "a page" ) << "https://github.com/rcsoccersim/RoboCup2026" << "rcsoccersim|RoboCup2026|HEAD";
    QTest::newRow( "a page of a branch" ) << "https://github.com/o/r/tree/logs-2025" << "o|r|logs-2025";
    QTest::newRow( "git" ) << "https://github.com/o/r.git" << "o|r|HEAD";
    QTest::newRow( "no scheme" ) << "github.com/o/my.repo/" << "o|my.repo|HEAD";
    QTest::newRow( "an owner" ) << "rcsoccersim" << "";
    QTest::newRow( "a path" ) << "o/r/logs" << "";
    QTest::newRow( "no branch" ) << "o/r@" << "";
    QTest::newRow( "another site" ) << "https://example.org/o/r" << "";
    QTest::newRow( "spaces inside" ) << "o r/x" << "";
    QTest::newRow( "up" ) << "o/.." << "";
    QTest::newRow( "nothing" ) << "" << "";
}

/*-------------------------------------------------------------------*/
void
TestGitHubLogSource::parseRepository()
{
    QFETCH( QString, text );
    QFETCH( QString, parsed );

    QString owner, repo, ref;
    const bool ok = GitHubLogSource::parseRepository( text, &owner, &repo, &ref );
    QCOMPARE( ok, ! parsed.isEmpty() );
    if ( ok )
    {
        QCOMPARE( owner + "|" + repo + "|" + ref, parsed );
        QCOMPARE( GitHubLogSource::repositoryText( owner, repo, ref ),
                  ref == "HEAD" ? owner + "/" + repo : owner + "/" + repo + "@" + ref );
    }
}

/*-------------------------------------------------------------------*/
//! the logs of the tree (not .rcl.gz, nor the others), their sizes and URLs
void
TestGitHubLogSource::parseTree()
{
    bool truncated = true;
    QString error = "x";
    const QVector< LogSourceEntry > entries = GitHubLogSource::parseTree( treeJson(),
                                                                          "https://raw.example.org/o/r/HEAD",
                                                                          &truncated, &error );
    QVERIFY( error.isEmpty() );
    QVERIFY( ! truncated );
    QCOMPARE( entries.size(), 21 );
    int finals = 0;
    for ( const LogSourceEntry & e : entries )
    {
        QVERIFY( e.path.endsWith( ".rcg.gz" ) );
        QVERIFY( ! e.directory );
        QVERIFY( e.size > 1000000 );
        QCOMPARE( e.url.toString(), "https://raw.example.org/o/r/HEAD/" + e.path );
        if ( e.path.startsWith( "logs/Finals/" ) ) ++finals;
    }
    QCOMPARE( finals, 12 );
}

/*-------------------------------------------------------------------*/
//! not a tree (e.g. GitHub's message): an error
void
TestGitHubLogSource::parseNotTree()
{
    for ( const QByteArray & json : { QByteArray( "{\"message\":\"Not Found\"}" ), QByteArray( "<html>" ), QByteArray() } )
    {
        bool truncated = true;
        QString error;
        QVERIFY( GitHubLogSource::parseTree( json, "https://raw", &truncated, &error ).isEmpty() );
        QVERIFY( ! error.isEmpty() );
    }
}

/*-------------------------------------------------------------------*/
/*!
  The tree from the API: kept in the cache file, and used without asking
  GitHub for an hour (another source of the same repository too), unless
  reloaded; asked again after an hour.
*/
void
TestGitHubLogSource::listAndCache()
{
    HttpTestServer server;
    server.route( TREE_PATH, HttpTestServer::ok( treeJson() ) );
    GitHubLogSource::setApiBase( server.url( "" ) );
    GitHubLogSource::setRawBase( server.url( "/raw" ) );
    QTemporaryDir cache;
    QVERIFY( cache.isValid() );

    {
        GitHubLogSource source( "o", "r", "", cache.path() );
        Told told( source );
        source.list( QString(), false );
        QCOMPARE( told.listed, 0 ); // not before list() returns
        QVERIFY( told.waitForEnd() );
        QVERIFY2( told.failures.isEmpty(), qPrintable( told.failures.join( ' ' ) ) );
        QCOMPARE( told.dir, QString() );
        QCOMPARE( told.entries.size(), 21 );
        QCOMPARE( told.entries.first().url.toString(),
                  server.url( "/raw/o/r/HEAD/" ).toString() + told.entries.first().path );
        QCOMPARE( server.requests( TREE_PATH ), 1 );
        QVERIFY( QFile::exists( source.cachePath() ) );
        QVERIFY2( server.userAgent().startsWith( "rcssmonitor/" ), server.userAgent().constData() );
    }
    {
        // from the cache
        GitHubLogSource source( "o", "r", "", cache.path() );
        Told told( source );
        source.list( QString(), false );
        QVERIFY( told.waitForEnd() );
        QCOMPARE( told.entries.size(), 21 );
        QCOMPARE( server.requests( TREE_PATH ), 1 );
        QVERIFY( ! told.notes.isEmpty() );
        QVERIFY2( told.notes.first().contains( "The list read at" ), qPrintable( told.notes.first() ) );

        // reloaded
        told.listed = 0;
        source.list( QString(), true );
        QVERIFY( told.waitForEnd() );
        QCOMPARE( server.requests( TREE_PATH ), 2 );
    }
    {
        // an hour old
        GitHubLogSource source( "o", "r", "", cache.path() );
        QFile file( source.cachePath() );
        QVERIFY( file.open( QIODevice::ReadWrite ) );
        QVERIFY( file.setFileTime( QDateTime::currentDateTime().addSecs( -2 * 3600 ), QFileDevice::FileModificationTime ) );
        file.close();
        Told told( source );
        source.list( QString(), false );
        QVERIFY( told.waitForEnd() );
        QCOMPARE( server.requests( TREE_PATH ), 3 );
    }
}

/*-------------------------------------------------------------------*/
/*!
  The limit of GitHub used up: told with the time when it is lifted; the
  list in the cache, however old, is shown with that note.
*/
void
TestGitHubLogSource::rateLimit()
{
    HttpTestServer server;
    const qint64 reset = QDateTime::currentSecsSinceEpoch() + 1800;
    HttpTestServer::Response limit;
    limit.status = 403;
    limit.reason = "Forbidden";
    limit.headers.append( qMakePair( QByteArray( "x-ratelimit-limit" ), QByteArray( "60" ) ) );
    limit.headers.append( qMakePair( QByteArray( "x-ratelimit-remaining" ), QByteArray( "0" ) ) );
    limit.headers.append( qMakePair( QByteArray( "x-ratelimit-reset" ), QByteArray::number( reset ) ) );
    limit.body = "{\"message\":\"API rate limit exceeded\"}";
    server.route( TREE_PATH, limit );
    GitHubLogSource::setApiBase( server.url( "" ) );
    QTemporaryDir cache;
    QVERIFY( cache.isValid() );
    const QString until = QDateTime::fromSecsSinceEpoch( reset ).toString( "yyyy-MM-dd HH:mm" );

    {
        // no cache
        GitHubLogSource source( "o", "r", "", cache.path() );
        Told told( source );
        source.list( QString(), true );
        QVERIFY( told.waitForEnd() );
        QCOMPARE( told.listed, 0 );
        QCOMPARE( told.failures.size(), 1 );
        QVERIFY2( told.failures.first().contains( "used up until " + until ), qPrintable( told.failures.first() ) );
    }
    {
        // an old cache
        GitHubLogSource source( "o", "r", "", cache.path() );
        QVERIFY( QDir().mkpath( QFileInfo( source.cachePath() ).absolutePath() ) );
        QFile file( source.cachePath() );
        QVERIFY( file.open( QIODevice::WriteOnly ) );
        file.write( treeJson() );
        QVERIFY( file.flush() ); // not written by close() after the time
        QVERIFY( file.setFileTime( QDateTime::currentDateTime().addDays( -3 ), QFileDevice::FileModificationTime ) );
        file.close();

        Told told( source );
        source.list( QString(), false );
        QVERIFY( told.waitForEnd() );
        QCOMPARE( told.failures.size(), 0 );
        QCOMPARE( told.entries.size(), 21 );
        QCOMPARE( told.notes.size(), 1 );
        QVERIFY2( told.notes.first().contains( "used up until " + until ), qPrintable( told.notes.first() ) );
        QVERIFY( told.notes.first().contains( "The list read at" ) );
    }
}

/*-------------------------------------------------------------------*/
//! a repository or a branch that is not there
void
TestGitHubLogSource::notFound()
{
    HttpTestServer server;
    GitHubLogSource::setApiBase( server.url( "" ) );
    QTemporaryDir cache;
    GitHubLogSource source( "o", "none", "", cache.path() );
    Told told( source );
    source.list( QString(), false );
    QVERIFY( told.waitForEnd() );
    QCOMPARE( told.failures.size(), 1 );
    QVERIFY2( told.failures.first().contains( "o/none is not found on GitHub" ), qPrintable( told.failures.first() ) );
}

/*-------------------------------------------------------------------*/
//! GitHub gave a part of the tree: the part, with a note
void
TestGitHubLogSource::truncated()
{
    HttpTestServer server;
    QByteArray json = treeJson();
    json.replace( "\"truncated\": false", "\"truncated\": true" );
    server.route( TREE_PATH, HttpTestServer::ok( json ) );
    GitHubLogSource::setApiBase( server.url( "" ) );
    QTemporaryDir cache;
    GitHubLogSource source( "o", "r", "", cache.path() );
    Told told( source );
    source.list( QString(), false );
    QVERIFY( told.waitForEnd() );
    QCOMPARE( told.entries.size(), 21 );
    QCOMPARE( told.notes.size(), 1 );
    QVERIFY( told.notes.first().contains( "only a part" ) );
}

/*-------------------------------------------------------------------*/
//! abort(): nothing told, also for a list from the cache
void
TestGitHubLogSource::aborted()
{
    HttpTestServer server;
    HttpTestServer::Response slow = HttpTestServer::ok( treeJson() );
    slow.hang = true;
    server.route( TREE_PATH, slow );
    GitHubLogSource::setApiBase( server.url( "" ) );
    QTemporaryDir cache;
    GitHubLogSource source( "o", "r", "", cache.path() );
    Told told( source );
    source.list( QString(), false );
    QTRY_COMPARE( server.requests( TREE_PATH ), 1 );
    source.abort();
    QTest::qWait( 300 );
    QCOMPARE( told.listed + told.failures.size(), 0 );

    // a cache: told after list() returns, not after abort()
    QVERIFY( QDir().mkpath( QFileInfo( source.cachePath() ).absolutePath() ) );
    QFile file( source.cachePath() );
    QVERIFY( file.open( QIODevice::WriteOnly ) );
    file.write( treeJson() );
    file.close();
    source.list( QString(), false );
    source.abort();
    QTest::qWait( 100 );
    QCOMPARE( told.listed, 0 );
}

QTEST_GUILESS_MAIN( TestGitHubLogSource )
#include "test_github_log_source.moc"
