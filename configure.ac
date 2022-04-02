#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([rcssmonitor],[17.0.0],[https://github.com/rcsoccersim/rcssmonitor])

#LT_PREREQ(2.2.4)

#AM_INIT_AUTOMAKE([gnu 1.7.2 check-news dist-bzip2 dist-zip])
AM_INIT_AUTOMAKE([gnu 1.7.2 check-news foreign])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

##################################################
# libtool settings
##################################################

LT_INIT([shared disable-static])
LT_LANG([C++])
AC_SUBST(LIBTOOL_DEPS)

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
#AC_CHECK_LIB([z], [deflate])
AX_CHECK_ZLIB([],
              [AC_MSG_NOTICE(Zlib not found.)])


##################################################
# Checks for header files.
##################################################

AC_CHECK_HEADERS([arpa/inet.h])

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

AX_HAVE_QT
if test "x$have_qt" != "xyes"; then
  AC_MSG_ERROR([Qt not found.])
fi

# ----------------------------------------------------------
# check C++

AX_CXX_COMPILE_STDCXX_14(noext)

# ----------------------------------------------------------
# check boost

#AX_BOOST_BASE([1.38.0])
#AX_BOOST_SYSTEM
#AX_BOOST_PROGRAM_OPTIONS
#CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
#LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"

# ----------------------------------------------------------
AC_CONFIG_FILES([Makefile
                 rcss/Makefile
				 rcss/rcg/Makefile
                 src/Makefile])
AC_OUTPUT
