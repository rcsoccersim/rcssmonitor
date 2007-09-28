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

#ifndef _OBJECT2D_H_
#define _OBJECT2D_H_

#include "angle.h"
#include "point2d.h"

struct Line2d {
  Point2d p1;
  Point2d p2;
  Line2d() {}
  Line2d(const Point2d& pp1, const Point2d& pp2) { p1= pp1; p2= pp2;}
};

struct Circle2d {
  Point2d center;
  double radius;
  Circle2d() { radius= 0.0; }
  Circle2d(const Point2d& c,double r) { center= c; radius= r; }
};

struct CircleArc2d {
  Point2d center;
  double radius;
  Angle ang1;
  Angle ang2;
  CircleArc2d() { radius= 0.0; }
  CircleArc2d(const Point2d& c,double r,const Angle& a1,const Angle& a2) { 
    center= c; radius= r; ang1= a1; ang2= a2;
  }
};
#endif
