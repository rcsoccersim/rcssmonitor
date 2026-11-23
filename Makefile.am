ACLOCAL_AMFLAGS=-I m4

SUBDIRS = rcss src .

EXTRA_DIST = \
	README.md \
	docs/build.md \
	docs/icons.md \
	docs/icons_en.md \
	data/rcssmonitor.desktop.in \
	CMakeLists.txt \
	cmake-config.h.in \
	icons/rcss.ico \
	test/CMakeLists.txt \
	test/check_desktop_entry.sh \
	test/check_icons.sh \
	test/test_message_splitter.cpp \
	icons/app/make_rcssmonitor_svg.py \
	icons/app/robocup_logo.png \
	resources/sounds/sounds.qrc \
	resources/sounds/whistle_long.wav \
	resources/sounds/whistle_short.wav \
	tools/make_whistle.py \
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

# the desktop entry: the directory of the executable is put in TryExec and
# Exec (as CMakeLists.txt does), so that the entry starts it even if bindir
# (e.g. ~/.local/bin) is not in PATH. DESTDIR is not included in the path.
desktopdir = $(datadir)/applications
desktop_DATA = rcssmonitor.desktop
rcssmonitor.desktop: $(srcdir)/data/rcssmonitor.desktop.in Makefile
	sed -e 's|@RCSSMONITOR_BINDIR@|$(bindir)|g' $(srcdir)/data/rcssmonitor.desktop.in > $@

# the application icon of all the sizes, and the scalable one, in the
# freedesktop.org hicolor theme (the desktop entry's Icon=rcssmonitor)
hicolor16dir = $(datadir)/icons/hicolor/16x16/apps
hicolor16_DATA = icons/app/16/rcssmonitor.png
hicolor32dir = $(datadir)/icons/hicolor/32x32/apps
hicolor32_DATA = icons/app/32/rcssmonitor.png
hicolor48dir = $(datadir)/icons/hicolor/48x48/apps
hicolor48_DATA = icons/app/48/rcssmonitor.png
hicolor64dir = $(datadir)/icons/hicolor/64x64/apps
hicolor64_DATA = icons/app/64/rcssmonitor.png
hicolor128dir = $(datadir)/icons/hicolor/128x128/apps
hicolor128_DATA = icons/app/128/rcssmonitor.png
hicolor256dir = $(datadir)/icons/hicolor/256x256/apps
hicolor256_DATA = icons/app/256/rcssmonitor.png
hicolorscalabledir = $(datadir)/icons/hicolor/scalable/apps
hicolorscalable_DATA = icons/app/svg/rcssmonitor.svg

CLEANFILES = *~ rcssmonitor.desktop
