# DistCheck.cmake
# ---------------
#
# Create the source tarball (see Dist.cmake) and check that it can be built,
# tested, installed, and uninstalled. This script is run by the "distcheck"
# target:
#
#   cmake --build build --target distcheck
#
# Steps:
#   1. create the tarball by Dist.cmake
#   2. extract it in WORK_DIR and make the sources read-only
#   3. configure, build, test (ctest), install it into WORK_DIR, check the
#      desktop entry, start the installed executable (--version), and
#      uninstall it (no file must be left)
#   4. remove WORK_DIR if all the steps succeed
#      (WORK_DIR is left for inspection if a step fails)
#
# Input variables (-D<var>=<value>), in addition to those of Dist.cmake:
#   WORK_DIR       the working directory (removed and recreated)
#   CTEST_COMMAND  the ctest command
#   GENERATOR      the CMake generator
#   CXX_COMPILER   the C++ compiler
#   QT_MAJOR       the value of QT_VERSION_MAJOR (5 or 6)
#   PREFIX_PATH    the value of CMAKE_PREFIX_PATH ('|' separated)

# The policies of the project's minimum version (as CMakeLists.txt).
# Without it, e.g. while(TRUE) is false in the script mode (CMP0012).
cmake_minimum_required(VERSION 3.16)

foreach(var WORK_DIR CTEST_COMMAND GENERATOR)
  if(NOT ${var})
    message(FATAL_ERROR "DistCheck.cmake: ${var} is not set.")
  endif()
endforeach()

# Guard against removing an unexpected directory.
get_filename_component(work_dir_name "${WORK_DIR}" NAME)
if(NOT work_dir_name STREQUAL "distcheck")
  message(FATAL_ERROR "DistCheck.cmake: WORK_DIR must be a directory named 'distcheck': ${WORK_DIR}")
endif()

find_program(CHMOD_EXECUTABLE chmod)

# Make the directory writable and remove it.
function(distcheck_remove_work_dir dir)
  if(EXISTS "${dir}")
    if(CHMOD_EXECUTABLE)
      execute_process(COMMAND ${CHMOD_EXECUTABLE} -R u+w "${dir}")
    endif()
    file(REMOVE_RECURSE "${dir}")
  endif()
endfunction()

# Run a command; stop with an error if it fails.
function(distcheck_run step)
  message(STATUS "distcheck: ${step}")
  execute_process(COMMAND ${ARGN} RESULT_VARIABLE result)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "distcheck: ${step} failed. "
      "The files are left in ${WORK_DIR} for inspection.")
  endif()
endfunction()

# 1. create the tarball
include(${CMAKE_CURRENT_LIST_DIR}/Dist.cmake)

# 2. extract it and make the sources read-only
distcheck_remove_work_dir("${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")
set(src_dir "${WORK_DIR}/${DIST_DIR_NAME}")
set(build_dir "${WORK_DIR}/build")
set(install_dir "${WORK_DIR}/install")
set(home_dir "${WORK_DIR}/home")
file(MAKE_DIRECTORY "${home_dir}")

message(STATUS "distcheck: extract ${DIST_TARBALL}")
execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar xzf "${DIST_TARBALL}"
  WORKING_DIRECTORY "${WORK_DIR}"
  RESULT_VARIABLE result
  )
if(NOT result EQUAL 0 OR NOT EXISTS "${src_dir}/CMakeLists.txt")
  message(FATAL_ERROR "distcheck: failed to extract ${DIST_TARBALL}")
endif()

if(CHMOD_EXECUTABLE)
  execute_process(COMMAND ${CHMOD_EXECUTABLE} -R a-w "${src_dir}")
else()
  message(STATUS "distcheck: chmod is not found. The sources are not made read-only.")
endif()

# 3. configure, build, test, and install
set(configure_args
  -S "${src_dir}"
  -B "${build_dir}"
  -G "${GENERATOR}"
  -DCMAKE_BUILD_TYPE=Release
  )
if(CXX_COMPILER)
  list(APPEND configure_args "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}")
endif()
if(QT_MAJOR)
  list(APPEND configure_args "-DQT_VERSION_MAJOR=${QT_MAJOR}")
endif()
# The value may contain ';'. It is passed as a quoted argument directly to
# execute_process(), because a list variable would split it.
string(REPLACE "|" ";" prefix_path "${PREFIX_PATH}")

cmake_host_system_information(RESULT jobs QUERY NUMBER_OF_LOGICAL_CORES)
if(NOT jobs OR jobs LESS 1)
  set(jobs 1)
endif()

message(STATUS "distcheck: configure")
execute_process(
  COMMAND ${CMAKE_COMMAND} ${configure_args}
          "-DCMAKE_PREFIX_PATH=${prefix_path}"
  RESULT_VARIABLE result
  )
if(NOT result EQUAL 0)
  message(FATAL_ERROR "distcheck: configure failed. "
    "The files are left in ${WORK_DIR} for inspection.")
endif()
distcheck_run("build" ${CMAKE_COMMAND} --build "${build_dir}" --parallel ${jobs})
message(STATUS "distcheck: test")
execute_process(
  COMMAND ${CTEST_COMMAND} --output-on-failure
  WORKING_DIRECTORY "${build_dir}"
  RESULT_VARIABLE result
  )
if(NOT result EQUAL 0)
  message(FATAL_ERROR "distcheck: test failed. "
    "The files are left in ${WORK_DIR} for inspection.")
endif()
distcheck_run("install" ${CMAKE_COMMAND} --install "${build_dir}" --prefix "${install_dir}")

foreach(file
    bin/rcssmonitor
    include/rcss/rcg/types.h
    share/icons/hicolor/48x48/apps/rcssmonitor.png
    share/applications/rcssmonitor.desktop)
  if(NOT EXISTS "${install_dir}/${file}")
    message(FATAL_ERROR "distcheck: ${file} is not installed. "
      "The files are left in ${WORK_DIR} for inspection.")
  endif()
endforeach()

# Exec of the desktop entry runs the installed executable.
set(desktop_file "${install_dir}/share/applications/rcssmonitor.desktop")
file(STRINGS "${desktop_file}" desktop_exec REGEX "^(Try)?Exec=")
set(expected_exec "TryExec=${install_dir}/bin/rcssmonitor;Exec=\"${install_dir}/bin/rcssmonitor\"")
if(NOT desktop_exec STREQUAL expected_exec)
  message(FATAL_ERROR "distcheck: Exec of the desktop entry is wrong: ${desktop_exec} "
    "(expected: ${expected_exec}). The files are left in ${WORK_DIR} for inspection.")
endif()
find_program(DESKTOP_FILE_VALIDATE desktop-file-validate)
if(DESKTOP_FILE_VALIDATE)
  distcheck_run("validate the desktop entry" ${DESKTOP_FILE_VALIDATE} "${desktop_file}")
endif()

# The installed executable starts without LD_LIBRARY_PATH: the path of its
# library (librcssrcg in lib) is embedded (RPATH). HOME is the work
# directory's, so that the user's ~/.rcssmonitor.conf is neither read nor
# written (rcssmonitor writes it on exit, also with --version).
message(STATUS "distcheck: start the installed executable")
execute_process(
  COMMAND ${CMAKE_COMMAND} -E env --unset=LD_LIBRARY_PATH
          "HOME=${home_dir}" QT_QPA_PLATFORM=offscreen
          "${install_dir}/bin/rcssmonitor" --version
  RESULT_VARIABLE result
  OUTPUT_VARIABLE version_output
  ERROR_VARIABLE version_error
  )
if(NOT result EQUAL 0 OR NOT version_output MATCHES "${PACKAGE_NAME}-${PACKAGE_VERSION}")
  message(FATAL_ERROR "distcheck: the installed executable did not start "
    "(exit ${result}):\n${version_output}${version_error}\n"
    "The files are left in ${WORK_DIR} for inspection.")
endif()

distcheck_run("uninstall" ${CMAKE_COMMAND} --build "${build_dir}" --target uninstall)
file(GLOB_RECURSE remaining_files "${install_dir}/*")
if(remaining_files)
  string(REPLACE ";" "\n  " remaining_files "${remaining_files}")
  message(FATAL_ERROR "distcheck: the following files are left after uninstall:\n  ${remaining_files}\n"
    "The files are left in ${WORK_DIR} for inspection.")
endif()
if(EXISTS "${install_dir}/include/rcss")
  message(FATAL_ERROR "distcheck: include/rcss is left after uninstall. "
    "The files are left in ${WORK_DIR} for inspection.")
endif()

# 4. clean up
distcheck_remove_work_dir("${WORK_DIR}")

message(STATUS "distcheck: ${DIST_TARBALL} is ready for distribution.")
