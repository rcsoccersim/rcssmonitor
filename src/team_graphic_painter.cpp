// -*-c++-*-

/*!
  \file team_graphic_painter.cpp
  \brief team logo image painter class Header File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
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

#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include "team_graphic_painter.h"

#include "disp_holder.h"
#include "options.h"

#include <iostream>
#include <cstring>


/*-------------------------------------------------------------------*/
/*!

*/
TeamGraphicPainter::TeamGraphicPainter( const DispHolder & disp_holder )
    : M_disp_holder( disp_holder )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
void
TeamGraphicPainter::draw( QPainter & painter )
{
    const Options & opt = Options::instance();

    if ( ! opt.showTeamGraphic() )
    {
        return;
    }

    DispConstPtr disp = M_disp_holder.currentDisp();
    if ( ! disp )
    {
        return;
    }

    // update left team pixmap
    if ( M_team_graphic_left_set.size() != M_disp_holder.teamGraphicLeft().tiles().size()
         || disp->team_[0].name_ != M_team_name_left )
    {
        M_team_name_left = disp->team_[0].name_;
        M_team_graphic_left_set.clear();
        M_team_graphic_pixmap_left = QPixmap();

        if ( ! M_disp_holder.teamGraphicLeft().tiles().empty() )
        {
            copyTeamGraphic( M_team_graphic_pixmap_left,
                             M_team_graphic_left_set,
                             M_disp_holder.teamGraphicLeft() );
        }
    }

    // update right team pixmap
    if ( M_team_graphic_right_set.size() != M_disp_holder.teamGraphicRight().tiles().size()
         || disp->team_[1].name_ != M_team_name_right )
    {
        M_team_name_right = disp->team_[1].name_;
        M_team_graphic_right_set.clear();
        M_team_graphic_pixmap_right = QPixmap();

        if ( ! M_disp_holder.teamGraphicRight().tiles().empty() )
        {
            copyTeamGraphic( M_team_graphic_pixmap_right,
                             M_team_graphic_right_set,
                             M_disp_holder.teamGraphicRight() );
        }
    }

    const double scale = std::max( 0.001, Options::instance().teamGraphicScale() );

    if ( ! M_team_graphic_pixmap_left.isNull() )
    {
        QRectF source_rect = M_team_graphic_pixmap_left.rect();
        QRectF dest_rect( 0.0, 0.0, source_rect.width() * scale, source_rect.height() * scale );
        painter.drawPixmap( dest_rect, M_team_graphic_pixmap_left, source_rect );
        // int x = 0;
        // int y = 0;
        // //int x = ( 256 - M_team_graphic_pixmap_left.width() ) / 2;
        // //int y = ( 64 -  M_team_graphic_pixmap_left.height() ) / 2;
        // painter.drawPixmap( x, y, M_team_graphic_pixmap_left );
    }

    if ( ! M_team_graphic_pixmap_right.isNull() )
    {
        double left_x = opt.canvasWidth() - M_team_graphic_pixmap_right.width() * scale;
        QRectF source_rect = M_team_graphic_pixmap_right.rect();
        QRectF dest_rect( left_x, 0.0, source_rect.width() * scale, source_rect.height() * scale );
        painter.drawPixmap( dest_rect, M_team_graphic_pixmap_right, source_rect );

        // int x = painter.window().width() - M_team_graphic_pixmap_right.width() - 1;
        // int y = 0;
        // //int x = painter.window().width() - 256
        // //    + ( 256 - M_team_graphic_pixmap_right.width() ) / 2;
        // //int y = ( 64 -  M_team_graphic_pixmap_right.height() ) / 2;
        // painter.drawPixmap( x, y, M_team_graphic_pixmap_right );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
TeamGraphicPainter::copyTeamGraphic( QPixmap & dst_pixmap,
                                     std::set< TeamGraphic::Index > & index_set,
                                     const TeamGraphic & team_graphic )
{
    if ( dst_pixmap.width() != (int)team_graphic.width()
         || dst_pixmap.height() != (int)team_graphic.height() )
    {
        dst_pixmap = QPixmap( team_graphic.width(),
                              team_graphic.height() );
        dst_pixmap.fill( Qt::transparent );
    }

    for ( TeamGraphic::Map::const_reverse_iterator tile = team_graphic.tiles().rbegin(), end = team_graphic.tiles().rend();
          tile != end;
          ++tile )
    {
        index_set.insert( tile->first );

        copyTeamGraphicXpmTile( dst_pixmap,
                                tile->first,
                                *(tile->second) );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
TeamGraphicPainter::copyTeamGraphicXpmTile( QPixmap & dst_pixmap,
                                            const TeamGraphic::Index & index,
                                            const TeamGraphic::XpmTile & tile )
{
    const size_t array_size = 1 + tile.colors().size() + tile.height();

    char ** xpm;
    xpm = new char*[ array_size ];

    // header
    xpm[0] = new char[64];
    snprintf( xpm[0], 64, "%d %d %zd %d",
              tile.width(), tile.height(),
              tile.colors().size(), tile.cpp() );

    // colors
    std::size_t idx = 1;
    for ( const std::shared_ptr< std::string > & col : tile.colors() )
    {
        xpm[idx] = new char[ col->length() + 1 ];
        std::strcpy( xpm[idx], col->c_str() );
        ++idx;
    }

    // pixels
    for ( const std::string & line : tile.pixelLines() )
    {
        xpm[idx] = new char[ tile.width() + 1 ];
        std::strcpy( xpm[idx], line.c_str() );
        ++idx;
    }

    copyTeamGraphicXpmTile( dst_pixmap, index.first, index.second, xpm );

    // release memories
    for ( size_t i = 0; i < array_size; ++i )
    {
        delete [] xpm[i];
    }
    delete [] xpm;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
TeamGraphicPainter::copyTeamGraphicXpmTile( QPixmap & dst_pixmap,
                                            const int x,
                                            const int y,
                                            const char * const * xpm )
{
    if ( dst_pixmap.width() < (x+1) * static_cast< int >( TeamGraphic::TILE_SIZE )
         || dst_pixmap.height() < (y+1) * static_cast< int >( TeamGraphic::TILE_SIZE ) )
    {
        return;
    }

    QPixmap pixmap( xpm );

    if ( pixmap.isNull()
         || pixmap.width() != static_cast< int >( TeamGraphic::TILE_SIZE )
         || pixmap.height() != static_cast< int >( TeamGraphic::TILE_SIZE ) )
    {
        std::cerr << "(TeamGraphicPainter::copyTeamGraphicXpmTile ) Failed to create a pixmap from the xpm tile (" << x << ',' << y << ')'
                  << std::endl;
        return;
    }

    QPainter painter( &dst_pixmap );
    painter.drawPixmap( x * 8, y * 8, pixmap );
    painter.end();
}
