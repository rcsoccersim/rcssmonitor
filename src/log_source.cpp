// -*-c++-*-

/*!
  \file log_source.cpp
  \brief a source of game logs to list (GitHub, the archive). Source File.
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

#include "log_source.h"

/*-------------------------------------------------------------------*/
LogSource::LogSource( QObject * parent )
    : QObject( parent )
{

}

/*-------------------------------------------------------------------*/
LogSource::~LogSource()
{

}

/*-------------------------------------------------------------------*/
bool
LogSource::isGameLog( const QString & file_name )
{
    return file_name.endsWith( QLatin1String( ".rcg" ), Qt::CaseInsensitive )
        || file_name.endsWith( QLatin1String( ".rcg.gz" ), Qt::CaseInsensitive );
}
