// -*-c++-*-

/*!
  \file export_range.cpp
  \brief the positions and the ranges of the frames to export. Source File.
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

#include "export_range.h"

#include <QCoreApplication>
#include <QRegularExpression>

#include <algorithm>
#include <limits>

namespace {

//! the order of the frames: the cycle, then the stopped frame
struct Key {
    rcss::rcg::UInt32 time;
    rcss::rcg::UInt32 stime;
};

bool
before( const DispConstPtr & disp,
        const Key & key )
{
    return disp->show_.time_ < key.time
        || ( disp->show_.time_ == key.time && disp->show_.stime_ < key.stime );
}

bool
after( const Key & key,
       const DispConstPtr & disp )
{
    return key.time < disp->show_.time_
        || ( key.time == disp->show_.time_ && key.stime < disp->show_.stime_ );
}

bool
at( const DispConstPtr & disp,
    const ExportPosition & pos )
{
    return disp->show_.time_ == rcss::rcg::UInt32( pos.cycle )
        && ( pos.stopped < 0 || disp->show_.stime_ == rcss::rcg::UInt32( pos.stopped ) );
}

}

/*-------------------------------------------------------------------*/
bool
ExportRange::parsePosition( const QString & text,
                            ExportPosition * pos,
                            QString * error )
{
    static const QRegularExpression re( QStringLiteral( "^([0-9]{1,9})(?:\\.([0-9]{1,9}))?$" ) );
    const QRegularExpressionMatch m = re.match( text.trimmed() );
    if ( ! m.hasMatch() )
    {
        if ( error ) *error = QCoreApplication::translate( "ExportRange", "Not a position: \"%1\" (a cycle, e.g. 1234, or a cycle and a stopped frame, e.g. 1234.3)" ).arg( text.trimmed() );
        return false;
    }
    if ( pos )
    {
        pos->cycle = m.captured( 1 ).toInt();
        pos->stopped = ( m.capturedLength( 2 ) > 0 ? m.captured( 2 ).toInt() : -1 );
    }
    if ( error ) error->clear();
    return true;
}

/*-------------------------------------------------------------------*/
QString
ExportRange::positionText( const rcss::rcg::ShowInfoT & show )
{
    return show.stime_ > 0
        ? QString( "%1.%2" ).arg( show.time_ ).arg( show.stime_ )
        : QString::number( show.time_ );
}

/*-------------------------------------------------------------------*/
QString
ExportRange::positionText( const DispCont & frames,
                           const std::size_t index )
{
    return index < frames.size()
        ? positionText( frames[index]->show_ )
        : QString();
}

/*-------------------------------------------------------------------*/
std::size_t
ExportRange::startIndex( const DispCont & frames,
                         const ExportPosition & pos,
                         bool * snapped )
{
    if ( snapped ) *snapped = false;
    if ( ! pos.isValid() )
    {
        return DispHolder::INVALID_INDEX;
    }

    // the first frame at or after the position
    const Key key{ rcss::rcg::UInt32( pos.cycle ), rcss::rcg::UInt32( std::max( 0, pos.stopped ) ) };
    const DispCont::const_iterator it = std::lower_bound( frames.begin(), frames.end(), key, before );
    if ( it == frames.end() )
    {
        return DispHolder::INVALID_INDEX;
    }
    if ( snapped ) *snapped = ! at( *it, pos );
    return std::distance( frames.begin(), it );
}

/*-------------------------------------------------------------------*/
std::size_t
ExportRange::endIndex( const DispCont & frames,
                       const ExportPosition & pos,
                       bool * snapped )
{
    if ( snapped ) *snapped = false;
    if ( ! pos.isValid() )
    {
        return DispHolder::INVALID_INDEX;
    }

    // the last frame at or before the position: all the frames stopped in
    // the cycle for a cycle
    const Key key{ rcss::rcg::UInt32( pos.cycle ),
                   pos.stopped < 0
                   ? std::numeric_limits< rcss::rcg::UInt32 >::max()
                   : rcss::rcg::UInt32( pos.stopped ) };
    DispCont::const_iterator it = std::upper_bound( frames.begin(), frames.end(), key, after );
    if ( it == frames.begin() )
    {
        return DispHolder::INVALID_INDEX;
    }
    --it;
    if ( snapped ) *snapped = ! at( *it, pos );
    return std::distance( frames.begin(), it );
}

/*-------------------------------------------------------------------*/
QString
ExportRange::gameTime( const int cycle,
                       const int step_ms )
{
    const long long seconds = static_cast< long long >( std::max( 0, cycle ) ) * std::max( 1, step_ms ) / 1000;
    return QString( "%1:%2" ).arg( seconds / 60 ).arg( seconds % 60, 2, 10, QLatin1Char( '0' ) );
}
