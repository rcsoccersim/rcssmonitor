// -*-c++-*-

/*!
  \file message_splitter.cpp
  \brief splitter of the NUL terminated messages Source File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "message_splitter.h"

/*-------------------------------------------------------------------*/
/*!

*/
MessageSplitter::MessageSplitter( const int max_size )
    : M_max_size( max_size )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
bool
MessageSplitter::append( const QByteArray & data )
{
    int begin = 0;

    while ( begin < data.size() )
    {
        const int end = data.indexOf( '\0', begin );

        if ( end < 0 )
        {
            M_partial.append( data.constData() + begin, data.size() - begin );
            break;
        }

        M_partial.append( data.constData() + begin, end - begin );

        if ( ! M_partial.isEmpty() )
        {
            M_messages.push_back( M_partial );
            M_partial.clear();
        }

        begin = end + 1;
    }

    return M_partial.size() <= M_max_size;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
MessageSplitter::pop( QByteArray & msg )
{
    if ( M_messages.empty() )
    {
        msg.clear();
        return false;
    }

    msg = M_messages.front();
    M_messages.pop_front();
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MessageSplitter::clear()
{
    M_partial.clear();
    M_messages.clear();
}
