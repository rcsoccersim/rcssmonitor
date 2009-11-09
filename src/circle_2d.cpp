// -*-c++-*-

/*!
  \file circle_2d.cpp
  \brief 2D circle region Source File.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "circle_2d.h"
#include "line_2d.h"

#include <iostream>
#include <cmath>

// available only this file
namespace {

/*-------------------------------------------------------------------*/
/*!
  \brief get squared value
  \param val input value
  \return squared value
 */
inline
double
SQUARE( const double & val )
{
    return val * val;
}

/*-------------------------------------------------------------------*/
/*!
  \brief solve quadratic fomula
  \param a fomula constant A
  \param b fomula constant B
  \param c fomula constant C
  \param sol1 reference to the result variable
  \param sol2 reference to the result variable
  \return number of solution
 */
inline
int
QUADRATIC_FOMULA( const double & a,
                  const double & b,
                  const double & c,
                  double & sol1,
                  double & sol2 )
{
    double d = SQUARE( b ) - 4.0 * a * c;
    // ignore small noise
    if ( std::fabs( d ) < 0.001 )
    {
        sol1 = -b / (2.0 * a);
        return 1;
    }
    else if ( d < 0.0 )
    {
        return 0;
    }
    else
    {
        d = std::sqrt( d );
        sol1 = (-b + d) / (2.0 * a);
        sol2 = (-b - d) / (2.0 * a);
        return 2;
    }
}

} // end of namespace


/*-------------------------------------------------------------------*/


const double Circle2D::EPSILON = 1.0e-5;

/*-------------------------------------------------------------------*/
/*!

 */
int
Circle2D::intersection( const Line2D & line,
                        Vector2D * sol1,
                        Vector2D * sol2 ) const
{
    if ( std::fabs( line.a() ) < EPSILON )
    {
        if ( std::fabs( line.b() ) < EPSILON )
        {
            std::cerr << "Circle2D::intersection() illegal line."
                      << std::endl;
            return 0;
        }

        // Line:    By + C = 0  ---> y = -C/B
        // Circle:  (x - cx)^2 + (y - cy)^2 = r^2
        // --->
        double x1 = 0.0, x2 = 0.0;
        int n_sol
            = QUADRATIC_FOMULA( 1.0,
                                -2.0 * center().x,
                                ( SQUARE( center().x )
                                  + SQUARE( line.c() / line.b() + center().y )
                                  - SQUARE( radius() ) ),
                                x1,
                                x2 );

        if ( n_sol > 0 )
        {
            double y1 = -line.c() / line.b();

            if ( sol1 )
            {
                sol1->assign( x1, y1 );
            }

            if ( n_sol > 1 && sol2 )
            {
                sol2->assign( x2, y1 );
            }
        }
        return n_sol;
    }
    else
    {
        // include (fabs(l.b()) < EPSILON) case
        // use line & circle formula
        //   Ax + By + C = 0
        //   (x - cx)^2 + (y - cy)^2 = r^2
        // make y's quadratic formula using these fomula.
        double m = line.b() / line.a();
        double d = line.c() / line.a();

        double a = 1.0 + m * m;
        double b = 2.0 * ( -center().y + ( d + center().x ) * m );
        double c = SQUARE( d + center().x )
            + SQUARE( center().y )
            - SQUARE( radius() );

        double y1 = 0.0, y2 = 0.0;
        int n_sol = QUADRATIC_FOMULA( a, b, c,
                                      y1, y2 );

        if ( n_sol > 0 && sol1 )
        {
            sol1->assign( line.getX( y1 ), y1 );
        }

        if ( n_sol > 1 && sol2 )
        {
            sol2->assign( line.getX( y2 ), y2 );
        }

        return n_sol;
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
int
Circle2D::intersection( const Circle2D & circle,
                        Vector2D * sol1,
                        Vector2D * sol2 ) const
{
    double rel_x = circle.center().x - this->center().x;
    double rel_y = circle.center().y - this->center().y;

    double center_dist2 = rel_x * rel_x + rel_y * rel_y;
    double center_dist = std::sqrt( center_dist2 );

    if ( center_dist < std::fabs( this->radius() - circle.radius() )
         || this->radius() + circle.radius() < center_dist )
    {
        return 0;
    }

    // line that passes through the intersection points
    Line2D line( -2.0 * rel_x,
                 -2.0 * rel_y,
                 circle.center().r2()
                 - circle.radius() * circle.radius()
                 - this->center().r2()
                 + this->radius() * this->radius() );

    return this->intersection( line, sol1, sol2 );
}
