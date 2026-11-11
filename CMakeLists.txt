# Qt6 needs CMake 3.16 or later
cmake_minimum_required(VERSION 3.16)

project(rcssmonitor VERSION 19.0.1)

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

# check Qt: Qt6 if it is found, else Qt5. -DQT_VERSION_MAJOR=5 selects Qt5.
if(NOT QT_VERSION_MAJOR)
  find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
endif()
if(QT_VERSION_MAJOR EQUAL 6)
  set(QT_MIN_VERSION 6.2)
  # use none of the API that Qt 6.0 or earlier deprecated
  set(QT_DISABLE_DEPRECATED_BEFORE 0x060000)
elseif(QT_VERSION_MAJOR EQUAL 5)
  set(QT_MIN_VERSION 5.15)
  # use none of the API that Qt 5.15 or earlier deprecated: most of it is
  # gone in Qt6
  set(QT_DISABLE_DEPRECATED_BEFORE 0x050F00)
else()
  message(FATAL_ERROR "Unsupported Qt version: ${QT_VERSION_MAJOR}")
endif()
find_package(Qt${QT_VERSION_MAJOR} ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Widgets Network)
message(STATUS "Qt: ${Qt${QT_VERSION_MAJOR}_VERSION}")
add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=${QT_DISABLE_DEPRECATED_BEFORE})

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
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test QUIET)
if(Qt${QT_VERSION_MAJOR}Test_FOUND)
  enable_testing()
  add_subdirectory(test)
else()
  message(STATUS "Qt${QT_VERSION_MAJOR} Test not found. The unit tests are not built.")
endif()
