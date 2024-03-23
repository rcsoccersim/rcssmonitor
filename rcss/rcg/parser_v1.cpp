// -*-c++-*-

/*!
  \file parser_v1.cpp
  \brief rcg v1 parser Source File.
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "parser_v1.h"

#include "handler.h"
#include "types.h"
#include <iostream>

namespace rcss {
namespace rcg {

/*-------------------------------------------------------------------*/
/*!

*/
bool
ParserV1::parse( std::istream & is,
                 Handler & handler ) const
{
    // streampos must be the first point!!!
    is.seekg( 0 );

    if ( ! is.good() )
    {
        return false;
    }

    // register log version
    // Because rcg v1 does not contain version information,
    // REC_OLD_VERSION is set without reading.
    if ( ! handler.handleLogVersion( REC_OLD_VERSION ) )
    {
        return false;
    }

    dispinfo_t info;
    while ( is.good() )
    {
        is.read( reinterpret_cast< char* >( &info ), sizeof( dispinfo_t ) );
        if ( is.gcount() != sizeof( dispinfo_t ) )
        {
            if ( is.eof() )
            {
                break;
            }
            //std::cerr << "failed to read dispinfo " << std::endl;
            return false;
        }

        // check cycle order and so on.
        if ( ! handler.handleDispInfo( info ) )
        {
            return false;
        }
    }

    if ( is.eof() )
    {
        return handler.handleEOF();
    }

    return false;
}

} // end of namespace
} // end of namespace
