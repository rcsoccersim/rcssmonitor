// -*-c++-*-

/*!
  \file archive_log_source.h
  \brief the game logs in a directory listed by h5ai, as
  archive.robocup.info. Header File.
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

#ifndef RCSSMONITOR_ARCHIVE_LOG_SOURCE_H
#define RCSSMONITOR_ARCHIVE_LOG_SOURCE_H

#include "log_source.h"

#include <QHash>
#include <QSet>

class QNetworkAccessManager;
class QNetworkReply;

/*!
  \class ArchiveLogSource
  \brief the game logs under a directory of a web server listed by h5ai,
  as archive.robocup.info (docs/plan/2026-11-22-02_remote-logs.md 4.3):
  a directory at a time, read by the JSON API of h5ai
  (POST /, action=get&items[href]=DIR&items[what]=1) when its branch is
  opened. The directories under it are told as not read yet, and of the
  files only the game logs.

  The answer of each directory is kept in a cache file for CACHE_SECONDS
  (the archive hardly changes). Reload reads the root again, and the
  directories opened after it without the caches from before it. When the
  server cannot be read, the list in the cache is shown, however old, with
  a note.
*/
class ArchiveLogSource
    : public LogSource {

    Q_OBJECT

public:

    //! how long the list of a directory in the cache is used without asking the server (s)
    static const int CACHE_SECONDS;

private:

    QUrl M_url; //!< the root directory (its path ends with '/')
    QString M_cache_dir;

    QNetworkAccessManager * M_manager;
    QHash< QNetworkReply *, QString > M_replies; //!< the directory read by each request
    int M_generation; //!< counted up by abort(), for the lists from the cache told later
    bool M_reloaded; //!< the root was reloaded: the caches of the directories not read since are not used
    QSet< QString > M_read; //!< the directories read from the server since the root was reloaded

public:

    /*!
      \param url the root directory (parseUrl()), e.g. the 2D logs of
      archive.robocup.info, one of Options::DEFAULT_LOG_SOURCES
      \param cache_dir where the lists are kept (defaultCacheDir())
    */
    explicit ArchiveLogSource( const QUrl & url,
                               const QString & cache_dir = defaultCacheDir(),
                               QObject * parent = nullptr );
    ~ArchiveLogSource() override;

    /*!
      \brief read the URL of a directory: http:// or https:// with a host,
      not on GitHub (a repository there is a GitHubLogSource). '/' is added
      to the end of its path, and the query and the fragment are dropped.
      \param error set to the reason when it is not one (may be null)
      \return the URL, or an invalid URL
    */
    static
    QUrl parseUrl( const QString & text,
                   QString * error );

    /*!
      \brief the entries right under a directory, from the answer of h5ai
      (the items under the directories up to the root are not taken)
      \param json the answer
      \param dir_url the URL of the directory (its path ends with '/')
      \param dir the directory under the root ("" for the root), put before the names
      \param error set to the reason if it is not an answer of h5ai
      \return the directories (not read yet) and the game logs
    */
    static
    QVector< LogSourceEntry > parseItems( const QByteArray & json,
                                          const QUrl & dir_url,
                                          const QString & dir,
                                          QString * error );

    //! where the lists are kept: QStandardPaths::CacheLocation/archive
    static
    QString defaultCacheDir();

    const QUrl & url() const { return M_url; }

    //! the URL of a directory under the root ("" for the root)
    QUrl dirUrl( const QString & dir ) const;

    //! where the list of a directory is kept
    QString cachePath( const QString & dir ) const;

    QString name() const override;
    QString text() const override;
    QString description() const override;
    void list( const QString & dir,
               const bool reload ) override;
    void abort() override;

private:

    /*!
      \brief tell the list of a directory in its cache file, with a note
      \param failure why the server could not be read, or empty (the cache is new)
      \return false if there is none
    */
    bool listCache( const QString & dir,
                    const QString & failure );

    //! why a reply failed, for the user
    QString describe( QNetworkReply * reply ) const;

    void handleReply( QNetworkReply * reply );
};

#endif
