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
#ifndef CONV_AREA2D_H
#define CONV_AREA2D_H

#include "area2d.h"

/*!
  \class ConvArea2d
  \brief Implements conversion of areas under constraints.

  The most important conastaint is the size of the window the area is
  living in. We just want to handle areas which have the same
  proportions as this window. If it is ever needed, then it will be
  an easy task to allow regions with other proportions (just introduce
  a switch use_win_size_as_constraint).

  All other constraints can be switched on/off depending on your needs.
*/
class ConvArea2d {
    /*!
      \brief indicates the level of changes.

      Is level=n, so you have to make changes on level n,n-1,...,1
      Is level= 0 so you don't have to change anything, the area doesnt need
      a change.
    */
    int level;

    /*!
      \brief set new level with regard to old value of level
      should always be used. Don't set level direct.
    */
    void update_level( int new_lev )
      {
          if ( new_lev > level ) level = new_lev;
      }

    // level 4 changes
    int        win_size_x;
    int        win_size_y;

    // level 3 changes
    double     min_size_x; //!< if not set it bevaves as equal 0
    bool   use_min_size_x;
    double     min_size_y; //!< if not set it bevaves as equal 0
    bool   use_min_size_y;

    //level 2 changes
    double     max_size_x; //!< if not set it bevaves as equal infinity
    bool   use_max_size_x;
    double     max_size_y; //!< if not set it bevaves as equal infinity
    bool   use_max_size_y;

    //level 1 changes
    double     min_pos_x;  //!< if not set it bevaves as equal -infinity
    bool   use_min_pos_x;
    double     min_pos_y;  //!< if not set it bevaves as equal -infinity
    bool   use_min_pos_y;
    double     max_pos_x;  //!< if not set it bevaves as equal infinity
    bool   use_max_pos_x;
    double     max_pos_y;  //!< if not set it bevaves as equal infinity
    bool   use_max_pos_y;

    Area2d area;

public:
    ConvArea2d();

    //! always call this method to check if there were some changes to the area
    bool needs_update()
      {
          return level != 0;
      }

    //! performs the update! returns true if some values of the area changed
    bool update();

    void set_win_size( int sx,
                       int sy );

    void   set_min_size_x( double v )
      {
          min_size_x = v;
          use_min_size_x = true;
          update_level( 3 );
      }

    void unset_min_size_x()
      {
          use_min_size_x = false;
      }

    void   set_min_size_y( double v )
      {
          min_size_y = v;
          use_min_size_y = true;
          update_level( 3 );
      }

    void unset_min_size_y()
      {
          use_min_size_y = false;
      }

    void   set_max_size_x( double v )
      {
          max_size_x = v;
          use_max_size_x = true;
          update_level( 2 );
      }

    void unset_max_size_x()
      {
          use_max_size_x = false;
      }

    void   set_max_size_y( double v )
      {
          max_size_y = v;
          use_max_size_y = true;
          update_level( 2 );
      }

    void unset_max_size_y()
      {
          use_max_size_y = false;
      }

    void   set_min_pos_x( double v )
      {
          min_pos_x = v;
          use_min_pos_x = true;
          update_level( 1 );
      }

    void unset_min_pos_x()
      {
          use_min_pos_x = false;
      }

    void   set_min_pos_y( double v )
      {
          min_pos_y = v;
          use_min_pos_y = true;
          update_level( 1 );
      }

    void unset_min_pos_y()
      {
          use_min_pos_y = false;
      }

    void   set_max_pos_x( double v )
      {
          max_pos_x = v;
          use_max_pos_x = true;
          update_level( 1 );
      }

    void unset_max_pos_x()
      {
          use_max_pos_x = false;
      }

    void   set_max_pos_y( double v )
      {
          max_pos_y = v;
          use_max_pos_y = true;
          update_level( 1 );
      }

    void unset_max_pos_y()
      {
          use_max_pos_y = false;
      }

    /** \short sets a new area
        because this area can be arbitrary, you have to check all levels
    */
    void set_area( const Area2d & a )
      {
          area = a;
          update_level( 4 );
      }

    void get_area( Area2d & a ) const
      {
          a = area;
      }

    /** \short use move_area* if you just move the area.
        It needs fewer update steps if it is know that the size of an area didn't change
    */
    void move_area_dir_x_using_factor( double factor );
    void move_area_dir_y_using_factor( double factor );

    void scale_area_using_factor( double factor );

    void change_area_using_subwin( int origin_x,
                                   int origin_y,
                                   int dest_x,
                                   int dest_y );

    void change_area_using_win_pos( int origin_x,
                                    int origin_y );

    void show_area()
      {
          std::cout << "\nArea: center= (" << area.center.x << "," << area.center.y
                    << ")  size_x= " << area.size_x << "  size_y= " << area.size_y;
      }

    Point2d get_point_from_win_pos( int x,
                                    int y ) const;

};

#endif
