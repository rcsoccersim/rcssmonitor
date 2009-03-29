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
#ifndef MENU_BASE_H
#define MENU_BASE_H

class MenuBase {

public:
    virtual ~MenuBase()
      { }

    virtual bool needs_redraw() const = 0;
    virtual bool set_number_of_buttons( int num ) = 0;
    virtual int  get_nubber_of_buttons() const = 0;
    virtual bool set_button_label( int num, const char * lab ) = 0;
    virtual const char * get_button_label( int num ) const = 0;
    virtual bool set_menu_extended( bool flag = true ) = 0;
    virtual bool set_exit_program() = 0;
};

#endif
