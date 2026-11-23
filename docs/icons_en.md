# Icons

[日本語](icons.md) | English

This document describes how the icons of rcssmonitor are made and embedded, and how to add or change an icon.
The mechanism is the same as soccerwindow2's (the plan `docs/plan/2026-11-22-01_icon-redesign_en.md`).

- [Overview](#overview)
- [Files](#files)
- [Adding or changing an icon](#adding-or-changing-an-icon)
- [Drawing rules](#drawing-rules)
- [The application icon](#the-application-icon)
- [palette.txt](#palettetxt)
- [generate.sh](#generatesh)
- [The Qt resource system (.qrc)](#the-qt-resource-system-qrc)
- [Tests](#tests)

## Overview

- Each icon is drawn in an SVG file (`icons/svg/NAME.svg`), in the colors of the light color scheme.
- `icons/generate.sh` makes eight PNG files from the SVG file with Inkscape:
  the images of 16×16, 24×24, 32×32 and 48×48 pixels for the light and the dark color schemes.
  The colors of the dark scheme are taken from `icons/palette.txt`.
  24×24 and 48×48 are for the buttons drawn at 24 pixels (Live Mode and Play).
- The PNG files are committed to the repository, so Inkscape is needed only to make them.
  The build and the CI do not need Inkscape.
- The PNG files are embedded in the program with the Qt resource system (`icons/icons.qrc`).
- The program makes an icon with `UIIcon::fromName( "NAME" )` (`src/ui_icon.h`).
  Whenever the icon is painted, the images of the scheme of the application's palette (light or dark; `UIPalette::isDark()`) are used, so the icons switch with View > UI Color Scheme.
  The size of the image is chosen by the size drawn and the device pixel ratio of the screen (e.g. a 16 pixel icon is drawn from the 32×32 image on a screen whose device pixel ratio is 2).
- The application icon (the window's icon, and the installed icon) is in `icons/app/` and does not switch with the color scheme ([The application icon](#the-application-icon)).
- Ten of the toolbar icons (open, live_mode, play, stop, step_back, step_forward, slower, faster, prev_score, next_score) are copies of soccerwindow2's SVG files (the comment in each names the original file).
  So that the buttons doing the same in the two tools look the same, consider changing them together with soccerwindow2.

## Files

```
icons/
  svg/NAME.svg          the source of an icon (edit this file)
  light/SIZE/NAME.png   the icon for the light scheme (SIZE: 16, 24, 32, 48)  } made by
  dark/SIZE/NAME.png    the icon for the dark scheme (SIZE: 16, 24, 32, 48)   } generate.sh
  palette.txt           the table of the colors (light scheme -> dark scheme)
  palette.awk           checks and replaces the colors (used by generate.sh and the tests)
  generate.sh           makes the PNG files and the lists below
  app/svg/NAME.svg      the source of an application icon (on a 32×32 grid)
  app/SIZE/NAME.png     the application icon (SIZE: 16, 32, 48, 64, 128, 256)  } made by generate.sh
  app/make_rcssmonitor_svg.py  the script that makes the SVG of rcssmonitor's icon
  app/robocup_logo.png  the official RoboCup logo (embedded in rcssmonitor's icon)
  icons.qrc             the Qt resource file (the list of the PNG files)  } made by
  icons.am              the lists of the files for Automake              } generate.sh
  generated.sha256      the checksums of the SVG files and palette.txt   }
  rcss.ico              the icon for Windows (src/rcssmonitor.rc refers to it; not used by today's builds)
  legacy/               the old icons (XPM and so on), kept as a record and not used by the build
test/check_icons.sh     checks the files above
```

Do not edit the files "made by generate.sh" by hand.

## Adding or changing an icon

1. Draw or edit `icons/svg/NAME.svg` with Inkscape (or a text editor), following the [drawing rules](#drawing-rules).
   NAME may have only `a-z`, `0-9` and `_`.
2. Make the PNG files and update the lists.
   ```
   icons/generate.sh NAME
   ```
3. Check the files, and look at the PNG files.
   ```
   test/check_icons.sh
   ```
4. (For a new icon) use the icon in the source code.
   ```cpp
   #include "ui_icon.h"

   act->setIcon( UIIcon::fromName( "NAME" ) );
   ```
5. Build and run the tests (`./cmake-helper.sh test`).
   CMake embeds the new files by itself.
   With autotools, `icons/icons.am` changes, so `make` runs automake again by itself.
6. Commit `icons/svg/NAME.svg`, the eight PNG files, `icons/icons.qrc`, `icons/icons.am` and `icons/generated.sha256`.

To remove an icon, remove the SVG file and the eight PNG files (`git rm`), and run `icons/generate.sh --lists`.
Remove its line from `generated.sha256` by hand (or make all the icons again with `icons/generate.sh`).

## Drawing rules

Follow these rules, so that the icons have the same line widths and colors (the same as soccerwindow2's).

- Size: `width="16" height="16" viewBox="0 0 16 16"`.
  The 24×24, 32×32 and 48×48 images are exported from the same SVG file at 1.5, 2 and 3 times the size.
- Pixel grid: put a 1 pixel line at the center of the pixels (e.g. `x="1.5"`), and the edges of a filled shape on the borders of the pixels (e.g. `x="2"`), so that the 16×16 image is not blurred.
  Then the 32×32 and 48×48 images are not blurred either (24×24 is 1.5 times, and is a little blurred).
- Margin: draw most icons within 1 to 15 (14×14 pixels).
  An outline may touch the edge if the shape needs it.
- Lines: outlines are 1 pixel, in `fg`.
  Symbols (arrows, crosses and so on) are drawn with 1 or 2 pixel lines.
  Lines are horizontal, vertical or at 45° where possible.
- Small circles (7 pixels or less in outer diameter): a `circle` is blurred at 16×16, so draw a round rectangle on the pixel grid.
  `rx` is 0.5 less than half the width.
  E.g. a circle of 5 pixels is `<rect x="1.5" y="1.5" width="4" height="4" rx="1.5"/>`, and one of 7 pixels is `width="6" height="6" rx="2.5"`.
  A large circle (e.g. the ball of kick_off) may be a `circle`.
- Symbols of the log player (play, stop, step and so on): triangles are 10 pixels high (y 3 to 13) and 5 pixels wide (45° sides), and bars are 2 pixels wide.
  They are used side by side, so their sizes are the same.
- Fills: things with parts (a folder, a ball and so on) have an outline in `fg`.
  Simple color marks have no outline.
  Black symbols (e.g. the triangle of play) are filled with `fg` and have no outline.
- Colors: use only the colors of the light scheme (LIGHT) of `palette.txt`.
  Do not use transparency to make a color lighter; add a color to `palette.txt`.
- Icons of players or teams use the colors that mean the same in the drawing of the field.

After editing with Inkscape, check that no color outside `palette.txt` was added (a gradient, a color picked with the dropper and so on).
`generate.sh` and `test/check_icons.sh` report such colors.

## The application icon

The application icons (e.g. the window's icon) differ from the toolbar icons as follows.

- The source is `icons/app/svg/NAME.svg`, drawn on a 32×32 grid (`viewBox="0 0 32 32"`).
- `generate.sh` makes `icons/app/SIZE/NAME.png` (SIZE: 16, 32, 48, 64, 128, 256).
- If details are lost at a small size, a source for that size, `icons/app/svg/NAME.SIZE.svg` (e.g. `NAME.16.svg`, drawn on a 16×16 grid), can be added. Only that size is made from it.
- They are shown by the window manager and the desktop, so they do not switch with the color scheme (no dark version is made), and their colors are not bound by `palette.txt`.
- The program makes an icon with the images of the six sizes by `UIIcon::appIcon( "NAME" )` (`setWindowIcon( UIIcon::appIcon( "rcssmonitor" ) )` of `MainWindow`).

Today's only application icon is `rcssmonitor`:
the official RoboCup logo in the screen of a monitor with a thin frame (a black bezel and a silver rim).
There is no SVG for 16 pixels; all six sizes are made from the same SVG.

- The logo is `icons/app/robocup_logo.png`, unmodified.
  It is the logo's part (397×252 pixels with the gray box and the green band) cut out of the RoboCup Federation's image `https://www.robocup.org/system/static_contents/images/000/000/010/list/robocup_logo_ext.jpg` (750×330), saved as PNG.
  The guidelines for the logo are at `https://robocup.org/logo_guidelines` (no vector or high-resolution version is offered there).
- The SVG (`icons/app/svg/rcssmonitor.svg`) is made by `icons/app/make_rcssmonitor_svg.py` (only Python 3's standard library).
  The logo is embedded as a data URI, so the SVG is about 100 KB.
  The screen has the logo's aspect ratio, and the sizes and colors of the frame, the pole and the foot are the constants at the top of the script.
  To change the frame, change the script, run it, and make the PNGs again with `icons/generate.sh rcssmonitor`.

```
python3 icons/app/make_rcssmonitor_svg.py
icons/generate.sh rcssmonitor
```

CMake and autotools install the images of the six sizes and the SVG to the place of the freedesktop.org hicolor theme (`PREFIX/share/icons/hicolor/SIZExSIZE/apps/rcssmonitor.png` and `PREFIX/share/icons/hicolor/scalable/apps/rcssmonitor.svg`; the top `CMakeLists.txt` and `Makefile.am`). `Icon=rcssmonitor` of the desktop entry (`data/rcssmonitor.desktop.in`) refers to them.

The icon for Windows, `icons/rcss.ico` (`src/rcssmonitor.rc` refers to it; today's CMake and autotools builds do not use it), was made from the PNGs of 16, 32, 48 and 256 pixels.
To make it again, use Python's Pillow:

```
python3 -c "from PIL import Image; s = [16, 32, 48, 256]; im = {n: Image.open(f'icons/app/{n}/rcssmonitor.png') for n in s}; im[256].save('icons/rcss.ico', sizes=[(n, n) for n in s], append_images=[im[n] for n in s[:-1]])"
```

No icon for macOS (`.icns`) is made (the old one is `icons/legacy/rcss.icns`).

## palette.txt

Each line has three fields: the role, the color of the light scheme (LIGHT) and the color of the dark scheme (DARK).

```
# role      light    dark
fg          #303030  #e0e0e0
paper       #ffffff  #505050
```

- The SVG files use only the LIGHT colors.
  `generate.sh` replaces each LIGHT color with its DARK color to make the icons of the dark scheme.
- Colors are written as `#rrggbb` (lower case). Roles and LIGHT colors must not repeat.
- For a color that is the same in both schemes (e.g. the white line of the field, `field_line`), write the same color as LIGHT and DARK.
  As a LIGHT color cannot repeat another role's, the white, for example, is #f8f8f8, apart from `paper` (#ffffff).
- Choose a DARK color that has about the same contrast to a dark background (e.g. `#323232`) as its LIGHT color has to a light background (e.g. `#efefef`).
- After changing `palette.txt`, run `icons/generate.sh` (with no names) to make all the icons again.
  With names, `generate.sh` makes all of them again by itself.
- `palette.txt` and `palette.awk` are the same as soccerwindow2's.

## generate.sh

```
icons/generate.sh [NAME...]     makes the PNG files of the icons (all of them, application icons too, if no name)
icons/generate.sh --lists       updates only icons.qrc and icons.am (no Inkscape)
icons/generate.sh --help        shows the usage
```

- Inkscape 1.0 or later is needed. If necessary, give the command by the environment variable `INKSCAPE`,
  e.g. `INKSCAPE=/snap/bin/inkscape`, `INKSCAPE="flatpak run org.inkscape.Inkscape"`.
- The commands are printed before they are run, e.g.:
  ```
  + inkscape --pipe --export-type=png --export-filename=- --export-background-opacity=0 -w 16 -h 16 < svg/open.svg > light/16/open.png
  + awk -v mode=dark -f palette.awk palette.txt svg/open.svg | inkscape --pipe ... -w 16 -h 16 > dark/16/open.png
  ```
  The SVG data is given on the standard input and the PNG data is taken from the standard output, so Inkscape does not read or write files by itself
  (Inkscape installed by snap cannot read files in hidden directories, and does not use the shell's current directory).
- Before running Inkscape, the colors of the SVG files are checked with `palette.txt`.
- After making the PNG files, `icons.qrc`, `icons.am` and `generated.sha256` are updated.
  `generated.sha256` records the checksums of `palette.txt` and the SVG files from which the PNG files were made.
  `test/check_icons.sh` uses it to find SVG files edited without running `generate.sh`.
- The only difference from soccerwindow2's `generate.sh` is the sizes of the toolbar icons (16, 24, 32 and 48; soccerwindow2 has 16 and 32).

## The Qt resource system (.qrc)

The Qt resource system embeds files in the program.
The files to embed are listed in a resource file (`.qrc`); the resource compiler `rcc` turns them into a C++ source file, which is compiled and linked into the program.
The program reads them by paths starting with `:/`, as files on disk
(e.g. `QPixmap( ":/icons/light/16/open.png" )`).
Besides the icons, rcssmonitor embeds the shaders of the 3D view (`src/view3d/shaders/view3d_shaders.qrc`) and the bundled figure packs (`resources/figures/view3d_figures.qrc`) in the same way.

### Format

`icons/icons.qrc` is an XML file.

```xml
<!DOCTYPE RCC>
<RCC version="1.0">
<qresource prefix="/icons">
    <file>light/16/open.png</file>
    <file>light/24/open.png</file>
    ...
    <file>app/16/rcssmonitor.png</file>
</qresource>
</RCC>
```

- `<file>` gives a file by its path relative to the directory of the `.qrc` file.
- The `prefix` of `<qresource>` comes before the path used in the program.
  The file `icons/light/16/open.png` is read as `:/icons/light/16/open.png`.
- `rcc` compresses a file when that makes it smaller. PNG files are usually stored as they are.

`generate.sh` makes `icons/icons.qrc` from the list of the SVG files, so it need not be edited by hand.
The SVG files of the application icons are not embedded (only the PNG files).

### Build

CMake (`src/CMakeLists.txt`): the `.qrc` file is a source of the static library `rcssmonitor_core`, and `AUTORCC` runs `rcc`.
CMake knows the files listed in the `.qrc` file, and runs `rcc` again when they change.

```cmake
add_library(rcssmonitor_core STATIC
  ...
  ${PROJECT_SOURCE_DIR}/icons/icons.qrc
  )
set_target_properties(rcssmonitor_core PROPERTIES AUTORCC ON)
```

autotools (`src/Makefile.am`): `make` makes `qrc_icons.cpp` with the `rcc` that `configure` found (`QT_RCC`).

```make
ICONS_DIR = $(top_srcdir)/icons
include $(top_srcdir)/icons/icons.am

qrc_icons.cpp: $(ICONS_DIR)/icons.qrc $(ICONS_PNG_FILES)
	$(QT_RCC) --name icons --output $@ $(ICONS_DIR)/icons.qrc
```

- `qrc_icons.cpp` is in `nodist_rcssmonitor_SOURCES`.
  The top `Makefile.am` includes `icons/icons.am` too, and adds the SVG and PNG files to the tarball of `make dist` (`EXTRA_DIST`).
- `configure` (`m4/ax_have_qt.m4`) looks for `rcc` in PATH, as for `moc` (not where qmake is).
  If PATH has another Qt (e.g. anaconda), that one is used: run `configure` with the `bin` of the Qt to use at the head of PATH, or give it as `make QT_RCC=/usr/lib/qt5/bin/rcc`.

### Initialization

Resources in a static library must be registered with `Q_INIT_RESOURCE( NAME )` before they are used.
NAME is the name of the `.qrc` file without the extension (or the name given by `rcc --name`).
Otherwise the linker may drop the resources. In the CMake build, the icons are linked into the static library `rcssmonitor_core`.

`UIIcon::fromName()` and `UIIcon::appIcon()` call `UIIcon::initResources()`, which calls `Q_INIT_RESOURCE( icons )`, by themselves.
Before reading files by the paths `:/icons/...` directly (e.g. in tests), call `UIIcon::initResources()`.
`Q_INIT_RESOURCE` must be called from a function outside a namespace.

## Tests

- `test/check_icons.sh` (`check_icons` of CTest) checks the following without Inkscape.
  - `palette.txt` is correct, and the SVG files use only its LIGHT colors.
  - Each SVG file has its eight PNG files of the correct sizes, and no PNG file is without its SVG file.
  - Each application icon has its six PNG files of the correct sizes, and the SVG files for a size are of a correct size.
  - `icons.qrc` and `icons.am` list exactly these files.
  - The PNG files were made from the current SVG files and `palette.txt` (`generated.sha256`).
- `test/test_ui_icon.cpp` (`ui_icon` of CTest) checks that the icons are embedded,
  that `UIIcon::fromName()` uses the images of the current scheme, the size drawn and the device pixel ratio,
  that the application icon has the images of the six sizes and is the window's icon,
  and that the buttons of MainWindow are drawn with the images of the current scheme.
- `test/test_toolbar.cpp` checks that the buttons of the tool bar are drawn with the images of the current scheme too.
