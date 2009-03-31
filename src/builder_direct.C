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

#include "builder_direct.h"

#define B_ERROR(xxx) std::cerr << "\n" << __LINE__ << ":" << xxx

BuilderDirect::BuilderDirect( DrawTree & t,
                              ConvArea2d & ca )
{
    dtree = &t;
    c_area = &ca;

    status_line = 0;
    status_line_len = 0;
    new_bg_color = 0;
}

///
bool
BuilderDirect::set_cmd_insert_frame( int parent_frame,
                                     int this_frame,
                                     const Point2d & pos,
                                     const Angle & ang,
                                     int layer )
{
    DrawFrame * f_dum = new DrawFrame( this_frame );
//     f_dum->M_rel_frame = Frame2d::Translation( pos.x, pos.y ) * f_dum->M_rel_frame;
//     f_dum->M_rel_frame = f_dum->M_rel_frame * Frame2d::Rotation( ang );
//     f_dum->M_layer = layer;
    f_dum->init( pos, ang, layer );

    bool res = dtree->insert_in_frame( parent_frame, f_dum );

    if ( ! res )
    {
        delete f_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_insert_visobject( int parent_frame,
                                         VisualObject2d * v_dum )
{
    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_insert_point( int parent_frame,
                                     int this_object,
                                     const Point2d & obj,
                                     int layer,
                                     const RGBcolor & col )
{
    VisualObject2d * v_dum = new VisualPoint2d( this_object, layer, col, obj );
    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

bool
BuilderDirect::set_cmd_insert_points( int parent_frame,
                                      int this_object,
                                      int len_data,
                                      const Point2d * data,
                                      int layer,
                                      const RGBcolor & col )
{
    VisualObject2d * v_dum = new VisualPoints2d( this_object, layer, col, len_data, data );
    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_insert_line( int parent_frame,
                                    int this_object,
                                    const Line2d & obj,
                                    int layer,
                                    const RGBcolor & col )
{
    VisualObject2d * v_dum = new VisualLine2d( this_object, layer, col, obj );
    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

bool
BuilderDirect::set_cmd_insert_lines( int parent_frame,
                                     int this_object,
                                     int len_data,
                                     const Line2d * data,
                                     int layer,
                                     const RGBcolor & col )
{
    VisualObject2d * v_dum = new VisualLines2d( this_object, layer, col, len_data, data );
    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_insert_circle( int parent_frame,
                                      int this_object,
                                      const Circle2d & obj,
                                      int layer,
                                      const RGBcolor & col )
{
    VisualObject2d * v_dum = new VisualCircle2d( this_object, layer, col, obj );
    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

bool
BuilderDirect::set_cmd_insert_circles( int parent_frame,
                                       int this_object,
                                       int len_data,
                                       const Circle2d * data,
                                       int layer,
                                       const RGBcolor & col )
{
    VisualObject2d * v_dum = new VisualCircles2d( this_object, layer, col, len_data, data );
    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

bool
BuilderDirect::set_cmd_insert_f_circle( int parent_frame,
                                        int this_object,
                                        const Circle2d & obj,
                                        int layer,
                                        const RGBcolor & col )
{
    VisualCircle2d * v_dum = new VisualCircle2d( this_object, layer, col, obj );
    v_dum->filled = true;

    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

bool
BuilderDirect::set_cmd_insert_f_circles( int parent_frame,
                                         int this_object,
                                         int len_data,
                                         const Circle2d * data,
                                         int layer,
                                         const RGBcolor & col )
{
    VisualCircles2d * v_dum = new VisualCircles2d( this_object, layer, col, len_data, data );
    v_dum->filled = true;

    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_insert_circlearc( int parent_frame,
                                         int this_object,
                                         const CircleArc2d & obj,
                                         int layer,
                                         const RGBcolor & col )
{
    VisualObject2d * v_dum = new VisualCircleArc2d( this_object, layer, col, obj );
    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( !res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

bool
BuilderDirect::set_cmd_insert_circlearcs( int parent_frame,
                                          int this_object,
                                          int len_data,
                                          const CircleArc2d * data,
                                          int layer,
                                          const RGBcolor & col )
{
    VisualObject2d * v_dum = new VisualCircleArcs2d( this_object, layer, col, len_data, data );
    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

bool
BuilderDirect::set_cmd_insert_f_circlearc( int parent_frame,
                                           int this_object,
                                           const CircleArc2d & obj,
                                           int layer,
                                           const RGBcolor & col )
{
    VisualCircleArc2d * v_dum = new VisualCircleArc2d( this_object, layer, col, obj );
    v_dum->filled = true;

    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

bool
BuilderDirect::set_cmd_insert_f_circlearcs( int parent_frame,
                                            int this_object,
                                            int len_data,
                                            const CircleArc2d * data,
                                            int layer,
                                            const RGBcolor & col )
{
    VisualCircleArcs2d * v_dum = new VisualCircleArcs2d( this_object, layer, col, len_data, data );
    v_dum->filled = true;

    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_insert_polyline( int parent_frame,
                                        int this_object,
                                        int len_data,
                                        const Point2d * data,
                                        int layer,
                                        const RGBcolor & col )
{
    VisualObject2d * v_dum = new VisualPolyline2d( this_object, layer, col, len_data, data );
    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_insert_polygon( int parent_frame,
                                       int this_object,
                                       int len_data,
                                       const Point2d * data,
                                       int layer,
                                       const RGBcolor & col )
{
    VisualObject2d * v_dum = new VisualPolygon2d( this_object, layer, col, len_data, data );
    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

bool
BuilderDirect::set_cmd_insert_f_polygon( int parent_frame,
                                         int this_object,
                                         int len_data,
                                         const Point2d * data,
                                         int layer,
                                         const RGBcolor & col )
{
    VisualPolygon2d * v_dum = new VisualPolygon2d( this_object, layer, col, len_data, data );
    v_dum->filled = true;

    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_insert_string( int parent_frame,
                                      int this_object,
                                      const Point2d & pos,
                                      int len_data,
                                      const char * data,
                                      int layer,
                                      const RGBcolor & col )
{
    VisualObject2d * v_dum = new VisualString2d( this_object, layer, col, pos, len_data, data );
    bool res = dtree->insert_in_frame( parent_frame, v_dum );

    if ( ! res )
    {
        delete v_dum;
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_set_object_visible( int frame,
                                           int this_object,
                                           int vis )
{
    B_ERROR( "BuilderDirect: not yet implemented" );
    return false;
}

///
bool
BuilderDirect::set_cmd_set_object_layer( int frame,
                                         int this_object,
                                         int layer )
{
    B_ERROR( "BuilderDirect: not yet implemented" );
    return false;
}

///
bool
BuilderDirect::set_cmd_set_object_color( int frame,
                                         int this_object,
                                         const RGBcolor & col )
{
    bool res = dtree->set_object_color( frame, this_object, col );

    if ( ! res )
    {
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_set_frame_visible( int this_frame,
                                          int visible )
{
    bool res = dtree->set_visible( this_frame, visible );

    if ( ! res )
    {
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_set_frame_layer( int this_frame,
                                        int layer )
{
    bool res = dtree->set_layer( this_frame, layer );

    if ( ! res )
    {
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_set_frame_pos( int this_frame,
                                      const Point2d & pos )
{
    bool res = dtree->set_position( this_frame, pos.x, pos.y );

    if ( ! res )
    {
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_set_frame_ang( int this_frame,
                                      const Angle & ang )
{
    bool res = dtree->set_angle( this_frame, ang );

    if ( ! res )
    {
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_set_frame_scale( int this_frame,
                                        double scale )
{
    bool res = dtree->set_scale( this_frame, scale );

    if ( ! res )
    {
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_set_frame_pos_ang( int this_frame,
                                          const Point2d & pos,
                                          const Angle & ang )
{
    bool res = dtree->set_pos_ang( this_frame, pos.x, pos.y, ang );

    if ( ! res )
    {
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_remove_frame( int this_frame )
{
    bool res = dtree->remove_frame( this_frame );

    if ( ! res )
    {
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_remove_object( int frame,
                                      int this_object )
{
    bool res = dtree->remove_in_frame( frame, this_object );

    if ( ! res )
    {
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_empty_frame( int this_frame )
{
    bool res = dtree->empty_frame( this_frame );

    if ( ! res )
    {
        B_ERROR( "BuilderDirect: some error occured" );
    }

    return res;
}

///
bool
BuilderDirect::set_cmd_set_view_area( const Area2d & a )
{
    c_area->set_area( a );
    return true;
}

///
bool
BuilderDirect::set_cmd_set_status_line( int len_data,
                                        const char * data )
{
    //BUG BUG!!! here a copy of mul.tab should be used!!!
    status_line = data;
    status_line_len = len_data;
    return true;
};

///
bool
BuilderDirect::set_cmd_set_background_color( const RGBcolor & col )
{
    bg_color = col;
    new_bg_color = true;
    return true;
}
