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

#ifndef _VISOBJECT_
#define _VISOBJECT_

#include "vector2d.h"
#include "frame2d.h"
#include "rgbcolor.h"
#include "object2d.h"
#include "area2d.h"
#include "multi.h"
#include "display_base.h"


/** \short Interface class for objects which can be drawn. 
    This class is the base for all drawables. If you derive your
    visual object from this class, you will have to provide the 
    draw and the actualize methods.
 */
class VisualObject2d {
 protected:
  int key;
  int layer;

 public:
  bool visible;

  VisualObject2d();
    virtual ~VisualObject2d()
      { }
  
  int get_key() const { return key; }
  int get_layer() const { return layer; }
  bool get_visible() const { return visible; }

  /// will be removed in future releases
  virtual void set_color(const RGBcolor &) { std::cout << "\n color change not yet supported" << std::flush; }

  ///actualize but don't draw
  virtual void actualize(const Frame2d & p_frame, bool chg)= 0;

  /** \short main interface method
      \param disp this is the Display we want to draw on
      \param area specification for the visual area, can be used for optimizations
      \param p_frame parent frame to which this visual object should conform
      \param chg  idicates if a change in p_frame occured since last call
      This method puts the drawable on the screen. It should call the actualize method
      to assert that the object conforms to the current p_frame.
   */
  virtual void draw(DisplayBase * disp, const Area2d & area, const Frame2d & p_frame, bool chg)=0;
};

class VisualPoint2d: public VisualObject2d {
  DisplayObject * d_obj;
  RGBcolor color;
  Point2d abs;

  void init();
 public:
  Point2d rel;
  bool changed;
  bool use_intersects_area;
  
  VisualPoint2d(); 
  VisualPoint2d(int my_key, int my_layer, const RGBcolor & my_color, const Point2d& my_data);
  virtual ~VisualPoint2d() { if (d_obj) delete d_obj; }
  void draw(DisplayBase * disp, const Area2d & area, const Frame2d & p_frame, bool chg);
  void actualize(const Frame2d& f, bool chg);
  void set_color(const RGBcolor &);
  
  bool intersects_area(const Area2d &);
};

class VisualPoints2d: public VisualObject2d {
  DisplayObject * d_obj;
  RGBcolor color;

  Multi<Point2d> abs;
  void init();
 public:
  Multi<Point2d> rel;
  bool changed;
  bool use_intersects_area;

  VisualPoints2d();
  VisualPoints2d(int my_key, int my_layer, const RGBcolor & my_color, int len_data, const Point2d * data);
  virtual ~VisualPoints2d() { if (d_obj) delete d_obj; }

  void draw(DisplayBase * disp, const Area2d & area, const Frame2d & p_frame, bool chg);
  void actualize(const Frame2d& f, bool chg);
  void set_color(const RGBcolor &);
  void set_points(int len_data, const Point2d * data);
  bool intersects_area(const Area2d &);
};

class VisualLine2d: public VisualObject2d {
  DisplayObject * d_obj;
  RGBcolor color;

  Line2d abs;
  void init();
 public:
  bool changed;
  bool use_intersects_area;
  Line2d rel;

  VisualLine2d();
  VisualLine2d(int my_key, int my_layer, const RGBcolor & my_color,const Line2d& my_data);
  virtual ~VisualLine2d() { if (d_obj) delete d_obj; }
  void draw(DisplayBase * disp, const Area2d & area, const Frame2d & p_frame, bool chg);
  void actualize(const Frame2d& f, bool chg);
  void set_color(const RGBcolor &);
  
  bool intersects_area(const Area2d &);
};

class VisualLines2d: public VisualObject2d {
  DisplayObject * d_obj;
  RGBcolor color;
  Multi<Line2d> abs;

  void init();
 public:
  Multi<Line2d> rel;
  bool changed;
  bool use_intersects_area;

  VisualLines2d();
  VisualLines2d(int my_key, int my_layer, const RGBcolor & my_color, int len_data, const Line2d * data);
  virtual ~VisualLines2d() { if (d_obj) delete d_obj; }
  void draw(DisplayBase * disp, const Area2d & area, const Frame2d & p_frame, bool chg);
  void actualize(const Frame2d& f, bool chg);
  void set_color(const RGBcolor &);
  void set_lines(int len_data, const Line2d * data);

  void set_max_size(int s) {
    abs.set_max_size(s);
    rel.set_max_size(s);
  }
  void set_cur_size(int s) {
    abs.set_cur_size(s);
    rel.set_cur_size(s);
  }
  bool intersects_area(const Area2d &);
};

class VisualCircle2d: public VisualObject2d {
  DisplayObject * d_obj;
  RGBcolor color;

  Circle2d abs;
  void init();
 public:
  Circle2d rel;
  bool changed;
  bool use_intersects_area;
  bool filled;

  VisualCircle2d(); 
  VisualCircle2d(int my_key, int my_layer, const RGBcolor & my_color,const Circle2d & my_data);
  virtual ~VisualCircle2d() { if (d_obj) delete d_obj; }
  void draw(DisplayBase * disp, const Area2d & area, const Frame2d & p_frame, bool chg);
  void actualize(const Frame2d& f, bool chg);
  void set_color(const RGBcolor &);
  
  bool intersects_area(const Area2d &);
};

class VisualCircles2d: public VisualObject2d {
  DisplayObject * d_obj;
  RGBcolor color;

  Multi<Circle2d> abs;
  void init();
 public:
  Multi<Circle2d> rel;
  bool changed;
  bool use_intersects_area;
  bool filled;

  VisualCircles2d();
  VisualCircles2d(int my_key, int my_layer, const RGBcolor & my_color, int len_data, const Circle2d * data);
  virtual ~VisualCircles2d() { if (d_obj) delete d_obj; }
  void draw(DisplayBase * disp, const Area2d & area, const Frame2d & p_frame, bool chg);
  void actualize(const Frame2d& f, bool chg);
  void set_color(const RGBcolor &);
  void set_circles(int len_data, const Circle2d * data);

  bool intersects_area(const Area2d &);
};

class VisualCircleArc2d: public VisualObject2d {
  DisplayObject * d_obj;
  RGBcolor color;
  CircleArc2d abs;

  void init();
 public:
  CircleArc2d rel;
  bool changed;
  bool use_intersects_area;
  bool filled;

  VisualCircleArc2d();
  VisualCircleArc2d(int my_key, int my_layer, const RGBcolor & my_color,const CircleArc2d & my_data);
  virtual ~VisualCircleArc2d() { if (d_obj) delete d_obj; }
  void draw(DisplayBase * disp, const Area2d & area, const Frame2d & p_frame, bool chg);
  void actualize(const Frame2d& f, bool chg);
  void set_color(const RGBcolor &);
  
  bool intersects_area(const Area2d &);
};

class VisualCircleArcs2d: public VisualObject2d {
  DisplayObject * d_obj;
  RGBcolor color;

  Multi<CircleArc2d> abs;
  void init();
 public:
  Multi<CircleArc2d> rel;
  bool changed;
  bool use_intersects_area;
  bool filled;

  VisualCircleArcs2d();
  VisualCircleArcs2d(int my_key, int my_layer, const RGBcolor & my_color, int len_data, const CircleArc2d * data);
  virtual ~VisualCircleArcs2d() { if (d_obj) delete d_obj; }
  void draw(DisplayBase * disp, const Area2d & area, const Frame2d & p_frame, bool chg);
  void actualize(const Frame2d& f, bool chg);
  void set_color(const RGBcolor &);
  void set_circlearcs(int len_data, const CircleArc2d * data);

  bool intersects_area(const Area2d &);
};

class VisualPolyline2d: public VisualObject2d {
  DisplayObject * d_obj;
  RGBcolor color;

  Multi<Point2d> abs;
  void init();
 public:
  Multi<Point2d> rel;
  bool changed;
  bool use_intersects_area;

  VisualPolyline2d();
  VisualPolyline2d(int my_key, int my_layer, const RGBcolor & my_color, int len_data, const Point2d * data);
  virtual ~VisualPolyline2d() { if (d_obj) delete d_obj; }
  void draw(DisplayBase * disp, const Area2d & area, const Frame2d & p_frame, bool chg);
  void actualize(const Frame2d& f, bool chg);
  void set_color(const RGBcolor &);
  void set_points(int len_data, const Point2d * data);  

  bool intersects_area(const Area2d &);
};

class VisualPolygon2d: public VisualObject2d {
  DisplayObject * d_obj;
  RGBcolor color;

  Multi<Point2d> abs;
  void init();
 public:
  Multi<Point2d> rel;
  bool changed;
  bool use_intersects_area;
  bool filled;
  
  VisualPolygon2d();
  VisualPolygon2d(int my_key, int my_layer, const RGBcolor & my_color, int len_data, const Point2d * data);
  virtual ~VisualPolygon2d() { if (d_obj) delete d_obj; }
  void draw(DisplayBase * disp, const Area2d & area, const Frame2d & p_frame, bool chg);
  void actualize(const Frame2d& f, bool chg);
  void set_color(const RGBcolor &);
  void set_points(int len_data, const Point2d * data);

  bool intersects_area(const Area2d &);
};

class VisualString2d: public VisualObject2d {
  DisplayObject * d_obj;
  RGBcolor color;

  bool use_intersects_area;
  Point2d abs;

  void init();
 public:
  bool changed;
  Point2d rel;
  Multi<char> content;

  VisualString2d(int my_key, int my_layer, const RGBcolor & my_color, const Point2d & my_pos, const Multi<char> & my_data);
  VisualString2d(int my_key, int my_layer, const RGBcolor & my_color, const Point2d & my_pos, int len_data, const char * data);
  VisualString2d(int my_key, int my_layer);

  VisualString2d();

  virtual ~VisualString2d() { if (d_obj) delete d_obj; }
  void draw(DisplayBase * disp, const Area2d & area, const Frame2d & p_frame, bool chg);
  void actualize(const Frame2d& f, bool chg);

  void set_color(const RGBcolor &);
  void set_changed() { changed= true; }

  bool intersects_area(const Area2d &);
};

#endif
