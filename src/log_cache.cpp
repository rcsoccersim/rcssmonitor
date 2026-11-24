// -*-c++-*-

/*!
  \file log_cache.cpp
  \brief the cache of the game logs downloaded from URLs. Source File.
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

#include "log_cache.h"

#include "log_url.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

/*-------------------------------------------------------------------*/
LogCache::LogCache( const QString & dir )
    : M_dir( dir )
{

}

/*-------------------------------------------------------------------*/
QString
LogCache::defaultDir()
{
    return QDir( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) ).filePath( QStringLiteral( "logs" ) );
}

/*-------------------------------------------------------------------*/
QString
LogCache::pathOf( const QUrl & url ) const
{
    const QByteArray hash = QCryptographicHash::hash( url.toString( QUrl::FullyEncoded ).toUtf8(),
                                                      QCryptographicHash::Sha1 ).toHex();

    // the log's name, safe for a file name on any system
    QString name = LogUrl::name( url );
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

    return QDir( M_dir ).filePath( QString::fromLatin1( hash.left( 16 ) ) + QLatin1Char( '/' ) + name );
}

/*-------------------------------------------------------------------*/
bool
LogCache::contains( const QUrl & url ) const
{
    return QFileInfo( pathOf( url ) ).isFile();
}

/*-------------------------------------------------------------------*/
qint64
LogCache::size() const
{
    qint64 bytes = 0;
    QDirIterator it( M_dir, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories );
    while ( it.hasNext() )
    {
        it.next();
        bytes += it.fileInfo().size();
    }
    return bytes;
}

/*-------------------------------------------------------------------*/
bool
LogCache::clear() const
{
    QDir dir( M_dir );
    return ! dir.exists() || dir.removeRecursively();
}
