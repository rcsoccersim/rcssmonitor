// -*-c++-*-

/*!
  \file archive_log_source.cpp
  \brief the game logs in a directory listed by a web server (h5ai, or a
  page of links), as archive.robocup.info. Source File.
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
#include <QRegularExpression>
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

//! a text of HTML with its character references decoded (&nbsp; as a space)
QString
decode_html( const QString & text )
{
    if ( ! text.contains( QLatin1Char( '&' ) ) )
    {
        return text;
    }

    static const QRegularExpression reference( QStringLiteral( "&(#[0-9]{1,7}|#[xX][0-9a-fA-F]{1,6}|[a-zA-Z]+);" ) );
    QString result;
    int last = 0;
    QRegularExpressionMatchIterator it = reference.globalMatch( text );
    while ( it.hasNext() )
    {
        const QRegularExpressionMatch m = it.next();
        result += text.mid( last, m.capturedStart() - last );
        last = m.capturedEnd();

        const QString name = m.captured( 1 );
        QString decoded;
        if ( name.startsWith( QLatin1Char( '#' ) ) )
        {
            const bool hex = name.size() > 1 && ( name[1] == QLatin1Char( 'x' ) || name[1] == QLatin1Char( 'X' ) );
            bool ok = false;
            const uint code = name.mid( hex ? 2 : 1 ).toUInt( &ok, hex ? 16 : 10 );
            if ( ok && 0 < code && code <= 0x10FFFF )
            {
                if ( QChar::requiresSurrogates( code ) )
                {
                    decoded += QChar( QChar::highSurrogate( code ) );
                    decoded += QChar( QChar::lowSurrogate( code ) );
                }
                else
                {
                    decoded = QChar( static_cast< ushort >( code ) );
                }
            }
        }
        else if ( name == QLatin1String( "amp" ) ) decoded = QStringLiteral( "&" );
        else if ( name == QLatin1String( "lt" ) ) decoded = QStringLiteral( "<" );
        else if ( name == QLatin1String( "gt" ) ) decoded = QStringLiteral( ">" );
        else if ( name == QLatin1String( "quot" ) ) decoded = QStringLiteral( "\"" );
        else if ( name == QLatin1String( "apos" ) ) decoded = QStringLiteral( "'" );
        else if ( name == QLatin1String( "nbsp" ) ) decoded = QStringLiteral( " " );

        result += decoded.isEmpty() ? m.captured( 0 ) : decoded;
    }
    result += text.mid( last );
    return result;
}

}

/*-------------------------------------------------------------------*/
ArchiveLogSource::ArchiveLogSource( const QUrl & url,
                                    const QString & cache_dir,
                                    QObject * parent )
    : LogSource( parent ),
      M_url( url ),
      M_cache_dir( cache_dir ),
      M_listing( UNKNOWN_LISTING ),
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
QVector< LogSourceEntry >
ArchiveLogSource::parseIndex( const QByteArray & html,
                              const QUrl & dir_url,
                              const QString & dir,
                              QString * error )
{
    QVector< LogSourceEntry > entries;
    if ( error ) error->clear();

    const QString page = QString::fromUtf8( html );
    static const QRegularExpression html_tag( QStringLiteral( "<(a|html|body|!doctype)[\\s>]" ),
                                              QRegularExpression::CaseInsensitiveOption );
    if ( ! html_tag.match( page ).hasMatch() )
    {
        if ( error ) *error = tr( "The server did not answer with a list of files." );
        return entries;
    }

    // <a href="...">, href quoted or not
    static const QRegularExpression link( QStringLiteral( "<a\\s(?:[^>]*\\s)?href\\s*=\\s*(?:\"([^\"]*)\"|'([^']*)'|([^\\s>]+))[^>]*>" ),
                                          QRegularExpression::CaseInsensitiveOption );
    static const QRegularExpression link_end( QStringLiteral( "</a\\s*>" ),
                                              QRegularExpression::CaseInsensitiveOption );
    // the text after a link: up to the next link, the end of the line, the row
    static const QRegularExpression text_end( QStringLiteral( "<a[\\s>]|</?tr[\\s>]|</?li[\\s>]|\\n" ),
                                              QRegularExpression::CaseInsensitiveOption );
    static const QRegularExpression tag( QStringLiteral( "<[^>]*>" ) );

    const QString scheme = dir_url.scheme();
    const int default_port = ( scheme == QLatin1String( "https" ) ? 443 : 80 );
    const QString dir_path = dir_url.path( QUrl::FullyDecoded );
    QHash< QString, int > found; // the index of each entry, by its name (with '/' for a directory)

    QRegularExpressionMatchIterator it = link.globalMatch( page );
    while ( it.hasNext() )
    {
        const QRegularExpressionMatch m = it.next();
        const QString href = decode_html( m.captured( 1 ) + m.captured( 2 ) + m.captured( 3 ) ).trimmed();
        if ( href.isEmpty() )
        {
            continue;
        }
        const QUrl url = dir_url.resolved( QUrl( href, QUrl::TolerantMode ) );
        if ( ! url.isValid()
             || url.scheme() != scheme
             || url.host().compare( dir_url.host(), Qt::CaseInsensitive ) != 0
             || url.port( default_port ) != dir_url.port( default_port )
             || url.hasQuery() // the links to sort the list
             || url.hasFragment() )
        {
            continue;
        }

        // right under the directory
        const QString path = url.path( QUrl::FullyDecoded );
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
             || name == QLatin1String( "." )
             || name == QLatin1String( ".." )
             || ( ! directory && ! isGameLog( name ) ) )
        {
            continue;
        }

        // the size in the text after the link
        qint64 size = -1;
        if ( ! directory )
        {
            const QRegularExpressionMatch end_of_link = link_end.match( page, m.capturedEnd() );
            const int start = end_of_link.hasMatch() ? end_of_link.capturedEnd() : m.capturedEnd();
            const QRegularExpressionMatch end_of_text = text_end.match( page, start );
            QString text = page.mid( start, ( end_of_text.hasMatch() ? end_of_text.capturedStart() : page.size() ) - start );
            text.replace( tag, QStringLiteral( " " ) );
            size = parseSize( decode_html( text ) );
        }

        const QString key = directory ? name + QLatin1Char( '/' ) : name;
        if ( found.contains( key ) )
        {
            // e.g. an icon linked to the same file
            LogSourceEntry & entry = entries[found.value( key )];
            if ( entry.size < 0 )
            {
                entry.size = size;
            }
            continue;
        }
        found.insert( key, entries.size() );

        LogSourceEntry entry;
        entry.path = dir.isEmpty() ? name : dir + QLatin1Char( '/' ) + name;
        if ( directory )
        {
            entry.directory = true;
            entry.listed = false; // read when it is opened
        }
        else
        {
            entry.size = size;
            entry.url = url;
        }
        entries.push_back( entry );
    }
    return entries;
}

/*-------------------------------------------------------------------*/
qint64
ArchiveLogSource::parseSize( const QString & text )
{
    // a word of a number, with a unit or not (not a part of a date or a time)
    static const QRegularExpression size( QStringLiteral( "(?:^|\\s)([0-9]+(?:\\.[0-9]+)?)\\s?([KMGT](?:i?B)?|B|bytes)?(?=\\s|$)" ),
                                          QRegularExpression::CaseInsensitiveOption );
    QRegularExpressionMatch last;
    QRegularExpressionMatchIterator it = size.globalMatch( text );
    while ( it.hasNext() )
    {
        last = it.next();
    }
    if ( ! last.hasMatch() )
    {
        return -1;
    }

    double bytes = last.captured( 1 ).toDouble();
    const QString unit = last.captured( 2 ).toUpper();
    // K: 1, M: 2, G: 3, T: 4; B and bytes: 0
    const int power = unit.isEmpty() ? 0 : QStringLiteral( "KMGT" ).indexOf( unit.at( 0 ) ) + 1;
    for ( int i = 0; i < power; ++i )
    {
        bytes *= 1024.0;
    }
    return static_cast< qint64 >( bytes + 0.5 );
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
ArchiveLogSource::cachePath( const QString & dir,
                             const Listing listing ) const
{
    const QByteArray hash = QCryptographicHash::hash( dirUrl( dir ).toString( QUrl::FullyEncoded ).toUtf8(),
                                                      QCryptographicHash::Sha1 ).toHex();
    return QDir( M_cache_dir ).filePath( M_url.host() + QLatin1Char( '/' )
                                         + QString::fromLatin1( hash.left( 16 ) )
                                         + ( listing == INDEX_LISTING ? QStringLiteral( ".html" ) : QStringLiteral( ".json" ) ) );
}

/*-------------------------------------------------------------------*/
ArchiveLogSource::Listing
ArchiveLogSource::cachedListing( const QString & dir ) const
{
    if ( M_listing != UNKNOWN_LISTING )
    {
        return QFileInfo::exists( cachePath( dir, M_listing ) ) ? M_listing : UNKNOWN_LISTING;
    }

    // not found out yet: the newer one
    const QFileInfo h5ai( cachePath( dir, H5AI_LISTING ) );
    const QFileInfo index( cachePath( dir, INDEX_LISTING ) );
    if ( h5ai.isFile() && ( ! index.isFile() || h5ai.lastModified() >= index.lastModified() ) )
    {
        return H5AI_LISTING;
    }
    return index.isFile() ? INDEX_LISTING : UNKNOWN_LISTING;
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
    return tr( "The game logs under %1 (listed by the web server: h5ai, or a page of links such as Apache's)."
               " A directory is read when it is opened." )
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
        // and how the server lists them, found out again
        M_listing = UNKNOWN_LISTING;
    }

    // one request at a time for a directory
    for ( auto it = M_replies.begin(); it != M_replies.end(); )
    {
        if ( it.value().dir == dir )
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
        const Listing cached = cachedListing( dir );
        const QFileInfo info( cachePath( dir, cached ) );
        if ( cached != UNKNOWN_LISTING
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

    // h5ai first, until the server is found to list the directories by pages
    request( dir, M_listing == INDEX_LISTING ? INDEX_LISTING : H5AI_LISTING );
}

/*-------------------------------------------------------------------*/
void
ArchiveLogSource::request( const QString & dir,
                           const Listing listing )
{
    const QUrl url = dirUrl( dir );
    QNetworkRequest request( url );
    request.setAttribute( QNetworkRequest::RedirectPolicyAttribute,
                          QNetworkRequest::NoLessSafeRedirectPolicy );
    // nothing of the user
    request.setHeader( QNetworkRequest::UserAgentHeader, QStringLiteral( "rcssmonitor/" VERSION ) );
    request.setTransferTimeout( 30 * 1000 );

    QNetworkReply * reply = nullptr;
    if ( listing == INDEX_LISTING )
    {
        // the page of the directory
        request.setRawHeader( "Accept", "text/html" );
        reply = M_manager->get( request );
    }
    else
    {
        // the API of h5ai, at the directory
        request.setHeader( QNetworkRequest::ContentTypeHeader, QStringLiteral( "application/x-www-form-urlencoded" ) );
        request.setRawHeader( "Accept", "application/json" );
        const QByteArray body = "action=get&items[href]="
            + QUrl::toPercentEncoding( url.path( QUrl::FullyEncoded ) )
            + "&items[what]=1";
        reply = M_manager->post( request, body );
    }

    M_replies.insert( reply, Request{ dir, listing } );
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
    const Listing listing = cachedListing( dir );
    if ( listing == UNKNOWN_LISTING )
    {
        return false;
    }
    QFile file( cachePath( dir, listing ) );
    if ( ! file.open( QIODevice::ReadOnly ) )
    {
        return false;
    }
    const QDateTime time = QFileInfo( file ).lastModified();
    QString error;
    const QVector< LogSourceEntry > entries = ( listing == INDEX_LISTING
                                                ? parseIndex( file.readAll(), dirUrl( dir ), dir, &error )
                                                : parseItems( file.readAll(), dirUrl( dir ), dir, &error ) );
    if ( ! error.isEmpty() )
    {
        return false;
    }
    if ( M_listing == UNKNOWN_LISTING )
    {
        M_listing = listing;
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
    const Request sent = M_replies.take( reply );
    const QString & dir = sent.dir;
    reply->deleteLater();

    // a failure: the list in the cache, however old, if there is one
    const auto fail = [this, &dir]( const QString & message ) {
        if ( ! listCache( dir, message ) )
        {
            emit failed( dir, message );
        }
    };

    QByteArray body;
    QVector< LogSourceEntry > entries;
    QString error;
    if ( reply->error() == QNetworkReply::NoError )
    {
        body = reply->readAll();
        entries = ( sent.listing == INDEX_LISTING
                    ? parseIndex( body, dirUrl( dir ), dir, &error )
                    : parseItems( body, dirUrl( dir ), dir, &error ) );
    }

    if ( reply->error() != QNetworkReply::NoError
         || ! error.isEmpty() )
    {
        // the server answered, but not by h5ai: the page of the directory
        if ( sent.listing == H5AI_LISTING
             && M_listing == UNKNOWN_LISTING
             && reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt() > 0 )
        {
            request( dir, INDEX_LISTING );
            return;
        }
        fail( reply->error() != QNetworkReply::NoError ? describe( reply ) : error );
        return;
    }

    if ( M_listing == UNKNOWN_LISTING )
    {
        M_listing = sent.listing;
    }

    // kept for a day
    M_read.insert( dir );
    const QString path = cachePath( dir, sent.listing );
    QDir().mkpath( QFileInfo( path ).absolutePath() );
    QSaveFile file( path );
    if ( file.open( QIODevice::WriteOnly ) )
    {
        file.write( body );
        file.commit();
    }

    emit listed( dir, entries );
}
