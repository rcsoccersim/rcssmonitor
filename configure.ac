#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([rcssmonitor],[16.1.0],[https://github.com/rcsoccersim/rcssmonitor])

#LT_PREREQ(2.2.4)

#AM_INIT_AUTOMAKE([gnu 1.7.2 check-news dist-bzip2 dist-zip])
AM_INIT_AUTOMAKE([gnu 1.7.2 check-news foreign])
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
# check Qt

AC_ARG_ENABLE(qt4,
              AC_HELP_STRING([--enable-qt4],
                             [use Qt4 tool kit (default no)]))
AC_ARG_ENABLE(gl,
              AC_HELP_STRING([--enable-gl],
                             [enable GLWidget for Qt (default no)]))

if test "x$enable_qt4" != "xyes"; then
  AC_MSG_NOTICE(enabled Qt5)

  QTMODULES="Qt5Core Qt5Gui Qt5Widgets Qt5Network"

  if test "x$enable_gl" == "xyes"; then
    AC_MSG_NOTICE(enabled GL5Widget)
    CFLAGS="-DUSE_GLWIDGET $CFLAGS"
    CXXFLAGS="-DUSE_GLWIDGET $CXXFLAGS"
    QTMODULES="$QTMODULES Qt5OpenGL"
  fi

  AX_QT([5.0.0],[$QTMODULES])

else
  AC_MSG_NOTICE(enabled Qt4)

  QTMODULES="QtCore QtGui QtNetwork"

  if test "x$enable_gl" == "xyes"; then
    AC_MSG_NOTICE(enabled GLWidget)
    CFLAGS="-DUSE_GLWIDGET $CFLAGS"
    CXXFLAGS="-DUSE_GLWIDGET $CXXFLAGS"
    QTMODULES="$QTMODULES QtOpenGL"
  fi

  AX_QT([4.3.0],[$QTMODULES])
fi

if test x$have_qt != xyes ; then
  AC_MSG_ERROR([$QTMODULES could not be found.])
  AM_CONDITIONAL(BUILD_QT, [test "1" = "0"])
else
  CFLAGS="-fPIC $CFLAGS"
  CXXFLAGS="-fPIC $CXXFLAGS"
  AM_CONDITIONAL(BUILD_QT, [test "1" = "1"])
fi

# ----------------------------------------------------------
# check C++

AX_CXX_COMPILE_STDCXX_14(noext)

# ----------------------------------------------------------
# check boost

AX_BOOST_BASE([1.38.0])
AX_BOOST_SYSTEM
AX_BOOST_PROGRAM_OPTIONS
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"

# ----------------------------------------------------------
AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
