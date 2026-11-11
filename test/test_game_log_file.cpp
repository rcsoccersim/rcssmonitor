// -*-c++-*-

/*!
  \file test_game_log_file.cpp
  \brief unit test of readGameLogFile() and loadGameLogFile()
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
#include <QTemporaryDir>

#include "disp_holder.h"
#include "game_log_file.h"
#include "rcg_handler.h"

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#include <fstream>
#include <iterator>
#include <string>

namespace {

std::string
dataPath( const char * name )
{
    return std::string( TEST_DATA_DIR ) + "/" + name;
}

std::string
readPlain( const std::string & path )
{
    std::ifstream fin( path, std::ios::binary );
    return std::string( std::istreambuf_iterator< char >( fin ),
                        std::istreambuf_iterator< char >() );
}

#ifdef HAVE_LIBZ
/*!
  \brief write data as a gzip file.
*/
bool
writeGzip( const std::string & path,
           const std::string & data )
{
    gzFile file = gzopen( path.c_str(), "wb" );
    if ( ! file )
    {
        return false;
    }
    const int n = gzwrite( file, data.data(), static_cast< unsigned int >( data.size() ) );
    gzclose( file );
    return n == static_cast< int >( data.size() );
}

/*!
  \brief write data as a file, as they are.
*/
bool
writePlain( const std::string & path,
            const std::string & data )
{
    std::ofstream fout( path, std::ios::binary );
    fout.write( data.data(), static_cast< std::streamsize >( data.size() ) );
    return static_cast< bool >( fout );
}

/*!
  \brief compress data as gzip, in memory.
*/
std::string
gzipBytes( const QTemporaryDir & dir,
           const std::string & data )
{
    const std::string path = dir.filePath( "bytes.gz" ).toStdString();
    if ( ! writeGzip( path, data ) )
    {
        return std::string();
    }
    return readPlain( path );
}

/*!
  \brief data larger than the 1 MB chunk of zlib.
*/
std::string
largeLog()
{
    std::string data;
    const std::string one = readPlain( dataPath( "head_json.rcg" ) );
    while ( data.size() < 3 * 1024 * 1024 )
    {
        data += one;
    }
    return data;
}
#endif

}

/*!
  \brief unit test of readGameLogFile() and loadGameLogFile()
*/
class TestGameLogFile
    : public QObject {
    Q_OBJECT

private slots:

    void readPlainFile();
    void readGzipFile();
    void readConcatenatedGzipFile();
    void readTruncatedGzipFile();
    void readGzipFileWithTrailingBytes();
    void readMissingFile();
    void loadGzipFile();
};

/*-------------------------------------------------------------------*/
/*!
  A file that is not compressed is read as it is.
*/
void
TestGameLogFile::readPlainFile()
{
    const std::string path = dataPath( "head_json.rcg" );
    std::string data;
    QVERIFY( readGameLogFile( path, &data ) );
    QCOMPARE( data, readPlain( path ) );
}

/*-------------------------------------------------------------------*/
/*!
  A gzip file is decompressed, also when it is larger than the 1 MB chunk
  that the reader uses.
*/
void
TestGameLogFile::readGzipFile()
{
#ifdef HAVE_LIBZ
    QTemporaryDir dir;
    QVERIFY( dir.isValid() );

    const std::string original = largeLog();
    const std::string path = dir.filePath( "large.rcg.gz" ).toStdString();
    QVERIFY( writeGzip( path, original ) );

    std::string data;
    QVERIFY( readGameLogFile( path, &data ) );
    QCOMPARE( data.size(), original.size() );
    QVERIFY( data == original );
#else
    QSKIP( "built without zlib" );
#endif
}

/*-------------------------------------------------------------------*/
/*!
  A gzip file of more than one member is read to the end. The last member
  is small, so its size in the trailer is smaller than the data.
*/
void
TestGameLogFile::readConcatenatedGzipFile()
{
#ifdef HAVE_LIBZ
    QTemporaryDir dir;
    QVERIFY( dir.isValid() );

    const std::string first = largeLog();
    const std::string second = readPlain( dataPath( "head_v5.rcg" ) );
    const std::string path = dir.filePath( "concatenated.rcg.gz" ).toStdString();
    QVERIFY( writePlain( path, gzipBytes( dir, first ) + gzipBytes( dir, second ) ) );

    std::string data;
    QVERIFY( readGameLogFile( path, &data ) );
    QCOMPARE( data.size(), first.size() + second.size() );
    QVERIFY( data == first + second );
#else
    QSKIP( "built without zlib" );
#endif
}

/*-------------------------------------------------------------------*/
/*!
  A gzip file that ends in the middle (the server stopped abnormally) is
  read up to there.
*/
void
TestGameLogFile::readTruncatedGzipFile()
{
#ifdef HAVE_LIBZ
    QTemporaryDir dir;
    QVERIFY( dir.isValid() );

    const std::string original = largeLog();
    const std::string compressed = gzipBytes( dir, original );
    const std::string path = dir.filePath( "truncated.rcg.gz" ).toStdString();
    QVERIFY( writePlain( path, compressed.substr( 0, compressed.size() / 2 ) ) );

    std::string data;
    QVERIFY( readGameLogFile( path, &data ) );
    QVERIFY( data.size() > original.size() / 4 );
    QVERIFY( data.size() < original.size() );
    QVERIFY( data == original.substr( 0, data.size() ) );
#else
    QSKIP( "built without zlib" );
#endif
}

/*-------------------------------------------------------------------*/
/*!
  Bytes that are not gzip after the gzip data are ignored, as zlib does.
*/
void
TestGameLogFile::readGzipFileWithTrailingBytes()
{
#ifdef HAVE_LIBZ
    QTemporaryDir dir;
    QVERIFY( dir.isValid() );

    const std::string original = readPlain( dataPath( "head_v5.rcg" ) );
    const std::string path = dir.filePath( "trailing.rcg.gz" ).toStdString();
    QVERIFY( writePlain( path, gzipBytes( dir, original ) + std::string( 100, '\0' ) ) );

    std::string data;
    QVERIFY( readGameLogFile( path, &data ) );
    QVERIFY( data == original );
#else
    QSKIP( "built without zlib" );
#endif
}

/*-------------------------------------------------------------------*/
/*!
  A file that does not exist fails.
*/
void
TestGameLogFile::readMissingFile()
{
    std::string data = "something";
    QVERIFY( ! readGameLogFile( dataPath( "no_such_file.rcg" ), &data ) );
    QVERIFY( data.empty() );
}

/*-------------------------------------------------------------------*/
/*!
  A gzip file is read and parsed, as the application opens a game log.
*/
void
TestGameLogFile::loadGzipFile()
{
#ifdef HAVE_LIBZ
    QTemporaryDir dir;
    QVERIFY( dir.isValid() );

    const std::string path = dir.filePath( "head_v5.rcg.gz" ).toStdString();
    QVERIFY( writeGzip( path, readPlain( dataPath( "head_v5.rcg" ) ) ) );

    DispHolder holder;
    RCGHandler handler( nullptr, holder );
    QVERIFY( loadGameLogFile( path, handler ) );
    QCOMPARE( holder.dispCont().size(), size_t( 3 ) );
    QCOMPARE( holder.dispCont().back()->show_.time_, rcss::rcg::UInt32( 4 ) );
#else
    QSKIP( "built without zlib" );
#endif
}

QTEST_MAIN( TestGameLogFile )

#include "test_game_log_file.moc"
