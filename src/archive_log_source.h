// -*-c++-*-

/*!
  \file archive_log_source.h
  \brief the game logs in a directory listed by a web server (h5ai, or a
  page of links), as archive.robocup.info. Header File.
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
  \brief the game logs under a directory of a web server, as
  archive.robocup.info (docs/plan/2026-11-22-02_remote-logs.md 4.3, 4.5):
  a directory at a time, read when its branch is opened. The directories
  under it are told as not read yet, and of the files only the game logs.

  The list of a directory is read by the JSON API of h5ai (a POST to the
  directory, action=get&items[href]=DIR&items[what]=1), or from the page of
  links the server shows (the autoindex of Apache, nginx and so on; GET).
  Which one is found out by the first directory read (listing()): h5ai is
  tried first, and when the server answers but not by h5ai, the page is
  read. Reload of the root finds it out again.

  The answer of each directory is kept in a cache file for CACHE_SECONDS
  (the archives hardly change). Reload reads the root again, and the
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

    //! how the server lists a directory
    enum Listing {
        UNKNOWN_LISTING = 0, //!< not found out yet
        H5AI_LISTING, //!< the JSON API of h5ai
        INDEX_LISTING, //!< a page of links (Apache, nginx, ...)
    };

private:

    //! a request for the list of a directory
    struct Request {
        QString dir;
        Listing listing;
    };

    QUrl M_url; //!< the root directory (its path ends with '/')
    QString M_cache_dir;
    Listing M_listing; //!< found out by the first directory read

    QNetworkAccessManager * M_manager;
    QHash< QNetworkReply *, Request > M_replies; //!< the directory read by each request
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

    /*!
      \brief the entries right under a directory, from the page of links of
      a web server (the autoindex of Apache, nginx, Python's http.server,
      the page of h5ai): the links to the directories and the game logs
      right under it, each once. The sizes are read from the text after the
      links (parseSize()), -1 if none.
      \param html the page
      \param dir_url the URL of the directory (its path ends with '/')
      \param dir the directory under the root ("" for the root), put before the names
      \param error set to the reason if it is not a page of HTML
      \return the directories (not read yet) and the game logs
    */
    static
    QVector< LogSourceEntry > parseIndex( const QByteArray & html,
                                          const QUrl & dir_url,
                                          const QString & dir,
                                          QString * error );

    /*!
      \brief the size given in the text after a link of a page of links:
      the last word of a number with a unit or not ("5.0M", "512K",
      "5123 KB", "1.2 MiB", "5242880"; a unit is 1024 times the one below)
      \return the bytes (rounded values as they are), or -1 if none
    */
    static
    qint64 parseSize( const QString & text );

    //! where the lists are kept: QStandardPaths::CacheLocation/archive
    static
    QString defaultCacheDir();

    const QUrl & url() const { return M_url; }

    //! how the server lists the directories, UNKNOWN_LISTING until a directory is read
    Listing listing() const { return M_listing; }

    //! the URL of a directory under the root ("" for the root)
    QUrl dirUrl( const QString & dir ) const;

    /*!
      \brief where the list of a directory is kept
      \param listing H5AI_LISTING (.json) or INDEX_LISTING (.html)
    */
    QString cachePath( const QString & dir,
                       const Listing listing ) const;

    QString name() const override;
    QString text() const override;
    QString description() const override;
    void list( const QString & dir,
               const bool reload ) override;
    void abort() override;

private:

    //! the kind of the cache file of a directory to use (UNKNOWN_LISTING if none)
    Listing cachedListing( const QString & dir ) const;

    //! ask the server for the list of a directory
    void request( const QString & dir,
                  const Listing listing );

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
