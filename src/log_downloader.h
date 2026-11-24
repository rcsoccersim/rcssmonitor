// -*-c++-*-

/*!
  \file log_downloader.h
  \brief downloads a game log from a URL into a file. Header File.
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

#ifndef RCSSMONITOR_LOG_DOWNLOADER_H
#define RCSSMONITOR_LOG_DOWNLOADER_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QUrl>

#include <functional>

class QNetworkAccessManager;
class QNetworkReply;
class QSaveFile;

/*!
  \class LogDownloader
  \brief downloads a game log from an http:// or https:// URL into a file
  (docs/plan/2026-11-22-02_remote-logs.md 4.1), one at a time.

  - The body is written through QSaveFile: to a temporary file, renamed to
    PATH when it is complete, so that a file cut in the middle is never
    taken for a log (nor two downloads of a URL at once mixed).
  - Redirects are followed, but not from https to http
    (NoLessSafeRedirectPolicy).
  - A Git LFS pointer on GitHub is followed to the file it stands for
    (LogUrl::githubLfsUrl()), whose size is the total of the progress.
  - A download larger than maxBytes() fails: at once if the server tells
    its length, or when that much has come.
  - The requests have the User-Agent rcssmonitor/VERSION, without anything
    of the user.
*/
class LogDownloader
    : public QObject {

    Q_OBJECT

public:

    //! the largest download by default (200 MB; a game is a few MB in gzip)
    static const qint64 MAX_BYTES;
    //! the time without data after which a download fails by default (ms)
    static const int TRANSFER_TIMEOUT;

    //! the URL of the file that a Git LFS pointer read from a URL stands for (invalid for none)
    typedef std::function< QUrl( const QUrl & ) > LfsResolver;

private:

    QNetworkAccessManager * M_manager;
    QNetworkReply * M_reply;
    QSaveFile * M_file; //!< to PATH

    QUrl M_url; //!< the URL given
    QString M_path; //!< where the log goes
    qint64 M_max_bytes;
    int M_transfer_timeout;
    LfsResolver M_lfs_resolver;

    qint64 M_received; //!< the bytes of the current request
    QByteArray M_head; //!< its first bytes (a Git LFS pointer is under 1024)
    qint64 M_total; //!< the size of the file that a Git LFS pointer gave, or -1
    bool M_lfs; //!< reading the file of a Git LFS pointer
    QString M_error; //!< why the current request was aborted by this, if it was
    int M_generation; //!< counted up by start() and abort(), for a failure told later

    // not used
    LogDownloader( const LogDownloader & ) = delete;
    LogDownloader & operator=( const LogDownloader & ) = delete;

public:

    explicit LogDownloader( QObject * parent = nullptr );
    ~LogDownloader() override;

    qint64 maxBytes() const { return M_max_bytes; }
    void setMaxBytes( const qint64 bytes ) { M_max_bytes = bytes; }
    //! the time without data after which a download fails (ms; 0 for none)
    void setTransferTimeout( const int ms ) { M_transfer_timeout = ms; }
    //! where a Git LFS pointer is followed (LogUrl::githubLfsUrl() by default; the tests give their server's)
    void setLfsResolver( LfsResolver resolver ) { M_lfs_resolver = std::move( resolver ); }

    //! a download is going on
    bool isRunning() const;

    //! the URL of the download going on or done last
    const QUrl & url() const { return M_url; }

    /*!
      \brief download a URL into a file, stopping a download going on.
      finished() or failed() tells the end, not before this returns; nothing
      does if abort() stops it.
      \param url an http:// or https:// URL (LogUrl::parse())
      \param path where the log goes; its directory is made
    */
    void start( const QUrl & url,
                const QString & path );

public slots:

    //! stop the download going on, and remove what came (no signal)
    void abort();

signals:

    //! bytes received of the file, and its size if known (-1 if not): as the data comes
    void progress( qint64 received,
                   qint64 total );

    //! the log is complete at path
    void finished( const QString & path );

    //! the download failed (nothing is left at the path)
    void failed( const QString & message );

private:

    //! open the file to write (again)
    bool openFile();
    void request( const QUrl & url );
    void fail( const QString & message );
    //! fail after start() returns, unless the download is stopped or started again
    void failLater( const QString & message );
    void cleanUp();

    //! why a reply failed, for the user
    QString describe( QNetworkReply * reply ) const;

private slots:

    void readBody();
    void checkHeaders();
    void reportProgress( qint64 received,
                         qint64 total );
    void handleFinished();
};

#endif
