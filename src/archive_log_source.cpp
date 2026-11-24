// -*-c++-*-

/*!
  \file archive_log_source.cpp
  \brief the game logs in a directory listed by h5ai, as
  archive.robocup.info. Source File.
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

#include "archive_log_source.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>

#ifndef VERSION
#define VERSION "x.x.x"
#endif

const int ArchiveLogSource::CACHE_SECONDS = 24 * 60 * 60;

namespace {

//! the time of a list, to show
QString
time_text( const QDateTime & time )
{
    return time.toLocalTime().toString( QStringLiteral( "yyyy-MM-dd HH:mm" ) );
}

}

/*-------------------------------------------------------------------*/
ArchiveLogSource::ArchiveLogSource( const QUrl & url,
                                    const QString & cache_dir,
                                    QObject * parent )
    : LogSource( parent ),
      M_url( url ),
      M_cache_dir( cache_dir ),
      M_manager( new QNetworkAccessManager( this ) ),
      M_generation( 0 ),
      M_reloaded( false )
{

}

/*-------------------------------------------------------------------*/
ArchiveLogSource::~ArchiveLogSource()
{
    abort();
}

/*-------------------------------------------------------------------*/
QUrl
ArchiveLogSource::parseUrl( const QString & text,
                            QString * error )
{
    const QString t = text.trimmed();
    QUrl url( t, QUrl::StrictMode );
    if ( ! url.isValid()
         || ( url.scheme() != QLatin1String( "http" )
              && url.scheme() != QLatin1String( "https" ) )
         || url.host().isEmpty() )
    {
        if ( error ) *error = tr( "Not the URL of a directory: %1 (http:// or https://)" ).arg( t );
        return QUrl();
    }
    if ( ! url.userInfo().isEmpty() )
    {
        if ( error ) *error = tr( "A URL with a user name or a password is not used: %1" ).arg( t );
        return QUrl();
    }
    const QString host = url.host().toLower();
    if ( host == QLatin1String( "github.com" )
         || host == QLatin1String( "www.github.com" ) )
    {
        if ( error ) *error = tr( "%1 is on GitHub: give OWNER/REPO, or the URL of the repository." ).arg( t );
        return QUrl();
    }

    url.setQuery( QString() );
    url.setFragment( QString() );
    const QString path = url.path( QUrl::FullyEncoded );
    if ( ! path.endsWith( QLatin1Char( '/' ) ) )
    {
        url.setPath( path + QLatin1Char( '/' ), QUrl::TolerantMode );
    }
    if ( error ) error->clear();
    return url;
}

/*-------------------------------------------------------------------*/
QVector< LogSourceEntry >
ArchiveLogSource::parseItems( const QByteArray & json,
                              const QUrl & dir_url,
                              const QString & dir,
                              QString * error )
{
    QVector< LogSourceEntry > entries;
    if ( error ) error->clear();

    QJsonParseError parse_error;
    const QJsonDocument doc = QJsonDocument::fromJson( json, &parse_error );
    if ( parse_error.error != QJsonParseError::NoError
         || ! doc.isObject()
         || ! doc.object().value( QStringLiteral( "items" ) ).isArray() )
    {
        if ( error ) *error = tr( "The server did not answer with a list of files (h5ai)." );
        return entries;
    }

    // the answer has the directories up to the root, and those beside them:
    // only those right under the directory
    const QString dir_path = dir_url.path( QUrl::FullyDecoded );
    for ( const QJsonValue & value : doc.object().value( QStringLiteral( "items" ) ).toArray() )
    {
        const QJsonObject item = value.toObject();
        const QByteArray href = item.value( QStringLiteral( "href" ) ).toString().toUtf8();
        const QString path = QUrl::fromPercentEncoding( href );
        if ( ! path.startsWith( dir_path )
             || path.size() == dir_path.size() )
        {
            continue;
        }

        QString name = path.mid( dir_path.size() );
        const bool directory = name.endsWith( QLatin1Char( '/' ) );
        if ( directory )
        {
            name.chop( 1 );
        }
        if ( name.isEmpty()
             || name.contains( QLatin1Char( '/' ) )
             || ( ! directory && ! isGameLog( name ) ) )
        {
            continue;
        }

        LogSourceEntry entry;
        entry.path = dir.isEmpty() ? name : dir + QLatin1Char( '/' ) + name;
        if ( directory )
        {
            entry.directory = true;
            entry.listed = false; // read when it is opened
        }
        else
        {
            entry.size = static_cast< qint64 >( item.value( QStringLiteral( "size" ) ).toDouble( -1 ) );
            entry.url = dir_url.resolved( QUrl::fromEncoded( href ) );
        }
        entries.push_back( entry );
    }
    return entries;
}

/*-------------------------------------------------------------------*/
QString
ArchiveLogSource::defaultCacheDir()
{
    return QDir( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) ).filePath( QStringLiteral( "archive" ) );
}

/*-------------------------------------------------------------------*/
QUrl
ArchiveLogSource::dirUrl( const QString & dir ) const
{
    if ( dir.isEmpty() )
    {
        return M_url;
    }
    QString path = M_url.path( QUrl::FullyEncoded );
    for ( const QString & part : dir.split( QLatin1Char( '/' ) ) )
    {
        path += QString::fromLatin1( QUrl::toPercentEncoding( part ) ) + QLatin1Char( '/' );
    }
    QUrl url( M_url );
    url.setPath( path, QUrl::TolerantMode );
    return url;
}

/*-------------------------------------------------------------------*/
QString
ArchiveLogSource::cachePath( const QString & dir ) const
{
    const QByteArray hash = QCryptographicHash::hash( dirUrl( dir ).toString( QUrl::FullyEncoded ).toUtf8(),
                                                      QCryptographicHash::Sha1 ).toHex();
    return QDir( M_cache_dir ).filePath( M_url.host() + QLatin1Char( '/' )
                                         + QString::fromLatin1( hash.left( 16 ) ) + QStringLiteral( ".json" ) );
}

/*-------------------------------------------------------------------*/
QString
ArchiveLogSource::name() const
{
    // the host and the path: archive.robocup.info/Soccer/Simulation/2D/logs
    QString n = M_url.adjusted( QUrl::RemoveScheme | QUrl::StripTrailingSlash ).toDisplayString();
    if ( n.startsWith( QLatin1String( "//" ) ) )
    {
        n.remove( 0, 2 );
    }
    return n;
}

/*-------------------------------------------------------------------*/
QString
ArchiveLogSource::text() const
{
    return M_url.toString( QUrl::FullyEncoded );
}

/*-------------------------------------------------------------------*/
QString
ArchiveLogSource::description() const
{
    return tr( "The game logs under %1 (listed by h5ai). A directory is read when it is opened." )
        .arg( M_url.toDisplayString() );
}

/*-------------------------------------------------------------------*/
void
ArchiveLogSource::list( const QString & dir,
                        const bool reload )
{
    if ( reload && dir.isEmpty() )
    {
        // the directories opened after this too (not by the times of the
        // files: they may be in the same millisecond)
        M_reloaded = true;
        M_read.clear();
    }

    // one request at a time for a directory
    for ( auto it = M_replies.begin(); it != M_replies.end(); )
    {
        if ( it.value() == dir )
        {
            QNetworkReply * reply = it.key();
            it = M_replies.erase( it );
            disconnect( reply, nullptr, this, nullptr );
            reply->abort();
            reply->deleteLater();
        }
        else
        {
            ++it;
        }
    }

    if ( ! reload )
    {
        const QFileInfo info( cachePath( dir ) );
        if ( info.isFile()
             && info.lastModified().secsTo( QDateTime::currentDateTime() ) < CACHE_SECONDS
             && ( ! M_reloaded || M_read.contains( dir ) ) )
        {
            const int generation = M_generation;
            QTimer::singleShot( 0, this, [this, dir, generation]() {
                if ( generation != M_generation ) return;
                if ( ! listCache( dir, QString() ) )
                {
                    list( dir, true );
                }
            } );
            return;
        }
    }

    // the API of h5ai, at the directory
    const QUrl url = dirUrl( dir );
    QNetworkRequest request( url );
    request.setAttribute( QNetworkRequest::RedirectPolicyAttribute,
                          QNetworkRequest::NoLessSafeRedirectPolicy );
    request.setHeader( QNetworkRequest::ContentTypeHeader, QStringLiteral( "application/x-www-form-urlencoded" ) );
    request.setRawHeader( "Accept", "application/json" );
    // nothing of the user
    request.setHeader( QNetworkRequest::UserAgentHeader, QStringLiteral( "rcssmonitor/" VERSION ) );
    request.setTransferTimeout( 30 * 1000 );
    const QByteArray body = "action=get&items[href]="
        + QUrl::toPercentEncoding( url.path( QUrl::FullyEncoded ) )
        + "&items[what]=1";

    QNetworkReply * reply = M_manager->post( request, body );
    M_replies.insert( reply, dir );
    connect( reply, &QNetworkReply::finished,
             this, [this, reply]() { handleReply( reply ); } );
}

/*-------------------------------------------------------------------*/
void
ArchiveLogSource::abort()
{
    ++M_generation;
    const QList< QNetworkReply * > replies = M_replies.keys();
    M_replies.clear();
    for ( QNetworkReply * reply : replies )
    {
        disconnect( reply, nullptr, this, nullptr );
        reply->abort();
        reply->deleteLater();
    }
}

/*-------------------------------------------------------------------*/
bool
ArchiveLogSource::listCache( const QString & dir,
                             const QString & failure )
{
    QFile file( cachePath( dir ) );
    if ( ! file.open( QIODevice::ReadOnly ) )
    {
        return false;
    }
    const QDateTime time = QFileInfo( file ).lastModified();
    QString error;
    const QVector< LogSourceEntry > entries = parseItems( file.readAll(), dirUrl( dir ), dir, &error );
    if ( ! error.isEmpty() )
    {
        return false;
    }

    emit listed( dir, entries );
    emit noted( failure.isEmpty()
                ? tr( "The list read at %1 (read again a day later, or by Reload)." ).arg( time_text( time ) )
                : tr( "%1 The list read at %2 is shown." ).arg( failure, time_text( time ) ) );
    return true;
}

/*-------------------------------------------------------------------*/
QString
ArchiveLogSource::describe( QNetworkReply * reply ) const
{
    const int status = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    if ( status == 404 )
    {
        return tr( "%1 is not found on the server." ).arg( reply->request().url().toDisplayString() );
    }
    if ( status >= 400 )
    {
        const QString reason = reply->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).toString();
        return QStringLiteral( "HTTP %1 %2" ).arg( status ).arg( reason ).trimmed();
    }
    if ( reply->error() == QNetworkReply::OperationCanceledError
         || reply->error() == QNetworkReply::TimeoutError )
    {
        return tr( "%1 did not answer for 30 seconds." ).arg( M_url.host() );
    }
    return reply->errorString();
}

/*-------------------------------------------------------------------*/
void
ArchiveLogSource::handleReply( QNetworkReply * reply )
{
    if ( ! M_replies.contains( reply ) )
    {
        return;
    }
    const QString dir = M_replies.take( reply );
    reply->deleteLater();

    // a failure: the list in the cache, however old, if there is one
    const auto fail = [this, &dir]( const QString & message ) {
        if ( ! listCache( dir, message ) )
        {
            emit failed( dir, message );
        }
    };

    if ( reply->error() != QNetworkReply::NoError )
    {
        fail( describe( reply ) );
        return;
    }

    const QByteArray json = reply->readAll();
    QString error;
    const QVector< LogSourceEntry > entries = parseItems( json, dirUrl( dir ), dir, &error );
    if ( ! error.isEmpty() )
    {
        fail( error );
        return;
    }

    // kept for a day
    M_read.insert( dir );
    QDir().mkpath( QFileInfo( cachePath( dir ) ).absolutePath() );
    QSaveFile file( cachePath( dir ) );
    if ( file.open( QIODevice::WriteOnly ) )
    {
        file.write( json );
        file.commit();
    }

    emit listed( dir, entries );
}
