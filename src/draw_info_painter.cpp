// -*-c++-*-

/*!
  \file draw_info_painter.cpp
  \brief draw info painter class Source File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa Akiyama

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

#include "draw_info_painter.h"

#include "disp_holder.h"
#include "options.h"

#include <iostream>

/*-------------------------------------------------------------------*/
/*!

*/
DrawInfoPainter::DrawInfoPainter( const DispHolder & disp_holder )
    : M_disp_holder( disp_holder )
    , M_pen( QColor( 255, 255, 255 ), 0, Qt::SolidLine )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
DrawInfoPainter::~DrawInfoPainter()
{

}

/*-------------------------------------------------------------------*/
/*!

*/
void
DrawInfoPainter::draw( QPainter & painter )
{
    const Options & opt = Options::instance();

    if ( ! opt.showDrawInfo() )
    {
        return;
    }

    DispConstPtr disp = M_disp_holder.currentDisp();

    if ( ! disp )
    {
        return;
    }

    const int current_time = disp->show_.time_;

    painter.setBrush( Qt::NoBrush );

    //
    // draw point
    //
    {
        PointCont::const_iterator p = M_disp_holder.pointCont().find( current_time );
        if ( p != M_disp_holder.pointCont().end() )
        {
            const PointCont::const_iterator end = M_disp_holder.pointCont().upper_bound( current_time );
            do
            {
                QColor col( p->second.color_.c_str() );
                if ( col.isValid() )
                {
                    M_pen.setColor( col );
                    painter.setPen( M_pen );
                    painter.drawRect( opt.screenX( p->second.x_ ) - 1,
                                      opt.screenY( p->second.y_ ) - 1,
                                      3, 3 );
                }
                ++p;
            }
            while ( p != end );
        }
    }

    //
    // draw circle
    //
    {
        CircleCont::const_iterator c = M_disp_holder.circleCont().find( current_time );
        if ( c != M_disp_holder.circleCont().end() )
        {
            const CircleCont::const_iterator end = M_disp_holder.circleCont().upper_bound( current_time );
            do
            {
                QColor col( c->second.color_.c_str() );
                if ( col.isValid() )
                {
                    M_pen.setColor( col );
                    painter.setPen( M_pen );

                    int r = opt.scale( c->second.r_ );
                    painter.drawEllipse( opt.screenX( c->second.x_ ) - r,
                                         opt.screenY( c->second.y_ ) - r,
                                         r * 2,
                                         r * 2 );
                }

                ++c;
            }
            while ( c != end );
        }
    }

    //
    // draw line
    //
    {
        LineCont::const_iterator l = M_disp_holder.lineCont().find( current_time );
        if ( l != M_disp_holder.lineCont().end() )
        {
            const LineCont::const_iterator end = M_disp_holder.lineCont().upper_bound( current_time );
            do
            {
                QColor col( l->second.color_.c_str() );
                if ( col.isValid() )
                {
                    M_pen.setColor( col );
                    painter.setPen( M_pen );

                    painter.drawLine( opt.screenX( l->second.x1_ ),
                                      opt.screenY( l->second.y1_ ),
                                      opt.screenX( l->second.x2_ ),
                                      opt.screenY( l->second.y2_ ) );
                }

                ++l;
            }
            while ( l != end );
        }
    }

}
