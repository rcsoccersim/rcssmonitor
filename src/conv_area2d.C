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

#include "conv_area2d.h"

//#define DEBUG(XXX) XXX
#define DEBUG(XXX) 

ConvArea2d::ConvArea2d() {
  set_win_size(100,50); 
  area= Area2d( Point2d(0,0), 100.0, 50.0);

  unset_min_size_x();
  unset_min_size_y();
  unset_max_size_x();
  unset_max_size_y();
			 
  unset_min_pos_x(); 
  unset_min_pos_y(); 
  			 
  unset_max_pos_x(); 
  unset_max_pos_y(); 
}

bool ConvArea2d::update() {
  DEBUG(cout << "\n--\nbefore update:"; show_area();)
  if (level == 0) {
    DEBUG(cout << "\nNO UPDATE NEEDED" << flush;)
    return false;
  }
  bool changed= false;
  if (level >= 4) { 
    /* The middle point of A keeps the same. Enlarge A.size_x XOR A.size_y so that

       win_size_x     A.size_x                      win_size_x     win_size_y
       ----------  =  --------    or equivalently   ----------  =  ----------
       win_size_y     A.size_y			     A.size_x       A.size_y

     */
    double ratio_x= area.size_x / double(win_size_x);
    double ratio_y= area.size_y / double(win_size_y);
    if (ratio_x > ratio_y) {
      area.size_y= win_size_y * ratio_x;
      changed= true;
    }
    else {
      area.size_x= win_size_x * ratio_y;
      changed= true;
    }
    DEBUG(cout << "\n >> after level 4:"; show_area();)
  }
  //---------------------------------------------------------------------------
  if (level >= 3) {
    /* The middle point of A keeps the same.
       1) Enlarge A.size_x and A.size_y (keeping the proportions) until 
          A.size_x = min_size_x;
       2) Enlarge A.size_y and A.size_x (keeping the proportions) until 
          A.size_y = min_size_y;
     */
    if (use_min_size_x && area.size_x < min_size_x) {
      area.size_y *= min_size_x / area.size_x; //the term on the right side is > 1
      area.size_x= min_size_x;
      changed= true;
    }
    if (use_min_size_y && area.size_y < min_size_y) {
      area.size_x *= min_size_y / area.size_y; //the term on the right side is > 1
      area.size_y= min_size_y;
      changed= true;
    }
    DEBUG(cout << "\n >> after level 3:"; show_area();)
  }
  //---------------------------------------------------------------------------
  if (level >= 2) {
    /* The middle point of A keeps the same.
       1) Shrink A.size_x and A.size_y (keeping the proportions) until 
          A.size_x = min_size_x;
       2) Shrink A.size_y and A.size_x (keeping the proportions) until 
          A.size_y = min_size_y;
     */
    if (use_max_size_x && area.size_x > max_size_x) {
      area.size_y *= max_size_x / area.size_x; //the term on the right side is < 1
      area.size_x= max_size_x;
      changed= true;
    }
    if (use_max_size_y && area.size_y > max_size_y) {
      area.size_x *= max_size_y / area.size_y; //the term on the right side is < 1
      area.size_y= max_size_y;
      changed= true;
    }
    DEBUG(cout << "\n >> after level 2:"; show_area();)
  }
  //---------------------------------------------------------------------------
  if (level >= 1) {
    /* The middle point of A can change. Also the size of A can change.
     */
    bool x_already_moved= false;
    bool y_already_moved= false;
    //changes for the x dimension first
    if (use_max_pos_x && use_min_pos_x) {
      double s_x= max_pos_x - min_pos_x;
      if (area.size_x > s_x) { //resize is needed
	area.size_y *= s_x / area.size_x; //the term on the right side is < 1
	area.size_x= s_x;
	area.center.x= min_pos_x + 0.5 * s_x;
	x_already_moved= true;
	changed= true;
      }
    }
    if (use_max_pos_x && !x_already_moved) { 
      //just put A to the left until A is to the left of max_pos_x (no resize is needed)
      if ( area.center.x + 0.5* area.size_x > max_pos_x ) {
	area.center.x= max_pos_x - 0.5 * area.size_x;
	x_already_moved= true;
	changed= true;
      }
    }
    if (use_min_pos_x && !x_already_moved) {
      //just put A to the right until A is to the right of min_pos_x (no resize is needed)
      if ( area.center.x - 0.5* area.size_x < min_pos_x ) {
	area.center.x= min_pos_x + 0.5 * area.size_x;
	x_already_moved= true;
	changed= true;
      }
    }
    DEBUG(cout << "\n >> after level 1x:"; show_area();)
    //now analogously the changes for the y dimension
    if (use_max_pos_y && use_min_pos_y) {
      double s_y= max_pos_y - min_pos_y;
      if (area.size_y > s_y) { //resize is needed
	area.size_y *= s_y / area.size_y; //the term on the right side is < 1
	area.size_y= s_y;
	area.center.y= min_pos_y + 0.5 * s_y;
	y_already_moved= true;
	changed= true;
      }
    }
    if (use_max_pos_y && !y_already_moved) { 
      //just put A downwards until A is under max_pos_y (no resize is needed)
      if ( area.center.y + 0.5* area.size_y > max_pos_y ) {
	area.center.y= max_pos_y - 0.5 * area.size_y;
	y_already_moved= true;
	changed= true;
      }
    }
    if (use_min_pos_y && !y_already_moved) {
      //just put A upwards  until A is over min_pos_y (no resize is needed)
      if ( area.center.y - 0.5* area.size_y < min_pos_y ) {
	area.center.y= min_pos_y + 0.5 * area.size_y;
	y_already_moved= true;
	changed= true;
      }
    }
    DEBUG(cout << "\n >> after level 1y:"; show_area();)
  }
  //---------------------------------------------------------------------------
  DEBUG(cout << "\nafter update:"; show_area(); cout << flush;)
  level= 0;
  return changed;
}

void ConvArea2d::set_win_size(int sx, int sy) { 
  if ( win_size_x != sx || win_size_y != sy) {
    if ( win_size_x != 0 && win_size_y != 0 ) {
      area.size_x *= double(sx)/double(win_size_x);
      area.size_y *= double(sy)/double(win_size_y);
    }
    win_size_x= sx; 
    win_size_y= sy; 
    update_level(4); 
  }
}

void ConvArea2d::move_area_dir_x_using_factor(double factor) { 
  area.center.x+= factor*area.size_x; 
  update_level(1); 
}

void ConvArea2d::move_area_dir_y_using_factor(double factor) { 
  area.center.y+= factor*area.size_y; 
  update_level(1); 
}

void ConvArea2d::scale_area_using_factor(double factor) {
  area.size_x *= factor;
  area.size_y *= factor;
  if (factor>1)
    update_level(2);
  else
    update_level(3);
}

void ConvArea2d::change_area_using_subwin(int origin_x, int origin_y, int dest_x, int dest_y) {
  Point2d p1,p2;
  p1= get_point_from_win_pos(origin_x,origin_y);
  p2= get_point_from_win_pos(dest_x,dest_y);

  area.center.x= 0.5* (p1.x+p2.x);
  area.center.y= 0.5* (p1.y+p2.y);
  area.size_x= fabs(p2.x-p1.x);
  area.size_y= fabs(p2.y-p1.y);

  update_level(4);
}

void ConvArea2d::change_area_using_win_pos(int origin_x, int origin_y) {
  area.center= get_point_from_win_pos(origin_x,origin_y);
  update_level(1);
}

Point2d ConvArea2d::get_point_from_win_pos(int x, int y) const {
  Point2d pos;
  pos.x=  area.center.x - 0.5* area.size_x + (area.size_x* x)/win_size_x;
  pos.y=  area.center.y - 0.5* area.size_y + (area.size_y* (win_size_y-y))/win_size_y;
  return pos;
}

#if 0
int main() {
  ConvArea2d CA;
  CA.set_win_size(100,50);
  CA.set_area( Area2d( Point2d(0,0), 20.0,5.0) );

  CA.update();
  CA.set_max_size_x(10);
  CA.update();
  cout << endl;
  return 1;
};
#endif
