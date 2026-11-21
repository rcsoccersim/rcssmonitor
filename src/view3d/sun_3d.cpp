// -*-c++-*-

/*!
  \file sun_3d.cpp
  \brief the sun of the 3D view and its shadows. Source File.
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

#include "sun_3d.h"

#include "ground_texture.h"

#include <QVector2D>

#include <algorithm>
#include <cmath>
#include <limits>

namespace Sun3D {

/*-------------------------------------------------------------------*/
QVector3D
direction()
{
    return QVector3D( -60.0f, 65.0f, -30.0f ).normalized();
}

/*-------------------------------------------------------------------*/
float
shadowLength()
{
    return float( GroundTexture::length() ) + 2.0f * SHADOW_MARGIN;
}

/*-------------------------------------------------------------------*/
float
shadowWidth()
{
    return float( GroundTexture::width() ) + 2.0f * SHADOW_MARGIN;
}

/*-------------------------------------------------------------------*/
QMatrix4x4
shadowViewProj()
{
    // looking along the rays at the middle of the ground, from far up the sun's way
    QMatrix4x4 view;
    view.lookAt( direction() * 200.0f, QVector3D( 0.0f, 0.0f, 0.0f ), QVector3D( 0.0f, 1.0f, 0.0f ) );

    // the box of the shadows as the sun sees it: its 8 corners
    const float hx = shadowLength() * 0.5f;
    const float hz = shadowWidth() * 0.5f;
    const float inf = std::numeric_limits< float >::max();
    QVector3D lo( inf, inf, inf );
    QVector3D hi( -inf, -inf, -inf );
    for ( const float x : { -hx, hx } )
    {
        for ( const float y : { 0.0f, SHADOW_HEIGHT } )
        {
            for ( const float z : { -hz, hz } )
            {
                const QVector3D p = view.map( QVector3D( x, y, z ) );
                lo = QVector3D( std::min( lo.x(), p.x() ), std::min( lo.y(), p.y() ), std::min( lo.z(), p.z() ) );
                hi = QVector3D( std::max( hi.x(), p.x() ), std::max( hi.y(), p.y() ), std::max( hi.z(), p.z() ) );
            }
        }
    }

    // the camera looks down its -z: the near and far planes at -hi.z and
    // -lo.z, a meter more each way
    QMatrix4x4 proj;
    proj.ortho( lo.x(), hi.x(), lo.y(), hi.y(), -hi.z() - 1.0f, -lo.z() + 1.0f );
    return proj * view;
}

/*-------------------------------------------------------------------*/
QMatrix4x4
blobShadow( const float x,
            const float z,
            const float height,
            const float width )
{
    // away from the sun on the ground, and how far the top's shadow is
    const QVector3D sun = direction();
    const QVector2D away = QVector2D( -sun.x(), -sun.z() ).normalized();
    const float reach = height * QVector2D( sun.x(), sun.z() ).length() / sun.y();

    // the ellipse from half the width behind the feet to the top's shadow
    // and half the width beyond: its middle halfway to the top's shadow
    QMatrix4x4 m;
    m.translate( x + away.x() * reach * 0.5f, SHADOW_PLANE_Y, z + away.y() * reach * 0.5f );
    // the disc's +x along away (turning about +y takes +x toward -z)
    m.rotate( float( -std::atan2( away.y(), away.x() ) * 180.0 / M_PI ), 0.0f, 1.0f, 0.0f );
    m.scale( ( reach + width ) * 0.5f, 1.0f, width * 0.5f );
    return m;
}

}
