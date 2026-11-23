// -*-c++-*-

/*!
  \file test_log_cache.cpp
  \brief unit test of LogCache: the cache of the game logs downloaded
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

#include "log_cache.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

/*
  docs/plan/2026-11-22-02_remote-logs.md D3. CTest runs this test with
  XDG_CACHE_HOME in the build tree (test/CMakeLists.txt).
*/

namespace {

//! write a file of a size, making its directory
bool
writeFile( const QString & path,
           const int size )
{
    if ( ! QDir().mkpath( QFileInfo( path ).absolutePath() ) ) return false;
    QFile file( path );
    return file.open( QIODevice::WriteOnly )
        && file.write( QByteArray( size, 'x' ) ) == size;
}

}

/*!
  \brief unit test of LogCache
*/
class TestLogCache
    : public QObject {
    Q_OBJECT

private slots:

    void defaultDir();
    void pathOf();
    void contains();
    void sizeAndClear();
};

/*-------------------------------------------------------------------*/
//! under the cache location: ~/.cache/APP/logs (in the test's cache here)
void
TestLogCache::defaultDir()
{
    const QString dir = LogCache::defaultDir();
    QVERIFY2( dir.endsWith( "/logs" ), qPrintable( dir ) );
    QVERIFY2( dir.startsWith( qEnvironmentVariable( "XDG_CACHE_HOME", QDir::homePath() ) ), qPrintable( dir ) );
    QCOMPARE( LogCache().dir(), dir );
    QCOMPARE( LogCache( "/tmp/x" ).dir(), QString( "/tmp/x" ) );
}

/*-------------------------------------------------------------------*/
/*!
  DIR/KEY/NAME: the same URL the same file, another URL of the same name
  another directory, and a name safe for a file name.
*/
void
TestLogCache::pathOf()
{
    const LogCache cache( "/cache/logs" );
    const QString a = cache.pathOf( QUrl( "https://example.org/a/game.rcg.gz" ) );
    const QString b = cache.pathOf( QUrl( "https://example.org/b/game.rcg.gz" ) );
    QCOMPARE( a, cache.pathOf( QUrl( "https://example.org/a/game.rcg.gz" ) ) );
    QVERIFY( a != b );
    QCOMPARE( QFileInfo( a ).fileName(), QString( "game.rcg.gz" ) );
    QCOMPARE( QFileInfo( b ).fileName(), QString( "game.rcg.gz" ) );
    QCOMPARE( QFileInfo( a ).dir().dirName().size(), 16 );
    QCOMPARE( QFileInfo( QFileInfo( a ).absolutePath() ).absolutePath(), QString( "/cache/logs" ) );

    QCOMPARE( QFileInfo( cache.pathOf( QUrl( "https://example.org/a%2Fb%3Ac.rcg" ) ) ).fileName(),
              QString( "a_b_c.rcg" ) );
    QCOMPARE( QFileInfo( cache.pathOf( QUrl( "https://example.org/" ) ) ).fileName(),
              QString( "example.org" ) );
    QCOMPARE( QFileInfo( cache.pathOf( QUrl( "https://example.org/.." ) ) ).fileName(),
              QString( "game.rcg" ) );
}

/*-------------------------------------------------------------------*/
//! a file at the URL's path (not a directory)
void
TestLogCache::contains()
{
    QTemporaryDir tmp;
    QVERIFY( tmp.isValid() );
    const LogCache cache( tmp.filePath( "logs" ) );
    const QUrl url( "https://example.org/a.rcg" );
    QVERIFY( ! cache.contains( url ) );
    QVERIFY( QDir().mkpath( cache.pathOf( url ) ) ); // a directory there
    QVERIFY( ! cache.contains( url ) );
    QVERIFY( QDir().rmdir( cache.pathOf( url ) ) );
    QVERIFY( writeFile( cache.pathOf( url ), 10 ) );
    QVERIFY( cache.contains( url ) );
    QVERIFY( ! cache.contains( QUrl( "https://example.org/b.rcg" ) ) );
}

/*-------------------------------------------------------------------*/
void
TestLogCache::sizeAndClear()
{
    QTemporaryDir tmp;
    QVERIFY( tmp.isValid() );
    const LogCache cache( tmp.filePath( "logs" ) );
    QCOMPARE( cache.size(), qint64( 0 ) );
    QVERIFY( cache.clear() ); // not there

    const QString a = cache.pathOf( QUrl( "https://example.org/a.rcg" ) );
    const QString b = cache.pathOf( QUrl( "https://example.org/b.rcg" ) );
    QVERIFY( writeFile( a, 1000 ) );
    QVERIFY( writeFile( b, 1000 ) );
    QCOMPARE( cache.size(), qint64( 2000 ) );

    QVERIFY( cache.clear() );
    QVERIFY( ! QFile::exists( a ) );
    QVERIFY( ! QDir( cache.dir() ).exists() );
    QCOMPARE( cache.size(), qint64( 0 ) );
}

QTEST_GUILESS_MAIN( TestLogCache )
#include "test_log_cache.moc"
