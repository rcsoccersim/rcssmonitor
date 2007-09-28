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
  friend std::ostream& operator<< (std::ostream&,const Angle&);
  friend std::istream& operator>> (std::istream&,Angle&);
  friend double cos(const Angle &);
  friend double sin(const Angle &);
  friend Angle operator+(const Angle &, const Angle &);
  friend Angle operator-(const Angle &, const Angle &);
  friend Angle operator-(const Angle &);
  double ang; 
  static const double TwoPi;
  //important class invariant:  0.0 <= ang < 2*PI
 public:
  Angle();
  Angle( const Angle& );
  Angle( double );
  double get_value() const { return ang; }
  void set_value(double);

  void    operator +=(const Angle & a);
  void    operator -=(const Angle & a);
};

#endif

