#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([rcssmonitor],[19.0.1],[https://github.com/rcsoccersim/rcssmonitor])

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

# libdeflate (optional): decompresses gzip game logs faster than zlib
AC_ARG_WITH([libdeflate],
            [AS_HELP_STRING([--without-libdeflate],
                            [do not use libdeflate to decompress game logs (default: use it if it is found)])],
            [],
            [with_libdeflate=check])
have_libdeflate=no
AS_IF([test "x$with_libdeflate" != xno],
      [AC_CHECK_HEADER([libdeflate.h],
                       [AC_CHECK_LIB([deflate], [libdeflate_gzip_decompress_ex],
                                     [LIBDEFLATE_LIBS="-ldeflate"
                                      have_libdeflate=yes
                                      AC_DEFINE([HAVE_LIBDEFLATE], [1],
                                                [Define to 1 if you have libdeflate.])])])
       AS_IF([test "x$with_libdeflate" = xyes && test "x$have_libdeflate" != xyes],
             [AC_MSG_ERROR([*** libdeflate not found ***])])])
AC_SUBST([LIBDEFLATE_LIBS])


##################################################
# Checks for header files.
##################################################

AC_CHECK_HEADERS([arpa/inet.h],
                 break,
                 [AC_MSG_ERROR([*** arpa/inet.h not found ***])])

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

AX_CXX_COMPILE_STDCXX_17(noext)

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
