// -*-c++-*-

/***************************************************************************
                                  options.cpp
                           Class for global options
                             -------------------
    begin                : 2009-04-02
    copyright            : (C) 2009 by The RoboCup Soccer Server
                           Maintenance Group.
    email                : sserver-admin@lists.sourceforge.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU GPL as published by the Free Software  *
 *   Foundation; either version 3 of the License, or (at your option) any  *
 *   later version.                                                        *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "options.h"

#include "input_dev.h"
#include "valueparser.h"

#include <fstream>
#include <cstdlib>
#include <cstring>


Options &
Options::instance()
{
    static Options s_instance;
    return s_instance;
}


Options::Options()
{
    set_defaults();
}


bool
Options::set_defaults()
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


void
Options::show_copyright_notice( std::ostream & os )
{
    os << "\n"
       << "/*****************************************************************************/\n"
       << '\n'
       << "                   "<< PACKAGE << "-" << VERSION << '\n'
       << '\n'
       << "Copyright (c) 1999 - 2001, Artur Merke\n"
       << "\t<amerke@ira.uka.de>\n"
       << "Copyright (c) 2001 - 2009, The RoboCup Soccer Simulator Maintainance Group.\n"
       << "\t<sserver-admin@lists.sourceforge.net>\n"
       << "\n"
       << PACKAGE << "-" << VERSION << " is free software; you can redistribute it and/or modify\n"
       << "it under the terms of the GNU General Public License as published by\n"
       << "the Free Software Foundation; either version 3, or (at your option)\n"
       << "any later version.\n"
       << '\n'
       << PACKAGE << "-" << VERSION << " is distributed in the hope that it will be useful,\n"
       << "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
       << "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
       << "GNU General Public License for more details.\n"
       << "/*****************************************************************************/\n";
}


bool
Options::read( int argc,
               char ** argv,
               InputDevice * input_dev )
{
    bool res = true;
    bool dum = true;

    ValueParser cl( argc, argv );
    //cl.set_verbose( true );

    if ( cl.get( "help", dum ) >= 0
         || cl.get( "h", dum ) >= 0 )
    {
        show_available_options( std::cout, PACKAGE, input_dev );
        std::exit( 0 );
    }

    if ( cl.get( "gen_conf_file", dum ) >= 0 )
    {
        generate_file_options( std::cout, PACKAGE, input_dev );
        std::exit( 0 );
    }

    if ( cl.get( "keys", dum ) >= 0 )
    {
        show_key_bindings( std::cout, input_dev );
        std::exit( 0 );
    }

    if ( cl.get( "copyleft", dum ) >= 0 )
    {
        show_copyright_notice( std::cout );
        std::exit( 0 );
    }

    //
    // create default conf file
    //
    createConfFile( input_dev );

    //
    // read specified conf file
    //
    cl.get( "conf_file", conf_file, MAX_STR_LEN );
    if ( std::strlen( conf_file ) )
    {
        std::cout << "reading options from file: " << conf_file << std::endl;;
        ValueParser vp( conf_file );
        //vp.set_verbose(true);
        process_options( vp );
        input_dev->process_options( conf_file );
    }

    process_options( cl );
    input_dev->process_options( argc, argv );
    //std::cout << "End process_options" << std::endl;
    return res;
}


bool
Options::createConfFile( const InputDevice * input_dev )
{
    //
    // read or generate the default conf file.
    //
    const char * HOME = std::getenv( "HOME" );
    if ( std::strlen( HOME ) + std::strlen( PACKAGE ) + std::strlen( ".conf" ) + 3
         < MAX_STR_LEN )
    {
        std::strcpy( conf_file, HOME );
        std::strcat( conf_file, "/." );
        std::strcat( conf_file, PACKAGE );
        std::strcat( conf_file, ".conf" );
        //test for existence of Options::instance().conf_file
        std::ifstream is( conf_file );
        if ( ! is )
        {
            std::ofstream out( conf_file );
            if ( ! out )
            {
                std::cerr << "couldn't open " << conf_file << " to create a conf file"
                          << std::endl;
                out.close();
            }
            else
            {
                generate_file_options( out, PACKAGE, input_dev );
                std::cout << "created config file, " << conf_file << std::endl;;
            }
            conf_file[0] = '\0';
        }
        is.close();
    }
    else
    {
        std::cerr << "HOME path too long" << std::endl;;
    }

    return true;
}


void
Options::generate_file_options( std::ostream & o,
                                const char * fname,
                                const InputDevice * input_dev ) const
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
    input_dev->generate_file_options( o );
}


void
Options::show_key_bindings( std::ostream & o,
                            const InputDevice * input_dev ) const
{
    o << "\n"
      << "/*****************************************************************************/\n"
      << "\n"
      << "                   "<< PACKAGE << "-" << VERSION << '\n'
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
    input_dev->help_char_command(o);
    o << "\n"
      << "\n"
      << "/*****************************************************************************/\n";
}


bool
Options::process_options( ValueParser & vp )
{
    vp.get( "plane_origin_x", plane.center.x );
    vp.get( "plane_origin_y", plane.center.y );
    vp.get( "plane_size_x", plane.size_x );
    vp.get( "plane_size_y", plane.size_y );

#if 0
    if ( vp.get( "window_left_x", window_left_x ) > 0 )
    {
        set_window_pos= true;
    }

    if ( vp.get( "window_top_y", window_top_y ) > 0 )
    {
        set_window_pos= true;
    }
#endif

    vp.get( "window_size_x", window_size_x );
    vp.get( "window_size_y", window_size_y );

    int tmp = line_thickness;
    vp.get( "line_thickness", tmp );
    if ( tmp < 4 && tmp > 0 ) line_thickness = tmp;

    vp.get( "font_name", font_name, MAX_STR_LEN );
    vp.get( "font_inside", font_inside, MAX_STR_LEN );

    vp.get( "auto_quit_mode", auto_quit_mode );
    vp.get( "auto_quit_wait", auto_quit_wait );

    vp.get( "auto_reconnect_mode", auto_reconnect_mode );
    vp.get( "auto_reconnect_wait", auto_reconnect_wait );
    return true;
}


void
Options::show_available_options( std::ostream & o,
                                 const char * fname,
                                 const InputDevice * input_dev ) const
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

    input_dev->help_options( o );
    o << "\n/*****************************************************************************/"
      << "\n";
}
