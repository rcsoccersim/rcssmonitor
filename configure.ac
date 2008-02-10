dnl Process this file with autoconf to produce a configure script.
AC_INIT([RCSSMonitor], 12.0.0_pre-20080210, sserver-admin@users.sf.net, rcssmonitor)

AC_PREREQ(2.59)

AC_CONFIG_SRCDIR([src/2dview.C])
#AM_INIT_AUTOMAKE([gnu 1.7.2 check-news dist-bzip2 dist-zip])
AM_INIT_AUTOMAKE([gnu 1.7.2 check-news])
AM_CONFIG_HEADER(config.h)
AC_PROG_MAKE_SET

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_AWK

dnl Checks for libraries.
AC_CHECK_LIB(m, cos)

dnl Checks for header files
##AC_PATH_X.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])
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
AC_TYPE_SIZE_T
AC_HEADER_TIME

AC_CHECK_TYPES([socklen_t], [], [], [
#include <sys/types.h>
#include <sys/socket.h>
])

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL
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
    echo disabling x
  else
    save_LDFLAGS=$LDFLAGS
    LDFLAGS="$LDFLAGS $X_LIBS"
    AC_CHECK_LIB([Xpm], [XpmCreatePixmapFromData],
                 [X_LIBS="$X_LIBS -lXpm"],
                 [AC_MSG_ERROR([*** -lXpm not found! ***])])
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

AC_ARG_VAR(SED, [name and location of sed executable.  If it is not
set, it configure looks for sed in your PATH])

if test "x$SED" = "x"; then
    AC_PATH_PROG(SED, sed)
fi

AX_EXTRA_DIST
AX_UPLOAD([ncftpput -v upload.sourceforge.net /incoming %%s])
AX_RPM_GEN_SPEC([spec.tmpl],
            [The RoboCup Soccer Simulator Monitor],
            [GPL],
            [Applications/Engineering],
            [http://sserver.sf.net],
            [http://prdownloads.sourceforge.net/sserver/],
[The RoboCup Soccer Simulator Monitor (rcssmonitor) is a viewer for moved 2d vector
graphics. It runs on Linux (Unix) computers that run the X Window System.])
AX_DIST_RPM([spec.tmpl])
#AX_DIST_MSI([Setup/Release/${PACKAGE_NAME}Setup.msi])
AX_CVS([:pserver:anonymous@cvs.sourceforge.net:/cvsroot/sserver])

AC_CONFIG_FILES([Makefile
     src/Makefile])
AC_OUTPUT
