// -*-c++-*-

/*!
  \file scenery_3d.cpp
  \brief the scenery of the 3D view: the sky, the haze and the ground beyond. Source File.
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

#include "scenery_3d.h"

#include <algorithm>
#include <cmath>

namespace Scenery3D {

namespace {

QVector3D
rgb( const QColor & c )
{
    return QVector3D( float( c.redF() ), float( c.greenF() ), float( c.blueF() ) );
}

}

/*-------------------------------------------------------------------*/
QColor
skyColor( const QVector3D & direction )
{
    // as sky.frag: from the horizon to the zenith, faster near the horizon;
    // below it, quickly to the color beyond the ground
    const float h = direction.normalized().y();
    QVector3D c;
    if ( h >= 0.0f )
    {
        const float t = std::pow( h, 0.55f );
        c = rgb( SKY_HORIZON ) * ( 1.0f - t ) + rgb( SKY_ZENITH ) * t;
    }
    else
    {
        const float t = std::min( 1.0f, -h * 12.0f );
        c = rgb( SKY_HORIZON ) * ( 1.0f - t ) + rgb( SKY_BELOW ) * t;
    }
    return QColor::fromRgbF( c.x(), c.y(), c.z() );
}

/*-------------------------------------------------------------------*/
float
haze( const float distance )
{
    return std::min( 1.0f, std::max( 0.0f, ( distance - HAZE_FROM ) / ( HAZE_TO - HAZE_FROM ) ) );
}

/*-------------------------------------------------------------------*/
QVector4D
farGroundColor( const QColor & field )
{
    return QVector4D( rgb( field ) * FAR_GROUND_SHADE, 1.0f );
}

}
