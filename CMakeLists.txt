cmake_minimum_required(VERSION 3.5)

project(rcssmonitor VERSION 17.0.0)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

# compiler options
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()
set(CMAKE_CXX_FLAGS "-W -Wall")

include(GNUInstallDirs)

# check header files
include(CheckIncludeFileCXX)
if(WIN32)
  check_include_file("windows.h" HAVE_WINDOWS_H)
endif()
check_include_file_cxx("arpa/inet.h" HAVE_ARPA_INET_H)

# check boost
#find_package(Boost 1.38.0 COMPONENTS program_options system REQUIRED)
#if(Boost_PROGRAM_OPTIONS_FOUND)
#  set(HAVE_BOOST_PROGRAM_OPTIONS TRUE)
#endif()

# check Qt
find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)
if(NOT Qt5_FOUND)
  message(FATAL_ERROR "Qt5 not found!")
endif()

set(CMAKE_AUTOMOC ON)

# check zlib
find_package(ZLIB)
if(ZLIB_FOUND)
  set(HAVE_LIBZ TRUE)
endif()


# generate config.h
add_definitions(-DHAVE_CONFIG_H)
configure_file(
  ${PROJECT_SOURCE_DIR}/cmake-config.h.in
  ${PROJECT_BINARY_DIR}/config.h
  )

#
add_subdirectory(rcss)
add_subdirectory(src)
