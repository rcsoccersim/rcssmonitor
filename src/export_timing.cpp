// -*-c++-*-

/*!
  \file export_timing.cpp
  \brief the timing of the exported images and videos. Source File.
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

#include "export_timing.h"

#include <algorithm>
#include <cmath>

/*-------------------------------------------------------------------*/
double
ExportTiming::framesPerImage( const double speed,
                              const int step_ms,
                              const double fps )
{
    if ( speed <= 0.0
         || step_ms <= 0
         || fps <= 0.0 )
    {
        return 0.0;
    }
    return speed * 1000.0 / ( step_ms * fps );
}

/*-------------------------------------------------------------------*/
std::size_t
ExportTiming::imageCount( const std::size_t first,
                          const std::size_t last,
                          const double per_image )
{
    if ( last < first
         || per_image <= 0.0 )
    {
        return 0;
    }
    return static_cast< std::size_t >( std::floor( ( last - first ) / per_image + EPSILON ) ) + 1;
}

/*-------------------------------------------------------------------*/
double
ExportTiming::position( const std::size_t first,
                        const std::size_t n,
                        const double per_image )
{
    return first + n * per_image;
}

/*-------------------------------------------------------------------*/
std::size_t
ExportTiming::frameAt( const double position,
                       const std::size_t last )
{
    if ( position <= 0.0 )
    {
        return 0;
    }
    return std::min( last, static_cast< std::size_t >( std::floor( position + EPSILON ) ) );
}

/*-------------------------------------------------------------------*/
std::vector< std::size_t >
ExportTiming::frames2D( const std::size_t first,
                        const std::size_t last,
                        const double per_image )
{
    std::vector< std::size_t > frames;
    const std::size_t n = imageCount( first, last, per_image );
    frames.reserve( n );
    for ( std::size_t i = 0; i < n; ++i )
    {
        frames.push_back( frameAt( position( first, i, per_image ), last ) );
    }
    return frames;
}

/*-------------------------------------------------------------------*/
double
ExportTiming::duration( const std::size_t images,
                        const double fps )
{
    return fps > 0.0 ? images / fps : 0.0;
}

/*-------------------------------------------------------------------*/
int
ExportTiming::nameDigits( const std::size_t images )
{
    const std::size_t max_number = ( images > 0 ? images - 1 : 0 );
    return std::max( 5, static_cast< int >( QString::number( qulonglong( max_number ) ).size() ) );
}

/*-------------------------------------------------------------------*/
QString
ExportTiming::imageName( const QString & prefix,
                         const std::size_t number,
                         const int digits,
                         const QString & ext )
{
    return prefix
        + QString( "%1" ).arg( qulonglong( number ), digits, 10, QLatin1Char( '0' ) )
        + QLatin1Char( '.' ) + ext;
}
