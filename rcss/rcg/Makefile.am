## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES = librcssrcg.la

librcssrcg_la_SOURCES = \
	handler.cpp \
	parser.cpp \
	parser_v1.cpp \
	parser_v2.cpp \
	parser_v3.cpp \
	parser_v4.cpp \
	parser_json.cpp \
	types.cpp \
	util.cpp \
	xpm_tile.cpp \
	team_graphic.cpp

librcssrcgincludedir = $(includedir)/rcss/rcg

#pkginclude_HEADERS
librcssrcginclude_HEADERS = \
	handler.h \
	parser.h \
	parser_v1.h \
	parser_v2.h \
	parser_v3.h \
	parser_v4.h \
	parser_json.h \
	types.h \
	util.h \
	xpm_tile.h \
	team_graphic.h

noinst_HEADERS = \
	nlohmann/json.hpp

librcssrcg_la_LDFLAGS = -version-info 17:0:0
#libXXXX_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
#		 1. Start with version information of `0:0:0' for each libtool library.
#
#		 2. Update the version information only immediately before a public
#				release of your software.	 More frequent updates are unnecessary,
#				and only guarantee that the current interface number gets larger
#				faster.
#
#		 3. If the library source code has changed at all since the last
#				update, then increment REVISION (`C:R:A' becomes `C:r+1:A').
#
#		 4. If any interfaces have been added, removed, or changed since the
#				last update, increment CURRENT, and set REVISION to 0.
#
#		 5. If any interfaces have been added since the last public release,
#				then increment AGE.
#
#		 6. If any interfaces have been removed since the last public release,
#				then set AGE to 0.

AM_CPPFLAGS = -I$(top_srcdir)
AM_CFLAGS = -Wall -W
AM_CXXFLAGS = -Wall -W
AM_LDFLAGS =

EXTRA_DIST = \
	CMakeLists.txt

CLEANFILES = *~
