// -*-c++-*-

/*!
  \file log_browser_dialog.h
  \brief the dialog of the lists of game logs (File > Browse Game Logs).
  Header File.
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

#ifndef RCSSMONITOR_LOG_BROWSER_DIALOG_H
#define RCSSMONITOR_LOG_BROWSER_DIALOG_H

#include "log_source.h"

#include <QDialog>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QUrl>
#include <QVector>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

/*!
  \class LogBrowserDialog
  \brief File > Browse Game Logs (docs/plan/2026-11-22-02_remote-logs.md
  4.2, D9, D10): the sources of game logs on the left (the repositories on
  GitHub, the archive), and the logs of the one chosen on the right, as a
  tree of its directories with the date, the teams, the score and the size
  of each log (LogFileName), filtered by a team's name and sorted by a
  column. A log is opened by a double click or Open (openRequested()); the
  dialog stays open (modeless), to open one game after another. A
  directory whose entries are not known yet is read when it is opened.

  The user can add repositories on GitHub (OWNER/REPO or OWNER/REPO@REF),
  and remove those added (repositoriesChanged()).
*/
class LogBrowserDialog
    : public QDialog {

    Q_OBJECT

public:

    //! the columns of the tree
    enum Column {
        DATE_COLUMN = 0, //!< the date of a log (or its file name), the name of a directory
        LEFT_COLUMN,
        SCORE_COLUMN,
        RIGHT_COLUMN,
        SIZE_COLUMN,
        COLUMN_COUNT
    };

private:

    //! a source and what it told
    struct Source {
        LogSource * source = nullptr;
        bool removable = false; //!< a repository the user added
        bool requested = false; //!< its root was asked for
        QMap< QString, LogSourceEntry > entries; //!< by path
        QString status; //!< a note or a failure
        bool failure = false;
    };

    QVector< Source > M_sources;

    QListWidget * M_source_list;
    QPushButton * M_add_button;
    QPushButton * M_remove_button;
    QLineEdit * M_filter;
    QPushButton * M_reload_button;
    QTreeWidget * M_tree;
    QLabel * M_status_label;
    QPushButton * M_open_button;

    //! the items of the directories of the source shown, by path ("" is not in it)
    QHash< QString, QTreeWidgetItem * > M_dir_items;

public:

    explicit LogBrowserDialog( QWidget * parent = nullptr );
    ~LogBrowserDialog() override;

    /*!
      \brief add a source (the dialog takes it)
      \param removable whether the user can remove it (a repository added)
    */
    void addSource( LogSource * source,
                    const bool removable );

    int sourceCount() const { return M_sources.size(); }
    LogSource * source( const int index ) const;
    int currentSource() const;
    void setCurrentSource( const int index );

    /*!
      \brief add a repository on GitHub (OWNER/REPO, OWNER/REPO@REF or its URL)
      and show it
      \param error set to the reason if it is not added (may be null)
      \return false if it is not a repository, or it is there already
    */
    bool addRepository( const QString & text,
                        QString * error );

    //! the repositories the user added (OWNER/REPO or OWNER/REPO@REF)
    QStringList addedRepositories() const;

    //! the tree (for the tests)
    QTreeWidget * tree() const { return M_tree; }

    //! the text of the status (for the tests)
    QString statusText() const;

    //! the URL of a log's item, or an invalid one (a directory)
    static QUrl urlOf( const QTreeWidgetItem * item );

public slots:

    //! read the source shown again from its server
    void reload();

    //! show only the logs whose team (or file name) has the text (set in the field too)
    void setFilter( const QString & text );

    //! open the log chosen (openRequested())
    void openCurrent();

signals:

    //! a log to open
    void openRequested( const QUrl & url );

    //! the repositories added changed (addedRepositories())
    void repositoriesChanged( const QStringList & repositories );

private:

    int indexOf( const LogSource * source ) const;

    //! the item of a directory of the source shown, made with those above it
    QTreeWidgetItem * dirItem( const QString & path );

    //! show the entries of a source: all of them, or those under a directory
    void showEntries( const Source & source,
                      const QString & dir );

    //! an item of an entry, under the item of its directory
    void addItem( const LogSourceEntry & entry );

    void handleListed( LogSource * source,
                       const QString & dir,
                       const QVector< LogSourceEntry > & entries );
    void handleFailed( LogSource * source,
                       const QString & dir,
                       const QString & message );
    void handleNoted( LogSource * source,
                      const QString & text );

    void updateStatus();
    void updateButtons();

private slots:

    void showCurrentSource();
    void addRepositoryByDialog();
    void removeCurrentSource();
    void readDirectory( QTreeWidgetItem * item );
};

#endif
