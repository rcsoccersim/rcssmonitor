#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([rcssmonitor],[15.1.1],[sserver-admin@users.sf.net])

#LT_PREREQ(2.2.4)

#AM_INIT_AUTOMAKE([gnu 1.7.2 check-news dist-bzip2 dist-zip])
AM_INIT_AUTOMAKE([gnu 1.7.2 check-news])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

##################################################
# Checks for programs.
##################################################

AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX

##################################################
# Checks for libraries.
##################################################

AC_CHECK_LIB([m], [cos],
             [LIBS="-lm $LIBS"],
             [AC_MSG_ERROR([*** -lm not found! ***])])
AC_CHECK_LIB([z], [deflate])

##################################################
# Checks for header files.
##################################################

AC_CHECK_HEADERS([netinet/in.h])

##################################################
# Checks for typedefs, structures, and compiler characteristics.
##################################################

AC_HEADER_STDBOOL
AC_C_INLINE
AC_C_CONST
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

##################################################
# Checks for library functions.
##################################################

AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([memset rint strtol pow sqrt])

# ----------------------------------------------------------
# check boost

AX_BOOST_BASE([1.32.0])
AX_BOOST_SYSTEM
AX_BOOST_PROGRAM_OPTIONS
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"

# ----------------------------------------------------------
# check Qt4
QT4MODULES="QtCore QtGui QtNetwork"

AC_ARG_ENABLE(gl,
              AC_HELP_STRING([--enable-gl],
                             [enable GLWidget for Qt4 (default no)]))
if test "x$enable_gl" == "xyes"; then
  AC_MSG_NOTICE(enabled QOpenGL)
  CFLAGS="-DUSE_GLWIDGET $CFLAGS"
  CXXFLAGS="-DUSE_GLWIDGET $CXXFLAGS"
  QT4MODULES="$QT4MODULES QtOpenGL"
fi

AX_QT4([4.3.0],[$QT4MODULES])

if test x$have_qt4 != xyes ; then
  AC_MSG_ERROR([$QT4MODULES could not be found.])
fi

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
