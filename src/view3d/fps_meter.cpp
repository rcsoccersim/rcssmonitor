// -*-c++-*-

/*!
  \file fps_meter.cpp
  \brief the frames per second of the 3D view, shown over it. Source File.
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

#include "fps_meter.h"

#include <cmath>

/*-------------------------------------------------------------------*/
void
FpsMeter::frame( const double now )
{
    M_frames.push_back( now );
    while ( ! M_frames.empty()
            && M_frames.front() < now - WINDOW )
    {
        M_frames.pop_front();
    }
}

/*-------------------------------------------------------------------*/
void
FpsMeter::gpu( const double total,
               const int count )
{
    if ( count > 0 )
    {
        M_gpu = total / count;
    }
}

/*-------------------------------------------------------------------*/
double
FpsMeter::fps( const double now ) const
{
    // the frames in the window before now
    std::size_t first = 0;
    while ( first < M_frames.size()
            && M_frames[first] < now - WINDOW )
    {
        ++first;
    }
    const std::size_t n = M_frames.size() - first;
    if ( n < 2 ) return -1.0;
    const double span = M_frames.back() - M_frames[first];
    if ( span <= 0.0 ) return -1.0;
    return double( n - 1 ) / span;
}

/*-------------------------------------------------------------------*/
QString
FpsMeter::text( const double now,
                const bool still ) const
{
    const double f = ( still ? -1.0 : fps( now ) );
    QString s = ( f < 0.0 ? QStringLiteral( "- fps" ) : QString::number( std::lround( f ) ) + QStringLiteral( " fps" ) );
    if ( M_gpu >= 0.0 )
    {
        s += QString::fromUtf8( " · GPU " ) + QString::number( M_gpu, 'f', 1 ) + QStringLiteral( " ms" );
    }
    return s;
}

/*-------------------------------------------------------------------*/
void
FpsMeter::clear()
{
    M_frames.clear();
    M_gpu = -1.0;
}
