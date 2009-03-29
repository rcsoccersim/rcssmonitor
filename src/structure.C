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
std::ostream& operator<< ( std::ostream& o, const DrawFrame &f )
{
    o << "Frame=[";

    if ( f.changed )
        o << "chg, ";

    if ( f.up )
        o << "up= " << f.up->key << ", ";

    o << "key=" <<   f.key
      << " ,layer=" <<   f.layer
        //<< "\n relFrame= " << f.relFrame
        //<< "\n absFrame= " << f.absFrame
      << "]";

    if ( false )
        if ( !f.objects.empty() )
        {
            o  << "\n Objects={";
            o << f.objects;
        }

    return o;
}

DrawFrame::DrawFrame( int k )
{
    key = k;
    changed = true;
    layer = 0;
    visible = true;

    up = 0;//NULL;
}

DrawFrame::~DrawFrame()
{
    //cout << "\nF.key= " << key;
}

bool
DrawFrame::insert( DrawFrame* f )
{
    f->up = this;
    return frames.insert( f );
}

bool
DrawFrame::remove( DrawFrame* f )
{
    bool res = frames.remove( f );
    //delete f;
    return res;
}

bool
DrawFrame::insert( VisualObject2d* f )
{
    return objects.insert( f );
}

bool
 DrawFrame::remove( VisualObject2d * f )
{
    bool res = objects.remove( f );
    //delete f;
    return res;
}

bool
DrawFrame::remove_all( DrawFrameMap & fmap )
{
    KeyMap_LayerList<DrawFrame>::Iterator f_itr( frames );
    KeyMap_LayerList<VisualObject2d>::Iterator o_itr( objects );
    DrawFrame *f_dum;
    VisualObject2d *o_dum;

    while ( ( f_dum = f_itr.get_next() ) )
    {
        fmap.remove( f_dum->key ); //remove the key of this frame from the cache.
        f_dum->remove_all( fmap );
        delete f_dum;
    }

    while ( ( o_dum = o_itr.get_next() ) )
    {
        //cout << "\nObject id= " << o_dum->key << flush;
        delete o_dum;
    }

    frames.remove_all(); //delete the frames list

    objects.remove_all();//delete the objects list
    return true;
}

void
DrawFrame::actualize( const Frame2d & p_frame,
                      bool chg )
{
    changed = changed || chg;

    if ( ! visible ) return;

    KeyMap_LayerList<DrawFrame>::Iterator f_itr( frames );

    KeyMap_LayerList<VisualObject2d>::Iterator o_itr( objects );

    DrawFrame *f_dum;

    VisualObject2d *o_dum;

    if ( changed )
    {
        if ( up )
        {
            //absFrame= up->absFrame * relFrame;
            absFrame = p_frame * relFrame;
        }
        else
        {
            absFrame = relFrame;
        }
    }

    while ( ( f_dum = f_itr.get_next() ) )
    {
        f_dum->actualize( absFrame, changed );
    }

    while ( ( o_dum = o_itr.get_next() ) )
    {
        o_dum->actualize( absFrame, changed );
    }

    changed = false;
}

void
DrawFrame::draw( DisplayBase * disp,
                 const Area2d & area,
                 const Frame2d & p_frame,
                 bool chg )
{
    changed = changed || chg;

    if ( ! visible )
    {
        return;
    }

    //cout << "\n " << key <<  " ----Frame: ";
    KeyMap_LayerList<DrawFrame>::Iterator f_itr( frames );

    KeyMap_LayerList<VisualObject2d>::Iterator o_itr( objects );

    DrawFrame *f_dum;

    VisualObject2d *o_dum;

    if ( changed )
    {
        if ( up )
        {
            //absFrame= up->absFrame * relFrame;
            absFrame = p_frame * relFrame;
        }
        else
        {
            absFrame = relFrame;
        }
    }

    //frames teke precedence over objects it they are in the same layer
    //cout << "\n\n>>>in frame: " << key << "\n";
    o_dum = o_itr.get_next();

    while ( ( f_dum = f_itr.get_next() ) )
    {
        while ( o_dum && o_dum->get_layer() < f_dum->layer )
        {
            o_dum->draw( disp, area, absFrame, changed );
            //cout << "\nobj_in: " << *o_dum;
            o_dum = o_itr.get_next();
        }

        //cout << "\n>frame: " << *f_dum;
        f_dum->draw( disp, area, absFrame, changed );
    }

    while ( o_dum )
    {
        o_dum->draw( disp, area, absFrame, changed );
        //cout << "\nobj_out: "<< *o_dum;
        o_dum = o_itr.get_next();
    }

#if 0
    Vector2d P, X, Y;

    P = absFrame.get_pos();
    X = P + absFrame.get_x_axis();
    Y = P + absFrame.get_y_axis();

    DDD->ASetForeground( "red" );
    DDD->ADrawLine( P.x, P.y, X.x, X.y );
    DDD->ASetForeground( "blue" );
    DDD->ADrawLine( P.x, P.y, Y.x, Y.y );

    //  DDD->ADrawString(P.x,P.y,1,"9");
#endif

    changed = false;
}

/*****************************************************************************/
std::ostream &
operator<< ( std::ostream & o,
             const DrawFrameMap & m )
{
    DrawFrameMap::Item * itr = m.list;

    while ( itr )
    {
        o << "\n" << *( itr->frame );
        itr = itr->next;
    }

    return o;
}

DrawFrameMap::Item::Item( DrawFrame * f )
{
    frame = f;
    key = frame->get_key();
    next = 0;
}

DrawFrameMap::DrawFrameMap()
{
    list = 0;

    for ( int i = 0; i < MAX_DIRECT; ++i )
    {
        direct_map[i] = 0;
    }
}

bool
DrawFrameMap::insert( DrawFrame * f )
{
    int k = f->get_key();

    if ( k >= 0 && k < MAX_DIRECT )
    {
        if ( direct_map[k] )
        {
            return false;
        }

        direct_map[k] = f;
        return true;
    }

    Item *ins = new Item( f );

    if ( 0 == list )
    {
        list = ins;
        return true;
    }

    if ( ins->key < list->key )
    {
        ins->next = list;
        list = ins;
        return true;
    }

    Item *itr = list;

    while ( itr->next && itr->next->key < ins->key )
    {
        itr = itr->next;
    }

    if ( 0 == itr->next )
    {
        itr->next = ins;
        return true;
    }

    if ( itr->next->key  > ins->key )
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
    if ( key >= 0 && key < MAX_DIRECT )
    {
        direct_map[key] = 0;
        return true;
    }

    if ( ! list )
        return true;

    Item *itr = list;

    if ( itr->key > key )
    {
        return true;
    }
    else if ( itr->key == key )
    {
        list = itr->next;
        delete itr;
        return true;
    }

    while ( itr->next && itr->next->key < key )
    {
        itr = itr->next;
    }

    if ( 0 == itr->next )
    {
        return true;
    }

    if ( itr->next->key  == key )
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
    if ( key >= 0 && key < MAX_DIRECT )
    {
        return direct_map[key];
    }

    Item * itr = list;

    while ( itr != 0 && itr->key < key )
    {
        itr = itr->next;
    }

    if ( 0 == itr )
    {
        return 0;
    }

    if ( itr->key != key )
    {
        return 0;
    }

    return itr->frame;
}

/*****************************************************************************/

std::ostream &
operator<< ( std::ostream & o,
             const DrawTree & t )
{
    return o << t.frameMap;
}

DrawTree::DrawTree()
{
    origin = new DrawFrame( 0 );
    frameMap.insert( origin );
}

bool
DrawTree::insert_in_frame( int fkey,
                           DrawFrame * f )
{
    DrawFrame *master = frameMap.get( fkey );

    if ( master
         && 0 == frameMap.get( f->get_key() ) )
    {
        if ( master->insert( f ) )
        {
            frameMap.insert( f );
            return true;
        }
    }

    return false;
}

bool
DrawTree::insert_in_frame( int fkey,
                           VisualObject2d *f )
{
    DrawFrame * master = frameMap.get( fkey );

    if ( master )
    {
        return master->insert( f );
    }

    return false;
}


bool
DrawTree::remove_frame( int fkey )
{
    DrawFrame *master = frameMap.get( fkey );

    if ( master && master->up )
    {
        DrawFrame * up = master->up;
        frameMap.remove( master->key );
        master->remove_all( frameMap );
        delete master;
        return up->remove( master );
    }

    return false;
}

bool
DrawTree::remove_in_frame( int fkey,
                           int fo_key )
{
    DrawFrame * master = frameMap.get( fkey );
    DrawFrame * slave = frameMap.get( fo_key );

    if ( master && slave && master != slave )
    {
        frameMap.remove( slave->key );
        slave->remove_all( frameMap );
        delete slave;
        return master->remove( slave );
    }

    return false;
}

bool
DrawTree::empty_frame( int fkey )
{
    DrawFrame *master = frameMap.get( fkey );

    if ( master )
    {
        master->remove_all( frameMap );
        return true;
    }

    return false;
}

bool
DrawTree::rotate_frame( int fkey,
                        const Angle & ang )
{
    DrawFrame * master = frameMap.get( fkey );

    if ( ! master )
    {
        return false;
    }

    master->changed = true;
    master->relFrame = master->relFrame * Frame2d::Rotation( ang );

    return true;
}

bool
DrawTree::translate_frame( int fkey,
                           double, double )
{
    return false;
}

bool
DrawTree::rotate_in_frame( int fkey,
                           int fo_key,
                           const Angle & )
{
    return false;
}

bool
DrawTree::translate_in_frame( int fkey,
                              int fo_key,
                              double x,
                              double y )
{
    DrawFrame * master = frameMap.get( fkey );
    DrawFrame * slave = frameMap.get( fo_key );

    if ( ! master || ! slave || master == slave )
    {
        return false;
    }

    slave->changed = true;
    //eigentlich braucht man hier master nicht!!!
    slave->relFrame = Frame2d::Translation( x, y ) * slave->relFrame;

    return true;
}

bool
DrawTree::set_object_color( int fkey,
                            int okey,
                            const RGBcolor & c )
{
    DrawFrame * master = frameMap.get( fkey );

    if ( ! master )
    {
        return false;
    }

    VisualObject2d * obj = master->objects.get( okey );

    if ( ! obj )
    {
        return false;
    }

    obj->set_color( c );
    return true;
}

bool
DrawTree::set_position( int fkey,
                        double x,
                        double y )
{
    DrawFrame * master = frameMap.get( fkey );

    if ( ! master )
    {
        return false;
    }

    master->changed = true;
    master->relFrame.set_position( x, y );
    return true;
}

bool
DrawTree::set_visible( int fkey,
                       bool val )
{
    DrawFrame * master = frameMap.get( fkey );

    if ( ! master )
    {
        return false;
    }

    if ( val != master->visible )
    {
        master->visible = val;
        master->changed = true;
    }

    return true;
}

bool
DrawTree::set_layer( int fkey,
                     int val )
{
    return false;
    /* buggy, it doesn't change the order in frameMap
       DrawFrame *master= frameMap.get(fkey);
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
DrawTree::set_angle( int fkey,
                     const Angle & ang )
{
    DrawFrame * master = frameMap.get( fkey );

    if ( ! master )
    {
        return false;
    }

    master->changed = true;
    master->relFrame.set_angle( ang );
    return true;
}

bool
DrawTree::set_scale( int fkey,
                     double scale )
{
    DrawFrame * master = frameMap.get( fkey );

    if ( ! master )
    {
        return false;
    }

    master->changed = true;
    master->relFrame.set_scale( scale );
    return true;
}

bool
DrawTree::set_pos_ang( int fkey,
                       double x,
                       double y,
                       const Angle & ang )
{
    DrawFrame * master = frameMap.get( fkey );

    if ( ! master )
    {
        return false;
    }

    master->changed = true;
    master->relFrame.set_position( x, y );
    master->relFrame.set_angle( ang );
    return true;
}

void
DrawTree::draw( DisplayBase * disp,
                const Area2d & area )
{
    static Frame2d dum;
    origin->draw( disp, area, dum, false );
    //cout << endl; //debug
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
