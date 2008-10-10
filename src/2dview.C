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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <iostream>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>	/* For erasing cursor - not important	*/
#include <X11/keysym.h>  /* ART! */
#include <X11/keysymdef.h> /* ART! */
#include <X11/xpm.h>

#include <csignal>
#include <sys/time.h>
#include <fstream>

#include "global_defs.h"
#include "tools.h"
#include "display_x11.h"
#include "menu_x11.h"
#include "popup_x11.h"
#include "structure.h"
#include "builder_direct.h"
#include "valueparser.h"
#include "udpsocket.h"
#include "rgb_db.h"

#include "smonitor_dev.h"
#include "ascii_dev.h"

#include <cerrno>   //for return values of select
#include <unistd.h>

#include "rcssmonitor.xpm"


namespace {
void
show_copyright_notice( std::ostream & o )
{
    o << "\n"
      << "/*****************************************************************************/\n"
      << "                                   "<< PACKAGE << "-" << VERSION << '\n'
      << "\n"
      << "Copyright (c) 1999 - 2001, Artur Merke\n"
      << "\t<amerke@ira.uka.de>\n"
      << "Copyright (c) 2001 - 2008, The RoboCup Soccer Simulator Maintainance Group.\n"
      << "\t<sserver-admin@lists.sourceforge.net>\n"
      << "\n"
      << PACKAGE << "-" << VERSION << " is free software; you can redistribute it and/or modify\n"
      << "it under the terms of the GNU General Public License as published by\n"
      << "the Free Software Foundation; either version 2, or (at your option)\n"
      << "any later version.\n"
      << "\n"
      << PACKAGE << "-" << VERSION << " is distributed in the hope that it will be useful,\n"
      << "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
      << "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
      << "GNU General Public License for more details.\n"
      << "/*****************************************************************************/\n";
}
}


DisplayX11 *DDD;

InputDevice * INPUTDEV;

#define POPUP

struct Pointer {
    int x;
    int y;
    Pointer(int xx, int yy)
        : x( xx )
        , y( yy )
      { }
};

struct Rectangle {
    bool active;
    int x, y;
    unsigned int width, height;
    Rectangle();
    Rectangle( int window_width, int window_height );
    void set_ratio( int window_width, int widow_height );
    void set_origin( int px, int py )
      {
          p1x = px;
          p1y = py;
          p2x = p1x;
          p2y = p1y;
          conform();
      }
    void set_point( int px, int py )
      {
          p2x = px;
          p2y= py;
          conform();
      }
private:
    void conform();
    bool use_ratio;
    bool use_center;
    int win_width, win_height;
    int p1x,p1y,p2x,p2y;
};

Rectangle::Rectangle()
{
    active = false;
    use_ratio = false;
    use_center = false;
    set_origin( 0, 0 );
    conform();
}

Rectangle::Rectangle( int window_width,
                      int window_height )
{
    //use_center= true;
    use_center = false;
    set_ratio( window_width, window_height );
    set_origin( 0, 0 );
    conform();
}

void
Rectangle::set_ratio( int window_width,
                      int window_height )
{
    use_ratio = true;
    win_width = window_width;
    win_height = window_height;
}

void
Rectangle::conform()
{
    if ( use_ratio )
    {
        double ratio_width = std::fabs( double( p2x - p1x ) / double( win_width ) );
        double ratio_height = std::fabs( double( p2y - p1y ) / double( win_height ) );

        if ( ratio_width > ratio_height )
        {
            if ( p2y > p1y )
            {
                p2y= p1y+ (int)rint( double( win_height ) * ratio_width );
            }
            else
            {
                p2y= p1y- (int)rint( double( win_height ) * ratio_width );
            }
        }
        else
        {
            if ( p2x > p1x )
            {
                p2x = p1x + (int)rint( double( win_width ) * ratio_height );
            }
            else
            {
                p2x = p1x - (int)rint( double( win_width ) * ratio_height );
            }
        }
    }

    width = std::abs( p1x - p2x );
    height = std::abs( p1y - p2y );

    if ( use_center )
    { //center
        x= p1x - width;
        y= p1y - height;
        width *= 2;
        height *= 2;
    }
    else
    {
        x = p1x < p2x ? p1x : p2x;
        y = p1y < p2y ? p1y : p2y;
    }

}


namespace Options {

//specifies the clip of euclidean plane
Area2d plane;

bool set_window_pos;

int window_left_x;
int window_top_y;

int window_size_x;
int window_size_y;

int line_thickness;
char font_name[MAX_NAME_LEN];
char font_inside[MAX_NAME_LEN];

bool auto_quit_mode;
int auto_quit_wait;

bool auto_reconnect_mode;
int auto_reconnect_wait;

char conf_file[MAX_NAME_LEN];

/* =========================================================================*/

void
show_available_options( std::ostream & o,
                        const char * fname )
{
    o << "\n/*****************************************************************************/"
      << "\n                                   " << PACKAGE << "-" << VERSION << std::endl
      << "\nUsage: " << fname << " options"
      << "\n"
      << "\nOptions can be read from a configuration file or from the command line."
      << "\nValues in square brackets represent default values."
      << "\n"
      << "\nCommand line options:"
      << "\n"
      << "\n-h              this page"
      << "\n-help           this page"
      << "\n-copyleft       shows program information"
      << "\n-gen_conf_file  generate a configuration file on standard output"
      << "\n-keys           shows key bindings"
      << "\n"
      << "\n-conf_file      [\"\"]  configuration file to be read|"
      << "\n"
      << "\nCommand line options and configuration file options (command line has priority):"
      << "\n"
      << "\ngeneral options:"
      << "\n"
      << "\n-plane_origin_x [" << plane.center.x << "]"
      << "\n-plane_origin_y [" << plane.center.y << "]"
      << "\n-plane_size_x [" << plane.size_x << "]"
      << "\n-plane_size_y [" << plane.size_y << "]"
      << "\n"
#if 0
      << "\n-window_left_x [not set]"
      << "\n-window_top_y  [not set]"
#endif
      << "\n-window_size_x [" << window_size_x << "]"
      << "\n-window_size_y [" << window_size_y << "]"
      << "\n"
      << "\n-line_thickness [" << line_thickness << "]"
      << "\nthis is the font in the status line"
      << "\n-font_name      [" << font_name << "]"
      << "\nthis is the font used in objects (like players in SMonitorDevice)"
      << "\n-font_inside    [" << font_inside << "]"
      << "\n"
      << "\n-auto_quit_mode [" << auto_quit_mode << "]"
      << "\n-auto_quit_wait [" << auto_quit_wait << "]"
      << "\n"
      << "\n-auto_reconnect_mode [" << auto_reconnect_mode << "]"
      << "\n-auto_reconnect_wait [" << auto_reconnect_wait << "]"
      << "\n";

    INPUTDEV->help_options( o );
    o << "\n/*****************************************************************************/"
      << "\n";
}

void
generate_file_options( std::ostream & o,
                       const char * fname )
{
    o << "#############################################################################"
      << "#                            " << PACKAGE << "-" << VERSION << std::endl
      << "#"
      << "# this configuration file was autamaticly generated by calling "
      << "\n"
      << "\n#    " << fname << " -gen_conf_file"
      << "\n# you can use it to customize " << PACKAGE << "-" << VERSION << std::endl
      << "\n"
      << "\n# general options:"
      << "\n"
      << "\n### plane is the smallest part of the euclidean plane you wish to be set es default"
      << "\n# x coordinate of the plane center (default " << plane.center.x << ")"
      << "\nplane_origin_x = " //<< plane.center.x
      << "\n# y coordinate of the plane center (default " << plane.center.x << ")"
      << "\nplane_origin_y = " //<< plane.center.y
      << "\n# width of the plane (default " << plane.size_x << ")"
      << "\nplane_size_x = " //<< plane.size_x
      << "\n# height of the plane (default " << plane.size_y << ")"
      << "\nplane_size_y = " //<< plane.size_y
      << "\n"
#if 0
      << "\n# left-top corner x pos of the window (default not set)"
      << "\nwindow_left_x = " //<< window_left_x
      << "\n# left-top corner y pos of the window (default not set)"
      << "\nwindow_top_y = " //<< window_top_y
#endif
      << "\n# window width (default " << window_size_x << ")"
      << "\nwindow_size_x = " //<< window_size_x
      << "\n# window height (default " << window_size_y << ")"
      << "\nwindow_size_y = " //<< window_size_y
      << "\n"
      << "\n# thickness of lines (default " << line_thickness << ")"
      << "\nline_thickness = " //<< line_thickness
      << "\n"
      << "\n# font which is used in the status line (default " << font_name << ")"
      << "\nfont_name = " //<< font_name
      << "\n# this is the font used in objects [like players in SMonitorDevice] (default " << font_inside << ")"
      << "\nfont_inside = " //<< font_inside
      << "\n"
      << "\n# determins whether monitor program quits automatically (default 0)."
      << "\nauto_quit_mode = "
      << "\n# wait seconds in auto_quit_mode (default " << auto_quit_wait << ")"
      << "\nauto_quit_wait = "
      << "\n"
      << "\n# determins whether monitor program tries to reconnect automatically (default 0)."
      << "\nauto_reconnect_mode = "
      << "\n# wait seconds in auto_reconnect_mode (default " << auto_reconnect_wait << ")"
      << "\nauto_reconnect_wait = "
      << "\n";
    INPUTDEV->generate_file_options( o );
}


bool
set_defaults()
{
    plane = Area2d( Point2d( 0.0, 2.0 ), 112.0, 85.0 );

    set_window_pos = false;
    window_left_x = 5;
    window_top_y =  5;

    window_size_x = 600;
    window_size_y = 450;

    line_thickness = 1;
    std::strcpy( font_name, "6x13bold" );
    //std::strcpy( font_inside, "6x13bold" );
    std::strcpy( font_inside, "6x13" );

    auto_quit_mode = false;
    auto_quit_wait = 5;

    auto_reconnect_mode = false;
    auto_reconnect_wait = 5;

    conf_file[0] = '\0';
    return true;
}

bool
process_options( ValueParser & vp )
{
    vp.get( "plane_origin_x", plane.center.x );
    vp.get( "plane_origin_y", plane.center.y );
    vp.get( "plane_size_x", plane.size_x );
    vp.get( "plane_size_y", plane.size_y );

#if 0
    if ( vp.get("window_left_x",window_left_x) > 0 )
        set_window_pos= true;
    if ( vp.get("window_top_y",window_top_y) > 0 )
        set_window_pos= true;
#endif

    vp.get("window_size_y",window_size_y);
    vp.get("window_size_x",window_size_x);
    vp.get("window_size_y",window_size_y);

    int tmp = line_thickness;
    vp.get("line_thickness",tmp);
    if ( tmp < 4 && tmp > 0 ) line_thickness= tmp;
    vp.get("font_name",font_name,MAX_NAME_LEN);
    vp.get("font_inside",font_inside,MAX_NAME_LEN);

    vp.get( "auto_quit_mode", auto_quit_mode );
    vp.get( "auto_quit_wait", auto_quit_wait );

    vp.get( "auto_reconnect_mode", auto_reconnect_mode );
    vp.get( "auto_reconnect_wait", auto_reconnect_wait );
    return true;
}


void
show_key_bindings( std::ostream & o )
{
    o << "\n"
      << "/*****************************************************************************/\n"
      << "\n"
      << "                                   " << PACKAGE << "-" << VERSION << std::endl
      << "\n"
      << "viewer commands:\n"
      << "f                  = freeze the screen\n"
      << "i                  = set viewing clip to the initial clip\n"
      << "k                  = show key bindings\n"
      << "q                  = quit viewer\n"
      << "+                  = zoom in\n"
      << "-                  = zoom out\n"
      << "single mouse click = change view center to mouse pointer position\n"
      << "drag & drop        = define new rectangle view area (too small regions will be rejected)\n";
    INPUTDEV->help_char_command(o);
    o << "\n"
      << "\n"
      << "/*****************************************************************************/\n";
}

bool
read( int argc,
               char ** argv)
{
    bool res = true;
    bool dum = true;

    ValueParser cl( argc, argv );
    //cl.set_verbose(true);

    if ( cl.get( "help", dum ) >= 0
         || cl.get( "h", dum ) >= 0 )
    {
        show_available_options( std::cout, PACKAGE );
        std::exit( 0 );
    }

    if ( cl.get( "gen_conf_file", dum ) >= 0 )
    {
        generate_file_options( std::cout, PACKAGE );
        std::exit( 0 );
    }

    if ( cl.get( "keys", dum ) >= 0 )
    {
        show_key_bindings( std::cout );
        std::exit( 0 );
    }

    if ( cl.get( "copyleft", dum ) >= 0 )
    {
        show_copyright_notice( std::cout );
        std::exit( 0 );
    }

    cl.get( "conf_file", conf_file, MAX_NAME_LEN );
    if ( std::strlen( conf_file ) )
    {
        std::cout << "\nreading options from file: " << conf_file;
        ValueParser vp( conf_file );
        //vp.set_verbose(true);
        process_options( vp );
        INPUTDEV->process_options( conf_file );
    }
    process_options( cl );
    INPUTDEV->process_options( argc, argv );
    std::cout << "End process_options" << std::endl;
    return res;
}

} // end namespace Options

namespace RUN {

DrawTree tree;
ConvArea2d conv_area;

BuilderDirect builder( tree, conv_area );

bool quit = false;
bool freeze = false;

void
toggle_freeze()
{
    freeze = ! freeze;
}

} // end namespace RUN

namespace WIN {

int win_width, win_height;
int win_depth;
Display * disp;
Pixmap pixmap;
Window window;
MenuX11 * menu = 0;
PopupX11 * popup = 0;

Pixmap icon_pixmap;
Pixmap icon_mask;

GC gc, xor_gc, bg_gc, sl_gc, bt_gc; //sl= status line, bt= button

Rectangle clip_rect;
Pointer mouse_point( 0, 0 );
unsigned int mouse_button = 0;
bool rect_was_drawn = false;
int x11_fd;

} // end namespace WIN


void
convert_area_2_area_under_window_constrains( Area2d & a,
                                             int win_size_x,
                                             int win_size_y )
{
    double ratio_x = a.size_x / double( win_size_x );
    double ratio_y = a.size_y / double( win_size_y );
    if ( ratio_x > ratio_y )
    {
        a.size_y = win_size_y * ratio_x;
    }
    else
    {
        a.size_x = win_size_x * ratio_y;
    }
}

void
init_4_tree_and_display()
{
    DDD = new DisplayX11( WIN::disp, WIN::pixmap, WIN::gc );

    DDD->ASetWindowArea( WIN::win_width, WIN::win_height );
    RUN::conv_area.set_win_size( WIN::win_width, WIN::win_height );

    RUN::conv_area.set_area( Options::plane );
    RUN::conv_area.update();
    RUN::conv_area.get_area( Options::plane );

    DDD->ASetPlainArea( Options::plane );
#if 0
    /*****************************************************************************/
    for ( int i = 1; i <= 2; ++i )
    {
        DrawFrame * d = new DrawFrame( i );
        d->layer = i;
        RUN::tree.insert_in_frame( i - 1, d );
    }
    RUN::tree.translate_in_frame( 0, 1, 10.0, 0.0 );
    RUN::tree.translate_in_frame( 1, 2, 0.0, 10.0 );
    std::cout << "\n----";
    std::cout << t;
    std::cout << "\n----";
#endif
}

void
print_event_type( const XEvent & event )
{
    switch( event.type ) {
    case KeyPress        : std::cout << "\n KeyPress        "; break;
    case KeyRelease      : std::cout << "\n KeyRelease      "; break;
    case ButtonPress     : std::cout << "\n ButtonPress     "; break;
    case ButtonRelease   : std::cout << "\n ButtonRelease   "; break;
    case MotionNotify    : std::cout << "\n MotionNotify    "; break;
    case EnterNotify     : std::cout << "\n EnterNotify     "; break;
    case LeaveNotify     : std::cout << "\n LeaveNotify     "; break;
    case FocusIn         : std::cout << "\n FocusIn         "; break;
    case FocusOut        : std::cout << "\n FocusOut        "; break;
    case KeymapNotify    : std::cout << "\n KeymapNotify    "; break;
    case Expose          : std::cout << "\n Expose          "; break;
    case GraphicsExpose  : std::cout << "\n GraphicsExpose  "; break;
    case NoExpose        : std::cout << "\n NoExpose        "; break;
    case VisibilityNotify: std::cout << "\n VisibilityNotify"; break;
    case CreateNotify    : std::cout << "\n CreateNotify    "; break;
    case DestroyNotify   : std::cout << "\n DestroyNotify   "; break;
    case UnmapNotify     : std::cout << "\n UnmapNotify     "; break;
    case MapNotify       : std::cout << "\n MapNotify       "; break;
    case MapRequest      : std::cout << "\n MapRequest      "; break;
    case ReparentNotify  : std::cout << "\n ReparentNotify  "; break;
    case ConfigureNotify : std::cout << "\n ConfigureNotify "; break;
    case ConfigureRequest: std::cout << "\n ConfigureRequest"; break;
    case GravityNotify   : std::cout << "\n GravityNotify   "; break;
    case ResizeRequest   : std::cout << "\n ResizeRequest   "; break;
    case CirculateNotify : std::cout << "\n CirculateNotify "; break;
    case CirculateRequest: std::cout << "\n CirculateRequest"; break;
    case PropertyNotify  : std::cout << "\n PropertyNotify  "; break;
    case SelectionClear  : std::cout << "\n SelectionClear  "; break;
    case SelectionRequest: std::cout << "\n SelectionRequest"; break;
    case SelectionNotify : std::cout << "\n SelectionNotify "; break;
    case ColormapNotify  : std::cout << "\n ColormapNotify  "; break;
    case ClientMessage   : std::cout << "\n ClientMessage   "; break;
    case MappingNotify   : std::cout << "\n MappingNotify   "; break;
    default:               std::cout << "\n UNKNOWN EVENT";
    }
    std::cout << std::flush;
}

void
init_window_hints( Display * disp,
                   Window win )
{
#if 1
    XSizeHints hint;
    XWMHints wmhints;

    //
    wmhints.flags = InputHint;
    wmhints.input = True;
    XSetWMHints(disp, win, &wmhints);

    //
    hint.flags = PMinSize;// | PBaseSize;
    hint.min_width= 400;
    hint.min_height= 260;
    //hint.base_width= 200;
    //hint.base_height= 300;

    XSetWMNormalHints(disp, win, &hint);
#endif

#if 0
    XSizeHints size_hints;
    XWMHints wm_hints;
    XClassHint class_hints;

    size_hints.min_width= 400;
    size_hints.min_height= 260;
    //size_hints.width= Options::window_size_x;
    //size_hints.height= Options::window_size_y;
    size_hints.flags = /*PSize |*/ PMinSize;

    //if (Options::window_left_x || Options::window_top_y) {
    //  size_hints.x = Options::window_left_x;
    //  size_hints.y = Options::window_top_y;
    //  size_hints.flags |= PPosition;
    //}

    //int screen = DefaultScreen(disp);
    size_hints.x = 200;//(XDisplayWidth(disp, screen)-size_hints.width)/2;
    size_hints.y = 20;//(XDisplayWidth(disp, screen)-size_hints.width)/2;
    size_hints.flags |= PPosition;

    wm_hints.initial_state = NormalState;
    wm_hints.input = True;
    //wm_hints.icon_pixmap = icon_pixmap;
    //wm_hints.icon_mask = iconmask_pixmap;
    wm_hints.flags = StateHint | /*IconPixmapHint | IconMaskHint |*/ InputHint;

    class_hints.res_name = PACKAGE;
    class_hints.res_class = PACKAGE_VERSION;


    XTextProperty windowName, iconName;
    XTextProperty * PwindowName= &windowName;
    XTextProperty * PiconName= &iconName;

    char * window_name= PACKAGE_VERSION;
    char * icon_name= PACKAGE;
    if ( ! XStringListToTextProperty(&window_name, 1, &windowName ) )
    {
        PwindowName = NULL;
    }
    if ( ! XStringListToTextProperty(&icon_name, 1, &iconName ) )
    {
        PiconName = NULL;
    }

    XSetWMProperties( disp, win, NULL,NULL,/* PwindowName, PiconName,*/
                      NULL, 0, &size_hints, &wm_hints,
                      &class_hints );

    if ( PwindowName )
    {
        XFree( windowName.value );
    }

    if ( PiconName )
    {
        XFree( iconName.value );
    }
#endif
#if 0
    int x_return, y_return;
    int width_return;
    int height_return;
    int gravity_return;
    char user_geom[] = "+5+0";
    char def_geom[] = "+0+0";
    XWMGeometry( disp,DefaultScreen( disp ), user_geom, def_geom,
                 5, &hint, &x_return, &y_return, &width_return, &height_return, &gravity_return );
    std::cout << "\nx= " << x_return
              << "\ny= " << y_return
              << "\nwidth= " << width_return
              << "\nheight= " << height_return
              << "\ngravity= " << gravity_return << std::flush;
#endif
}

void
init_window_input( Display * disp,
                   Window win )
{
#if 1
    XSelectInput( disp, win,
                  KeyPressMask
                  | KeyReleaseMask
                  | StructureNotifyMask
                  | ExposureMask
                  | ButtonPressMask
                  | ButtonReleaseMask
                  | ButtonMotionMask
                  //| PointerMotionMask  //nur fuer den smonitor Modus, um immer die aktuelle Pointer Position zu wissen, ///doch nicht notwedig event.xkey.{x,y} erreicht das selbe
                  //| Button1PressMask | Button1ReleaseMask | Button1MotionMask
                  //| Button2PressMask | Button2ReleaseMask | Button2MotionMask
                  );
#else
    XSelectInput( disp,win,
                  KeyPressMask
                  | KeyReleaseMask
                  | ButtonPressMask
                  | ButtonReleaseMask
                  | EnterWindowMask
                  | LeaveWindowMask
                  | PointerMotionMask
                  | PointerMotionHintMask
                  | Button1MotionMask
                  | Button2MotionMask
                  | Button3MotionMask
                  | ButtonMotionMask
                  | KeymapStateMask
                  | ExposureMask
                  | VisibilityChangeMask
                  | StructureNotifyMask
                  | ResizeRedirectMask
                  | PropertyChangeMask
                  );
#endif
}

void
XEvent_to_InputEvent( const XEvent & xevent,
                      InputEvent & input_event )
{
    int x = 0;
    int y = 0;
    int len;
    KeySym key;
    static char text[30];
    input_event.init();
    switch ( xevent.type ) {
    case KeyPress:
    case KeyRelease:
        //cout << "\n KeyPress or Release        ";
        x = xevent.xkey.x;
        y = xevent.xkey.y;
        len = XLookupString( (XKeyEvent *)&xevent, text, 10, &key, 0 );
        input_event.key= text[0];
        break;
    case ButtonPress:
        x = xevent.xbutton.x;
        y = xevent.xbutton.y;
        input_event.time = xevent.xbutton.time;
        input_event.mouse_button = xevent.xbutton.button;
        input_event.mouse_button_state = InputEvent::MOUSE_BUTTON_PRESSED;
        //cout << "\nPRESSED " << input_event.time << flush;
        break;
    case ButtonRelease:
        x = xevent.xbutton.x;
        y = xevent.xbutton.y;
        input_event.time = xevent.xbutton.time;
        input_event.mouse_button = xevent.xbutton.button;
        input_event.mouse_button_state = InputEvent::MOUSE_BUTTON_RELEASED;
        //cout << "\nRELEASE " << input_event.time << flush;
        break;
    case MotionNotify:
        x = xevent.xbutton.x;
        y = xevent.xbutton.y;
        input_event.time = xevent.xbutton.time;
        input_event.mouse_button = xevent.xbutton.button;
        input_event.mouse_button_state = InputEvent::MOUSE_BUTTON_MOTION;
        //cout << "\nMOTION " << input_event.time << flush;
        break;
#if 0
    case EnterNotify     : std::cout << "\n EnterNotify     "; break;
    case LeaveNotify     : std::cout << "\n LeaveNotify     "; break;
    case FocusIn         : std::cout << "\n FocusIn         "; break;
    case FocusOut        : std::cout << "\n FocusOut        "; break;
    case KeymapNotify    : std::cout << "\n KeymapNotify    "; break;
    case Expose          : std::cout << "\n Expose          "; break;
    case GraphicsExpose  : std::cout << "\n GraphicsExpose  "; break;
    case NoExpose        : std::cout << "\n NoExpose        "; break;
    case VisibilityNotify: std::cout << "\n VisibilityNotify"; break;
    case CreateNotify    : std::cout << "\n CreateNotify    "; break;
    case DestroyNotify   : std::cout << "\n DestroyNotify   "; break;
    case UnmapNotify     : std::cout << "\n UnmapNotify     "; break;
    case MapNotify       : std::cout << "\n MapNotify       "; break;
    case MapRequest      : std::cout << "\n MapRequest      "; break;
    case ReparentNotify  : std::cout << "\n ReparentNotify  "; break;
    case ConfigureNotify : std::cout << "\n ConfigureNotify "; break;
    case ConfigureRequest: std::cout << "\n ConfigureRequest"; break;
    case GravityNotify   : std::cout << "\n GravityNotify   "; break;
    case ResizeRequest   : std::cout << "\n ResizeRequest   "; break;
    case CirculateNotify : std::cout << "\n CirculateNotify "; break;
    case CirculateRequest: std::cout << "\n CirculateRequest"; break;
    case PropertyNotify  : std::cout << "\n PropertyNotify  "; break;
    case SelectionClear  : std::cout << "\n SelectionClear  "; break;
    case SelectionRequest: std::cout << "\n SelectionRequest"; break;
    case SelectionNotify : std::cout << "\n SelectionNotify "; break;
    case ColormapNotify  : std::cout << "\n ColormapNotify  "; break;
    case ClientMessage   : std::cout << "\n ClientMessage   "; break;
    case MappingNotify   : std::cout << "\n MappingNotify   "; break;
    default:               std::cout << "\n UNKNOWN EVENT";
#endif
    }
    input_event.pos = RUN::conv_area.get_point_from_win_pos( x, y );
}

bool
process_x11_events()
{
    if ( RUN::quit )
    {
        return false;  //ein hack, damit das Abschalten richtig funkioniert!
    }

    bool redraw = false;

    while ( XEventsQueued( WIN::disp, QueuedAfterReading ) )
    {
        bool do_process_key = false;
        char process_key = '\0';
        XEvent event;
        KeySym key;
        char text[30];
        int len;
        Point2d dum_pos;
        bool dum_res;
        int dum1,dum2,dum3,dum4;
        Pixmap dum_pixmap;
        InputEvent input_event;

        XNextEvent( WIN::disp, &event );
        //print_event_type(event);

        if ( WIN::menu->responsible_for_event( event ) )
        {
            dum_res= WIN::menu->process_event( event );
            if ( dum_res )
            { //don't wait for the expose event in the main window, redraw now
                XCopyArea( WIN::disp, WIN::pixmap, WIN::window, WIN::gc,
                           0, 0, WIN::win_width, WIN::win_height, 0, 0 );
                XFlush( WIN::disp );
                //cout << "\nexposing main window after menu event" << flush;
            }

            int dum_button,dum_mouse_button;
            int res= WIN::menu->button_pressed( event,dum_button,dum_mouse_button );
            if ( res )
            {
                XEvent_to_InputEvent( event,input_event );
                input_event.menu_button = dum_button;
                input_event.mouse_button = dum_mouse_button;
                res = INPUTDEV->process_menu_button( &RUN::builder, WIN::menu,input_event );
                if ( WIN::menu->needs_redraw() )
                {
                    redraw= true;
                    WIN::menu->redraw();
                }

                if ( WIN::menu->get_exit_program() )
                {
                    RUN::quit = true;
                }
                redraw = true;
            }
        }
#ifdef POPUP
        else if ( WIN::popup->responsible_for_event( event ) )
        {
            dum_res = WIN::popup->process_event( event );
            if ( dum_res )
            { //don't wait for the expose event in the main window, redraw now
                std::cout << "\nshould never enter this code " << std::flush;
                XCopyArea( WIN::disp, WIN::pixmap, WIN::window, WIN::gc,
                           0, 0, WIN::win_width, WIN::win_height, 0, 0 );
                XFlush( WIN::disp );
                //cout << "\nexposing main window after menu event" << flush;
            }

            int dum_button,dum_mouse_button;
            int res = WIN::popup->button_pressed( event,dum_button, dum_mouse_button );

            if ( res )
            {
                //cout << "\npressed button " << dum_button << " mouse_button " << dum_mouse_button << flush;
                XEvent_to_InputEvent( event,input_event );
                input_event.menu_button = dum_button;

                res = INPUTDEV->process_popup_button( &RUN::builder, WIN::popup, input_event );
                //if (WIN::popup->needs_redraw() ) {
                //  redraw= true;
                //  WIN::popup->redraw();
                //}

                WIN::popup->set_popup_invisible();
                WIN::popup->get_rectangle( dum1, dum2, dum3, dum4 );
                XCopyArea( WIN::disp, WIN::pixmap, WIN::window, WIN::gc,
                           dum1, dum2, dum3 + 2, dum4 + 2, dum1, dum2 );

                XFlush( WIN::disp );
                WIN::mouse_button = 0;
            }
        }
#endif
        else
        {
            switch ( event.type ) {
            case MappingNotify:
                XRefreshKeyboardMapping( (XMappingEvent *)&event );
                break;
            case Expose:
                if ( 0 == event.xexpose.count )
                {
                    //cout << "\nexpose in draw window" << flush;
                    //don't need to set redraw= true; because the picture is still in our pixmap!!!
                    XCopyArea( WIN::disp, WIN::pixmap, WIN::window, WIN::gc,
                               0, 0, WIN::win_width, WIN::win_height, 0, 0 );
                    XFlush( WIN::disp );
                    //cout << "\nexposing main window, redraw= " << redraw << flush;
                    //redraw= true;
                }
                break;
            case ConfigureNotify:
                if ( WIN::win_width != event.xconfigure.width
                     || WIN::win_height != event.xconfigure.height )
                {
                    redraw = true;
                    WIN::win_width = event.xconfigure.width;
                    WIN::win_height = event.xconfigure.height;//-letheight-1;
                    WIN::clip_rect.set_ratio( WIN::win_width, WIN::win_height );

                    RUN::conv_area.set_win_size( WIN::win_width, WIN::win_height );
#if 0
                    RUN::conv_area.update();
                    RUN::conv_area.get_area( dum_area );
                    DDD->ASetPlainArea( dum_area );
#endif
                    DDD->ASetWindowArea( WIN::win_width, WIN::win_height );

                    dum_pixmap = WIN::pixmap;
                    //set new size for the pixmap
                    WIN::pixmap = XCreatePixmap( WIN::disp, WIN::window,
                                                 WIN::win_width, WIN::win_height, WIN::win_depth );
                    //XFillRectangle(WIN::disp, WIN::pixmap, WIN::bg_gc, 0, 0, WIN::win_width, WIN::win_height);
                    DDD->ASetNewPixmap( WIN::pixmap );
                    XFreePixmap( WIN::disp, dum_pixmap );

                    WIN::menu->resize( WIN::win_width );
                }
                break;
            case  ButtonPress: //speichere 1 Koordinate
                //cout << "\nxbutton: state= " << event.xbutton.state << ", button= " << event.xbutton.button << flush;
                if (event.xany.window != WIN::window) //mouse event should be in the same window
                    break;

                if (WIN::mouse_button) //if a button is already down, don't process further button events
                    break;

                WIN::mouse_button= event.xbutton.button; //store the currently pressed mouse button

                //if (event.xbutton.state & Button2Mask)
                if (event.xbutton.button == 1) {
                    WIN::clip_rect.set_origin(event.xbutton.x,event.xbutton.y);
                    WIN::clip_rect.active= true;
                    WIN::rect_was_drawn= false;
                }
                else if (event.xbutton.button == 2 || event.xbutton.button == 3) {
                    XEvent_to_InputEvent(event,input_event);

                    if ( INPUTDEV->uses_mouse() ) {
                        dum_res= INPUTDEV->process_mouse_button_event(&RUN::builder, input_event);
                        redraw= dum_res || redraw;
                    }
                    if ( INPUTDEV->uses_popup() ) {
                        WIN::popup->set_popup_visible( event.xbutton.x, event.xbutton.y, WIN::win_width, WIN::win_height);
                        dum_res= INPUTDEV->process_popup_button(&RUN::builder,WIN::popup,input_event);
                        redraw= dum_res || redraw;
                    }
                }
                break;
            case ButtonRelease: //speichere 2 Koordinate
                if (event.xany.window != WIN::window)
                    break;
                if (WIN::mouse_button != event.xbutton.button) {//just process the originaly pressed button
                    //cerr << "\nold button= " << WIN::mouse_button << "!=" << event.xbutton.button << " = new button -> break;";
                    break;
                }
                //cerr << "\nbutton released";
                WIN::mouse_button= 0;

                if ( event.xbutton.button == 1 )
                {
                    if ( WIN::rect_was_drawn )
                    { //remove old rectangle
                        XDrawRectangle( WIN::disp, WIN::window, WIN::xor_gc,
                                        WIN::clip_rect.x, WIN::clip_rect.y,
                                        WIN::clip_rect.width, WIN::clip_rect.height );
                        XFlush( WIN::disp );
                        WIN::rect_was_drawn = false;
                    }
                    WIN::clip_rect.set_point( event.xbutton.x, event.xbutton.y );
                    WIN::clip_rect.active= false;
                    if ( WIN::clip_rect.width == 0 || WIN::clip_rect.height == 0 )
                    {
                        redraw = true;
                        RUN::conv_area.change_area_using_win_pos( WIN::clip_rect.x, WIN::clip_rect.y );
                        std::cerr << "\nnew center";
                        break;
                    }
                    if ( WIN::clip_rect.width < 20 || WIN::clip_rect.height < 20 )
                    {
                        std::cerr << "\nregion too small";
                        break;
                    }
                    RUN::conv_area.change_area_using_subwin( WIN::clip_rect.x, WIN::clip_rect.y,
                                                             WIN::clip_rect.x + WIN::clip_rect.width,
                                                             WIN::clip_rect.y + WIN::clip_rect.height );
                    redraw = true;
                }
                else if ( event.xbutton.button == 2 || event.xbutton.button == 3 )
                {
                    XEvent_to_InputEvent( event,input_event );
                    if ( INPUTDEV->uses_mouse() )
                    {
                        dum_res = INPUTDEV->process_mouse_button_event( &RUN::builder, input_event );
                        redraw = ( dum_res || redraw );
                    }
                    WIN::popup->set_popup_invisible();
                    if ( INPUTDEV->uses_popup() )
                    {
                        WIN::popup->get_rectangle( dum1, dum2, dum3, dum4 );
                        XCopyArea( WIN::disp, WIN::pixmap, WIN::window, WIN::gc,
                                   dum1, dum2, dum3 + 2, dum4 + 2, dum1, dum2 );
                        XFlush( WIN::disp );
                        //cout << "\nXFlush BUTTON REALESSED" << flush;
                    }
                }
                break;
            case MotionNotify:
                while ( XCheckMaskEvent( WIN::disp,PointerMotionMask | ButtonMotionMask, &event ) )
                    ; //nimmt nur das letzte Event vom Typ ButtonMotionMask, alle anderen werden verworfen
                //cout << event.xmotion.x << "," << event.xmotion.y << endl;
                if ( WIN::mouse_button == 1 )
                {
                    WIN::mouse_point = Pointer( event.xmotion.x, event.xmotion.y );
                    if ( ! WIN::clip_rect.active )
                    {
                        break;
                    }

                    if ( WIN::rect_was_drawn )
                    { // remove old rectangle
                        XDrawRectangle( WIN::disp, WIN::window, WIN::xor_gc,
                                        WIN::clip_rect.x, WIN::clip_rect.y,
                                        WIN::clip_rect.width, WIN::clip_rect.height );
                    }
                    //draw new rectangle
                    WIN::clip_rect.set_point( WIN::mouse_point.x, WIN::mouse_point.y );

                    XDrawRectangle( WIN::disp, WIN::window, WIN::xor_gc,
                                    WIN::clip_rect.x, WIN::clip_rect.y,
                                    WIN::clip_rect.width, WIN::clip_rect.height );
                    WIN::rect_was_drawn = true;
                }
                else if ( WIN::mouse_button == 2 )
                {
                    XEvent_to_InputEvent( event,input_event );
                    //dum_pos= RUN::conv_area.get_point_from_win_pos( event.xbutton.x, event.xbutton.y );
                    dum_res = INPUTDEV->process_mouse_button_event( &RUN::builder, input_event );
                    redraw = dum_res || redraw;
                }
                else if ( WIN::mouse_button == 3 )
                {
                    XEvent_to_InputEvent( event, input_event );
                    //dum_pos= RUN::conv_area.get_point_from_win_pos( event.xbutton.x, event.xbutton.y );
                    dum_res= INPUTDEV->process_mouse_button_event( &RUN::builder, input_event );
                    redraw = ( dum_res || redraw );
                }
                break;
            case KeyPress:
                //cout << "\nkey pressed" << flush;
                len = XLookupString( (XKeyEvent *)&event, text, 10, &key, 0 );
                switch (key) {  /* ART! */
                    /*
                      case XK_Left:  t.translate_in_frame(0,I.actframe,-5.0,  0.0); break;
                      case XK_Right: t.translate_in_frame(0,I.actframe, 5.0,  0.0); break;
                      case XK_Up:    t.translate_in_frame(0,I.actframe, 0.0,  5.0); break;
                      case XK_Down:  t.translate_in_frame(0,I.actframe, 0.0, -5.0); break;
                    */
                case XK_Left:  RUN::conv_area.move_area_dir_x_using_factor( -0.25 ); redraw = true; break;
                case XK_Right: RUN::conv_area.move_area_dir_x_using_factor(  0.25 ); redraw = true; break;
                case XK_Up:    RUN::conv_area.move_area_dir_y_using_factor(  0.25 ); redraw = true; break;
                case XK_Down:  RUN::conv_area.move_area_dir_y_using_factor( -0.25 ); redraw = true; break;
                }
                if ( len != 1 ) break;
                switch ( text[0] ) {
                case 'R':
                    redraw = true;
                    //RUN::tree.rotate_frame(I.actframe,-M_PI/4);
                    break;
                case '+': redraw = true; RUN::conv_area.scale_area_using_factor( 3.0/4.0 ); break;
                case '-': redraw = true; RUN::conv_area.scale_area_using_factor( 4.0/3.0 ); break;
                case 'f': RUN::toggle_freeze(); break;
                case 'i':
                    redraw = true;
                    RUN::conv_area.set_area( Options::plane );
                    break;
                case 'k':
                    Options::show_key_bindings( std::cerr );
                    break;
                case 'q':
                    RUN::quit = true;
                    redraw = false;
                    break;
                }
                dum_pos = RUN::conv_area.get_point_from_win_pos( event.xkey.x, event.xkey.y );
                do_process_key = true;
                process_key = text[0];
                break;
            case KeyRelease:
                len = XLookupString( (XKeyEvent *)&event, text, 10, &key, 0 );
                if ( len != 1 ) break;
                switch ( text[0] ) {
                case 'q':
                    RUN::quit = true;
                    redraw = false;
                    break;
                }
                break;
            }
        }

        if ( do_process_key )
        {
            XEvent_to_InputEvent( event, input_event );
            dum_res = INPUTDEV->process_char_command( &RUN::builder, WIN::menu, input_event );
            if ( WIN::menu->needs_redraw() )
            {
                WIN::menu->redraw();
            }
            redraw= dum_res || redraw;
        }
    }

    return redraw;
}

void
redraw_current_tree()
{
    Area2d area;
    //cout << "\nredrawing main window\n--" << flush;
    if ( RUN::conv_area.needs_update() )
    {
        RUN::conv_area.update();
        RUN::conv_area.get_area( area );
        DDD->ASetPlainArea( area );
    }
    else
    {
        RUN::conv_area.get_area( area );
    }

    /* erase objects	*/
    XFillRectangle(WIN::disp, WIN::pixmap, WIN::bg_gc, 0, 0, WIN::win_width, WIN::win_height);

    //RUN::tree.draw(area);
    RUN::tree.draw(DDD,area);

    const char* dum;
    int dum_len;
    if ( RUN::builder.status_line )
    {
        dum = RUN::builder.status_line;
        dum_len = RUN::builder.status_line_len;
        //staus line should persist RUN::builder.status_line= 0;
    }
    else
    {
        dum = INPUTDEV->status_line();
        dum_len= std::strlen( dum );
    }
    if ( dum_len )
    {
        XDrawImageString( WIN::disp, WIN::pixmap, WIN::sl_gc,
                          0, WIN::win_height - 2,
                          dum, dum_len );
    }

    /* update display:	*/
    XCopyArea( WIN::disp, WIN::pixmap, WIN::window, WIN::gc,
               0, 0, WIN::win_width, WIN::win_height, 0, 0 );

    if ( WIN::rect_was_drawn )//( WIN::clip_rect.active)
    {
        XDrawRectangle( WIN::disp, WIN::window, WIN::xor_gc,
                        WIN::clip_rect.x, WIN::clip_rect.y,
                        WIN::clip_rect.width, WIN::clip_rect.height );
    }

    XFlush(WIN::disp);
}

int
main( int argc, char ** argv )
{
    TOOLS::get_current_ms_time(); //just init the starting time of this application;

    std::cout << "Copyright (c) 1999 - 2001, Artur Merke <amerke@ira.uka.de>"
              << "\nCopyright (c) 2001 - 2008, The RoboCup Soccer Server Maintainance Group."
              << "\n\t<sserver-admin@lists.sourceforge.net>";

    --argc; ++argv;
    bool smonitor_dev= true;
    if ( argc > 0
         && ! std::strncmp( argv[0], "-ascii", std::strlen( "-ascii" ) ) )
    {
        argc--; argv++;
        smonitor_dev= false;
    }

    if ( smonitor_dev )  //standard device
    {
        INPUTDEV = new SMonitorDevice();

        RUN::conv_area.set_min_size_x( 10.0 );
        RUN::conv_area.set_min_size_y( 10.0 );
        double extent= 200;
        RUN::conv_area.set_max_size_x( extent );
        RUN::conv_area.set_max_size_y( extent );

        RUN::conv_area.set_min_pos_x( -0.5*extent );
        RUN::conv_area.set_min_pos_y( -0.5*extent );
        RUN::conv_area.set_max_pos_x( 0.5*extent );
        RUN::conv_area.set_max_pos_y( 0.5*extent );
    }
    else
    {
        INPUTDEV = new AsciiDevice( 6010 );
    }

    Options::set_defaults();
    INPUTDEV->set_defaults();

    const char * HOME = getenv( "HOME" );
    if ( std::strlen( HOME ) + std::strlen( PACKAGE ) + std::strlen( ".conf" ) + 3
         < MAX_NAME_LEN )
    {
        std::strcpy( Options::conf_file, HOME );
        std::strcat( Options::conf_file, "/." );
        std::strcat( Options::conf_file, PACKAGE );
        std::strcat( Options::conf_file, ".conf" );
        //test for existence of Options::conf_file
        std::ifstream is( Options::conf_file );
        if ( ! is )
        {
            std::ofstream out( Options::conf_file );
            if ( ! out )
            {
                ERROR_OUT << "\ncouldn't open " << Options::conf_file << " to create a conf file";
                out.close();
            }
            else
            {
                Options::generate_file_options( out, PACKAGE );
                std::cout << "\ncreated config file, " << Options::conf_file;
            }
            Options::conf_file[0]= '\0';
        }
        is.close();
    }
    else
    {
        ERROR_OUT << "\nHOME path too long";
    }

    Options::read( argc, argv );
    std::cout << std::flush;

    INPUTDEV->set_initial_area( Options::plane );
    /////////////////////////////////////
    Colormap cmap;
    Cursor cursor;
    Font font;
    Font font_inside;

    XSizeHints hint;

    int screen;

    unsigned long fg, bg;

    WIN::disp = XOpenDisplay( 0 );
    if ( WIN::disp == (Display *)NULL )
    {
        ERROR_OUT << "\nCould not open display!";
        return 1;
    }
    WIN::x11_fd = XConnectionNumber( WIN::disp );
    RGB_DB::disp = WIN::disp;
    //cout << "\n connection fd= " << WIN::x11_fd << flush;
    screen = DefaultScreen( WIN::disp );
    fg = BlackPixel( WIN::disp, screen );
    bg = WhitePixel( WIN::disp, screen );
    //hint.x = 150; hint.y = 10;
    hint.x = 200; hint.y = 20;
    hint.width = Options::window_size_x;
    hint.height = Options::window_size_y;
    hint.flags = PPosition | PSize;
    WIN::win_width = hint.width; WIN::win_height = hint.height;//-letheight-1;
    WIN::win_depth = DefaultDepth( WIN::disp, screen );
    WIN::window = XCreateSimpleWindow( WIN::disp, DefaultRootWindow(WIN::disp),
                                       hint.x, hint.y, hint.width, hint.height, 5, fg, bg );
    WIN::pixmap = XCreatePixmap( WIN::disp, WIN::window, WIN::win_width, WIN::win_height, WIN::win_depth );
    XSetStandardProperties( WIN::disp, WIN::window,
                            PACKAGE_VERSION,
                            PACKAGE,
                            None, //WIN::icon_pixmap,
                            argv, argc, &hint );

    WIN::gc = XCreateGC( WIN::disp, WIN::window, 0, 0 );
    XSetGraphicsExposures( WIN::disp, WIN::gc, 0 );	/* IMPORTANT!  If you do not
                                                       specifically ask not to get Expose events, every XCopyArea
                                                       will generate one, & the event queue will fill up.	*/
    XSetLineAttributes( WIN::disp, WIN::gc,
                        Options::line_thickness, LineSolid,
                        CapButt, JoinMiter );
    //XCopyGC( WIN::disp,WIN::gc,0,WIN::xor_gc );

    WIN::xor_gc = XCreateGC( WIN::disp, WIN::window, 0, 0 );
    //XSetFunction(WIN::disp,WIN::xor_gc,GXxor);
    XSetFunction( WIN::disp, WIN::xor_gc, GXinvert );
    //XSetPlaneMask(WIN::disp,WIN::xor_gc,3); //wichtig, aber warum? ;-)

    font = XLoadFont( WIN::disp, Options::font_name );
    font_inside = XLoadFont( WIN::disp,Options::font_inside );
    //font = XLoadFont(WIN::disp, "10x20\0");	/* If you don't have this font, try replacing it with 9x15\0	*/
    XSetFont( WIN::disp, WIN::gc, font_inside );

    WIN::bg_gc = XCreateGC( WIN::disp, WIN::window, 0, 0 );
    WIN::sl_gc = XCreateGC( WIN::disp, WIN::window, 0, 0 );
    WIN::bt_gc = XCreateGC( WIN::disp, WIN::window, 0, 0 );
    XSetBackground( WIN::disp, WIN::gc, bg );
    XSetForeground( WIN::disp, WIN::gc, fg );
    XSetBackground( WIN::disp, WIN::xor_gc, bg );
    XSetForeground( WIN::disp, WIN::xor_gc, fg );
    XSetForeground( WIN::disp, WIN::bg_gc, bg );  /* fg of pixmap is bg of window */
    XSetBackground( WIN::disp, WIN::bg_gc, fg );

    XSetForeground( WIN::disp, WIN::sl_gc, bg );
    XSetBackground( WIN::disp, WIN::sl_gc, fg );

    XSetForeground( WIN::disp, WIN::bt_gc, fg );
    XSetBackground( WIN::disp, WIN::bt_gc, bg );
    XSetFont( WIN::disp, WIN::bg_gc, font );
    XSetFont( WIN::disp, WIN::sl_gc, font );

    init_window_hints( WIN::disp,WIN::window );
    init_window_input( WIN::disp,WIN::window );
#if 0
    XMoveWindow( WIN::disp, WIN::window, 10, 10 );
    XWindowChanges dum;
    dum.x = 10;
    dum.y = 10;
    XConfigureWindow( WIN::disp, WIN::window, CWX | CWY, &dum );
#endif

    XMapRaised( WIN::disp, WIN::window );
    //XMapWindow( WIN::disp, WIN::window );

#if 0
    if ( Options::set_window_pos )
    {
        XMoveWindow( WIN::disp, WIN::window, Options::window_left_x, Options::window_top_y );
    }
#endif
    /* Erase cursor. Just delete next 5 lines if any error.	*/
    cmap = XDefaultColormap( WIN::disp, screen );
#if 1
    //XAllocNamedColor(WIN::disp, cmap, "Black", &exact, &black);
    cursor = XCreateFontCursor( WIN::disp, XC_crosshair );
    //XRecolorCursor(WIN::disp, cursor, &black, &black);
    XDefineCursor( WIN::disp, WIN::window, cursor );
#endif
    //XFillRectangle (WIN::disp, WIN::pixmap, WIN::bg_gc, 0, 0, WIN::win_width, WIN::win_height);
    {
        XpmCreatePixmapFromData( WIN::disp, WIN::window,
				 const_cast< char** >( rcssmonitor_xpm ),
                                 &WIN::icon_pixmap,
                                 &WIN::icon_mask,
                                 NULL );
        XWMHints * h = XGetWMHints( WIN::disp, WIN::window );
        XWMHints wm_hints;
        bool got_hints = ( h != 0 );
        if ( ! got_hints )
        {
            h = &wm_hints;
            h->flags = 0;
        }

        h->icon_pixmap = WIN::icon_pixmap;
        h->icon_mask = WIN::icon_mask;
        h->flags |= IconPixmapHint | IconMaskHint;
        XSetWMHints( WIN::disp, WIN::window, h );

        if ( got_hints )
        {
            XFree( (char *)h );
        }
    }

    /////////////////////
    // View Converter init
    init_4_tree_and_display();

    //init the menu
    WIN::menu= new MenuX11(WIN::disp,WIN::window,WIN::bt_gc,WIN::win_width,15);
    WIN::menu->set_background_color(RGBcolor(0xcc,0xcc,0xcc));
    WIN::menu->set_foreground_color(RGBcolor(0,0,0));
    if ( INPUTDEV->init_menu( WIN::menu ) )
    {
        WIN::menu->redraw();
    }

#ifdef POPUP
    //init the popup
    WIN::popup= new PopupX11(WIN::disp,WIN::window,WIN::bt_gc);
    WIN::popup->set_background_color(RGBcolor(0xcc,0xcc,0xcc));
    WIN::popup->set_foreground_color(RGBcolor(0,0,0));
    WIN::popup->set_highlight_color(RGBcolor(0x99,0x99,0x99));
    WIN::popup->set_popup_invisible();
    INPUTDEV->init_popup(WIN::popup);
#endif

    //init frames
    INPUTDEV->init_frames( &RUN::builder );
    if ( RUN::conv_area.needs_update() )
    {
        RUN::conv_area.update();
        RUN::conv_area.get_area( Options::plane ); //this area will be restored when 'i' is pressed!
        DDD->ASetPlainArea( Options::plane );
    }

    if ( RUN::builder.new_bg_color )
    {
        RUN::builder.new_bg_color = false;
        XSetForeground( WIN::disp, WIN::bg_gc, DDD->AGetColor( RUN::builder.bg_color ) );
        DDD->set_background_color( RUN::builder.bg_color );
        //WIN::menu->set_background_color( RUN::builder.bg_color );
    }

    redraw_current_tree();
    WIN::menu->redraw();
    WIN::clip_rect.set_ratio(WIN::win_width,WIN::win_height);

    INPUTDEV->init_connection();

    bool received = false;
    int nodata_seconds = 0;
    int timeover_seconds = 0;
    std::time_t last_receive_time = std::time( NULL );

    //Gezieltes Abfragen der sockets
    fd_set rfds;
    struct timeval tv;
    int retval;

    if ( FD_SETSIZE < 4 )
    {
        std::cerr << "\nFD_SETSIZE < 4";
    }

    /* Watch stdin (fd 0) to see when it has input. */
    while ( ! RUN::quit )
    {
        FD_ZERO( &rfds );
        //FD_SET(0, &rfds); //Standardeingabe
        FD_SET( WIN::x11_fd, &rfds );
        int num = INPUTDEV->set_fds( &rfds );
        int max_fd_plus_1 = std::max( WIN::x11_fd, num ) + 1;

        /* Wait up to five seconds. */
        tv.tv_sec = 5; tv.tv_usec = 0;

        retval = select( max_fd_plus_1, &rfds, NULL, NULL, &tv );
        /* Don't rely on the value of tv now! */

        if ( retval > 0 )
        {
            bool redraw = false;
            nodata_seconds = 0;
            if ( FD_ISSET( WIN::x11_fd, &rfds ) )
            {
                if ( process_x11_events() )
                {
                    redraw = true;
                }
            }

            if ( INPUTDEV->got_fds( &rfds ) )
            {
                if ( INPUTDEV->process_input( &rfds, &RUN::builder ) )
                {
                    received = true;
                    if ( ! RUN::freeze )
                    {
                        redraw= true;
                    }
                }

                if ( RUN::builder.new_bg_color )
                {
                    RUN::builder.new_bg_color = false;
                    XSetForeground( WIN::disp, WIN::bg_gc,
                                    DDD->AGetColor( RUN::builder.bg_color ) );
                    DDD->set_background_color( RUN::builder.bg_color );
                    //WIN::menu->set_background_color( RUN::builder.bg_color );
                    redraw = true;
                }
            }

            if ( redraw )
            {
                redraw_current_tree();
            }

            if ( Options::auto_quit_mode )
            {
                if ( INPUTDEV->is_timeover() )
                {
                    std::time_t current;
                    std::time( &current );
                    timeover_seconds = static_cast< int >( std::difftime( current, last_receive_time ) );
                    if ( timeover_seconds >= Options::auto_quit_wait )
                    {
                        std::cout << "\n Game is over." << std::endl;
                        RUN::quit= true;
                    }
                }
                else
                {
                    std::time( &last_receive_time );
                }
            }
        }
        else if ( retval == 0 )
        {
            //cout << "No Data is available now." << endl;
            nodata_seconds += 5;
            if ( Options::auto_reconnect_mode )
            {
                if ( nodata_seconds + timeover_seconds >= Options::auto_reconnect_wait )
                {
                    INPUTDEV->reconnect();
                }
            }
            else if ( Options::auto_quit_mode )
            {
                if ( nodata_seconds + timeover_seconds >= Options::auto_quit_wait )
                {
                    if ( received )
                    {
                        std::cout << "\nNo messeges from server." << std::endl;
                        RUN::quit = true;
                    }
                }
            }
        }
        else
        {
#if 1
            ERROR_OUT << "\nselect: error occured";
            switch ( errno ) {
            case EBADF:
                ERROR_STREAM << "\n an  invalid file descriptor was given in one of the sets";
                break;
            case EINTR:
                ERROR_STREAM << "\n a non blocked signal was caught";
                break;
            case EINVAL:
                ERROR_STREAM << "\n n is negative";
                break;
            case ENOMEM:
                ERROR_STREAM << "\n select was unable to allocate memory for internal tables";
                break;
            default:
                ERROR_STREAM << "\n error code " << errno;
            }
#endif
        }
    }

    XFreeGC( WIN::disp, WIN::gc );
    XFreeGC( WIN::disp, WIN::xor_gc );
    XFreeGC( WIN::disp, WIN::bg_gc );
    XFreeGC( WIN::disp, WIN::sl_gc );
    XFreePixmap( WIN::disp, WIN::pixmap );
    XFreePixmap( WIN::disp, WIN::icon_pixmap );
    XFreePixmap( WIN::disp, WIN::icon_mask );

    INPUTDEV->destruct();
    delete INPUTDEV;

    delete WIN::menu;
    delete DDD;

    XDestroyWindow (WIN::disp, WIN::window);
    XCloseDisplay (WIN::disp);

    ///
    long t = TOOLS::get_current_ms_time() / 1000;
    std::cerr << "\n" << PACKAGE << "-" << VERSION << " was active for ";
    if ( t / 3600 ) std::cerr << t / 3600 << "h ";
    if ( t / 60 )   std::cerr << ( t / 60 ) % 60   << "m ";
    std::cerr << t % 60 << "s\n";
    return 0;
}
