// -*-c++-*-

/***************************************************************************
                                  options.h
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

#ifndef RCSSMONITOR_OPTIONS_H
#define RCSSMONITOR_OPTIONS_H

#include "global_defs.h"
#include "area2d.h"

#include <iostream>

class InputDevice;
class ValueParser;

class Options {
public:

    enum {
        MAX_STR_LEN = 512,
    };

    //specifies the clip of euclidean plane
    Area2d plane;

    bool set_window_pos;

    int window_left_x;
    int window_top_y;

    int window_size_x;
    int window_size_y;

    int line_thickness;
    char font_name[MAX_STR_LEN];
    char font_inside[MAX_STR_LEN];

    bool auto_quit_mode;
    int auto_quit_wait;

    bool auto_reconnect_mode;
    int auto_reconnect_wait;

private:
    char conf_file[MAX_STR_LEN];

    // private for singleton
    Options();

    // not used
    Options( const Options & );
    Options & operator=( const Options & );


    bool set_defaults();
    void show_copyright_notice( std::ostream & os );

public:

    static
    Options & instance();


    bool createConfFile( const InputDevice * input_dev );
    bool read( int argc,
               char ** argv,
               InputDevice * input_dev );

    void generate_file_options( std::ostream & os,
                                const char * fname,
                                const InputDevice * input_dev ) const;
    void show_key_bindings( std::ostream & o,
                            const InputDevice * input_dev ) const;


private:
    bool process_options( ValueParser & vp );
    void show_available_options( std::ostream & os,
                                 const char * fname,
                                 const InputDevice * input_dev ) const;
};


#endif
