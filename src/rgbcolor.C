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

#include "rgbcolor.h"

// bool
// RGBcolor::operator!=( const RGBcolor & col ) const
// {
//     if ( red != col.red || green != col.green || blue != col.blue ) return true;
//     return false;
// }

#if 0
#include <iostream>
#include <iomanip>

main() {
    RGBcolor w(0,0,0);
    RGBcolor b(1,0,0);
    if (w != b) {
        cout << "\nw!=b";
    }
    else
        cout << "\nw==b";

    return 0;
    for (unsigned char i; i<128; i++) {
        cout.setf(ios::dec);
        cout << "\n " << i;
        cout.setf(ios::oct);
        cout << " "  << i;
        cout.setf(ios::hex);
        cout << " "  << i;
    }
    cout << "\nsizeof(unsigned short)= " << sizeof(unsigned short);
    cout << "\nsizeof(unsigned char)= " << sizeof(unsigned char);
    RGBcolor c(255,255,255);
    cout << "\nred= "   << c.red << "(" << c.get_red() << ")"
         << "  green= " << c.green << "(" << c.get_green() << ")"
         << "  blue= "  << c.blue << "(" << c.get_blue() << ")";

    unsigned char x= 255;
    unsigned int X = (unsigned int)(x);
    cout << "\n" << X ;
    cout << "\n" << (X*257) ;
    cout << "\n";
    return 1;
}
#endif
