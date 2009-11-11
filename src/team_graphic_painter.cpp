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

#include <QtGui>

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

    // update left team pixmap
    if ( M_team_graphic_left_set.size() != M_disp_holder.teamGraphicLeft().tiles().size() )
    {
        if ( M_disp_holder.teamGraphicLeft().tiles().empty() )
        {
            M_team_graphic_left_set.clear();
            M_team_graphic_pixmap_left = QPixmap();
        }
        else
        {
            copyTeamGraphic( M_team_graphic_pixmap_left,
                             M_team_graphic_left_set,
                             M_disp_holder.teamGraphicLeft() );
        }
    }

    // update right team pixmap
    if ( M_team_graphic_right_set.size() != M_disp_holder.teamGraphicRight().tiles().size() )
    {
        if ( M_disp_holder.teamGraphicRight().tiles().empty() )
        {
            M_team_graphic_right_set.clear();
            M_team_graphic_pixmap_right = QPixmap();
        }
        else
        {
            copyTeamGraphic( M_team_graphic_pixmap_right,
                             M_team_graphic_right_set,
                             M_disp_holder.teamGraphicRight() );
        }
    }

    if ( ! M_team_graphic_pixmap_left.isNull() )
    {
        int x = 0;
        int y = 0;
        //int x = ( 256 - M_team_graphic_pixmap_left.width() ) / 2;
        //int y = ( 64 -  M_team_graphic_pixmap_left.height() ) / 2;
        painter.drawPixmap( x, y, M_team_graphic_pixmap_left );
    }

    if ( ! M_team_graphic_pixmap_right.isNull() )
    {
        int x = painter.window().width() - M_team_graphic_pixmap_right.width() - 1;
        int y = 0;
        //int x = painter.window().width() - 256
        //    + ( 256 - M_team_graphic_pixmap_right.width() ) / 2;
        //int y = ( 64 -  M_team_graphic_pixmap_right.height() ) / 2;
        painter.drawPixmap( x, y, M_team_graphic_pixmap_right );
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
    if ( dst_pixmap.width() != team_graphic.width()
         || dst_pixmap.height() != team_graphic.height() )
    {
        dst_pixmap = QPixmap( team_graphic.width(),
                              team_graphic.height() );
        dst_pixmap.fill( Qt::transparent );
    }

    const TeamGraphic::Map::const_reverse_iterator end = team_graphic.tiles().rend();
    for ( TeamGraphic::Map::const_reverse_iterator tile = team_graphic.tiles().rbegin();
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
    const int array_size = 1 + tile.colors().size() + tile.height();

    char ** xpm;
    xpm = new char*[ array_size ];

    // header
    xpm[0] = new char[64];
    snprintf( xpm[0], 64, "%d %d %d %d",
              tile.width(), tile.height(),
              static_cast< int >( tile.colors().size() ), tile.cpp() );

    // colors
    std::size_t idx = 1;
    for ( std::vector< boost::shared_ptr< std::string > >::const_iterator col = tile.colors().begin();
          col != tile.colors().end();
          ++col )
    {
        xpm[idx] = new char[ (*col)->length() + 1 ];
        std::strcpy( xpm[idx], (*col)->c_str() );
        ++idx;
    }

    // pixels
    for ( std::vector< std::string >::const_iterator line = tile.pixelLines().begin();
          line != tile.pixelLines().end();
          ++line )
    {
        xpm[idx] = new char[ tile.width() + 1 ];
        std::strcpy( xpm[idx], line->c_str() );
        ++idx;
    }

    copyTeamGraphicXpmTile( dst_pixmap, index.first, index.second, xpm );

    // release memories
    for ( int i = 0; i < array_size; ++i )
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
    if ( dst_pixmap.width() < (x+1) * TeamGraphic::TILE_SIZE
         || dst_pixmap.height() < (y+1) * TeamGraphic::TILE_SIZE )
    {
        return;
    }

    QPixmap pixmap( xpm );

    if ( pixmap.isNull()
         || pixmap.width() != TeamGraphic::TILE_SIZE
         || pixmap.height() != TeamGraphic::TILE_SIZE )
    {
        std::cerr << "Failed to create a pixmap from the xpm tile ("
                  << x << ',' << y << ')'
                  << std::endl;
        return;
    }

    QPainter painter( &dst_pixmap );
    painter.drawPixmap( x * 8, y * 8, pixmap );
    painter.end();
}
