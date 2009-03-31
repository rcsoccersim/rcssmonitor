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

#ifndef MAPLIST_H
#define MAPLIST_H

#include <iostream>

template < typename T >
class KeyMap_LayerList {
public: //vorlauefig, wegen <<

    struct Item {
        T * content;
        Item * next;
        Item( T * );
    };

    Item * list;

private:
    bool exists_item_with_key( int );
    bool remove_item_with_key( int );

public:
    KeyMap_LayerList();

    class Iterator {
        Item * point;
    public:
        Iterator( KeyMap_LayerList &L );
        T * get_next();
    };

    bool empty() const
      {
          return 0 == list;
      }

    bool insert( T * );
    bool remove ( int );
    bool remove ( T * );
    bool remove_all();
    T * get( int );
    void print();
};

template < typename T >
std::ostream &
operator<<( std::ostream & o,
            const KeyMap_LayerList< T > & L )
{
    typename KeyMap_LayerList< T >::Item * itr = L.list;

    while ( itr )
    {
        //o << "\n" << *(itr->content);
        itr = itr->next;
    }

    return o;
}

template < typename T >
KeyMap_LayerList< T >::Item::Item( T * t )
{
    content = t;
    next = static_cast< Item * >( 0 );
}

template < typename T >
KeyMap_LayerList< T >::Iterator::Iterator( KeyMap_LayerList & L )
{
    point = L.list;
}

template < typename T >
T *
KeyMap_LayerList< T >::Iterator::get_next()
{
    if ( ! point ) return static_cast< T * >( 0 );

    T * p = point->content;

    point = point->next;

    return p;
}

template < typename T >
KeyMap_LayerList< T >::KeyMap_LayerList()
{
    list = static_cast< Item * >( 0 );
}

template < typename T >
bool
KeyMap_LayerList< T >::exists_item_with_key( int k )
{
    Item * itr = list;

    while ( itr
            && itr->content->get_key() != k )
    {
        itr = itr->next;
    }

    if ( itr )
    {
        return true;
    }

    return false;
}

template < typename T >
bool
KeyMap_LayerList< T >::remove_item_with_key( int k )
{
    Item * itr = list;

    if ( ! itr )
    {
        return false;
    }

    if ( itr->content->get_key() == k )
    {
        list = list->next;
        delete itr;
        return true;
    }

    while ( itr->next && itr->next->content->get_key() != k )
    {
        itr = itr->next;
    }

    if ( itr->next )
    {
        Item * dum = itr->next;
        itr->next = dum->next;
        delete dum;
        return true;
    }
    else
    {
        return false;
    }
}

template < typename T >
bool
KeyMap_LayerList< T >::insert( T * t )
{
    //there can be only one object for a key != 0
    //but multiple objects with the key== 0 are allowed
    if ( t->get_key() != 0
         && exists_item_with_key( t->get_key() ) )
    {
        return false;
    }

    Item * ins = new Item( t );

    if ( ! list )
    {
        list = ins;
        return true;
    }

    if ( ins->content->get_layer() < list->content->get_layer() )
    {
        ins->next = list;
        list = ins;
        return true;
    }

    Item * itr = list;

    while ( itr->next
            && itr->next->content->get_layer() <= ins->content->get_layer() )
    {
        itr = itr->next;
    }

    ins->next = itr->next;
    itr->next = ins;

    return true;
}

template < typename T >
bool
KeyMap_LayerList< T >::remove( T * t )
{
    if ( ! t ) return false;

    if ( ! list )
    {
        return false;
    }

    if ( list->content == t )
    {
        Item * dum = list;
        list = list->next;
        delete dum;
        return true;
    }

    Item * itr = list;

    while ( itr->next && t != itr->next->content )
    {
        itr = itr->next;
    }

    if ( itr->next )
    {
        Item * dum = itr->next;
        itr->next = dum->next;
        delete dum;
        return true;
    }

    return false;
}

template < typename T >
bool
KeyMap_LayerList< T >::remove( int k )
{
    return remove_item_with_key( k );
}

template < typename T >
bool
KeyMap_LayerList< T >::remove_all()
{
    Item * itr = list;
    list = static_cast< Item * >( 0 );

    while ( itr )
    {
        Item * dum = itr;

        if ( itr->next )
        {
            itr = itr->next;
        }
        else
        {
            itr = static_cast< Item * >( 0 );
        }

        delete dum;
    }

    return true;
}

template < typename T >
T *
KeyMap_LayerList< T >::get( int k )
{
    Item * itr = list;

    while ( itr
            && itr->content->get_key() != k )
    {
        itr = itr->next;
    }

    if ( itr )
    {
        return itr->content;
    }

    return static_cast< T * >( 0 );
}

template < typename T >
void
KeyMap_LayerList< T >::print()
{
    Item * itr = list;

    while ( itr )
    {
        std::cout << "\n" << *( itr->content );
        itr = itr->next;
    }
}

#endif
