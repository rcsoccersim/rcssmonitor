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

#ifndef _DISPLAY_BASE_
#define _DISPLAY_BASE_

#include "rgbcolor.h"
#include "vector2d.h"
#include "object2d.h"

struct DisplayObject {
  DisplayObject() { mark_pos_change(); mark_col_change(); }
    virtual
    ~DisplayObject() 
      { }
  void mark_pos_change() { area_number= 0; }
  void mark_col_change() { col_change= true; }
  virtual void set_max_size(int size) {};

  bool col_change;
  unsigned int area_number;
};

class DisplayBase {
public:
    virtual
    ~DisplayBase()
      { }

  virtual void draw_point(DisplayObject * d_obj, const RGBcolor & col, const Point2d & )=0;
  virtual DisplayObject * create_point()=0; //{ return 0; }

  virtual void draw_points(DisplayObject * d_obj, const RGBcolor & col, int num, const Point2d * )=0;
  virtual DisplayObject * create_points(int num)=0; //{ return 0; }

  //the endpoints of the line are absolute coordinates
  virtual void draw_line(DisplayObject * d_obj, const RGBcolor & col, const Line2d & )=0;
  virtual DisplayObject * create_line()=0; //{ return 0; }

  virtual void draw_lines(DisplayObject * d_obj, const RGBcolor & col, int num, const Line2d * )=0;
  virtual DisplayObject * create_lines(int num)=0; //{ return 0; }

  virtual void draw_circle(DisplayObject * d_obj, const RGBcolor & col, const Circle2d & )=0;
  virtual void fill_circle(DisplayObject * d_obj, const RGBcolor & col, const Circle2d & )=0;
  virtual DisplayObject * create_circle()=0; //{ return 0; }

  virtual void draw_circles(DisplayObject * d_obj, const RGBcolor & col, int num, const Circle2d * )=0;
  virtual void fill_circles(DisplayObject * d_obj, const RGBcolor & col, int num, const Circle2d * )=0;
  virtual DisplayObject * create_circles(int num)=0; //{ return 0; }


  //specifies an arc with absolute begin in ang1 and absolute!!! end in ang2, the path goes from ang1 to ang2 couterclockwise
  virtual void draw_circlearc(DisplayObject * d_obj, const RGBcolor & col, const CircleArc2d & )=0;
  virtual void fill_circlearc(DisplayObject * d_obj, const RGBcolor & col, const CircleArc2d & )=0;
  virtual DisplayObject * create_circlearc()=0; //{ return 0; }

  virtual void draw_circlearcs(DisplayObject * d_obj, const RGBcolor & col, int num, const CircleArc2d * )=0;
  virtual void fill_circlearcs(DisplayObject * d_obj, const RGBcolor & col, int num, const CircleArc2d * )=0;
  virtual DisplayObject * create_circlearcs(int num)=0; //{ return 0; }

  //all boudary points of the polygon are treated as absolute coordinates
  virtual void draw_polyline(DisplayObject * d_obj, const RGBcolor & col, int num, const Point2d * )=0;
  virtual DisplayObject * create_polyline(int num)=0; //{ return 0; }

  virtual void draw_polygon(DisplayObject * d_obj, const RGBcolor & col, int num, const Point2d * )=0;
  virtual void fill_polygon(DisplayObject * d_obj, const RGBcolor & col, int num, const Point2d * )=0;
  virtual DisplayObject * create_polygon(int num)=0; //{ return 0; }

  virtual void draw_string(DisplayObject * d_obj, const RGBcolor & col, const Point2d &,int len, const char * str)=0;
  virtual DisplayObject * create_string()=0; //{ return 0; }

  //
  virtual void set_background_color(const RGBcolor & col)=0;
};
#endif
