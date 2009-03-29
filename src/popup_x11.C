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

#include "global_defs.h"
#include "popup_x11.h"

#include <iostream>
#include <cstring>

#define DEBUG(X)
/******************************************************************************/
/******************************************************************************/

const int PopupX11::space_between_buttons = 6;
const int PopupX11::space_left_of_buttons = 6;


PopupX11::MyWin::MyWin( Display * d,
                        Window w )
{
    disp = d;
    parent_win = w;

    x = 0;
    y = 0;
    size_x = 0;
    size_y = 0;

    exists = false;
    redraw = true;
    visible = false;
    mapped = false;
}

void
PopupX11::MyWin::resize( int s_x,
                         int s_y )
{
    if ( size_x == s_x && size_y == s_y )
    {
        return;
    }

    size_x = s_x;
    size_y = s_y;

    if ( visible )
    {
        destroy();
        create();
    }
}

void
PopupX11::MyWin::destroy()
{
    if ( exists )
    {
        XDestroyWindow( disp, win );
        XFreePixmap( disp, pixmap );
    }

    exists = false;
    mapped = false;
}

void
PopupX11::MyWin::create()
{
    if ( ! visible )
    {
        return;
    }

    XSetWindowAttributes attr;

    attr.background_pixel = bg_pixel;
    attr.border_pixel = fg_pixel;
    attr.backing_store = NotUseful;
    attr.event_mask = ButtonPressMask    | ExposureMask | ButtonReleaseMask;
    //    | EnterWindowMask | LeaveWindowMask;

    attr.bit_gravity = NorthWestGravity;
    attr.win_gravity = NorthWestGravity;
    attr.save_under = False;

    win = XCreateWindow( disp, parent_win, x, y, size_x, size_y,
                         1, 0, InputOutput, CopyFromParent,
                         CWBackPixel | CWBorderPixel | CWEventMask |
                         CWBitGravity | CWWinGravity | CWBackingStore |
                         CWSaveUnder, &attr );

    pixmap = XCreatePixmap ( disp, win, size_x, size_y, DefaultDepth ( disp, DefaultScreen( disp ) ) );

    exists = true;
}

void
PopupX11::MyWin::set_visible( bool flag )
{
    visible = flag;

    if ( visible )
    {
        if ( ! exists )
        {
            create();
        }

        if ( ! mapped )
        {
            DEBUG( cout << "\nMapping window" << flush; );
            XMapWindow( disp, win );
            mapped = true;
            XFlush( disp );
        }
    }
    else
    {
        if ( mapped )
        {
            XUnmapWindow( disp, win );
            mapped = false;
            //XFlush(disp);
        }
    }
}

void
PopupX11::MyWin::move( int xx,
                       int yy )
{
    x = xx;
    y = yy;

    if ( ! exists )
    {
        return;
    }

    XMoveWindow( disp, win, x, y );

    if ( mapped && visible )
    {
        XFlush( disp );
    }
}

/******************************************************************************/
int
PopupX11::get_text_width( int len ) const
{
    return len* char_max_width;
}

void
PopupX11::compute_size_for_buttons_window( int & s_x,
                                           int & s_y ) const
{
    s_x = max_text_width_of_button + 2 * space_left_of_buttons;

    if ( s_x < 50 )
    {
        s_x = 50;
    }

    DEBUG( cout << "\n-----\ns_x= " << s_x << "    max_text_width_of_button= " << max_text_width_of_button << " char_max_width= " << char_max_width; );

    s_y = num_buttons_max * ( char_max_descent + char_max_ascent + space_between_buttons );
}

PopupX11::PopupX11( Display * d,
                    Window pw,
                    GC g )
  : subwin( d, pw )
{
    disp = d;
    win = pw;
    gc = g;
    max_text_width_of_button = 0;

    label = 0;
    num_buttons = 0;
    num_buttons_max = 0;
    highlighted_label = -1;

    gc_context = XGContextFromGC( gc );
    XFontStruct * fontstruct = XQueryFont( disp, gc_context );

    if ( fontstruct )
    {
        char_max_width = fontstruct->max_bounds.width;
        char_max_ascent = fontstruct->max_bounds.ascent;
        char_max_descent = fontstruct->max_bounds.descent;

        //cout << "\n max font: width=" << char_max_width << " ascent=" << char_max_ascent << " descent=" << char_max_descent << flush;

        XFreeFontInfo( 0, fontstruct, 1 );
    }
    else
    {
        char_max_width = 7;
        char_max_ascent = 12;
        char_max_descent = 3;
        WARNING_OUT << "\nfontstruct= 0, setting default values " << std::flush;
    }

#if 1
    set_number_of_buttons( 1 );
    set_button_label( 0, "menu empty" );
#else
    set_number_of_buttons( 10 );

    set_button_label( 0, "drop ball" );
    set_button_label( 1, "free kick left" );
    set_button_label( 2, "free kick right" );
    set_button_label( 3, "dummy" );
    set_button_label( 4, "dummy" );
    set_button_label( 5, "dummy" );
    set_button_label( 6, "dummy" );
    set_button_label( 7, "dummy" );
    set_button_label( 8, "dummy" );
    set_button_label( 9, "dummy" );

#endif
}

PopupX11::~PopupX11()
{
    subwin.destroy();
}

unsigned int
PopupX11::get_color_pixel( const RGBcolor & col ) const
{
    XColor xcol;
    xcol.red = col.get_red();
    xcol.green = col.get_green();
    xcol.blue = col.get_blue();

    if ( ! XAllocColor( disp, DefaultColormap( disp, 0 ), &xcol ) )
    {
        std::cerr << "\nwarning: problem with allocating color:"
                  << "\n(red,green,blue)= (" << int( col.red ) << "," << int( col.green ) << "," << int( col.blue )
                  << "\n(Red,Green,Blue)= (" << col.get_red() << "," << col.get_green() << "," << col.get_blue() << ")"
                  << "\nresulting in XColor.pixel= " << xcol.pixel
                  << "\n(XRed,XGreen,XBlue)= (" << xcol.red << "," << xcol.green << "," << xcol.blue << ")";
    }

    return xcol.pixel;
}

bool
PopupX11::is_in_buttons_window( int x,
                                int y ) const
{
    if ( x < subwin.x || y < subwin.y )
    {
        return false;
    }

    if ( x > subwin.x + subwin.size_x || y > subwin.y + subwin.size_y )
    {
        return false;
    }

    DEBUG( cout << "\nis in button window " << flush; );
    return true;
}

void
PopupX11::check_buttons_redraw( int highlight )
{
    highlighted_label = highlight;

    if ( ! subwin.visible )
    {
        return;
    }

    DEBUG( cout << "\nredrawing popup text" << flush; );

    XSetForeground( disp, gc, subwin.bg_pixel );
    XFillRectangle( disp, subwin.pixmap, gc, 0, 0, subwin.size_x, subwin.size_y );

    XSetForeground( disp, gc, subwin.fg_pixel );

    for ( int i = 0; i < num_buttons; ++i )
    {
        int y1, y2;
        get_y_bounds_of_label( i, y1, y2 );

        if ( i == highlight )
        {
            XSetForeground( disp, gc, subwin.highlight_pixel );
            XFillRectangle( disp, subwin.pixmap, gc, 0, y1, subwin.size_x, y2 - y1 );
            XSetForeground( disp, gc, subwin.fg_pixel );

            if ( i > 0 )
                XDrawLine( disp, subwin.pixmap, gc, 0, y1 , subwin.size_x, y1 );
        }

        if ( label[i].str )
        {
            XDrawString( disp, subwin.pixmap, gc,
                         space_left_of_buttons,
                         y2 - char_max_descent - space_between_buttons / 2,
                         label[i].str, label[i].len );
        }

        XDrawLine( disp, subwin.pixmap, gc, 0, y2 , subwin.size_x, y2 );
    }

    XCopyArea( disp, subwin.pixmap, subwin.win, gc, 0, 0, subwin.size_x, subwin.size_y, 0, 0 );

    //XFlush(disp);
}

void
PopupX11::check_buttons_resize_window()
{
    int s_x, s_y;
    compute_size_for_buttons_window( s_x, s_y );
    subwin.resize( s_x, s_y );
}

void
PopupX11::get_y_bounds_of_label( int i,
                                 int & y_top,
                                 int & y_bottom ) const
{
    int dum = ( char_max_descent + char_max_ascent + space_between_buttons );
    y_top = i * dum;
    y_bottom = y_top + dum;
}

int
PopupX11::get_label_number_from_y_value( int y ) const
{
    if ( y <= 0 || y >= subwin.size_y )
    {
        return -1;
    }

    int dum = ( char_max_descent + char_max_ascent + space_between_buttons );

    int i = y / dum;
    if ( i >= num_buttons )
    {
        return -1;
    }

    return i;
}

DEBUG(
      void __print_event_type( int type )
      {
          switch ( type )
          {

          case KeyPress        :
              cout << "\n KeyPress        ";
              break;

          case KeyRelease      :
              cout << "\n KeyRelease      ";
              break;

          case ButtonPress     :
              cout << "\n ButtonPress     ";
              break;

          case ButtonRelease   :
              cout << "\n ButtonRelease   ";
              break;

          case MotionNotify    :
              cout << "\n MotionNotify    ";
              break;

          case EnterNotify     :
              cout << "\n EnterNotify     ";
              break;

          case LeaveNotify     :
              cout << "\n LeaveNotify     ";
              break;

          case FocusIn         :
              cout << "\n FocusIn         ";
              break;

          case FocusOut        :
              cout << "\n FocusOut        ";
              break;

          case KeymapNotify    :
              cout << "\n KeymapNotify    ";
              break;

          case Expose          :
              cout << "\n Expose          ";
              break;

          case GraphicsExpose  :
              cout << "\n GraphicsExpose  ";
              break;

          case NoExpose        :
              cout << "\n NoExpose        ";
              break;

          case VisibilityNotify:
              cout << "\n VisibilityNotify";
              break;

          case CreateNotify    :
              cout << "\n CreateNotify    ";
              break;

          case DestroyNotify   :
              cout << "\n DestroyNotify   ";
              break;

          case UnmapNotify     :
              cout << "\n UnmapNotify     ";
              break;

          case MapNotify       :
              cout << "\n MapNotify       ";
              break;

          case MapRequest      :
              cout << "\n MapRequest      ";
              break;

          case ReparentNotify  :
              cout << "\n ReparentNotify  ";
              break;

          case ConfigureNotify :
              cout << "\n ConfigureNotify ";
              break;

          case ConfigureRequest:
              cout << "\n ConfigureRequest";
              break;

          case GravityNotify   :
              cout << "\n GravityNotify   ";
              break;

          case ResizeRequest   :
              cout << "\n ResizeRequest   ";
              break;

          case CirculateNotify :
              cout << "\n CirculateNotify ";
              break;

          case CirculateRequest:
              cout << "\n CirculateRequest";
              break;

          case PropertyNotify  :
              cout << "\n PropertyNotify  ";
              break;

          case SelectionClear  :
              cout << "\n SelectionClear  ";
              break;

          case SelectionRequest:
              cout << "\n SelectionRequest";
              break;

          case SelectionNotify :
              cout << "\n SelectionNotify ";
              break;

          case ColormapNotify  :
              cout << "\n ColormapNotify  ";
              break;

          case ClientMessage   :
              cout << "\n ClientMessage   ";
              break;

          case MappingNotify   :
              cout << "\n MappingNotify   ";
              break;

          default:
              cout << "\n UNKNOWN EVENT";
          }
      }

      )

bool
PopupX11::responsible_for_event( const XEvent & event )
{
    //cout << "\nresponsible_for_event ??? subwin.win= " << subwin.win << " win= " << event.xany.window << flush;

    if ( ! subwin.exists || ! subwin.visible )
    {
        return false;
    }

    if ( event.type == NoExpose )
    {
        DEBUG( cout << "\nevent.xany.window == subwin.win"; );
        return false;
    }

    if ( event.xany.window == subwin.win )
    {
        DEBUG( cout << "\nevent.xany.window == subwin.win"; );
        return true;
    }

    if ( ( event.type == ButtonRelease || event.type == MotionNotify )
         && is_in_buttons_window( event.xbutton.x, event.xbutton.y ) )
    {
        return true;
    }

    //not responsible

    if ( highlighted_label >= 0 )
    {
        check_buttons_redraw();
    }

    DEBUG( __print_event_type( event.type ); cout << "<- not responsible " << flush; );

    return false;
}

bool
PopupX11::process_event( const XEvent & event )
{
    if ( subwin.exists && subwin.visible && event.xany.window == subwin.win )
    {
        switch ( event.type ) {

        case Expose:
            //cout << "\nexpose me";
            subwin.redraw = true;
            break;

        case ButtonPress:
            break;
        }
    }

    bool redrawn = false;

    int i = -1;
    DEBUG( __print_event_type( event.type ); );

    if ( true || event.type == MotionNotify )
    {
        int event_pos_y = event.xbutton.y;

        if ( true || event.type != NoExpose || event.xany.window != subwin.win )
        {
            DEBUG( cout << "\n --------------------- " << event_pos_y; );
            event_pos_y -= subwin.y;
            DEBUG( cout << "    " << event_pos_y; );
            i = get_label_number_from_y_value( event_pos_y );
        }

        DEBUG( cout << "\n>>>> i= " << i << flush; );
    }

    if ( /* subwin.redraw ||*/ i >= 0 && i != highlighted_label )
    {
        DEBUG( cout << "\n}}}}} i= " << i << "highlighted_label= " << highlighted_label << flush; );
        check_buttons_redraw( i );
        redrawn = true;
    }

    if ( redrawn )
    {
        XFlush( disp );
    }

    return false;
}

bool
PopupX11::button_pressed( const XEvent & event,
                          int & button,
                          int & mouse_button ) const
{
    if ( ! subwin.exists || ! subwin.visible )
    {
        return false;
    }

    int event_pos_x = event.xbutton.x;
    int event_pos_y = event.xbutton.y;

    if ( event.xany.window != subwin.win )
    {
        event_pos_x += subwin.x;
        event_pos_y -= subwin.y;
    }

    if ( event.type == ButtonRelease )
    {
        int i = get_label_number_from_y_value( event_pos_y );
        button = i;
        mouse_button = event.xbutton.button;
        DEBUG( cout << "\n>>>>>>button " << i << flush; );
        return true;
    }

    return false;
}

void
PopupX11::redraw()
{
    check_buttons_redraw();
}

void
PopupX11::set_background_color( const RGBcolor & col )
{
    unsigned int pixel = get_color_pixel( col );

    if ( subwin.bg_pixel !=  pixel )
    {
        subwin.bg_color = col;
        subwin.bg_pixel = pixel;

        if ( subwin.exists )
        {
            XSetWindowBackground( disp, subwin.win, subwin.bg_pixel );
        }
    }
}

void
PopupX11::set_foreground_color( const RGBcolor & col )
{
    unsigned int pixel = get_color_pixel( col );
    subwin.fg_color = col;
    subwin.fg_pixel = pixel;
    XSetForeground( disp, gc, pixel ) ;
}

void
PopupX11::set_highlight_color( const RGBcolor & col )
{
    unsigned int pixel = get_color_pixel( col );
    subwin.highlight_color = col;
    subwin.highlight_pixel = pixel;
}

/******************************************************************************/

bool
PopupX11::set_number_of_buttons( int num )
{
    if ( num <= 0 )
    {
        return false;
    }

    if ( num < num_buttons_max )
    {
        num_buttons = num;
    }
    else
    {
        Label * old = label;
        label = new Label[num];

        for ( int i = 0; i < num_buttons_max; ++i )
        {
            label[i].str = old[i].str;
            label[i].len = old[i].len;
            label[i].len_max = old[i].len_max;
            label[i].width_max = old[i].width_max;
        }

        num_buttons_max = num;

        num_buttons = num;
        delete[] old;
    }

    check_buttons_resize_window();

    return true;
}

bool
PopupX11::set_button_label( int num,
                            const char * lab )
{
    if ( num < 0 || num >= num_buttons )
    {
        return false;
    }

    int len = std::strlen( lab );

    if ( len >= label[num].len_max )
    {
        if ( label[num].str )
        {
            delete[] label[num].str;
        }

        label[num].str = new char[len+1];

        label[num].len_max = len;
    }

    std::strcpy( label[num].str, lab );

    label[num].len = len;
    label[num].width_max = get_text_width( len );

    max_text_width_of_button = 0;

    for ( int i = 0; i < num_buttons; ++i )
    {
        if ( label[i].width_max > max_text_width_of_button )
        {
            max_text_width_of_button = label[i].width_max;
        }
    }

    check_buttons_resize_window(); //lazy evaluation

    return true;
}

const char *
PopupX11::get_button_label( int num ) const
{
    if ( num < 0 || num >= num_buttons )
    {
        return 0;
    }

    return label[num].str;
}

void
PopupX11::set_popup_visible( int x,
                             int y,
                             int max_size_x,
                             int max_size_y )
{
    if ( x + subwin.size_x > max_size_x )
    {
        x = x - subwin.size_x;

        if ( x < 0 )
        {
            x = 0;
        }
    }

    if ( y + subwin.size_y > max_size_y )
    {
        y = y - subwin.size_y;

        if ( y < 0 )
        {
            y = 0;
        }
    }

    subwin.move( x, y );

    subwin.set_visible( true );
    check_buttons_redraw();
    XFlush( disp );
}

/******************************************************************************/
