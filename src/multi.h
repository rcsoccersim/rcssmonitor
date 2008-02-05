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

#ifndef _MULTI_H_
#define _MULTI_H_

#include <cstring> //memcpy

template < typename T >
struct Multi {
    int cur_size;
    int max_size;
    T * tab;
    Multi()
        : cur_size( 0 ),
          max_size( 0 ),
          tab( static_cast< T * >( 0 ) )
      { }

    explicit
    Multi( const int msize )
      {
          set_max_size( msize );
      }

    ~Multi()
      {
          if ( tab ) delete [] tab;
      }

    bool set_max_size( const int s )
      {
          cur_size = 0;
          if ( s < 0) return false;
          if ( s <= max_size) return true;
          max_size = s;
          if ( tab ) delete [] tab;
          if ( 0 == max_size )
          {
              tab = static_cast< T * >( 0 );;
          }
          else
          {
              tab = new T[max_size];
          }

          return true;
      }

    bool set_max_size_and_preserve_cur_size( const int s )
      {
        if ( s < 0 ) return false;
        if ( s <= max_size ) return true;
        if ( cur_size > max_size )
        {
            cur_size= max_size;
        }
        max_size= s;
        if ( cur_size <= 0 || tab == 0 )
        {
            if ( tab ) delete [] tab;
            if ( 0 == max_size )
            {
                tab = 0;
            }
            else
            {
                tab = new T[max_size];
            }
        }
        else
        {
            T * dum = tab;
            tab = new T[max_size];
            std::memcpy( (void*)tab, (void*)dum, sizeof( T ) * cur_size );
            delete [] dum;
        }
        return true;
    }

    bool set_cur_size( const int s )
      {
        cur_size = 0;
        if ( s > max_size || s < 0 )
        {
            return false;
        }
        cur_size = s;
        return true;
    }

    void operator=( const Multi< T > & mul )
      {
        if ( set_max_size( mul.cur_size ) )
        {
            cur_size= mul.cur_size;
            //cout << "\nMULTI: cur_size= " << cur_size;
            for ( int i = 0; i < cur_size; ++i )
            {
                tab[i] = mul.tab[i];
            }
        }
    }

    void clone( const int len,
                const T * val )
      {
          set_max_size( len );
          cur_size = len;
          std::memcpy( (void*)tab, (void*)val, sizeof( T ) * cur_size );
      }
};

#endif
