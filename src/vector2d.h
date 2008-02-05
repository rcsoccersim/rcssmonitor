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

#ifndef _VECTOR2D_H_
#define _VECTOR2D_H_

#include "angle.h"

#include <iostream>
#include <cmath>

/**
   The class Vector2d is used to represent 2 dimensional vectors. It supports
   (almost) all operations  you can perform with 2 dimensional vectors.
*/
class Vector2d {
public:
    double x;
    double y;

    explicit
    Vector2d( const double & vx = 0.0,
              const double & vy = 0.0 )
        : x( vx ),
          y( vy )
      { }

    const
    Vector2d & init_polar( const double & n,
                           const Angle & a )
      {
          x = n * a.cos();
          y = n * a.sin();
          return *this;
      }

    const
    Vector2d & operator+=( const Vector2d & v )
      {
          x += v.x;
          y += v.y;
          return *this;
      }

    const
    Vector2d & operator-=( const Vector2d & v )
      {
          x -= v.x;
          y -= v.y;
          return *this;
      }

    const
    Vector2d & operator*=( const double & a )
      {
          x *= a;
          y *= a;
          return *this;
      }

    const
    Vector2d & operator/=( const double & a )
      {
          x /= a;
          y /= a;
          return *this;
      }

    /// computes the euclidian norm of the vector
    double norm() const
      {
          return std::sqrt( x*x + y*y );
      }

    /** computes the argument of this vector as it would be a complex number. In
        other words it computes the angle between this vector and the vector
        (1,0). If this vector in the null vector a 0 is returned */
    Angle arg() const;

    /** */
    //void normalize(double l = 1.0);

    /// computes the euclidian distance between this vector and the vector given as a parameter
    double distance( const Vector2d & orig ) const;

    /// computes the angle between this vector and the vector given as a parameter
    Angle angle( const Vector2d & dir ) const
      {
          return dir.arg() - arg();
      }

    /// rotates the vector by the given angle
    const
    Vector2d & rotate( const Angle & ang );

};

inline
std::ostream &
operator<<( std::ostream & o,
            const Vector2d & v )
{
    return o << "#V[" << v.x << "," << v.y << "]";
}

inline
Vector2d
operator+( const Vector2d & lhs,
           const Vector2d & rhs )
{
    return Vector2d( lhs ) += rhs;
}

inline
Vector2d
operator-( const Vector2d & lhs,
           const Vector2d & rhs )
{
    return Vector2d( lhs ) -= rhs;
}

inline
Vector2d
operator*( const double & lhs,
           const Vector2d & rhs )
{
    return Vector2d( rhs ) *= lhs;
}

inline
double
Vector2d::distance( const Vector2d & orig ) const
{
    return ( *this - orig ).norm();
}

#endif
