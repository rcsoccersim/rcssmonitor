// -*-c++-*-

/*!
  \file bench_load.cpp
  \brief a benchmark of loading game logs, as MainWindow does
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

/*
  Usage: bench_load [-n <repeat>] [--progress] <game log>...

  Loads each game log <repeat> times (default 5) with loadGameLogFile(), as
  MainWindow::openGameLogFileImpl() does, and prints the best time. With
  --progress, RCGHandler gets a parent widget and shows its progress
  dialog, as in the application.

  CTest does not run this. Run it with HOME set to a temporary directory
  and QT_QPA_PLATFORM=offscreen, for example:
    HOME=$(mktemp -d) QT_QPA_PLATFORM=offscreen ./test/bench_load game.rcg.gz
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "disp_holder.h"
#include "game_log_file.h"
#include "rcg_handler.h"

#include <QApplication>
#include <QWidget>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

/*!
  \brief load a game log as MainWindow::openGameLogFileImpl() does.
  \return the number of frames, or -1 on failure.
*/
long
loadGameLog( const char * path,
             QWidget * parent )
{
    DispHolder holder;
    RCGHandler handler( parent, holder );
    if ( ! loadGameLogFile( path, handler ) )
    {
        return -1;
    }

    return static_cast< long >( holder.dispCont().size() );
}

}

int
main( int argc, char ** argv )
{
    QApplication app( argc, argv );

    int repeat = 5;
    bool progress = false;
    std::vector< const char * > files;
    for ( int i = 1; i < argc; ++i )
    {
        if ( ! std::strcmp( argv[i], "-n" ) && i + 1 < argc )
        {
            repeat = std::max( 1, std::atoi( argv[++i] ) );
        }
        else if ( ! std::strcmp( argv[i], "--progress" ) )
        {
            progress = true;
        }
        else
        {
            files.push_back( argv[i] );
        }
    }

    if ( files.empty() )
    {
        std::fprintf( stderr, "Usage: %s [-n <repeat>] [--progress] <game log>...\n", argv[0] );
        return 2;
    }

    QWidget parent;

    for ( const char * path : files )
    {
        double best = 1.0e9;
        long frames = -1;
        for ( int r = 0; r < repeat; ++r )
        {
            const auto start = std::chrono::steady_clock::now();
            frames = loadGameLog( path, progress ? &parent : nullptr );
            const double ms = std::chrono::duration< double, std::milli >( std::chrono::steady_clock::now() - start ).count();
            if ( frames < 0 )
            {
                std::fprintf( stderr, "failed to load %s\n", path );
                return 1;
            }
            best = std::min( best, ms );
        }

        std::printf( "%s: %ld frames, best %.0f ms of %d%s\n",
                     path, frames, best, repeat, progress ? " (with the progress dialog)" : "" );
    }

    return 0;
}
