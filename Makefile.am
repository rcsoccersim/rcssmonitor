ACLOCAL_AMFLAGS=-I m4

SUBDIRS = rcss src .

EXTRA_DIST = \
	README.md \
	docs/build.md \
	CMakeLists.txt \
	cmake-config.h.in \
	icons/rcss.ico \
	test/CMakeLists.txt \
	test/check_icons.sh \
	test/test_message_splitter.cpp \
	icons/app/make_rcssmonitor_svg.py \
	icons/app/robocup_logo.png \
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
