// -*-c++-*-

/*!
  \file circle_2d.h
  \brief 2D circle region Header File.
*/

/*
 *Copyright:

 Copyright (C) Hidehisa Akiyama

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef RCSSMONITOR_CIRCLE2D_H
#define RCSSMONITOR_CIRCLE2D_H

#include "vector_2d.h"

#include <iostream>

class Line2D;

/*!
  \class Circle2D
  \brief 2d circle class
 */
class Circle2D {
private:

    //! center point
    Vector2D M_center;

    //! radius of this circle
    double M_radius;

    static const double EPSILON;


public:
    /*!
      \brief create a zero area circle at (0,0)
     */
    Circle2D()
        : M_center( 0.0, 0.0 )
        , M_radius( 0.0 )
      { }

    /*!
      \brief constructor with all values
      \param c center point
      \param r radius value
     */
    Circle2D( const Vector2D & c,
              const double & r )
        : M_center( c )
        , M_radius( r )
      {
          if ( r < 0.0 )
          {
              std::cerr << "Circle2D::Circle2D(). radius must be positive value."
                        << std::endl;
              M_radius = 0.0;
          }
      }

    /*!
      \brief assign new value.
      \param c center point
      \param r radius value
      \return const reference to this
     */
    const
    Circle2D & assign( const Vector2D & c,
                       const double & r )
      {
          M_center = c;
          M_radius = r;
          if ( r < 0.0 )
          {
              std::cerr << "Circle2D::assign(). radius must be positive value."
                        << std::endl;
              M_radius = 0.0;
          }
          return *this;
      }

    /*!
      \brief check if point is within this region
      \param point considered point
      \return true if point is contained by this circle
     */
    bool contains( const Vector2D & point ) const
      {
          return M_center.dist2( point ) < M_radius * M_radius;
      }

    /*!
      \brief get the center point
      \return center point coordinate value
     */
    const
    Vector2D & center() const
      {
          return M_center;
      }

    /*!
      \brief get the radius value
      \return radius value
     */
    const
    double & radius() const
      {
          return M_radius;
      }

    /*!
      \brief caluclate the intersection with straight line
      \param line considerd line
      \param sol1 pointer to the 1st solution variable
      \param sol2 pointer to the 2nd solution variable
      \return the number of solution
     */
    int intersection( const Line2D & line,
                      Vector2D * sol1,
                      Vector2D * sol2 ) const;

    /*!
      \brief calculate the intersection with another circle
      \param circle considerd circle
      \param sol1 pointer to the 1st solution variable
      \param sol2 pointer to the 2nd solution variable
      \return the number of solution
     */
    int intersection( const Circle2D & circle,
                      Vector2D * sol1,
                      Vector2D * sol2 ) const;

};

#endif
