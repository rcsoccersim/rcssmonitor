/*
 * Copyright (c) 1999 - 2001, Artur Merke <amerke@ira.uka.de>
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _ANGLE_H_
#define _ANGLE_H_

#include <cmath>
#include <iostream>

/**
   This class is used for unambiguous representation of angles.

   Angles are given in radians, and normalized to be in [0,...,2*PI).
   It is allowed to use values not from [0,...,2*PI) as parameters, they
   will be always normalized to fit into this interval.

            ^
            | 0.5 Pi
            |
   Pi       |          0
     -------O-------->
            |
            |
            | 1.5 Pi
*/
class Angle {
private:

    static const double TwoPi;
    //important class invariant:  0.0 <= ang < 2*PI

    double ang;

public:

    Angle()
        : ang( 0.0 )
      { }

    Angle( const Angle & a )
        : ang( a.ang )
      { }

    Angle( const double & a )
      {
          set_value( a );
      }

    const
    double & get_value() const
      {
          return ang;
      }

    void set_value( const double & );

    double cos() const
      {
          return std::cos( this->ang );
      }

    double sin() const
      {
          return std::sin( this->ang );
      }

    Angle operator-() const
      {
          Angle res( *this );
          res.ang = -res.ang + Angle::TwoPi;
          return res;
      }

    const
    Angle & operator+=(const Angle & a);
    const
    Angle & operator-=(const Angle & a);
};


inline
Angle
operator+( const Angle & lhs,
           const Angle & rhs )
{
    return Angle( lhs ) += rhs;
}

inline
Angle
operator-( const Angle & lhs,
           const Angle & rhs )
{
    return Angle( lhs ) -= rhs;
}


#endif
