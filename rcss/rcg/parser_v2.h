// -*-c++-*-

/*!
  \file parser_v2.h
  \brief rcg v2 parser Header File.
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

#ifndef RCSS_RCG_PARSER_V2_H
#define RCSS_RCG_PARSER_V2_H

#include <rcss/rcg/parser.h>
#include <rcss/rcg/types.h>

namespace rcss {
namespace rcg {

/*!
  \class ParserV2
  \brief rcg v2 parser
 */
class ParserV2
    : public Parser {
public:

    /*!
      \brief get supported rcg version
      \return version number
     */
    int version() const override
      {
          return REC_VERSION_2;
      }

    /*!
      \brief parse input stream. hander will handle each data block.
      \param is reference to the imput stream (usually ifstream).
      \param handler reference to the rcg data handler.
      \retval true if successfuly parsed.
      \retval false if incorrect format is detected.
    */
    bool parse( std::istream & is,
                Handler & handler ) const override;

private:
    /*!
      \brief parse data block.
      \param is reference to the imput stream (usually ifstream).
      \param handler reference to the rcg data handler.
      \retval true if successfuly parsed.
      \retval false if incorrect format is detected.

      This method is called in the loop of main parse() method.
      First, check the type of data mode.
      Second, call matched handler method.
    */
    bool parseData( std::istream & is,
                    Handler & handler ) const;

    /*!
      \brief parse MSG_MODE info(msg_info_t)
      \param is reference to the input stream
      \param handler reference to the data handler object
      \retval true if successfully parsed.
      \retval false if failed to parse.
    */
    bool parseMsgInfo( std::istream & is,
                       Handler & handler ) const;
};

} // end of namespace
} // end of namespace

#endif
