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
#ifndef POPUP_X11_H
#define POPUP_X11_H

#include "menu_base.h"
#include "rgbcolor.h"

#include <X11/Xlib.h>


class PopupX11
    : public MenuBase {

    struct MyWin {
        Display * disp;
        Window parent_win;
        Window win;
        Pixmap pixmap;

        int x;
        int y;
        int size_x;
        int size_y;

        bool exists;
        bool mapped;  //< indicates, if the win is mapped
        bool visible;
        bool redraw;

        RGBcolor bg_color;
        unsigned long bg_pixel;
        RGBcolor fg_color;
        unsigned long fg_pixel;
        RGBcolor highlight_color;
        unsigned long highlight_pixel;

        MyWin( Display * d,
               Window w );

        void resize( int s_x, int s_y );
        void destroy();
        void create();
        void set_visible( bool flag = true );
        void move( int x, int y );
    };

    struct Label {

        int len;
        int len_max;
        char * str;
        int width_max;

        Label()
            : len( 0 ),
              len_max( 0 ),
              str( 0 )
          { }
    };

    /******************************************************************************/
    static const int space_between_buttons;
    static const int space_left_of_buttons;

    Display *disp;
    Window win;
    GC gc;
    GContext gc_context; //needed to get information about the font (XQueryExtent, XQueryTextExtent)

    int char_max_ascent;
    int char_max_descent;
    int char_max_width;

    int max_text_width_of_button;

    int num_buttons;
    int num_buttons_max;
    int highlighted_label;

public: //debug
    MyWin subwin;
    Label * label;

    int get_text_width( int len ) const;
    void compute_size_for_buttons_window( int & s_x,
                                          int & s_y ) const;
    bool is_in_buttons_window( int x,
                               int y ) const;
    void check_buttons_redraw( int i = -1 );
    void check_buttons_resize_window();

    //void get_x_bounds_of_label(int i);
    void get_y_bounds_of_label( int i,
                                int & y_top,
                                int & y_bottom ) const;
    int get_label_number_from_y_value( int y ) const;

public:
    PopupX11( Display * d,
              Window pw,
              GC g );
    virtual ~PopupX11();

    unsigned int get_color_pixel( const RGBcolor & col ) const;

    bool responsible_for_event( const XEvent & event );
    /** \short returns true if some buttons disappeared and a redraw should be done.
        Normally a Expose event will be triggered and the underlying window can
        react on it. But sometimes it's not fast enough, so that this information
        can be useful
    */
    bool process_event( const XEvent & event );
    bool button_pressed( const XEvent & event,
                         int & button,
                         int & mouse_button ) const;

    void redraw();

    void set_background_color( const RGBcolor & col );
    void set_foreground_color( const RGBcolor & col );
    void set_highlight_color( const RGBcolor & col );

public:
    void get_rectangle( int & x,
                        int & y,
                        int & size_x,
                        int & size_y ) const
      {
          x = subwin.x;
          y = subwin.y;
          size_x = subwin.size_x;
          size_y = subwin.size_y;
      }

    void set_popup_visible( int x,
                            int y,
                            int max_size_x,
                            int max_size_y );

    void set_popup_invisible()
      {
          subwin.set_visible( false );
      }

public:
    ///this are the interface methods used by the devices!
    bool needs_redraw() const
      {
          return false;
      }

    bool set_number_of_buttons( int num );

    int  get_nubber_of_buttons() const
      {
          return num_buttons;
      }

    bool set_button_label( int num,
                           const char * lab );

    const char * get_button_label( int num ) const;

    bool set_menu_extended( bool flag )
      {
          if ( flag )
          {
              set_popup_invisible();
              return true;
          }

          return false;
      }

    bool set_exit_program()
      {
          return false;
      }
};

#endif
