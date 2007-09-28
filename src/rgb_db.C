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

#include "rgb_db.h"
#include <iostream>
#include <cerrno>
Display * RGB_DB::disp=0;

RGBcolor RGB_DB::XNamedColor_to_RGBcolor(const char * colstr) {
  XColor screen;
  XColor exact;
  if (!XAllocNamedColor(disp, DefaultColormap (disp, 0),colstr, &screen, &exact)) {
    std::cout << "\ncolor allocation failed, using black";
    if (errno == BadColor) 
      std::cout << "(color " << colstr << " not in the rgb database";
    return RGBcolor(0,0,0);
  }
  return RGBcolor(exact.red/257,exact.green/257,exact.blue/257);
}
