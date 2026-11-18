// -*-c++-*-

/*!
  \file tool_icon.cpp
  \brief toolbar icons in the colors of the palette. Source File.
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

#include "tool_icon.h"

#include <QPixmap>

/*-------------------------------------------------------------------*/
/*!

 */
QImage
tintIcon( const QImage & image,
          const QColor & color )
{
    QImage tinted = image.convertToFormat( QImage::Format_ARGB32 );

    for ( int y = 0; y < tinted.height(); ++y )
    {
        for ( int x = 0; x < tinted.width(); ++x )
        {
            const QRgb pixel = tinted.pixel( x, y );
            // 255 for black, 0 for white
            const int darkness = 255 - qGray( pixel );
            const int alpha = qAlpha( pixel ) * darkness / 255 * color.alpha() / 255;
            tinted.setPixel( x, y, qRgba( color.red(), color.green(), color.blue(), alpha ) );
        }
    }

    return tinted;
}

/*-------------------------------------------------------------------*/
/*!

 */
QIcon
toolIcon( const char * const xpm[],
          const QPalette & palette )
{
    return QIcon( QPixmap::fromImage( tintIcon( QPixmap( xpm ).toImage(),
                                                palette.color( QPalette::ButtonText ) ) ) );
}
