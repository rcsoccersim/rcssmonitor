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
#include "menu_x11.h"
#include <iostream>
using namespace std;
#include "global_defs.h"
#include <cstring>

/******************************************************************************/
/******************************************************************************/

const int MenuX11::opener_size_x= 6;

MenuX11::MyWin::MyWin() {
  exists= false;
  redraw= true;
  visible= false;
  mapped= false;
}

void MenuX11::MyWin::destroy_window(Display * disp) {
  if (exists) {
    XDestroyWindow(disp, win);
    exists= false;
    mapped= false;
  }
}

void MenuX11::MyWin::create_window(Display * disp, Window parent, int x, int y, int size_x,int size_y) {
  XSetWindowAttributes attr;

  attr.background_pixel = bg_pixel;
  attr.border_pixel = fg_pixel;
  attr.backing_store = NotUseful;
  attr.event_mask = ButtonPressMask    |ExposureMask | ButtonReleaseMask;
    //    | EnterWindowMask | LeaveWindowMask;

  attr.bit_gravity = NorthWestGravity;
  attr.win_gravity = NorthWestGravity;
  attr.save_under = False;

  win = XCreateWindow(disp, parent, x, y, size_x, size_y,
		      1, 0, InputOutput, CopyFromParent,
		      CWBackPixel | CWBorderPixel | CWEventMask |
		      CWBitGravity | CWWinGravity | CWBackingStore |
		      CWSaveUnder, &attr);
  exists= true;
  //visible= false;
}

void MenuX11::MyWin::set_window_visible(Display * disp,bool flag) {
  if (!exists)
    return;

  visible= flag;
  if (flag) {
    if (!mapped) {
      XMapWindow(disp,win);
      mapped= true;
    }
  }
  else
    if (mapped) {
      XUnmapWindow(disp,win);
      mapped= false;
    }
}


/******************************************************************************/
int MenuX11::get_text_width(const char * str,int len) const {
  return len* text_max_width;
}

MenuX11::MenuX11(Display *d, Window pw, GC g, int x, int y) {
 label= NULL;
 disp= d;
 win= pw;
 gc= g;
 size_x= x;
 size_y= y;
 buttons_size_x= size_x - opener_size_x;
 num_buttons= 0;
 num_buttons_max= 0;
 do_exit= false;

 flag_needs_redraw= true;
#if 1
 gc_context= XGContextFromGC(gc);
 //XQueryTextExtents
 //XFontStruct * fontstruct= XQueryFont(disp, font);
 XFontStruct * fontstruct= XQueryFont(disp, gc_context);

 if (fontstruct) {
   text_max_width= fontstruct->max_bounds.width;
   text_max_ascent= fontstruct->max_bounds.ascent;
   text_max_descent= fontstruct->max_bounds.descent;

   //cout << "\n max font: width=" << text_max_width << " ascent=" << text_max_ascent << " descent=" << text_max_descent << flush;

   XFreeFontInfo(0,fontstruct,1);
   size_y= text_max_ascent+text_max_descent + 4;
 }
 else {
   text_max_width= 7;
   text_max_ascent= 12;
   text_max_descent= 3;
   size_y= 17;
   WARNING_OUT << "\nfontstruct= 0, setting default values " << flush;
 }
#endif
}

MenuX11::~MenuX11() {
  buttons.destroy_window(disp);
  opener.destroy_window(disp);
}

unsigned int MenuX11::get_color_pixel(const RGBcolor & col) const {
  XColor xcol;
  xcol.red= col.get_red();
  xcol.green= col.get_green();
  xcol.blue= col.get_blue();
  if (!XAllocColor(disp, DefaultColormap(disp, 0), &xcol)) {
    cerr << "\nwarning: problem with allocating color:"
         << "\n(red,green,blue)= (" << int(col.red) << "," << int(col.green) << "," << int(col.blue)
         << "\n(Red,Green,Blue)= (" << col.get_red() << "," << col.get_green() << ","<< col.get_blue() << ")"
         << "\nresulting in XColor.pixel= " << xcol.pixel
         << "\n(XRed,XGreen,XBlue)= (" << xcol.red << "," << xcol.green << ","<< xcol.blue << ")";
  }
  return xcol.pixel;
}



void MenuX11::buttons_redraw() {
  if (!buttons.visible)
    return;

  XClearWindow(disp,buttons.win);

  int x=0;

  int dum_xx= 0;
  for (int i=0; i< num_buttons; i++)
    dum_xx+= label[i].width_max;
  if (dum_xx >= buttons_size_x)
    dum_xx= 2;
  else
    dum_xx= (buttons_size_x -dum_xx)/(num_buttons);


  for (int i=0; i< num_buttons; i++) {
    if (label[i].str)
      XDrawString(disp,buttons.win,gc,x+dum_xx/2,size_y- text_max_descent-1,label[i].str,label[i].len);
    //x+= dum_x;
    x+= label[i].width_max + dum_xx;
    //cout << "x[" << i << "]= " << x;
    XDrawLine(disp,buttons.win,gc, x,0,x,size_y);
  }
}

void MenuX11::opener_redraw() {
  XPoint pp[3];

  if (!opener.visible)
    return;

  if (!buttons.visible) {
    pp[0].x= 0; pp[0].y= 0;
    pp[1].x= 0; pp[1].y= size_y;
    pp[2].x= opener_size_x; pp[2].y= size_y/2;
  }
  else {
    pp[0].x= opener_size_x; pp[0].y= 0;
    pp[1].x= opener_size_x; pp[1].y= size_y;
    pp[2].x= 0; pp[2].y= size_y/2;
  }
  XClearWindow(disp,opener.win);
  XFillPolygon(disp,opener.win,gc,pp,3,Complex,CoordModeOrigin);
}

bool MenuX11::responsible_for_event(const XEvent & event) const {
  if (buttons.exists && buttons.visible && event.xany.window == buttons.win )
    return true;

  if (opener.exists && opener.visible && event.xany.window == opener.win )
    return true;

  return false;
}

bool MenuX11::process_event(const XEvent & event) {
  if (buttons.exists && buttons.visible && event.xany.window == buttons.win )
    switch (event.type) {
    case Expose:
      //cout << "\nexpose me";
      buttons.redraw= true;
      break;
    case ButtonPress:
      break;
    }

  bool buttons_window_disappears= false;
  if (opener.exists && opener.visible && event.xany.window == opener.win )
    switch (event.type) {
    case Expose:
      //cout << "\nexpose me";
      opener.redraw= true;
      break;
    case ButtonPress:
      if (!buttons.exists)
	buttons.create_window(disp,win, opener_size_x+1, 0, buttons_size_x, size_y);

      if (buttons.visible) {
	buttons.set_window_visible(disp,false);
	buttons_window_disappears= true;
      }
      else
	buttons.set_window_visible(disp,true);

      opener.redraw= true;
      //cerr << "\n buttons.visible= " << buttons.visible;
      break;
    }

  bool redrawn= false;
  if (buttons.redraw) {
    buttons_redraw();
    buttons.redraw= false;
    redrawn= true;
  }

  if (opener.redraw) {
    opener_redraw();
    opener.redraw= false;
    redrawn= true;
  }

  if (redrawn)
    XFlush(disp);

  return buttons_window_disappears;
}

bool MenuX11::button_pressed(const XEvent & event, int & button, int & mouse_button) const {
  if (!buttons.exists || !buttons.visible || event.xany.window != buttons.win )
    return false;

  if (event.type == ButtonPress) {
    //button= event.xbutton.x/(size_x / num_buttons);
    int x=0;
    int res_button= -1;

    int dum_xx= 0;
    for (int i=0; i< num_buttons; i++)
      dum_xx+= label[i].width_max;
    if (dum_xx >= buttons_size_x)
      dum_xx= 2;
    else
      dum_xx= (buttons_size_x -dum_xx)/(num_buttons);

    for (int i=0; i< num_buttons; i++) {
      x+= label[i].width_max + dum_xx;
      if (x> event.xbutton.x) {
	res_button= i;
	break;
      }
    }
    if (res_button>= 0) {
      button= res_button;
      mouse_button= event.xbutton.button;
      return true;
    }
    //cout << "\nyou pressed button num= " << button;
  }
  return false;
}

void MenuX11::redraw() {
  buttons_redraw();
  opener_redraw();
  //cout << "\nnow should redraw" << flush;
  XFlush(disp);
  flag_needs_redraw= false;
}

void MenuX11::set_background_color(const RGBcolor & col) {
  unsigned int pixel= get_color_pixel(col);

  if (buttons.bg_pixel !=  pixel) {
    buttons.bg_color= col;
    buttons.bg_pixel= pixel;
    if (buttons.exists)
      XSetWindowBackground(disp,buttons.win,buttons.bg_pixel);
  }

  if (opener.bg_pixel !=  pixel) {
    opener.bg_color= col;
    opener.bg_pixel= pixel;
    if (opener.exists)
      XSetWindowBackground(disp,opener.win,opener.bg_pixel);
  }
}

void MenuX11::set_foreground_color(const RGBcolor & col) {
  unsigned int pixel= get_color_pixel(col);
  buttons.fg_color= col;
  opener.fg_color= col;
  buttons.fg_pixel= pixel;
  opener.fg_pixel= pixel;
  XSetForeground(disp, gc, pixel) ;
}

void MenuX11::resize(int s_x,int s_y) {
  if ( s_x == size_x && s_y == size_y)
    return;

  size_x= s_x;
  buttons_size_x= size_x - opener_size_x;

  if ( s_y != size_y ) {
    size_y= s_x;
    opener.destroy_window(disp);
    opener.create_window(disp, win, 0, 0, opener_size_x, size_y);
    opener.set_window_visible(disp, opener.visible);
  }
  buttons.destroy_window(disp);
  buttons.create_window(disp, win, opener_size_x+1, 0, buttons_size_x, size_y);
  buttons.set_window_visible(disp, buttons.visible);
}

void MenuX11::resize(int s_x) {
  resize(s_x,size_y);
}

/******************************************************************************/

bool MenuX11::set_number_of_buttons(int num) {
  if (num <= 0)
    return false;

  if (num < num_buttons_max)
    num_buttons= num;
  else {
    Label *old= label;
    label= new Label[num];
    for (int i=0; i< num_buttons_max; i++) {
      label[i].str= old[i].str;
      label[i].len= old[i].len;
      label[i].len_max= old[i].len_max;
      label[i].width_max= old[i].width_max;
    }
    num_buttons_max= num;
    num_buttons= num;
    delete[] old;
  }

  if ( !buttons.exists ) {
    buttons.create_window(disp, win, opener_size_x+1, 0, buttons_size_x, size_y);
    //buttons.set_window_visible(disp);
  }
  if ( !opener.exists ) {
    opener.create_window(disp, win, 0, 0, opener_size_x, size_y);
    opener.set_window_visible(disp);
  }

  return true;
}

bool MenuX11::set_button_label(int num, const char * lab) {
  if (num < 0 || num >= num_buttons)
    return false;

  int len= strlen(lab);
  if ( len >= label[num].len_max ) {
    if ( label[num].str )
      delete[] label[num].str;
    label[num].str = new char[len+1];
    label[num].len_max= len;
  }
  strcpy(label[num].str,lab);
  label[num].len= len;
  label[num].width_max= get_text_width(lab,len);

  flag_needs_redraw= true;
  return true;
}

const char * MenuX11::get_button_label(int num) const {
  if (num < 0 || num >= num_buttons)
    return 0;

  return label[num].str;
}

bool MenuX11::set_menu_extended(bool flag) {
  buttons.set_window_visible(disp,flag);
  return true;
}

/******************************************************************************/
