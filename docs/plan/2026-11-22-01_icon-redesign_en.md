# Icon Redesign (from XPM to PNG, for the light and the dark color schemes): Plan

| Item | Content |
|---|---|
| Created | 2026-11-22 |
| Target | rcssmonitor develop (5affd90, 20.0.0-beta.1) |
| Branch | `feature/icon-redesign` (from develop after the color scheme (F1) is merged; the user creates it on GitHub) |
| Status | Plan written (2026-11-22). Stage 0 (the design and the candidates for the application icon) is done. Stage 1 (the mechanism and the playback icons) is committed (26bc103). Stage 2 (the remaining toolbar icons) is committed (1650477). Stage 3 (the application icon) is committed (49100ac). Stage 4 (cleaning up the old mechanism) is committed (6783caa). Stage 5 (the documents) is committed (2cd182e). Merged into develop by PR #72 (Squash Merge) (2026-11-23) |
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
| D3 | The design rules (lines, grid, margins, color roles, `palette.txt`) are the same as soccerwindow2's. Of the 15 icons, 10 (open, live_mode, play, stop, step_back, step_forward, slower, faster, prev_score, next_score) use soccerwindow2's SVGs (`open_rcg`, `logplayer_*`), and 5 (connect, disconnect, kick_off, loop, live_delay) are drawn anew by the same rules (stage 0; §4.1) |
| D4 | PNGs of 16, 24, 32 and 48 px are made. The LIVE and play buttons are drawn at 24 px, so 24 px (1x) and 48 px (2x) are needed too. Changed from soccerwindow2 (16 and 32 only) |
| D5 | The application icon redraws the current motif (the RoboCup logo in a screen). The logo is the official one (stage 0; §4.4) |
| D6 | The 16 and 32 px images of the application icon are the same SVG scaled down (no SVG for the small sizes; stage 3) |
| D7 | `rcss.ico` is remade from the 16, 32, 48 and 256 px images of the new application icon and kept with `src/rcssmonitor.rc` (as in soccerwindow2). `rcss.icns` has no macOS build, and remaking it would enlarge the logo, so it moves to `icons/legacy/` in stage 4 (stage 3) |
| D8 | The frame of the application icon (the logo unchanged): a screen with a thin black bezel (0.8) and a thin silver rim around it (0.3; a vertical gradient and a gray outline), a thin gray pole and a dark gray round foot (5f of stage 3; the look of today's real monitors, and it does not melt into a dark background) |

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

- The design of the application icon: redraw the current one (the RoboCup logo in a screen), base it on the rcsoccersim logo (two players, yellow and red, and a ball; `rcsoccersim.github.io/static/img/logo.png`), or something else. In stage 0, an image of the candidates side by side is made, and the user chooses. → The current motif redrawn, with the official logo (D5).
- `rcss.ico`, `rcss.icns` and `src/rcssmonitor.rc` (used by no build): remake them from the new icon, or move them to legacy (stage 3). → `rcss.ico` is remade and kept with `rcssmonitor.rc`, and `rcss.icns` moves to legacy (D7).
- The names of the documents: `docs/icons.md` and the English version (`docs/icons_en.md`, following the `_en` convention of this repository). → Done as planned (stage 5). Documents for developers are in `docs/dev/` too (not in the tarball), but the scripts such as `palette.txt` refer to `docs/icons.md`, and the files of the mechanism are in the tarball, so it went to `docs/`.

## 4. Design

### 4.1 List of the icons (the names were decided in stage 0, as proposed here)

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

### 4.4 The application icon (decided in stages 0 and 3; D5, D6, D8)

- On a 32×32 grid, a screen (27 wide, the logo's aspect ratio 397×252) with a thin black bezel (0.8) and a thin silver rim (0.3) around it, a thin gray pole and a dark gray round foot are drawn, and the official RoboCup logo is put in the screen, unmodified (D8). The whole is centered vertically.
- The logo is a PNG cut out of the logo's part of robocup.org's `robocup_logo_ext.jpg` (750×330). It is embedded in the SVG as a data URI.
- The application icon does not switch with the color scheme, so its colors are not bound by `palette.txt` (as in soccerwindow2).
- How 16 and 24 px are made, and where the logo PNG and the script are kept, are decided in stage 3 (the record of stage 0 in §9). → The sizes of the application icon are 16, 32, 48, 64, 128 and 256, with no 24. 16 and 32 px are scaled down as they are (D6). The logo is `icons/app/robocup_logo.png`, and `icons/app/make_rcssmonitor_svg.py` makes the SVG `icons/app/svg/rcssmonitor.svg`.

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

### 2026-11-22: Stage 0 (the design and the candidates for the application icon)

- After F1 (PR #71) was merged into develop (6c950f4), the user created `feature/icon-redesign` on GitHub. Before stage 0, the user confirmed S1 to S6 as planned.
- Samples made (kept in the work area, not committed; they go into the repository in stages 1 and 3):
  - The 15 toolbar SVGs. 10 are copies of soccerwindow2's SVGs, and 5 were drawn anew (connect and disconnect as a plug and a socket, kick_off as a ball, loop as arrows in a loop, live_delay as a stopwatch; each redraws the web version's shape by soccerwindow2's rules). The colors were checked and the dark versions made with soccerwindow2's `palette.awk`, and Inkscape made 16, 24 and 32 px.
  - Images comparing the current toolbar and the proposed one (1x and 2x, light and dark), and the 16 px icons magnified 6 times.
  - The three candidates for the application icon (A: the current motif redrawn, B: the rcsoccersim logo, C: a screen of the 2D view) at 16 to 128 px, on light and dark backgrounds.
- Decided by the user: D3 (the same design rules as soccerwindow2), D4 (16, 24, 32 and 48 px), D5 (the application icon is candidate A, with the official RoboCup logo).
- The official logo (§4.4):
  - The image of robocup.org that the user pointed to, `robocup_logo_ext.jpg` (750×330, JPG), is used. The logo guidelines page (`https://robocup.org/logo_guidelines`) was checked on 2026-11-22; it offers no vector or high-resolution version (only a 229×99 PNG and so on).
  - The logo's part (397×252 px with the gray box and the green band, from (175, 38) of the JPG) was cut out unmodified, saved as a PNG, and embedded in the SVG. The screen has the logo's aspect ratio. Even at 256 px the screen is about 200 px wide, so the logo is not enlarged.
  - By the guidelines page, the RoboCup logo may be used by RoboCup Partners and participating teams, for academic purposes and PR (use in the media needs consulting the RoboCup office). rcssmonitor is an official tool of the RoboCup soccer simulation, and the current icon uses this logo too. The user decided to use the official logo, and Claude told the user what the guidelines say.
- To decide in stage 3: the logo cannot be read at 16 and 24 px (make simplified versions?). Where to keep the logo PNG and the script that makes the SVG embedding it (after soccerwindow2's `make_soccerwindow2_svg.py`). The SVG installed as hicolor's scalable icon by the desktop (F3) is the one embedding the PNG (about 100 KB).

### 2026-11-22: Stage 1 (the mechanism and the playback icons)

- Stage 1 started at the user's instruction.
- Made:
  - `icons/palette.txt`, `icons/palette.awk`: soccerwindow2's, as they are (D3).
  - `icons/generate.sh`: moved from soccerwindow2, with the sizes 16, 24, 32 and 48 (D4). It makes the directories of the sizes if missing. In the comment of `icons.am`, `src/qt/Makefile.am` became `src/Makefile.am`.
  - The 9 in `icons/svg/` (live_mode, play, stop, step_back, step_forward, slower, faster, prev_score, next_score): copies of soccerwindow2's `logplayer_*.svg`, with the original file's name added to the comment in each. `generate.sh` made `icons/{light,dark}/{16,24,32,48}/*.png` (72 files), `icons.qrc`, `icons.am` and `generated.sha256` (Inkscape 1.4.4).
  - `src/ui_icon.{h,cpp}` (new): soccerwindow2's `UIIcon::fromName()` and `SchemeIconEngine`. The scheme is detected by `UIPalette::isDark()`, and the sizes are `UIIcon::SIZES` (16, 24, 32, 48). The application icon (`appIcon`) is added in stage 3. The number icons (`fromNumber`) are not moved.
  - `MainWindow`: the icons of the 9 playback actions are now `UIIcon::fromName()`, and so is `togglePlayOrStopIcon`. The `#include`s of the 9 XPMs no longer used were removed (the XPM files move to legacy in stage 4; the old mechanism such as `setToolIcon` is removed in stage 4, after the remaining icons in stage 2).
  - CMake: `icons/icons.qrc` is in `rcssmonitor_core`, and `AUTORCC` moved out of the 3D part to be always ON (S4).
  - autotools: `src/Makefile.am` includes `icons.am` and has the rule of `qrc_icons.cpp` (`$(QT_RCC) --name icons`), and the top `Makefile.am` has the icon mechanism, the SVGs and the PNGs in `EXTRA_DIST` (S5).
  - Tests: `test/check_icons.sh` (moved from soccerwindow2, with the four sizes) is registered in CTest (no Inkscape needed). `test/test_ui_icon.cpp` (new): the 9 icons and the images of each size, choosing the images by the scheme (the same icon changes when the application's palette changes), the drawn size (24 from the 24 image, never larger than 48), the device pixel ratio (at 2, 16 from the 32 image and 24 from the 48 image; at 1.5, 16 from the 24 image), the disabled look, an unknown name, and the playback buttons of MainWindow (including the switch between Play and Stop). `test/test_tool_icon.cpp`: the check of recoloring Live Mode (`followPalette`) was removed (moved to `test_ui_icon`; the check of `tintIcon` stays until stage 4).
- Found: autotools' `m4/ax_have_qt.m4` looks for `moc` and `rcc` in PATH (not where qmake is). If PATH has another Qt (e.g. anaconda), that one is used. This is the old behavior (`moc` too) and is not fixed in this stage. Without it in PATH, `/usr/lib/qt5/bin/rcc` is used.
- Checked (Ubuntu 24.04):
  - The CMake builds with Qt 6.4.2 and Qt 5.15.13 had no warnings, and the 59 tests of `ctest` (including the new `ui_icon` and `check_icons`) passed.
  - autotools (Qt5): on a copy of the working tree, `./bootstrap`, `./configure` and `make` passed. The system's `rcc` (`/usr/lib/qt5/bin/rcc`) made `qrc_icons.cpp`, and it started with `--help`.
  - Offscreen, the tool bar was written to images at 1x and 2x in the light and dark schemes and looked at.
- The user checked it on the screen (Qt6 and Qt5).
- Next: the commit and push, the CI, and `./cmake-helper.sh distcheck qt6`.
- Commit 26bc103 of stage 1 was pushed. All five jobs of the CI passed. Locally, `./cmake-helper.sh distcheck qt6` passed under Xvfb too (the tarball has the SVGs and PNGs of the icons, and the 59 tests passed).

### 2026-11-23: Stage 2 (the remaining toolbar icons)

- Stage 2 started at the user's instruction.
- Made:
  - 6 were added to `icons/svg/`: open (a copy of soccerwindow2's `open_rcg.svg`), connect, disconnect, kick_off, loop and live_delay (drawn in stage 0; loop is the shape of soccerwindow2's `refresh.svg` in `fg`). The XML declaration was added to the 5, as soccerwindow2's SVGs have. `generate.sh` made the PNGs of these 6 and updated the lists and the checksums (15 icons and 120 PNGs in all).
  - `MainWindow`: the icons of the 7 actions with the 6 icons drawn in code (Open Game Log, Connect, Connect to, Disconnect, KickOff, Loop Playback, Live Delay) are now `UIIcon::fromName()`. Nothing calls `setToolIcon` and `setDrawnIcon` any more (the mechanism is removed in stage 4).
  - The cards (yellow and red) are drawn by `card_icon()` in `referee_menu.cpp`, apart from `toolbar_icons`. They stay as they are (S2). In stage 4, `toolbar_icons` can be removed whole.
  - Tests: in `buttons()` of `test_toolbar.cpp`, the check of "recolored with the palette's text color" was replaced by "the images of the application palette's scheme are used" (5 buttons, dark and light schemes). `icons()` (the check of the shapes of `DrawnIcon`) stays until stage 4. `test_ui_icon.cpp` checks the 15 icons and the 15 actions of MainWindow.
- Checked (Ubuntu 24.04):
  - The CMake builds with Qt 6.4.2 and Qt 5.15.13 had no warnings, and the 59 tests of `ctest` passed. The icon tests passed three times each.
  - autotools (Qt5): on a copy of the working tree, `./bootstrap`, `./configure` and `make` passed, and it started with `--help`.
  - Offscreen, the tool bar was written to images at 1x and 2x in the light and dark schemes and looked at (all 15 are the new icons).
- The user checked it on the screen (Qt6 and Qt5).
- Next: the commit and push, the CI, and `./cmake-helper.sh distcheck qt6`.
- Commit 1650477 of stage 2 was pushed. All five jobs of the CI passed, and locally `./cmake-helper.sh distcheck qt6` (under Xvfb) passed too (59 tests).

### 2026-11-23: Stage 3 (the application icon)

- Before stage 3, the user decided: D6 (16 and 32 px are scaled down as they are) and D7 (`rcss.ico` is remade, and `rcss.icns` goes to legacy). The place of the logo and the script is Claude's proposal (`icons/app/`).
  - For comparison, a version for 16 and 32 px with a thin frame and a larger screen was made too, but the letters cannot be read at 16 px in either, and the thin frame melted into a dark background.
- Made:
  - `icons/app/robocup_logo.png`: the PNG cut out unmodified of the logo's part (397×252) of robocup.org's `robocup_logo_ext.jpg` (stage 0). It was checked to be the same, pixel by pixel, as the cut-out of the JPG.
  - `icons/app/make_rcssmonitor_svg.py` (new; only Python 3's standard library): it reads the logo's size from the PNG's IHDR, draws the screen, the frame and the stand on the 32×32 grid, and makes `icons/app/svg/rcssmonitor.svg` (about 100 KB) with the logo embedded as a data URI. It first made candidate A of stage 0 (a dark gray flat display), centered vertically, and the frame was later changed to D8 ("choosing the frame again" below). The logo's source and how it was cut out are written at the top of the script.
  - `icons/generate.sh rcssmonitor` made `icons/app/{16,32,48,64,128,256}/rcssmonitor.png` and updated the lists and the checksums (`generate.sh` and `check_icons.sh` handle application icons already).
  - `UIIcon::appIcon()` (the same as soccerwindow2's) and `UIIcon::APP_SIZES`. The window icon of `MainWindow` is now `UIIcon::appIcon( "rcssmonitor" )`, and the `#include` of `rcss.xpm` was removed (the XPM goes to legacy in stage 4).
  - The 48 px icon that CMake installs (`hicolor/48x48/apps/rcssmonitor.png`) is now `icons/app/48/rcssmonitor.png`. The other sizes and the scalable one are installed by the desktop (F3). The old `icons/rcssmonitor.png` goes to legacy in stage 4.
  - `icons/rcss.ico`: remade with Pillow from `icons/app/{16,32,48,256}/rcssmonitor.png` (D7; all four are the same as the PNGs, pixel by pixel). How to make it goes into the document (stage 5).
  - autotools: the logo PNG and the script were added to `EXTRA_DIST` of the top `Makefile.am` (the SVG and the PNGs are in the lists of `icons.am`).
  - Tests: `appIcon()` was added to `test_ui_icon.cpp` (the images of the six sizes, unchanged by the color scheme, an unknown name, and the window icon of MainWindow).
- Checked (Ubuntu 24.04):
  - The CMake builds with Qt 6.4.2 and Qt 5.15.13 had no warnings, and the 59 tests of `ctest` passed. `check_icons.sh` passed with 15 icons and 1 application icon.
  - Installed by CMake to a temporary place, the 48 px icon was the same as `icons/app/48/rcssmonitor.png`.
  - autotools (Qt5): on a copy of the working tree, `./bootstrap`, `./configure`, `make` and `make dist` passed, and the tarball has `icons/app/`.
- To note: in GNOME on Wayland, the desktop entry's `Icon=` (the installed icon) is used instead of the window icon, and without `setDesktopFileName` they may not be linked now. The desktop (F3) deals with it.
- Choosing the frame again (the user's request; the motif, the logo in a screen, unchanged):
  - Six candidates with other frames (1: the current one, 2: a silver all-in-one, 3: a CRT, 4: a laptop, 5: a thin black border with a thin pole and a round foot, 6: a big screen of a stadium) were put side by side at 16 to 256 px on light and dark backgrounds. The user chose 5 (it has the look of today's real monitors best).
  - 5's border melted into a dark background, so three candidates combining the thin bezel with 2's silver were made (5a: a thin silver rim outside the black bezel; 5b: a silver rim and only a black line; 5c: 5a with a silver chin). The user decided on the screen of 5a with the pole and the foot of 5 (5d).
  - Candidates with a thinner silver rim (0.7 → 0.45 and 0.3) were made, and the user decided on 0.3 (5f; D8).
  - `make_rcssmonitor_svg.py` was rewritten with 5f, and the PNGs and `rcss.ico` were made again. The 256 px PNG was checked to be the same, pixel by pixel, as the 5f the user saw. The 59 tests of `ctest` passed with Qt6 and Qt5.
- The user checked it on the screen.
- Next: the commit and push, the CI, and `./cmake-helper.sh distcheck qt6`.
- Commit 49100ac of stage 3 was pushed. All five jobs of the CI passed, and locally `./cmake-helper.sh distcheck qt6` (under Xvfb) passed too (59 tests, including the check of the installation).

### 2026-11-23: Stage 4 (cleaning up the old mechanism)

- Stage 4 started at the user's instruction.
- The record of the old icons (`icons/legacy/`; S3; after soccerwindow2):
  - By `git mv`, the 18 XPMs (`icons/*.xpm`) moved to `icons/legacy/xpm/`, and `icons/rcssmonitor.png` and `icons/rcss.icns` to `icons/legacy/` (D7). The `rcss.ico` (48×48) before it was remade in stage 3 was also put in `icons/legacy/rcss.ico` from the Git history.
  - Before the code was removed, the old icons were written to images (the XPMs as they are, as drawn in the tool bar (recolored black by `tintIcon`), and the 6 drawn in code (black, by `drawnIconImage`)): `icons/legacy/old-icons.png` (the list) and `icons/legacy/comparison.png` (the old one, and the new one in the light and dark schemes).
  - `icons/legacy/README.md` and `README_en.md` (the `_en` convention of this repository; soccerwindow2 has `README.en.md`): the list of the files, the old icons and the new ones, the 8 not used at the time of the replacement (blank, ff, go, open, quit, rec, rev, rew), and the two images.
  - `/icons/legacy export-ignore` was added to `.gitattributes`. The XPMs and `rcss.icns` were removed from `EXTRA_DIST` of the top `Makefile.am` (`rcss.ico` stays).
- The old code removed:
  - `src/tool_icon.{h,cpp}` (`tintIcon`, `toolIcon`) and `src/toolbar_icons.{h,cpp}` (`DrawnIcon`, `drawnIcon`, `drawnIconImage`) were removed (from the source lists of CMake and autotools too). The cards are `card_icon()` of `referee_menu.cpp`, so they stay (S2).
  - `MainWindow`: `M_tool_icons`, `M_drawn_icons`, `setToolIcon`, `setDrawnIcon`, `updateToolIcons`, and the recoloring in `changeEvent` were removed (marking the wrong fields again stays). The includes of `<utility>` and `<vector>`, no longer used, were removed too.
  - Tests: `test/test_tool_icon.cpp` (the check of `tintIcon`) was removed, and so was `icons()` of `test_toolbar.cpp` (the check of the shapes drawn in code).
- Checked (Ubuntu 24.04):
  - The CMake builds with Qt 6.4.2 and Qt 5.15.13 had no warnings, and the 58 tests of `ctest` passed (`tool_icon` is gone).
  - The sources have no `#include` of XPM files and no recoloring code (the remaining `xpm` is of the teams' logos in the logs, unrelated to this plan).
  - `git archive` made with a temporary index has no `icons/legacy/` (171 entries of icons).
  - autotools (Qt5): on a copy of the working tree, `./bootstrap`, `./configure`, `make` and `make dist` passed, and the tarball has no legacy and no XPMs.
- The look on the screen is the same as in stage 3 (the old mechanism was no longer called after stage 2).
- Next: the commit and push, the CI, and `./cmake-helper.sh distcheck qt6`.
- Commit 6783caa of stage 4 was pushed. All five jobs of the CI passed, and locally `./cmake-helper.sh distcheck qt6` (under Xvfb) passed too (58 tests; no `icons/legacy/` and no XPM in the tarball).

### 2026-11-23: Stage 5 (the documents)

- Stage 5 started at the user's instruction.
- Written:
  - `docs/icons.md` (Japanese) and `docs/icons_en.md` (English; new): based on soccerwindow2's `docs/icons.md`, adapted to rcssmonitor. The overview (the eight PNGs, why 24 and 48, `UIIcon::fromName()`, the 10 shared with soccerwindow2), the list of the files, how to add, change and remove an icon, the drawing rules (the same as soccerwindow2's), the application icon (the source of the official logo and its guidelines, `make_rcssmonitor_svg.py`, the installation, how to remake `rcss.ico`, no `.icns`), `palette.txt`, `generate.sh` (the only difference from soccerwindow2's is the sizes), the Qt resource system (the format, the CMake and autotools builds, `ax_have_qt.m4` looking for `rcc` in PATH and what to do about it, `Q_INIT_RESOURCE`), and the tests.
  - The Notes of `docs/build.md`: the icons are PNGs made by `icons/generate.sh`, and Inkscape is needed only to change an icon (not to build), with a reference to `docs/icons_en.md`.
  - The color scheme section of the README: the toolbar icons switch with the scheme, with a reference to `docs/icons_en.md`.
  - NEWS (the entry of 20.0.0-beta.1) and ChangeLog: the renewed icons.
  - autotools: `docs/icons.md` and `docs/icons_en.md` were added to `EXTRA_DIST` of the top `Makefile.am` (distributed with the files of the icon mechanism).
- Checked: with autotools (Qt5), `make` and `make dist` passed on a copy of the working tree, and the tarball has the two documents.
- The user checked the documents.
- Next: the commit and push, the CI, and `./cmake-helper.sh distcheck qt6`. After that, the proposal of the PR.

### 2026-11-23: Merging into develop (PR #72)

- Commit 2cd182e of stage 5 was pushed. All five jobs of the CI passed, and locally `./cmake-helper.sh distcheck qt6` (under Xvfb) passed too (58 tests; the tarball has `docs/icons.md` and `docs/icons_en.md`).
- PR: the user opened PR #72 from `feature/icon-redesign` to develop ("Renew the icons: SVG-based icons for the light and dark schemes, and the application icon with the official RoboCup logo"), with the title and description Claude proposed. The branch has the six commits of stages 0 to 5 (3011609, 26bc103, 1650477, 49100ac, 6783caa, 2cd182e) and this record.
- It is merged by a Squash Merge (one feature, renewing the icons, whose history by stage is in this plan; §3 of the overall plan). Last time (PR #71) the commit's title became GitHub's default "Merge pull request ...", so the user was told to change it to the PR's title when merging.
- This record was committed to the branch of PR #72 before the merge (CLAUDE.md). To stay true after the merge, it does not give the commit of the merge.
- Next: when PR #72 is merged, the desktop (F3; §B of the overall plan) follows. The user creates `feature/desktop-entry` on GitHub from develop. Installing all the sizes of the application icon (`icons/app/`) and the scalable SVG, and `setDesktopFileName` (linking to the Wayland dash) are done in F3.

### 2026-11-23: Correction (the Qt5 builds)

- "The CMake build with Qt 5.15.13" in the records of this plan was actually Qt 5.15.2 of an anaconda environment (the PATH of the work environment has anaconda at its head, and CMake found Qt5 there). The autotools checks and the Qt5 job of the CI used the system's Qt5. See the entry "stage 1 of §C the referee's whistle" of 2026-11-23 in §6 of the overall plan [2026-11-22-00_v20-features_en.md](2026-11-22-00_v20-features_en.md).
