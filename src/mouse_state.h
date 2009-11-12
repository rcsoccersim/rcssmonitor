// -*-c++-*-

/*!
	\file mouse_state.h
	\brief mouse state class Header File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.	If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef RCSSMONITOR_MOUSE_STATE_H
#define RCSSMONITOR_MOUSE_STATE_H

#include <QPoint>

#include <cstdlib>

class MouseState {
private:

    bool M_pressed;
    bool M_menu_failed;
    QPoint M_pressed_point;
    QPoint M_dragged_point;

public:
    MouseState()
        : M_pressed( false )
        , M_menu_failed( false )
        , M_pressed_point( 0, 0 )
        , M_dragged_point( 0, 0 )
          { }


    void pressed( const QPoint & point )
      {
          M_pressed = true;
          M_pressed_point = M_dragged_point = point;
      }

    void released()
      {
          M_pressed = false;
      }

    void setMenuFailed( const bool value )
      {
          M_menu_failed = value;
      }

    void moved( const QPoint & point )
      {
          if ( M_pressed )
          {
              M_dragged_point = point;
          }
      }

    bool isPressed() const
      {
          return M_pressed;
      }

    bool isMenuFailed() const
      {
          return M_menu_failed;
      }

    const
    QPoint & pressedPoint() const
      {
          return M_pressed_point;
      }

    const
    QPoint & draggedPoint() const
      {
          return M_dragged_point;
      }

    bool isDragged() const
      {
          return ( M_pressed
                   && ( std::abs( M_pressed_point.x() - M_dragged_point.x() ) > 2
                        || std::abs( M_pressed_point.y() - M_dragged_point.y() ) > 2 )
                   );
      }

};

#endif
