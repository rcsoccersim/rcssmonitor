// -*-c++-*-

/*!
  \file parser_v3.h
  \brief rcg v3 parser Header File.
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

#ifndef RCSS_RCG_PARSER_V3_H
#define RCSS_RCG_PARSER_V3_H

#include <rcss/rcg/parser.h>
#include <rcss/rcg/types.h>

namespace rcss {
namespace rcg {

/*!
  \class ParserV3
  \brief rcg v3 parser class
 */
class ParserV3
    : public Parser {
public:

    /*!
      \brief get supported rcg version
      \return version number
     */
    int version() const override
      {
          return REC_VERSION_3;
      }

    /*!
      \brief parse input stream
      \param is reference to the imput stream (usually ifstream/gzifstream).
      \param handler reference to the rcg data handler.
      \retval true, if successfuly parsed.
      \retval false, if incorrect format is detected.
    */
    bool parse( std::istream & is,
                Handler & handler ) const override;

private:
    /*!
      \brief parse data block.
      \param is reference to the imput stream (usually ifstream).
      \param handler reference to the rcg data handler.
      \retval true, if successfuly parsed.
      \retval false, if incorrect format is detected.

      First, check the type of data mode.
      Second, call each data item parsing method.
    */
    bool parseData( std::istream & is,
                    Handler & handler ) const;

    /*!
      \brief parse SHOW_MODE inof, actually short_showinfo_t2
      \param is reference to the input stream
      \param handler reference to the data handler object
      \retval true if successfully parsed.
      \retval false if failed to parse.
    */
    bool parseShowInfo( std::istream & is,
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

    /*!
      \brief parse PM_MODE info(playmode)
      \param is reference to the input stream
      \param handler reference to the data handler object
      \retval true if successfully parsed.
      \retval false if failed to parse.
    */
    bool parsePlayMode( std::istream & is,
                        Handler & handler ) const;

    /*!
      \brief parse TEAM_MODE info(team_t * 2)
      \param is reference to the input stream
      \param handler reference to the data handler object
      \retval true if successfully parsed.
      \retval false if failed to parse.
    */
    bool parseTeamInfo( std::istream & is,
                        Handler & handler ) const;

    /*!
      \brief parse PT_MODE info(player_type_t)
      \param is reference to the input stream
      \param handler reference to the data handler object
      \retval true if successfully parsed.
      \retval false if failed to parse.
    */
    bool parsePlayerType( std::istream & is,
                          Handler & handler ) const;

    /*!
      \brief parse PARAM_MODE info(server_params_t)
      \param is reference to the input stream
      \param handler reference to the data handler object
      \retval true if successfully parsed.
      \retval false if failed to parse.
    */
    bool parseServerParam( std::istream & is,
                           Handler & handler ) const;

    /*!
      \brief parse PPARAM_MODE info(player_params_t)
      \param is reference to the input stream
      \param handler reference to the data handler object
      \retval true if successfully parsed.
      \retval false if failed to parse.
    */
    bool parsePlayerParam( std::istream & is,
                           Handler & handler ) const;
};

} // end of namespace
} // end of namespace

#endif
