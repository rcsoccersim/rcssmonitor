// -*-c++-*-

/*!
  \file log_cache.h
  \brief the cache of the game logs downloaded from URLs. Header File.
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

#ifndef RCSSMONITOR_LOG_CACHE_H
#define RCSSMONITOR_LOG_CACHE_H

#include <QString>
#include <QUrl>

/*!
  \class LogCache
  \brief the cache of the game logs downloaded from URLs, in a directory
  (docs/plan/2026-11-22-02_remote-logs.md D3): a log is kept at DIR/KEY/NAME,
  KEY the first 16 hex digits of the SHA-1 of its URL, and NAME its name
  (LogUrl::name()) made safe for a file name, so that the file has the log's
  name (the window's title, Save a Copy). A URL opened again is opened from
  there.
*/
class LogCache {
private:

    QString M_dir;

public:

    /*!
      \brief the cache in a directory
      \param dir the directory (made when a log is downloaded into it)
    */
    explicit LogCache( const QString & dir = defaultDir() );

    /*!
      \brief the cache directory of rcssmonitor:
      QStandardPaths::CacheLocation/logs (e.g. ~/.cache/rcssmonitor/logs)
    */
    static
    QString defaultDir();

    //! the directory
    const QString & dir() const { return M_dir; }

    //! where the log of a URL is kept (whether it is there or not)
    QString pathOf( const QUrl & url ) const;

    //! the log of a URL is there (downloaded completely before)
    bool contains( const QUrl & url ) const;

    //! the bytes of the files in the directory (0 if it is not there)
    qint64 size() const;

    //! remove the directory and all in it
    bool clear() const;
};

#endif
