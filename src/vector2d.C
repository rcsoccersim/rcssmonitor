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

void Vector2d::init_polar(double n, const Angle& a) {
  x= n*cos(a);
  y= n*sin(a);
}

void Vector2d::operator +=(const Vector2d& v) {
  x += v.x ;
  y += v.y ;
}

void Vector2d::operator -=(const Vector2d& v) {
  x -= v.x ;
  y -= v.y ;
}

void Vector2d::operator *=(double a) {
  x *= a ;
  y *= a ;
}

void Vector2d::operator /=(double a) {
  x /= a ;
  y /= a ;
}

Angle Vector2d::arg() const {
  if (0.0==x && 0.0==y) return 0.0;
  double tmp= atan2(y,x);
  return Angle(tmp);  
}

std::ostream& operator<< (std::ostream& o, const Vector2d& v) {
  return o << "#V[" << v.x << "," << v.y << "]" ;
}

Vector2d operator +(const Vector2d& a, const Vector2d& b) {
  return Vector2d((a.x + b.x), (a.y + b.y)) ;
}

Vector2d operator -(const Vector2d& a, const Vector2d& b) {
  return Vector2d((a.x - b.x), (a.y - b.y)) ;
}

Vector2d operator *(double a, const Vector2d& b) {
  return Vector2d((a * b.x), (a * b.y)) ;
}

double Vector2d::distance(const Vector2d& orig) const {
  return (*this - orig).norm() ;
}

Angle Vector2d::angle(const Vector2d& dir) const {
  return dir.arg() - arg() ;
}

void Vector2d::rotate(const Angle& ang) {
  double r1 = norm() ;
  Angle th1 = arg() ;

  x = r1 * cos(th1 + ang) ;
  y = r1 * sin(th1 + ang) ;
}

