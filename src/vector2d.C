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

#include "vector2d.h"

Angle
Vector2d::arg() const
{
    if ( 0.0 == x && 0.0 == y )
    {
        return Angle( 0.0 );
    }

    return Angle( std::atan2( y, x ) );
}

const
Vector2d &
Vector2d::rotate( const Angle & ang )
{
    double r1 = norm();
    Angle a = arg() + ang;

    x = r1 * a.cos();
    y = r1 * a.sin();

    return *this;
}
