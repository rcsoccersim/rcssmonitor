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

#include "log_filter.h"
#include "log_source.h"

#include <QDialog>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QUrl>
#include <QVector>

class QCheckBox;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

/*!
  \class LogBrowserDialog
  \brief File > Browse Game Logs (docs/plan/2026-11-22-02_remote-logs.md
  4.2, 4.3, D9, D10, D13): the sources of game logs on the left (the
  repositories on GitHub, the directories listed by h5ai as the archive),
  and the logs of the one chosen on the right, as a
  tree of its directories with the date, the teams, the score and the size
  of each log (LogFileName), filtered by a team's name and sorted by a
  column (LogFilter: a team on a side, its opponent and result, a range of
  dates; 4.7, D14). A log is opened by a double click or Open (openRequested()); the
  dialog stays open (modeless), to open one game after another. A
  directory whose entries are not known yet is read when it is opened.

  The user can add sources (createSource(): OWNER/REPO, OWNER/REPO@REF, the
  URL of a directory listed by h5ai), the default ones removed too, and
  remove any (sourcesChanged()).
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
        bool requested = false; //!< its root was asked for
        QMap< QString, LogSourceEntry > entries; //!< by path
        QString status; //!< a note or a failure
        bool failure = false;
    };

    QVector< Source > M_sources;

    QListWidget * M_source_list;
    QPushButton * M_add_button;
    QPushButton * M_remove_button;
    // the filter (LogFilter)
    QLineEdit * M_team;
    QComboBox * M_side;
    QLineEdit * M_opponent;
    QComboBox * M_result;
    QCheckBox * M_from_check;
    QDateEdit * M_from;
    QCheckBox * M_to_check;
    QDateEdit * M_to;
    QPushButton * M_clear_button;
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
      \brief make a source from the text the user gives: a repository on
      GitHub (OWNER/REPO, OWNER/REPO@REF or its URL), or the URL of a
      directory listed by h5ai (as archive.robocup.info)
      \param error set to the reason if it is not one (may be null)
      \return the source (not read yet), or null
    */
    static
    LogSource * createSource( const QString & text,
                              QString * error );

    //! add a source (the dialog takes it), not chosen if another is
    void addSource( LogSource * source );

    /*!
      \brief set the sources (those of the settings), the first one chosen;
      the texts that are not sources (createSource()) are left out
    */
    void setSources( const QStringList & texts );

    int sourceCount() const { return M_sources.size(); }
    LogSource * source( const int index ) const;
    int currentSource() const;
    void setCurrentSource( const int index );

    /*!
      \brief add a source the user gives (createSource()) and show it
      (sourcesChanged())
      \param error set to the reason if it is not added (may be null)
      \return false if it is not a source, or it is there already
    */
    bool addSource( const QString & text,
                    QString * error );

    //! the texts of the sources (LogSource::text()), to keep in the settings
    QStringList sourceTexts() const;

    //! the default sources not in the list (offered by Add...)
    QStringList missingDefaultSources() const;

    //! the tree (for the tests)
    QTreeWidget * tree() const { return M_tree; }

    //! the text of the status (for the tests)
    QString statusText() const;

    //! the URL of a log's item, or an invalid one (a directory)
    static QUrl urlOf( const QTreeWidgetItem * item );

    //! the filter of the fields
    LogFilter filter() const;

    //! set the fields of the filter, and show the logs that match
    void setFilter( const LogFilter & filter );

public slots:

    //! read the source shown again from its server
    void reload();

    //! show only the logs that match the filter of the fields
    void applyFilter();

    //! no filter: all the logs shown
    void clearFilter();

    //! open the log chosen (openRequested())
    void openCurrent();

signals:

    //! a log to open
    void openRequested( const QUrl & url );

    //! a source was added or removed (sourceTexts())
    void sourcesChanged( const QStringList & sources );

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

    //! the dates of the filter not checked: the oldest and the newest days of the logs listed
    void updateDateRange();

    void updateStatus();
    void updateButtons();

private slots:

    void showCurrentSource();
    void addSourceByDialog();
    void removeCurrentSource();
    void readDirectory( QTreeWidgetItem * item );
};

#endif
