ACLOCAL_AMFLAGS=-I m4

SUBDIRS = rcss src .

EXTRA_DIST = \
	README.md \
	docs/build.md \
	CMakeLists.txt \
	cmake-config.h.in \
	icons/accelerate.xpm \
	icons/blank.xpm \
	icons/decelerate.xpm \
	icons/ff.xpm \
	icons/go.xpm \
	icons/live.xpm \
	icons/minus.xpm \
	icons/next_score.xpm \
	icons/open.xpm \
	icons/play.xpm \
	icons/plus.xpm \
	icons/prev_score.xpm \
	icons/quit.xpm \
	icons/rcss.icns \
	icons/rcss.ico \
	icons/rcss.xpm \
	icons/rec.xpm \
	icons/rev.xpm \
	icons/rew.xpm \
	icons/stop.xpm \
	test/CMakeLists.txt \
	test/check_icons.sh \
	test/test_message_splitter.cpp \
	icons/generate.sh \
	icons/generated.sha256 \
	icons/icons.am \
	icons/icons.qrc \
	icons/palette.awk \
	icons/palette.txt \
	$(ICONS_SVG_FILES) \
	$(ICONS_PNG_FILES)

# the icons embedded with the Qt resource system (icons/generate.sh; UIIcon).
# icons.am defines the lists of the files (ICONS_SVG_FILES, ICONS_PNG_FILES).
ICONS_DIR = icons
include $(srcdir)/icons/icons.am

CLEANFILES = *~
