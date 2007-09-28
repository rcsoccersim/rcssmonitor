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

#include "frame2d.h"

std::ostream& operator<< (std::ostream& o,const Frame2d& f) {
  return o << "\n   [" << f.n_x << " " << -f.n_y << " " << f.p_x << "]"
	   << "\n   [" << f.n_y << " " <<  f.n_x << " " << f.p_y << "]";
}

Frame2d operator*(const Frame2d &f1, const Frame2d &f2) {
  Frame2d res;
  res.scale= f1.scale* f2.scale;
  res.n_x= f1.n_x*f2.n_x - f1.n_y*f2.n_y;
  res.n_y= f1.n_y*f2.n_x + f1.n_x*f2.n_y;
  res.p_x= f1.n_x*f2.p_x - f1.n_y*f2.p_y + f1.p_x;
  res.p_y= f1.n_y*f2.p_x + f1.n_x*f2.p_y + f1.p_y;
  return res;
}

Vector2d operator*(const Frame2d &f1, const Vector2d &v) {
  Vector2d res;
  res.x= f1.n_x*v.x - f1.n_y*v.y + f1.p_x;
  res.y= f1.n_y*v.x + f1.n_x*v.y + f1.p_y;
  return res;
}

Point2d operator*(const Frame2d &f1, const Point2d &v) {
  Point2d res;
  res.x= f1.n_x*v.x - f1.n_y*v.y + f1.p_x;
  res.y= f1.n_y*v.x + f1.n_x*v.y + f1.p_y;
  return res;
}

Frame2d::Frame2d() {
  scale= 1.0;
  n_x= 1.0;
  n_y= 0.0;
  p_x= 0.0;
  p_y= 0.0;
}


Angle Frame2d::get_angle() const {
  Vector2d vec(n_x,n_y);
  return vec.arg();
}

void Frame2d::set_angle(const Angle & a) {
  n_x= cos(a) * scale;
  n_y= sin(a) * scale;
  //f1.n_y= sqrt(1-f1.n_x*f1.n_x);
}

void Frame2d::set_angle(const Vector2d & vec) {
  double norm= vec.norm();
  if (norm == 0.0) {
    //this is consistent with vec.arg() == 0.0 if vec.norm() == 0.0
    n_x= 1.0 * scale;
    n_y= 0.0;
    return;
  }
  norm= scale/norm;
  n_x= vec.x * norm;
  n_y= vec.y * norm;
  //f1.n_y= sqrt(1-f1.n_x*f1.n_x);
}

void Frame2d::set_position(double x,double y) {
  p_x= x;
  p_y= y;
}

void Frame2d::set_scale(double s) {
  n_x /= scale;
  n_y /= scale;
  scale= s;
  n_x *= scale;
  n_y *= scale;
}

Frame2d Frame2d::Translation(double x, double y) {
  Frame2d f1;
  f1.set_position(x,y);
  return f1;
}

Frame2d Frame2d::Rotation(const Angle& a) {
  Frame2d f1;
  f1.set_angle(a);
  return f1;
}
/*****************************************************************************/
