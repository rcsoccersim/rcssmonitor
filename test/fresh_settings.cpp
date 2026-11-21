// -*-c++-*-

/*!
  \file fresh_settings.cpp
  \brief every test starts from the default options
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

/*
  Options reads ~/.rcssmonitor.conf when it is made and writes it when the
  program ends. CTest runs every test with HOME in the build tree
  (test/CMakeLists.txt), and one test that fails half way (with an option
  toggled) would leave its options there for every later run. So, before
  main(), the settings file there is removed, and the test starts from the
  defaults.

  Only when HOME is the test home that CTest gives (RCSSMONITOR_TEST_HOME):
  a test run by hand with the user's HOME does not remove the user's file.
*/

namespace {

struct FreshSettings {
    FreshSettings()
      {
          const char * home = std::getenv( "HOME" );
          const char * test_home = std::getenv( "RCSSMONITOR_TEST_HOME" );
          if ( home && test_home
               && std::strcmp( home, test_home ) == 0 )
          {
              std::remove( ( std::string( home ) + "/.rcssmonitor.conf" ).c_str() );
          }
      }
};

const FreshSettings fresh_settings;

}
