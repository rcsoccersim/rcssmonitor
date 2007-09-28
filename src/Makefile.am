bin_PROGRAMS = \
rcssmonitor @CLIENT2DVIEW@

rcssmonitor_SOURCES = \
2dview.C \
angle.C \
area2d.C \
coder.C \
conv.C \
display_x11.C \
frame2d.C \
rgbcolor.C \
structure.C \
vector2d.C \
visobject.C \
rgb_db.C \
smonitor_dev.C \
ascii_dev.C \
builder_direct.C \
ascii_processor.C \
menu_x11.C \
popup_x11.C \
tools.C \
conv_area2d.C \
valueparser.C \
str2val.C \
udpsocket.C \
tcpsocket.C \
angle.h            conv_area2d.h   maplist.h    rgb_db.h        valueparser.h \
area2d.h           conv.h          menu_base.h  rwbin.h         vector2d.h \
ascii_dev.h        display_base.h  menu_x11.h   smonitor_dev.h  visobject.h \
ascii_processor.h  display_x11.h   multi.h      str2val.h \
builder_base.h     frame2d.h       object2d.h   structure.h \
builder_direct.h   global_defs.h   point2d.h    tools.h \
coder.h            input_dev.h     rgbcolor.h   udpsocket.h tcpsocket.h popup_x11.h

rcssmonitor_CXXFLAGS =
rcssmonitor_LDADD = @X_PRE_LIBS@ @X_LIBS@ @X_EXTRA_LIBS@

AM_CXXFLAGS = -Wall @COMP_FLAGS@ @X_CFLAGS@

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

@INCLUDE_MK@