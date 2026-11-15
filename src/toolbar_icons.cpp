// -*-c++-*-

/*!
  \file toolbar_icons.cpp
  \brief the toolbar icons drawn in code, as the web version's. Source File.
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

#include "toolbar_icons.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPointF>

#include <cmath>
#include <initializer_list>

namespace {

//! a line through points
QPainterPath
polyline( std::initializer_list< QPointF > points )
{
    QPainterPath path;
    bool first = true;
    for ( const QPointF & p : points )
    {
        if ( first ) path.moveTo( p );
        else path.lineTo( p );
        first = false;
    }
    return path;
}

/*!
  an arc of a circle of radius r from a to b, the short way, in the
  direction of the increasing angle on the screen (clockwise, y down), as
  SVG's "a r r 0 0 1"
*/
QPainterPath
arc( const QPointF & a,
     const QPointF & b,
     const double r )
{
    const QPointF m = ( a + b ) * 0.5;
    const QPointF d = b - a;
    const double half = std::hypot( d.x(), d.y() ) * 0.5;
    const double h = std::sqrt( std::max( 0.0, r * r - half * half ) );
    const QPointF normal( -d.y() / ( 2.0 * half ), d.x() / ( 2.0 * half ) );
    // of the two centers, the one from which a to b turns less than half
    // a round with the angle increasing
    for ( const double side : { 1.0, -1.0 } )
    {
        const QPointF c = m + normal * ( side * h );
        const double a0 = std::atan2( a.y() - c.y(), a.x() - c.x() );
        double turn = std::atan2( b.y() - c.y(), b.x() - c.x() ) - a0;
        while ( turn < 0.0 ) turn += 2.0 * M_PI;
        if ( turn <= M_PI )
        {
            QPainterPath path( a );
            const int n = 16;
            for ( int k = 1; k <= n; ++k )
            {
                const double t = a0 + turn * k / n;
                path.lineTo( c + QPointF( r * std::cos( t ), r * std::sin( t ) ) );
            }
            return path;
        }
    }
    return polyline( { a, b } );
}

//! draw an icon on the 16 x 16 square of the web version's shapes
void
paint( QPainter & painter,
       const DrawnIcon icon,
       const QColor & color )
{
    // a stroke of a width, its corners round if asked (SVG's defaults: butt
    // ends, miter corners)
    const auto stroke = [&]( const QPainterPath & path, const double width, const bool round = false ) {
        QPen pen( color, width, Qt::SolidLine, Qt::FlatCap, round ? Qt::RoundJoin : Qt::MiterJoin );
        painter.strokePath( path, pen );
    };
    const auto fill = [&]( const QPainterPath & path ) {
        painter.fillPath( path, color );
    };
    const auto rounded = []( const QRectF & rect, const double r ) {
        QPainterPath path;
        path.addRoundedRect( rect, r, r );
        return path;
    };

    switch ( icon ) {
    case DrawnIcon::Open:
        stroke( polyline( { { 1.5, 4.5 }, { 1.5, 12.5 }, { 13.5, 12.5 }, { 14.5, 6.5 }, { 5.5, 6.5 }, { 4.5, 8.5 } } ), 1.3, true );
        stroke( polyline( { { 1.5, 4.5 }, { 1.5, 3.5 }, { 5.5, 3.5 }, { 6.5, 4.5 }, { 12.5, 4.5 }, { 12.5, 6.5 } } ), 1.3, true );
        break;
    case DrawnIcon::Connect:
        for ( const QPainterPath & p : { polyline( { { 0.5, 8.0 }, { 2.5, 8.0 } } ),
                                         polyline( { { 7.5, 6.5 }, { 10.5, 6.5 } } ),
                                         polyline( { { 7.5, 9.5 }, { 10.5, 9.5 } } ) } )
        {
            stroke( p, 1.3 );
        }
        fill( rounded( QRectF( 2.5, 4.5, 5.0, 7.0 ), 1.0 ) );
        stroke( polyline( { { 10.5, 3.5 }, { 14.5, 3.5 }, { 14.5, 12.5 }, { 10.5, 12.5 } } ), 1.3 );
        break;
    case DrawnIcon::Disconnect:
        for ( const QPainterPath & p : { polyline( { { 0.5, 8.0 }, { 1.5, 8.0 } } ),
                                         polyline( { { 6.5, 6.5 }, { 8.5, 6.5 } } ),
                                         polyline( { { 6.5, 9.5 }, { 8.5, 9.5 } } ) } )
        {
            stroke( p, 1.3 );
        }
        fill( rounded( QRectF( 1.5, 4.5, 5.0, 7.0 ), 1.0 ) );
        stroke( polyline( { { 11.0, 3.5 }, { 14.5, 3.5 }, { 14.5, 12.5 }, { 11.0, 12.5 } } ), 1.3 );
        break;
    case DrawnIcon::KickOff:
    {
        QPainterPath circle;
        circle.addEllipse( QPointF( 8.0, 8.0 ), 6.0, 6.0 );
        stroke( circle, 1.3 );
        QPainterPath pentagon = polyline( { { 8.0, 5.2 }, { 10.6, 7.1 }, { 9.6, 10.2 }, { 6.4, 10.2 }, { 5.4, 7.1 } } );
        pentagon.closeSubpath();
        fill( pentagon );
        for ( const QPainterPath & p : { polyline( { { 8.0, 5.2 }, { 8.0, 2.0 } } ),
                                         polyline( { { 10.6, 7.1 }, { 13.6, 6.1 } } ),
                                         polyline( { { 9.6, 10.2 }, { 11.5, 12.8 } } ),
                                         polyline( { { 6.4, 10.2 }, { 4.5, 12.8 } } ),
                                         polyline( { { 5.4, 7.1 }, { 2.4, 6.1 } } ) } )
        {
            stroke( p, 1.0 );
        }
        break;
    }
    case DrawnIcon::Loop:
        stroke( arc( { 3.0, 7.5 }, { 12.0, 5.0 }, 5.0 ), 1.4 );
        stroke( arc( { 13.0, 8.5 }, { 4.0, 11.0 }, 5.0 ), 1.4 );
        stroke( polyline( { { 12.8, 2.2 }, { 12.8, 5.5 }, { 9.5, 5.5 } } ), 1.4 );
        stroke( polyline( { { 3.2, 13.8 }, { 3.2, 10.5 }, { 6.5, 10.5 } } ), 1.4 );
        break;
    }
}

}

/*-------------------------------------------------------------------*/
QImage
drawnIconImage( const DrawnIcon icon,
                const QColor & color,
                const int size )
{
    QImage image( size, size, QImage::Format_ARGB32_Premultiplied );
    image.fill( Qt::transparent );
    QPainter painter( &image );
    painter.setRenderHint( QPainter::Antialiasing );
    painter.scale( size / 16.0, size / 16.0 );
    paint( painter, icon, color );
    return image;
}

/*-------------------------------------------------------------------*/
QIcon
drawnIcon( const DrawnIcon icon,
           const QPalette & palette )
{
    QIcon result;
    for ( const int size : { 16, 24, 32, 48 } )
    {
        result.addPixmap( QPixmap::fromImage( drawnIconImage( icon, palette.color( QPalette::ButtonText ), size ) ) );
    }
    return result;
}
