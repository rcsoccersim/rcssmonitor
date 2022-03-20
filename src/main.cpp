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

#include "main_window.h"
#include "options.h"

#include <iostream>
#include <locale>

int
main( int argc,
      char ** argv )
{
    std::cout << PACKAGE_NAME << "-" << VERSION << "\n\n"
              << "Copyright (C) 2009 - 2022 RoboCup Soccer Simulator Maintenance Group.\n"
              << std::endl;

    QApplication app( argc, argv );
    QApplication::setApplicationName( PACKAGE_NAME );
    QApplication::setApplicationVersion( VERSION );

    std::locale::global( std::locale::classic() );

    if ( ! Options::instance().parseCmdLine( argc, argv ) )
    {
        return 1;
    }

    MainWindow win;
    win.show();
    win.init();

    return app.exec();
}
