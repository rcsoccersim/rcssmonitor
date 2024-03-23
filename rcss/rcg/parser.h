// -*-c++-*-

/*!
  \file parser.h
  \brief abstract rcg parser class Header File.
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

#ifndef RCSS_RCG_PARSER_H
#define RCSS_RCG_PARSER_H

#include <memory>
#include <istream>

namespace rcss {
namespace rcg {

class Handler;

/////////////////////////////////////////////////////////////////////

/*!
  \class Parser
  \brief rcg stream parser interface class
*/
class Parser {
public:

    typedef std::shared_ptr< Parser > Ptr; //!< rcg parser pointer type
    typedef Ptr (*Creator)(); //!< rcg parser creator function

    /*!
      \brief create a suitable version parser instance depending on the input stream.
      \param is reference to the imput stream.
      \return smart pointer to the rcg parser instance

      poionted index of istream becomes 4.
     */
    static
    Ptr create( std::istream & is );

protected:

    /*!
      \brief constructor is accessible only from the derived classes.
     */
    Parser() = default;

public:

    /*!
      \brief virtual destructor
    */
    virtual
    ~Parser()
      { }

    /*!
      \brief (pure virtual) get log version
      \return version number
     */
    virtual
    int version() const = 0;

    /*!
      \brief (pure virtual) analyze log data from input stream
      \param is reference to the imput stream (usually ifstream/gzifstream).
      \param handler reference to the rcg data handler.
      \retval true, if successfuly parsed.
      \retval false, if incorrect format is detected.
     */
    virtual
    bool parse( std::istream & is,
                Handler & handler ) const = 0;

    virtual
    bool parse( const std::string & filepath,
                Handler & handler ) const;
};

} // end of namespace
} // end of namespace

#endif
