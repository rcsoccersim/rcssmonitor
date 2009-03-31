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


#ifndef CONV_H
#define CONV_H

#include "angle.h"
#include "point2d.h"

#include <iostream>

/**
  The class WinPlaneConverter converts coordintes and sizes between a window and the
  euclidean plane


  a window is given through

                   width
            /---------------------\
            |                     |
    height  |                     |
            |                     |
            \---------------------/

  where the x-coordinates grow from left to right (taking values [0,...,width] )
  and the y-coordinats grow from top to down (taking values [0,...,height] )
  Angles in a window are given in degrees (taking values [0,...,360) )

            | 90 degree
	        |
   180      |          0 degree
     -------O-------->
	        |
            |
            | 270 degree

  ------------------------------------------------------------------------

  A clipping from the euclidean plane is defined through his south-east
  corner and the x, y size. This is the internal representation. At
  the interface we use a clipping characterization, which uses the
  center of the clipping area, and NOT the south-east corner. This makes it
  compatible to the Area2d class.

  We use the usual mathematical coordinate system

            ^ Y axis
           /|\
            |
            |
     -------O-------->  X axis
            |
            |
            |

  Angles are given in radias, and normalized to be in [0,...,2*PI).
  It is allowed to use values not from [0,...,2*PI) as parameters, they
  will be always normalized to fit into this interval.


            | 0.5 Pi
            |
   Pi       |          0
     -------O-------->
            |
            |
            | 1.5 Pi

  The coordinates to be converted are not constrained to lie in the given
  clipping areas.
*/

class WinPlaneConverter {
private:
    int Win_width;
    int Win_height;
    double Plane_east_x;
    double Plane_size_x;
    double Plane_south_y;
    double Plane_size_y;
public:
    WinPlaneConverter();
    WinPlaneConverter( int width,
                       int height,
                       const Point2d & center,
                       double size_x,
                       double size_y );
    void init( int width,
               int height,
               const Point2d & center,
               double size_x,
               double size_y );
    void init_win( int width,
                   int height );
    void init_plane( const Point2d & center,
                     double size_x,
                     double size_y );

    double x_Win_to_x_Plane( int x ) const
      {
          return Plane_east_x + Plane_size_x * x / Win_width;
      }

    double y_Win_to_y_Plane( int y ) const
      {
          return Plane_south_y + Plane_size_y * ( Win_height - y ) / Win_height;
      }

    double size_x_Win_to_size_x_Plane( int sx ) const
      {
          return ( Plane_size_x * sx ) / double( Win_width );
      }

    double size_y_Win_to_size_y_Plane( int sy ) const
      {
          return ( Plane_size_y * sy ) / double( Win_height );
      }

    int x_Plane_to_x_Win( double x ) const
      {
          return ( int )floor( ( double( Win_width ) * ( x - Plane_east_x ) ) / Plane_size_x );
      }

    int y_Plane_to_y_Win( double y ) const
      {
          return ( int )floor( double( Win_height ) - ( double( Win_height )*( y - Plane_south_y ) ) / Plane_size_y );
      }

    int size_x_Plane_to_size_x_Win( double s ) const
      {
          return ( int )rint( ( double( Win_width )*s ) / Plane_size_x );
      }

    int size_y_Plane_to_size_y_Win( double s ) const
      {
          return ( int )rint( ( double( Win_height )*s ) / Plane_size_y );
      }

    int angle_Plane_to_angle_Win( const Angle & a ) const
      {
          return ( int )rint( 64*a.get_value()*180.0 / M_PI );
      }

    void get_Win( int & width, int & height ) const;
    void get_Plane( Point2d & center, double & size_x, double & size_y ) const;

    std::ostream & print( std::ostream & os ) const;
};

inline
std::ostream &
operator<<( std::ostream & os,
            const WinPlaneConverter & c )
{
    return c.print( os );
}

#endif
