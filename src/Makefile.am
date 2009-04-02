
bin_PROGRAMS = \
	rcssmonitor @CLIENT2DVIEW@

rcssmonitor_SOURCES = \
	angle.C \
	area2d.C \
	ascii_dev.C \
	ascii_processor.C \
	builder_direct.C \
	coder.C \
	conv.C \
	conv_area2d.C \
	display_x11.C \
	frame2d.C \
	menu_x11.C \
	options.cpp \
	popup_x11.C \
	rectangle.cpp \
	rgb_db.C \
	rgbcolor.C \
	smonitor_dev.C \
	str2val.C \
	structure.C \
	tcpsocket.C \
	team_graphic.cpp \
	tools.C \
	udpsocket.C \
	valueparser.C \
	vector2d.C \
	visobject.C \
	2dview.C

rcssmonitor_LDADD = @X_PRE_LIBS@ @X_LIBS@ @X_EXTRA_LIBS@

noinst_HEADERS = \
	angle.h \
	area2d.h \
	ascii_dev.h \
	ascii_processor.h \
	builder_base.h \
	builder_direct.h \
	coder.h \
	conv.h \
	conv_area2d.h \
	display_base.h \
	display_x11.h \
	frame2d.h\
	input_dev.h \
	maplist.h \
	menu_base.h
	menu_x11.h \
	multi.h \
	object2d.h \
	options.h
	point2d.h \
	popup_x11.h \
	rgbcolor.h \
	rectangle.h \
	rgb_db.h\
	rwbin.h \
	smonitor_dev.h \
	str2val.h \
	structure.h \
	tcpsocket.h \
	team_graphic.h \
	tools.h \
	udpsocket.h \
	valueparser.h \
	vector2d.h \
	visobject.h


AM_CPPFLAGS =
AM_CFLAGS = -W -Wall
AM_CXXFLAGS = -W -Wall
AM_LDFLAGS =

EXTRA_PROGRAMS = \
	client2dview

client2dview_SOURCES = \
	angle.C \
	client2dview.C \
	coder.C \
	vector2d.C \
	udpsocket.C \
	tcpsocket.C \
	valueparser.C

pkgdata_DATA = \
	colors.fvd \
	field_robocup_middlesize.fvd \
	rcssmonitor.png \
	rcssmonitor.xpm

EXTRA_DIST = \
	colors.fvd \
	field_robocup_middlesize.fvd \
	rcssmonitor.png \
	rcssmonitor.xpm \
	rcssmonitor.conf

CLEANFILES = \
	*~ \
	core
