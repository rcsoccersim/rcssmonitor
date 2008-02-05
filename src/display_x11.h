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

#ifndef _DISPLAY_X11_
#define _DISPLAY_X11_

#include "X11/Xlib.h"
#include "display_base.h"
#include "conv.h"
#include "area2d.h"
#include "multi.h"

#include <algorithm>


class DisplayX11: public DisplayBase {
private:
  Display *disp;
  //Window win;
  Pixmap pixmap;
  GC gc;
  WinPlaneConverter conv;
  unsigned int area_number;
  void init_area_number();
  void step_area_number();

  void ASetForeground(unsigned long pix) { XSetForeground (disp, gc, pix); }

 public:
  DisplayX11(Display*,Pixmap,GC);
  virtual ~DisplayX11() {}
  void ASetNewPixmap(Pixmap p) { pixmap= p; }
  void ASetPlainArea( const Area2d &);
  void ASetWindowArea(int width, int height);

  unsigned long AGetColor(const RGBcolor&);

#if 0
  void ASetForeground(char*);
  void ASetBackground(char*);
  void AFillRectangle(double a1,double a2,double w,double h);
  void ARefresh();
  void AReset();
#endif
  //here the interface of DisplayBase is filled
 protected:
  struct D_XPoint: public DisplayObject {
    virtual ~D_XPoint() {}
    unsigned long XColor_pixel;
    XPoint p;
  };

  struct D_XPoints: public DisplayObject {
    D_XPoints(int num) { pp.set_max_size(num); }
    virtual ~D_XPoints() {}
    unsigned long XColor_pixel;
    Multi<XPoint> pp;
    /** because D_XPoints is also used in representations of Polygons
	(which need one point more) at this point we just spend an
	additional field element!
    */
    void set_max_size(int num) { pp.set_max_size(num+1); }
  };

  struct D_XSegment: public DisplayObject {
    virtual ~D_XSegment() {}
    unsigned long XColor_pixel;
    XSegment s;
  };

  struct D_XSegments: public DisplayObject {
    D_XSegments(int num) { ss.set_max_size(num); }
    virtual ~D_XSegments() {}
    unsigned long XColor_pixel;
    Multi<XSegment> ss;
    void set_max_size(int num) { ss.set_max_size(num); }
  };

  struct D_XArc: public DisplayObject {
    virtual ~D_XArc() {}
    unsigned long XColor_pixel;
    XArc a;
  };

  struct D_XArcs: public DisplayObject {
    D_XArcs(int num) { aa.set_max_size(num); }
    virtual ~D_XArcs() {}
    unsigned long XColor_pixel;
    Multi<XArc> aa;
    void set_max_size(int num) { aa.set_max_size(num); }
  };

  D_XPoint d_xpoint;
  D_XPoints d_xpoints;

  D_XSegment d_xsegment;
  D_XSegments d_xsegments;

  D_XArc d_xarc;
  D_XArcs d_xarcs;
 public:
  void draw_point(DisplayObject * d_obj, const RGBcolor & col, const Point2d & );
  DisplayObject * create_point() { return new(D_XPoint); }

  void draw_points(DisplayObject * d_obj, const RGBcolor & col, int num, const Point2d * );
  DisplayObject * create_points(int num) { return new(D_XPoints)(num); }

  //the endpoints of the line are absolute coordinates
  void draw_line(DisplayObject * d_obj, const RGBcolor & col, const Line2d & );
  DisplayObject * create_line() { return new(D_XSegment); }

  void draw_lines(DisplayObject * d_obj, const RGBcolor & col, int num, const Line2d * );
  DisplayObject * create_lines(int num) { return new(D_XSegments)(num); }

  void draw_circle(DisplayObject * d_obj, const RGBcolor & col, const Circle2d & );
  void fill_circle(DisplayObject * d_obj, const RGBcolor & col, const Circle2d & );
  DisplayObject * create_circle() { return new(D_XArc); }

  void draw_circles(DisplayObject * d_obj, const RGBcolor & col, int num, const Circle2d * );
  void fill_circles(DisplayObject * d_obj, const RGBcolor & col, int num, const Circle2d * );
  DisplayObject * create_circles(int num) { return new(D_XArcs)(num); }


  //specifies an arc with absolute begin in ang1 and absolute!!! end in ang2, the path goes from ang1 to ang2 couterclockwise
  void draw_circlearc(DisplayObject * d_obj, const RGBcolor & col, const CircleArc2d & );
  void fill_circlearc(DisplayObject * d_obj, const RGBcolor & col, const CircleArc2d & );
  DisplayObject * create_circlearc() { return new(D_XArc); }

  void draw_circlearcs(DisplayObject * d_obj, const RGBcolor & col, int num, const CircleArc2d * );
  void fill_circlearcs(DisplayObject * d_obj, const RGBcolor & col, int num, const CircleArc2d * );
  DisplayObject * create_circlearcs(int num) { return new(D_XArcs)(num); }

  //all boudary points of the polygon are treated as absolute coordinates
  void draw_polyline(DisplayObject * d_obj, const RGBcolor & col, int num, const Point2d * );
  DisplayObject * create_polyline(int num) { return new(D_XPoints)(num); }

  void draw_polygon(DisplayObject * d_obj, const RGBcolor & col, int num, const Point2d * );
  void fill_polygon(DisplayObject * d_obj, const RGBcolor & col, int num, const Point2d * );
  DisplayObject * create_polygon(int num) { return new(D_XPoints)(num+1); }

  void draw_string(DisplayObject * d_obj, const RGBcolor & col, const Point2d &,int len, const char * str);
  DisplayObject * create_string() { return new(D_XPoint); }

  //
  void set_background_color(const RGBcolor & col);
 protected:

  inline void copy_2_XPoint(XPoint & p, const Point2d & point) const {
    p.x= conv.x_Plane_to_x_Win(point.x);
    p.y= conv.y_Plane_to_y_Win(point.y);
  }

  inline void copy_2_XSegment(XSegment & s, const Line2d & line) const {
    s.x1= conv.x_Plane_to_x_Win(line.p1.x);
    s.y1= conv.y_Plane_to_y_Win(line.p1.y);
    s.x2= conv.x_Plane_to_x_Win(line.p2.x);
    s.y2= conv.y_Plane_to_y_Win(line.p2.y);
  }

  inline void copy_2_XArc(XArc & a, const Circle2d & circle) const {
    const short ang= 360*64;
#if 0
    a.x= conv.x_Plane_to_x_Win(circle.center.x-circle.radius);
    a.y= conv.y_Plane_to_y_Win(circle.center.y+circle.radius);
    a.width= conv.size_x_Plane_to_size_x_Win(circle.radius*2);
    a.height= conv.size_y_Plane_to_size_y_Win(circle.radius*2);
#else
    a.width= std::max( conv.size_x_Plane_to_size_x_Win(circle.radius), circle.min_pixel);
    a.height= std::max( conv.size_y_Plane_to_size_y_Win(circle.radius), circle.min_pixel);
    a.x= conv.x_Plane_to_x_Win(circle.center.x);
    a.y= conv.y_Plane_to_y_Win(circle.center.y);
    a.x-= a.width;
    a.y-= a.height;
    a.width *= 2;
    a.height *= 2;
#endif
    a.angle1= 0;
    a.angle2= ang;
  }

  inline void copy_2_XArc(XArc & a, const CircleArc2d & circlearc) const {
    const short two_pi= 360*64;
    Angle ang = circlearc.ang2 - circlearc.ang1;
    a.x= conv.x_Plane_to_x_Win(circlearc.center.x-circlearc.radius);
    a.y= conv.y_Plane_to_y_Win(circlearc.center.y+circlearc.radius);
    a.width= conv.size_x_Plane_to_size_x_Win(circlearc.radius*2);
    a.height= conv.size_y_Plane_to_size_y_Win(circlearc.radius*2);
    if (circlearc.ang1.get_value() == circlearc.ang2.get_value() ) {
      a.angle1= 0;
      a.angle2= two_pi;
    }
    else {
      a.angle1= conv.angle_Plane_to_angle_Win(circlearc.ang1);
      a.angle2= conv.angle_Plane_to_angle_Win(ang);
    }
  }

  void copy_2_XPoints(XPoint * dest, int num,const Point2d * vec) const;
  void copy_2_XPoints(XPoint * dest, int num,const Point2d * vec, const Point2d&) const;
  void copy_2_XSegments(XSegment * dest, int num,const Line2d * vec) const;
  void copy_2_XArcs(XArc * dest, int num,const Circle2d * vec) const;
  void copy_2_XArcs(XArc * dest, int num,const CircleArc2d * vec) const;
};

#endif
