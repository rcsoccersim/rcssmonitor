// -*-c++-*-

/*!
  \file line_parser.cpp
  \brief parsing the lines of a game log, in parallel. Source File.
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

#include "line_parser.h"

#include "handler.h"
#include "types.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <thread>

namespace rcss {
namespace rcg {

namespace {

//! the most threads that parse a log
const std::size_t MAX_THREADS = 8;

//! the fewest lines for a thread; fewer lines are parsed in one thread
const std::size_t MIN_LINES_PER_THREAD = 1024;

/*!
  \class RecordingHandler
  \brief a handler that records what a parser passes to it, to pass it to
  another handler later, in the same order.
*/
class RecordingHandler
    : public Handler {
private:
    std::vector< std::function< bool ( Handler & ) > > M_records;

public:

    void replay( Handler & handler ) const
      {
          for ( const std::function< bool ( Handler & ) > & record : M_records )
          {
              record( handler );
          }
      }

    bool handleServerVersion( const std::string & version ) override
      {
          M_records.emplace_back( [version]( Handler & h ) { return h.handleServerVersion( version ); } );
          return true;
      }

    bool handleTimestamp( const std::string & timestamp ) override
      {
          M_records.emplace_back( [timestamp]( Handler & h ) { return h.handleTimestamp( timestamp ); } );
          return true;
      }

    bool handleEOF() override
      {
          return true;
      }

    bool handleShow( const ShowInfoT & show ) override
      {
          M_records.emplace_back( [show]( Handler & h ) { return h.handleShow( show ); } );
          return true;
      }

    bool handleMsg( const int time,
                    const int board,
                    const std::string & msg ) override
      {
          M_records.emplace_back( [time, board, msg]( Handler & h ) { return h.handleMsg( time, board, msg ); } );
          return true;
      }

    bool handleDraw( const int time,
                     const drawinfo_t & draw ) override
      {
          M_records.emplace_back( [time, draw]( Handler & h ) { return h.handleDraw( time, draw ); } );
          return true;
      }

    bool handlePlayMode( const int time,
                         const PlayMode pm ) override
      {
          M_records.emplace_back( [time, pm]( Handler & h ) { return h.handlePlayMode( time, pm ); } );
          return true;
      }

    bool handleTeam( const int time,
                     const TeamT & team_l,
                     const TeamT & team_r ) override
      {
          M_records.emplace_back( [time, team_l, team_r]( Handler & h ) { return h.handleTeam( time, team_l, team_r ); } );
          return true;
      }

    bool handleServerParam( const ServerParamT & param ) override
      {
          // not copy constructible
          std::shared_ptr< ServerParamT > copy = std::make_shared< ServerParamT >();
          copy->copyFrom( param );
          M_records.emplace_back( [copy]( Handler & h ) { return h.handleServerParam( *copy ); } );
          return true;
      }

    bool handlePlayerParam( const PlayerParamT & param ) override
      {
          // not copy constructible
          std::shared_ptr< PlayerParamT > copy = std::make_shared< PlayerParamT >();
          copy->copyFrom( param );
          M_records.emplace_back( [copy]( Handler & h ) { return h.handlePlayerParam( *copy ); } );
          return true;
      }

    bool handlePlayerType( const PlayerTypeT & param ) override
      {
          M_records.emplace_back( [param]( Handler & h ) { return h.handlePlayerType( param ); } );
          return true;
      }

    bool handleTeamGraphic( const char side,
                            const int x,
                            const int y,
                            const std::vector< std::string > & xpm_data ) override
      {
          M_records.emplace_back( [side, x, y, xpm_data]( Handler & h ) { return h.handleTeamGraphic( side, x, y, xpm_data ); } );
          return true;
      }
};

/*!
  \brief parse a line, taking an exception as a failure.
*/
bool
parse_one( const LineParser & parse,
           const LineRef & line,
           Handler & handler )
{
    try
    {
        return parse( line, handler );
    }
    catch ( ... )
    {
        return false;
    }
}

/*!
  \struct Block
  \brief consecutive lines that a thread parses
*/
struct Block {
    std::size_t first_; //!< the index of the first line
    std::size_t last_; //!< the index after the last line
    RecordingHandler recorder_; //!< what the parser passed
    std::vector< int > failed_; //!< the numbers of the lines that failed
};

}

/*-------------------------------------------------------------------*/
/*!

 */
std::vector< LineRef >
split_lines( const char * data,
             const std::size_t size )
{
    std::vector< LineRef > lines;

    const char * const end = data + size;
    const char * pos = data;
    int number = 0;
    while ( pos < end )
    {
        const char * eol = static_cast< const char * >( std::memchr( pos, '\n', end - pos ) );
        if ( ! eol )
        {
            eol = end;
        }

        lines.push_back( LineRef{ pos, static_cast< std::size_t >( eol - pos ), ++number } );
        pos = ( eol < end ? eol + 1 : end );
    }

    return lines;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::vector< int >
parse_lines( const std::vector< LineRef > & lines,
             const LineParserFactory & make_parser,
             const bool stop_at_failure,
             Handler & handler )
{
    std::vector< int > failed;

    const std::size_t hardware = std::max( 1u, std::thread::hardware_concurrency() );
    const std::size_t threads = std::min( { hardware, MAX_THREADS, lines.size() / MIN_LINES_PER_THREAD } );

    if ( threads <= 1 )
    {
        const LineParser parse = make_parser();
        for ( const LineRef & line : lines )
        {
            if ( ! parse_one( parse, line, handler ) )
            {
                failed.push_back( line.number_ );
                if ( stop_at_failure )
                {
                    break;
                }
            }
        }
        return failed;
    }

    std::vector< Block > blocks( threads );
    for ( std::size_t i = 0; i < threads; ++i )
    {
        blocks[i].first_ = lines.size() * i / threads;
        blocks[i].last_ = lines.size() * ( i + 1 ) / threads;
    }

    std::vector< std::thread > workers;
    workers.reserve( threads );
    for ( Block & block : blocks )
    {
        workers.emplace_back( [&lines, &make_parser, &block, stop_at_failure]()
                              {
                                  const LineParser parse = make_parser();
                                  for ( std::size_t i = block.first_; i < block.last_; ++i )
                                  {
                                      if ( ! parse_one( parse, lines[i], block.recorder_ ) )
                                      {
                                          block.failed_.push_back( lines[i].number_ );
                                          if ( stop_at_failure )
                                          {
                                              break;
                                          }
                                      }
                                  }
                              } );
    }

    for ( std::thread & worker : workers )
    {
        worker.join();
    }

    for ( const Block & block : blocks )
    {
        block.recorder_.replay( handler );
        failed.insert( failed.end(), block.failed_.begin(), block.failed_.end() );
        if ( stop_at_failure
             && ! block.failed_.empty() )
        {
            break;
        }
    }

    return failed;
}

}
}
