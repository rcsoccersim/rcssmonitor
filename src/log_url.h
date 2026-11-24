// -*-c++-*-

/*!
  \file log_url.h
  \brief the URLs of game logs: what the user gives, GitHub's files and the
  name of a log. Header File.
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

#ifndef RCSSMONITOR_LOG_URL_H
#define RCSSMONITOR_LOG_URL_H

#include <QByteArray>
#include <QString>
#include <QUrl>

/*!
  \class LogUrl
  \brief the URLs of game logs (docs/plan/2026-11-22-02_remote-logs.md 4.1),
  ported from src/io/fetch-log.ts of rcssmonitor-web. It has no state: only
  static functions.

  Files on GitHub: a file page (github.com/OWNER/REPO/blob/REF/PATH) or a
  raw link (.../raw/REF/PATH) is read from its raw file,
  raw.githubusercontent.com/OWNER/REPO/REF/PATH, as GitHub's raw link
  redirects to. A file stored with Git LFS is a small pointer there, and
  its content is on media.githubusercontent.com.
*/
class LogUrl {
private:
    // not used
    LogUrl() = delete;

public:

    /*!
      \brief whether a game log given on the command line or dropped is an
      http:// or https:// URL (to download), not a local path
    */
    static
    bool isUrl( const QString & text );

    /*!
      \brief the URL of a game log from the text the user gave (trimmed): an
      http:// or https:// URL with a host, without a user name or password. A
      GitHub file page or raw link becomes the URL of its raw file.
      \param text what the user typed or gave
      \param error set to the reason when it is not one (may be null)
      \return the URL, or an invalid URL
    */
    static
    QUrl parse( const QString & text,
                QString * error );

    /*!
      \brief the raw file of a GitHub file page or raw link
      (github.com/OWNER/REPO/blob/REF/PATH or .../raw/REF/PATH; the branch may
      have slashes, GitHub resolves it). The query and the fragment are not
      part of the file.
      \return the URL on raw.githubusercontent.com, or an invalid URL for others
    */
    static
    QUrl githubRawUrl( const QUrl & url );

    /*!
      \brief where GitHub serves the Git LFS object of a
      raw.githubusercontent.com URL (media.githubusercontent.com/media/OWNER/REPO/REF/PATH)
      \return the URL, or an invalid URL for other hosts
    */
    static
    QUrl githubLfsUrl( const QUrl & url );

    /*!
      \brief the size of the file that a Git LFS pointer stands for, if the
      bytes are one (a few lines of text under 1024 bytes: version, oid, size)
      \return the size, or -1 if they are not a pointer
    */
    static
    qint64 lfsPointerSize( const QByteArray & bytes );

    /*!
      \brief the name of the log at a URL, to show: the last part of its path,
      decoded, or its host (and port) if the path has none. The raw text of
      the part if it cannot be decoded.
    */
    static
    QString name( const QUrl & url );
};

#endif
