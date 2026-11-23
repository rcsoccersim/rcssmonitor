// -*-c++-*-

/*!
  \file github_log_source.h
  \brief the game logs in a repository on GitHub. Header File.
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

#ifndef RCSSMONITOR_GITHUB_LOG_SOURCE_H
#define RCSSMONITOR_GITHUB_LOG_SOURCE_H

#include "log_source.h"

#include <QByteArray>
#include <QDateTime>
#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;

/*!
  \class GitHubLogSource
  \brief the game logs in a public repository on GitHub
  (docs/plan/2026-11-22-02_remote-logs.md 4.2): the whole tree of the
  repository by one request of the Git Trees API
  (GET /repos/OWNER/REPO/git/trees/REF?recursive=1), and its files whose
  names are game logs, read from raw.githubusercontent.com.

  GitHub allows 60 requests an hour without an account, and a request
  answered "not modified" counts too. So the tree is kept in a cache file,
  and read again only after CACHE_SECONDS or when reloaded. When GitHub
  cannot be read (e.g. the limit is used up), the tree in the cache is
  shown, however old, with a note.
*/
class GitHubLogSource
    : public LogSource {

    Q_OBJECT

public:

    //! the repositories listed by default: the logs of RoboCup 2026
    static const QStringList DEFAULT_REPOSITORIES;
    //! how long the tree in the cache is used without asking GitHub (s)
    static const int CACHE_SECONDS;

private:

    QString M_owner;
    QString M_repo;
    QString M_ref; //!< the branch, the tag or the commit (HEAD: the default branch)
    QString M_cache_dir;

    QNetworkAccessManager * M_manager;
    QNetworkReply * M_reply;
    int M_generation; //!< counted up by list() and abort(), for a result told later

public:

    /*!
      \param owner the owner (a user or an organization)
      \param repo the repository
      \param ref the branch, the tag or the commit (empty: HEAD, the default branch)
      \param cache_dir where the trees are kept (defaultCacheDir())
    */
    GitHubLogSource( const QString & owner,
                     const QString & repo,
                     const QString & ref,
                     const QString & cache_dir = defaultCacheDir(),
                     QObject * parent = nullptr );
    ~GitHubLogSource() override;

    /*!
      \brief read a repository: OWNER/REPO, OWNER/REPO@REF, or the URL of a
      repository on GitHub (https://github.com/OWNER/REPO, .../tree/REF)
      \return false if it is not one
    */
    static
    bool parseRepository( const QString & text,
                          QString * owner,
                          QString * repo,
                          QString * ref );

    //! a repository as the user gives it: OWNER/REPO, or OWNER/REPO@REF
    static
    QString repositoryText( const QString & owner,
                            const QString & repo,
                            const QString & ref );

    /*!
      \brief the game logs in a tree answered by the Git Trees API
      \param json the answer
      \param raw_base where the files are: RAW/OWNER/REPO/REF (no '/' at the end)
      \param truncated set to whether GitHub gave only a part of the tree
      \param error set to the reason if it is not a tree
      \return the files, by their paths
    */
    static
    QVector< LogSourceEntry > parseTree( const QByteArray & json,
                                         const QString & raw_base,
                                         bool * truncated,
                                         QString * error );

    //! where the trees are kept: QStandardPaths::CacheLocation/github
    static
    QString defaultCacheDir();

    //! the server of the API (https://api.github.com; the tests give their own)
    static
    void setApiBase( const QUrl & url );
    static
    QUrl apiBase();

    //! the server of the files (https://raw.githubusercontent.com; the tests give their own)
    static
    void setRawBase( const QUrl & url );
    static
    QUrl rawBase();

    const QString & owner() const { return M_owner; }
    const QString & repo() const { return M_repo; }
    const QString & ref() const { return M_ref; }

    //! where the tree of this repository is kept
    QString cachePath() const;

    QString name() const override;
    QString description() const override;
    void list( const QString & dir,
               const bool reload ) override;
    void abort() override;

private:

    /*!
      \brief tell the tree in the cache file, with a note of its time
      \param failure why GitHub could not be read, or empty (the cache is new)
      \return false if there is none
    */
    bool listCache( const QString & failure );

    //! why a reply failed, for the user
    QString describe( QNetworkReply * reply ) const;

    void handleReply();
};

#endif
