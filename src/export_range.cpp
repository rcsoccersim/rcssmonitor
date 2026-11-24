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
#include <cmath>
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
int
ExportRange::cyclesOf( const double seconds,
                       const int step_ms )
{
    return static_cast< int >( std::lround( seconds * 1000.0 / std::max( 1, step_ms ) ) );
}

/*-------------------------------------------------------------------*/
std::vector< ExportPeriod >
ExportRange::periods( const DispCont & frames,
                      const rcss::rcg::ServerParamT & param )
{
    std::vector< ExportPeriod > list;
    if ( frames.empty() )
    {
        return list;
    }

    // the penalty shootout: from its first play mode to the end
    std::size_t penalties = frames.size();
    for ( std::size_t i = 0; i < frames.size(); ++i )
    {
        const rcss::rcg::PlayMode pm = frames[i]->pmode_;
        if ( rcss::rcg::PM_PenaltySetup_Left <= pm && pm <= rcss::rcg::PM_PenaltyScore_Right )
        {
            penalties = i;
            break;
        }
    }

    // the ends of the halves (cycles), with their keys and labels
    struct End {
        int cycle;
        QString key;
        QString label;
    };
    std::vector< End > ends;
    const int step = std::max( 1, param.simulator_step_ );
    const int half = cyclesOf( param.half_time_, step );
    if ( half > 0 )
    {
        const int normal = std::max( 1, param.nr_normal_halfs_ );
        for ( int k = 1; k <= normal; ++k )
        {
            ends.push_back( { half * k,
                              k == 1 ? QStringLiteral( "first-half" ) : k == 2 ? QStringLiteral( "second-half" ) : QString( "half-%1" ).arg( k ),
                              k == 1 ? QCoreApplication::translate( "ExportRange", "First half" )
                              : k == 2 ? QCoreApplication::translate( "ExportRange", "Second half" )
                              : QCoreApplication::translate( "ExportRange", "Half %1" ).arg( k ) } );
        }
        const int extra = cyclesOf( param.extra_half_time_, step );
        for ( int j = 1; extra > 0 && j <= param.nr_extra_halfs_; ++j )
        {
            ends.push_back( { half * normal + extra * j,
                              QString( "extra-%1" ).arg( j ),
                              QCoreApplication::translate( "ExportRange", "Extra time %1" ).arg( j ) } );
        }
    }

    // the frames before the shootout, by the end of each part (the frames
    // after the last end in the last part)
    std::size_t first = 0;
    std::size_t part = 0;
    for ( std::size_t i = 0; i <= penalties && first < penalties; ++i )
    {
        std::size_t p = ends.size();
        if ( i < penalties )
        {
            const int cycle = static_cast< int >( frames[i]->show_.time_ );
            p = 0;
            while ( p + 1 < ends.size() && cycle > ends[p].cycle ) ++p;
        }
        if ( i == penalties || p != part )
        {
            if ( i > first )
            {
                ExportPeriod period;
                period.first = first;
                period.last = i - 1;
                period.key = ( ends.empty() ? QStringLiteral( "game" ) : ends[part].key );
                period.label = ( ends.empty() ? QCoreApplication::translate( "ExportRange", "The game" ) : ends[part].label );
                list.push_back( period );
            }
            first = i;
            part = p;
        }
    }

    if ( penalties < frames.size() )
    {
        ExportPeriod period;
        period.first = penalties;
        period.last = frames.size() - 1;
        period.key = QStringLiteral( "penalties" );
        period.label = QCoreApplication::translate( "ExportRange", "Penalty shootout" );
        list.push_back( period );
    }
    return list;
}

/*-------------------------------------------------------------------*/
std::vector< ExportGoal >
ExportRange::goals( const DispCont & frames,
                    const std::vector< std::size_t > & score_changed )
{
    std::vector< ExportGoal > list;
    // the score before each change: from the change before it (not from
    // the frame at the index, which may have the new score in some logs)
    int left = 0;
    int right = 0;
    for ( const std::size_t index : score_changed )
    {
        if ( index >= frames.size() )
        {
            continue;
        }
        const DispConstPtr & after = frames[std::min( index + 1, frames.size() - 1 )];
        const int new_left = after->team_[0].score_;
        const int new_right = after->team_[1].score_;
        for ( int side = 0; side < 2; ++side )
        {
            if ( ( side == 0 ? new_left > left : new_right > right ) )
            {
                ExportGoal goal;
                goal.index = index;
                goal.cycle = static_cast< int >( after->show_.time_ );
                goal.side = side;
                goal.left = new_left;
                goal.right = new_right;
                goal.team = QString::fromStdString( after->team_[side].name_ );
                list.push_back( goal );
            }
        }
        left = new_left;
        right = new_right;
    }
    return list;
}

/*-------------------------------------------------------------------*/
ExportRange
ExportRange::around( const DispCont & frames,
                     const int cycle,
                     const double before_s,
                     const double after_s,
                     const int step_ms )
{
    ExportRange r;
    if ( frames.empty() )
    {
        return r;
    }

    ExportPosition start;
    start.cycle = std::max( 0, cycle - cyclesOf( before_s, step_ms ) );
    ExportPosition end;
    end.cycle = cycle + cyclesOf( after_s, step_ms );

    r.first = startIndex( frames, start, nullptr );
    r.last = endIndex( frames, end, nullptr );
    if ( r.first == DispHolder::INVALID_INDEX ) r.first = frames.size() - 1;
    if ( r.last == DispHolder::INVALID_INDEX ) r.last = 0;
    if ( ! r.isValid() )
    {
        r = ExportRange();
    }
    return r;
}

/*-------------------------------------------------------------------*/
QString
ExportRange::gameTime( const int cycle,
                       const int step_ms )
{
    const long long seconds = static_cast< long long >( std::max( 0, cycle ) ) * std::max( 1, step_ms ) / 1000;
    return QString( "%1:%2" ).arg( seconds / 60 ).arg( seconds % 60, 2, 10, QLatin1Char( '0' ) );
}
