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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "area2d.h"


bool
Area2d::intersects( const Point2d & p ) const
{
    if ( std::fabs( center.x - p.x ) < 0.5*size_x
         && std::fabs( center.y - p.y ) < 0.5*size_y )
    {
        return true;
    }
    return false;
}

bool
Area2d::intersects( const Line2d & l ) const
{
    // ist auf achsenparallele Gerade optimiert!
    if ( std::fabs( center.x - l.p1.x ) > size_x*0.5
         && std::fabs( center.x - l.p2.x ) > size_x*0.5
         && ( ( center.x > l.p1.x && center.x > l.p2.x )
              || ( center.x < l.p1.x && center.x < l.p2.x ) ) )
    {
        return false;
    }

    if ( std::fabs( center.y - l.p1.y ) > size_y*0.5
         && std::fabs( center.y -l.p2.y ) > size_y*0.5
         && ( ( center.y > l.p1.y && center.y > l.p2.y )
              || ( center.y < l.p1.y && center.y < l.p2.y ) ) )
    {
        return false;
    }

    return true;
}

/* all the following routines can be significantly improved */
bool
Area2d::intersects( const Circle2d & c ) const
{
    if ( std::fabs( center.x - c.center.x ) > size_x*0.5 + c.radius )
    {
        return false;
    }

    if ( std::fabs( center.y - c.center.y ) > size_y*0.5 + c.radius )
    {
        return false;
    }

    return true;
}

bool
Area2d::intersects( const CircleArc2d & c ) const
{
    if ( std::fabs( center.x - c.center.x ) > size_x*0.5 + c.radius )
    {
        return false;
    }

    if ( std::fabs( center.y - c.center.y ) > size_y*0.5 + c.radius )
    {
        return false;
    }

    return true;
}

bool
Area2d::intersects_area_of( const Circle2d & c ) const
{
    if ( std::fabs( center.x - c.center.x ) > size_x*0.5 + c.radius )
    {
        return false;
    }

    if ( std::fabs( center.y - c.center.y ) > size_y*0.5 + c.radius )
    {
        return false;
    }

    return true;
}

bool
Area2d::intersects_area_of( const CircleArc2d & c ) const
{
    if ( std::fabs( center.x - c.center.x ) > size_x*0.5 + c.radius )
    {
        return false;
    }

    if ( std::fabs( center.y - c.center.y ) > size_y*0.5 + c.radius )
    {
        return false;
    }

    return true;
}

/*****************************************************************************/
