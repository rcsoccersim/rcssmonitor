// -*-c++-*-

/*!
  \file game_log_file.h
  \brief reading and parsing a game log file. Header File.
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

#ifndef RCSSMONITOR_GAME_LOG_FILE_H
#define RCSSMONITOR_GAME_LOG_FILE_H

#include <string>

namespace rcss {
namespace rcg {
class Handler;
}
}

/*!
  \brief read a whole game log file into memory.
  \param path the file path (in the encoding of the file system)
  \param data the file contents. A gzip file is decompressed.
  \return false if the file cannot be read

  The file is read at once into one buffer: decompressing a gzip stream
  through std::istream (gzifstream) and copying it again is much slower.
  With libdeflate, a gzip file is decompressed in memory at once, a few
  times faster than with zlib. Data that libdeflate cannot decompress (a
  file that ends in the middle, as the server stopped abnormally) are
  read with zlib, which reads them up to where they are broken.
*/
bool readGameLogFile( const std::string & path,
                      std::string * data );

/*!
  \brief read a game log file and parse it.
  \param path the file path (in the encoding of the file system)
  \param handler the handler that receives the parsed data
  \return false if the file cannot be read or parsed
*/
bool loadGameLogFile( const std::string & path,
                      rcss::rcg::Handler & handler );

#endif
