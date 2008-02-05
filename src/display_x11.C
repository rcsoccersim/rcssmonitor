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

#include "display_x11.h"
#include <iostream>

void DisplayX11::init_area_number() {
  area_number= 1;
}

void DisplayX11::step_area_number() {
  area_number++;
  if (area_number==0) //skip 0 after rewinding
    init_area_number();
}

DisplayX11::DisplayX11(Display* d,Pixmap p,GC g) : d_xpoints(32), d_xsegments(32), d_xarcs(32) {
   disp= d;
   pixmap= p;
   gc= g;
   init_area_number();
}

void DisplayX11::ASetPlainArea(const Area2d & a) {
  step_area_number();
  conv.init_plane( a.center, a.size_x, a.size_y);
}

void DisplayX11::ASetWindowArea(int width, int height) {
  step_area_number();
  conv.init_win(width,height);
}

unsigned long DisplayX11::AGetColor(const RGBcolor & col) {
  XColor xcol;
  xcol.red= col.get_red();
  xcol.green= col.get_green();
  xcol.blue= col.get_blue();
  if (!XAllocColor(disp, DefaultColormap(disp, 0), &xcol)) {
    std::cerr << "\nwarning: problem with allocating color:"
              << "\n(red,green,blue)= (" << int(col.red) << "," << int(col.green) << "," << int(col.blue) << ")"
              << "\n(Red,Green,Blue)= (" << col.get_red() << "," << col.get_green() << ","<< col.get_blue() << ")"
              << "\nresulting in XColor.pixel= " << xcol.pixel
              << "\n(XRed,XGreen,XBlue)= (" << xcol.red << "," << xcol.green << ","<< xcol.blue << ")";
  }
  return xcol.pixel;
}

#if 0
void DisplayX11::ASetForeground(char* color) {
  XColor def, closest ;
  XAllocNamedColor(disp, DefaultColormap(disp, 0),color, &def, &closest) ;
  XSetForeground(disp, gc, closest.pixel) ;
}

void DisplayX11::ASetBackground(char* color) {
  XColor def, closest ;
  XAllocNamedColor(disp, DefaultColormap(disp, 0),color, &def, &closest) ;
  XSetBackground(disp, gc, closest.pixel) ;
}

void DisplayX11::AFillRectangle(double a1,double a2,double w,double h) {
  XFillRectangle(disp, pixmap, gc,
		 conv.x_Plane_to_x_Win(a1),
		 conv.y_Plane_to_y_Win(a2),
		 conv.size_x_Plane_to_size_x_Win(w),
		 conv.size_y_Plane_to_size_y_Win(h));
}
#endif

void DisplayX11::draw_point(DisplayObject * d_obj, const RGBcolor & col, const Point2d & point) {
  D_XPoint *myobj;

  if (!d_obj) {
    myobj= &d_xpoint;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XPoint*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XPoint(myobj->p,point);
  }

  ASetForeground(myobj->XColor_pixel);
  XDrawPoint(disp,pixmap,gc, myobj->p.x, myobj->p.y );
}

void DisplayX11::draw_points(DisplayObject * d_obj, const RGBcolor & col, int num, const Point2d * vec){
  D_XPoints *myobj;

  if (!d_obj) {
    if (d_xpoints.pp.max_size < num)
      d_xpoints.pp.set_max_size(num);
    d_xpoints.pp.set_cur_size(num);
    myobj= &d_xpoints;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XPoints*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?


  //here we have the assumption num= myobj->pp.max_size; i.e. the it is not allowed to change the number of points

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XPoints(myobj->pp.tab, num, vec);
  }

  ASetForeground(myobj->XColor_pixel);
  XDrawPoints(disp,pixmap,gc,myobj->pp.tab,num,CoordModeOrigin);
}

void DisplayX11::draw_line(DisplayObject * d_obj, const RGBcolor & col, const Line2d & line){
  D_XSegment *myobj;

  if (!d_obj) {
    myobj= &d_xsegment;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XSegment*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XSegment(myobj->s,line);
  }

  ASetForeground(myobj->XColor_pixel);
  XDrawLine(disp,pixmap,gc, myobj->s.x1, myobj->s.y1, myobj->s.x2, myobj->s.y2);
}

void DisplayX11::draw_lines(DisplayObject * d_obj, const RGBcolor & col, int num, const Line2d * vec){
  D_XSegments *myobj;

  if (!d_obj) {
    if (d_xsegments.ss.max_size < num)
      d_xsegments.ss.set_max_size(num);
    d_xsegments.ss.set_cur_size(num);
    myobj= &d_xsegments;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XSegments*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?

  //here we have the assumption num= myobj->ss.max_size; i.e. the it is not allowed to change the number of points

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XSegments(myobj->ss.tab, num, vec);
  }

  ASetForeground(myobj->XColor_pixel);
  XDrawSegments(disp,pixmap,gc,myobj->ss.tab,num);
}

void DisplayX11::draw_circle(DisplayObject * d_obj, const RGBcolor & col, const Circle2d & circle) {
  D_XArc *myobj;

  if (!d_obj) {
    myobj= &d_xarc;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XArc*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XArc(myobj->a,circle);
  }

  ASetForeground(myobj->XColor_pixel);
  XDrawArc(disp,pixmap,gc,
           myobj->a.x,
           myobj->a.y,
           myobj->a.width,
           myobj->a.height,
           myobj->a.angle1,myobj->a.angle2);
}

void DisplayX11::draw_circles(DisplayObject * d_obj, const RGBcolor & col, int num, const Circle2d * vec){
  D_XArcs *myobj;

  if (!d_obj) {
    if (d_xarcs.aa.max_size < num)
      d_xarcs.aa.set_max_size(num);
    d_xarcs.aa.set_cur_size(num);
    myobj= &d_xarcs;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XArcs*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?

  //here we have the assumption num= myobj->pp.max_size; i.e. the it is not allowed to change the number of arcs

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XArcs(myobj->aa.tab, num, vec);
  }

  ASetForeground(myobj->XColor_pixel);
  XDrawArcs(disp,pixmap,gc,myobj->aa.tab,num);
}

void DisplayX11::fill_circle(DisplayObject * d_obj, const RGBcolor & col, const Circle2d & circlearc){
  D_XArc *myobj;

  if (!d_obj) {
    myobj= &d_xarc;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XArc*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XArc(myobj->a,circlearc);
  }

  ASetForeground(myobj->XColor_pixel);
  XFillArc(disp,pixmap,gc,
           myobj->a.x,
           myobj->a.y,
           myobj->a.width,
           myobj->a.height,
           myobj->a.angle1,myobj->a.angle2);
}

void DisplayX11::fill_circles(DisplayObject * d_obj, const RGBcolor & col, int num, const Circle2d * vec){
  D_XArcs *myobj;

  if (!d_obj) {
    if (d_xarcs.aa.max_size < num)
      d_xarcs.aa.set_max_size(num);
    d_xarcs.aa.set_cur_size(num);
    myobj= &d_xarcs;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XArcs*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?

  //here we have the assumption num= myobj->pp.max_size; i.e. the it is not allowed to change the number of arcs

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XArcs(myobj->aa.tab, num, vec);
  }

  ASetForeground(myobj->XColor_pixel);
  XFillArcs(disp,pixmap,gc,myobj->aa.tab,num);
}

  //specifies an arc with absolute begin in ang1 and absolute!!! end in ang2, the path goes from ang1 to ang2 couterclockwise
void DisplayX11::draw_circlearc(DisplayObject * d_obj, const RGBcolor & col, const CircleArc2d & circlearc){
  D_XArc *myobj;

  if (!d_obj) {
    myobj= &d_xarc;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XArc*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XArc(myobj->a,circlearc);
  }

  ASetForeground(myobj->XColor_pixel);
  XDrawArc(disp,pixmap,gc,
           myobj->a.x,
           myobj->a.y,
           myobj->a.width,
           myobj->a.height,
           myobj->a.angle1,myobj->a.angle2);
}

void DisplayX11::draw_circlearcs(DisplayObject * d_obj, const RGBcolor & col, int num, const CircleArc2d * vec){
  D_XArcs *myobj;

  if (!d_obj) {
    if (d_xarcs.aa.max_size < num)
      d_xarcs.aa.set_max_size(num);
    d_xarcs.aa.set_cur_size(num);
    myobj= &d_xarcs;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XArcs*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?

  //here we have the assumption num= myobj->pp.max_size; i.e. the it is not allowed to change the number of arcs

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XArcs(myobj->aa.tab, num, vec);
  }

  ASetForeground(myobj->XColor_pixel);
  XDrawArcs(disp,pixmap,gc,myobj->aa.tab,num);
}

void DisplayX11::fill_circlearc(DisplayObject * d_obj, const RGBcolor & col, const CircleArc2d & circlearc){
  D_XArc *myobj;

  if (!d_obj) {
    myobj= &d_xarc;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XArc*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XArc(myobj->a,circlearc);
  }

  ASetForeground(myobj->XColor_pixel);
  XFillArc(disp,pixmap,gc,
           myobj->a.x,
           myobj->a.y,
           myobj->a.width,
           myobj->a.height,
           myobj->a.angle1,myobj->a.angle2);
}

void DisplayX11::fill_circlearcs(DisplayObject * d_obj, const RGBcolor & col, int num, const CircleArc2d * vec){
  D_XArcs *myobj;

  if (!d_obj) {
    if (d_xarcs.aa.max_size < num)
      d_xarcs.aa.set_max_size(num);
    d_xarcs.aa.set_cur_size(num);
    myobj= &d_xarcs;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XArcs*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?

  //here we have the assumption num= myobj->pp.max_size; i.e. the it is not allowed to change the number of arcs

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XArcs(myobj->aa.tab, num, vec);
  }

  ASetForeground(myobj->XColor_pixel);
  XFillArcs(disp,pixmap,gc,myobj->aa.tab,num);
}

//all boudary points of the polygon are treated as absolute coordinates
void DisplayX11::draw_polyline(DisplayObject * d_obj, const RGBcolor & col, int num, const Point2d * vec){
  D_XPoints *myobj;

  if (!d_obj) {
    if (d_xpoints.pp.max_size < num)
      d_xpoints.pp.set_max_size(num);
    d_xpoints.pp.set_cur_size(num);
    myobj= &d_xpoints;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XPoints*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?

  //here we have the assumption num= myobj->pp.max_size; i.e. the it is not allowed to change the number of points

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XPoints(myobj->pp.tab, num, vec);
  }

  ASetForeground(myobj->XColor_pixel);
  XDrawLines(disp,pixmap,gc,myobj->pp.tab,num,CoordModeOrigin);
}

void DisplayX11::draw_polygon(DisplayObject * d_obj, const RGBcolor & col, int num, const Point2d * vec) {
  D_XPoints *myobj;

  if (!d_obj) {
    if (d_xpoints.pp.max_size < num+1)
      d_xpoints.pp.set_max_size(num+1);
    d_xpoints.pp.set_cur_size(num+1);
    myobj= &d_xpoints;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XPoints*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?

  //here we have the assumption num= myobj->pp.max_size; i.e. the it is not allowed to change the number of points

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XPoints(myobj->pp.tab, num, vec, vec[0]);
  }

  ASetForeground(myobj->XColor_pixel);
  XDrawLines(disp,pixmap,gc,myobj->pp.tab,num+1,CoordModeOrigin);
}

void DisplayX11::fill_polygon(DisplayObject * d_obj, const RGBcolor & col, int num, const Point2d * vec){
  D_XPoints *myobj;

  if (!d_obj) {
    if (d_xpoints.pp.max_size < num)
      d_xpoints.pp.set_max_size(num);
    d_xpoints.pp.set_cur_size(num);
    myobj= &d_xpoints;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XPoints*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?

  //here we have the assumption num= myobj->pp.max_size; i.e. the it is not allowed to change the number of points

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XPoints(myobj->pp.tab, num, vec);
  }

  ASetForeground(myobj->XColor_pixel);
  XFillPolygon(disp,pixmap,gc,myobj->pp.tab,num,Complex,CoordModeOrigin);
}

void DisplayX11::draw_string( DisplayObject * d_obj,
                              const RGBcolor & col,
                              const Point2d & pos,
                              int len,
                              const char * str )
{
  D_XPoint *myobj;

  if (!d_obj) {
    myobj= &d_xpoint;
    myobj->mark_col_change();
    myobj->mark_pos_change();
  }
  else
    myobj= static_cast<D_XPoint*>(d_obj); //dynamic_cast should also work, but it doesn't <- why?

  if (myobj->col_change) {
    myobj->col_change= false;
    myobj->XColor_pixel= AGetColor(col);
  }

  if (myobj->area_number != area_number) {
    myobj->area_number= area_number;
    copy_2_XPoint(myobj->p,pos);
  }

  ASetForeground(myobj->XColor_pixel);
  XDrawPoint(disp,pixmap,gc, myobj->p.x, myobj->p.y );
  XDrawString(disp,pixmap,gc,
              myobj->p.x,
              myobj->p.y,
              str,
              len);
}

void DisplayX11::set_background_color(const RGBcolor & col) {
  XSetBackground(disp, gc, AGetColor(col) );
}

void DisplayX11::copy_2_XPoints(XPoint * dest,int num,const Point2d * vec) const  {
  for (int i= 0; i<num; i++)
    copy_2_XPoint(dest[i],vec[i]);
}

void DisplayX11::copy_2_XPoints(XPoint * dest,int num,const Point2d * vec, const Point2d & point) const  {
  for (int i= 0; i<num; i++)
    copy_2_XPoint(dest[i],vec[i]);

  copy_2_XPoint(dest[num],point);
}

void DisplayX11::copy_2_XSegments(XSegment * dest,int num, const Line2d * vec) const  {
  for (int i= 0; i<num; i++)
    copy_2_XSegment(dest[i],vec[i]);
}

void DisplayX11::copy_2_XArcs(XArc * dest,int num, const Circle2d * vec) const  {
  for (int i= 0; i<num; i++)
    copy_2_XArc(dest[i],vec[i]);
}

void DisplayX11::copy_2_XArcs(XArc * dest,int num,const CircleArc2d * vec) const  {
  for (int i= 0; i<num; i++)
    copy_2_XArc(dest[i],vec[i]);
}
