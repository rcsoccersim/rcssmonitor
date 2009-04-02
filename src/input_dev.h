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
#ifndef INPUT_DEV_H
#define INPUT_DEV_H

// #include "builder_base.h"
#include "menu_base.h"
#include "object2d.h"

#include <sys/types.h> /* fd_set */

class Area2d;
class BuilderBase;

/**
   \short device event information

   \param button is in {2,3},
   0 is not supported for efficiency reasons
   (don't want to receive motion events if no button is pressed!)
   1 is already used by frameview itself

   \param type is one of MOUSE_BUTTON_*

   \param time is the time in milliseconds when the event occured

*/

struct InputEvent {
    enum {
        MOUSE_BUTTON_PRESSED = 1,
        MOUSE_BUTTON_RELEASED = 2,
        MOUSE_BUTTON_MOTION = 3,
    };

    Point2d pos;
    unsigned long time;

    /// is -1 if no button was pressed
    int menu_button;

    /// is '\0' if no key was pressed
    char key;
    int mouse_button;
    int mouse_button_state;

    void init()
      {
          time = 0;
          pos = Point2d();
          menu_button = -1;
          mouse_button = -1;
      }

    InputEvent()
      {
          init();
      }
};


/** \short base class for all devices
    To include a new device you have to write a subclass of this InputDevice.
*/

class InputDevice {

protected:
    void add_fd_to_set( int fd, fd_set * set )
      {
          FD_SET( fd, set );
      }

    bool is_fd_in_set( int fd, fd_set * set )
      {
          return FD_ISSET( fd, set );
      }

public:
    virtual
    ~InputDevice()
      { }

    ///use this method to set initial values
    virtual
    bool set_defaults()
      {
          return true;
      }

    virtual
    bool set_initial_area( const Area2d & area )
      {
          return false;
      }

    /** \short read options from a file
        \param fname name of the file including options
    */
    virtual
    bool process_options( const char * fname )
      {
          return true;
      }

    /** \short read options from array of strings
        \param argc number of entries
        \param argv double array of '\0' terminated strings
    */
    virtual
    bool process_options( int argc,
                          char const * const * argv )
      {
          return true;
      }

    ///shows help for options provided by this device
    virtual
    void help_options( std::ostream & o ) const
      {
          o << "\nhelp not yet implementd";
      }

    virtual
    void help_char_command( std::ostream & o ) const
      {
          o << "\nhelp not yet implementd";
      }

    virtual
    void generate_file_options( std::ostream & o ) const
      {
          o << "\n[not yet implemeted]";
      }

    /**
       \short process a keyboard event
       \return true, if a redraw is needed afterwards, false else.
    */
    virtual
    bool process_char_command( BuilderBase * build,
                               MenuBase * menu,
                               const InputEvent & event )
      {
          return false;
      }

    /**
       \short process a mouse button event
       \return true, if a redraw is needed afterwards, false else.
    */
    virtual
    bool process_mouse_button_event( BuilderBase * build,
                                     const InputEvent & event )
      {
          return false;
      }

    /**
       \short process a menu button event
       \return true, if a redraw is needed afterwards, false else.
    */
    virtual
    bool process_menu_button( BuilderBase * build,
                              MenuBase * menu,
                              const InputEvent & event )
      {
          return false;
      }

    /**
       \short process a popup menu button event
       \return true, if a redraw is needed afterwards, false else.
    */
    virtual
    bool process_popup_button( BuilderBase * build,
                               MenuBase * popup,
                               const InputEvent & event )
      {
          return false;
      }

    virtual
    bool init_menu( MenuBase * menu )
      {
          return false;
      }

    virtual
    bool init_popup( MenuBase * popup )
      {
          popup->set_number_of_buttons( 1 );
          popup->set_button_label( 0, "empty" );
          return false;
      }

    virtual
    bool init_frames( BuilderBase * build ) = 0;

    virtual
    bool init_connection()
      {
          return true;
      }

    virtual
    bool uses_mouse() const
      {
          return false;
      }

    virtual
    bool uses_popup() const
      {
          return false;
      }

    virtual
    bool destruct()
      {
          return false;
      }

    /**
        \short main method to process input
        \return true, if a redraw is needed afterwards, false else.
    */
    virtual
    bool process_input( fd_set * , BuilderBase * build ) = 0;

    /// this routine shouldn't any longer be used (use the corresponding BuiderBase command)
    virtual
    const char * status_line() const
      {
          return "\0";
      }

    /**
       sets file descriptors this device is responsible for
       \return the biggest file descriptor set in this method
    */
    virtual
    int set_fds( fd_set * set ) = 0;

    /** \short reports if this device is responsible for file descriptors in fd_set
        \param set a set of file descriptors (filled in by select)
        \return true, if this device is responsible for some fd's in \param set
    */
    virtual
    bool got_fds( fd_set * set ) = 0;

    virtual
    bool is_timeover() const
      {
          return false;
      }

    virtual
    bool reconnect()
      {
          return false;
      }
};

#endif
