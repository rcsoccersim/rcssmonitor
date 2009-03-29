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
#include "angle.h"
#include "vector2d.h"
#include "maplist.h"
#include "visobject.h"


class DrawFrameMap;

class DrawFrame
{

    friend class DrawTree;
    friend std::ostream & operator<<( std::ostream &, const DrawFrame & );
    //  friend main();

    bool changed;

    DrawFrame * up;
    KeyMap_LayerList< DrawFrame > frames;
    //KeyMap_LayerList< DrawObject > objects;
    KeyMap_LayerList< VisualObject2d > objects;

    Frame2d absFrame; // der Frame in Bezug auf den Ursprungs-Frame

public:
    Frame2d relFrame; // der Frame selbst;
    int key;
    int layer;
    int get_key() const
      {
          return key;
      }

    int get_layer() const
      {
          return layer;
      }

    bool visible;

    DrawFrame( int k );
    ~DrawFrame();
    // Precondition: f->key is unique, f->left= 0; f->right==0, f->up==0
    bool insert( DrawFrame * );
    bool remove( DrawFrame * );
    bool insert( VisualObject2d * );
    bool remove( VisualObject2d * );
    //removes all frames and object which belong to this frame.
    //additionally, all frame keys are removed from the DrawFrameMap fmap
    bool remove_all( DrawFrameMap & fmap );

    void actualize( const Frame2d & p_frame,
                    bool );
    void draw();
    void draw( DisplayBase * disp,
               const Area2d & area,
               const Frame2d & p_frame,
               bool chg );
};

class DrawFrameMap
{

    friend std::ostream & operator<<( std::ostream &,
                                      const DrawFrameMap & );

    class Item {
    public:
        Item( DrawFrame* );
        int key;
        DrawFrame * frame;
        Item * next;
    };

    Item * list;

    enum {
        MAX_DIRECT = 100 // frames with key in (0..MAX_DIRECT) are mapped in O(1)
    };

    DrawFrame * direct_map[MAX_DIRECT];

public:
    DrawFrameMap();
    bool insert( DrawFrame * );
    bool remove( int key );
    DrawFrame * get( int key );
};

class DrawTree
{

    friend std::ostream & operator<<( std::ostream &,
                                      const DrawTree & );
    DrawFrame * origin;
    DrawFrameMap frameMap;

public:
    DrawTree();
    bool insert_in_frame( int fkey,
                          DrawFrame * );
    bool insert_in_frame( int fkey,
                          VisualObject2d * );

    bool remove_frame( int fkey );
    bool remove_in_frame( int fkey,
                          int fo_key );
    bool empty_frame( int fkey );

    //Drehung um eigenen Ursprung um einen Winkel, alle Objekte und Unterframes
    //drehen sich mit.
    bool rotate_frame( int fkey,
                       const Angle & );
    //Translation in Bezug auf das eigene Koordinatensystem, alle Objekte und
    //Unterframes werden auch verschoben!
    bool translate_frame( int fkey,
                          double,
                          double );

    //Drehe ein Objekt oder ein UnterFrame (mit fo_key) um den Ursprung um einen Winkel
    bool rotate_in_frame( int fkey,
                          int fo_key,
                          const Angle & );
    //Translation
    bool translate_in_frame( int fkey,
                             int fo_key,
                             double,
                             double );

    bool set_object_color( int fkey,
                           int okey,
                           const RGBcolor & );

    bool set_position( int fkey,
                       double,
                       double );
    bool set_visible( int fkey,
                      bool val );
    bool set_layer( int fkey,
                    int val );
    //bool set_positon(int fkey,int ofkey,double,double);
    bool set_angle( int fkey,
                    const Angle & );
    bool set_scale( int fkey,
                    double scale );
    bool set_pos_ang( int fkey,
                      double,
                      double,
                      const Angle & );
    void draw( const Area2d & );
    void draw( DisplayBase * disp,
               const Area2d & area );
};

#endif
