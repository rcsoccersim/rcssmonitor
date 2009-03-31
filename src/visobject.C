/*
 * Copyright (c ) 1999 - 2001, Artur Merke <amerke@ira.uka.de>
 *
 * This file is part of FrameView2d.
 *
 * FrameView2d is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option )
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

#include "visobject.h"

VisualObject2d::VisualObject2d()
    : key( 0 ),
      layer( 0 ),
      visible( true )
{

}

#if 0
void
VisualObject2d::set_color( const RGBcolor & col )
{
    if ( color != col )
    {
        color = col;

        if ( d_obj )
            d_obj->mark_col_change();
    }
}

#endif

VisualPoint2d::VisualPoint2d()
    : VisualObject2d()
{
    init();
}

VisualPoint2d::VisualPoint2d( const int my_key,
                              const int my_layer,
                              const RGBcolor & my_color,
                              const Point2d & my_data )
    : VisualObject2d()
{
    init();

    key = my_key;
    layer = my_layer;
    color = my_color;
    rel = my_data;
}

void
VisualPoint2d::init()
{
    changed = true;
    use_intersects_area = true;
    key = 0;
    layer = 0;
    d_obj = static_cast< DisplayObject * >( 0 );
}

void
VisualPoint2d::draw( DisplayBase * disp,
                     const Area2d & area,
                     const Frame2d & p_frame,
                     const bool chg )
{
    if ( ! visible )
    {
        return;
    }

    actualize( p_frame, chg );

    if ( use_intersects_area
         && ! intersects_area( area ) )
    {
        return;
    }

    if ( ! d_obj )
    {
        d_obj = disp->create_point();
    }

    disp->draw_point( d_obj, color, abs );
}

void
VisualPoint2d::actualize( const Frame2d & f,
                          const bool chg )
{
    if ( ! chg && ! changed )
    {
        return;
    }

    //abs = f * rel;
    abs = f.transform( rel );

    if ( d_obj )
    {
        d_obj->mark_pos_change();
    }

    changed = false;
}

void
VisualPoint2d::set_color( const RGBcolor & col )
{
    if ( color != col )
    {
        color = col;

        if ( d_obj )
        {
            d_obj->mark_col_change();
        }
    }
}

bool
VisualPoint2d::intersects_area( const Area2d & a )
{
    return a.intersects( abs );
}

////
VisualPoints2d::VisualPoints2d()
    : VisualObject2d()
{
    init();
}

VisualPoints2d::VisualPoints2d( const int my_key,
                                const int my_layer,
                                const RGBcolor & my_color,
                                const int len_data,
                                const Point2d * data )
    : VisualObject2d()
{
    init();

    key = my_key;
    layer = my_layer;
    color = my_color;

    rel.clone( len_data, data );
    abs = rel;
}

void
VisualPoints2d::init()
{
    changed = true;
    use_intersects_area = true;
    key = 0;
    layer = 0;
    d_obj = static_cast< DisplayObject * >( 0 );
}

void
VisualPoints2d::draw( DisplayBase * disp,
                      const Area2d & area,
                      const Frame2d & p_frame,
                      const bool chg )
{
    if ( ! visible )
    {
        return;
    }

    actualize( p_frame, chg );

    if ( use_intersects_area
         && ! intersects_area( area ) )
    {
        return;
    }

    if ( ! d_obj )
    {
        d_obj = disp->create_points( abs.max_size );
    }

    disp->draw_points( d_obj, color, abs.cur_size, abs.tab );
}

void
VisualPoints2d::actualize( const Frame2d & f,
                           const bool chg )
{
    if ( ! chg && ! changed )
    {
        return;
    }

    for ( int i = 0; i < abs.cur_size; ++i )
    {
        //abs.tab[i] = f * rel.tab[i];
        abs.tab[i] = f.transform( rel.tab[i] );
    }

    if ( d_obj )
    {
        d_obj->mark_pos_change();
    }

    changed = false;
}

void
VisualPoints2d::set_color( const RGBcolor & col )
{
    if ( color != col )
    {
        color = col;

        if ( d_obj )
        {
            d_obj->mark_col_change();
        }
    }
}

void
VisualPoints2d::set_points( const int len_data,
                            const Point2d * data )
{
    if ( d_obj )
    {
        d_obj->set_max_size( len_data );
    }

    rel.clone( len_data, data );

    abs = rel;
    changed = true;
}

bool
VisualPoints2d::intersects_area( const Area2d & a )
{
    for ( int i = 0; i < abs.cur_size; ++i )
    {
        if ( a.intersects( abs.tab[i] ) ) return true;
    }

    return false;
}

/*****************************************************************************/

VisualLine2d::VisualLine2d()
    : VisualObject2d()
{
    init();
}

VisualLine2d::VisualLine2d( const int my_key,
                            const int my_layer,
                            const RGBcolor & my_color,
                            const Line2d & my_data )
    : VisualObject2d()
{
    init();

    key = my_key;
    layer = my_layer;
    color = my_color;

    rel = my_data;
}

void
VisualLine2d::init()
{
    changed = true;
    use_intersects_area = true;
    key = 0;
    layer = 0;
    d_obj = static_cast< DisplayObject * >( 0 );
}

void
VisualLine2d::draw( DisplayBase * disp,
                    const Area2d & area,
                    const Frame2d & p_frame,
                    const bool chg )
{
    if ( ! visible )
    {
        return;
    }

    actualize( p_frame, chg );

    if ( use_intersects_area
         && ! intersects_area( area ) )
    {
        return;
    }

    if ( ! d_obj )
    {
        d_obj = disp->create_line();
    }

    disp->draw_line( d_obj, color, abs );
}

void
VisualLine2d::actualize( const Frame2d & f,
                         const bool chg )
{
    if ( ! chg
         && ! changed )
    {
        return;
    }

    //abs.p1 = f * rel.p1;
    abs.p1 = f.transform( rel.p1 );
    //abs.p2 = f * rel.p2;
    abs.p2 = f.transform( rel.p2 );

    if ( d_obj )
    {
        d_obj->mark_pos_change();
    }

    changed = false;
}

void
VisualLine2d::set_color( const RGBcolor & col )
{
    if ( color != col )
    {
        color = col;

        if ( d_obj )
        {
            d_obj->mark_col_change();
        }
    }
}

bool
VisualLine2d::intersects_area( const Area2d & a )
{
    return a.intersects( abs );
}

////
VisualLines2d::VisualLines2d()
    : VisualObject2d()
{
    init();
}

VisualLines2d::VisualLines2d( const int my_key,
                              const int my_layer,
                              const RGBcolor & my_color,
                              const int len_data,
                              const Line2d * data )
    : VisualObject2d()
{
    init();

    key = my_key;
    layer = my_layer;
    color = my_color;

    rel.clone( len_data, data );
    abs = rel;
    d_obj = static_cast< DisplayObject * >( 0 );
}

void
VisualLines2d::init()
{
    changed = true;
    use_intersects_area = true;
    key = 0;
    layer = 0;
    d_obj = static_cast< DisplayObject * >( 0 );
}

void
VisualLines2d::draw( DisplayBase * disp,
                     const Area2d & area,
                     const Frame2d & p_frame,
                     const bool chg )
{
    if ( ! visible )
    {
        return;
    }

    actualize( p_frame, chg );

    if ( use_intersects_area
         && ! intersects_area( area ) )
    {
        return;
    }

    if ( ! d_obj )
    {
        d_obj = disp->create_lines( abs.max_size );
    }

    disp->draw_lines( d_obj, color, abs.cur_size, abs.tab );
}

void
VisualLines2d::actualize( const Frame2d & f,
                          const bool chg )
{
    if ( ! chg && ! changed )
    {
        return;
    }

    for ( int i = 0; i < abs.cur_size; ++i )
    {
        // abs.tab[i].p1 = f * rel.tab[i].p1;
        // abs.tab[i].p2 = f * rel.tab[i].p2;
        abs.tab[i].p1 = f.transform( rel.tab[i].p1 );
        abs.tab[i].p2 = f.transform( rel.tab[i].p2 );
    }

    if ( d_obj )
    {
        d_obj->mark_pos_change();
    }

    changed = false;
}

void
VisualLines2d::set_color( const RGBcolor & col )
{
    if ( color != col )
    {
        color = col;

        if ( d_obj )
        {
            d_obj->mark_col_change();
        }
    }
}

void
VisualLines2d::set_lines( const int len_data,
                          const Line2d * data )
{
    if ( d_obj )
    {
        d_obj->set_max_size( len_data );
    }

    rel.clone( len_data, data );

    abs = rel;
    changed = true;
}

bool
VisualLines2d::intersects_area( const Area2d & a )
{
    for ( int i = 0; i < abs.cur_size; ++i )
    {
        if ( a.intersects( abs.tab[i] ) ) return true;
    }

    return false;
}

/*****************************************************************************/

VisualCircle2d::VisualCircle2d()
{
    init();
}

VisualCircle2d::VisualCircle2d( const int my_key,
                                const int my_layer,
                                const RGBcolor & my_color,
                                const Circle2d & my_data )
    : VisualObject2d()
{
    init();

    key = my_key;
    layer = my_layer;
    color = my_color;

    rel = my_data;
    abs = rel;
}

void
VisualCircle2d::init()
{
    changed = true;
    use_intersects_area = true;
    key = 0;
    layer = 0;
    d_obj = static_cast< DisplayObject * >( 0 );
    filled = false;
}

void
VisualCircle2d::draw( DisplayBase * disp,
                      const Area2d & area,
                      const Frame2d & p_frame,
                      const bool chg )
{
    if ( ! visible )
    {
        return;
    }

    actualize( p_frame, chg );

    if ( use_intersects_area
         && ! intersects_area( area ) )
    {
        return;
    }

    if ( ! d_obj )
    {
        d_obj = disp->create_circle();
    }

    if ( ! filled )
    {
        disp->draw_circle( d_obj, color, abs );
    }
    else
    {
        disp->fill_circle( d_obj, color, abs );
    }
}

void
VisualCircle2d::actualize( const Frame2d & f,
                           const bool chg )
{
    if ( ! chg && ! changed )
    {
        return;
    }

    //abs.center = f * rel.center;
    abs.center = f.transform( rel.center );

    abs.radius = f.get_scale() * rel.radius;

    if ( d_obj )
    {
        d_obj->mark_pos_change();
    }

    changed = false;
}

void
VisualCircle2d::set_color( const RGBcolor & col )
{
    if ( color != col )
    {
        color = col;

        if ( d_obj )
        {
            d_obj->mark_col_change();
        }
    }
}

bool
VisualCircle2d::intersects_area( const Area2d & a )
{
    if ( ! filled )
    {
        return a.intersects( abs );
    }

    return a.intersects_area_of( abs );
}

////
VisualCircles2d::VisualCircles2d()
    : VisualObject2d()
{
    init();
}

VisualCircles2d::VisualCircles2d( const int my_key,
                                  const int my_layer,
                                  const RGBcolor & my_color,
                                  const int len_data,
                                  const Circle2d * data )
    : VisualObject2d()
{
    init();

    key = my_key;
    layer = my_layer;
    color = my_color;

    rel.clone( len_data, data );
    abs = rel;
}

void
VisualCircles2d::init()
{
    changed = true;
    use_intersects_area = true;
    key = 0;
    layer = 0;
    d_obj = static_cast< DisplayObject * >( 0 );
    filled = false;
}

void
VisualCircles2d::draw( DisplayBase * disp,
                       const Area2d & area,
                       const Frame2d & p_frame,
                       const bool chg )
{
    if ( ! visible )
    {
        return;
    }

    actualize( p_frame, chg );

    if ( use_intersects_area
         && ! intersects_area( area ) )
    {
        return;
    }

    if ( ! d_obj )
    {
        d_obj = disp->create_circles( abs.max_size );
    }

    if ( ! filled )
    {
        disp->draw_circles( d_obj, color, abs.cur_size, abs.tab );
    }
    else
    {
        disp->fill_circles( d_obj, color, abs.cur_size, abs.tab );
    }
}

void
VisualCircles2d::actualize( const Frame2d & f,
                            const bool chg )
{
    if ( ! chg && ! changed )
    {
        return;
    }

    for ( int i = 0; i < abs.cur_size; ++i )
    {
        //abs.tab[i].center = f * rel.tab[i].center;
        abs.tab[i].center = f.transform( rel.tab[i].center );
        abs.tab[i].radius = f.get_scale() * rel.tab[i].radius;
    }

    if ( d_obj )
    {
        d_obj->mark_pos_change();
    }

    changed = false;
}

void
VisualCircles2d::set_color( const RGBcolor & col )
{
    if ( color != col )
    {
        color = col;

        if ( d_obj )
        {
            d_obj->mark_col_change();
        }
    }
}

void
VisualCircles2d::set_circles( const int len_data,
                              const Circle2d * data )
{
    if ( d_obj )
    {
        d_obj->set_max_size( len_data );
    }

    rel.clone( len_data, data );

    abs = rel;
    changed = true;
}

bool
VisualCircles2d::intersects_area( const Area2d & a )
{
    if ( ! filled )
    {
        for ( int i = 0; i < abs.cur_size; ++i )
        {
            if ( a.intersects( abs.tab[i] ) ) return true;
        }
    }
    else
    {
        for ( int i = 0; i < abs.cur_size; ++i )
        {
            if ( a.intersects_area_of( abs.tab[i] ) ) return true;
        }
    }

    return false;
}

/*****************************************************************************/

VisualCircleArc2d::VisualCircleArc2d()
{
    init();
}

VisualCircleArc2d::VisualCircleArc2d( const int my_key,
                                      const int my_layer,
                                      const RGBcolor & my_color,
                                      const CircleArc2d & my_data )
    : VisualObject2d()
{
    init();

    key = my_key;
    layer = my_layer;
    color = my_color;

    rel = my_data;
    abs = rel;
}

void
VisualCircleArc2d::init()
{
    changed = true;
    use_intersects_area = true;
    key = 0;
    layer = 0;
    d_obj = static_cast< DisplayObject * >( 0 );
    filled = false;
}

void VisualCircleArc2d::draw( DisplayBase * disp,
                              const Area2d & area,
                              const Frame2d & p_frame,
                              const bool chg )
{
    if ( ! visible )
    {
        return;
    }

    actualize( p_frame, chg );

    if ( use_intersects_area
         && ! intersects_area( area ) )
    {
        return;
    }

    if ( ! d_obj )
    {
        d_obj = disp->create_circlearc();
    }

    if ( ! filled )
    {
        disp->draw_circlearc( d_obj, color, abs );
    }
    else
    {
        disp->fill_circlearc( d_obj, color, abs );
    }
}

void
VisualCircleArc2d::actualize( const Frame2d & f,
                              const bool chg )
{
    if ( ! chg && ! changed )
    {
        return;
    }

    //abs.center = f * rel.center;
    abs.center = f.transform( rel.center );

    abs.radius = f.get_scale() * rel.radius;

    Angle ang = f.get_angle();
    abs.ang1 = ang + rel.ang1;
    abs.ang2 = ang + rel.ang2;

    if ( d_obj )
    {
        d_obj->mark_pos_change();
    }

    changed = false;
}

void
VisualCircleArc2d::set_color( const RGBcolor & col )
{
    if ( color != col )
    {
        color = col;

        if ( d_obj )
        {
            d_obj->mark_col_change();
        }
    }
}

bool
VisualCircleArc2d::intersects_area( const Area2d & a )
{
    if ( ! filled )
    {
        return a.intersects( abs );
    }

    return a.intersects_area_of( abs );
}

////
VisualCircleArcs2d::VisualCircleArcs2d()
    : VisualObject2d()
{
    init();
}

VisualCircleArcs2d::VisualCircleArcs2d( const int my_key,
                                        const int my_layer,
                                        const RGBcolor & my_color,
                                        const int len_data,
                                        const CircleArc2d * data )
    : VisualObject2d()
{
    init();

    key = my_key;
    layer = my_layer;
    color = my_color;

    rel.clone( len_data, data );
    abs = rel;
}

void
VisualCircleArcs2d::init()
{
    changed = true;
    use_intersects_area = true;
    key = 0;
    layer = 0;
    d_obj = static_cast< DisplayObject * >( 0 );
    filled = false;
}

void
VisualCircleArcs2d::draw( DisplayBase * disp,
                          const Area2d & area,
                          const Frame2d & p_frame,
                          const bool chg )
{
    if ( ! visible )
    {
        return;
    }

    actualize( p_frame, chg );

    if ( use_intersects_area
         && ! intersects_area( area ) )
    {
        return;
    }

    if ( ! d_obj )
    {
        d_obj = disp->create_circlearcs( abs.max_size );
    }

    if ( ! filled )
    {
        disp->draw_circlearcs( d_obj, color, abs.cur_size, abs.tab );
    }
    else
    {
        disp->fill_circlearcs( d_obj, color, abs.cur_size, abs.tab );
    }
}

void
VisualCircleArcs2d::actualize( const Frame2d & f,
                               const bool chg )
{
    if ( ! chg && ! changed )
    {
        return;
    }

    for ( int i = 0; i < abs.cur_size; ++i )
    {
        //abs.tab[i].center = f * rel.tab[i].center;
        abs.tab[i].center = f.transform( rel.tab[i].center );
        abs.tab[i].radius = f.get_scale() * rel.tab[i].radius;
        Angle ang = f.get_angle();
        abs.tab[i].ang1 = ang + rel.tab[i].ang1;
        abs.tab[i].ang2 = ang + rel.tab[i].ang2;
    }

    if ( d_obj )
    {
        d_obj->mark_pos_change();
    }

    changed = false;
}

void
VisualCircleArcs2d::set_color( const RGBcolor & col )
{
    if ( color != col )
    {
        color = col;

        if ( d_obj )
        {
            d_obj->mark_col_change();
        }
    }
}

void
VisualCircleArcs2d::set_circlearcs( const int len_data,
                                    const CircleArc2d * data )
{
    if ( d_obj )
    {
        d_obj->set_max_size( len_data );
    }

    rel.clone( len_data, data );

    abs = rel;
    changed = true;
}

bool
VisualCircleArcs2d::intersects_area( const Area2d & a )
{
    if ( !filled )
    {
        for ( int i = 0; i < abs.cur_size; i++ )
        {
            if ( a.intersects( abs.tab[i] ) ) return true;
        }
    }
    else
    {
        for ( int i = 0; i < abs.cur_size; i++ )
        {
            if ( a.intersects_area_of( abs.tab[i] ) ) return true;
        }
    }

    return false;
}

/*****************************************************************************/

VisualPolyline2d::VisualPolyline2d()
    : VisualObject2d()
{
    init();
}

VisualPolyline2d::VisualPolyline2d( const int my_key,
                                    const int my_layer,
                                    const RGBcolor & my_color,
                                    const int len_data,
                                    const Point2d * data )
    : VisualObject2d()
{
    init();

    key = my_key;
    layer = my_layer;
    color = my_color;

    rel.clone( len_data, data );
    abs = rel;
}

void
VisualPolyline2d::init()
{
    changed = true;
    use_intersects_area = true;
    key = 0;
    layer = 0;
    d_obj = static_cast< DisplayObject * >( 0 );
}

void
VisualPolyline2d::draw( DisplayBase * disp,
                        const Area2d & area,
                        const Frame2d & p_frame,
                        const bool chg )
{
    if ( ! visible )
    {
        return;
    }

    actualize( p_frame, chg );

    if ( use_intersects_area
         && !intersects_area( area ) )
    {
        return;
    }

    if ( ! d_obj )
    {
        d_obj = disp->create_polyline( abs.max_size );
    }

    disp->draw_polyline( d_obj, color, abs.cur_size, abs.tab );
}

void
VisualPolyline2d::actualize( const Frame2d & f,
                             const bool chg )
{
    if ( ! chg && ! changed )
    {
        return;
    }

    for ( int i = 0; i < rel.cur_size; ++i )
    {
        //abs.tab[i] = f * rel.tab[i];
        abs.tab[i] = f.transform( rel.tab[i] );
    }

    if ( d_obj )
    {
        d_obj->mark_pos_change();
    }

    changed = false;
}

void
VisualPolyline2d::set_color( const RGBcolor & col )
{
    if ( color != col )
    {
        color = col;

        if ( d_obj )
        {
            d_obj->mark_col_change();
        }
    }
}

void
VisualPolyline2d::set_points( const int len_data,
                              const Point2d * data )
{
    if ( d_obj )
    {
        d_obj->set_max_size( len_data );
    }

    rel.clone( len_data, data );

    abs = rel;
    changed = true;
}

bool
VisualPolyline2d::intersects_area( const Area2d & a )
{
    //this routine has a bug:
    //it the vertices of the polyline are out of the area a, then the corresponding line
    //segment will not be drawn ( but acutally there are situation, where the line segment
    //is visible!
    for ( int i = 0; i < abs.cur_size; ++i )
    {
        if ( a.intersects( abs.tab[i] ) ) return true;
    }

    return false;
}

/*****************************************************************************/

VisualPolygon2d::VisualPolygon2d()
    : VisualObject2d()
{
    init();
}

VisualPolygon2d::VisualPolygon2d( const int my_key,
                                  const int my_layer,
                                  const RGBcolor & my_color,
                                  const int len_data,
                                  const Point2d * data )
    : VisualObject2d()
{
    init();

    key = my_key;
    layer = my_layer;
    color = my_color;

    rel.clone( len_data, data );
    abs = rel;
}

void
VisualPolygon2d::init()
{
    changed = true;
    use_intersects_area = true;
    key = 0;
    layer = 0;
    d_obj = static_cast< DisplayObject * >( 0 );
    filled = false;
}

void
VisualPolygon2d::draw( DisplayBase * disp,
                       const Area2d & area,
                       const Frame2d & p_frame,
                       const bool chg )
{
    if ( ! visible )
    {
        return;
    }

    actualize( p_frame, chg );

    if ( use_intersects_area
         && ! intersects_area( area ) )
    {
        return;
    }

    if ( ! d_obj )
    {
        d_obj = disp->create_polygon( abs.max_size );
    }

    if ( filled )
    {
        disp->fill_polygon( d_obj, color, abs.cur_size, abs.tab );
    }
    else
    {
        disp->draw_polygon( d_obj, color, abs.cur_size, abs.tab );
    }
}

void
VisualPolygon2d::actualize( const Frame2d & f,
                            const bool chg )
{
    if ( ! chg && ! changed )
    {
        return;
    }

    for ( int i = 0; i < rel.cur_size; ++i )
    {
        abs.tab[i] = f.transform( rel.tab[i] );
    }

    if ( d_obj )
    {
        d_obj->mark_pos_change();
    }

    changed = false;
}

void
VisualPolygon2d::set_color( const RGBcolor & col )
{
    if ( color != col )
    {
        color = col;

        if ( d_obj )
        {
            d_obj->mark_col_change();
        }
    }
}

void
VisualPolygon2d::set_points( const int len_data,
                             const Point2d * data )
{
    if ( d_obj )
    {
        d_obj->set_max_size( len_data );
    }

    rel.clone( len_data, data );

    abs = rel;
    changed = true;
}

bool
VisualPolygon2d::intersects_area( const Area2d & a )
{
    if ( ! filled )
    {
        for ( int i = 0; i < abs.cur_size; i++ )
        {
            if ( a.intersects( abs.tab[i] ) ) return true;
        }
    }
    else
    {
        return true; // a better decision is not yet implemented
    }

    return false;
}

/*****************************************************************************/

void VisualString2d::init()
{
    key = 0;
    layer = 0;
    d_obj = static_cast< DisplayObject * >( 0 );
    changed = true;
    use_intersects_area = true;
}

VisualString2d::VisualString2d( const int my_key,
                                const int my_layer,
                                const RGBcolor & my_color,
                                const Point2d & my_pos,
                                //const Multi< char > & my_data )
                                const std::string & my_data )
    : VisualObject2d()
{
    init();

    key = my_key;
    layer = my_layer;
    color = my_color;

    rel = my_pos;
    content = my_data;
}

VisualString2d::VisualString2d( const int my_key,
                                const int my_layer,
                                const RGBcolor & my_color,
                                const Point2d & my_pos,
                                const int len_data,
                                const char * data )
    : VisualObject2d()
{
    init();

    key = my_key;
    layer = my_layer;
    color = my_color;

    rel = my_pos;
    //content.clone( len_data, data );
    content.assign( data, len_data );
}

VisualString2d::VisualString2d()
    : VisualObject2d()
{
    init();
}

VisualString2d::VisualString2d( const int my_key,
                                const int my_layer )
    : VisualObject2d()
{
    init();

    key = my_key;
    layer = my_layer;
}

void
VisualString2d::draw( DisplayBase * disp,
                      const Area2d & area,
                      const Frame2d & p_frame,
                      const bool chg )
{
    if ( ! visible )
    {
        return;
    }

    actualize( p_frame, chg );

    if ( use_intersects_area
         && ! intersects_area( area ) )
    {
        return;
    }

    if ( ! d_obj )
    {
        d_obj = disp->create_string();
    }

    //disp->draw_string( d_obj, color, abs, content.cur_size, content.tab );
    disp->draw_string( d_obj, color, abs, content.length(), content.c_str() );
}


void
VisualString2d::actualize( const Frame2d & f,
                           const bool chg )
{
    if ( ! chg && ! changed )
    {
        return;
    }

    //abs = f * rel;
    abs = f.transform( rel );

    if ( d_obj )
    {
        d_obj->mark_pos_change();
    }

    changed = false;
}

void
VisualString2d::set_color( const RGBcolor & col )
{
    if ( color != col )
    {
        color = col;

        if ( d_obj )
        {
            d_obj->mark_col_change();
        }
    }
}

bool
VisualString2d::intersects_area( const Area2d & a )
{
    return a.intersects( abs );
    //return true; // a better decision is not yet implemented
}
