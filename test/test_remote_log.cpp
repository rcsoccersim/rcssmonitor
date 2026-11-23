// -*-c++-*-

/*!
  \file test_remote_log.cpp
  \brief unit test of opening game logs from URLs and by drops, and saving
  them, in the main window (docs/plan/2026-11-22-02_remote-logs.md)
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
#include "log_url.h"
#include "main_window.h"
#include "open_url_dialog.h"
#include "options.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDropEvent>
#include <QFile>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QNetworkProxy>
#include <QProgressDialog>
#include <QPushButton>
#include <QTemporaryDir>

/*
  Options reads and writes ~/.rcssmonitor.conf, and the downloads go to
  the cache in ~/.cache: CTest runs this test with HOME (and
  XDG_CACHE_HOME) in the build tree (test/CMakeLists.txt). The logs are
  served by a small HTTP server in the test.
*/

namespace {

const QString LOG = QStringLiteral( TEST_DATA_DIR "/head_v5.rcg" );

QByteArray
contentOf( const QString & path )
{
    QFile file( path );
    return file.open( QIODevice::ReadOnly ) ? file.readAll() : QByteArray();
}

//! the window's title has a name (a log is open)
bool
waitForTitle( const MainWindow & window,
              const QString & name )
{
    return QTest::qWaitFor( [&window, &name]() { return window.windowTitle().startsWith( name ); }, 10000 );
}

//! close the message boxes that come in a time, and count them
struct MessageBoxCloser {
    int closed = 0;
    QString text;
    QTimer timer;

    MessageBoxCloser()
      {
          timer.setInterval( 50 );
          QObject::connect( &timer, &QTimer::timeout, [this]() {
              for ( QWidget * w : QApplication::topLevelWidgets() )
              {
                  QMessageBox * box = qobject_cast< QMessageBox * >( w );
                  if ( box && box->isVisible() )
                  {
                      text = box->text();
                      ++closed;
                      box->accept();
                  }
              }
          } );
          timer.start();
      }
};

}

/*!
  \brief unit test of opening game logs from URLs and by drops
*/
class TestRemoteLog
    : public QObject {
    Q_OBJECT

private slots:

    void initTestCase()
      {
          QNetworkProxy::setApplicationProxy( QNetworkProxy::NoProxy );
      }

    void init()
      {
          clearLogCache( MainWindow::gameLogCacheDir() );
          Options::instance().setGameLogFile( std::string() );
      }

    void cacheDir();
    void droppedGameLog();
    void dropFile();
    void openUrl();
    void openUrlInParts();
    void commandLine();
    void cancel();
    void failure();
    void saveCopy();
    void dialog();
    void recentUrls();
};

/*-------------------------------------------------------------------*/
//! under the cache location: ~/.cache/APP/logs (in the test's HOME here)
void
TestRemoteLog::cacheDir()
{
    const QString dir = MainWindow::gameLogCacheDir();
    QVERIFY2( dir.endsWith( "/logs" ), qPrintable( dir ) );
    QVERIFY2( dir.startsWith( qEnvironmentVariable( "XDG_CACHE_HOME", QDir::homePath() ) ), qPrintable( dir ) );
}

/*-------------------------------------------------------------------*/
/*!
  The first URL of the data, a local file or an http(s) URL, or a text that
  is a URL; nothing else.
*/
void
TestRemoteLog::droppedGameLog()
{
    QTemporaryDir tmp;
    QVERIFY( tmp.isValid() );
    {
        QMimeData data;
        data.setUrls( { QUrl::fromLocalFile( LOG ) } );
        QCOMPARE( MainWindow::droppedGameLog( &data ), LOG );
    }
    {
        QMimeData data;
        data.setUrls( { QUrl( "https://example.org/logs/game.rcg.gz" ) } );
        QCOMPARE( MainWindow::droppedGameLog( &data ), QString( "https://example.org/logs/game.rcg.gz" ) );
    }
    {
        // a directory, an ftp URL: skipped for the next
        QMimeData data;
        data.setUrls( { QUrl::fromLocalFile( tmp.path() ), QUrl( "ftp://example.org/a.rcg" ), QUrl::fromLocalFile( LOG ) } );
        QCOMPARE( MainWindow::droppedGameLog( &data ), LOG );
    }
    {
        QMimeData data;
        data.setText( "  http://192.168.0.10:8000/game.rcg \n" );
        QCOMPARE( MainWindow::droppedGameLog( &data ), QString( "http://192.168.0.10:8000/game.rcg" ) );
    }
    {
        QMimeData data;
        data.setText( "a text" );
        QCOMPARE( MainWindow::droppedGameLog( &data ), QString() );
        data.setText( "https://example.org/a.rcg\nhttps://example.org/b.rcg" );
        QCOMPARE( MainWindow::droppedGameLog( &data ), QString() );
    }
    {
        QMimeData data;
        data.setUrls( { QUrl::fromLocalFile( tmp.filePath( "none.rcg" ) ) } );
        QCOMPARE( MainWindow::droppedGameLog( &data ), QString() );
    }
    QCOMPARE( MainWindow::droppedGameLog( nullptr ), QString() );
}

/*-------------------------------------------------------------------*/
//! a file dropped on the window: opened, as File > Open Game Log
void
TestRemoteLog::dropFile()
{
    MainWindow window;
    QVERIFY( window.acceptDrops() );
    QMimeData data;
    data.setUrls( { QUrl::fromLocalFile( LOG ) } );

    QDragEnterEvent enter( QPoint( 10, 10 ), Qt::CopyAction, &data, Qt::LeftButton, Qt::NoModifier );
    QApplication::sendEvent( &window, &enter );
    QVERIFY( enter.isAccepted() );

    QDropEvent drop( QPointF( 10, 10 ), Qt::CopyAction, &data, Qt::LeftButton, Qt::NoModifier );
    QApplication::sendEvent( &window, &drop );
    QVERIFY( drop.isAccepted() );
    QVERIFY( waitForTitle( window, "head_v5.rcg" ) );
    QVERIFY( window.findChild< QAction * >( "save_copy" )->isEnabled() );

    // something else: not taken
    QMimeData text;
    text.setText( "a text" );
    QDragEnterEvent other( QPoint( 10, 10 ), Qt::CopyAction, &text, Qt::LeftButton, Qt::NoModifier );
    QApplication::sendEvent( &window, &other );
    QVERIFY( ! other.isAccepted() );
}

/*-------------------------------------------------------------------*/
/*!
  A URL: downloaded into the cache and opened (the window has the log's
  name); opened again from the cache without a request; the newest of the
  recent URLs; Save a Copy enabled.
*/
void
TestRemoteLog::openUrl()
{
    HttpTestServer server;
    server.route( "/logs/head_v5.rcg", HttpTestServer::ok( contentOf( LOG ) ) );
    const QUrl url = server.url( "/logs/head_v5.rcg" );

    MainWindow window;
    QAction * save_copy = window.findChild< QAction * >( "save_copy" );
    QVERIFY( save_copy );
    QVERIFY( ! save_copy->isEnabled() );
    QVERIFY( window.findChild< QAction * >( "open_url" ) );

    window.openGameLogUrl( url.toString() );
    QVERIFY( waitForTitle( window, "head_v5.rcg" ) );
    QCOMPARE( server.requests( "/logs/head_v5.rcg" ), 1 );
    const QString cached = cachedLogPath( MainWindow::gameLogCacheDir(), url );
    QCOMPARE( contentOf( cached ), contentOf( LOG ) );
    QVERIFY( save_copy->isEnabled() );
    QCOMPARE( Options::instance().recentLogUrls().value( 0 ), url.toString() );

    // from the cache
    window.openGameLogFile( LOG ); // another log first
    window.setWindowTitle( "none" );
    window.openGameLogUrl( "  " + url.toString() + " " );
    QVERIFY( waitForTitle( window, "head_v5.rcg" ) );
    QCOMPARE( server.requests( "/logs/head_v5.rcg" ), 1 );
}

/*-------------------------------------------------------------------*/
/*!
  A download in parts, with the window and the progress dialog shown: the
  progress while more comes, the end coming while the progress is shown
  (it crashed: the dialog's setValue() ran the events, and the end of the
  download closed the dialog under it).
*/
void
TestRemoteLog::openUrlInParts()
{
    HttpTestServer server;
    HttpTestServer::Response r = HttpTestServer::ok( contentOf( LOG ) );
    r.split_delay = 100;
    server.route( "/parts/head_v5.rcg", r );

    MainWindow window;
    window.show();
    QVERIFY( QTest::qWaitForWindowExposed( &window ) );
    for ( int i = 0; i < 3; ++i )
    {
        clearLogCache( MainWindow::gameLogCacheDir() );
        window.setWindowTitle( "none" );
        window.openGameLogUrl( server.url( "/parts/head_v5.rcg" ).toString() );
        QVERIFY( waitForTitle( window, "head_v5.rcg" ) );
    }
    QCOMPARE( server.requests( "/parts/head_v5.rcg" ), 3 );
}

/*-------------------------------------------------------------------*/
//! a URL as the game log of the command line: downloaded and opened at the start
void
TestRemoteLog::commandLine()
{
    HttpTestServer server;
    server.route( "/game/head_v5.rcg", HttpTestServer::ok( contentOf( LOG ) ) );

    MainWindow window;
    Options::instance().setGameLogFile( server.url( "/game/head_v5.rcg" ).toString().toStdString() );
    window.show();
    window.init();
    QVERIFY( waitForTitle( window, "head_v5.rcg" ) );
    QCOMPARE( server.requests( "/game/head_v5.rcg" ), 1 );
    Options::instance().setGameLogFile( std::string() );
}

/*-------------------------------------------------------------------*/
//! Cancel of the progress dialog: stopped, nothing kept, the log open stays
void
TestRemoteLog::cancel()
{
    HttpTestServer server;
    HttpTestServer::Response r = HttpTestServer::ok( QByteArray( 100000, 'x' ) );
    r.hang = true;
    server.route( "/slow.rcg", r );

    MainWindow window;
    window.show();
    window.openGameLogFile( LOG );
    QVERIFY( waitForTitle( window, "head_v5.rcg" ) );

    window.openGameLogUrl( server.url( "/slow.rcg" ).toString() );
    QPointer< QProgressDialog > progress;
    QTRY_VERIFY( ( progress = window.findChild< QProgressDialog * >() ) != nullptr );
    QTRY_VERIFY( progress->isVisible() );
    QTRY_VERIFY( progress->labelText().contains( "slow.rcg" ) );
    QTRY_VERIFY( progress->labelText().contains( "of 97.7 KB" ) ); // the progress, of the length told
    progress->cancel();
    QTRY_VERIFY( ! progress || ! progress->isVisible() ); // deleted later

    QVERIFY( ! QFile::exists( cachedLogPath( MainWindow::gameLogCacheDir(), server.url( "/slow.rcg" ) ) ) );
    QVERIFY( window.windowTitle().startsWith( "head_v5.rcg" ) );
    QVERIFY( window.findChild< QAction * >( "save_copy" )->isEnabled() );
}

/*-------------------------------------------------------------------*/
//! a download that fails, and a text that is not a URL: told in a message box
void
TestRemoteLog::failure()
{
    HttpTestServer server;
    MainWindow window;
    window.show();

    {
        MessageBoxCloser closer;
        window.openGameLogUrl( server.url( "/none.rcg" ).toString() );
        QTRY_COMPARE( closer.closed, 1 );
        QVERIFY2( closer.text.contains( "HTTP 404 Not Found" ), qPrintable( closer.text ) );
        QVERIFY2( closer.text.contains( "/none.rcg" ), qPrintable( closer.text ) );
        QProgressDialog * progress = window.findChild< QProgressDialog * >();
        QVERIFY( ! progress || ! progress->isVisible() );
    }
    {
        MessageBoxCloser closer;
        window.openGameLogUrl( "ftp://example.org/game.rcg" );
        QCOMPARE( closer.closed, 1 ); // a modal box, closed by the timer
        QVERIFY2( closer.text.contains( "Not an http://" ), qPrintable( closer.text ) );
    }
}

/*-------------------------------------------------------------------*/
//! the file of the log open, copied as it is; nothing without a log
void
TestRemoteLog::saveCopy()
{
    QTemporaryDir tmp;
    QVERIFY( tmp.isValid() );

    MainWindow window;
    QVERIFY( ! window.saveGameLogCopy( tmp.filePath( "none.rcg" ) ) );
    QVERIFY( ! QFile::exists( tmp.filePath( "none.rcg" ) ) );

    window.openGameLogFile( LOG );
    QVERIFY( waitForTitle( window, "head_v5.rcg" ) );
    const QString out = tmp.filePath( "saved.rcg" );
    QVERIFY( window.saveGameLogCopy( out ) );
    QCOMPARE( contentOf( out ), contentOf( LOG ) );

    // over a file
    QVERIFY( window.saveGameLogCopy( out ) );
    QCOMPARE( contentOf( out ), contentOf( LOG ) );
    // to itself
    QVERIFY( window.saveGameLogCopy( LOG ) );
    QCOMPARE( contentOf( LOG ).isEmpty(), false );
}

/*-------------------------------------------------------------------*/
/*!
  OpenUrlDialog: the recent URLs; a text that is not a URL shows why and
  keeps the dialog; a URL is accepted (GitHub's page as its raw file); the
  cache, its size and Clear Cache.
*/
void
TestRemoteLog::dialog()
{
    QTemporaryDir tmp;
    QVERIFY( tmp.isValid() );
    const QString cache = tmp.filePath( "logs" );

    {
        OpenUrlDialog dialog( nullptr, { "https://a.example.org/x.rcg", "https://b.example.org/y.rcg" }, cache );
        QComboBox * box = dialog.findChild< QComboBox * >( "url" );
        QLabel * error = dialog.findChild< QLabel * >( "error" );
        QLabel * label = dialog.findChild< QLabel * >( "cache" );
        QPushButton * clear = dialog.findChild< QPushButton * >( "clear_cache" );
        QVERIFY( box && error && label && clear );
        QCOMPARE( box->count(), 2 );
        QCOMPARE( box->itemText( 0 ), QString( "https://a.example.org/x.rcg" ) );
        QCOMPARE( dialog.text(), QString() );
        QVERIFY( label->text().contains( "0 bytes" ) );
        QVERIFY( ! clear->isEnabled() );

        dialog.show();
        dialog.setText( "ftp://example.org/game.rcg" );
        dialog.accept();
        QVERIFY( dialog.isVisible() );
        QVERIFY( error->isVisible() );
        QVERIFY( error->text().contains( "Not an http://" ) );
        dialog.setText( "https://github.com/o/r/blob/main/game.rcg" ); // the error goes
        QVERIFY( ! error->isVisible() );
        dialog.accept();
        QCOMPARE( dialog.result(), int( QDialog::Accepted ) );
        QCOMPARE( dialog.url(), QUrl( "https://raw.githubusercontent.com/o/r/main/game.rcg" ) );
        QCOMPARE( dialog.text(), QString( "https://github.com/o/r/blob/main/game.rcg" ) );
    }

    // a cache with logs: cleared
    const QString path = cachedLogPath( cache, QUrl( "https://example.org/a.rcg" ) );
    QVERIFY( QDir().mkpath( QFileInfo( path ).absolutePath() ) );
    {
        QFile file( path );
        QVERIFY( file.open( QIODevice::WriteOnly ) );
        file.write( QByteArray( 2048, 'x' ) );
    }
    OpenUrlDialog dialog( nullptr, QStringList(), cache );
    QLabel * label = dialog.findChild< QLabel * >( "cache" );
    QPushButton * clear = dialog.findChild< QPushButton * >( "clear_cache" );
    QVERIFY2( label->text().contains( "2.0 KB" ), qPrintable( label->text() ) );
    QCOMPARE( label->toolTip(), QDir::toNativeSeparators( cache ) );
    QVERIFY( clear->isEnabled() );
    clear->click();
    QVERIFY( ! QFile::exists( path ) );
    QVERIFY( label->text().contains( "0 bytes" ) );
    QVERIFY( ! clear->isEnabled() );

    QCOMPARE( OpenUrlDialog::formatBytes( 512 ), QString( "512 bytes" ) );
    QCOMPARE( OpenUrlDialog::formatBytes( 1536 ), QString( "1.5 KB" ) );
    QCOMPARE( OpenUrlDialog::formatBytes( 5 * 1024 * 1024 ), QString( "5.0 MB" ) );
}

/*-------------------------------------------------------------------*/
//! the newest first, once each, at most MAX_RECENT_LOG_URLS; written to the settings now
void
TestRemoteLog::recentUrls()
{
    Options & opt = Options::instance();
    QCOMPARE( Options::MAX_RECENT_LOG_URLS, 10 );
    for ( int i = 0; i < 12; ++i )
    {
        opt.addRecentLogUrl( QStringLiteral( "https://example.org/%1.rcg" ).arg( i ) );
    }
    QCOMPARE( opt.recentLogUrls().size(), 10 );
    QCOMPARE( opt.recentLogUrls().first(), QString( "https://example.org/11.rcg" ) );
    QCOMPARE( opt.recentLogUrls().last(), QString( "https://example.org/2.rcg" ) );

    opt.addRecentLogUrl( " https://example.org/5.rcg " );
    QCOMPARE( opt.recentLogUrls().size(), 10 );
    QCOMPARE( opt.recentLogUrls().first(), QString( "https://example.org/5.rcg" ) );
    QCOMPARE( opt.recentLogUrls().count( "https://example.org/5.rcg" ), 1 );
    opt.addRecentLogUrl( "   " );
    QCOMPARE( opt.recentLogUrls().first(), QString( "https://example.org/5.rcg" ) );

    opt.writeRecentLogUrls();
    QSettings settings( Options::CONF_FILE, QSettings::IniFormat );
    const QStringList saved = settings.value( "GameLog/recent_urls" ).toStringList();
    QCOMPARE( saved, opt.recentLogUrls() );
}

QTEST_MAIN( TestRemoteLog )
#include "test_remote_log.moc"
