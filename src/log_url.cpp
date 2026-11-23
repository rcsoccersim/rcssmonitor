// -*-c++-*-

/*!
  \file log_url.cpp
  \brief the URLs of game logs: what the user gives, GitHub's files, the
  name of a log and where it is kept. Source File.
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

#include "log_url.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringList>

namespace {

const char * const LFS_VERSION = "version https://git-lfs.github.com/spec/v1\n";

//! the messages of parseLogUrl(), translated in the context of LogUrl
QString
tr( const char * text )
{
    return QCoreApplication::translate( "LogUrl", text );
}

}

/*-------------------------------------------------------------------*/
bool
isLogUrl( const QString & text )
{
    const QString t = text.trimmed();
    return t.startsWith( QLatin1String( "http://" ), Qt::CaseInsensitive )
        || t.startsWith( QLatin1String( "https://" ), Qt::CaseInsensitive );
}

/*-------------------------------------------------------------------*/
QUrl
parseLogUrl( const QString & text,
             QString * error )
{
    const auto fail = [error]( const QString & message ) {
        if ( error ) *error = message;
        return QUrl();
    };

    const QString t = text.trimmed();
    if ( t.isEmpty() )
    {
        return fail( tr( "No URL is given." ) );
    }

    const QUrl url( t, QUrl::StrictMode );
    if ( ! url.isValid()
         || url.scheme().isEmpty() )
    {
        return fail( tr( "Not a URL: %1" ).arg( t ) );
    }
    const QString scheme = url.scheme().toLower();
    if ( scheme != QLatin1String( "http" )
         && scheme != QLatin1String( "https" ) )
    {
        return fail( tr( "Not an http:// or https:// URL: %1" ).arg( t ) );
    }
    if ( url.host().isEmpty() )
    {
        return fail( tr( "Not a URL: %1" ).arg( t ) );
    }
    if ( ! url.userName().isEmpty()
         || ! url.password().isEmpty() )
    {
        return fail( tr( "A URL with a user name or a password cannot be opened: %1" ).arg( t ) );
    }

    if ( error ) error->clear();
    const QUrl raw = githubRawUrl( url );
    return raw.isValid() ? raw : url;
}

/*-------------------------------------------------------------------*/
QUrl
githubRawUrl( const QUrl & url )
{
    const QString host = url.host().toLower();
    if ( host != QLatin1String( "github.com" )
         && host != QLatin1String( "www.github.com" ) )
    {
        return QUrl();
    }

    static const QRegularExpression re( QStringLiteral( "^/([^/]+)/([^/]+)/(?:blob|raw)/(.+)$" ) );
    const QRegularExpressionMatch m = re.match( url.path( QUrl::FullyEncoded ) );
    if ( ! m.hasMatch() )
    {
        return QUrl();
    }
    return QUrl::fromEncoded( QStringLiteral( "https://raw.githubusercontent.com/%1/%2/%3" )
                              .arg( m.captured( 1 ), m.captured( 2 ), m.captured( 3 ) ).toUtf8(),
                              QUrl::StrictMode );
}

/*-------------------------------------------------------------------*/
QUrl
githubLfsUrl( const QUrl & url )
{
    if ( url.host().toLower() != QLatin1String( "raw.githubusercontent.com" ) )
    {
        return QUrl();
    }
    return QUrl::fromEncoded( "https://media.githubusercontent.com/media"
                              + url.path( QUrl::FullyEncoded ).toUtf8(),
                              QUrl::StrictMode );
}

/*-------------------------------------------------------------------*/
qint64
lfsPointerSize( const QByteArray & bytes )
{
    const QByteArray version( LFS_VERSION );
    if ( bytes.size() >= 1024
         || ! bytes.startsWith( version ) )
    {
        return -1;
    }

    static const QRegularExpression re( QStringLiteral( "^size (\\d+)$" ),
                                        QRegularExpression::MultilineOption );
    const QRegularExpressionMatch m = re.match( QString::fromUtf8( bytes ) );
    if ( ! m.hasMatch() )
    {
        return -1;
    }
    bool ok = false;
    const qint64 size = m.captured( 1 ).toLongLong( &ok );
    return ok ? size : -1;
}

/*-------------------------------------------------------------------*/
QString
logNameFromUrl( const QUrl & url )
{
    QString last;
    for ( const QString & part : url.path( QUrl::FullyEncoded ).split( QLatin1Char( '/' ) ) )
    {
        if ( ! part.isEmpty() ) last = part;
    }

    if ( last.isEmpty() )
    {
        return url.port() >= 0
            ? url.host() + QLatin1Char( ':' ) + QString::number( url.port() )
            : url.host();
    }

    // decoded, if the bytes are UTF-8
    const QByteArray bytes = QByteArray::fromPercentEncoding( last.toUtf8() );
    const QString decoded = QString::fromUtf8( bytes );
    return decoded.toUtf8() == bytes ? decoded : last;
}

/*-------------------------------------------------------------------*/
QString
cachedLogPath( const QString & cache_dir,
               const QUrl & url )
{
    const QByteArray hash = QCryptographicHash::hash( url.toString( QUrl::FullyEncoded ).toUtf8(),
                                                      QCryptographicHash::Sha1 ).toHex();

    // the log's name, safe for a file name on any system
    QString name = logNameFromUrl( url );
    for ( QChar & c : name )
    {
        if ( c < QLatin1Char( ' ' )
             || c == QLatin1Char( '/' ) || c == QLatin1Char( '\\' )
             || c == QLatin1Char( ':' ) || c == QLatin1Char( '*' )
             || c == QLatin1Char( '?' ) || c == QLatin1Char( '"' )
             || c == QLatin1Char( '<' ) || c == QLatin1Char( '>' )
             || c == QLatin1Char( '|' ) )
        {
            c = QLatin1Char( '_' );
        }
    }
    name = name.trimmed();
    if ( name.size() > 200 ) name = name.right( 200 );
    if ( name.isEmpty()
         || name == QLatin1String( "." )
         || name == QLatin1String( ".." ) )
    {
        name = QStringLiteral( "game.rcg" );
    }

    return QDir( cache_dir ).filePath( QString::fromLatin1( hash.left( 16 ) ) + QLatin1Char( '/' ) + name );
}

/*-------------------------------------------------------------------*/
QString
defaultLogCacheDir()
{
    return QDir( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) ).filePath( QStringLiteral( "logs" ) );
}

/*-------------------------------------------------------------------*/
qint64
logCacheSize( const QString & cache_dir )
{
    qint64 size = 0;
    QDirIterator it( cache_dir, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories );
    while ( it.hasNext() )
    {
        it.next();
        size += it.fileInfo().size();
    }
    return size;
}

/*-------------------------------------------------------------------*/
bool
clearLogCache( const QString & cache_dir )
{
    QDir dir( cache_dir );
    return ! dir.exists() || dir.removeRecursively();
}
