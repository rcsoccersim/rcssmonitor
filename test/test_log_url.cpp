// -*-c++-*-

/*!
  \file test_log_url.cpp
  \brief unit test of LogUrl: the URLs of game logs
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

#include "log_url.h"


/*
  Ported from test/unit/fetch-log.test.ts of rcssmonitor-web
  (docs/plan/2026-11-22-02_remote-logs.md 6).
*/

namespace {

const QString LOG = QStringLiteral( "logs/Finals/202607050404-HELIOS2026_0-vs-YuShan2026_1.rcg.gz" );

//! LogUrl::parse() of a text: the URL, or the error
QString
parsed( const QString & text )
{
    QString error;
    const QUrl url = LogUrl::parse( text, &error );
    return url.isValid() ? url.toString( QUrl::FullyEncoded ) : QStringLiteral( "error: " ) + error;
}

//! LogUrl::githubRawUrl(), or "none"
QString
raw( const QString & url )
{
    const QUrl r = LogUrl::githubRawUrl( QUrl( url ) );
    return r.isValid() ? r.toString( QUrl::FullyEncoded ) : QStringLiteral( "none" );
}

//! a Git LFS pointer for a file of a size
QByteArray
lfsPointer( const qint64 size )
{
    return "version https://git-lfs.github.com/spec/v1\n"
           "oid sha256:e34fc2431000f093ce95a9681d8d8e0ebb551296c62cf152345cb68a78e3a0c7\n"
           "size " + QByteArray::number( size ) + "\n";
}

}

/*!
  \brief unit test of LogUrl
*/
class TestLogUrl
    : public QObject {
    Q_OBJECT

private slots:

    void isUrl_data();
    void isUrl();
    void parse();
    void refused();
    void githubRaw();
    void githubOthers();
    void githubLfs();
    void lfsPointer();
    void name();
};

/*-------------------------------------------------------------------*/
void
TestLogUrl::isUrl_data()
{
    QTest::addColumn< QString >( "text" );
    QTest::addColumn< bool >( "url" );

    QTest::newRow( "https" ) << "https://example.org/game.rcg.gz" << true;
    QTest::newRow( "http" ) << "http://192.168.0.10:8000/game.rcg" << true;
    QTest::newRow( "upper case" ) << "HTTPS://example.org/game.rcg" << true;
    QTest::newRow( "spaces" ) << "  https://example.org/game.rcg " << true;
    QTest::newRow( "a path" ) << "/home/user/logs/game.rcg.gz" << false;
    QTest::newRow( "a relative path" ) << "game.rcg" << false;
    QTest::newRow( "a path named http" ) << "http/game.rcg" << false;
    QTest::newRow( "file" ) << "file:///home/user/game.rcg" << false;
    QTest::newRow( "ftp" ) << "ftp://example.org/game.rcg" << false;
}

/*-------------------------------------------------------------------*/
/*!
  The command line and a drop: an http:// or https:// URL is downloaded,
  anything else is a local path.
*/
void
TestLogUrl::isUrl()
{
    QFETCH( QString, text );
    QFETCH( bool, url );
    QCOMPARE( LogUrl::isUrl( text ), url );
}

/*-------------------------------------------------------------------*/
void
TestLogUrl::parse()
{
    QCOMPARE( parsed( " https://logs.example.org/a/game.rcg.gz " ),
              QString( "https://logs.example.org/a/game.rcg.gz" ) );
    QCOMPARE( parsed( "http://192.168.0.10:8000/game.rcg" ),
              QString( "http://192.168.0.10:8000/game.rcg" ) );
    QCOMPARE( parsed( "https://logs.example.org/game.rcg.gz?x=1" ),
              QString( "https://logs.example.org/game.rcg.gz?x=1" ) );
    QCOMPARE( parsed( "HTTPS://Logs.Example.org/game.rcg" ),
              QString( "https://logs.example.org/game.rcg" ) );
}

/*-------------------------------------------------------------------*/
/*!
  Not opened: nothing, not a URL, another scheme, no host, a user name or a
  password (the error tells which).
*/
void
TestLogUrl::refused()
{
    QCOMPARE( parsed( "" ), QString( "error: No URL is given." ) );
    QCOMPARE( parsed( "   " ), QString( "error: No URL is given." ) );
    QVERIFY( parsed( "http://" ).startsWith( "error: Not a URL" ) );
    QVERIFY( parsed( "game.rcg" ).startsWith( "error: Not a URL" ) );
    QVERIFY( parsed( "/home/user/game.rcg" ).startsWith( "error: Not a URL" ) );
    QVERIFY( parsed( "javascript:alert(1)" ).startsWith( "error: Not an http:// or https:// URL" ) );
    QVERIFY( parsed( "ftp://example.org/game.rcg" ).startsWith( "error: Not an http://" ) );
    QVERIFY( parsed( "file:///home/user/game.rcg" ).startsWith( "error: Not an http://" ) );
    QVERIFY( parsed( "data:,abc" ).startsWith( "error: Not an http://" ) );
    QVERIFY( parsed( "https://user:pw@example.org/game.rcg" ).contains( "user name or a password" ) );
    QVERIFY( parsed( "https://user@example.org/game.rcg" ).contains( "user name or a password" ) );
    QVERIFY( parsed( "https://exa mple.org/game.rcg" ).startsWith( "error: " ) );
}

/*-------------------------------------------------------------------*/
/*!
  A GitHub file page or raw link is read from raw.githubusercontent.com.
*/
void
TestLogUrl::githubRaw()
{
    const QString expected = "https://raw.githubusercontent.com/rcsoccersim/RoboCup2026/main/" + LOG;
    QCOMPARE( raw( "https://github.com/rcsoccersim/RoboCup2026/blob/main/" + LOG ), expected );
    QCOMPARE( raw( "https://github.com/rcsoccersim/RoboCup2026/raw/main/" + LOG ), expected );
    QCOMPARE( raw( "https://www.github.com/rcsoccersim/RoboCup2026/blob/main/" + LOG ), expected );
    // the page's query and fragment are not part of the file
    QCOMPARE( raw( "https://github.com/rcsoccersim/RoboCup2026/blob/main/" + LOG + "?raw=true#L1" ), expected );
    // branches with slashes and full refs stay as they are, as GitHub's raw link does
    QCOMPARE( raw( "https://github.com/o/r/blob/feature/x/a%20b.rcg" ),
              QString( "https://raw.githubusercontent.com/o/r/feature/x/a%20b.rcg" ) );
    QCOMPARE( raw( "https://github.com/o/r/blob/refs/heads/main/game.rcg" ),
              QString( "https://raw.githubusercontent.com/o/r/refs/heads/main/game.rcg" ) );

    // when parsing, also from http://
    QCOMPARE( parsed( "http://github.com/rcsoccersim/RoboCup2026/blob/main/" + LOG ), expected );
    QCOMPARE( parsed( "https://github.com/rcsoccersim/RoboCup2026/blob/HEAD/" + LOG ),
              "https://raw.githubusercontent.com/rcsoccersim/RoboCup2026/HEAD/" + LOG );
}

/*-------------------------------------------------------------------*/
void
TestLogUrl::githubOthers()
{
    QCOMPARE( raw( "https://github.com/rcsoccersim/RoboCup2026" ), QString( "none" ) );
    QCOMPARE( raw( "https://github.com/rcsoccersim/RoboCup2026/tree/main/logs" ), QString( "none" ) );
    QCOMPARE( raw( "https://github.com/o/r/blob/" ), QString( "none" ) );
    QCOMPARE( raw( "https://raw.githubusercontent.com/o/r/main/game.rcg" ), QString( "none" ) );
    QCOMPARE( raw( "https://example.org/o/r/blob/main/game.rcg" ), QString( "none" ) );
    // a raw file stays as it is when parsing
    QCOMPARE( parsed( "https://raw.githubusercontent.com/o/r/main/game.rcg" ),
              QString( "https://raw.githubusercontent.com/o/r/main/game.rcg" ) );
}

/*-------------------------------------------------------------------*/
void
TestLogUrl::githubLfs()
{
    QCOMPARE( LogUrl::githubLfsUrl( QUrl( "https://raw.githubusercontent.com/o/r/main/a/game.rcg" ) ).toString(),
              QString( "https://media.githubusercontent.com/media/o/r/main/a/game.rcg" ) );
    QCOMPARE( LogUrl::githubLfsUrl( QUrl( "https://raw.githubusercontent.com/o/r/main/a%20b.rcg" ) ).toString( QUrl::FullyEncoded ),
              QString( "https://media.githubusercontent.com/media/o/r/main/a%20b.rcg" ) );
    QVERIFY( ! LogUrl::githubLfsUrl( QUrl( "https://example.org/o/r/main/game.rcg" ) ).isValid() );
    QVERIFY( ! LogUrl::githubLfsUrl( QUrl( "http://127.0.0.1:8000/game.rcg" ) ).isValid() );
}

/*-------------------------------------------------------------------*/
void
TestLogUrl::lfsPointer()
{
    QCOMPARE( LogUrl::lfsPointerSize( ::lfsPointer( 870464 ) ), qint64( 870464 ) );
    QCOMPARE( LogUrl::lfsPointerSize( ::lfsPointer( 0 ) ), qint64( 0 ) );

    // anything else
    QCOMPARE( LogUrl::lfsPointerSize( QByteArray( "\x1f\x8b\x08\x00", 4 ) ), qint64( -1 ) );
    QCOMPARE( LogUrl::lfsPointerSize( "ULG\n" ), qint64( -1 ) );
    QCOMPARE( LogUrl::lfsPointerSize( QByteArray() ), qint64( -1 ) );
    QCOMPARE( LogUrl::lfsPointerSize( "version https://git-lfs.github.com/spec/v1\noid sha256:00\n" ), qint64( -1 ) );
    // a pointer is under 1024 bytes: a log that starts like one is not one
    QByteArray long_one = ::lfsPointer( 10 );
    long_one.append( QByteArray( 2048, 'x' ) );
    QCOMPARE( LogUrl::lfsPointerSize( long_one ), qint64( -1 ) );
}

/*-------------------------------------------------------------------*/
void
TestLogUrl::name()
{
    QCOMPARE( LogUrl::name( QUrl( "https://example.org/logs/game.rcg.gz?raw=1#x" ) ), QString( "game.rcg.gz" ) );
    QCOMPARE( LogUrl::name( QUrl( "https://example.org/logs/HELIOS%20vs%20CYRUS.rcg" ) ), QString( "HELIOS vs CYRUS.rcg" ) );
    QCOMPARE( LogUrl::name( QUrl( "https://example.org/logs/" ) ), QString( "logs" ) );
    QCOMPARE( LogUrl::name( QUrl( "https://example.org/logs/%E8%A9%A6%E5%90%88.rcg" ) ), QString::fromUtf8( "試合.rcg" ) );
    // the host, or the raw text if it is not UTF-8
    QCOMPARE( LogUrl::name( QUrl( "https://example.org:8080/" ) ), QString( "example.org:8080" ) );
    QCOMPARE( LogUrl::name( QUrl( "https://example.org" ) ), QString( "example.org" ) );
    QCOMPARE( LogUrl::name( QUrl( "https://example.org/%E0%A4.rcg" ) ), QString( "%E0%A4.rcg" ) );
}

QTEST_GUILESS_MAIN( TestLogUrl )
#include "test_log_url.moc"
