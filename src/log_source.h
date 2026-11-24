// -*-c++-*-

/*!
  \file log_source.h
  \brief a source of game logs to list (GitHub, the archive). Header File.
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

#ifndef RCSSMONITOR_LOG_SOURCE_H
#define RCSSMONITOR_LOG_SOURCE_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVector>

/*!
  \struct LogSourceEntry
  \brief a game log file, or a directory, in a source
*/
struct LogSourceEntry {
    QString path; //!< under the source's root, separated by '/', without '/' at its ends
    bool directory = false;
    bool listed = true; //!< a directory whose entries are known (false: LogSource::list() it when it is opened)
    qint64 size = -1; //!< the bytes of a file, or -1 if not known
    QUrl url; //!< the URL of a file
};

/*!
  \class LogSource
  \brief a source of game logs to list in the dialog of the list
  (docs/plan/2026-11-22-02_remote-logs.md 4.2, 4.3): a repository on GitHub,
  the archive. It lists the entries under a directory: game log files (at
  any depth), and directories, whose entries may be listed later (when its
  branch is opened).
*/
class LogSource
    : public QObject {

    Q_OBJECT

public:

    explicit LogSource( QObject * parent = nullptr );
    ~LogSource() override;

    //! the name to show (e.g. "rcsoccersim/RoboCup2026")
    virtual QString name() const = 0;

    /*!
      \brief the text that makes this source again, as the user enters it
      (LogBrowserDialog::createSource(); kept in the settings): OWNER/REPO
      for GitHub, the URL for the archive
    */
    virtual QString text() const = 0;

    //! what it is, for a tip
    virtual QString description() const = 0;

    /*!
      \brief read the entries under a directory, and tell them by listed()
      or failed(), after this returns
      \param dir the directory ("" for the root)
      \param reload read them again from the server, not from the cache
    */
    virtual void list( const QString & dir,
                       const bool reload ) = 0;

    //! stop reading (nothing is told)
    virtual void abort() = 0;

    //! the file names that are game logs: .rcg and .rcg.gz
    static bool isGameLog( const QString & file_name );

signals:

    //! the entries under a directory
    void listed( const QString & dir,
                 const QVector< LogSourceEntry > & entries );

    //! reading a directory failed
    void failed( const QString & dir,
                 const QString & message );

    //! something to tell besides (e.g. the list is from the cache)
    void noted( const QString & text );
};

#endif
