// -*-c++-*-

/*!
  \file github_log_source.cpp
  \brief the game logs in a repository on GitHub. Source File.
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

#include "github_log_source.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>

#ifndef VERSION
#define VERSION "x.x.x"
#endif

const int GitHubLogSource::CACHE_SECONDS = 60 * 60;

namespace {

QUrl &
api_base()
{
    static QUrl s_url( QStringLiteral( "https://api.github.com" ) );
    return s_url;
}

QUrl &
raw_base()
{
    static QUrl s_url( QStringLiteral( "https://raw.githubusercontent.com" ) );
    return s_url;
}

//! a part of a path in a URL, encoded ('/' kept if keep_slash)
QString
encoded( const QString & part,
         const bool keep_slash = false )
{
    return QString::fromLatin1( QUrl::toPercentEncoding( part, keep_slash ? QByteArray( "/" ) : QByteArray() ) );
}

//! the time of a file or a reset, to show
QString
time_text( const QDateTime & time )
{
    return time.toLocalTime().toString( QStringLiteral( "yyyy-MM-dd HH:mm" ) );
}

}

/*-------------------------------------------------------------------*/
GitHubLogSource::GitHubLogSource( const QString & owner,
                                  const QString & repo,
                                  const QString & ref,
                                  const QString & cache_dir,
                                  QObject * parent )
    : LogSource( parent ),
      M_owner( owner ),
      M_repo( repo ),
      M_ref( ref.isEmpty() ? QStringLiteral( "HEAD" ) : ref ),
      M_cache_dir( cache_dir ),
      M_manager( new QNetworkAccessManager( this ) ),
      M_reply( nullptr ),
      M_generation( 0 )
{

}

/*-------------------------------------------------------------------*/
GitHubLogSource::~GitHubLogSource()
{
    abort();
}

/*-------------------------------------------------------------------*/
bool
GitHubLogSource::parseRepository( const QString & text,
                                  QString * owner,
                                  QString * repo,
                                  QString * ref )
{
    // a user or an organization: letters, digits and '-'; a repository:
    // letters, digits, '.', '_' and '-'
    static const QRegularExpression plain( QStringLiteral( "^([A-Za-z0-9-]+)/([A-Za-z0-9._-]+)(?:@(\\S+))?$" ) );
    static const QRegularExpression page( QStringLiteral( "^(?:https?://)?(?:www\\.)?github\\.com/([A-Za-z0-9-]+)/([A-Za-z0-9._-]+?)(?:\\.git)?(?:/tree/(\\S+?))?/?$" ) );

    const QString t = text.trimmed();
    QRegularExpressionMatch m = page.match( t );
    if ( ! m.hasMatch() )
    {
        m = plain.match( t );
    }
    if ( ! m.hasMatch()
         || m.captured( 2 ) == QLatin1String( "." )
         || m.captured( 2 ) == QLatin1String( ".." ) )
    {
        return false;
    }
    if ( owner ) *owner = m.captured( 1 );
    if ( repo ) *repo = m.captured( 2 );
    if ( ref ) *ref = m.captured( 3 ).isEmpty() ? QStringLiteral( "HEAD" ) : m.captured( 3 );
    return true;
}

/*-------------------------------------------------------------------*/
QString
GitHubLogSource::repositoryText( const QString & owner,
                                 const QString & repo,
                                 const QString & ref )
{
    const QString text = owner + QLatin1Char( '/' ) + repo;
    return ( ref.isEmpty() || ref == QLatin1String( "HEAD" ) )
        ? text
        : text + QLatin1Char( '@' ) + ref;
}

/*-------------------------------------------------------------------*/
QVector< LogSourceEntry >
GitHubLogSource::parseTree( const QByteArray & json,
                            const QString & raw_base,
                            bool * truncated,
                            QString * error )
{
    QVector< LogSourceEntry > entries;
    if ( truncated ) *truncated = false;
    if ( error ) error->clear();

    QJsonParseError parse_error;
    const QJsonDocument doc = QJsonDocument::fromJson( json, &parse_error );
    if ( parse_error.error != QJsonParseError::NoError
         || ! doc.isObject()
         || ! doc.object().value( QStringLiteral( "tree" ) ).isArray() )
    {
        if ( error ) *error = tr( "GitHub did not answer with a tree of files." );
        return entries;
    }

    const QJsonObject root = doc.object();
    if ( truncated ) *truncated = root.value( QStringLiteral( "truncated" ) ).toBool();

    for ( const QJsonValue & value : root.value( QStringLiteral( "tree" ) ).toArray() )
    {
        const QJsonObject item = value.toObject();
        const QString path = item.value( QStringLiteral( "path" ) ).toString();
        if ( item.value( QStringLiteral( "type" ) ).toString() != QLatin1String( "blob" )
             || ! isGameLog( path.section( QLatin1Char( '/' ), -1 ) ) )
        {
            continue;
        }

        LogSourceEntry entry;
        entry.path = path;
        entry.size = static_cast< qint64 >( item.value( QStringLiteral( "size" ) ).toDouble( -1 ) );
        entry.url = QUrl::fromEncoded( ( raw_base + QLatin1Char( '/' ) + encoded( path, true ) ).toUtf8() );
        entries.push_back( entry );
    }
    return entries;
}

/*-------------------------------------------------------------------*/
QString
GitHubLogSource::defaultCacheDir()
{
    return QDir( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) ).filePath( QStringLiteral( "github" ) );
}

/*-------------------------------------------------------------------*/
void
GitHubLogSource::setApiBase( const QUrl & url )
{
    api_base() = url;
}

/*-------------------------------------------------------------------*/
QUrl
GitHubLogSource::apiBase()
{
    return api_base();
}

/*-------------------------------------------------------------------*/
void
GitHubLogSource::setRawBase( const QUrl & url )
{
    raw_base() = url;
}

/*-------------------------------------------------------------------*/
QUrl
GitHubLogSource::rawBase()
{
    return raw_base();
}

/*-------------------------------------------------------------------*/
QString
GitHubLogSource::cachePath() const
{
    return QDir( M_cache_dir ).filePath( M_owner + QLatin1Char( '/' ) + M_repo + QLatin1Char( '/' )
                                         + encoded( M_ref ) + QStringLiteral( ".json" ) );
}

/*-------------------------------------------------------------------*/
QString
GitHubLogSource::name() const
{
    return repositoryText( M_owner, M_repo, M_ref );
}

/*-------------------------------------------------------------------*/
QString
GitHubLogSource::description() const
{
    return tr( "The game logs in the repository %1 on GitHub (%2)." )
        .arg( name(), M_ref == QLatin1String( "HEAD" ) ? tr( "its default branch" ) : M_ref );
}

/*-------------------------------------------------------------------*/
void
GitHubLogSource::list( const QString & /* dir */,
                       const bool reload )
{
    // the whole tree at once, whatever the directory
    abort();
    const int generation = M_generation;

    if ( ! reload )
    {
        const QFileInfo info( cachePath() );
        if ( info.isFile()
             && info.lastModified().secsTo( QDateTime::currentDateTime() ) < CACHE_SECONDS )
        {
            QTimer::singleShot( 0, this, [this, generation]() {
                if ( generation != M_generation ) return;
                if ( ! listCache( QString() ) )
                {
                    list( QString(), true );
                }
            } );
            return;
        }
    }

    const QUrl url( apiBase().toString() + QStringLiteral( "/repos/" ) + encoded( M_owner ) + QLatin1Char( '/' )
                    + encoded( M_repo ) + QStringLiteral( "/git/trees/" ) + encoded( M_ref, true )
                    + QStringLiteral( "?recursive=1" ) );
    QNetworkRequest request( url );
    request.setAttribute( QNetworkRequest::RedirectPolicyAttribute,
                          QNetworkRequest::NoLessSafeRedirectPolicy );
    request.setRawHeader( "Accept", "application/vnd.github+json" );
    request.setRawHeader( "X-GitHub-Api-Version", "2022-11-28" );
    // GitHub asks for one; nothing of the user
    request.setHeader( QNetworkRequest::UserAgentHeader, QStringLiteral( "rcssmonitor/" VERSION ) );
    request.setTransferTimeout( 30 * 1000 );

    M_reply = M_manager->get( request );
    connect( M_reply, &QNetworkReply::finished,
             this, &GitHubLogSource::handleReply );
}

/*-------------------------------------------------------------------*/
void
GitHubLogSource::abort()
{
    ++M_generation;
    if ( M_reply )
    {
        QNetworkReply * reply = M_reply;
        M_reply = nullptr;
        disconnect( reply, nullptr, this, nullptr );
        reply->abort();
        reply->deleteLater();
    }
}

/*-------------------------------------------------------------------*/
bool
GitHubLogSource::listCache( const QString & failure )
{
    QFile file( cachePath() );
    if ( ! file.open( QIODevice::ReadOnly ) )
    {
        return false;
    }
    const QDateTime time = QFileInfo( file ).lastModified();
    const QString base = rawBase().toString() + QLatin1Char( '/' ) + encoded( M_owner ) + QLatin1Char( '/' )
        + encoded( M_repo ) + QLatin1Char( '/' ) + encoded( M_ref, true );
    bool truncated = false;
    QString error;
    const QVector< LogSourceEntry > entries = parseTree( file.readAll(), base, &truncated, &error );
    if ( ! error.isEmpty() )
    {
        return false;
    }

    emit listed( QString(), entries );
    emit noted( failure.isEmpty()
                ? tr( "The list read at %1 (read again an hour later, or by Reload)." ).arg( time_text( time ) )
                : tr( "%1 The list read at %2 is shown." ).arg( failure, time_text( time ) ) );
    if ( truncated )
    {
        emit noted( tr( "GitHub gave only a part of the list: the repository is too large." ) );
    }
    return true;
}

/*-------------------------------------------------------------------*/
QString
GitHubLogSource::describe( QNetworkReply * reply ) const
{
    const int status = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    if ( ( status == 403 || status == 429 )
         && reply->rawHeader( "x-ratelimit-remaining" ) == "0" )
    {
        const qint64 reset = reply->rawHeader( "x-ratelimit-reset" ).toLongLong();
        return tr( "GitHub allows 60 requests an hour without an account, and they are used up until %1." )
            .arg( time_text( QDateTime::fromSecsSinceEpoch( reset ) ) );
    }
    if ( status == 404 )
    {
        return tr( "%1 is not found on GitHub: the repository is not public, or the branch is not there." )
            .arg( name() );
    }
    if ( status >= 400 )
    {
        const QString reason = reply->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).toString();
        return QStringLiteral( "HTTP %1 %2" ).arg( status ).arg( reason ).trimmed();
    }
    if ( reply->error() == QNetworkReply::OperationCanceledError
         || reply->error() == QNetworkReply::TimeoutError )
    {
        return tr( "GitHub did not answer for 30 seconds." );
    }
    return reply->errorString();
}

/*-------------------------------------------------------------------*/
void
GitHubLogSource::handleReply()
{
    QNetworkReply * reply = M_reply;
    if ( ! reply ) return;
    M_reply = nullptr;
    reply->deleteLater();

    // a failure: the list in the cache, however old, if there is one
    const auto fail = [this]( const QString & message ) {
        if ( ! listCache( message ) )
        {
            emit failed( QString(), message );
        }
    };

    if ( reply->error() != QNetworkReply::NoError )
    {
        fail( describe( reply ) );
        return;
    }

    const QByteArray json = reply->readAll();
    const QString base = rawBase().toString() + QLatin1Char( '/' ) + encoded( M_owner ) + QLatin1Char( '/' )
        + encoded( M_repo ) + QLatin1Char( '/' ) + encoded( M_ref, true );
    bool truncated = false;
    QString error;
    const QVector< LogSourceEntry > entries = parseTree( json, base, &truncated, &error );
    if ( ! error.isEmpty() )
    {
        fail( error );
        return;
    }

    // kept for an hour
    QDir().mkpath( QFileInfo( cachePath() ).absolutePath() );
    QSaveFile file( cachePath() );
    if ( file.open( QIODevice::WriteOnly ) )
    {
        file.write( json );
        file.commit();
    }

    emit listed( QString(), entries );
    if ( truncated )
    {
        emit noted( tr( "GitHub gave only a part of the list: the repository is too large." ) );
    }
}
