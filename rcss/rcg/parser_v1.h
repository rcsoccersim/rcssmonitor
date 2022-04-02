// -*-c++-*-

/*!
  \file parser_v1.h
  \brief rcg v1 parser Header File.
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

#ifndef RCSS_RCG_PARSER_V1_H
#define RCSS_RCG_PARSER_V1_H

#include <rcss/rcg/parser.h>
#include <rcss/rcg/types.h>

namespace rcss {
namespace rcg {

/*!
  \class ParserV1
  \brief rcg v1 parser class
 */
class ParserV1
    : public Parser {
protected:

    //! The last parsed game time
    int M_time;

public:
    /*!
      \brief default constructor
     */
    ParserV1();

    /*!
      \brief get supported rcg version
      \return version number
     */
    virtual
    int version() const override
    {
        return REC_OLD_VERSION;
    }

    /*!
      \brief parse input stream
      \param is input stream
      \param handler reference to the rcg data handler.
      \retval true, if successfuly parsed.
      \retval false, if incorrect format is detected.
    */
    virtual
    bool parse( std::istream & is,
                Handler & handler ) override;

    /*!
      \brief handle dispinfo_t. This method can be used to parse the monitor packet.
      \param disp the read/received data
      \param handler reference to the data handler
     */
    bool handleDisp( const dispinfo_t & disp,
                     Handler & handler );

protected:

    bool handleShow( const showinfo_t & show,
                     Handler & handler );
    bool handleMsg( const msginfo_t & msg,
                    Handler & handler );
    bool handleDraw( const drawinfo_t & draw,
                     Handler & handler );

};

} // end of namespace
} // end of namespace

#endif
