// -*-c++-*-

/*!
  \file log_downloader.cpp
  \brief downloads a game log from a URL into a file. Source File.
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

#include "log_downloader.h"

#include "log_url.h"

#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#if QT_CONFIG(ssl)
#include <QSslSocket>
#endif

#ifndef VERSION
#define VERSION "x.x.x"
#endif

const qint64 LogDownloader::MAX_BYTES = 200 * 1024 * 1024;
const int LogDownloader::TRANSFER_TIMEOUT = 30 * 1000;

/*-------------------------------------------------------------------*/
LogDownloader::LogDownloader( QObject * parent )
    : QObject( parent ),
      M_manager( new QNetworkAccessManager( this ) ),
      M_reply( nullptr ),
      M_file( nullptr ),
      M_max_bytes( MAX_BYTES ),
      M_transfer_timeout( TRANSFER_TIMEOUT ),
      M_lfs_resolver( &githubLfsUrl ),
      M_received( 0 ),
      M_total( -1 ),
      M_lfs( false ),
      M_generation( 0 )
{

}

/*-------------------------------------------------------------------*/
LogDownloader::~LogDownloader()
{
    abort();
}

/*-------------------------------------------------------------------*/
bool
LogDownloader::isRunning() const
{
    return M_reply != nullptr;
}

/*-------------------------------------------------------------------*/
void
LogDownloader::start( const QUrl & url,
                      const QString & path )
{
    abort();

    M_url = url;
    M_path = path;
    M_total = -1;
    M_lfs = false;

    if ( url.scheme().toLower() == QLatin1String( "https" ) )
    {
#if QT_CONFIG(ssl)
        const bool tls = QSslSocket::supportsSsl();
#else
        const bool tls = false;
#endif
        if ( ! tls )
        {
            failLater( tr( "https:// is not available: Qt cannot use TLS here. "
                           "Install OpenSSL (e.g. libssl3 on Ubuntu), or use an http:// URL." ) );
            return;
        }
    }

    if ( ! QDir().mkpath( QFileInfo( path ).absolutePath() )
         || ! openFile() )
    {
        failLater( tr( "Cannot write %1." ).arg( QDir::toNativeSeparators( path ) ) );
        return;
    }

    request( url );
}

/*-------------------------------------------------------------------*/
bool
LogDownloader::openFile()
{
    if ( M_file )
    {
        M_file->cancelWriting();
        delete M_file;
    }
    M_file = new QSaveFile( M_path, this );
    return M_file->open( QIODevice::WriteOnly );
}

/*-------------------------------------------------------------------*/
void
LogDownloader::request( const QUrl & url )
{
    QNetworkRequest req( url );
    // followed, but not from https to http (the default of Qt6; Qt5 follows none)
    req.setAttribute( QNetworkRequest::RedirectPolicyAttribute,
                      QNetworkRequest::NoLessSafeRedirectPolicy );
    req.setMaximumRedirectsAllowed( 10 );
    // GitHub asks for one; nothing of the user
    req.setHeader( QNetworkRequest::UserAgentHeader, QStringLiteral( "rcssmonitor/" VERSION ) );
    req.setTransferTimeout( M_transfer_timeout );

    M_received = 0;
    M_head.clear();
    M_error.clear();
    M_reply = M_manager->get( req );
    connect( M_reply, &QNetworkReply::metaDataChanged,
             this, &LogDownloader::checkHeaders );
    connect( M_reply, &QNetworkReply::readyRead,
             this, &LogDownloader::readBody );
    connect( M_reply, &QNetworkReply::finished,
             this, &LogDownloader::handleFinished );
}

/*-------------------------------------------------------------------*/
void
LogDownloader::checkHeaders()
{
    if ( ! M_reply ) return;

    const QVariant length = M_reply->header( QNetworkRequest::ContentLengthHeader );
    const int status = M_reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    if ( status < 300
         && length.isValid()
         && length.toLongLong() > M_max_bytes )
    {
        M_error = tr( "The file is larger than the limit (%1 MB)." ).arg( M_max_bytes / ( 1024 * 1024 ) );
        M_reply->abort();
    }
}

/*-------------------------------------------------------------------*/
void
LogDownloader::readBody()
{
    if ( ! M_reply
         || ! M_file
         || ! M_error.isEmpty() )
    {
        return;
    }

    const QByteArray data = M_reply->readAll();
    M_received += data.size();
    if ( M_head.size() < 1024 )
    {
        M_head.append( data.left( 1024 - M_head.size() ) );
    }

    if ( M_received > M_max_bytes )
    {
        M_error = tr( "The file is larger than the limit (%1 MB)." ).arg( M_max_bytes / ( 1024 * 1024 ) );
        M_reply->abort();
        return;
    }
    if ( M_file->write( data ) != data.size() )
    {
        M_error = tr( "Cannot write %1: %2" ).arg( QDir::toNativeSeparators( M_path ), M_file->errorString() );
        M_reply->abort();
        return;
    }

    // as the data comes (QNetworkReply::downloadProgress() may not tell each)
    if ( ! data.isEmpty()
         && ! ( M_lfs == false && M_received < 1024 && lfsPointerSize( M_head ) >= 0 ) )
    {
        const QVariant length = M_reply->header( QNetworkRequest::ContentLengthHeader );
        reportProgress( M_received, length.isValid() ? length.toLongLong() : -1 );
    }
}

/*-------------------------------------------------------------------*/
void
LogDownloader::reportProgress( qint64 received,
                               qint64 total )
{
    if ( M_lfs && M_total >= 0 )
    {
        total = M_total;
    }
    emit progress( received, total > 0 ? total : -1 );
}

/*-------------------------------------------------------------------*/
void
LogDownloader::handleFinished()
{
    QNetworkReply * reply = M_reply;
    if ( ! reply ) return;

    if ( M_error.isEmpty()
         && reply->error() == QNetworkReply::NoError )
    {
        readBody(); // the rest
    }

    M_reply = nullptr;
    reply->deleteLater();

    if ( ! M_error.isEmpty() )
    {
        fail( M_error );
        return;
    }
    if ( reply->error() != QNetworkReply::NoError )
    {
        fail( describe( reply ) );
        return;
    }

    // a Git LFS pointer instead of the file (a file of a repository on
    // GitHub stored with Git LFS): the file it stands for
    if ( ! M_lfs
         && M_received < 1024 )
    {
        const qint64 size = lfsPointerSize( M_head );
        if ( size >= 0 )
        {
            const QUrl media = ( M_lfs_resolver ? M_lfs_resolver( reply->url() ) : QUrl() );
            if ( ! media.isValid() )
            {
                fail( tr( "The server gave a Git LFS pointer instead of the file." ) );
                return;
            }
            if ( size > M_max_bytes )
            {
                fail( tr( "The file is larger than the limit (%1 MB)." ).arg( M_max_bytes / ( 1024 * 1024 ) ) );
                return;
            }
            M_lfs = true;
            M_total = size;
            if ( ! openFile() )
            {
                fail( tr( "Cannot write %1." ).arg( QDir::toNativeSeparators( M_path ) ) );
                return;
            }
            request( media );
            return;
        }
    }

    if ( ! M_file->commit() )
    {
        const QString message = tr( "Cannot write %1: %2" ).arg( QDir::toNativeSeparators( M_path ),
                                                                M_file->errorString() );
        delete M_file;
        M_file = nullptr;
        fail( message );
        return;
    }
    delete M_file;
    M_file = nullptr;

    emit finished( M_path );
}

/*-------------------------------------------------------------------*/
void
LogDownloader::abort()
{
    ++M_generation;
    cleanUp();
}

/*-------------------------------------------------------------------*/
void
LogDownloader::cleanUp()
{
    if ( M_reply )
    {
        QNetworkReply * reply = M_reply;
        M_reply = nullptr;
        disconnect( reply, nullptr, this, nullptr );
        reply->abort();
        reply->deleteLater();
    }
    if ( M_file )
    {
        M_file->cancelWriting();
        delete M_file;
        M_file = nullptr;
    }
}

/*-------------------------------------------------------------------*/
void
LogDownloader::fail( const QString & message )
{
    cleanUp();
    emit failed( message );
}

/*-------------------------------------------------------------------*/
void
LogDownloader::failLater( const QString & message )
{
    const int generation = M_generation;
    QMetaObject::invokeMethod( this,
                               [this, generation, message]() {
                                   if ( generation == M_generation )
                                   {
                                       fail( message );
                                   }
                               },
                               Qt::QueuedConnection );
}

/*-------------------------------------------------------------------*/
QString
LogDownloader::describe( QNetworkReply * reply ) const
{
    const int status = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    if ( status >= 400 )
    {
        const QString reason = reply->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).toString();
        return reason.isEmpty()
            ? QStringLiteral( "HTTP %1" ).arg( status )
            : QStringLiteral( "HTTP %1 %2" ).arg( status ).arg( reason );
    }
    // this aborts a reply only after disconnecting it (abort()) or with a
    // reason (M_error): canceled otherwise is the transfer timeout
    if ( reply->error() == QNetworkReply::OperationCanceledError
         && M_transfer_timeout > 0 )
    {
        return tr( "No data came for %1 seconds." ).arg( M_transfer_timeout / 1000.0 );
    }
    return reply->errorString();
}
