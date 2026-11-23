// -*-c++-*-

/*!
  \file test_log_browser_dialog.cpp
  \brief unit test of LogBrowserDialog (File > Browse Game Logs), with
  sources made in the test and an HTTP server on the local machine for
  GitHub
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
#include "log_browser_dialog.h"
#include "log_cache.h"
#include "main_window.h"
#include "open_url_dialog.h"
#include "options.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

/*
  docs/plan/2026-11-22-02_remote-logs.md 4.2, D9, D10. Options and the
  caches are in HOME and XDG_CACHE_HOME in the build tree
  (test/CMakeLists.txt).
*/

namespace {

LogSourceEntry
logEntry( const QString & path,
          const qint64 size )
{
    LogSourceEntry e;
    e.path = path;
    e.size = size;
    e.url = QUrl( "https://example.org/" + path );
    return e;
}

LogSourceEntry
dirEntry( const QString & path,
          const bool listed )
{
    LogSourceEntry e;
    e.path = path;
    e.directory = true;
    e.listed = listed;
    return e;
}

/*!
  \class FakeSource
  \brief a source whose entries are set by the test, told after list()
  returns, as the real ones
*/
class FakeSource
    : public LogSource {
public:
    QString M_name;
    QMap< QString, QVector< LogSourceEntry > > M_dirs; //!< the entries told for each directory
    QMap< QString, QString > M_failures; //!< the directories that fail, and why
    QStringList M_notes; //!< told after the root
    QStringList M_asked; //!< the directories asked ("|reload" after those reloaded)
    int M_aborts = 0;

    explicit FakeSource( const QString & name )
        : M_name( name )
      { }

    QString name() const override { return M_name; }
    QString description() const override { return "a source of the test"; }

    void list( const QString & dir,
               const bool reload ) override
      {
          M_asked << dir + ( reload ? "|reload" : "" );
          const int aborts = M_aborts;
          QTimer::singleShot( 0, this, [this, dir, aborts]() {
              if ( aborts != M_aborts ) return;
              if ( M_failures.contains( dir ) )
              {
                  emit failed( dir, M_failures.value( dir ) );
                  return;
              }
              emit listed( dir, M_dirs.value( dir ) );
              if ( dir.isEmpty() )
              {
                  for ( const QString & n : M_notes ) emit noted( n );
              }
          } );
      }

    void abort() override { ++M_aborts; }
};

const QString HELIOS_YUSHAN = "2026/Finals/202607050404-HELIOS2026_0-vs-YuShan2026_1.rcg.gz";
const QString FRA_NEXUS = "2026/Finals/202607050025-FRA-UNIted_0_4-vs-NexusPrime_0_2.rcg.gz";
const QString HELIOS_OXSY = "2026/GroupA/202607031200-HELIOS2026_5-vs-Oxsy_0.rcg.gz";
const QString A_B = "2026/202607010000-A_1-vs-B_0.rcg.gz";
const QString OTHER = "other/game.rcg";
const QString MERSAD = "2004/200407031414-Mersad_3-vs-TsinghuAeolus_2.rcg.gz";

//! a source with logs in directories (as GitHub's), and a directory read later (as the archive's)
FakeSource *
makeSource( const QString & name = "fake" )
{
    FakeSource * s = new FakeSource( name );
    s->M_dirs[""] = { logEntry( HELIOS_YUSHAN, 5000000 ),
                      logEntry( FRA_NEXUS, 900000 ),
                      logEntry( HELIOS_OXSY, 4000000 ),
                      logEntry( A_B, 10 ),
                      logEntry( OTHER, 2048 ),
                      dirEntry( "2004", false ) };
    s->M_dirs["2004"] = { logEntry( MERSAD, 800 ),
                          dirEntry( "2004/round1", false ) };
    return s;
}

//! the item of a log by its path
QTreeWidgetItem *
itemOf( const LogBrowserDialog & dialog,
        const QString & path )
{
    for ( QTreeWidgetItemIterator it( dialog.tree() ); *it; ++it )
    {
        if ( LogBrowserDialog::urlOf( *it ) == QUrl( "https://example.org/" + path ) ) return *it;
        if ( ( *it )->toolTip( 0 ) == path ) return *it;
    }
    return nullptr;
}

//! the first column of the children of an item
QStringList
childTexts( const QTreeWidgetItem * item )
{
    QStringList list;
    for ( int i = 0; i < item->childCount(); ++i ) list << item->child( i )->text( 0 );
    return list;
}

//! an item and the directories above it are shown
bool
isShown( const QTreeWidgetItem * item )
{
    for ( ; item; item = item->parent() )
    {
        if ( item->isHidden() ) return false;
    }
    return true;
}

bool
markedWrong( const QWidget * w )
{
    return w->property( "rcssmonitor_wrong_mark" ).toInt() == 2;
}

QByteArray
contentOf( const QString & path )
{
    QFile file( path );
    return file.open( QIODevice::ReadOnly ) ? file.readAll() : QByteArray();
}

//! answer the input dialog that comes with a text, or close a message box
struct DialogAnswerer {
    QString input;
    QString message;
    QTimer timer;

    DialogAnswerer()
      {
          timer.setInterval( 50 );
          QObject::connect( &timer, &QTimer::timeout, [this]() {
              for ( QWidget * w : QApplication::topLevelWidgets() )
              {
                  if ( ! w->isVisible() ) continue;
                  if ( QInputDialog * d = qobject_cast< QInputDialog * >( w ) )
                  {
                      d->setTextValue( input );
                      d->accept();
                  }
                  else if ( QMessageBox * box = qobject_cast< QMessageBox * >( w ) )
                  {
                      message = box->text();
                      box->accept();
                  }
              }
          } );
          timer.start();
      }
};

}

/*!
  \brief unit test of LogBrowserDialog
*/
class TestLogBrowserDialog
    : public QObject {
    Q_OBJECT

private slots:

    void initTestCase()
      {
          QNetworkProxy::setApplicationProxy( QNetworkProxy::NoProxy );
      }

    void init()
      {
          QDir( GitHubLogSource::defaultCacheDir() ).removeRecursively();
          LogCache().clear();
      }

    void cleanupTestCase()
      {
          GitHubLogSource::setApiBase( QUrl( "https://api.github.com" ) );
          GitHubLogSource::setRawBase( QUrl( "https://raw.githubusercontent.com" ) );
      }

    void columns();
    void lazyDirectory();
    void filter();
    void sort();
    void open();
    void failure();
    void sources();
    void repositories();
    void mainWindow();
};

/*-------------------------------------------------------------------*/
/*!
  The tree: the directories of the paths, before the logs; a log's date,
  teams, score and size; a log of another name by its name.
*/
void
TestLogBrowserDialog::columns()
{
    LogBrowserDialog dialog;
    dialog.addSource( makeSource(), false );
    QCOMPARE( dialog.currentSource(), 0 );
    QTRY_COMPARE( dialog.tree()->topLevelItemCount(), 3 );

    QTreeWidgetItem * root = dialog.tree()->invisibleRootItem();
    QCOMPARE( childTexts( root ), QStringList( { "2004", "2026", "other" } ) );
    QCOMPARE( childTexts( root->child( 1 ) ), QStringList( { "Finals", "GroupA", "2026-07-01 00:00" } ) );

    const QTreeWidgetItem * game = itemOf( dialog, FRA_NEXUS );
    QVERIFY( game );
    QCOMPARE( game->text( LogBrowserDialog::DATE_COLUMN ), QString( "2026-07-05 00:25" ) );
    QCOMPARE( game->text( LogBrowserDialog::LEFT_COLUMN ), QString( "FRA-UNIted" ) );
    QCOMPARE( game->text( LogBrowserDialog::SCORE_COLUMN ), QString( "0 (4) - 0 (2)" ) );
    QCOMPARE( game->text( LogBrowserDialog::RIGHT_COLUMN ), QString( "NexusPrime" ) );
    QCOMPARE( game->text( LogBrowserDialog::SIZE_COLUMN ), OpenUrlDialog::formatBytes( 900000 ) );
    QCOMPARE( game->toolTip( LogBrowserDialog::DATE_COLUMN ), FRA_NEXUS );

    const QTreeWidgetItem * other = itemOf( dialog, OTHER );
    QVERIFY( other );
    QCOMPARE( other->text( LogBrowserDialog::DATE_COLUMN ), QString( "game.rcg" ) );
    QCOMPARE( other->text( LogBrowserDialog::LEFT_COLUMN ), QString() );
    QCOMPARE( LogBrowserDialog::urlOf( other ), QUrl( "https://example.org/" + OTHER ) );
    QCOMPARE( LogBrowserDialog::urlOf( root->child( 1 ) ), QUrl() );

    QCOMPARE( dialog.statusText(), QString( "5 game logs." ) );
    QVERIFY( ! root->child( 1 )->isExpanded() );
}

/*-------------------------------------------------------------------*/
/*!
  A directory not read yet: read when it is opened, once; one that fails
  is read again when it is opened next.
*/
void
TestLogBrowserDialog::lazyDirectory()
{
    LogBrowserDialog dialog;
    FakeSource * source = makeSource();
    source->M_failures["2004/round1"] = "No way to read it.";
    dialog.addSource( source, false );
    QTRY_COMPARE( dialog.tree()->topLevelItemCount(), 3 );
    QCOMPARE( source->M_asked, QStringList( { "" } ) );

    QTreeWidgetItem * year = dialog.tree()->topLevelItem( 0 );
    QCOMPARE( year->text( 0 ), QString( "2004" ) );
    QCOMPARE( year->childCount(), 0 );
    QCOMPARE( year->childIndicatorPolicy(), QTreeWidgetItem::ShowIndicator );

    year->setExpanded( true );
    QCOMPARE( source->M_asked, QStringList( { "", "2004" } ) );
    QVERIFY( dialog.statusText().contains( "Reading 2004..." ) );
    QTRY_COMPARE( year->childCount(), 2 );
    QCOMPARE( childTexts( year ), QStringList( { "round1", "2004-07-03 14:14" } ) );
    QCOMPARE( dialog.statusText(), QString( "6 game logs." ) );
    year->setExpanded( false );
    year->setExpanded( true );
    QCOMPARE( source->M_asked.size(), 2 );

    // a failure: told, and asked again
    QTreeWidgetItem * round = year->child( 0 );
    round->setExpanded( true );
    QTRY_VERIFY( dialog.statusText().contains( "No way to read it." ) );
    QVERIFY( markedWrong( dialog.findChild< QLabel * >( "status" ) ) );
    QVERIFY( ! round->isExpanded() );
    source->M_failures.clear();
    source->M_dirs["2004/round1"] = { logEntry( "2004/round1/200407011000-X_1-vs-Y_1.rcg.gz", 900 ) };
    round->setExpanded( true );
    QCOMPARE( source->M_asked.last(), QString( "2004/round1" ) );
    QTRY_COMPARE( round->childCount(), 1 );
    QCOMPARE( dialog.statusText(), QString( "7 game logs." ) );
    QVERIFY( ! markedWrong( dialog.findChild< QLabel * >( "status" ) ) );
}

/*-------------------------------------------------------------------*/
/*!
  The filter: the logs of the teams with the text (in any case), or of
  the names with it; the directories with a log shown, or not read yet.
*/
void
TestLogBrowserDialog::filter()
{
    LogBrowserDialog dialog;
    dialog.addSource( makeSource(), false );
    QTRY_COMPARE( dialog.tree()->topLevelItemCount(), 3 );
    QLineEdit * edit = dialog.findChild< QLineEdit * >( "filter" );
    QVERIFY( edit );

    edit->setText( "helios" );
    QVERIFY( isShown( itemOf( dialog, HELIOS_YUSHAN ) ) );
    QVERIFY( isShown( itemOf( dialog, HELIOS_OXSY ) ) );
    QVERIFY( ! isShown( itemOf( dialog, FRA_NEXUS ) ) );
    QVERIFY( ! isShown( itemOf( dialog, A_B ) ) );
    QVERIFY( ! isShown( itemOf( dialog, OTHER ) ) );
    QVERIFY( itemOf( dialog, HELIOS_YUSHAN )->parent()->isExpanded() );
    QVERIFY( isShown( dialog.tree()->topLevelItem( 0 ) ) ); // 2004: not read
    QVERIFY( dialog.tree()->topLevelItem( 2 )->isHidden() ); // other
    QCOMPARE( dialog.statusText(), QString( "2 of 5 game logs." ) );

    dialog.setFilter( "Yushan" );
    QVERIFY( isShown( itemOf( dialog, HELIOS_YUSHAN ) ) );
    QVERIFY( ! isShown( itemOf( dialog, HELIOS_OXSY ) ) );
    QCOMPARE( dialog.statusText(), QString( "1 of 5 game logs." ) );

    dialog.setFilter( "game" );
    QVERIFY( isShown( itemOf( dialog, OTHER ) ) );
    QCOMPARE( dialog.statusText(), QString( "1 of 5 game logs." ) );

    // the logs read later: filtered too
    dialog.setFilter( "mersad" );
    dialog.tree()->topLevelItem( 0 )->setExpanded( true );
    QTRY_VERIFY( itemOf( dialog, MERSAD ) );
    QVERIFY( isShown( itemOf( dialog, MERSAD ) ) );
    QVERIFY( ! isShown( itemOf( dialog, HELIOS_YUSHAN ) ) );
    QCOMPARE( dialog.statusText(), QString( "1 of 6 game logs." ) );

    edit->clear();
    QVERIFY( isShown( itemOf( dialog, FRA_NEXUS ) ) );
    QVERIFY( isShown( itemOf( dialog, OTHER ) ) );
    QCOMPARE( dialog.statusText(), QString( "6 game logs." ) );
}

/*-------------------------------------------------------------------*/
//! sorted by a column, in either order: the directories first
void
TestLogBrowserDialog::sort()
{
    LogBrowserDialog dialog;
    dialog.addSource( makeSource(), false );
    QTRY_COMPARE( dialog.tree()->topLevelItemCount(), 3 );
    QTreeWidgetItem * year = dialog.tree()->topLevelItem( 1 );
    QTreeWidgetItem * finals = year->child( 0 );
    QCOMPARE( finals->text( 0 ), QString( "Finals" ) );

    // by the date
    QCOMPARE( finals->child( 0 )->text( LogBrowserDialog::LEFT_COLUMN ), QString( "FRA-UNIted" ) );
    dialog.tree()->sortByColumn( LogBrowserDialog::DATE_COLUMN, Qt::DescendingOrder );
    QCOMPARE( finals->child( 0 )->text( LogBrowserDialog::LEFT_COLUMN ), QString( "HELIOS2026" ) );
    QCOMPARE( childTexts( year ), QStringList( { "GroupA", "Finals", "2026-07-01 00:00" } ) );

    // by the size, not by the text ("4.8 MB" < "878.9 KB")
    dialog.tree()->sortByColumn( LogBrowserDialog::SIZE_COLUMN, Qt::AscendingOrder );
    QCOMPARE( finals->child( 0 )->text( LogBrowserDialog::LEFT_COLUMN ), QString( "FRA-UNIted" ) );
    QCOMPARE( year->child( 2 )->text( LogBrowserDialog::LEFT_COLUMN ), QString( "A" ) );
    QCOMPARE( childTexts( dialog.tree()->invisibleRootItem() ).size(), 3 );
    dialog.tree()->sortByColumn( LogBrowserDialog::SIZE_COLUMN, Qt::DescendingOrder );
    QCOMPARE( finals->child( 0 )->text( LogBrowserDialog::LEFT_COLUMN ), QString( "HELIOS2026" ) );
    QCOMPARE( year->child( 2 )->text( LogBrowserDialog::LEFT_COLUMN ), QString( "A" ) );

    // by a team
    dialog.tree()->sortByColumn( LogBrowserDialog::RIGHT_COLUMN, Qt::AscendingOrder );
    QCOMPARE( finals->child( 0 )->text( LogBrowserDialog::RIGHT_COLUMN ), QString( "NexusPrime" ) );
}

/*-------------------------------------------------------------------*/
/*!
  A log opened by Open, or a double click (activated); not a directory;
  the dialog stays open, and is not modal.
*/
void
TestLogBrowserDialog::open()
{
    LogBrowserDialog dialog;
    QVERIFY( ! dialog.isModal() );
    dialog.show();
    QVERIFY( QTest::qWaitForWindowExposed( &dialog ) );
    dialog.addSource( makeSource(), false );
    QTRY_COMPARE( dialog.tree()->topLevelItemCount(), 3 );
    QSignalSpy spy( &dialog, &LogBrowserDialog::openRequested );
    QPushButton * open = dialog.findChild< QPushButton * >( "open" );
    QVERIFY( open );
    QVERIFY( ! open->isEnabled() );

    dialog.tree()->setCurrentItem( dialog.tree()->topLevelItem( 1 ) );
    QVERIFY( ! open->isEnabled() );
    dialog.openCurrent();
    QCOMPARE( spy.count(), 0 );

    QTreeWidgetItem * game = itemOf( dialog, HELIOS_YUSHAN );
    dialog.tree()->setCurrentItem( game );
    QVERIFY( open->isEnabled() );
    open->click();
    QCOMPARE( spy.count(), 1 );
    QCOMPARE( spy.last().at( 0 ).toUrl(), QUrl( "https://example.org/" + HELIOS_YUSHAN ) );
    QVERIFY( dialog.isVisible() );

    // Enter in the tree (as a double click: activated): once, not by Open too
    QTreeWidgetItem * other = itemOf( dialog, OTHER );
    dialog.tree()->setCurrentItem( other );
    dialog.tree()->setFocus();
    QTest::keyClick( dialog.tree(), Qt::Key_Return );
    QTest::qWait( 50 );
    QCOMPARE( spy.count(), 2 );
    QCOMPARE( spy.last().at( 0 ).toUrl(), QUrl( "https://example.org/" + OTHER ) );
    emit dialog.tree()->itemActivated( other, LogBrowserDialog::SIZE_COLUMN );
    QCOMPARE( spy.count(), 3 );
    emit dialog.tree()->itemActivated( other->parent(), 0 );
    QCOMPARE( spy.count(), 3 );

    // Enter in the filter: nothing opened
    QLineEdit * filter = dialog.findChild< QLineEdit * >( "filter" );
    filter->setFocus();
    QTest::keyClick( filter, Qt::Key_Return );
    QTest::qWait( 50 );
    QCOMPARE( spy.count(), 3 );
    QVERIFY( dialog.isVisible() );
}

/*-------------------------------------------------------------------*/
/*!
  A source that fails: told and marked; Reload asks the server again; a
  note (e.g. a list from the cache) told after the number.
*/
void
TestLogBrowserDialog::failure()
{
    LogBrowserDialog dialog;
    FakeSource * source = makeSource();
    source->M_failures[""] = "The server did not answer.";
    dialog.addSource( source, false );
    QLabel * status = dialog.findChild< QLabel * >( "status" );
    QTRY_COMPARE( dialog.statusText(), QString( "The server did not answer." ) );
    QVERIFY( markedWrong( status ) );
    QCOMPARE( dialog.tree()->topLevelItemCount(), 0 );

    source->M_failures.clear();
    source->M_notes << "The list read at 2026-11-24 10:00.";
    QPushButton * reload = dialog.findChild< QPushButton * >( "reload" );
    QVERIFY( reload && reload->isEnabled() );
    reload->click();
    QCOMPARE( source->M_asked, QStringList( { "", "|reload" } ) );
    QCOMPARE( dialog.statusText(), QString( "Reading the list..." ) );
    QTRY_COMPARE( dialog.tree()->topLevelItemCount(), 3 );
    QTRY_COMPARE( dialog.statusText(), QString( "5 game logs. The list read at 2026-11-24 10:00." ) );
    QVERIFY( ! markedWrong( status ) );
}

/*-------------------------------------------------------------------*/
/*!
  The sources: one read when it is chosen first, and kept when another is
  chosen; what one not shown tells does not change the tree.
*/
void
TestLogBrowserDialog::sources()
{
    LogBrowserDialog dialog;
    FakeSource * first = makeSource( "first" );
    FakeSource * second = new FakeSource( "second" );
    second->M_dirs[""] = { logEntry( "202607050404-HELIOS2026_0-vs-YuShan2026_1.rcg.gz", 100 ) };
    dialog.addSource( first, false );
    dialog.addSource( second, false );
    QCOMPARE( dialog.sourceCount(), 2 );
    QCOMPARE( dialog.source( 1 ), second );
    QCOMPARE( dialog.source( 2 ), nullptr );
    QCOMPARE( dialog.currentSource(), 0 );
    QTRY_COMPARE( dialog.tree()->topLevelItemCount(), 3 );
    QCOMPARE( second->M_asked, QStringList() );

    dialog.setCurrentSource( 1 );
    QCOMPARE( second->M_asked, QStringList( { "" } ) );
    QTRY_COMPARE( dialog.tree()->topLevelItemCount(), 1 );
    QCOMPARE( dialog.statusText(), QString( "1 game log." ) );

    // the first again: not asked again; told while the second is shown
    first->list( QString(), true );
    QTest::qWait( 50 );
    QCOMPARE( dialog.tree()->topLevelItemCount(), 1 );
    dialog.setCurrentSource( 0 );
    QCOMPARE( first->M_asked, QStringList( { "", "|reload" } ) );
    QCOMPARE( dialog.tree()->topLevelItemCount(), 3 );
    QCOMPARE( dialog.statusText(), QString( "5 game logs." ) );

    // the sources go with the dialog
    {
        LogBrowserDialog * d = new LogBrowserDialog();
        FakeSource * s = makeSource();
        QSignalSpy gone( s, &QObject::destroyed );
        d->addSource( s, false );
        delete d;
        QCOMPARE( gone.count(), 1 );
    }
}

/*-------------------------------------------------------------------*/
/*!
  The repositories on GitHub that the user adds (read from the server of
  the test) and removes; the default ones cannot be removed.
*/
void
TestLogBrowserDialog::repositories()
{
    HttpTestServer server;
    QFile tree( QStringLiteral( TEST_DATA_DIR "/github_tree.json" ) );
    QVERIFY( tree.open( QIODevice::ReadOnly ) );
    server.route( "/repos/o/r/git/trees/HEAD?recursive=1", HttpTestServer::ok( tree.readAll() ) );
    GitHubLogSource::setApiBase( server.url( "" ) );
    GitHubLogSource::setRawBase( server.url( "/raw" ) );

    LogBrowserDialog dialog;
    dialog.addSource( makeSource(), false );
    QSignalSpy spy( &dialog, &LogBrowserDialog::repositoriesChanged );
    QPushButton * remove = dialog.findChild< QPushButton * >( "remove_repository" );
    QVERIFY( remove );
    QVERIFY( ! remove->isEnabled() );

    QString error;
    QVERIFY( dialog.addRepository( " https://github.com/o/r ", &error ) );
    QCOMPARE( dialog.sourceCount(), 2 );
    QCOMPARE( dialog.currentSource(), 1 );
    QCOMPARE( dialog.source( 1 )->name(), QString( "o/r" ) );
    QCOMPARE( spy.count(), 1 );
    QCOMPARE( spy.last().at( 0 ).toStringList(), QStringList( { "o/r" } ) );
    QCOMPARE( dialog.addedRepositories(), QStringList( { "o/r" } ) );
    QVERIFY( remove->isEnabled() );
    QTRY_COMPARE( dialog.statusText(), QString( "21 game logs." ) );
    QCOMPARE( server.requests( "/repos/o/r/git/trees/HEAD?recursive=1" ), 1 );
    QCOMPARE( childTexts( dialog.tree()->invisibleRootItem() ), QStringList( { "logs" } ) );
    // a directory alone: opened
    QVERIFY( dialog.tree()->topLevelItem( 0 )->isExpanded() );
    QCOMPARE( childTexts( dialog.tree()->topLevelItem( 0 ) ), QStringList( { "Finals", "GroupA", "GroupQ" } ) );
    QVERIFY( ! dialog.tree()->topLevelItem( 0 )->child( 0 )->isExpanded() );

    // there already, or not a repository
    dialog.setCurrentSource( 0 );
    QVERIFY( ! dialog.addRepository( "O/R", &error ) );
    QVERIFY2( error.contains( "o/r is in the list already" ), qPrintable( error ) );
    QCOMPARE( dialog.currentSource(), 1 );
    QVERIFY( ! dialog.addRepository( "rcsoccersim", &error ) );
    QVERIFY2( error.contains( "Not a repository on GitHub" ), qPrintable( error ) );
    QCOMPARE( dialog.sourceCount(), 2 );
    QCOMPARE( spy.count(), 1 );

    // by the button: the input dialog
    {
        DialogAnswerer answerer;
        answerer.input = "o/r@dev";
        dialog.findChild< QPushButton * >( "add_repository" )->click();
        QCOMPARE( dialog.sourceCount(), 3 );
        QCOMPARE( spy.last().at( 0 ).toStringList(), QStringList( { "o/r", "o/r@dev" } ) );
        // not on GitHub
        QTRY_VERIFY( dialog.statusText().contains( "o/r@dev is not found on GitHub" ) );

        answerer.input = "not a repository";
        dialog.findChild< QPushButton * >( "add_repository" )->click();
        QTRY_VERIFY( answerer.message.contains( "Not a repository on GitHub" ) );
        QCOMPARE( dialog.sourceCount(), 3 );
    }

    // removed
    dialog.setCurrentSource( 1 );
    remove->click();
    QCOMPARE( dialog.sourceCount(), 2 );
    QCOMPARE( spy.last().at( 0 ).toStringList(), QStringList( { "o/r@dev" } ) );
    QCOMPARE( dialog.source( 1 )->name(), QString( "o/r@dev" ) );
    dialog.setCurrentSource( 0 );
    QVERIFY( ! remove->isEnabled() );
    remove->click();
    QCOMPARE( dialog.sourceCount(), 2 );
}

/*-------------------------------------------------------------------*/
/*!
  File > Browse Game Logs: the dialog of the default repository and those
  saved, made once, not modal; a log opened from it (downloaded) in the
  window; a repository added saved in the options.
*/
void
TestLogBrowserDialog::mainWindow()
{
    const QString name = "202607050404-HELIOS2026_0-vs-YuShan2026_1.rcg";
    const QByteArray tree = "{\"sha\":\"x\",\"tree\":["
        "{\"path\":\"logs\",\"type\":\"tree\"},"
        "{\"path\":\"logs/" + name.toUtf8() + "\",\"type\":\"blob\",\"size\":"
        + QByteArray::number( contentOf( QStringLiteral( TEST_DATA_DIR "/head_v5.rcg" ) ).size() ) + "}"
        "],\"truncated\":false}";
    HttpTestServer server;
    server.route( "/repos/rcsoccersim/RoboCup2026/git/trees/HEAD?recursive=1", HttpTestServer::ok( tree ) );
    server.route( "/raw/rcsoccersim/RoboCup2026/HEAD/logs/" + name.toUtf8(),
                  HttpTestServer::ok( contentOf( QStringLiteral( TEST_DATA_DIR "/head_v5.rcg" ) ) ) );
    GitHubLogSource::setApiBase( server.url( "" ) );
    GitHubLogSource::setRawBase( server.url( "/raw" ) );
    const QStringList saved = Options::instance().logRepositories();
    Options::instance().setLogRepositories( { "o/saved" } );

    MainWindow window;
    window.show();
    QVERIFY( QTest::qWaitForWindowExposed( &window ) );
    QAction * browse = window.findChild< QAction * >( "browse_logs" );
    QVERIFY( browse );
    QCOMPARE( browse->shortcut(), QKeySequence( Qt::CTRL | Qt::SHIFT | Qt::Key_B ) );
    QVERIFY( ! window.findChild< LogBrowserDialog * >() );

    browse->trigger();
    LogBrowserDialog * dialog = window.findChild< LogBrowserDialog * >();
    QVERIFY( dialog );
    QVERIFY( dialog->isVisible() );
    QVERIFY( ! dialog->isModal() );
    QCOMPARE( dialog->sourceCount(), 2 );
    QCOMPARE( dialog->source( 0 )->name(), QString( "rcsoccersim/RoboCup2026" ) );
    QCOMPARE( dialog->source( 1 )->name(), QString( "o/saved" ) );
    QCOMPARE( dialog->addedRepositories(), QStringList( { "o/saved" } ) );
    QCOMPARE( dialog->currentSource(), 0 );
    QTRY_COMPARE( dialog->statusText(), QString( "1 game log." ) );

    // opened in the window; the dialog stays
    QTreeWidgetItem * game = dialog->tree()->topLevelItem( 0 )->child( 0 );
    QVERIFY( game );
    dialog->tree()->setCurrentItem( game );
    dialog->openCurrent();
    QVERIFY( QTest::qWaitFor( [&]() { return window.windowTitle().startsWith( name ); }, 10000 ) );
    QVERIFY( dialog->isVisible() );

    // added: saved
    QString error;
    QVERIFY( dialog->addRepository( "o/added", &error ) );
    QCOMPARE( Options::instance().logRepositories(), QStringList( { "o/saved", "o/added" } ) );

    // the same dialog again
    dialog->close();
    browse->trigger();
    QCOMPARE( window.findChildren< LogBrowserDialog * >().size(), 1 );
    QVERIFY( dialog->isVisible() );
    QCOMPARE( dialog->sourceCount(), 3 );

    Options::instance().setLogRepositories( saved );
    Options::instance().writeLogRepositories();
}

QTEST_MAIN( TestLogBrowserDialog )
#include "test_log_browser_dialog.moc"
