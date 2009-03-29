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

#ifndef ANGLE_H
#define ANGLE_H

#include <cmath>
#include <iostream>

/*!
  \class Angle
  \brief This class is used for unambiguous representation of angles.

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

    double M_angle;

public:

    Angle()
        : M_angle( 0.0 )
      { }

    Angle( const double & a )
      {
          assign( a );
      }

    const
    double & get_value() const
      {
          return M_angle;
      }

    const
    Angle & assign( const double & d )
      {
          M_angle = std::fmod( d, TwoPi );
          if ( M_angle < 0.0 )
          {
              M_angle += TwoPi;
          }
          return *this;
      }

    double cos() const
      {
          return std::cos( this->M_angle );
      }

    double sin() const
      {
          return std::sin( this->M_angle );
      }

    Angle operator-() const
      {
          Angle res( *this );
          res.M_angle = -res.M_angle + TwoPi;
          return res;
      }

    const
    Angle & operator+=( const Angle & a )
      {
          M_angle += a.M_angle;
          if ( M_angle >= TwoPi )
          {
              M_angle -= TwoPi;
          }
          return *this;
      }

    const
    Angle & operator-=( const Angle & a )
      {
          M_angle -= a.M_angle;
          if ( M_angle < 0.0 )
          {
              M_angle += TwoPi;
          }
          return *this;
      }

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
