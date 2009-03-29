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

#ifndef BUILDER_DIRECT_H
#define BUILDER_DIRECT_H

#include "builder_base.h"
#include "structure.h"
#include "conv_area2d.h"

class BuilderDirect
    : public BuilderBase {
    DrawTree * dtree;
    ConvArea2d * c_area;

public:
    const char * status_line;
    int status_line_len;

    RGBcolor bg_color;
    bool new_bg_color;

    BuilderDirect( DrawTree &, ConvArea2d & );

    ///
    bool set_cmd_insert_frame( int parent_frame,
                               int this_frame,
                               const Point2d & pos,
                               const Angle & ang,
                               int layer );

    ///
    bool set_cmd_insert_visobject( int parent_frame,
                                   VisualObject2d * v_obj );

    ///
    bool set_cmd_insert_point( int parent_frame,
                               int this_object,
                               const Point2d & obj,
                               int layer,
                               const RGBcolor & col );

    bool set_cmd_insert_points( int parent_frame,
                                int this_object,
                                int len_data,
                                const Point2d * data,
                                int layer,
                                const RGBcolor & col );

    ///
    bool set_cmd_insert_line( int parent_frame,
                              int this_object,
                              const Line2d & obj,
                              int layer,
                              const RGBcolor & col );

    bool set_cmd_insert_lines( int parent_frame,
                               int this_object,
                               int len_data,
                               const Line2d * data,
                               int layer,
                               const RGBcolor & col );

    ///
    bool set_cmd_insert_circle( int parent_frame,
                                int this_object,
                                const Circle2d & obj,
                                int layer,
                                const RGBcolor & col );

    bool set_cmd_insert_circles( int parent_frame,
                                 int this_object,
                                 int len_data,
                                 const Circle2d * data,
                                 int layer,
                                 const RGBcolor & col );

    bool set_cmd_insert_f_circle( int parent_frame,
                                  int this_object,
                                  const Circle2d & obj,
                                  int layer,
                                  const RGBcolor & col );

    bool set_cmd_insert_f_circles( int parent_frame,
                                   int this_object,
                                   int len_data,
                                   const Circle2d * data,
                                   int layer,
                                   const RGBcolor & col );

    ///
    bool set_cmd_insert_circlearc( int parent_frame,
                                   int this_object,
                                   const CircleArc2d & obj,
                                   int layer,
                                   const RGBcolor & col );

    bool set_cmd_insert_circlearcs( int parent_frame,
                                    int this_object,
                                    int len_data,
                                    const CircleArc2d * data,
                                    int layer,
                                    const RGBcolor & col );

    bool set_cmd_insert_f_circlearc( int parent_frame,
                                     int this_object,
                                     const CircleArc2d & obj,
                                     int layer,
                                     const RGBcolor & col );

    bool set_cmd_insert_f_circlearcs( int parent_frame,
                                      int this_object,
                                      int len_data,
                                      const CircleArc2d * data,
                                      int layer,
                                      const RGBcolor & col );

    ///
    bool set_cmd_insert_polyline( int parent_frame,
                                  int this_object,
                                  int len_data,
                                  const Point2d * data,
                                  int layer,
                                  const RGBcolor & col );

    ///
    bool set_cmd_insert_polygon( int parent_frame,
                                 int this_object,
                                 int len_data,
                                 const Point2d * data,
                                 int layer,
                                 const RGBcolor & col );

    bool set_cmd_insert_f_polygon( int parent_frame,
                                   int this_object,
                                   int len_data,
                                   const Point2d * data,
                                   int layer,
                                   const RGBcolor & col );

    ///
    bool set_cmd_insert_string( int parent_frame,
                                int this_object,
                                const Point2d & pos,
                                int len_data,
                                const char * data,
                                int layer,
                                const RGBcolor & col );

    ///
    bool set_cmd_set_object_visible( int frame,
                                     int this_object,
                                     int vis );

    ///
    bool set_cmd_set_object_layer( int frame,
                                   int this_object,
                                   int layer );

    ///
    bool set_cmd_set_object_color( int frame,
                                   int this_object,
                                   const RGBcolor & col );

    ///
    bool set_cmd_set_frame_visible( int this_frame,
                                    int visible );

    ///
    bool set_cmd_set_frame_layer( int this_frame,
                                  int layer );

    ///
    bool set_cmd_set_frame_pos( int this_frame,
                                const Point2d & pos );

    ///
    bool set_cmd_set_frame_ang( int this_frame,
                                const Angle & ang );

    ///
    bool set_cmd_set_frame_scale( int this_frame,
                                  double scale );

    ///
    bool set_cmd_set_frame_pos_ang( int this_frame,
                                    const Point2d & pos,
                                    const Angle & ang );

    ///
    bool set_cmd_remove_frame( int this_frame );

    ///
    bool set_cmd_remove_object( int frame,
                                int this_object );

    ///
    bool set_cmd_empty_frame( int this_frame );

    ///
    bool set_cmd_set_view_area( const Area2d & area );

    ///
    bool set_cmd_set_status_line( int len_data,
                                  const char * data );

    ///
    bool set_cmd_set_background_color( const RGBcolor & col );
};

#endif
