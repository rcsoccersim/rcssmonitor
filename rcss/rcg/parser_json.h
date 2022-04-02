// -*-c++-*-

/*!
  \file parser_json.h
  \brief rcg v6 (JSON format) parser Header File.
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

#ifndef RCSS_RCG_PARSER_JSON_H
#define RCSS_RCG_PARSER_JSON_H

#include <rcss/rcg/parser.h>
#include <rcss/rcg/types.h>

#include <string>
#include <memory>

namespace rcss {
namespace rcg {

/*!
  \class ParserJSON
  \brief rcg v6 parser class
 */
class ParserJSON
    : public Parser {
private:
    struct Impl;
    std::shared_ptr< Impl > M_impl;

public:

    /*!
      \brief create a Impl instance
     */
    ParserJSON();

    /*!
      \brief get supported rcg version
      \return version number
     */
    virtual
    int version() const override
      {
          return REC_VERSION_JSON;
      }

    /*!
      \brief parse input stream
      \param is reference to the imput stream (usually ifstream/gzifstream).
      \param handler reference to the rcg data handler.
      \retval true, if successfuly parsed.
      \retval false, if incorrect format is detected.
    */
    virtual
    bool parse( std::istream & is,
                Handler & handler ) override;

    /*!
      \brief assume to parse one monitor packet.
      \param input the data string
      \param handler reference to the rcg data handler.
      \retval true, if successfuly parsed.
      \retval false, if incorrect format is detected.

      First, check the type of data mode.
      Second, call each data item parsing method.
    */
    bool parseData( const std::string & input,
                    Handler & handler );

};

} // end of namespace
} // end of namespace

#endif
