// -*-c++-*-

/*!
  \file parser.cpp
  \brief abstract rcg parser class Source File.
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

#include "parser.h"

#include "types.h"

#include "parser_v1.h"
#include "parser_v2.h"
#include "parser_v3.h"
#include "parser_v4.h"
#include "parser_json.h"

#include <iostream>

namespace rcss {
namespace rcg {

/*-------------------------------------------------------------------*/
/*!

*/
// Parser::Creators &
// Parser::creators()
// {
//     static Creators s_instance;
//     return s_instance;
// }


/*-------------------------------------------------------------------*/
/*!

*/
Parser::Ptr
Parser::create( std::istream & is )
{
    Parser::Ptr ptr;

    char header[5];
    int version = REC_OLD_VERSION;

    is.read( header, 4 ); // read 'U', 'L', 'G', <version>

    if ( is.gcount() != 4 )
    {
        return ptr;
    }

    if ( header[0] == 'J'
         && header[1] == 'S'
         && header[2] == 'O'
         && header[3] == 'N' )
    {
        std::cerr << "(rcss::rcg::Parser::create) game log version = " << version - static_cast< int >( '0' ) << " (json)" << std::endl;
        ptr = Parser::Ptr( new ParserJSON() );
        return ptr;
    }


    if ( header[0] == 'U'
         && header[1] == 'L'
         && header[2] == 'G' )
    {
        version = static_cast< int >( header[3] );
    }

    if ( version == static_cast< int >( '0' ) + REC_VERSION_6 )
    {
        // ParserV4 can parse the v6 format.
        std::cerr << "(rcss::rcg::Parser::crete) game log version = " << version - static_cast< int >( '0' ) << std::endl;
        ptr = Parser::Ptr( new rcss::rcg::ParserV4() );

    }
    else if ( version == static_cast< int >( '0' ) + REC_VERSION_5 )
    {
        // ParserV4 can parse the v5 format.
        std::cerr << "(rcss::rcg::Parser::crete) game log version = " << version - static_cast< int >( '0' ) << std::endl;
        ptr = Parser::Ptr( new rcss::rcg::ParserV4() );
    }
    else if ( version == static_cast< int >( '0' ) + REC_VERSION_4 )
    {
        std::cerr << "(rcss::rcg::Parser::create) game log version = " << version - static_cast< int >( '0' ) << std::endl;
        ptr = Parser::Ptr( new rcss::rcg::ParserV4() );
    }
    else if ( version == REC_VERSION_3 )
    {
        std::cerr << "(rcss::rcg::Parser::create) game log version = " << version << std::endl;
        ptr = Parser::Ptr( new rcss::rcg::ParserV3() );
    }
    else if ( version == REC_VERSION_2 )
    {
        std::cerr << "(rcss::rcg::Parser::create) game log version = " << version << std::endl;
        ptr = Parser::Ptr( new rcss::rcg::ParserV2() );
    }
    else if ( version == REC_OLD_VERSION )
    {
        std::cerr << "(rcss::rcg::Parser::create) game log version = " << version << std::endl;
        ptr = Parser::Ptr( new ParserV1() );
    }

    return ptr;
}

}
}
