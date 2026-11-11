cmake_minimum_required(VERSION 3.5)

project(rcssmonitor VERSION 19.0.1)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

# compiler options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

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

# check libdeflate (optional): decompresses gzip game logs faster than zlib
option(ENABLE_LIBDEFLATE "Use libdeflate to decompress game logs, if it is found" ON)
if(ENABLE_LIBDEFLATE)
  find_path(LIBDEFLATE_INCLUDE_DIR libdeflate.h)
  find_library(LIBDEFLATE_LIBRARY NAMES deflate libdeflate)
  if(LIBDEFLATE_INCLUDE_DIR AND LIBDEFLATE_LIBRARY)
    set(HAVE_LIBDEFLATE TRUE)
    message(STATUS "Found libdeflate: ${LIBDEFLATE_LIBRARY}")
  else()
    message(STATUS "libdeflate not found. Game logs are decompressed with zlib.")
  endif()
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

# desktop entry and icon
install(FILES data/rcssmonitor.desktop
  DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
  )
install(FILES icons/rcssmonitor.png
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps
  )

# unit tests
find_package(Qt5 COMPONENTS Test QUIET)
if(Qt5Test_FOUND)
  enable_testing()
  add_subdirectory(test)
else()
  message(STATUS "Qt5 Test not found. The unit tests are not built.")
endif()
