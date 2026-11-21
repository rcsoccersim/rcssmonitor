# Qt6 needs CMake 3.16 or later
cmake_minimum_required(VERSION 3.16)

project(rcssmonitor VERSION 19.0.1)

# compiler options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

# install destination: ~/.local by default
# (the desktop environments search ~/.local/share for the .desktop file and
# the icons, and ~/.local/bin is in PATH on Ubuntu).
# A build directory configured before keeps its value in the cache.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Install destination path" FORCE)
endif()
include(GNUInstallDirs)

# The installed executable finds the rcg library installed beside it
# (PREFIX/lib, e.g. ~/.local/lib, is not in the library search path).
if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_LIBDIR}")
else()
  set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()

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
# Not find_package(QT NAMES Qt6 Qt5): it takes the one that it finds first
# in the directory, which is not always Qt6.
if(NOT QT_VERSION_MAJOR)
  find_package(Qt6 6.2 QUIET COMPONENTS Core)
  if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6 CACHE STRING "The major version of Qt: 5 or 6")
  else()
    set(QT_VERSION_MAJOR 5 CACHE STRING "The major version of Qt: 5 or 6")
  endif()
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

# check Qt WebSockets
option(ENABLE_WEBSOCKET "Enable the WebSocket connection (requires Qt WebSockets)" ON)
if(ENABLE_WEBSOCKET)
  find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WebSockets QUIET)
  if(Qt${QT_VERSION_MAJOR}WebSockets_FOUND)
    set(HAVE_QT_WEBSOCKETS TRUE)
  else()
    message(WARNING "Qt${QT_VERSION_MAJOR} WebSockets not found. The WebSocket connection is disabled.")
  endif()
endif()

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
# The directory of the executable is embedded in TryExec and Exec at the
# installation, so that it is correct even if the prefix is given by
# "cmake --install --prefix DIR". DESTDIR is not included in the path.
if(IS_ABSOLUTE "${CMAKE_INSTALL_BINDIR}")
  set(_rcssmonitor_desktop_bindir "${CMAKE_INSTALL_BINDIR}")
else()
  set(_rcssmonitor_desktop_bindir "\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
endif()
install(CODE "
  set(RCSSMONITOR_BINDIR \"${_rcssmonitor_desktop_bindir}\")
  file(READ \"${PROJECT_SOURCE_DIR}/data/rcssmonitor.desktop.in\" _rcssmonitor_desktop)
  string(CONFIGURE \"\${_rcssmonitor_desktop}\" _rcssmonitor_desktop @ONLY)
  file(WRITE \"${PROJECT_BINARY_DIR}/rcssmonitor.desktop\" \"\${_rcssmonitor_desktop}\")
  ")
install(FILES ${PROJECT_BINARY_DIR}/rcssmonitor.desktop
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

# uninstall: cmake --build build --target uninstall
# (removes the files listed in install_manifest.txt of the build directory,
# and the directories of the rcg library's headers if they become empty)
add_custom_target(uninstall
  COMMAND ${CMAKE_COMMAND}
          -DMANIFEST=${PROJECT_BINARY_DIR}/install_manifest.txt
          -DPACKAGE_DIRS=rcss/rcg|rcss
          -P ${PROJECT_SOURCE_DIR}/cmake/Uninstall.cmake
  COMMENT "Uninstalling ${PROJECT_NAME}"
  VERBATIM
  )

# source tarball: cmake --build build --target dist
# (rcssmonitor-VERSION.tar.gz, created from the committed files of HEAD by git archive)
set(RCSSMONITOR_DIST_DIR "${PROJECT_SOURCE_DIR}" CACHE PATH
  "Directory where the dist target creates the source tarball (default: the top source directory)")
find_package(Git QUIET)
add_custom_target(dist
  COMMAND ${CMAKE_COMMAND}
          -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
          -DOUTPUT_DIR=${RCSSMONITOR_DIST_DIR}
          -DPACKAGE_NAME=${PROJECT_NAME}
          -DPACKAGE_VERSION=${PROJECT_VERSION}
          -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
          -P ${PROJECT_SOURCE_DIR}/cmake/Dist.cmake
  COMMENT "Creating the source tarball"
  VERBATIM
  )

# check the source tarball: cmake --build build --target distcheck
# (extract, configure, build, test, install, and uninstall it in <build>/distcheck)
# The list is passed with '|' because ';' in a custom command is a separator.
string(REPLACE ";" "|" _rcssmonitor_prefix_path "${CMAKE_PREFIX_PATH}")
add_custom_target(distcheck
  COMMAND ${CMAKE_COMMAND}
          -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
          -DOUTPUT_DIR=${RCSSMONITOR_DIST_DIR}
          -DPACKAGE_NAME=${PROJECT_NAME}
          -DPACKAGE_VERSION=${PROJECT_VERSION}
          -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
          -DWORK_DIR=${PROJECT_BINARY_DIR}/distcheck
          -DCTEST_COMMAND=${CMAKE_CTEST_COMMAND}
          -DGENERATOR=${CMAKE_GENERATOR}
          -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
          -DQT_MAJOR=${QT_VERSION_MAJOR}
          -DPREFIX_PATH=${_rcssmonitor_prefix_path}
          -P ${PROJECT_SOURCE_DIR}/cmake/DistCheck.cmake
  COMMENT "Checking the source tarball"
  VERBATIM
  USES_TERMINAL
  )
