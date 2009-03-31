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

#ifndef FRAME2D_H
#define FRAME2D_H

#include "vector2d.h"
#include "point2d.h"

/**
   interesting facts about frames can be found in the very recommandable book
   by R.P. Paul "Robot Manipulators" MIT Press 1981

   A two dimensional frame has the following shape:

   / n_x  -n_y   p_x \
   |                 |
   | n_y   n_x   p_y |
   |                 |
   \  0     0     1  /

   where we require n_x^2+n_y^2= 1 (i.e. unit length). From this follows, that
   all n_x and  n_y can be represented by the cos(a) or sin(a) values for a particular angle a.

   A translation is a frame of the following form

   /  1     0     x  \
   |                 |
   |  0     1     y  | = T(x,y)
   |                 |
   \  0     0     1  /

   and a rotation can be represented by a frame of the form

   / cos(a) -sin(a) 0 \
   |                  |
   | sin(a) cos(a)  0 | = R(a)
   |                  |
   \   0      0     1 /

*/

class Frame2d {
private:
    /*!
      \brief scale influences the magnification factor of all objects in a frame
      normally scale should always be = 1.0, but if you want all objects in
      your frame to be 2.5 as large, scale should be = 2.5
    */
    double scale;
    double n_x, n_y, p_x, p_y;

public:

    /*!
      \brief init as identity frame
     */
    Frame2d();

    static Frame2d Translation( const double &,
                                const double & );

    static Frame2d Rotation( const Angle & );

    /*!
      \brief get rotation angle
     */
    Angle get_angle() const;

    /*!
      \brief get scalig factor
     */
    double get_scale() const
      {
          return scale;
      }

    /*!
      \brief get translation vector
     */
    Vector2d get_pos() const
      {
          return Vector2d( p_x, p_y );
      }

    Vector2d get_x_axis() const
      {
          return Vector2d( n_x, n_y );
      }

    Vector2d get_y_axis() const
      {
          return Vector2d( -n_y, n_x );
      }

    /*!
      \brief set rotation matrix
     */
    void set_angle( const Angle & );

    /*!
      \brief set rotation matrix correspoinding to the given vector
     */
    void set_angle( const Vector2d & );

    /*!
      \brief set translation matrix
     */
    void set_position( const double &,
                       const double & );

    /*!
      \brief set scaling factor
     */
    void set_scale( const double & );


    Vector2d transform( const Vector2d & v ) const
      {
          return Vector2d( n_x * v.x - n_y * v.y + p_x,
                           n_y * v.x + n_x * v.y + p_y );
      }

    Point2d transform( const Point2d & v ) const
      {
          return Point2d( n_x * v.x - n_y * v.y + p_x,
                          n_y * v.x + n_x * v.y + p_y );
      }

    Frame2d & operator*=( const Frame2d & f );

    /*!
      \brief output matrix values to an output stream
     */
    std::ostream & print( std::ostream & os ) const;

};

inline
Frame2d
operator*( const Frame2d & lhs,
           const Frame2d & rhs )
{
    return Frame2d( lhs ) *= rhs;
}

inline
Vector2d
operator*( const Frame2d & lhs,
           const Vector2d & rhs )
{
    return lhs.transform( rhs );
}

inline
Point2d
operator*( const Frame2d & lhs,
           const Point2d & rhs )
{
    return lhs.transform( rhs );
}

inline
std::ostream &
operator<<( std::ostream & os,
            const Frame2d & f )
{
    return f.print( os );
}

#endif
