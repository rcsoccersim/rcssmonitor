// -*-c++-*-

/*!
  \file line_parser.h
  \brief parsing the lines of a game log, in parallel. Header File.
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

#ifndef RCSS_RCG_LINE_PARSER_H
#define RCSS_RCG_LINE_PARSER_H

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace rcss {
namespace rcg {

class Handler;

/*!
  \struct LineRef
  \brief a line of the log data in memory, without its line break
*/
struct LineRef {
    const char * begin_; //!< the first character
    std::size_t length_; //!< the number of characters
    int number_; //!< the line number, from 1
};

/*!
  \brief split the data into lines, as std::getline() does: '\n' ends a
  line (a '\r' before it stays in the line), and a '\n' at the end of the
  data does not make an empty line after it.
  \param data the data
  \param size the size of data
  \return the lines
*/
std::vector< LineRef > split_lines( const char * data,
                                    const std::size_t size );

/*!
  \brief a function that parses a line and passes its data to a handler.
  It returns false if the line cannot be parsed.
*/
using LineParser = std::function< bool ( const LineRef & line,
                                         Handler & handler ) >;

/*!
  \brief a function that makes a LineParser for a thread, with the state
  that the thread uses alone (a parser object, a buffer, ...).
*/
using LineParserFactory = std::function< LineParser () >;

/*!
  \brief parse lines and pass their data to a handler in the order of the
  lines. With many lines, consecutive blocks of them are parsed in
  parallel, each into a recording handler, and the records are passed to
  the handler in order afterwards. A line that fails (or throws) is
  skipped.
  \param lines the lines
  \param make_parser makes the line parser of each thread
  \param handler the handler
  \return the numbers of the lines that failed, in order

  The return values of the handler's functions are not used.
*/
std::vector< int > parse_lines( const std::vector< LineRef > & lines,
                                const LineParserFactory & make_parser,
                                Handler & handler );

/*!
  \brief print the numbers of the lines that were skipped to std::cerr:
  the first ones one by one, and then the number of them.
  \param who the name of the parser, at the head of each message
  \param what "lines" or "records"
  \param numbers the numbers of the lines (or records) that were skipped
*/
void report_skipped( const std::string & who,
                     const std::string & what,
                     const std::vector< int > & numbers );

}
}

#endif
