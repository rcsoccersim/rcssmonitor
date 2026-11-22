# Icon Redesign (from XPM to PNG, for the light and the dark color schemes): Plan

| Item | Content |
|---|---|
| Created | 2026-11-22 |
| Target | rcssmonitor develop (5affd90, 20.0.0-beta.1) |
| Branch | `feature/icon-redesign` (from develop after the color scheme (F1) is merged; the user creates it on GitHub) |
| Status | Plan written (2026-11-22). Starts after the color scheme (F1) |
| Japanese version | [2026-11-22-01_icon-redesign.md](2026-11-22-01_icon-redesign.md) (same content) |
| Related | F2 of the overall plan [2026-11-22-00_v20-features_en.md](2026-11-22-00_v20-features_en.md). soccerwindow2's `docs/plan/2026-11-21_icon-redesign.md` and `docs/icons.md` |

## 1. Goal and approach

- Keep the icons as PNG instead of XPM (C arrays `#include`d in the sources). The PNGs are embedded in the executable as Qt resources (`.qrc`).
- Take the same way as soccerwindow2 (the user, 2026-11-22): draw the icons in SVG, make PNGs for the light and the dark color schemes with Inkscape at development time, and commit them. The build and the CI need no Inkscape. At run time, the color scheme is checked each time an icon is painted, and the light or the dark one is chosen.
- All the toolbar icons (the 9 XPMs and the 6 drawn by code) are redrawn in a unified design. The application icon is redrawn in SVG too, and PNGs of each size are made (the user, 2026-11-22; U4 of the overall plan).
- The detection of the scheme (`isDark(palette)`) only matters after the color scheme (F1), so this comes after F1. The sizes of the application icon are installed by the desktop (F3).

## 2. Current state (develop 5affd90)

### 2.1 Icons and how they are used

| Kind | Icons | Use |
|---|---|---|
| XPM (used) | rcss (48×48, the application icon) | `setWindowIcon( QIcon( QPixmap( rcss_xpm ) ) )` at `main_window.cpp:105` |
| XPM (used) | live, play, stop, plus, minus, decelerate, accelerate, prev_score, next_score (16×16, black on white, no transparency) | Included at `main_window.cpp:71-84`. `MainWindow::setToolIcon()` (line 1432) passes them to `toolIcon(xpm, palette)`. `togglePlayOrStopIcon()` (line 3123) swaps play and stop |
| XPM (not used) | blank, ff, go, open, quit, rec, rev, rew | Not included (rev, rew, rec are commented out) |
| Drawn by code | Open, Connect, Disconnect, KickOff, Loop, LiveDelay | `DrawnIcon` in `src/toolbar_icons.{h,cpp}`. Drawn with QPainter on a 16-unit square into pixmaps of 16, 24, 32 and 48 px. `MainWindow::setDrawnIcon()` (line 1458) |
| Drawn by code (filled colors) | The yellow and red cards | Filled in `toolbar_icons.cpp`. Independent of the scheme |

- Both the XPMs and the drawn shapes are painted in the color of `QPalette::ButtonText` by `tintIcon()` in `src/tool_icon.cpp` (black → that color, white → transparent, gray → translucent). When the palette changes, `MainWindow::changeEvent()` recolors them with `updateToolIcons()` (line 1470).
- The only PNG of the application icon is the 48×48 `icons/rcssmonitor.png`, installed by CMake (CMakeLists.txt:167).
- `icons/rcss.ico` (48×48), `icons/rcss.icns` and `src/rcssmonitor.rc` are used by no build. The autotools `EXTRA_DIST` has the XPMs and `rcss.ico`, `rcss.icns` (Makefile.am).
- The only `.qrc` files are the two of the 3D view (`src/view3d/shaders/view3d_shaders.qrc`, `resources/figures/view3d_figures.qrc`). They go into the static library `rcssmonitor_core`, so `Q_INIT_RESOURCE` is needed (`gl/shader_program.cpp:56`, `figure_pack.cpp:64`). `AUTORCC` is ON only in the 3D part (src/CMakeLists.txt:114).
- `m4/ax_have_qt.m4` of autotools does `AC_SUBST` of `QT_RCC`, but `src/Makefile.am` does not use it.
- The XPMs of team logos (`team_graphic*.cpp`, `rcss/rcg/parser*`) are data of the logs and unrelated to this plan.

### 2.2 Tests

- `test/test_tool_icon.cpp`: how `tintIcon` paints, and that the Live Mode icon follows `ButtonText`.
- `test/test_toolbar.cpp`: `icons()` (the pixels of the `DrawnIcon` shapes), `buttons()` (the button order, tooltips, that there are icons, the recoloring of Loop and Open when `ButtonText` changes), `liveDelay()`, etc.
- `cmake/DistCheck.cmake`: the installed 48×48 icon.

### 2.3 soccerwindow2's implementation (the source to port)

soccerwindow2 (`../../rctools/soccerwindow2`, develop 1df28c1): its plan `docs/plan/2026-11-21_icon-redesign.md` and the document `docs/icons.md`. Main commits: db7c68b (SVGs, `palette.txt`/`palette.awk`, `generate.sh`, the first PNGs), 6e6d971 (embedding by `.qrc`, CMake and autotools), 62e92fd and c2ae992 (the application icons and installing them in hicolor), 9dd965f and 13dc45d (removing the old mechanism, the XPMs to legacy).

| Part | Content |
|---|---|
| `icons/svg/NAME.svg` | The sources, on a 16×16 grid, using only the light colors of `palette.txt` |
| `icons/palette.txt` | The light and the dark color of each role (e.g. `fg #303030 #e0e0e0`, `paper #ffffff #505050`) |
| `icons/palette.awk` | `check` (only light colors), `dark` (replace the light colors by the dark ones), `palette` |
| `icons/generate.sh` | Makes `light/{16,32}/NAME.png`, `dark/{16,32}/NAME.png` and `app/{16,32,48,64,128,256}/NAME.png` with Inkscape (`--pipe --export-type=png -w N -h N`), and remakes `icons.qrc`, `icons.am` and `generated.sha256` |
| `icons/app/svg/NAME.svg` | The application icon (32×32 grid). `NAME.16.svg` can simplify the 16 px one |
| `src/qt/ui_icon.{h,cpp}` | `UIIcon::fromName()` (`SchemeIconEngine`: chooses light or dark by `isDark(QGuiApplication::palette())` each time it paints; on HiDPI it scales down the 32 px image; handles the change to logical sizes in Qt 6.8 and Qt5's `virtual_hook`), `UIIcon::appIcon()` |
| `test/check_icons.sh` | Checks the colors, sizes, lists and sha256 |
| `icons/legacy/xpm/` | The old XPMs. Not used by the build, `export-ignore` |

## 3. Decisions

### 3.1 The user's decisions (2026-11-22)

| # | Content |
|---|---|
| D1 | The same way as soccerwindow2 (SVG → PNGs for light and dark → `.qrc`) |
| D2 | All the toolbar icons (the 9 XPMs and the 6 drawn by code) are redrawn. The application icon is redrawn too |

### 3.2 Design decisions (Claude; the user confirms them at the start of the stages)

| # | Content | Reason |
|---|---|---|
| S1 | Port the file layout, `palette.txt`, `palette.awk`, `generate.sh` and `UIIcon` from soccerwindow2, with the same names and places | The two projects are maintained the same way |
| S2 | The cards (yellow and red) stand for the colors themselves, so they stay filled by code (checked in stage 0) | No need to change them with the scheme |
| S3 | The old XPMs are not deleted but moved by `git mv` to `icons/legacy/xpm/`. `icons/legacy/README.md` has their list and images comparing the old and new icons. `export-ignore` in `.gitattributes` keeps them out of the tarball | Same as soccerwindow2. Kept as a record |
| S4 | The `.qrc` goes into `rcssmonitor_core`, `AUTORCC` is always ON, and `Q_INIT_RESOURCE( icons )` is called | Same as the `.qrc` files of 3D |
| S5 | In autotools, add a rule that makes `qrc_icons.cpp` with `QT_RCC`, and include `icons.am` made by `generate.sh` | Same as soccerwindow2 |
| S6 | No icons are made for the unused XPMs (blank, ff, etc.) | Nothing uses them |

### 3.3 To decide at the start of the stages

- The design of the application icon: redraw the current one (the RoboCup logo in a screen), base it on the rcsoccersim logo (two players, yellow and red, and a ball; `rcsoccersim.github.io/static/img/logo.png`), or something else. In stage 0, an image of the candidates side by side is made, and the user chooses.
- `rcss.ico`, `rcss.icns` and `src/rcssmonitor.rc` (used by no build): remake them from the new icon, or move them to legacy (stage 3).
- The names of the documents: `docs/icons.md` and the English version (`docs/icons_en.md`, following the `_en` convention of this repository).

## 4. Design

### 4.1 List of the icons (the names are decided in stage 0)

| Current icon | New name (proposed) | Used for |
|---|---|---|
| live.xpm | `live_mode` | Live Mode |
| play.xpm / stop.xpm | `play`, `stop` | Switching play and stop (`togglePlayOrStopIcon`) |
| plus.xpm / minus.xpm | `step_forward`, `step_back` | Stepping |
| accelerate.xpm / decelerate.xpm | `faster`, `slower` | Speed |
| next_score.xpm / prev_score.xpm | `next_score`, `prev_score` | Jumps to the goals |
| DrawnIcon::Open | `open` | Open Game Log |
| DrawnIcon::Connect / Disconnect | `connect`, `disconnect` | Connecting and disconnecting |
| DrawnIcon::KickOff | `kick_off` | The referee's kick-off |
| DrawnIcon::Loop | `loop` | Loop playback |
| DrawnIcon::LiveDelay | `live_delay` | Delayed live |
| rcss.xpm | `rcssmonitor` (application) | The window icon, installation |

### 4.2 Code

- `src/ui_icon.{h,cpp}` (new): move soccerwindow2's `UIIcon`: `fromName( name )`, `appIcon( name )`. The detection of the scheme uses `UIPalette::isDark( palette )`, which the color scheme (F1) put in `src/ui_palette.h` (2026-11-22; the same test as soccerwindow2's `UIIcon::isDark()`). `fromNumber` (soccerwindow2's debug number icons) is not needed.
- Remove: `src/tool_icon.{h,cpp}`, `DrawnIcon` of `src/toolbar_icons.{h,cpp}` (decide where the card filling of S2 stays), and in `MainWindow`: `M_tool_icons`, `M_drawn_icons`, `setToolIcon`, `setDrawnIcon`, `updateToolIcons`, and the recoloring on palette changes in `changeEvent`.
- `togglePlayOrStopIcon` uses `UIIcon::fromName( "play" / "stop" )`.
- The window icon is `UIIcon::appIcon( "rcssmonitor" )` (all sizes).

### 4.3 Build

- CMake (`src/CMakeLists.txt`): add `${PROJECT_SOURCE_DIR}/icons/icons.qrc` to the sources of `rcssmonitor_core`, and move `AUTORCC ON` out of the 3D part.
- Autotools (`src/Makefile.am`, `Makefile.am`): `include $(top_srcdir)/icons/icons.am`, the rule `qrc_icons.cpp: $(ICONS_DIR)/icons.qrc $(ICONS_PNG_FILES)` (`$(QT_RCC) --name icons`), `nodist_rcssmonitor_SOURCES`, and the SVGs and PNGs in `EXTRA_DIST`. Remove the XPMs from `EXTRA_DIST`.
- The CMake dist (`git archive`) follows `.gitattributes`, so add `export-ignore` for `icons/legacy/`.

## 5. Stages

At each stage, the user looks at images comparing the light and the dark schemes before the commit.

| # | Work | Completion criteria |
|---|---|---|
| 0 | The design rules (line widths, color roles, margins) and samples (a few, e.g. the playback icons). Images comparing light and dark. Candidates for the application icon (§3.3) | The user decides the design rules and the application icon |
| 1 | The mechanism: `palette.txt`, `palette.awk`, `generate.sh`, `UIIcon`, `icons.qrc`, CMake and autotools. Replace the playback icons (play, stop, stepping, speed, goals, live) | Both builds and the CI pass. The user checks the screen |
| 2 | The other toolbar icons (open, connect, disconnect, kick_off, loop, live_delay) | Same as above |
| 3 | The application icon (SVG, PNGs of each size, the window icon). What to do with `rcss.ico` etc. | Same as above, looking at each size |
| 4 | Remove the old code (`tool_icon`, `DrawnIcon`, etc.), move the XPMs to `icons/legacy/xpm/`, `icons/legacy/README.md`, `export-ignore` | No XPM includes nor recoloring code remain. distcheck passes |
| 5 | Documents (`docs/icons.md`, `docs/icons_en.md`, the references from `docs/build.md`, README, NEWS) | The user checks them |

## 6. Tests

- Replace `test_tool_icon.cpp` by `test_ui_icon.cpp`: different images are chosen with a light and a dark palette, on HiDPI (dpr 2) it paints from the 32 px image, all sizes of the application icon exist, and the handling of unknown names.
- `test_toolbar.cpp`: remove `icons()` (the pixel checks of DrawnIcon), and change the recoloring checks of `buttons()` to "the dark images are chosen with a dark palette". Keep the checks of the button order and the tooltips.
- `test/check_icons.sh` (from soccerwindow2): the SVGs use only light colors, the sizes of the PNGs, the lists of `icons.qrc` and `icons.am` match the SVGs, `generated.sha256`. Registered in CTest (no Inkscape needed).
- The icon checks of `cmake/DistCheck.cmake` are changed to all sizes by the desktop (F3).

## 7. Files to change (expected)

| File | Content |
|---|---|
| `icons/svg/*.svg`, `icons/app/svg/*.svg`, `icons/palette.txt`, `icons/palette.awk`, `icons/generate.sh` (new) | The sources and how to make the PNGs |
| `icons/{light,dark}/{16,32}/*.png`, `icons/app/*/rcssmonitor.png`, `icons/icons.qrc`, `icons/icons.am`, `icons/generated.sha256` (new; generated files committed) | The embedded PNGs |
| `icons/legacy/xpm/*.xpm` (moved), `icons/legacy/README.md` (new) | The old icons |
| `src/ui_icon.{h,cpp}` (new), `src/tool_icon.{h,cpp}` and `src/toolbar_icons.{h,cpp}` (removed or reduced) | Code |
| `src/main_window.{h,cpp}` | How the icons are used |
| `src/CMakeLists.txt`, `src/Makefile.am`, `Makefile.am`, `.gitattributes` | Build and distribution |
| `test/test_ui_icon.cpp` (new), `test/test_toolbar.cpp`, `test/check_icons.sh` (new), `test/CMakeLists.txt` | Tests |
| `docs/icons.md`, `docs/icons_en.md` (new), `docs/build.md`, README, NEWS | Documents |

## 8. Out of scope

- Changing the order of the toolbars and menus.
- The colors of the field drawing.

## 9. Record

### 2026-11-22: Writing the plan

- Written together with the overall plan (2026-11-22-00_v20-features_en.md). The user's decisions D1 and D2 were given.
- Inkscape 1.4.4 (the snap package, `/snap/bin/inkscape`) is available locally.
