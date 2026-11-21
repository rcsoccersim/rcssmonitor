# Dist.cmake
# ----------
#
# Create the source tarball from the committed files of HEAD by "git archive".
# This script is run by the "dist" target:
#
#   cmake --build build --target dist
#
# The tarball is named <PACKAGE_NAME>-<PACKAGE_VERSION>.tar.gz (the version
# of project() in CMakeLists.txt), and it is extracted to
# <PACKAGE_NAME>-<PACKAGE_VERSION>/.
# The files with the "export-ignore" attribute in .gitattributes are excluded.
# Uncommitted changes are not included.
#
# Input variables (-D<var>=<value>):
#   SOURCE_DIR       the top source directory (a git working tree)
#   OUTPUT_DIR       the directory where the tarball is created
#   PACKAGE_NAME     the package name (rcssmonitor)
#   PACKAGE_VERSION  the package version (e.g. 19.0.1)
#   GIT_EXECUTABLE   the git command
#
# Output variables (when this script is included by another script):
#   DIST_TARBALL   the path of the created tarball
#   DIST_DIR_NAME  the name of the top directory in the tarball

# The policies of the project's minimum version (as CMakeLists.txt).
# Without it, e.g. while(TRUE) is false in the script mode (CMP0012).
cmake_minimum_required(VERSION 3.16)

foreach(var SOURCE_DIR OUTPUT_DIR PACKAGE_NAME PACKAGE_VERSION)
  if(NOT ${var})
    message(FATAL_ERROR "Dist.cmake: ${var} is not set.")
  endif()
endforeach()
if(NOT GIT_EXECUTABLE)
  message(FATAL_ERROR "git is not found. The dist target requires git.")
endif()

# The source directory must be the top of a git working tree.
# (If the sources extracted from a tarball are placed in another git working
# tree, "git archive" would archive the other repository.)
execute_process(
  COMMAND ${GIT_EXECUTABLE} rev-parse --show-toplevel
  WORKING_DIRECTORY ${SOURCE_DIR}
  RESULT_VARIABLE result
  OUTPUT_VARIABLE toplevel
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
  )
get_filename_component(source_dir_real "${SOURCE_DIR}" REALPATH)
if(toplevel)
  get_filename_component(toplevel "${toplevel}" REALPATH)
endif()
if(NOT result EQUAL 0 OR NOT toplevel STREQUAL source_dir_real)
  message(FATAL_ERROR "${SOURCE_DIR} is not the top of a git working tree. "
    "The dist target creates the tarball from the git repository of ${PACKAGE_NAME}.")
endif()

# warn about uncommitted changes
execute_process(
  COMMAND ${GIT_EXECUTABLE} status --porcelain --untracked-files=no
  WORKING_DIRECTORY ${SOURCE_DIR}
  OUTPUT_VARIABLE changes
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )
if(changes)
  message(WARNING "There are uncommitted changes. "
    "The tarball is created from HEAD and does not include them:\n${changes}")
endif()

# the name of the release, as the tarballs made by "make dist" of autotools
set(DIST_DIR_NAME "${PACKAGE_NAME}-${PACKAGE_VERSION}")
set(DIST_TARBALL "${OUTPUT_DIR}/${DIST_DIR_NAME}.tar.gz")

file(MAKE_DIRECTORY ${OUTPUT_DIR})
execute_process(
  COMMAND ${GIT_EXECUTABLE} archive --format=tar.gz
          --prefix=${DIST_DIR_NAME}/
          --output=${DIST_TARBALL}
          HEAD
  WORKING_DIRECTORY ${SOURCE_DIR}
  RESULT_VARIABLE result
  )
if(NOT result EQUAL 0)
  message(FATAL_ERROR "git archive failed.")
endif()

message(STATUS "Created ${DIST_TARBALL}")
