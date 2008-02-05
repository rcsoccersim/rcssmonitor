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

#include <iostream>

/**
   This is the main class to represent colors. I uses the RGB representation
   with values in the range [0,...,255]
*/
class RGBcolor {
private:
    /// map from [0..255] -> [0..65535] or [0..2^8-1] -> [0..2^16-1]
    unsigned short uchar_to_ushort( unsigned char c ) const
      {
          return static_cast< unsigned short >( c )
              * static_cast< unsigned short >( 257 );
      };
public:

    unsigned char red, green, blue;

    RGBcolor()
        : red( 0 ),
          green( 0 ),
          blue( 0 )
      { }

    RGBcolor( unsigned char r,
              unsigned char g,
              unsigned char b )
        : red( r ),
          green( g ),
          blue( b )
      { }

    unsigned short get_red() const   { return uchar_to_ushort( red ); }
    unsigned short get_green() const { return uchar_to_ushort( green ); }
    unsigned short get_blue() const  { return uchar_to_ushort( blue ); }
    //bool operator!=(const RGBcolor & col) const;
    //bool operator==(const RGBcolor & col) const { return !(this->operator!=(col));}
};

inline
std::ostream &
operator<<( std::ostream & o,
            const RGBcolor & col )
{
    const char numbers[16]= {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    o << numbers[col.red/16] << numbers[col.red%16]
      << numbers[col.green/16] <<numbers[ col.green%16]
      << numbers[col.blue/16] << numbers[col.blue%16];
    return o;
}


inline
bool
operator!=( const RGBcolor & lhs,
            const RGBcolor & rhs )
{
    return ( lhs.red != rhs.red
             || lhs.green != rhs.green
             || lhs.blue != rhs.blue );
}

inline
bool
operator==( const RGBcolor & lhs,
            const RGBcolor & rhs )
{
    return ! ( lhs != rhs );
}


#endif
