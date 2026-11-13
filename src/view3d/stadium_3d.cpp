// -*-c++-*-

/*!
  \file stadium_3d.cpp
  \brief the surroundings of the pitch in the team colors. Source File.
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

#include "stadium_3d.h"

#include "options.h"

#include <QFont>
#include <QFontMetricsF>
#include <QHash>
#include <QMatrix4x4>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace Stadium3D {

namespace {

QVector4D
rgba( const QColor & c )
{
    return QVector4D( float( c.redF() ), float( c.greenF() ), float( c.blueF() ), 1.0f );
}

//! the largest bold font up to max pixels with which the text fits in the width
QFont
fit_font( const QString & text,
          const double max,
          const double width )
{
    QFont font;
    font.setBold( true );
    font.setPixelSize( std::max( 1, int( max ) ) );
    const double w = QFontMetricsF( font ).horizontalAdvance( text );
    if ( w > width && w > 0.0 )
    {
        font.setPixelSize( std::max( 8, int( std::floor( max * width / w ) ) ) );
    }
    return font;
}

/*!
  a panel of the unit quad (facing +z): width x height, centered at, its
  front turned by turn degrees about y from +z
*/
void
add_panel( std::vector< Instance3D > & list,
           const float width,
           const float height,
           const QVector3D & at,
           const float turn,
           const QVector4D & back )
{
    QMatrix4x4 m;
    m.translate( at );
    m.rotate( turn, 0.0f, 1.0f, 0.0f );
    m.scale( width, height, 1.0f );
    list.push_back( Instance3D{ m, back } );
}

}

/*-------------------------------------------------------------------*/
QString
TeamLook::key() const
{
    const uint logo_hash = ( logo.isNull()
                             ? 0u
                             : uint( qHashBits( logo.constBits(), size_t( logo.sizeInBytes() ) ) ) );
    return color.name() + '|' + name + '|' + QString::number( logo.width() ) + 'x'
        + QString::number( logo.height() ) + '|' + QString::number( logo_hash );
}

/*-------------------------------------------------------------------*/
QColor
shade( const QColor & color,
       const double k )
{
    const auto f = [k]( const int v ) { return std::min( 255, std::max( 0, int( std::lround( v * k ) ) ) ); };
    return QColor( f( color.red() ), f( color.green() ), f( color.blue() ) );
}

/*-------------------------------------------------------------------*/
QImage
boardImage( const TeamLook & look )
{
    QImage image( int( BOARD_TILE * BOARD_PIXELS_PER_METER ), int( BOARD_HEIGHT * BOARD_PIXELS_PER_METER ),
                  QImage::Format_RGB32 );
    image.fill( shade( look.color, BOARD_SHADE ) );
    QPainter painter( &image );
    painter.fillRect( 0, 0, image.width(), 5, look.color );
    painter.fillRect( 0, image.height() - 3, image.width(), 3, look.color );
    if ( ! look.name.isEmpty() )
    {
        painter.setRenderHint( QPainter::TextAntialiasing );
        painter.setFont( fit_font( look.name, 36.0, image.width() * 0.9 ) );
        painter.setPen( look.color );
        painter.drawText( QRect( 0, 2, image.width(), image.height() ), Qt::AlignCenter, look.name );
    }
    return image;
}

/*-------------------------------------------------------------------*/
QImage
bannerImage( const TeamLook & look )
{
    QImage image( int( BANNER_WIDTH * BANNER_PIXELS_PER_METER ), int( BANNER_HEIGHT * BANNER_PIXELS_PER_METER ),
                  QImage::Format_RGB32 );
    image.fill( shade( look.color, BOARD_SHADE ) );
    QPainter painter( &image );
    painter.setPen( QPen( look.color, 8 ) );
    painter.setBrush( Qt::NoBrush );
    painter.drawRect( QRectF( 4, 4, image.width() - 8, image.height() - 8 ) );

    // whole pixels for the logo (at most 256 x 64), as the 2D view draws it
    const bool has_logo = ! look.logo.isNull();
    const int k = ( has_logo
                    ? std::max( 1, int( std::floor( std::min( image.height() * 0.7 / look.logo.height(),
                                                              440.0 / look.logo.width() ) ) ) )
                    : 0 );
    const int logo_width = ( has_logo ? look.logo.width() * k : 0 );
    const int gap = ( has_logo && ! look.name.isEmpty() ? 32 : 0 );
    QFont font;
    double name_width = 0.0;
    if ( ! look.name.isEmpty() )
    {
        font = fit_font( look.name, 96.0, image.width() * 0.9 - logo_width - gap );
        name_width = QFontMetricsF( font ).horizontalAdvance( look.name );
    }
    double x = ( image.width() - logo_width - gap - name_width ) / 2.0;
    if ( has_logo )
    {
        painter.drawImage( QRectF( x, ( image.height() - look.logo.height() * k ) / 2.0,
                                   logo_width, look.logo.height() * k ),
                           look.logo );
        x += logo_width + gap;
    }
    if ( ! look.name.isEmpty() )
    {
        painter.setRenderHint( QPainter::TextAntialiasing );
        painter.setFont( font );
        painter.setPen( look.color );
        painter.drawText( QRectF( x, 4, name_width + 4, image.height() ), Qt::AlignLeft | Qt::AlignVCenter, look.name );
    }
    return image;
}

/*-------------------------------------------------------------------*/
void
addPanels( SceneContent & content,
           const bool boards,
           const bool banners,
           const QColor & left,
           const QColor & right )
{
    const float x = float( Options::PITCH_HALF_LENGTH ) + BOARD_OFFSET;
    const float z = float( Options::PITCH_HALF_WIDTH ) + BOARD_OFFSET;
    const float h = BOARD_HEIGHT;
    for ( int end = 0; end < 2; ++end )
    {
        const float side = ( end == 0 ? -1.0f : 1.0f );
        const QVector4D back = rgba( shade( end == 0 ? left : right, BOARD_SHADE ) );
        // facing the pitch from behind the goal line: +z turned toward -side x
        const float facing = -side * 90.0f;
        if ( boards )
        {
            // along the touch lines, from the halfway line to the corner, facing the pitch
            add_panel( content.boards[end], x, h, QVector3D( side * x / 2.0f, h / 2.0f, z ), 180.0f, back );
            add_panel( content.boards[end], x, h, QVector3D( side * x / 2.0f, h / 2.0f, -z ), 0.0f, back );
            // behind the goal line
            add_panel( content.boards[end], 2.0f * z, h, QVector3D( side * x, h / 2.0f, 0.0f ), facing, back );
        }
        if ( banners )
        {
            // high above, behind the goal
            add_panel( content.banners[end], BANNER_WIDTH, BANNER_HEIGHT,
                       QVector3D( side * ( float( Options::PITCH_HALF_LENGTH ) + BANNER_BEHIND ),
                                  BANNER_BOTTOM + BANNER_HEIGHT / 2.0f,
                                  0.0f ),
                       facing, back );
        }
    }
}

}
