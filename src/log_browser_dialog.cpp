// -*-c++-*-

/*!
  \file log_browser_dialog.cpp
  \brief the dialog of the lists of game logs (File > Browse Game Logs).
  Source File.
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

#include "log_browser_dialog.h"

#include "archive_log_source.h"
#include "github_log_source.h"
#include "log_file_name.h"
#include "open_url_dialog.h"
#include "options.h"
#include "ui_palette.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>
#include <iostream>

namespace {

// the data of the items
const int URL_ROLE = Qt::UserRole + 1; //!< a log's URL (the first column)
const int SORT_ROLE = Qt::UserRole + 2; //!< what a column is sorted by
const int PATH_ROLE = Qt::UserRole + 3; //!< a directory's path (the first column)
const int LISTED_ROLE = Qt::UserRole + 4; //!< a directory whose entries are known

//! the parent directory of a path ("" for the root)
QString
parent_path( const QString & path )
{
    const int slash = path.lastIndexOf( QLatin1Char( '/' ) );
    return slash < 0 ? QString() : path.left( slash );
}

/*!
  \class LogItem
  \brief an item of the tree: the directories before the logs, and the
  columns sorted by their data (the date, the size)
*/
class LogItem
    : public QTreeWidgetItem {
public:
    using QTreeWidgetItem::QTreeWidgetItem;

    bool isDirectory() const
      {
          return data( LogBrowserDialog::DATE_COLUMN, PATH_ROLE ).isValid();
      }

    bool operator<( const QTreeWidgetItem & other ) const override
      {
          const LogItem * o = dynamic_cast< const LogItem * >( &other );
          if ( o && isDirectory() != o->isDirectory() )
          {
              // directories first, in either order
              const bool ascending = ( ! treeWidget()
                                       || treeWidget()->header()->sortIndicatorOrder() == Qt::AscendingOrder );
              return isDirectory() == ascending;
          }
          const int column = treeWidget() ? treeWidget()->sortColumn() : 0;
          const QVariant a = data( column, SORT_ROLE );
          const QVariant b = other.data( column, SORT_ROLE );
          if ( a.isValid() && b.isValid() )
          {
              if ( a.userType() == QMetaType::LongLong
                   && b.userType() == QMetaType::LongLong )
              {
                  return a.toLongLong() < b.toLongLong();
              }
              return a.toString().localeAwareCompare( b.toString() ) < 0;
          }
          return text( column ).localeAwareCompare( other.text( column ) ) < 0;
      }
};

}

/*-------------------------------------------------------------------*/
LogBrowserDialog::LogBrowserDialog( QWidget * parent )
    : QDialog( parent )
{
    this->setWindowTitle( tr( "Browse Game Logs" ) );

    QVBoxLayout * layout = new QVBoxLayout();
    QSplitter * splitter = new QSplitter( Qt::Horizontal );

    // the sources
    {
        QWidget * box = new QWidget();
        QVBoxLayout * left = new QVBoxLayout();
        left->setContentsMargins( 0, 0, 0, 0 );
        left->addWidget( new QLabel( tr( "Sources" ) ) );
        M_source_list = new QListWidget();
        M_source_list->setObjectName( "sources" );
        // a long name (e.g. the archive's URL) cut in the middle: the whole in the tip
        M_source_list->setTextElideMode( Qt::ElideMiddle );
        M_source_list->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        connect( M_source_list, &QListWidget::currentRowChanged,
                 this, &LogBrowserDialog::showCurrentSource );
        left->addWidget( M_source_list );
        QHBoxLayout * buttons = new QHBoxLayout();
        M_add_button = new QPushButton( tr( "Add..." ) );
        M_add_button->setObjectName( "add_source" );
        M_add_button->setAutoDefault( false );
        M_add_button->setToolTip( tr( "Add a source: a public repository on GitHub (OWNER/REPO, OWNER/REPO@BRANCH, or its URL),"
                                      " or the URL of a directory listed by h5ai. A default source removed can be added again." ) );
        connect( M_add_button, &QPushButton::clicked,
                 this, &LogBrowserDialog::addSourceByDialog );
        buttons->addWidget( M_add_button );
        M_remove_button = new QPushButton( tr( "Remove" ) );
        M_remove_button->setObjectName( "remove_source" );
        M_remove_button->setAutoDefault( false );
        M_remove_button->setToolTip( tr( "Remove the source chosen (a default one can be added again)." ) );
        connect( M_remove_button, &QPushButton::clicked,
                 this, &LogBrowserDialog::removeCurrentSource );
        buttons->addWidget( M_remove_button );
        left->addLayout( buttons );
        box->setLayout( left );
        splitter->addWidget( box );
    }

    // the logs
    {
        QWidget * box = new QWidget();
        QVBoxLayout * right = new QVBoxLayout();
        right->setContentsMargins( 0, 0, 0, 0 );
        QHBoxLayout * row = new QHBoxLayout();
        QLabel * label = new QLabel( tr( "Team:" ) );
        row->addWidget( label );
        M_filter = new QLineEdit();
        M_filter->setObjectName( "filter" );
        M_filter->setClearButtonEnabled( true );
        M_filter->setPlaceholderText( tr( "a team's name" ) );
        M_filter->setToolTip( tr( "Show only the games of the teams whose names have this." ) );
        label->setBuddy( M_filter );
        connect( M_filter, &QLineEdit::textChanged,
                 this, &LogBrowserDialog::setFilter );
        row->addWidget( M_filter, 1 );
        M_reload_button = new QPushButton( tr( "Reload" ) );
        M_reload_button->setObjectName( "reload" );
        M_reload_button->setAutoDefault( false );
        M_reload_button->setToolTip( tr( "Read the list again from the server." ) );
        connect( M_reload_button, &QPushButton::clicked,
                 this, &LogBrowserDialog::reload );
        row->addWidget( M_reload_button );
        right->addLayout( row );

        M_tree = new QTreeWidget();
        M_tree->setObjectName( "logs" );
        M_tree->setColumnCount( COLUMN_COUNT );
        M_tree->setHeaderLabels( { tr( "Date" ), tr( "Left" ), tr( "Score" ), tr( "Right" ), tr( "Size" ) } );
        M_tree->setUniformRowHeights( true );
        M_tree->setSortingEnabled( true );
        M_tree->sortByColumn( DATE_COLUMN, Qt::AscendingOrder );
        M_tree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );
        M_tree->header()->setStretchLastSection( false );
        connect( M_tree, &QTreeWidget::itemActivated,
                 this, [this]( QTreeWidgetItem * item, int ) {
                     const QUrl url = urlOf( item );
                     if ( url.isValid() ) emit openRequested( url );
                 } );
        connect( M_tree, &QTreeWidget::itemExpanded,
                 this, &LogBrowserDialog::readDirectory );
        connect( M_tree, &QTreeWidget::currentItemChanged,
                 this, [this]() { updateButtons(); } );
        right->addWidget( M_tree, 1 );

        M_status_label = new QLabel();
        M_status_label->setObjectName( "status" );
        M_status_label->setWordWrap( true );
        right->addWidget( M_status_label );
        box->setLayout( right );
        splitter->addWidget( box );
    }
    splitter->setStretchFactor( 0, 0 );
    splitter->setStretchFactor( 1, 1 );
    splitter->setSizes( { 240, 660 } );
    layout->addWidget( splitter, 1 );

    QDialogButtonBox * buttons = new QDialogButtonBox();
    // not the default button: Enter in the tree opens a log by
    // itemActivated, and the default button would open it again (nor
    // AcceptRole: QDialogButtonBox makes the first of them the default one)
    M_open_button = buttons->addButton( tr( "Open" ), QDialogButtonBox::ActionRole );
    M_open_button->setObjectName( "open" );
    M_open_button->setAutoDefault( false );
    M_open_button->setToolTip( tr( "Open the game log chosen (a double click opens it too). The dialog stays open." ) );
    // not accept(): the dialog stays open (D10)
    connect( M_open_button, &QPushButton::clicked,
             this, &LogBrowserDialog::openCurrent );
    QPushButton * close = buttons->addButton( QDialogButtonBox::Close );
    close->setAutoDefault( false );
    connect( close, &QPushButton::clicked,
             this, &LogBrowserDialog::close );
    layout->addWidget( buttons );

    this->setLayout( layout );
    this->resize( 900, 520 );
    updateButtons();
}

/*-------------------------------------------------------------------*/
LogBrowserDialog::~LogBrowserDialog()
{
    for ( const Source & s : M_sources )
    {
        s.source->abort();
    }
}

/*-------------------------------------------------------------------*/
LogSource *
LogBrowserDialog::createSource( const QString & text,
                                QString * error )
{
    QString owner, repo, ref;
    if ( GitHubLogSource::parseRepository( text, &owner, &repo, &ref ) )
    {
        if ( error ) error->clear();
        return new GitHubLogSource( owner, repo, ref );
    }

    const QString t = text.trimmed();
    if ( t.startsWith( QLatin1String( "http://" ), Qt::CaseInsensitive )
         || t.startsWith( QLatin1String( "https://" ), Qt::CaseInsensitive ) )
    {
        const QUrl url = ArchiveLogSource::parseUrl( t, error );
        return url.isValid() ? new ArchiveLogSource( url ) : nullptr;
    }

    if ( error ) *error = tr( "Not a source of game logs: %1\n"
                              "(a repository on GitHub: OWNER/REPO, OWNER/REPO@BRANCH, or its URL;"
                              " or the URL of a directory listed by h5ai, as archive.robocup.info)" ).arg( t );
    return nullptr;
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::addSource( LogSource * source )
{
    source->setParent( this );
    Source s;
    s.source = source;
    M_sources.push_back( s );

    connect( source, &LogSource::listed,
             this, [this, source]( const QString & dir, const QVector< LogSourceEntry > & entries ) {
                 handleListed( source, dir, entries );
             } );
    connect( source, &LogSource::failed,
             this, [this, source]( const QString & dir, const QString & message ) {
                 handleFailed( source, dir, message );
             } );
    connect( source, &LogSource::noted,
             this, [this, source]( const QString & text ) {
                 handleNoted( source, text );
             } );

    QListWidgetItem * item = new QListWidgetItem( source->name() );
    item->setToolTip( source->description() );
    M_source_list->addItem( item );
    if ( M_source_list->currentRow() < 0 )
    {
        M_source_list->setCurrentRow( 0 );
    }
    updateButtons();
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::setSources( const QStringList & texts )
{
    for ( const QString & text : texts )
    {
        QString error;
        LogSource * s = createSource( text, &error );
        if ( ! s )
        {
            std::cerr << "(LogBrowserDialog::setSources) " << error.toStdString() << std::endl;
            continue;
        }
        if ( sourceTexts().contains( s->text(), Qt::CaseInsensitive ) )
        {
            delete s;
            continue;
        }
        addSource( s );
    }
}

/*-------------------------------------------------------------------*/
LogSource *
LogBrowserDialog::source( const int index ) const
{
    return ( 0 <= index && index < M_sources.size() ) ? M_sources[index].source : nullptr;
}

/*-------------------------------------------------------------------*/
int
LogBrowserDialog::currentSource() const
{
    return M_source_list->currentRow();
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::setCurrentSource( const int index )
{
    M_source_list->setCurrentRow( index );
}

/*-------------------------------------------------------------------*/
int
LogBrowserDialog::indexOf( const LogSource * source ) const
{
    for ( int i = 0; i < M_sources.size(); ++i )
    {
        if ( M_sources[i].source == source ) return i;
    }
    return -1;
}

/*-------------------------------------------------------------------*/
bool
LogBrowserDialog::addSource( const QString & text,
                             QString * error )
{
    LogSource * source = createSource( text, error );
    if ( ! source )
    {
        return false;
    }
    for ( int i = 0; i < M_sources.size(); ++i )
    {
        if ( M_sources[i].source->text().compare( source->text(), Qt::CaseInsensitive ) == 0 )
        {
            if ( error ) *error = tr( "%1 is in the list already." ).arg( M_sources[i].source->name() );
            delete source;
            setCurrentSource( i );
            return false;
        }
    }

    addSource( source );
    setCurrentSource( M_sources.size() - 1 );
    emit sourcesChanged( sourceTexts() );
    return true;
}

/*-------------------------------------------------------------------*/
QStringList
LogBrowserDialog::sourceTexts() const
{
    QStringList list;
    for ( const Source & s : M_sources )
    {
        list << s.source->text();
    }
    return list;
}

/*-------------------------------------------------------------------*/
QStringList
LogBrowserDialog::missingDefaultSources() const
{
    // the defaults are written as text() gives them
    const QStringList texts = sourceTexts();
    QStringList list;
    for ( const QString & text : Options::DEFAULT_LOG_SOURCES )
    {
        if ( ! texts.contains( text, Qt::CaseInsensitive ) ) list << text;
    }
    return list;
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::addSourceByDialog()
{
    // the field: the default sources removed as its items
    QStringList items = missingDefaultSources();
    items.prepend( QString() );
    bool ok = false;
    const QString text = QInputDialog::getItem( this,
                                                tr( "Add a Source" ),
                                                tr( "A source of game logs:\n"
                                                    "- a public repository on GitHub: OWNER/REPO, OWNER/REPO@BRANCH, or its URL\n"
                                                    "- the URL of a directory listed by h5ai, as archive.robocup.info\n"
                                                    "The default sources removed are in the list." ),
                                                items,
                                                0,
                                                true,
                                                &ok );
    if ( ! ok || text.trimmed().isEmpty() )
    {
        return;
    }
    QString error;
    if ( ! addSource( text, &error ) )
    {
        QMessageBox::warning( this, tr( "Add a Source" ), error );
    }
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::removeCurrentSource()
{
    const int index = currentSource();
    if ( index < 0 )
    {
        return;
    }
    LogSource * removed = M_sources[index].source;
    removed->abort();
    {
        // not shown while the list and the sources differ: takeItem() changes
        // the current row before the item is gone
        const QSignalBlocker blocker( M_source_list );
        delete M_source_list->takeItem( index );
        M_sources.remove( index );
        M_source_list->setCurrentRow( std::min( index, int( M_sources.size() ) - 1 ) );
    }
    removed->deleteLater();
    emit sourcesChanged( sourceTexts() );
    showCurrentSource();
}

/*-------------------------------------------------------------------*/
QTreeWidgetItem *
LogBrowserDialog::dirItem( const QString & path )
{
    if ( path.isEmpty() )
    {
        return M_tree->invisibleRootItem();
    }
    QTreeWidgetItem * item = M_dir_items.value( path );
    if ( item )
    {
        return item;
    }

    QTreeWidgetItem * parent = dirItem( parent_path( path ) );
    item = new LogItem( parent );
    item->setText( DATE_COLUMN, path.section( QLatin1Char( '/' ), -1 ) );
    item->setData( DATE_COLUMN, PATH_ROLE, path );
    item->setData( DATE_COLUMN, SORT_ROLE, path.section( QLatin1Char( '/' ), -1 ) );
    item->setData( DATE_COLUMN, LISTED_ROLE, true );
    item->setToolTip( DATE_COLUMN, path );
    M_dir_items.insert( path, item );
    return item;
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::addItem( const LogSourceEntry & entry )
{
    if ( entry.directory )
    {
        QTreeWidgetItem * item = dirItem( entry.path );
        if ( ! entry.listed
             && item->childCount() == 0 )
        {
            // read when it is opened (readDirectory())
            item->setData( DATE_COLUMN, LISTED_ROLE, false );
            item->setChildIndicatorPolicy( QTreeWidgetItem::ShowIndicator );
        }
        return;
    }

    const QString file_name = entry.path.section( QLatin1Char( '/' ), -1 );
    const LogFileName name = LogFileName::parse( file_name );
    QTreeWidgetItem * item = new LogItem( dirItem( parent_path( entry.path ) ) );
    if ( name.isValid() )
    {
        item->setText( DATE_COLUMN, name.dateTime().toString( QStringLiteral( "yyyy-MM-dd HH:mm" ) ) );
        item->setData( DATE_COLUMN, SORT_ROLE, name.dateTime().toString( QStringLiteral( "yyyyMMddHHmmss" ) ) );
        item->setText( LEFT_COLUMN, name.leftTeam() );
        item->setText( SCORE_COLUMN, name.score() );
        item->setTextAlignment( SCORE_COLUMN, Qt::AlignCenter );
        item->setText( RIGHT_COLUMN, name.rightTeam() );
    }
    else
    {
        // a name of another form: as it is
        item->setText( DATE_COLUMN, file_name );
        item->setData( DATE_COLUMN, SORT_ROLE, file_name );
    }
    if ( entry.size >= 0 )
    {
        item->setText( SIZE_COLUMN, OpenUrlDialog::formatBytes( entry.size ) );
        item->setData( SIZE_COLUMN, SORT_ROLE, qlonglong( entry.size ) );
        item->setTextAlignment( SIZE_COLUMN, Qt::AlignRight | Qt::AlignVCenter );
    }
    item->setData( DATE_COLUMN, URL_ROLE, entry.url );
    item->setToolTip( DATE_COLUMN, entry.path );
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::showEntries( const Source & source,
                               const QString & dir )
{
    M_tree->setSortingEnabled( false );
    if ( dir.isEmpty() )
    {
        M_tree->clear();
        M_dir_items.clear();
    }
    else if ( QTreeWidgetItem * item = M_dir_items.value( dir ) )
    {
        // its entries again
        for ( QTreeWidgetItem * child : item->takeChildren() )
        {
            const QString path = child->data( DATE_COLUMN, PATH_ROLE ).toString();
            for ( auto it = M_dir_items.begin(); it != M_dir_items.end(); )
            {
                if ( ! path.isEmpty()
                     && ( it.key() == path || it.key().startsWith( path + QLatin1Char( '/' ) ) ) )
                {
                    it = M_dir_items.erase( it );
                }
                else
                {
                    ++it;
                }
            }
            delete child;
        }
        item->setData( DATE_COLUMN, LISTED_ROLE, true );
        item->setChildIndicatorPolicy( QTreeWidgetItem::DontShowIndicatorWhenChildless );
    }

    const QString prefix = dir.isEmpty() ? QString() : dir + QLatin1Char( '/' );
    for ( const LogSourceEntry & entry : source.entries )
    {
        if ( prefix.isEmpty() || entry.path.startsWith( prefix ) )
        {
            addItem( entry );
        }
    }
    if ( dir.isEmpty() )
    {
        // a directory alone (e.g. "logs" of GitHub) opened, and one alone in it
        QTreeWidgetItem * item = M_tree->invisibleRootItem();
        while ( item->childCount() == 1
                && item->child( 0 )->data( DATE_COLUMN, PATH_ROLE ).isValid()
                && item->child( 0 )->data( DATE_COLUMN, LISTED_ROLE ).toBool() )
        {
            item = item->child( 0 );
            item->setExpanded( true );
        }
    }
    M_tree->setSortingEnabled( true );
    setFilter( M_filter->text() );
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::showCurrentSource()
{
    const int index = currentSource();
    M_tree->clear();
    M_dir_items.clear();
    if ( index < 0 )
    {
        updateStatus();
        updateButtons();
        return;
    }

    Source & s = M_sources[index];
    if ( ! s.requested )
    {
        s.requested = true;
        s.status = tr( "Reading the list..." );
        s.failure = false;
        s.source->list( QString(), false );
    }
    showEntries( s, QString() );
    updateStatus();
    updateButtons();
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::reload()
{
    const int index = currentSource();
    if ( index < 0 ) return;
    Source & s = M_sources[index];
    s.requested = true;
    s.status = tr( "Reading the list..." );
    s.failure = false;
    s.source->list( QString(), true );
    updateStatus();
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::readDirectory( QTreeWidgetItem * item )
{
    if ( ! item
         || item->data( DATE_COLUMN, LISTED_ROLE ).toBool() )
    {
        return;
    }
    const int index = currentSource();
    if ( index < 0 ) return;

    const QString path = item->data( DATE_COLUMN, PATH_ROLE ).toString();
    item->setData( DATE_COLUMN, LISTED_ROLE, true ); // asked once
    M_sources[index].status = tr( "Reading %1..." ).arg( path );
    M_sources[index].failure = false;
    M_sources[index].source->list( path, false );
    updateStatus();
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::handleListed( LogSource * source,
                                const QString & dir,
                                const QVector< LogSourceEntry > & entries )
{
    const int index = indexOf( source );
    if ( index < 0 ) return;
    Source & s = M_sources[index];

    // the entries under the directory, again
    const QString prefix = dir.isEmpty() ? QString() : dir + QLatin1Char( '/' );
    for ( auto it = s.entries.begin(); it != s.entries.end(); )
    {
        if ( prefix.isEmpty() || it.key().startsWith( prefix ) )
        {
            it = s.entries.erase( it );
        }
        else
        {
            ++it;
        }
    }
    for ( const LogSourceEntry & entry : entries )
    {
        s.entries.insert( entry.path, entry );
    }
    if ( ! dir.isEmpty()
         && s.entries.contains( dir ) )
    {
        s.entries[dir].listed = true;
    }
    s.status.clear();
    s.failure = false;

    if ( index == currentSource() )
    {
        showEntries( s, dir );
        updateStatus();
    }
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::handleFailed( LogSource * source,
                                const QString & dir,
                                const QString & message )
{
    const int index = indexOf( source );
    if ( index < 0 ) return;
    Source & s = M_sources[index];
    s.status = message;
    s.failure = true;
    if ( ! dir.isEmpty() )
    {
        // asked again when it is opened next
        if ( QTreeWidgetItem * item = ( index == currentSource() ? M_dir_items.value( dir ) : nullptr ) )
        {
            item->setData( DATE_COLUMN, LISTED_ROLE, false );
            item->setExpanded( false );
        }
    }
    if ( index == currentSource() )
    {
        updateStatus();
    }
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::handleNoted( LogSource * source,
                               const QString & text )
{
    const int index = indexOf( source );
    if ( index < 0 ) return;
    Source & s = M_sources[index];
    s.status = s.status.isEmpty() ? text : s.status + QLatin1Char( ' ' ) + text;
    if ( index == currentSource() )
    {
        updateStatus();
    }
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::setFilter( const QString & text )
{
    if ( M_filter->text() != text )
    {
        // the field too: this is called again by it
        M_filter->setText( text );
        return;
    }
    const QString t = text.trimmed();

    // a log is shown if a team (or its name) has the text; a directory if
    // it has a log shown, its entries are not read yet (it may have one),
    // or nothing is filtered
    std::function< bool( QTreeWidgetItem * ) > apply = [&]( QTreeWidgetItem * item ) -> bool {
        bool shown = false;
        if ( item->data( DATE_COLUMN, PATH_ROLE ).isValid() )
        {
            for ( int i = 0; i < item->childCount(); ++i )
            {
                if ( apply( item->child( i ) ) ) shown = true;
            }
            if ( t.isEmpty()
                 || ! item->data( DATE_COLUMN, LISTED_ROLE ).toBool() )
            {
                shown = true;
            }
            else if ( shown )
            {
                item->setExpanded( true );
            }
        }
        else
        {
            shown = t.isEmpty()
                || item->text( LEFT_COLUMN ).contains( t, Qt::CaseInsensitive )
                || item->text( RIGHT_COLUMN ).contains( t, Qt::CaseInsensitive )
                || ( item->text( LEFT_COLUMN ).isEmpty()
                     && item->text( DATE_COLUMN ).contains( t, Qt::CaseInsensitive ) );
        }
        item->setHidden( ! shown );
        return shown;
    };

    QTreeWidgetItem * root = M_tree->invisibleRootItem();
    for ( int i = 0; i < root->childCount(); ++i )
    {
        apply( root->child( i ) );
    }
    updateStatus();
}

/*-------------------------------------------------------------------*/
QUrl
LogBrowserDialog::urlOf( const QTreeWidgetItem * item )
{
    return item ? item->data( DATE_COLUMN, URL_ROLE ).toUrl() : QUrl();
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::openCurrent()
{
    const QUrl url = urlOf( M_tree->currentItem() );
    if ( url.isValid() )
    {
        emit openRequested( url );
    }
}

/*-------------------------------------------------------------------*/
QString
LogBrowserDialog::statusText() const
{
    return M_status_label->text();
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::updateStatus()
{
    const int index = currentSource();
    if ( index < 0 )
    {
        M_status_label->clear();
        return;
    }
    const Source & s = M_sources[index];

    int logs = 0;
    int shown = 0;
    int unread = 0; //!< directories not read yet
    for ( QTreeWidgetItemIterator it( M_tree ); *it; ++it )
    {
        if ( ( *it )->data( DATE_COLUMN, PATH_ROLE ).isValid()
             && ! ( *it )->data( DATE_COLUMN, LISTED_ROLE ).toBool() )
        {
            ++unread;
        }
        if ( urlOf( *it ).isValid() )
        {
            ++logs;
            bool visible = ! ( *it )->isHidden();
            for ( QTreeWidgetItem * p = ( *it )->parent(); p && visible; p = p->parent() )
            {
                visible = ! p->isHidden();
            }
            if ( visible ) ++shown;
        }
    }

    QString text;
    if ( logs == 0 && unread > 0 )
    {
        // e.g. the archive: its logs are in the directories
        text = tr( "Open a directory to list its game logs." );
    }
    else if ( logs > 0 || s.status.isEmpty() )
    {
        // not "0 game logs." before e.g. "Reading the list..." or a failure

        // no translation: the plural by hand
        const QString count = ( logs == 1 ) ? tr( "1 game log" ) : tr( "%1 game logs" ).arg( logs );
        text = ( logs == shown )
            ? count + QLatin1Char( '.' )
            : tr( "%1 of %2." ).arg( shown ).arg( count );
    }
    if ( ! s.status.isEmpty() )
    {
        text = text.isEmpty() ? s.status : text + QLatin1Char( ' ' ) + s.status;
    }
    M_status_label->setText( text );
    UIPalette::markWrongText( M_status_label, s.failure );
}

/*-------------------------------------------------------------------*/
void
LogBrowserDialog::updateButtons()
{
    const int index = currentSource();
    M_remove_button->setEnabled( index >= 0 );
    M_reload_button->setEnabled( index >= 0 );
    M_open_button->setEnabled( urlOf( M_tree->currentItem() ).isValid() );
}
