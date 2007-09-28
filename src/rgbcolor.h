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

#ifndef _RGBCOLOR_H_
#define _RGBCOLOR_H_

/**
   This is the main class to represent colors. I uses the RGB representation
   with values in the range [0,...,255]
 */
class RGBcolor {
 private:
  /// map from [0..255] -> [0..65535] or [0..2^8-1] -> [0..2^16-1]
  unsigned short uchar_to_ushort(unsigned char c) const { 
    return (unsigned short)(c)* (unsigned short)(257);
  };
 public:
  unsigned char red, green, blue;
  RGBcolor(unsigned char r,unsigned char g, unsigned char b) { red= r ; green= g; blue= b;}
  RGBcolor() { red= 0; green= 0; blue= 0; }
  unsigned short get_red() const   { return uchar_to_ushort(red); }
  unsigned short get_green() const { return uchar_to_ushort(green); }
  unsigned short get_blue() const  { return uchar_to_ushort(blue); }
  bool operator!=(const RGBcolor & col) const;
  bool operator==(const RGBcolor & col) const { return !(this->operator!=(col));}
};

#endif
