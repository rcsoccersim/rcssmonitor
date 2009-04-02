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

#ifndef CODER_H
#define CODER_H

#include "rgbcolor.h"
#include "object2d.h"
#include "multi.h"
#include "rwbin.h"

class BuilderBase;

inline
void
wbin( std::ostream & o,
      const RGBcolor & col )
{
    wbin( o, col.red );
    wbin( o, col.green );
    wbin( o, col.blue );
}

inline
void
rbin( std::istream & i,
      RGBcolor & col )
{
    rbin( i, col.red );
    rbin( i, col.green );
    rbin( i, col.blue );
}

inline
void
wbin( std::ostream & o,
      const Angle & ang )
{
    wbin( o, ang.get_value() );
}

inline
void
rbin( std::istream & i,
      Angle & ang )
{
    double d;
    rbin( i, d );
    ang = Angle( d );
}

inline
void wbin( std::ostream & o,
           const Point2d & p )
{
    wbin( o, p.x );
    wbin( o, p.y );
}

inline
void rbin( std::istream & i,
           Point2d & p )
{
    rbin( i, p.x );
    rbin( i, p.y );
}

inline
void wbin( std::ostream & o,
           const Line2d & line )
{
    wbin( o, line.p1 );
    wbin( o, line.p2 );
}

inline
void rbin( std::istream & i,
           Line2d & line )
{
    rbin( i, line.p1 );
    rbin( i, line.p2 );
}

inline
void wbin( std::ostream & o,
           const Circle2d & circ )
{
    wbin( o, circ.center );
    wbin( o, circ.radius );
}

inline
void rbin( std::istream & i,
           Circle2d & circ )
{
    rbin( i, circ.center );
    rbin( i, circ.radius );
}

inline
void wbin( std::ostream & o,
           const CircleArc2d & circ )
{
    wbin( o, circ.center );
    wbin( o, circ.radius );
    wbin( o, circ.ang1 );
    wbin( o, circ.ang2 );
}

inline
void rbin( std::istream & i,
           CircleArc2d & circ )
{
    rbin( i, circ.center );
    rbin( i, circ.radius );
    rbin( i, circ.ang1 );
    rbin( i, circ.ang2 );
}

template< typename T >
inline
void
wbin( std::ostream & o,
      const Multi< T > & mult )
{
    wbin( o, mult.cur_size );

    for ( int k = 0; k < mult.cur_size; ++k )
    {
        wbin( o, mult.tab[k] );
    }
}

template< typename T >
inline
void
rbin( std::istream & i,
      Multi< T > & mult )
{
    int dum;
    rbin( i, dum );

    if ( i.fail() ) return;

    mult.cur_size = dum;

    if ( mult.cur_size > mult.max_size )
    {
        if ( mult.tab ) delete[] mult.tab;

        mult.max_size = mult.cur_size;

        mult.tab = new T[mult.max_size];
    }

    for ( int k = 0; k < mult.cur_size; ++k )
    {
        rbin( i, mult.tab[k] );
    }
}

class EnDeCoderBin {
public:
    bool build_from_binary( BuilderBase * build,
                            std::istream & in );

    static const int cmd_insert_frame;

    static const int cmd_insert_point;
    static const int cmd_insert_points;

    static const int cmd_insert_line;
    static const int cmd_insert_lines;

    static const int cmd_insert_circle;
    static const int cmd_insert_circles;
    static const int cmd_insert_f_circle; //f means filled
    static const int cmd_insert_f_circles; //f means filled

    static const int cmd_insert_circlearc;
    static const int cmd_insert_circlearcs;
    static const int cmd_insert_f_circlearc;
    static const int cmd_insert_f_circlearcs;

    static const int cmd_insert_polyline;

    static const int cmd_insert_polygon;
    static const int cmd_insert_f_polygon;

    static const int cmd_insert_string;


    static const int cmd_set_object_visible;
    static const int cmd_set_object_layer;
    static const int cmd_set_object_color;

    static const int cmd_set_frame_visible;
    static const int cmd_set_frame_layer;
    static const int cmd_set_frame_pos;
    static const int cmd_set_frame_ang;
    static const int cmd_set_frame_pos_ang;

    static const int cmd_remove_frame;
    static const int cmd_remove_object;

    static const int cmd_empty_frame;

    /*   static const int cmd_rotate_frame       = 10; */
    /*   static const int cmd_translate_frame    = 11; */

    bool get_cmd_type( std::istream & i,
                       int & cmd )
      {
          rbin( i, cmd );
          return ! i.eof();
      }

    ////
    bool set_cmd_insert_frame( std::ostream & o,
                               int parent_frame,
                               int this_frame,
                               const Point2d & pos,
                               const Angle & ang,
                               int layer )
      {
          wbin( o, cmd_insert_frame );
          wbin( o, parent_frame );
          wbin( o, this_frame );
          wbin( o, pos );
          wbin( o, ang );
          wbin( o, layer );
          return ! o.fail();
      }

    bool get_cmd_insert_frame( std::istream & i,
                               int & parent_frame,
                               int & this_frame,
                               Point2d & pos,
                               Angle & angle,
                               int & layer )
      {
          rbin( i, parent_frame );
          rbin( i, this_frame );
          rbin( i, pos );
          rbin( i, angle );
          rbin( i, layer );
          return ! i.fail();
      }

    ////
    bool set_cmd_insert_point( std::ostream & o,
                               int parent_frame,
                               int this_object,
                               const Point2d & obj,
                               int layer,
                               const RGBcolor & col )
    {
        wbin( o, cmd_insert_point );
        wbin( o, parent_frame );
        wbin( o, this_object );
        wbin( o, obj );
        wbin( o, layer );
        wbin( o, col );
        return ! o.fail();
    }

    bool get_cmd_insert_point( std::istream & i,
                               int & parent_frame,
                               int & this_object,
                               Point2d & obj,
                               int & layer,
                               RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, obj );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    bool set_cmd_insert_points( std::ostream & o,
                                int parent_frame,
                                int this_object,
                                const Multi< Point2d > & mul,
                                int layer,
                                const RGBcolor & col )
      {
          wbin( o, cmd_insert_points );
          wbin( o, parent_frame );
          wbin( o, this_object );
          wbin( o, mul );
          wbin( o, layer );
          wbin( o, col );
          return ! o.fail();
      }

    bool get_cmd_insert_points( std::istream & i,
                                int & parent_frame,
                                int & this_object,
                                Multi< Point2d > & mul,
                                int & layer,
                                RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, mul );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    ////
    bool set_cmd_insert_line( std::ostream & o,
                              int parent_frame,
                              int this_object,
                              const Line2d & obj,
                              int layer,
                              const RGBcolor & col )
      {
          wbin( o, cmd_insert_line );
          wbin( o, parent_frame );
          wbin( o, this_object );
          wbin( o, obj );
          wbin( o, layer );
          wbin( o, col );
          return ! o.fail();
      }

    bool get_cmd_insert_line( std::istream & i,
                              int & parent_frame,
                              int & this_object,
                              Line2d & obj,
                              int & layer,
                              RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, obj );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    bool set_cmd_insert_lines( std::ostream & o,
                               int parent_frame,
                               int this_object,
                               const Multi< Line2d > & mul,
                               int layer,
                               const RGBcolor & col )
      {
          wbin( o, cmd_insert_lines );
          wbin( o, parent_frame );
          wbin( o, this_object );
          wbin( o, mul );
          wbin( o, layer );
          wbin( o, col );
          return ! o.fail();
      }

    bool get_cmd_insert_lines( std::istream & i,
                               int & parent_frame,
                               int & this_object,
                               Multi< Line2d > & mul,
                               int & layer,
                               RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, mul );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    ////
    bool set_cmd_insert_circle( std::ostream & o,
                                int parent_frame,
                                int this_object,
                                const Circle2d & obj,
                                int layer,
                                const RGBcolor & col )
      {
          wbin( o, cmd_insert_circle );
          wbin( o, parent_frame );
          wbin( o, this_object );
          wbin( o, obj );
          wbin( o, layer );
          wbin( o, col );
          return !o.fail();
      }

    bool get_cmd_insert_circle( std::istream & i,
                                int & parent_frame,
                                int & this_object,
                                Circle2d & obj,
                                int & layer,
                                RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, obj );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    bool set_cmd_insert_circles( std::ostream & o,
                                 int parent_frame,
                                 int this_object,
                                 const Multi< Circle2d > & mul,
                                 int layer,
                                 const RGBcolor & col )
      {
          wbin( o, cmd_insert_circles );
          wbin( o, parent_frame );
          wbin( o, this_object );
          wbin( o, mul );
          wbin( o, layer );
          wbin( o, col );
          return ! o.fail();
      }

    bool get_cmd_insert_circles( std::istream & i,
                                 int & parent_frame,
                                 int & this_object,
                                 Multi< Circle2d > & mul,
                                 int & layer,
                                 RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, mul );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    bool set_cmd_insert_f_circle( std::ostream & o,
                                  int parent_frame,
                                  int this_object,
                                  const Circle2d & obj,
                                  int layer,
                                  const RGBcolor & col )
      {
          wbin( o, cmd_insert_f_circle );
          wbin( o, parent_frame );
          wbin( o, this_object );
          wbin( o, obj );
          wbin( o, layer );
          wbin( o, col );
          return ! o.fail();
      }

    bool get_cmd_insert_f_circle( std::istream & i,
                                  int & parent_frame,
                                  int & this_object,
                                  Circle2d & obj,
                                  int & layer,
                                  RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, obj );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    bool set_cmd_insert_f_circles( std::ostream & o,
                                   int parent_frame,
                                   int this_object,
                                   const Multi< Circle2d > & mul,
                                   int layer,
                                   const RGBcolor & col )
      {
          wbin( o, cmd_insert_f_circles );
          wbin( o, parent_frame );
          wbin( o, this_object );
          wbin( o, mul );
          wbin( o, layer );
          wbin( o, col );
          return ! o.fail();
      }

    bool get_cmd_insert_f_circles( std::istream & i,
                                   int & parent_frame,
                                   int & this_object,
                                   Multi< Circle2d > & mul,
                                   int & layer,
                                   RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, mul );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    ////
    bool set_cmd_insert_circlearc( std::ostream & o,
                                   int parent_frame,
                                   int this_object,
                                   const CircleArc2d & obj,
                                   int layer,
                                   const RGBcolor & col )
      {
          wbin( o, cmd_insert_circlearc );
          wbin( o, parent_frame );
          wbin( o, this_object );
          wbin( o, obj );
          wbin( o, layer );
          wbin( o, col );
          return ! o.fail();
      }

    bool get_cmd_insert_circlearc( std::istream & i,
                                   int & parent_frame,
                                   int & this_object,
                                   CircleArc2d & obj,
                                   int & layer,
                                   RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, obj );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    bool set_cmd_insert_circlearcs( std::ostream & o,
                                    int parent_frame,
                                    int this_object,
                                    const Multi< CircleArc2d > & mul,
                                    int layer,
                                    const RGBcolor & col )
      {
          wbin( o, cmd_insert_circlearcs );
          wbin( o, parent_frame );
          wbin( o, this_object );
          wbin( o, mul );
          wbin( o, layer );
          wbin( o, col );
          return !o.fail();
      }

    bool get_cmd_insert_circlearcs( std::istream & i,
                                    int & parent_frame,
                                    int & this_object,
                                    Multi< CircleArc2d > & mul,
                                    int & layer,
                                    RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, mul );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    bool set_cmd_insert_f_circlearc( std::ostream & o,
                                     int parent_frame,
                                     int this_object,
                                     const CircleArc2d & obj,
                                     int layer,
                                     const RGBcolor & col )
      {
          wbin( o, cmd_insert_f_circlearc );
          wbin( o, parent_frame );
          wbin( o, this_object );
          wbin( o, obj );
          wbin( o, layer );
          wbin( o, col );
          return ! o.fail();
      }

    bool get_cmd_insert_f_circlearc( std::istream & i,
                                     int & parent_frame,
                                     int & this_object,
                                     CircleArc2d & obj,
                                     int & layer,
                                     RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, obj );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    bool set_cmd_insert_f_circlearcs( std::ostream & o,
                                      int parent_frame,
                                      int this_object,
                                      const Multi< CircleArc2d > & mul,
                                      int layer,
                                      const RGBcolor & col )
      {
          wbin( o, cmd_insert_f_circlearcs );
          wbin( o, parent_frame );
          wbin( o, this_object );
          wbin( o, mul );
          wbin( o, layer );
          wbin( o, col );
          return ! o.fail();
      }

    bool get_cmd_insert_f_circlearcs( std::istream & i,
                                      int & parent_frame,
                                      int & this_object,
                                      Multi< CircleArc2d > & mul,
                                      int & layer,
                                      RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, mul );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    ////
    bool set_cmd_insert_polyline( std::ostream & o,
                                  int parent_frame,
                                  int this_object,
                                  const Multi< Point2d > & mul,
                                  int layer,
                                  const RGBcolor & col )
      {
          wbin( o, cmd_insert_polyline );
          wbin( o, parent_frame );
          wbin( o, this_object );
          wbin( o, mul );
          wbin( o, layer );
          wbin( o, col );
          return ! o.fail();
      }

    bool get_cmd_insert_polyline( std::istream & i,
                                  int & parent_frame,
                                  int & this_object,
                                  Multi< Point2d > & mul,
                                  int & layer,
                                  RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, mul );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    ////
    bool set_cmd_insert_polygon( std::ostream & o,
                                 int parent_frame,
                                 int this_object,
                                 const Multi< Point2d > & mul,
                                 int layer,
                                 const RGBcolor & col )
      {
          wbin( o, cmd_insert_polygon );
          wbin( o, parent_frame );
          wbin( o, this_object );
          wbin( o, mul );
          wbin( o, layer );
          wbin( o, col );
          return !o.fail();
      }

    bool get_cmd_insert_polygon( std::istream & i,
                                 int & parent_frame,
                                 int & this_object,
                                 Multi< Point2d > & mul,
                                 int & layer,
                                 RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, mul );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    bool set_cmd_insert_f_polygon( std::ostream & o,
                                   int parent_frame,
                                   int this_object,
                                   const Multi< Point2d > & mul,
                                   int layer,
                                   const RGBcolor & col )
      {
          wbin( o, cmd_insert_f_polygon );
          wbin( o, parent_frame );
          wbin( o, this_object );
          wbin( o, mul );
          wbin( o, layer );
          wbin( o, col );
          return ! o.fail();
      }

    bool get_cmd_insert_f_polygon( std::istream & i,
                                   int & parent_frame,
                                   int & this_object,
                                   Multi< Point2d > & mul,
                                   int & layer,
                                   RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, mul );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    ////
    bool set_cmd_insert_string( std::ostream & o,
                                int parent_frame,
                                int this_object,
                                const Point2d & pos,
                                const Multi< char > & mul,
                                int layer,
                                const RGBcolor & col )
      {
          wbin( o, cmd_insert_string );
          wbin( o, parent_frame );
          wbin( o, this_object );
          wbin( o, pos );
          wbin( o, mul );
          wbin( o, layer );
          wbin( o, col );
          return !o.fail();
      }

    bool get_cmd_insert_string( std::istream & i,
                                int & parent_frame,
                                int & this_object,
                                Point2d & pos,
                                Multi< char > & mul,
                                int & layer,
                                RGBcolor & col )
      {
          rbin( i, parent_frame );
          rbin( i, this_object );
          rbin( i, pos );
          rbin( i, mul );
          rbin( i, layer );
          rbin( i, col );
          return ! i.fail();
      }

    ////
    bool set_cmd_set_object_visible( std::ostream & o,
                                     int frame,
                                     int this_object,
                                     int vis )
      {
          wbin( o, cmd_set_object_visible );
          wbin( o, frame );
          wbin( o, this_object );
          wbin( o, vis );
          return ! o.fail();
      }

    bool get_cmd_set_object_visible( std::istream & i,
                                     int & frame,
                                     int & this_object,
                                     int & vis )
      {
          rbin( i, frame );
          rbin( i, this_object );
          rbin( i, vis );
          return !i.fail();
      }

    ////
    bool set_cmd_set_object_layer( std::ostream & o,
                                   int frame,
                                   int this_object,
                                   int layer )
      {
          wbin( o, cmd_set_object_layer );
          wbin( o, frame );
          wbin( o, this_object );
          wbin( o, layer );
          return ! o.fail();
      }

    bool get_cmd_set_object_layer( std::istream & i,
                                   int & frame,
                                   int & this_object,
                                   int & layer )
      {
          rbin( i, frame );
          rbin( i, this_object );
          rbin( i, layer );
          return ! i.fail();
      }

    ////
    bool set_cmd_set_object_color( std::ostream & o,
                                   int frame,
                                   int this_object,
                                   const RGBcolor & col )
      {
          wbin( o, cmd_set_object_color );
          wbin( o, frame );
          wbin( o, this_object );
          wbin( o, col );
          return ! o.fail();
      }

    bool get_cmd_set_object_color( std::istream & i,
                                   int & frame,
                                   int & this_object,
                                   RGBcolor & col )
      {
          rbin( i, frame );
          rbin( i, this_object );
          rbin( i, col );
          return ! i.fail();
      }

    ////
    bool set_cmd_set_frame_visible( std::ostream & o,
                                    int this_frame,
                                    int visible )
      {
          wbin( o, cmd_set_frame_visible );
          wbin( o, this_frame );
          wbin( o, visible );
          return ! o.fail();
      }

    bool get_cmd_set_frame_visible( std::istream & i,
                                    int & this_frame,
                                    int & visible )
      {
          rbin( i, this_frame );
          rbin( i, visible );
          return ! i.fail();
      }

    ////
    bool set_cmd_set_frame_layer( std::ostream & o,
                                  int this_frame,
                                  int layer )
      {
          wbin( o, cmd_set_frame_layer );
          wbin( o, this_frame );
          wbin( o, layer );
          return ! o.fail();
      }

    bool get_cmd_set_frame_layer( std::istream & i,
                                  int & this_frame,
                                  int & layer )
      {
          rbin( i, this_frame );
          rbin( i, layer );
          return ! i.fail();
      }

    ////
    bool set_cmd_set_frame_pos( std::ostream & o,
                                int this_frame,
                                const Point2d & pos )
      {
          wbin( o, cmd_set_frame_pos );
          wbin( o, this_frame );
          wbin( o, pos );
          return ! o.fail();
      }

    bool get_cmd_set_frame_pos( std::istream & i,
                                int & this_frame,
                                Point2d & pos )
      {
          rbin( i, this_frame );
          rbin( i, pos );
          return ! i.fail();
      }

    ////
    bool set_cmd_set_frame_ang( std::ostream & o,
                                int this_frame,
                                const Angle & ang )
      {
          wbin( o, cmd_set_frame_ang );
          wbin( o, this_frame );
          wbin( o, ang );
          return ! o.fail();
      }

    bool get_cmd_set_frame_ang( std::istream & i,
                                int & this_frame,
                                Angle & ang )
      {
          rbin( i, this_frame );
          rbin( i, ang );
          return ! i.fail();
      }

    ////
    bool set_cmd_set_frame_pos_ang( std::ostream & o,
                                    int this_frame,
                                    const Point2d & pos,
                                    const Angle & ang )
      {
          wbin( o, cmd_set_frame_pos_ang );
          wbin( o, this_frame );
          wbin( o, pos );
          wbin( o, ang );
          return ! o.fail();
      }

    bool get_cmd_set_frame_pos_ang( std::istream & i,
                                    int & this_frame,
                                    Point2d & pos,
                                    Angle & ang )
      {
          rbin( i, this_frame );
          rbin( i, pos );
          rbin( i, ang );
          return ! i.fail();
      }

    ////
    bool set_cmd_remove_frame( std::ostream & o,
                               int this_frame )
      {
          wbin( o, cmd_remove_frame );
          wbin( o, this_frame );
          return ! o.fail();
      }

    bool get_cmd_remove_frame( std::istream & i,
                               int & this_frame )
      {
          rbin( i, this_frame );
          return ! i.fail();
      }

    ////
    bool set_cmd_remove_object( std::ostream & o,
                                int frame,
                                int this_object )
      {
          wbin( o, cmd_remove_object );
          wbin( o, frame );
          wbin( o, this_object );
          return ! o.fail();
      }

    bool get_cmd_remove_object( std::istream & i,
                                int & frame,
                                int & this_object )
      {
          rbin( i, frame );
          rbin( i, this_object );
          return ! i.fail();
      }

    ////
    bool set_cmd_empty_frame( std::ostream & o,
                              int this_frame )
      {
          wbin( o, cmd_empty_frame );
          wbin( o, this_frame );
          return ! o.fail();
      }

    bool get_cmd_empty_frame( std::istream & i,
                              int & this_frame )
      {
          rbin( i, this_frame );
          return ! i.fail();
      }

    ////
};

#endif
