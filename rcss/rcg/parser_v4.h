// -*-c++-*-

/*!
  \file parser_v4.h
  \brief rcg v4 parser Header File.
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

#ifndef RCSS_RCG_PARSER_V4_H
#define RCSS_RCG_PARSER_V4_H

#include <rcss/rcg/parser.h>
#include <rcss/rcg/types.h>

#include <string>

namespace rcss {
namespace rcg {

/*!
  \class ParserV4
  \brief rcg v4 parser class
 */
class ParserV4
    : public Parser {
public:

    /*!
      \brief get supported rcg version
      \return version number
     */
    virtual
    int version() const override
      {
          return REC_VERSION_4;
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
      \brief parse data line.
      \param n_line the number of total read line
      \param line the data string
      \param handler reference to the rcg data handler.
      \retval true, if successfuly parsed.
      \retval false, if incorrect format is detected.

      First, check the type of data mode.
      Second, call each data item parsing method.
    */
    bool parseLine( const int n_line,
                    const std::string & line,
                    Handler & handler );

protected:

    /*!
      \brief parse show data line
      \param n_line the number of total read line
      \param line the data string
      \param handler reference to the data handler object
      \retval true if successfully parsed.
      \retval false if failed to parse.
    */
    virtual
    bool parseShow( const int n_line,
                    const std::string & line,
                    Handler & handler );

    /*!
      \brief parse msg data line
      \param n_line the number of total read line
      \param line the data string
      \param handler reference to the data handler object
      \retval true if successfully parsed.
      \retval false if failed to parse.
    */
    bool parseMsg( const int n_line,
                   const std::string & line,
                   Handler & handler );

    /*!
      \brief parse playmode data line
      \param n_line the number of total read line
      \param line the data string
      \param handler reference to the data handler object
      \retval true if successfully parsed.
      \retval false if failed to parse.
    */
    bool parsePlayMode( const int n_line,
                        const std::string & line,
                        Handler & handler );

    /*!
      \brief parse team data line
      \param n_line the number of total read line
      \param line the data string
      \param handler reference to the data handler object
      \retval true if successfully parsed.
      \retval false if failed to parse.
    */
    bool parseTeam( const int n_line,
                    const std::string & line,
                    Handler & handler );

    /*!
      \brief parse server_param data line
      \param n_line the number of total read line
      \param line the data string
      \param handler reference to the data handler object
      \retval true if successfully parsed.
      \retval false if failed to parse.
    */
    bool parseServerParam( const int n_line,
                           const std::string & line,
                           Handler & handler );

    /*!
      \brief parse player_param data line
      \param n_line the number of total read line
      \param line the data string
      \param handler reference to the data handler object
      \retval true if successfully parsed.
      \retval false if failed to parse.
    */
    bool parsePlayerParam( const int n_line,
                           const std::string & line,
                           Handler & handler );

    /*!
      \brief parse player_type data line
      \param n_line the number of total read line
      \param line the data string
      \param handler reference to the data handler object
      \retval true if successfully parsed.
      \retval false if failed to parse.
    */
    bool parsePlayerType( const int n_line,
                          const std::string & line,
                          Handler & handler );

};

} // end of namespace
} // end of namespace

#endif
