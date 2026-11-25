// -*-c++-*-

/*!
  \file main.cpp
  \brief main function source file
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QApplication>
#include <QLocale>

#include "export_batch.h"
#include "main_window.h"
#include "options.h"

#include <cstring>
#include <iostream>
#include <locale>

namespace {

/*!
  \brief an export from the command line is asked for (--export-video or
  --export-images): read before the application is made, to choose its
  platform
*/
bool
exportAsked( int argc,
             char ** argv )
{
    for ( int i = 1; i < argc; ++i )
    {
        for ( const char * name : { "--export-video", "--export-images" } )
        {
            const std::size_t n = std::strlen( name );
            if ( std::strncmp( argv[i], name, n ) == 0
                 && ( argv[i][n] == '\0' || argv[i][n] == '=' ) )
            {
                return true;
            }
        }
    }
    return false;
}

}

int
main( int argc,
      char ** argv )
{
    std::cout << PACKAGE_NAME << "-" << VERSION << "\n\n"
              << "Copyright (C) 2009 - 2026 RoboCup Soccer Simulator Maintenance Group.\n"
              << std::endl;

#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    // Scale by the device pixel ratio of the screen, as Qt6 always does,
    // also by a fractional one (e.g. 1.29 with 124 dpi): the same canvas
    // size makes the same window with Qt5 and Qt6.
    QApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
    QApplication::setAttribute( Qt::AA_UseHighDpiPixmaps );
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy( Qt::HighDpiScaleFactorRoundingPolicy::PassThrough );
#endif

    // The export from the command line shows no window: on the platform
    // offscreen (whatever QT_QPA_PLATFORM the desktop sets; -platform of
    // Qt still chooses another), it runs without a display too. The 3D view
    // gets OpenGL from an X display there, when there is one (the desktop,
    // or xvfb-run; docs/plan/2026-11-22-03_frame-export.md 4.16).
    if ( exportAsked( argc, argv ) )
    {
        qputenv( "QT_QPA_PLATFORM", "offscreen" );
    }

    QApplication app( argc, argv );
    QApplication::setApplicationName( PACKAGE_NAME );
    QApplication::setApplicationVersion( VERSION );
    // the name of the desktop entry (rcssmonitor.desktop): the app_id of
    // Wayland, by which the desktop (e.g. GNOME's dash) links the window to
    // the entry and its icon. On X11, WM_CLASS and StartupWMClass do it.
    QGuiApplication::setDesktopFileName( QStringLiteral( "rcssmonitor" ) );

    std::locale::global( std::locale::classic() );

    if ( ! Options::instance().parseCmdLine( argc, argv ) )
    {
        return 1;
    }

    // the export from the command line: no window, the exit code of the export
    if ( Options::instance().batchExport().enabled() )
    {
        ExportBatch batch( Options::instance().batchExport(), std::cerr );
        return batch.run( QString::fromLocal8Bit( Options::instance().gameLogFile().c_str() ) );
    }

    MainWindow win;
    win.show();
    win.init();

    return app.exec();
}
