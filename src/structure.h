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

#ifndef STRUCTURE_H
#define STRUCTURE_H

#include "frame2d.h"
#include "maplist.h"


class Angle;
class Point2d;
class Area2d;
class VisualObject2d;
class RGBcolor;
class DisplayBase;

class DrawFrameMap;

class DrawFrame {

    friend class DrawTree;

    int M_key; //!< global index value of this object
    int M_layer;

    DrawFrame * M_parent; //!< parent frame
    KeyMap_LayerList< DrawFrame > M_frames;
    //KeyMap_LayerList< DrawObject > objects;
    KeyMap_LayerList< VisualObject2d > M_objects;

    Frame2d M_abs_transform; //!< grobal transformation matrix
    Frame2d M_rel_transform; //!< relative transformation matrix

    bool M_visible;
    bool M_changed;
public:

    int get_key() const
      {
          return M_key;
      }

    int get_layer() const
      {
          return M_layer;
      }

    DrawFrame( int key );
    ~DrawFrame();

    void init( const Point2d & translation,
               const Angle & rotation,
               const int layer );

    // Precondition: f->key is unique, f->parent==0
    bool insert( DrawFrame * );
    bool remove( DrawFrame * );
    bool insert( VisualObject2d * );
    bool remove( VisualObject2d * );
    //removes all frames and object which belong to this frame.
    //additionally, all frame keys are removed from the DrawFrameMap fmap
    bool remove_all( DrawFrameMap & fmap );

    void actualize( const Frame2d & p_frame,
                    const bool chg );
    void draw();
    void draw( DisplayBase * disp,
               const Area2d & area,
               const Frame2d & p_frame,
               const bool chg );

    std::ostream & print( std::ostream & os ) const;
};

class DrawFrameMap {

    enum {
        MAX_DIRECT = 100 //!< frames with key in (0..MAX_DIRECT) are mapped in O(1)
    };

    /*!
      \brief list item
     */
    class Item {
    public:
        Item( DrawFrame * );
        int key;
        DrawFrame * frame;
        Item * next;
    };

    DrawFrame * M_direct_map[MAX_DIRECT]; //!< array frames
    Item * M_list; //!< linked list frames

public:
    DrawFrameMap();
    bool insert( DrawFrame * );
    bool remove( int key );
    DrawFrame * get( int key );


    std::ostream & print( std::ostream & os ) const;
};

class DrawTree {

    DrawFrame * M_origin;
    DrawFrameMap M_frame_map;

public:
    DrawTree();
    bool insert_in_frame( const int fkey,
                          DrawFrame * );
    bool insert_in_frame( const int fkey,
                          VisualObject2d * );

    bool remove_frame( const int fkey );
    bool remove_in_frame( const int fkey,
                          const int fo_key );
    bool empty_frame( const int fkey );

    //Drehung um eigenen Ursprung um einen Winkel, alle Objekte und Unterframes
    //drehen sich mit.
    bool rotate_frame( const int fkey,
                       const Angle & );

    //Translation
    bool translate_in_frame( const int fkey,
                             const int fo_key,
                             const double & x,
                             const double & y );

    bool set_object_color( const int fkey,
                           const int okey,
                           const RGBcolor & );

    bool set_position( const int fkey,
                       const double & x,
                       const double & y );
    bool set_visible( const int fkey,
                      const bool val );
    bool set_layer( const int fkey,
                    const int val );
    //bool set_positon(int fkey,int ofkey,double,double);
    bool set_angle( const int fkey,
                    const Angle & );
    bool set_scale( const int fkey,
                    const double & scale );
    bool set_pos_ang( const int fkey,
                      const double & x ,
                      const double & y,
                      const Angle & angle );
    void draw( const Area2d & );
    void draw( DisplayBase * disp,
               const Area2d & area );

    std::ostream & print( std::ostream & os ) const;
};

#endif
