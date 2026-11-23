// -*-c++-*-

/*!
  \file test_log_downloader.cpp
  \brief unit test of LogDownloader, with an HTTP server on the local machine
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

#include "http_test_server.h"
#include "log_downloader.h"

#include <QDir>
#include <QFile>
#include <QNetworkProxy>
#include <QTemporaryDir>

/*
  docs/plan/2026-11-22-02_remote-logs.md 6: the downloads from a small HTTP
  server in the test (http_test_server.h); the CI never uses the network
  outside.
*/

namespace {

//! bytes that look like nothing in particular
QByteArray
someBytes( const int size )
{
    QByteArray bytes( size, '\0' );
    for ( int i = 0; i < size; ++i )
    {
        bytes[i] = static_cast< char >( ( i * 131 + i / 7 ) & 0xff );
    }
    return bytes;
}

//! a Git LFS pointer for a file of a size
QByteArray
lfsPointer( const qint64 size )
{
    return "version https://git-lfs.github.com/spec/v1\n"
           "oid sha256:e34fc2431000f093ce95a9681d8d8e0ebb551296c62cf152345cb68a78e3a0c7\n"
           "size " + QByteArray::number( size ) + "\n";
}

//! the content of a file, or a null array
QByteArray
contentOf( const QString & path )
{
    QFile file( path );
    return file.open( QIODevice::ReadOnly ) ? file.readAll() : QByteArray();
}

/*!
  \struct Fixture
  \brief a server, a directory, and a downloader with its signals recorded
*/
struct Fixture {
    HttpTestServer server;
    QTemporaryDir dir;
    LogDownloader downloader;
    QSignalSpy finished;
    QSignalSpy failed;
    QSignalSpy progress;

    Fixture()
        : finished( &downloader, SIGNAL( finished( const QString & ) ) ),
          failed( &downloader, SIGNAL( failed( const QString & ) ) ),
          progress( &downloader, SIGNAL( progress( qint64, qint64 ) ) )
      { }

    QString path( const QString & name = QStringLiteral( "key/game.rcg.gz" ) ) const
      {
          return dir.filePath( name );
      }

    //! wait for finished() or failed()
    bool waitForEnd( const int ms = 10000 )
      {
          return QTest::qWaitFor( [this]() { return finished.count() + failed.count() > 0; }, ms );
      }

    QString failure() const
      {
          return failed.isEmpty() ? QString() : failed.first().first().toString();
      }

    //! the files in the directory of path(), besides it: none should be left
    QStringList filesBeside( const QString & path ) const
      {
          QStringList files = QFileInfo( path ).dir().entryList( QDir::Files | QDir::Hidden );
          files.removeAll( QFileInfo( path ).fileName() );
          return files;
      }
};

}

/*!
  \brief unit test of LogDownloader
*/
class TestLogDownloader
    : public QObject {
    Q_OBJECT

private slots:

    void initTestCase()
      {
          // the server is on this machine, whatever the environment's proxy
          QNetworkProxy::setApplicationProxy( QNetworkProxy::NoProxy );
      }

    void success();
    void replaced();
    void redirect();
    void notFound();
    void refused();
    void aborted();
    void restarted();
    void tooLargeByLength();
    void tooLargeWhileReading();
    void lfs();
    void lfsTooLarge();
    void lfsWithoutResolver();
    void timeout();
    void unwritable();
};

/*-------------------------------------------------------------------*/
/*!
  The bytes at the path, the progress to the end, the User-Agent, and no
  other file left (QSaveFile).
*/
void
TestLogDownloader::success()
{
    Fixture f;
    QVERIFY( f.server.isListening() );
    QCOMPARE( LogDownloader::MAX_BYTES, qint64( 200 ) * 1024 * 1024 );

    const QByteArray body = someBytes( 300 * 1024 );
    HttpTestServer::Response r = HttpTestServer::ok( body );
    r.split_delay = 50; // in two parts: the progress more than once
    f.server.route( "/logs/game.rcg.gz", r );

    f.downloader.start( f.server.url( "/logs/game.rcg.gz" ), f.path() );
    QVERIFY( f.downloader.isRunning() );
    QCOMPARE( f.downloader.url(), f.server.url( "/logs/game.rcg.gz" ) );
    QVERIFY( f.finished.isEmpty() ); // not before start() returns
    QVERIFY( f.waitForEnd() );
    QVERIFY2( f.failed.isEmpty(), qPrintable( f.failure() ) );
    QCOMPARE( f.finished.count(), 1 );
    QCOMPARE( f.finished.first().first().toString(), f.path() );
    QVERIFY( ! f.downloader.isRunning() );

    QCOMPARE( contentOf( f.path() ), body );
    QCOMPARE( f.filesBeside( f.path() ), QStringList() );

    QVERIFY( f.progress.count() >= 2 );
    const QList< QVariant > last = f.progress.last();
    QCOMPARE( last.at( 0 ).toLongLong(), qint64( body.size() ) );
    QCOMPARE( last.at( 1 ).toLongLong(), qint64( body.size() ) );

    QVERIFY2( f.server.userAgent().startsWith( "rcssmonitor/" ), f.server.userAgent().constData() );
}

/*-------------------------------------------------------------------*/
//! a file at the path is replaced
void
TestLogDownloader::replaced()
{
    Fixture f;
    QVERIFY( QDir().mkpath( QFileInfo( f.path() ).absolutePath() ) );
    {
        QFile old( f.path() );
        QVERIFY( old.open( QIODevice::WriteOnly ) );
        old.write( "old content, longer than the new one" );
    }
    f.server.route( "/game.rcg", HttpTestServer::ok( "new" ) );
    f.downloader.start( f.server.url( "/game.rcg" ), f.path() );
    QVERIFY( f.waitForEnd() );
    QCOMPARE( f.finished.count(), 1 );
    QCOMPARE( contentOf( f.path() ), QByteArray( "new" ) );
}

/*-------------------------------------------------------------------*/
//! followed (http to http)
void
TestLogDownloader::redirect()
{
    Fixture f;
    f.server.route( "/github/raw/main/game.rcg", HttpTestServer::redirect( "/raw/main/game.rcg" ) );
    f.server.route( "/raw/main/game.rcg", HttpTestServer::ok( someBytes( 5000 ) ) );
    f.downloader.start( f.server.url( "/github/raw/main/game.rcg" ), f.path() );
    QVERIFY( f.waitForEnd() );
    QVERIFY2( f.failed.isEmpty(), qPrintable( f.failure() ) );
    QCOMPARE( contentOf( f.path() ), someBytes( 5000 ) );
    QCOMPARE( f.server.requests( "/raw/main/game.rcg" ), 1 );
}

/*-------------------------------------------------------------------*/
//! the status and the reason; nothing left
void
TestLogDownloader::notFound()
{
    Fixture f;
    f.downloader.start( f.server.url( "/none.rcg" ), f.path() );
    QVERIFY( f.waitForEnd() );
    QCOMPARE( f.finished.count(), 0 );
    QCOMPARE( f.failure(), QString( "HTTP 404 Not Found" ) );
    QVERIFY( ! QFile::exists( f.path() ) );
    QCOMPARE( f.filesBeside( f.path() ), QStringList() );
}

/*-------------------------------------------------------------------*/
//! no server: the network's error
void
TestLogDownloader::refused()
{
    QTcpServer closed;
    QVERIFY( closed.listen( QHostAddress::LocalHost, 0 ) );
    const quint16 port = closed.serverPort();
    closed.close();

    Fixture f;
    f.downloader.start( QUrl( QStringLiteral( "http://127.0.0.1:%1/game.rcg" ).arg( port ) ), f.path() );
    QVERIFY( f.waitForEnd() );
    QCOMPARE( f.finished.count(), 0 );
    QVERIFY( ! f.failure().isEmpty() );
    QVERIFY( ! QFile::exists( f.path() ) );
}

/*-------------------------------------------------------------------*/
//! abort(): no signal, nothing left
void
TestLogDownloader::aborted()
{
    Fixture f;
    HttpTestServer::Response r = HttpTestServer::ok( someBytes( 100000 ) );
    r.hang = true;
    f.server.route( "/slow.rcg", r );
    f.downloader.start( f.server.url( "/slow.rcg" ), f.path() );
    QTRY_VERIFY( f.progress.count() > 0 );
    f.downloader.abort();
    QVERIFY( ! f.downloader.isRunning() );
    QTest::qWait( 300 );
    QCOMPARE( f.finished.count(), 0 );
    QCOMPARE( f.failed.count(), 0 );
    QVERIFY( ! QFile::exists( f.path() ) );
    QCOMPARE( f.filesBeside( f.path() ), QStringList() );
}

/*-------------------------------------------------------------------*/
//! start() again stops the download going on: only the new one ends
void
TestLogDownloader::restarted()
{
    Fixture f;
    HttpTestServer::Response r = HttpTestServer::ok( someBytes( 100000 ) );
    r.hang = true;
    f.server.route( "/slow.rcg", r );
    f.server.route( "/fast.rcg", HttpTestServer::ok( "fast" ) );
    f.downloader.start( f.server.url( "/slow.rcg" ), f.path( "a/slow.rcg" ) );
    f.downloader.start( f.server.url( "/fast.rcg" ), f.path( "b/fast.rcg" ) );
    QVERIFY( f.waitForEnd() );
    QTest::qWait( 200 );
    QCOMPARE( f.finished.count(), 1 );
    QCOMPARE( f.finished.first().first().toString(), f.path( "b/fast.rcg" ) );
    QCOMPARE( f.failed.count(), 0 );
    QVERIFY( ! QFile::exists( f.path( "a/slow.rcg" ) ) );

    // also a failure told later (an https URL without TLS, a path not
    // writable) is not told after start() again
    f.finished.clear();
    f.downloader.start( QUrl( "http://127.0.0.1:1/x.rcg" ), QStringLiteral( "/proc/no/such/dir/x.rcg" ) );
    f.downloader.start( f.server.url( "/fast.rcg" ), f.path( "c/fast.rcg" ) );
    QVERIFY( f.waitForEnd() );
    QTest::qWait( 100 );
    QCOMPARE( f.finished.count(), 1 );
    QCOMPARE( f.failed.count(), 0 );
}

/*-------------------------------------------------------------------*/
//! the server tells a length over the limit: failed before the body
void
TestLogDownloader::tooLargeByLength()
{
    Fixture f;
    f.downloader.setMaxBytes( 1000 );
    QCOMPARE( f.downloader.maxBytes(), qint64( 1000 ) );
    HttpTestServer::Response r = HttpTestServer::ok( someBytes( 5000 ) );
    r.hang = true; // the body never ends: only the length tells
    f.server.route( "/big.rcg", r );
    f.downloader.start( f.server.url( "/big.rcg" ), f.path() );
    QVERIFY( f.waitForEnd() );
    QCOMPARE( f.finished.count(), 0 );
    QVERIFY2( f.failure().contains( "larger than the limit" ), qPrintable( f.failure() ) );
    QVERIFY( ! QFile::exists( f.path() ) );
    QCOMPARE( f.filesBeside( f.path() ), QStringList() );
}

/*-------------------------------------------------------------------*/
//! no length: failed when that much has come
void
TestLogDownloader::tooLargeWhileReading()
{
    Fixture f;
    f.downloader.setMaxBytes( 1000 );
    HttpTestServer::Response r = HttpTestServer::ok( someBytes( 5000 ) );
    r.content_length = false;
    f.server.route( "/big.rcg", r );
    f.downloader.start( f.server.url( "/big.rcg" ), f.path() );
    QVERIFY( f.waitForEnd() );
    QCOMPARE( f.finished.count(), 0 );
    QVERIFY2( f.failure().contains( "larger than the limit" ), qPrintable( f.failure() ) );
    QVERIFY( ! QFile::exists( f.path() ) );

    // the limit itself is taken
    Fixture g;
    g.downloader.setMaxBytes( 5000 );
    g.server.route( "/big.rcg", r );
    g.downloader.start( g.server.url( "/big.rcg" ), g.path() );
    QVERIFY( g.waitForEnd() );
    QVERIFY2( g.failed.isEmpty(), qPrintable( g.failure() ) );
    QCOMPARE( contentOf( g.path() ), someBytes( 5000 ) );
}

/*-------------------------------------------------------------------*/
/*!
  A Git LFS pointer: the file it stands for, from where the resolver tells,
  with its size as the total of the progress.
*/
void
TestLogDownloader::lfs()
{
    Fixture f;
    const QByteArray body = someBytes( 40000 );
    f.server.route( "/raw/o/r/main/game.rcg", HttpTestServer::ok( lfsPointer( body.size() ) ) );
    HttpTestServer::Response media = HttpTestServer::ok( body );
    media.content_length = false; // only the pointer tells the size
    f.server.route( "/media/o/r/main/game.rcg", media );

    QUrl from;
    HttpTestServer & server = f.server;
    f.downloader.setLfsResolver( [&from, &server]( const QUrl & url ) {
        from = url;
        return server.url( "/media" + url.path().mid( 4 ).toUtf8() );
    } );
    f.downloader.start( f.server.url( "/raw/o/r/main/game.rcg" ), f.path() );
    QVERIFY( f.waitForEnd() );
    QVERIFY2( f.failed.isEmpty(), qPrintable( f.failure() ) );
    QCOMPARE( from, f.server.url( "/raw/o/r/main/game.rcg" ) );
    QCOMPARE( f.server.requests( "/media/o/r/main/game.rcg" ), 1 );
    QCOMPARE( contentOf( f.path() ), body );
    QCOMPARE( f.progress.last().at( 1 ).toLongLong(), qint64( body.size() ) );
    QCOMPARE( f.filesBeside( f.path() ), QStringList() );
}

/*-------------------------------------------------------------------*/
//! a pointer to a file over the limit: failed without reading it
void
TestLogDownloader::lfsTooLarge()
{
    Fixture f;
    f.downloader.setMaxBytes( 1000 );
    f.server.route( "/raw/game.rcg", HttpTestServer::ok( lfsPointer( 5000 ) ) );
    HttpTestServer & server = f.server;
    f.downloader.setLfsResolver( [&server]( const QUrl & ) { return server.url( "/media/game.rcg" ); } );
    f.downloader.start( f.server.url( "/raw/game.rcg" ), f.path() );
    QVERIFY( f.waitForEnd() );
    QVERIFY2( f.failure().contains( "larger than the limit" ), qPrintable( f.failure() ) );
    QCOMPARE( f.server.requests( "/media/game.rcg" ), 0 );
    QVERIFY( ! QFile::exists( f.path() ) );
}

/*-------------------------------------------------------------------*/
//! a pointer not on raw.githubusercontent.com (githubLfsUrl()): failed
void
TestLogDownloader::lfsWithoutResolver()
{
    Fixture f;
    f.server.route( "/game.rcg", HttpTestServer::ok( lfsPointer( 1234 ) ) );
    f.downloader.start( f.server.url( "/game.rcg" ), f.path() );
    QVERIFY( f.waitForEnd() );
    QVERIFY2( f.failure().contains( "Git LFS pointer" ), qPrintable( f.failure() ) );
    QVERIFY( ! QFile::exists( f.path() ) );
}

/*-------------------------------------------------------------------*/
//! no data for the transfer timeout: failed, telling it
void
TestLogDownloader::timeout()
{
    Fixture f;
    QCOMPARE( LogDownloader::TRANSFER_TIMEOUT, 30000 );
    f.downloader.setTransferTimeout( 500 );
    HttpTestServer::Response r = HttpTestServer::ok( someBytes( 10000 ) );
    r.hang = true;
    f.server.route( "/stuck.rcg", r );
    f.downloader.start( f.server.url( "/stuck.rcg" ), f.path() );
    QVERIFY( f.waitForEnd( 5000 ) );
    QCOMPARE( f.finished.count(), 0 );
    QVERIFY2( f.failure().contains( "No data came for 0.5 seconds" ), qPrintable( f.failure() ) );
    QVERIFY( ! QFile::exists( f.path() ) );
}

/*-------------------------------------------------------------------*/
//! a path that cannot be written: failed, after start() returns
void
TestLogDownloader::unwritable()
{
    Fixture f;
    f.server.route( "/game.rcg", HttpTestServer::ok( "x" ) );
    QFile file( f.dir.filePath( "a_file" ) );
    QVERIFY( file.open( QIODevice::WriteOnly ) );
    file.close();
    f.downloader.start( f.server.url( "/game.rcg" ), f.dir.filePath( "a_file/game.rcg" ) );
    QVERIFY( f.failed.isEmpty() );
    QVERIFY( f.waitForEnd() );
    QVERIFY2( f.failure().contains( "Cannot write" ), qPrintable( f.failure() ) );
    QCOMPARE( f.server.requests( "/game.rcg" ), 0 );
}

QTEST_GUILESS_MAIN( TestLogDownloader )
#include "test_log_downloader.moc"
