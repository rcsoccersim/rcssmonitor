// -*-c++-*-

/*!
  \file ground_texture.cpp
  \brief the image of the ground of the 3D view. Source File.
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

#include "ground_texture.h"

#include "field_painter.h"
#include "options.h"

#include <rcss/rcg/types.h>

#include <QPainter>
#include <QStringList>

#include <cmath>

namespace GroundTexture {

/*-------------------------------------------------------------------*/
double
length()
{
    return Options::PITCH_LENGTH + 2.0 * Options::PITCH_MARGIN;
}

/*-------------------------------------------------------------------*/
double
width()
{
    return Options::PITCH_WIDTH + 2.0 * Options::PITCH_MARGIN;
}

/*-------------------------------------------------------------------*/
QImage
draw( FieldPainter & painter )
{
    const int w = static_cast< int >( std::lround( length() * SCALE ) );
    const int h = static_cast< int >( std::lround( width() * SCALE ) );
    QImage image( w, h, QImage::Format_RGBA8888 );
    image.fill( Options::instance().fieldBrush().color() );

    // the field painter maps the field with the options: map it to the
    // image for a while
    Options & opt = Options::instance();
    const Options::FieldTransform saved = opt.fieldTransform();
    opt.setFieldTransform( Options::FieldTransform{ SCALE, QPoint( w / 2, h / 2 ) } );
    {
        QPainter p( &image );
        p.setRenderHint( QPainter::Antialiasing );
        painter.drawGround( p, PEN_SCALE );
    }
    opt.setFieldTransform( saved );

    return image;
}

/*-------------------------------------------------------------------*/
QString
key( const rcss::rcg::ServerParamT & param )
{
    const Options & opt = Options::instance();
    return QStringList( {
            QString::number( param.keepaway_mode_ ),
            QString::number( param.keepaway_length_ ),
            QString::number( param.keepaway_width_ ),
            opt.fieldBrush().color().name( QColor::HexArgb ),
            opt.linePen().color().name( QColor::HexArgb ),
            QString::number( opt.showFlag() ),
            QString::number( opt.showKeepawayArea() ),
            QString::number( opt.gridStep() ),
            QString::number( opt.showGridCoord() ),
        } ).join( ',' );
}

}
