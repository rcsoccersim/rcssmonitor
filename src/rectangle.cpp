// -*-c++-*-

/***************************************************************************
                                rectangle.cpp
                             Class for rectangle
                             -------------------
    begin                : 2009-04-02
    copyright            : (C) 2009 by The RoboCup Soccer Server
                           Maintenance Group.
    email                : sserver-admin@lists.sourceforge.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU GPL as published by the Free Software  *
 *   Foundation; either version 3 of the License, or (at your option) any  *
 *   later version.                                                        *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "rectangle.h"

#include <cmath>
#include <cstdlib> // abs()


Rectangle::Rectangle()
    : active( false ),
      use_ratio( false ),
      use_center( false )
{
    set_origin( 0, 0 );
    conform();
}


Rectangle::Rectangle( int window_width,
                      int window_height )
    : width( 0 ),
      height( 0 ),
      use_ratio( false ),
      use_center( false )
{
    //use_center = true;
    set_ratio( window_width, window_height );
    set_origin( 0, 0 );
    conform();
}


void
Rectangle::set_ratio( int window_width,
                      int window_height )
{
    use_ratio = true;
    win_width = window_width;
    win_height = window_height;
}


void
Rectangle::conform()
{
    if ( use_ratio )
    {
        double ratio_width = std::fabs( double( p2x - p1x ) / double( win_width ) );
        double ratio_height = std::fabs( double( p2y - p1y ) / double( win_height ) );

        if ( ratio_width > ratio_height )
        {
            if ( p2y > p1y )
            {
                p2y = p1y + (int)rint( double( win_height ) * ratio_width );
            }
            else
            {
                p2y = p1y - (int)rint( double( win_height ) * ratio_width );
            }
        }
        else
        {
            if ( p2x > p1x )
            {
                p2x = p1x + (int)rint( double( win_width ) * ratio_height );
            }
            else
            {
                p2x = p1x - (int)rint( double( win_width ) * ratio_height );
            }
        }
    }

    width = std::abs( p1x - p2x );
    height = std::abs( p1y - p2y );

    if ( use_center )
    { //center
        x= p1x - width;
        y= p1y - height;
        width *= 2;
        height *= 2;
    }
    else
    {
        x = p1x < p2x ? p1x : p2x;
        y = p1y < p2y ? p1y : p2y;
    }
}
