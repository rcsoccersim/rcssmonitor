// -*-c++-*-

/*!
  \file log_file_name.cpp
  \brief the names of game log files: the date, the teams and the score.
  Source File.
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

#include "log_file_name.h"

#include <QRegularExpression>

/*-------------------------------------------------------------------*/
LogFileName::LogFileName()
    : M_valid( false ),
      M_left_score( 0 ),
      M_right_score( 0 ),
      M_left_penalties( -1 ),
      M_right_penalties( -1 )
{

}

/*-------------------------------------------------------------------*/
LogFileName
LogFileName::parse( const QString & file_name )
{
    // the team names are the shortest that leave a score after them
    static const QRegularExpression whole( QStringLiteral( "^(\\d{12}|\\d{14})-(.+)-vs-(.+)\\.rcg(?:\\.gz)?$" ) );
    static const QRegularExpression side( QStringLiteral( "^(.*?)_(\\d+)(?:_(\\d+))?$" ) );

    LogFileName name;
    const QRegularExpressionMatch m = whole.match( file_name );
    if ( ! m.hasMatch() )
    {
        return name;
    }
    const QRegularExpressionMatch left = side.match( m.captured( 2 ) );
    const QRegularExpressionMatch right = side.match( m.captured( 3 ) );
    if ( ! left.hasMatch()
         || ! right.hasMatch()
         || left.captured( 1 ).isEmpty()
         || right.captured( 1 ).isEmpty() )
    {
        return name;
    }
    const QString date = m.captured( 1 );
    name.M_date_time = QDateTime::fromString( date,
                                              date.size() == 12
                                              ? QStringLiteral( "yyyyMMddHHmm" )
                                              : QStringLiteral( "yyyyMMddHHmmss" ) );
    if ( ! name.M_date_time.isValid() )
    {
        return name;
    }

    name.M_left_team = left.captured( 1 );
    name.M_right_team = right.captured( 1 );
    name.M_left_score = left.captured( 2 ).toInt();
    name.M_right_score = right.captured( 2 ).toInt();
    // capturedStart(): hasCaptured() is not in Qt 5
    if ( left.capturedStart( 3 ) >= 0 && right.capturedStart( 3 ) >= 0 )
    {
        name.M_left_penalties = left.captured( 3 ).toInt();
        name.M_right_penalties = right.captured( 3 ).toInt();
    }
    name.M_valid = true;
    return name;
}

/*-------------------------------------------------------------------*/
QString
LogFileName::score() const
{
    if ( ! M_valid )
    {
        return QString();
    }
    if ( M_left_penalties >= 0 )
    {
        return QStringLiteral( "%1 (%2) - %3 (%4)" )
            .arg( M_left_score ).arg( M_left_penalties )
            .arg( M_right_score ).arg( M_right_penalties );
    }
    return QStringLiteral( "%1 - %2" ).arg( M_left_score ).arg( M_right_score );
}
