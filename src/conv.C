/*
 * Copyright (c) 1999 - 2001, Artur Merke <amerke@ira.uka.de>
 *
 * This file is part of FrameView2d.
 *
 * FrameView2d is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * FrameView2d is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FrameView2d; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "conv.h"

#include <cmath>
#include <iostream>
#include "global_defs.h"

std::ostream &
operator<<( std::ostream & o,
            const WinPlaneConverter & c )
{
    std::cout << "[ Win= (" << c.Win_width << ", " << c.Win_height << ")"
              << "  Plane= (" << c.Plane_east_x << ", " << c.Plane_south_y << ")"
              << "  size=(" << c.Plane_size_x << ", " << c.Plane_size_y << ") ]";
    return o;
}


WinPlaneConverter::WinPlaneConverter()
{
}

WinPlaneConverter::WinPlaneConverter( int width,
                                      int height,
                                      const Point2d & center,
                                      double size_x,
                                      double size_y )
{
    init( width, height, center, size_x, size_y );
}

void
WinPlaneConverter::init( int width,
                         int height,
                         const Point2d & center,
                         double size_x,
                         double size_y )
{
    init_win( width, height );
    init_plane( center, size_x, size_y );
}

void
WinPlaneConverter::init_win( int width,
                             int height )
{
    if ( width < 20 || height < 20 )
    {
        ERROR_OUT << "wrong view parameter";
    }

    Win_width    = width;
    Win_height    = height;
}


void
WinPlaneConverter::init_plane( const Point2d & center,
                               double size_x,
                               double size_y )
{
    if ( size_x < 0 || size_y < 0 )
    {
        ERROR_OUT << "wrong view parameter";
    }

    Plane_size_x = size_x;
    Plane_size_y = size_y;

    Plane_east_x  = center.x - size_x * 0.5;
    Plane_south_y = center.y - size_y * 0.5;
}

void
WinPlaneConverter::get_Win( int & width,
                            int & height ) const
{
    width = Win_width;
    height = Win_height;
}

void
WinPlaneConverter::get_Plane( Point2d & center,
                              double & size_x,
                              double & size_y ) const
{
    size_x = Plane_size_x;
    size_y = Plane_size_y;
    center.x = Plane_east_x + size_x * 0.5;
    center.y = Plane_south_y + size_y * 0.5;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/******************** T E S T ********* T E S T*******************************/

#if 0

main()
{
    WinPlaneConverter v( 40, 40, -50, -30, 50, 30 );

    std::cout << "\n" << v.x_Win_to_x_Plane( v.x_Plane_to_x_Win( 0.0 ) );
    std::cout << "\n" << v.x_Plane_to_x_Win( -50.0 );
    std::cout << "\n" << v.y_Plane_to_y_Win( -30.0 );
}



#endif
