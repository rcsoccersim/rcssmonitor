cmake_minimum_required(VERSION 3.5)

project(rcssmonitor VERSION 16.0.0)

add_definitions(-DPACKAGE_NAME=\"rcssmonitor\")
add_definitions(-DVERSION=\"16.0.0\")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_AUTOMOC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Boost 1.36.0 COMPONENTS program_options system REQUIRED)

find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)
if(NOT Qt5_FOUND)
  find_package(Qt4 4.1.0 COMPONENTS QtCore QtGui QtNetwork REQUIRED)
  if(NOT Qt4_FOUND)
	message(FATAL_ERROR "Qt not found!")
  endif()
endif()

find_package(ZLIB)

option(HAVE_NETINET_IN_H "use netinet/in.h" ON)
if(HAVE_NETINET_IN_H)
  add_definitions(-DHAVE_NETINET_IN_H)
endif()

option(HAVE_WINDOWS_H "use windows.h" OFF)
if(HAVE_WINDOWS_H)
  add_definitions(-DHAVE_WINDOWS_H)
endif()

if(Boost_PROGRAM_OPTIONS_FOUND)
#  message( "found boost::program_options")
  add_definitions(-DHAVE_BOOST_PROGRAM_OPTIONS)
endif()

if(ZLIB_FOUND)
  add_definitions(-DHAVE_LIBZ)
endif()

add_executable(rcssmonitor
  src/rcsslogplayer/gzfstream.cpp
  src/rcsslogplayer/parser.cpp
  src/rcsslogplayer/types.cpp
  src/rcsslogplayer/util.cpp
  src/angle_deg.cpp
  src/ball_painter.cpp
  src/circle_2d.cpp
  src/config_dialog.cpp
  src/disp_holder.cpp
  src/draw_info_painter.cpp
  src/field_canvas.cpp
  src/field_painter.cpp
  src/line_2d.cpp
  src/log_player.cpp
  src/log_player_slider.cpp
  src/main_window.cpp
  src/monitor_client.cpp
  src/options.cpp
  src/player_painter.cpp
  src/player_type_dialog.cpp
  src/score_board_painter.cpp
  src/team_graphic.cpp
  src/team_graphic_painter.cpp
  src/vector_2d.cpp
  src/main.cpp
  )

target_include_directories(rcssmonitor PUBLIC
  "${PROJECT_BINARY_DIR}"
  "${Boost_INCLUDE_DIRS}"
  "${ZLIB_INCLUDE_DIRS}"
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/src"
  )

if(Qt5_FOUND)
  target_link_libraries(rcssmonitor
	Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network
	Boost::program_options Boost::system
	ZLIB::ZLIB
	)
elseif(Qt4_FOUND)
  target_link_libraries(rcssmonitor
	Qt4::QtCore Qt4::QtGui Qt4::QtNetwork
	Boost::program_options Boost::system
	ZLIB::ZLIB
	)
endif()


install(TARGETS rcssmonitor DESTINATION bin)
