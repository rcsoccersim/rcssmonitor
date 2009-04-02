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
#ifndef MENU_X11_H
#define MENU_X11_H

#include "menu_base.h"
#include "rgbcolor.h"

#include <X11/Xlib.h>


class MenuX11
    : public MenuBase {

    Display * disp;
    Window win;
    GC gc;
    GContext gc_context; //needed to get information about the font (XQueryExtent, XQueryTextExtent)

    bool do_exit;

    int text_max_ascent;
    int text_max_descent;
    int text_max_width;

    int get_text_width( const char * str, int len ) const;

    struct MyWin {
        MyWin();
        Window win;

        bool exists;
        ///indicates, if the win is mapped
        bool mapped;
        bool visible;
        bool redraw;

        RGBcolor bg_color;
        unsigned long bg_pixel;
        RGBcolor fg_color;
        unsigned long fg_pixel;

        void destroy_window( Display * disp );
        void create_window( Display * disp, Window parent, int x, int y, int size_x, int size_y );
        void set_window_visible( Display * disp, bool flag = true );

    };

    MyWin buttons;
    MyWin opener;
    int size_x;
    int size_y;
    int buttons_size_x;
    static const int opener_size_x;


    struct Label {
        Label()
          {
              len = 0;
              str = 0;
              len_max = 0;
          }

        int len;
        int len_max;
        char * str;
        int width_max;
    };

    Label * label;

    int num_buttons;
    int num_buttons_max;

    bool flag_needs_redraw;
    void buttons_redraw();
    void opener_redraw();

public:
#if 0
    static const int MODE_INVISIBLE = 0;
    static const int MODE_EXTENDED  = 1;
    static const int MODE_CONTRACTED = 2;
    int  get_mode() const;
    void set_mode( int m );
#endif

    MenuX11( Display *d, Window pw, GC g, int x, int y );
    virtual ~MenuX11();

    unsigned int get_color_pixel( const RGBcolor & col ) const;

    bool responsible_for_event( const XEvent & event ) const;
    /** \short returns true if some buttons disappeared and a redraw should be done.
        Normally a Expose event will be triggered and the underlying window can
        react on it. But sometimes it's not fast enough, so that this information
        can be useful
    */
    bool process_event( const XEvent & event );
    bool button_pressed( const XEvent & event,
                         int & button,
                         int & mouse_button ) const;

    bool needs_redraw() const
      {
          return flag_needs_redraw;
      }

    void redraw();

    void set_background_color( const RGBcolor & col );
    void set_foreground_color( const RGBcolor & col );

    void resize( int s_x, int s_y );
    void resize( int s_x );

    bool get_exit_program()
      {
          return do_exit;
      };

public:
    ///this are the interface methods used by the devices!
    bool set_number_of_buttons( int num );
    int  get_nubber_of_buttons() const
      {
          return num_buttons;
      }

    bool set_button_label( int num, const char * lab );
    const char * get_button_label( int num ) const;
    bool set_menu_extended( bool flag = true );

    bool set_exit_program()
      {
          do_exit = true;
          return true;
      };
};

#endif
