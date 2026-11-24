// -*-c++-*-

/*!
  \file export_range_list.cpp
  \brief the ranges to export, as the user gives them. Source File.
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

#include "export_range_list.h"

#include <QRegularExpression>

#include <algorithm>

/*-------------------------------------------------------------------*/
ExportRangeList::ExportRangeList( QObject * parent )
    : QObject( parent ),
      M_current( 0 )
{
    M_ranges.push_back( ExportRangeSpec{ QString(), QString(), newName() } );
}

/*-------------------------------------------------------------------*/
void
ExportRangeList::setCurrent( const int index )
{
    if ( index < 0 || M_ranges.size() <= index || index == M_current )
    {
        return;
    }
    M_current = index;
    emit changed();
}

/*-------------------------------------------------------------------*/
void
ExportRangeList::setStart( const QString & text )
{
    if ( M_ranges[M_current].start == text ) return;
    M_ranges[M_current].start = text;
    emit changed();
}

/*-------------------------------------------------------------------*/
void
ExportRangeList::setEnd( const QString & text )
{
    if ( M_ranges[M_current].end == text ) return;
    M_ranges[M_current].end = text;
    emit changed();
}

/*-------------------------------------------------------------------*/
void
ExportRangeList::setName( const QString & name )
{
    if ( M_ranges[M_current].name == name ) return;
    M_ranges[M_current].name = name;
    emit changed();
}

/*-------------------------------------------------------------------*/
void
ExportRangeList::setCurrentRange( const ExportRangeSpec & spec )
{
    if ( M_ranges[M_current] == spec ) return;
    M_ranges[M_current] = spec;
    emit changed();
}

/*-------------------------------------------------------------------*/
void
ExportRangeList::add( const ExportRangeSpec & spec )
{
    M_ranges.push_back( spec );
    M_current = M_ranges.size() - 1;
    emit changed();
}

/*-------------------------------------------------------------------*/
void
ExportRangeList::remove( const int index )
{
    if ( index < 0 || M_ranges.size() <= index )
    {
        return;
    }
    if ( M_ranges.size() == 1 )
    {
        clear();
        return;
    }
    M_ranges.remove( index );
    if ( M_current >= M_ranges.size()
         || M_current > index )
    {
        M_current = std::max( 0, M_current - 1 );
    }
    emit changed();
}

/*-------------------------------------------------------------------*/
void
ExportRangeList::move( const int from,
                       const int to )
{
    if ( from < 0 || M_ranges.size() <= from
         || to < 0 || M_ranges.size() <= to
         || from == to )
    {
        return;
    }
    const ExportRangeSpec spec = M_ranges[from];
    M_ranges.remove( from );
    M_ranges.insert( to, spec );
    if ( M_current == from ) M_current = to;
    else if ( from < M_current && M_current <= to ) --M_current;
    else if ( to <= M_current && M_current < from ) ++M_current;
    emit changed();
}

/*-------------------------------------------------------------------*/
void
ExportRangeList::setRanges( const QVector< ExportRangeSpec > & ranges )
{
    if ( ranges.isEmpty() )
    {
        clear();
        return;
    }
    M_ranges = ranges;
    M_current = 0;
    emit changed();
}

/*-------------------------------------------------------------------*/
void
ExportRangeList::clear()
{
    if ( isCleared() )
    {
        return;
    }
    M_ranges.clear();
    M_ranges.push_back( ExportRangeSpec{ QString(), QString(), QStringLiteral( "range1" ) } );
    M_current = 0;
    emit changed();
}

/*-------------------------------------------------------------------*/
bool
ExportRangeList::isCleared() const
{
    return M_ranges.size() == 1
        && M_ranges.front().start.isEmpty()
        && M_ranges.front().end.isEmpty()
        && M_ranges.front().name == QLatin1String( "range1" );
}

/*-------------------------------------------------------------------*/
QString
ExportRangeList::newName( const QString & base ) const
{
    for ( int n = 1; ; ++n )
    {
        const QString name = base + QString::number( n );
        if ( std::none_of( M_ranges.begin(), M_ranges.end(),
                           [&name]( const ExportRangeSpec & s ) { return s.name == name; } ) )
        {
            return name;
        }
    }
}

/*-------------------------------------------------------------------*/
QString
ExportRangeList::safeName( const QString & name )
{
    static const QRegularExpression other( QStringLiteral( "[^A-Za-z0-9._-]" ) );
    QString s = name.trimmed();
    s.replace( other, QStringLiteral( "_" ) );
    if ( s.isEmpty() || s == QLatin1String( "." ) || s == QLatin1String( ".." ) )
    {
        s = QStringLiteral( "range" );
    }
    return s;
}

/*-------------------------------------------------------------------*/
ExportRange
ExportRangeList::resolve( const ExportRangeSpec & spec,
                          const DispCont & frames,
                          QString * error,
                          bool * start_snapped,
                          bool * end_snapped )
{
    if ( start_snapped ) *start_snapped = false;
    if ( end_snapped ) *end_snapped = false;
    if ( error ) error->clear();

    ExportRange r;
    if ( frames.empty() )
    {
        if ( error ) *error = tr( "There is no game log." );
        return r;
    }

    // an end: its index, or INVALID_INDEX with the reason
    const auto index = [&]( const QString & text, const bool start, bool * snapped ) -> std::size_t {
        if ( text.trimmed().isEmpty() )
        {
            return start ? 0 : frames.size() - 1;
        }
        ExportPosition pos;
        if ( ! ExportRange::parsePosition( text, &pos, error ) )
        {
            return DispHolder::INVALID_INDEX;
        }
        const std::size_t i = ( start
                                ? ExportRange::startIndex( frames, pos, snapped )
                                : ExportRange::endIndex( frames, pos, snapped ) );
        if ( i == DispHolder::INVALID_INDEX && error )
        {
            *error = ( start
                       ? tr( "The start %1 is after the last frame (%2)." ).arg( text.trimmed(), ExportRange::positionText( frames, frames.size() - 1 ) )
                       : tr( "The end %1 is before the first frame (%2)." ).arg( text.trimmed(), ExportRange::positionText( frames, 0 ) ) );
        }
        return i;
    };

    r.first = index( spec.start, true, start_snapped );
    if ( r.first == DispHolder::INVALID_INDEX )
    {
        return ExportRange();
    }
    r.last = index( spec.end, false, end_snapped );
    if ( r.last == DispHolder::INVALID_INDEX )
    {
        return ExportRange();
    }
    if ( r.first > r.last )
    {
        if ( error ) *error = tr( "The start is after the end." );
        return ExportRange();
    }
    return r;
}
