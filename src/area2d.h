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

#ifndef _AREA2d_
#define _AREA2d_

#include <iostream>
#include "object2d.h"


struct Area2d {
  friend std::ostream& operator<< (std::ostream&,const Area2d&);
  Point2d center;
  double size_x;
  double size_y;
  Area2d() { size_x= 0.0; size_y= 0.0; }
  Area2d(const Point2d &,double sx,double sy); 
  void init(const Point2d &,double sx,double sy); 
  /** most of the routines do get a result, which is conservative, i.e. 
      1) if false is returned, then the is surely no intersection
      2) if true is returned, then the is not nececcerly a intersection.
  */
  bool intersects(const Point2d &) const;
  bool intersects(const Line2d &) const;
  bool intersects(const Circle2d &) const;
  bool intersects(const CircleArc2d &) const;
  bool intersects_area_of(const Circle2d &) const;
  bool intersects_area_of(const CircleArc2d &) const;
};

#endif
