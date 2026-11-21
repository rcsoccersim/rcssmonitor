# Uninstall.cmake
# ---------------
#
# Remove the files installed by "cmake --install". This script is run by the
# "uninstall" target:
#
#   cmake --build build --target uninstall
#
# The files listed in install_manifest.txt (created in the build directory by
# the install) are removed. The directories of the package's own (the headers
# of the rcg library, include/rcss/rcg and include/rcss) are also removed if
# they become empty. The other directories (e.g. bin, lib) are not removed.
#
# Input variables (-D<var>=<value>):
#   MANIFEST      the path of install_manifest.txt
#   PACKAGE_DIRS  the package's own directories, as the ends of their paths,
#                 deepest first, separated by '|' (e.g. "rcss/rcg|rcss")

# The policies of the project's minimum version (as CMakeLists.txt).
# Without it, e.g. while(TRUE) is false in the script mode (CMP0012).
cmake_minimum_required(VERSION 3.16)

foreach(var MANIFEST PACKAGE_DIRS)
  if(NOT ${var})
    message(FATAL_ERROR "Uninstall.cmake: ${var} is not set.")
  endif()
endforeach()

if(NOT EXISTS "${MANIFEST}")
  message(FATAL_ERROR "${MANIFEST} is not found. "
    "The uninstall target removes the files installed from this build directory.")
endif()

string(REPLACE "|" ";" package_dirs "${PACKAGE_DIRS}")

file(STRINGS "${MANIFEST}" installed_files)

set(removed_dirs)
foreach(file IN LISTS installed_files)
  set(path "$ENV{DESTDIR}${file}")
  if(IS_SYMLINK "${path}" OR EXISTS "${path}")
    message(STATUS "Uninstalling: ${path}")
    file(REMOVE "${path}")
    if(IS_SYMLINK "${path}" OR EXISTS "${path}")
      message(FATAL_ERROR "Failed to remove ${path}")
    endif()
    get_filename_component(dir "${path}" DIRECTORY)
    list(APPEND removed_dirs "${dir}")
  else()
    message(STATUS "Not found (skipped): ${path}")
  endif()
endforeach()

# Remove the package's own directories if they are empty, going up from the
# directories of the removed files while a directory is one of them.
if(removed_dirs)
  list(REMOVE_DUPLICATES removed_dirs)
endif()
foreach(dir IN LISTS removed_dirs)
  while(TRUE)
    set(own FALSE)
    foreach(end IN LISTS package_dirs)
      string(LENGTH "/${end}" end_length)
      string(LENGTH "${dir}" dir_length)
      if(dir_length GREATER end_length)
        math(EXPR start "${dir_length} - ${end_length}")
        string(SUBSTRING "${dir}" ${start} -1 tail)
        if(tail STREQUAL "/${end}")
          set(own TRUE)
        endif()
      endif()
    endforeach()
    if(NOT own OR NOT IS_DIRECTORY "${dir}")
      break()
    endif()
    file(GLOB entries LIST_DIRECTORIES true "${dir}/*" "${dir}/.*")
    list(FILTER entries EXCLUDE REGEX "/\\.\\.?$")
    if(entries)
      break()
    endif()
    message(STATUS "Removing the empty directory: ${dir}")
    file(REMOVE_RECURSE "${dir}")
    get_filename_component(dir "${dir}" DIRECTORY)
  endwhile()
endforeach()
