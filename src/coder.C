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

#include "coder.h"

const int EnDeCoderBin::cmd_insert_frame       = 1;

const int EnDeCoderBin::cmd_insert_point       = 2;
const int EnDeCoderBin::cmd_insert_points      = 3;

const int EnDeCoderBin::cmd_insert_line        = 4;
const int EnDeCoderBin::cmd_insert_lines       = 5;

const int EnDeCoderBin::cmd_insert_circle      = 6;
const int EnDeCoderBin::cmd_insert_circles     = 7;
const int EnDeCoderBin::cmd_insert_f_circle    = 8; //f means filled
const int EnDeCoderBin::cmd_insert_f_circles   = 9; //f means filled

const int EnDeCoderBin::cmd_insert_circlearc   = 10;
const int EnDeCoderBin::cmd_insert_circlearcs  = 11;
const int EnDeCoderBin::cmd_insert_f_circlearc = 12;
const int EnDeCoderBin::cmd_insert_f_circlearcs= 13;

const int EnDeCoderBin::cmd_insert_polyline    = 14;

const int EnDeCoderBin::cmd_insert_polygon     = 15;
const int EnDeCoderBin::cmd_insert_f_polygon   = 16;

const int EnDeCoderBin::cmd_insert_string      = 17;


const int EnDeCoderBin::cmd_set_object_visible = 18;
const int EnDeCoderBin::cmd_set_object_layer   = 19;
const int EnDeCoderBin::cmd_set_object_color   = 20;

const int EnDeCoderBin::cmd_set_frame_visible  = 21;
const int EnDeCoderBin::cmd_set_frame_layer    = 22;
const int EnDeCoderBin::cmd_set_frame_pos      = 23;
const int EnDeCoderBin::cmd_set_frame_ang      = 24;
const int EnDeCoderBin::cmd_set_frame_pos_ang  = 25;

const int EnDeCoderBin::cmd_remove_frame       = 26;
const int EnDeCoderBin::cmd_remove_object      = 27;

const int EnDeCoderBin::cmd_empty_frame        = 28;



bool EnDeCoderBin::build_from_binary(BuilderBase * build, std::istream & in) {
  bool res=false;
  int cmd;
  int i1,i2,i3;
  Point2d point;
  Line2d line;
  Circle2d circle;
  CircleArc2d circlearc;
  Multi<Point2d> m_point;
  Multi<Line2d> m_line;
  Multi<Circle2d> m_circle;
  Multi<CircleArc2d> m_circlearc;
  Multi<char> m_char;
  Angle ang1;
  int layer;
  RGBcolor col;

  while ( get_cmd_type(in,cmd) ) {
    //cout << "\n>>true recv cmd= " << cmd << endl;
    res= false;
    switch (cmd) {
    case EnDeCoderBin::cmd_insert_frame       :
      if (get_cmd_insert_frame(in,i1,i2,point,ang1,layer))
	 res= build->set_cmd_insert_frame(i1,i2,point,ang1,layer);
      break;
    case EnDeCoderBin::cmd_insert_point       :
      if (get_cmd_insert_point(in,i1,i2,point,layer,col))
	res= build->set_cmd_insert_point(i1,i2,point,layer,col);
      break;
    case EnDeCoderBin::cmd_insert_points       :
      if (get_cmd_insert_points(in,i1,i2,m_point,layer,col))
	res= build->set_cmd_insert_points(i1,i2,m_point.cur_size,m_point.tab,layer,col);
      break;
    case EnDeCoderBin::cmd_insert_line        :
      if (get_cmd_insert_line(in,i1,i2,line,layer,col))
	res= build->set_cmd_insert_line(i1,i2,line,layer,col);
      break;
    case EnDeCoderBin::cmd_insert_lines        :
      if (get_cmd_insert_lines(in,i1,i2,m_line,layer,col))
	res= build->set_cmd_insert_lines(i1,i2,m_line.cur_size,m_line.tab,layer,col);
      break;
    case EnDeCoderBin::cmd_insert_circle      :
      if (get_cmd_insert_circle(in,i1,i2,circle,layer,col))
	res= build->set_cmd_insert_circle(i1,i2,circle,layer,col);
      break;
    case EnDeCoderBin::cmd_insert_circles      :
      if (get_cmd_insert_circles(in,i1,i2,m_circle,layer,col))
	res= build->set_cmd_insert_circles(i1,i2,m_circle.cur_size,m_circle.tab,layer,col);
      break;
    case EnDeCoderBin::cmd_insert_f_circle      :
      if (get_cmd_insert_f_circle(in,i1,i2,circle,layer,col))
	res= build->set_cmd_insert_f_circle(i1,i2,circle,layer,col);
      break;
    case EnDeCoderBin::cmd_insert_f_circles      :
      if (get_cmd_insert_f_circles(in,i1,i2,m_circle,layer,col))
	res= build->set_cmd_insert_f_circles(i1,i2,m_circle.cur_size,m_circle.tab,layer,col);
      break;
      ////
    case EnDeCoderBin::cmd_insert_circlearc      :
      if (get_cmd_insert_circlearc(in,i1,i2,circlearc,layer,col))
	res= build->set_cmd_insert_circlearc(i1,i2,circlearc,layer,col);
      break;
    case EnDeCoderBin::cmd_insert_circlearcs      :
      if (get_cmd_insert_circlearcs(in,i1,i2,m_circlearc,layer,col))
	res= build->set_cmd_insert_circlearcs(i1,i2,m_circlearc.cur_size,m_circlearc.tab,layer,col);
      break;
    case EnDeCoderBin::cmd_insert_f_circlearc      :
      if (get_cmd_insert_f_circlearc(in,i1,i2,circlearc,layer,col))
	res= build->set_cmd_insert_f_circlearc(i1,i2,circlearc,layer,col);
      break;
    case EnDeCoderBin::cmd_insert_f_circlearcs      :
      if (get_cmd_insert_f_circlearcs(in,i1,i2,m_circlearc,layer,col))
	res= build->set_cmd_insert_f_circlearcs(i1,i2,m_circlearc.cur_size,m_circlearc.tab,layer,col);
      break;
    case EnDeCoderBin::cmd_insert_polyline     :
      if (get_cmd_insert_polyline(in,i1,i2,m_point,layer,col))
	res= build->set_cmd_insert_polyline(i1,i2,m_point.cur_size,m_point.tab,layer,col);
      break;
    case EnDeCoderBin::cmd_insert_polygon     :
      if (get_cmd_insert_polygon(in,i1,i2,m_point,layer,col))
	res= build->set_cmd_insert_polygon(i1,i2,m_point.cur_size,m_point.tab,layer,col);
      break;
    case EnDeCoderBin::cmd_insert_f_polygon     :
      if (get_cmd_insert_f_polygon(in,i1,i2,m_point,layer,col))
	res= build->set_cmd_insert_f_polygon(i1,i2,m_point.cur_size,m_point.tab,layer,col);
      break;
    case EnDeCoderBin::cmd_insert_string      :
      if (get_cmd_insert_string(in,i1,i2,point,m_char,layer,col))
	res= build->set_cmd_insert_string(i1,i2,point,m_char.cur_size,m_char.tab,layer,col);
      break;
    case  EnDeCoderBin::cmd_set_object_visible :
      if (get_cmd_set_object_visible(in,i1,i2,i3))
	res= build->set_cmd_set_object_visible(i1,i2,i3);
      break;
    case  EnDeCoderBin::cmd_set_object_layer :
      if (get_cmd_set_object_layer(in,i1,i2,i3))
	res= build->set_cmd_set_object_layer(i1,i2,i3);
      break;
    case  EnDeCoderBin::cmd_set_object_color :
      if (get_cmd_set_object_color(in,i1,i2,col))
	res= build->set_cmd_set_object_color(i1,i2,col);
      break;
    case  EnDeCoderBin::cmd_set_frame_visible :
      if (get_cmd_set_frame_visible(in,i1,i2))
	res= build->set_cmd_set_frame_visible(i1,i2);
      break;
    case  EnDeCoderBin::cmd_set_frame_layer :
      if (get_cmd_set_frame_layer(in,i1,i2))
	res= build->set_cmd_set_frame_layer(i1,i2);
      break;
    case EnDeCoderBin::cmd_set_frame_pos      :
      if (get_cmd_set_frame_pos(in,i1,point))
	res= build->set_cmd_set_frame_pos(i1,point);
      break;
    case EnDeCoderBin::cmd_set_frame_ang      :
      if (get_cmd_set_frame_ang(in,i1,ang1))
	res= build->set_cmd_set_frame_ang(i1,ang1);
      break;
    case EnDeCoderBin::cmd_set_frame_pos_ang  :
      if (get_cmd_set_frame_pos_ang(in,i1,point,ang1))
	res= build->set_cmd_set_frame_pos_ang(i1,point,ang1);
      break;
    case EnDeCoderBin::cmd_remove_frame  :
      if (get_cmd_remove_frame(in,i1))
	res= build->set_cmd_remove_frame(i1);
      break;
    case EnDeCoderBin::cmd_remove_object :
      if (get_cmd_remove_object(in,i1,i2))
	res= build->set_cmd_remove_object(i1,i2);
      break;
    case EnDeCoderBin::cmd_empty_frame  :
      if (get_cmd_empty_frame(in,i1))
	res= build->set_cmd_empty_frame(i1);
      break;
    default:
      std::cerr << "\ncmd not recognized";
      return false;
    }
  }
  //cout << "\nexecute_cmd returns " << res << endl;
  return res;
}
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/*************      T E S T     ***************************************/

#if 0

#include <iostream>

main() {
  ofstream o("ccc.dat");
  Multi<Point2d> points;
  points.set_max_size(3);
  points.tab[0]= Point2d(1,1);
  points.tab[1]= Point2d(2,2);
  points.tab[2]= Point2d(3,3);
  points.set_cur_size(3);

  CircleArc2d c(Point2d(1.0,1.0),2.0,1.0,2.0);

  EnDeCoderBin enc;
  //enc.set_cmd_insert_points(o,0,0,points,0,RGBcolor(0,0,0));
  //enc.set_cmd_insert_circlearc(o,0,0,c,0,RGBcolor(0,0,0));
  Angle a(2.0);
  Angle a2;
  o << a;
  o.close();

  ifstream i("ccc.dat");
  i >> a2;
  std::cout << "\na2= " << a2.get_value();
  return 0;
  int cmd;
  enc.get_cmd_type(i,cmd);
  int i1,i2,i3;
  RGBcolor col;
  Multi<Point2d> points2;
  CircleArc2d c2;

  std::cout << "\nresult= " << enc.get_cmd_insert_circlearc(i,i1,i2,c2,i3,col);

  std::cout << "\n[ ";
  std::cout << "\n" << c2.center.x << " " << c2.center.y << " "
       << c2.radius << " " << c2.ang1.get_value() << " "
       << c2.ang2.get_value();
  std::cout << std::endl;
  return 0;
  for (int k= 0; k< points2.cur_size; k++)
    std::cout << "(" << points2.tab[k].x << "," << points2.tab[k].y << ") ";
}


#endif
