dnl Process this file with autoconf to produce a configure script.
AC_INIT([RCSSMonitor], 13.1.0, sserver-admin@users.sf.net, rcssmonitor)

AC_PREREQ(2.61)

#AM_INIT_AUTOMAKE([gnu 1.7.2 check-news dist-bzip2 dist-zip])
AM_INIT_AUTOMAKE([gnu 1.7.2 check-news])
AC_CONFIG_SRCDIR([config.h.in])
AM_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_MAKE_SET

dnl Checks for libraries.
AC_CHECK_LIB([m], [cos])

dnl Checks for header files
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])
AC_CHECK_HEADERS(linux/joystick.h)

AC_ARG_WITH(sstream,
            AC_HELP_STRING([--with-sstream],
                           [use sstream instead of strstream (default=yes)]),
                            use_sstream=$withval, use_sstream=yes)
if test "$use_sstream" = "yes"; then
   AC_CXX_HAVE_SSTREAM
fi


dnl Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
#AC_TYPE_INT16_T
#AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

AC_CHECK_TYPES([socklen_t], [], [], [
#include <sys/types.h>
#include <sys/socket.h>
])

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_HEADER_STDC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRTOD
AC_CHECK_FUNCS([floor gethostbyname gettimeofday inet_ntoa memset rint select socket sqrt strtol])

AX_BOOST_BASE(1.32.0)
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"


dnl optional features


AC_SUBST(COMP_FLAGS)
AC_ARG_ENABLE(debug,
[  --enable-debug[=ARG]    build with debugging information [ARG=no]],
if test -n $enableval; then
  if test $enableval = yes; then
    COMP_FLAGS=-g
    AC_DEFINE(DEBUG, 1, debuging option)
    echo enabling debug
  elif test $enableval = no; then
    COMP_FLAGS=
    echo disabling debug
  fi
fi
, COMP_FLAGS=)

no_x=no
AC_PATH_XTRA
#AC_SUBST(X_LIBS)
if test -n $no_x; then
  if test $no_x = yes; then
#    echo disabling x
    AC_MSG_ERROR([
    X libraries cannot be found on your system. Please
    install them, or specify the library location.])
  else
    save_LDFLAGS=$LDFLAGS
    LDFLAGS="$LDFLAGS $X_LIBS"
    AC_CHECK_LIB([Xpm], [XpmCreatePixmapFromData],
                 [X_LIBS="$X_LIBS -lXpm"],
                 [AC_MSG_ERROR([*** libXpm cannot be found on your system! ***])])
    AC_CHECK_HEADER([X11/xpm.h],
                    [AC_DEFINE([HAVE_XPM_H],,[The presence of libXpm header file.])],
                    [AC_MSG_ERROR([*** X11/xpm.h cannot be found on your system! ***])])
    AC_CHECK_LIB([X11], [XFree],
                 [X_LIBS="$X_LIBS -lX11"])
  fi
  LDFLAGS=$save_LDFLAGS
fi

AC_SUBST(CLIENT2DVIEW)
AC_ARG_ENABLE(client2dview,
[  --enable-client2dview[=ARG] build client2dview [ARG=yes]],
if test -n $enableval; then
  if test $enableval = yes; then
    CLIENT2DVIEW=client2dview
    echo enabling client2dview
  elif test $enableval = no; then
    CLIENT2DVIEW=
    echo disabling client2dview
  fi
fi
, CLIENT2DVIEW=client2dview)

AC_DEFINE_UNQUOTED(PACKAGE_VERSION, "${PACKAGE}-${VERSION}", Package name and version)

AC_CONFIG_FILES([Makefile
                src/Makefile])
AC_OUTPUT
