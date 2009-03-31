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

#include "structure.h"
#include <iostream>

#if 0
#include "display_base.h"
extern DisplayBase *DDD
#endif


/*****************************************************************************/

DrawFrame::DrawFrame( int key )
    : M_key( key ),
    M_parent( static_cast< DrawFrame * >( 0 ) ),
    M_visible( true ),
    M_changed( true ),
    M_layer( 0 )
{

}

DrawFrame::~DrawFrame()
{
    //cout << "\nF.key= " << key;
}


void
DrawFrame::init( const Point2d & translation,
                 const Angle & rotation,
                 const int layer )
{
    M_rel_transform = Frame2d::Translation( translation.x, translation.y ) * M_rel_transform;
    M_rel_transform = M_rel_transform * Frame2d::Rotation( rotation );

    M_layer = layer;
}

bool
DrawFrame::insert( DrawFrame * f )
{
    f->M_parent = this;
    return M_frames.insert( f );
}

bool
DrawFrame::remove( DrawFrame * f )
{
    bool res = M_frames.remove( f );
    //delete f;
    return res;
}

bool
DrawFrame::insert( VisualObject2d * f )
{
    return M_objects.insert( f );
}

bool
DrawFrame::remove( VisualObject2d * f )
{
    bool res = M_objects.remove( f );
    //delete f;
    return res;
}

bool
DrawFrame::remove_all( DrawFrameMap & fmap )
{
    KeyMap_LayerList< DrawFrame >::Iterator f_itr( M_frames );
    KeyMap_LayerList< VisualObject2d >::Iterator o_itr( M_objects );

    DrawFrame * f_dum;
    while ( f_dum = f_itr.get_next() )
    {
        fmap.remove( f_dum->get_key() ); //remove the key of this frame from the cache.
        f_dum->remove_all( fmap );
        delete f_dum;
    }

    VisualObject2d * o_dum;
    while ( o_dum = o_itr.get_next() )
    {
        //cout << "\nObject id= " << o_dum->key << flush;
        delete o_dum;
    }

    M_frames.remove_all(); //delete the frames list
    M_objects.remove_all();//delete the objects list
    return true;
}

void
DrawFrame::actualize( const Frame2d & p_frame,
                      const bool chg )
{
    M_changed = M_changed || chg;

    if ( ! M_visible ) return;

    KeyMap_LayerList< DrawFrame >::Iterator f_itr( M_frames );
    KeyMap_LayerList< VisualObject2d >::Iterator o_itr( M_objects );

    if ( M_changed )
    {
        if ( M_parent )
        {
            //M_abs_transform= M_parent->M_abs_transform * M_rel_transform;
            M_abs_transform = p_frame * M_rel_transform;
        }
        else
        {
            M_abs_transform = M_rel_transform;
        }
    }

    DrawFrame * f_dum;
    while ( f_dum = f_itr.get_next() )
    {
        f_dum->actualize( M_abs_transform, M_changed );
    }

    VisualObject2d * o_dum;
    while ( o_dum = o_itr.get_next() )
    {
        o_dum->actualize( M_abs_transform, M_changed );
    }

    M_changed = false;
}

void
DrawFrame::draw( DisplayBase * disp,
                 const Area2d & area,
                 const Frame2d & p_frame,
                 const bool chg )
{
    M_changed = M_changed || chg;

    if ( ! M_visible )
    {
        return;
    }

    //cout << "\n " << key <<  " ----Frame: ";
    KeyMap_LayerList< DrawFrame >::Iterator f_itr( M_frames );
    KeyMap_LayerList< VisualObject2d >::Iterator o_itr( M_objects );

    if ( M_changed )
    {
        if ( M_parent )
        {
            //M_abs_transform= M_parent->M_abs_transform * M_rel_transform;
            M_abs_transform = p_frame * M_rel_transform;
        }
        else
        {
            M_abs_transform = M_rel_transform;
        }
    }

    //frames teke precedence over objects it they are in the same layer
    //cout << "\n\n>>>in frame: " << key << "\n";

    DrawFrame * f_dum;
    VisualObject2d * o_dum = o_itr.get_next();
    while ( f_dum = f_itr.get_next() )
    {
        while ( o_dum
                && o_dum->get_layer() < f_dum->get_layer() )
        {
            o_dum->draw( disp, area, M_abs_transform, M_changed );
            //cout << "\nobj_in: " << *o_dum;
            o_dum = o_itr.get_next();
        }

        //cout << "\n>frame: " << *f_dum;
        f_dum->draw( disp, area, M_abs_transform, M_changed );
    }

    while ( o_dum )
    {
        o_dum->draw( disp, area, M_abs_transform, M_changed );
        //cout << "\nobj_out: "<< *o_dum;
        o_dum = o_itr.get_next();
    }

#if 0
    Vector2d P, X, Y;

    P = M_abs_transform.get_pos();
    X = P + M_abs_transform.get_x_axis();
    Y = P + M_abs_transform.get_y_axis();

    DDD->ASetForeground( "red" );
    DDD->ADrawLine( P.x, P.y, X.x, X.y );
    DDD->ASetForeground( "blue" );
    DDD->ADrawLine( P.x, P.y, Y.x, Y.y );

    //  DDD->ADrawString(P.x,P.y,1,"9");
#endif

    M_changed = false;
}

std::ostream &
DrawFrame::print( std::ostream & os ) const
{
    os << "Frame=[";

    if ( M_changed )
    {
        os << "chg, ";
    }

    if ( M_parent )
    {
        os << "up=" << M_parent->get_key() << ", ";
    }

    os << "key=" << get_key() << ", layer=" << get_layer()
        //<< "\n M_rel_transform= " << f.M_rel_transform
        //<< "\n M_abs_transform= " << f.M_abs_transform
       << "]";

    if ( false )
    {
        if ( ! M_objects.empty() )
        {
            os << "\n Objects={";
            os << M_objects;
        }
    }

    return os;
}

/*****************************************************************************/

DrawFrameMap::Item::Item( DrawFrame * f )
{
    frame = f;
    key = frame->get_key();
    next = 0;
}

DrawFrameMap::DrawFrameMap()
{
    M_list = static_cast< Item * >( 0 );

    for ( int i = 0; i < MAX_DIRECT; ++i )
    {
        M_direct_map[i] = static_cast< DrawFrame * >( 0 );
    }
}

bool
DrawFrameMap::insert( DrawFrame * f )
{
    int k = f->get_key();

    //
    // insert to the array frames (direct access frames)
    //
    if ( 0 <= k && k < MAX_DIRECT )
    {
        if ( M_direct_map[k] )
        {
            return false;
        }

        M_direct_map[k] = f;
        return true;
    }

    //
    // insert to the list frames
    //

    Item * ins = new Item( f );

    if ( ! M_list )
    {
        M_list = ins;
        return true;
    }

    if ( ins->key < M_list->key )
    {
        ins->next = M_list;
        M_list = ins;
        return true;
    }

    Item * itr = M_list;

    while ( itr->next
            && itr->next->key < ins->key )
    {
        itr = itr->next;
    }

    if ( ! itr->next )
    {
        itr->next = ins;
        return true;
    }

    if ( itr->next->key > ins->key )
    {
        ins->next = itr->next;
        itr->next = ins;
        return true;
    }

    std::cerr << "\nwarning: frame " << f->get_key() << " already in.";

    delete ins;
    return false;
}

bool
DrawFrameMap::remove( int key )
{
    //
    // remove from array
    //
    if ( 0 <= key && key < MAX_DIRECT )
    {
        M_direct_map[key] = static_cast< DrawFrame * >( 0 );
        return true;
    }

    //
    // remove from list
    //
    if ( ! M_list )
    {
        return true;
    }

    Item * itr = M_list;

    if ( itr->key > key )
    {
        return true;
    }
    else if ( itr->key == key )
    {
        M_list = itr->next;
        delete itr;
        return true;
    }

    while ( itr->next
            && itr->next->key < key )
    {
        itr = itr->next;
    }

    if ( ! itr->next )
    {
        return true;
    }

    if ( itr->next->key == key )
    {
        Item * dum = itr->next;
        itr->next = dum->next;
        delete dum;
    }

    return true;
}

DrawFrame *
DrawFrameMap::get( int key )
{
    //
    // get from array
    //
    if ( 0 <= key && key < MAX_DIRECT )
    {
        return M_direct_map[key];
    }

    //
    // get from list
    //
    Item * itr = M_list;

    while ( itr
            && itr->key < key )
    {
        itr = itr->next;
    }

    if ( ! itr )
    {
        return 0;
    }

    if ( itr->key != key )
    {
        return static_cast< DrawFrame * >( 0 );
    }

    return itr->frame;
}

std::ostream &
DrawFrameMap::print( std::ostream & os ) const
{
    //
    // print array frames
    //
    for ( int i = 0; i < MAX_DIRECT; ++i )
    {
        if ( M_direct_map[i] )
        {
            os << '\n';
            M_direct_map[i]->print( os );
        }
    }

    //
    // print list frames
    //
    DrawFrameMap::Item * itr = M_list;
    while ( itr )
    {
        //os << "\n" << *( itr->frame );
        itr->frame->print( os ) << '\n';
        itr = itr->next;
    }

    return os;
}

/*****************************************************************************/

DrawTree::DrawTree()
{
    M_origin = new DrawFrame( 0 );
    M_frame_map.insert( M_origin );
}

bool
DrawTree::insert_in_frame( const int fkey,
                           DrawFrame * f )
{
    DrawFrame * master = M_frame_map.get( fkey );

    if ( master
         && ! M_frame_map.get( f->get_key() ) )
    {
        if ( master->insert( f ) )
        {
            M_frame_map.insert( f );
            return true;
        }
    }

    return false;
}

bool
DrawTree::insert_in_frame( const int fkey,
                           VisualObject2d * f )
{
    DrawFrame * master = M_frame_map.get( fkey );

    if ( master )
    {
        return master->insert( f );
    }

    return false;
}

bool
DrawTree::remove_frame( const int fkey )
{
    DrawFrame * master = M_frame_map.get( fkey );

    if ( master
         && master->M_parent )
    {
        DrawFrame * parent = master->M_parent;
        M_frame_map.remove( master->get_key() );
        master->remove_all( M_frame_map ); // remove children frames
        delete master;
        return parent->remove( master );
    }

    return false;
}

bool
DrawTree::remove_in_frame( const int fkey,
                           const int fo_key )
{
    DrawFrame * master = M_frame_map.get( fkey );
    DrawFrame * slave = M_frame_map.get( fo_key );

    if ( master && slave && master != slave )
    {
        M_frame_map.remove( slave->get_key() );
        slave->remove_all( M_frame_map );
        delete slave;
        return master->remove( slave );
    }

    return false;
}

bool
DrawTree::empty_frame( const int fkey )
{
    DrawFrame * master = M_frame_map.get( fkey );

    if ( master )
    {
        master->remove_all( M_frame_map );
        return true;
    }

    return false;
}

bool
DrawTree::rotate_frame( const int fkey,
                        const Angle & ang )
{
    DrawFrame * master = M_frame_map.get( fkey );

    if ( ! master )
    {
        return false;
    }

    master->M_changed = true;
    master->M_rel_transform = master->M_rel_transform * Frame2d::Rotation( ang );

    return true;
}

bool
DrawTree::translate_in_frame( const int fkey,
                              const int fo_key,
                              const double & x,
                              const double & y )
{
    DrawFrame * master = M_frame_map.get( fkey );
    DrawFrame * slave = M_frame_map.get( fo_key );

    if ( ! master || ! slave || master == slave )
    {
        return false;
    }

    slave->M_changed = true;
    //eigentlich braucht man hier master nicht!!!
    slave->M_rel_transform = Frame2d::Translation( x, y ) * slave->M_rel_transform;

    return true;
}

bool
DrawTree::set_object_color( const int fkey,
                            const int okey,
                            const RGBcolor & c )
{
    DrawFrame * master = M_frame_map.get( fkey );

    if ( ! master )
    {
        return false;
    }

    VisualObject2d * obj = master->M_objects.get( okey );

    if ( ! obj )
    {
        return false;
    }

    obj->set_color( c );
    return true;
}

bool
DrawTree::set_position( const int fkey,
                        const double & x,
                        const double & y )
{
    DrawFrame * master = M_frame_map.get( fkey );

    if ( ! master )
    {
        return false;
    }

    master->M_changed = true;
    master->M_rel_transform.set_position( x, y );
    return true;
}

bool
DrawTree::set_visible( const int fkey,
                       const bool val )
{
    DrawFrame * master = M_frame_map.get( fkey );

    if ( ! master )
    {
        return false;
    }

    if ( val != master->M_visible )
    {
        master->M_visible = val;
        master->M_changed = true;
    }

    return true;
}

bool
DrawTree::set_layer( const int fkey,
                     const int val )
{
    return false;
    /* buggy, it doesn't change the order in M_frame_map
       DrawFrame *master= M_frame_map.get(fkey);
       if (!master)
       return false;
       if (val!= master->layer) {
       master->layer= val;
       master->changed= true;
       }
       return true;
    */
}

bool
DrawTree::set_angle( const int fkey,
                     const Angle & ang )
{
    DrawFrame * master = M_frame_map.get( fkey );

    if ( ! master )
    {
        return false;
    }

    master->M_changed = true;
    master->M_rel_transform.set_angle( ang );
    return true;
}

bool
DrawTree::set_scale( const int fkey,
                     const double & scale )
{
    DrawFrame * master = M_frame_map.get( fkey );

    if ( ! master )
    {
        return false;
    }

    master->M_changed = true;
    master->M_rel_transform.set_scale( scale );
    return true;
}

bool
DrawTree::set_pos_ang( const int fkey,
                       const double & x,
                       const double & y,
                       const Angle & ang )
{
    DrawFrame * master = M_frame_map.get( fkey );

    if ( ! master )
    {
        return false;
    }

    master->M_changed = true;
    master->M_rel_transform.set_position( x, y );
    master->M_rel_transform.set_angle( ang );
    return true;
}

void
DrawTree::draw( DisplayBase * disp,
                const Area2d & area )
{
    static Frame2d dum;
    M_origin->draw( disp, area, dum, false );
    //cout << endl; //debug
}

std::ostream &
DrawTree::print( std::ostream & os ) const
{
    return M_frame_map.print( os );
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/*************      T E S T     ***************************************/

#if 0

main()
{
#if 0
    KeyMap_LayerList<DrawFrame> L;

    for ( int i = 1; i <= 9; i++ )
    {
        DrawFrame* d = new DrawFrame( i );
        d->layer = ( i * 19 ) % 5;
        L.insert( d );
    }

    cout << L;

    L.remove( 1 );
    L.remove( 2 );
    L.remove( 2 );
    cout << "\n-\n" << L;
    return 1;
#endif
    DrawTree t;

    for ( int i = 1; i <= 2; i++ )
    {
        DrawFrame *d = new DrawFrame( i );
        d->layer = i;
        t.insert_in_frame( i - 1, d );
    }

    //   t.insert_in_frame(1, new DrawFrame(30));
    //   t.insert_in_frame(1,new VisualObject2d(1));
    //   t.insert_in_frame(0,new DrawObject(2));
    t.translate_in_frame( 0, 1, 10.0, 0.0 );
    t.rotate_frame( 1, M_PI*0.5 );
    t.translate_in_frame( 0, 2, 10.0, 0.0 );

    cout << "\n----";
    cout << t;
    cout << "\n----";
    t.draw();
    cout << t;
}


#endif
