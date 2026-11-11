# rcssmonitor 3D view and Qt6 support: development plan

| Item | Content |
|---|---|
| Created | 2026-11-11 |
| Target | rcssmonitor. The work is based on `develop` (01f880a, 19.0.1). It does not include the TCP / WebSocket support (`feature/support-tcp-ws`) (D7) |
| Reference implementation | rcssmonitor-web 20.0.0-beta.2 (tag `v20.0.0-beta.2`, ed36941). Its working tree is `../rcssmonitor-web`, next to this repository |
| Server referred to | The asio version of rcssserver 19.x. Its working tree is `../rcssserver` (branch `feature/asio-migration`), next to this repository |
| Japanese version | [PLAN_3D-VIEW.md](PLAN_3D-VIEW.md) (same content; §0.1) |
| Status | Phase 0 (started 2026-11-11) |

## 0. How to use this plan

This plan is loaded to drive the development of rcssmonitor. The design and history of the 3D view built for the web version (rcssmonitor-web) are in the following documents of the web version (written in Japanese). Before building a component, read the corresponding document and source.

| Document | Content |
|---|---|
| `rcssmonitor-web/docs/DEVELOPMENT_PLAN.md` §5 "Phase 6: 3D view" | Specification and design of the 3D view (cameras, player view, overlays, figures, shadows, scenery, ball ring and trail, etc.) |
| Same, §9 "Phase 6", steps 1–16 | Record in the order things were built: user decisions, bugs found and their causes, measurements |
| `rcssmonitor-web/docs/parity.md` | Feature correspondence with rcssmonitor. §7 lists rcssmonitor bugs, §8 the key compatibility table |
| `rcssmonitor-web/docs/protocol.md` | Notes on the monitor protocol and the rcg formats (JSON fields, state bits, play modes) |
| `rcssmonitor-web/src/view/render3d/` and others | Implementation of the 3D view (mapping in §4.2) |
| `rcssmonitor-web/test/unit/`, `test/e2e/view3d.spec.ts` | Unit tests (models for porting) and E2E tests |

### 0.1 Working agreements (with the user)

- Report in Japanese. When reporting an implementation, attach a proposed commit message.
- Claude does the commits and pushes. Before that, show the user the proposed commit message (and the files to include) and get it confirmed (from 2026-11-11; until then the user did them). Write commit messages in English and end them with the designated `Co-Authored-By` line.
- Merges suited to a PR (such as merging into master) are done by the user.
- When running the program to try it, do not modify the user's settings file `~/.rcssmonitor.conf`. Start it with `HOME` set to a temporary directory (e.g. `HOME=$(mktemp -d) ./src/rcssmonitor`).
- When running rcssserver, set `RCSS_CONF_DIR` to a temporary directory (do not touch `~/.rcssserver`).
- Do not run too many GPU tests in parallel (during the web version work, a GPU page fault brought the work machine down).
- At each step, add a record to §14 (decisions, bugs found, measurements, tests).
- Keep the Japanese version (`PLAN_3D-VIEW.md`) and the English version (this document) of the plan in parallel. When one is changed, change the other to the same content in the same step.
- The user creates branches on GitHub; locally, fetch them and switch to them (§9.1).

## 1. Goals and scope

1. **3D view**: implement in rcssmonitor the same features as the 3D view of rcssmonitor-web (§4, §5).
2. **Bug fixes**: fix the 14 rcssmonitor bugs found while porting it to the web (§6).
3. **JSON parser improvements**: read JSON logs and live messages as robustly as the web version does (§7).
4. **Qt6 support**: build with Qt6 and replace deprecated APIs (§8).
5. Along with these, set up CI (GitHub Actions) and tests (§10).

Out of scope: §13.

## 2. Decisions, and what to decide at the start

### 2.1 Already decided (user)

- The 3D view will be equivalent to the 3D view of rcssmonitor-web, including the cameras, the player view, overlays, figures, shadows, scenery, the ball ring and trail, settings, and keys.
- The bug fixes, the JSON parser improvements, and the Qt6 support are also part of this plan.
- The plan is written in Japanese, with an English version kept in parallel (§0.1).
- The work is done on branches derived from `develop`. The TCP / WebSocket support is not merged for now, because whether to adopt it officially has not been decided (D7, 2026-11-11).
- Phases 0–3 (up to the Qt6 support) are merged into develop as each one is done. The 3D view (Phases 4–8) is merged into develop only after it is complete and its official adoption is decided (§9.1, 2026-11-11).
- The qmake build is dropped (D11, 2026-11-11).

### 2.2 To decide at the start

Confirm with the user in Phase 0 before proceeding. The recommendations are assumed by the following sections of this plan.

State on 2026-11-11: the user decided D7, D10, and D11 ("Decided" in the table; D11 is a problem found in Phase 0). D1 is decided by the result of the Phase 0 prototype. D2–D6, D8, and D9 proceed with the recommendations.

| # | Item | Recommendation | Alternatives | Reason |
|---|---|---|---|---|
| D1 | 3D rendering technology | `QOpenGLWidget` with our own rendering (OpenGL 3.3 core, our own shaders) | Qt Quick 3D (embedded with `QQuickWidget`) | §2.3 below |
| D2 | Supported Qt versions | Both Qt 5.15 and Qt 6.2 or later (one source, switched) | Qt6 only | Ubuntu 22.04 has both Qt 5.15 and Qt 6.2. After a period of supporting both, Qt5 can be dropped in the next version |
| D3 | Build systems | CMake is the main one. Qt6 is supported by CMake only; autotools stays on Qt5 (3D is built by CMake only). qmake (`.pro`) is dropped by D11 | Make all three support Qt6 / drop autotools and qmake | Maintaining three is a lot of work. autotools' `AX_HAVE_QT` finds Qt through qmake, which makes Qt6 support hard |
| D4 | Is 3D optional in the build | CMake option `ENABLE_3D` (default ON). Where OpenGL 3.3 is unavailable, 3D is disabled at run time and the program works in 2D only | Make it mandatory | Keep 2D monitoring usable as before on old GPUs and virtual machines |
| D5 | glTF loading | cgltf (single header, MIT) plus our own animation evaluation | tinygltf (MIT), Assimp (BSD, large) | As in §5.8, the pose is computed from the log every time, so no animation player is needed; reading the keyframes is enough |
| D6 | Figure model | Re-export the web version's `Mannequin.glb` (CC0) without compression and put it in the Qt resources (qrc) | Bundle the meshoptimizer decoder (MIT) and read the web version's file as is | The web version's file requires `EXT_meshopt_compression` and `KHR_mesh_quantization`. For a desktop app, fewer dependencies matter more than size |
| D7 | Base of the work | **Decided**: derive from `develop`. Leave `feature/support-tcp-ws` unmerged, and merge it once it is adopted (§3.5) | (Original recommendation) merge `feature/support-tcp-ws` first and branch from there / branch directly from `feature/support-tcp-ws` | Whether to adopt the TCP / WebSocket support officially has not been decided (user). The 3D view diff can also be reviewed separately |
| D8 | Version | 20.0.0 (aligned with asio rcssserver 20 and rcssmonitor-web 20) | 19.1.0 etc. | The Qt6 support and the 3D view are big changes |
| D9 | Branch policy | Same as the web version (master: releases, develop: integration, feature/*: work, hotfix/*). Merges create merge commits. Per-phase branches in §9.1 | Follow rcssmonitor's current practice | The web version's `docs/release.md` |
| D10 | master's AppImage workflow (§3.2) | **Decided direction**: the current workflow is a maintenance burden, so disable it. Replace it with an easier-to-maintain method if one can be established (user). Recommended replacement in §2.4 | Keep it as is (the code would have to stay buildable with Qt 5.12, which conflicts with D2) | It requires maintaining an Ubuntu 20.04 (end of support) Docker image, `qt5-default`, and custom shell and Python scripts. It builds with autotools, so 3D would not be included (D3) |
| D11 | The qmake build (§3.2) | **Decided**: drop it. Delete `rcssmonitor.pro`, `src/src.pro`, and `src/rcsslogplayer/`, an old copy of a parser that only the qmake build used (`feature/ci`). Mention it in NEWS (Phase 9). autotools uses qmake to find Qt, so the `qt5-qmake` package is still needed | Fix it (one definition, the version, boost) and add it to CI | No machine will use qmake any more (user). It was already broken on develop, and nobody had noticed. CMake and autotools are enough |

### 2.3 Comparison for D1 (3D rendering technology)

| Option | Pros | Cons |
|---|---|---|
| **QOpenGLWidget with our own rendering** (recommended) | The same code works on Qt5 and Qt6. No QML is brought in (rcssmonitor is a QWidget-only app). Everything done in the web version (instanced drawing, shadow maps, custom shaders, deciding skeleton poses from the log every time, the upper limit on the jersey number size, managing state per draw) stays under our control. The only dependencies are Qt's OpenGL module and cgltf | A lot of rendering infrastructure to write (about 10 shaders, shadow map, skinned meshes, lines, billboards). OpenGL is deprecated on macOS (works up to 4.1 core) |
| Qt Quick 3D | Materials, shadows, fog, skinned meshes, instancing, and picking are all there | Requires Qt6 (skeletal animation and instancing are Qt 6 features). The scene is written in QML, and C++ passes data to it. For skinned glTF, the documented path is build-time conversion (Balsam); whether run-time loading (RuntimeLoader) handles skeletal animation has not been confirmed. Whether moving the bones ourselves every frame (§5.8) works naturally also needs testing |
| Qt 3D | Has a C++ API | Deprecated in Qt 6.8 and removed from the standard release. Not for new use |

Decide after confirming with the recommended option, in the Phase 0 prototype (§9), "22 figures with shadows at 1920×1080 at 60 fps".

### 2.4 Replacement options for D10 (AppImage)

| Option | Content | What to maintain |
|---|---|---|
| **A. Build with CMake and linuxdeploy in a tag-triggered `release.yml`** (recommended) | Same shape as the web version's `release.yml`. Runs only when a tag (`rcssmonitor-X.Y.Z`) is pushed or when run by hand. Builds with CMake in `container: ubuntu:22.04` (Qt 5.15.3 / Qt 6.2.4, glibc 2.35) on the runner, installs into `DESTDIR`, bundles an AppImage with linuxdeploy and linuxdeploy-plugin-qt, and attaches it to the release with `gh release`. The desktop file and the icon are installed by CMake's `install()` | One workflow (about 50 lines). Raise the base version when an Ubuntu LTS ends (22.04 → 24.04). Raise the linuxdeploy version (pinned) |
| B. Drop the AppImage | Delete the workflow and `utils/appimage/`. Distribution is the source and the per-distribution build instructions (README) only | Nothing |

- With either option, disable the current `main.yml`. It runs on pushes to any branch and tag, and creates a release when there is none for the version in `CMakeLists.txt`. So merely raising the version on a work branch creates a release. Disabling it in the GitHub UI (Actions → ci → Disable workflow) stops it without a commit, and it can be undone. Delete the files when merging 20.0.0 into master (or replace them, with option A).
- Try option A while building the CI in Phase 0. A manually triggered workflow (`workflow_dispatch`) cannot be run unless the same file is on the default branch (master), so during the trial it runs on pushes to `feature/ci`; when adopted, it runs on tag pushes and by hand. Decide on adoption after confirming that the resulting AppImage starts on other environments (Ubuntu 22.04, 24.04, 26.04). Whether an AppImage with 3D works with the user's OpenGL (linuxdeploy does not bundle libGL) is checked in Phase 4 or later.
- The base is 22.04 because an AppImage does not run on a glibc older than the one it was built with. The Qt of 22.04 (5.15.3, 6.2.4) equals the lower bounds of D2, so this also checks that the code builds with the lowest versions.
- The current workflow also uses linuxdeploy (the unpinned `continuous` release). What option A changes: CMake instead of autotools, `container: ubuntu:22.04` instead of a custom Dockerfile, running on tags only instead of on every push, and a pinned linuxdeploy version.
- master's `utils/appimage/rcssmonitor.png` is actually a 512×512 JPEG (the RoboCup logo), so its extension is wrong. The trial uses `icons/rcssmonitor.png`, a PNG made from the app's window icon `icons/rcss.xpm` (48×48).

## 3. Current rcssmonitor

### 3.1 Structure

| File | Role |
|---|---|
| `src/main_window.cpp` | Menus, toolbar, keys (`createActions*`), referee operations. `createFieldCanvas()` makes `FieldCanvas` the central widget |
| `src/field_canvas.cpp` | 2D drawing target. A `QWidget` (a `QGLWidget` if `USE_GLWIDGET` is defined). Calls the painters in order. Mouse operations, right-click menu |
| `src/field_painter.cpp`, `player_painter.cpp`, `ball_painter.cpp`, `draw_info_painter.cpp` | 2D drawing. Calculations such as player state colors, catch / tackle / kick areas, and the offside line are also here |
| `src/disp_holder.cpp` | Holds the received frames, for both live and logs |
| `src/log_player.cpp` | Advances one frame at a time with a `QTimer`. No interpolation between frames |
| `src/monitor_client.cpp` | Live connection. UDP only on develop (TCP / WebSocket are in `*_monitor_transport.cpp` on `feature/support-tcp-ws`) |
| `src/options.cpp` | Settings, saved to `~/.rcssmonitor.conf` with `QSettings`. The command line options are here too |
| `src/config_dialog.cpp` | Settings dialog (Show, Canvas, Color, Font tabs) |
| `src/player_type_dialog.cpp` | Player type list |
| `rcss/rcg/` | Parsing of logs and messages. `parser_simdjson.cpp` (JSON, uses the bundled simdjson), `parser_v1–v4.cpp`, `types.cpp` (parameter defaults), `util.cpp` (play mode names, etc.). `nlohmann/json.hpp` is also bundled |
| `test/` | Not on develop (no unit tests). `feature/support-tcp-ws` has the Qt Test `test_message_splitter.cpp`. This plan added `test_rcg_parser.cpp` and fixtures (`test/data/`) on `feature/ci` (§14) |

### 3.2 Build

- There are three: CMake (`CMakeLists.txt`, `src/CMakeLists.txt`), autotools (`configure.ac`, `m4/ax_have_qt.m4`), and qmake (`rcssmonitor.pro`). qmake was dropped by D11 (`feature/ci`).
- All are hard-wired to Qt5 (`find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)`). `feature/support-tcp-ws` optionally looks for WebSockets and Test.
- C++17, CMake 3.5 or later.
- The qmake build fails on develop (checked on 2026-11-11). `src/src.pro` lacks the `HAVE_ARPA_INET_H` definition, so it stops at `ntohs` in `disp_holder.cpp`. Its version is still 16.0.0, and it links the unused boost. Dropped by D11. The CMake and autotools builds pass.
- develop has no CI (no `.github/workflows`).
- Only master has the AppImage workflow of PR #59 (`.github/workflows/main.yml`, named `ci`) and `utils/appimage/`. It runs on pushes to any branch and tag, builds with autotools in an Ubuntu 20.04 Docker image (Qt 5.12.8 from `qt5-default`), and creates a release when there is none for the version in `CMakeLists.txt`. It does not check the build or run tests. Handled by D10.

### 3.3 Places that change for Qt6 (counted on 2026-11-11 in `src/` and `rcss/rcg/`, excluding moc output and bundled libraries. Recounting on develop gives the same numbers. `feature/support-tcp-ws` has one more `QT_VERSION_CHECK`)

| Content | Count | How to write it for Qt6 |
|---|---|---|
| `QKeySequence` built with `Qt::CTRL + Qt::Key_X` / `Qt::ALT + …` | 19 + 5 | `Qt::CTRL \| Qt::Key_X` (`+` is deprecated in Qt 6; `\|` also works in Qt 5.15) |
| `QLayout::setMargin()` | 34 | `setContentsMargins()` (removed in Qt6) |
| `QMouseEvent::pos()`, `globalPos()` | 13, 1 | In Qt6, `position().toPoint()`, `globalPosition().toPoint()` (`pos()` is deprecated in Qt 6). `position()` does not exist in Qt 5.15, so for dual support a small function switches with `QT_VERSION_CHECK` |
| `QWheelEvent::delta()`, `orientation()` | 2, 1 | `angleDelta()` (removed in Qt6) |
| `QSignalMapper` | 2 | Lambda connections |
| `QFontMetrics::width()` | A few (some already switch to `horizontalAdvance` with `QT_VERSION_CHECK`) | `horizontalAdvance()` |
| `QTime::start()` / `elapsed()` | 1 | `QElapsedTimer` |
| `QGLWidget` (with `USE_GLWIDGET`) | 3 | `QOpenGLWidget` (`Qt6::OpenGLWidgets` in Qt6). If nobody uses this define for 2D, remove the define altogether |
| `QT_VERSION_CHECK` branches | 22 | Remove the branches for versions older than Qt 5.15 |
| `QAction` (93 places) | — | Moved to QtGui in Qt6, but `#include <QAction>` works on both. Fix any `#include <QtWidgets/QAction>` |

`std::endl` is unrelated to Qt and can stay. `QRegExp`, `QDesktopWidget`, and `QTextCodec` are not used.

### 3.4 Branches (2026-11-11)

- `develop` (01f880a, 19.0.1): the base of the work.
- `master` (42848cd, 19.0.1): develop plus the AppImage workflow of PR #59, `utils/appimage/`, and `.gitignore` (§3.2). Otherwise the same as develop.
- `feature/support-tcp-ws` (45bca98): develop (01f880a) plus 5 commits for the TCP / WebSocket support. Not merged (D7). Has the plan `docs/TRANSPORT_PLAN.md` (not in the repository).
- `feature/ci`: the first branch of this plan (§9.1). Created from develop (01f880a) on 2026-11-11.
- `origin/feature/json-reformat` (2024-03): 2 commits that adapt the old parser `rcss/rcg/parser_json.cpp` and `parser.cpp` to the new JSON format. The current develop uses `parser_simdjson.cpp`, so check the content and decide whether to take it into the §7 work or discard it.
- `origin/develop_observation` (2022-11) and `origin/fix/qt-asprintf` (merged into master) are unrelated to this plan.

### 3.5 When merging the TCP / WebSocket support later

As in D7, the branches of this plan do not include `feature/support-tcp-ws`. When it is adopted and merged, the following happens.

- Files changed by both: `src/main_window.cpp`, `src/options.{h,cpp}`, `CMakeLists.txt`, `src/CMakeLists.txt`, `configure.ac`, `src/Makefile.am`, `src/src.pro`, `README.md`, `NEWS`, `test/CMakeLists.txt` (both create it). The features are separate, so resolving the conflicts is expected to be easy.
- This plan sets up the tests in the same shape as `feature/support-tcp-ws` (the top-level `CMakeLists.txt` looks for Test and, if found, calls `enable_testing()` and `add_subdirectory(test)`; `test/CMakeLists.txt` has an `add_executable` and an `add_test` per test). When merging, putting both sets of tests side by side should be enough.
- Phase 3 (Qt6 support) does not cover the files added by the TCP / WebSocket support (`connect_dialog.cpp`, `*_monitor_transport.cpp`, `message_splitter.cpp`). When merging, merge develop into `feature/support-tcp-ws` (no rebase, since it has been pushed), port those files and the CMake lookup of WebSockets (`Qt${QT_VERSION_MAJOR}::WebSockets`) to Qt6 there, and then make the PR. WebSocket on Qt6 needs `qt6-websockets-dev` (not installed on the work machine).
- Of the line numbers in this plan (§6), those in `main_window.cpp` shift after the merge.

## 4. The 3D view of the reference implementation (rcssmonitor-web)

### 4.1 Features

The "3D view" section of the web version's README and the "New: the 3D view" section of the release notes `docs/release-notes/v20.0.0-beta.2.md` give a summary.

- **Switching**: View > 3D View, the 3D toolbar button, key D. Switching between 2D and 3D keeps the frame, the player selection, and the state of the referee menu.
- **Cameras** (View > 3D Camera, the toolbar selector, key E for the next): free (left-drag to orbit, Shift / Ctrl + drag to pan, wheel to zoom), broadcast, close-up (ball), close-up (selected player), top, behind the goal (left, right), the selected player's view. Dragging in any other camera switches to free.
- **Whole-pitch viewpoints**: I for the view from the side of the pitch, Ctrl+I for the next viewpoint (9). Also selectable from the menu and a toolbar button. In 2D, I fits the field to the window as before, and Ctrl+I only tells that it is for 3D.
- **Player view**: over the shoulder (default) or at eye height. Direction: head (body + neck), body, or in between. Motion: smooth (angular speed limited) or instant. Field of view: fixed (default 120°), matching the view width, or eased. What to draw: everything, only within the view, or the server's vision rules (default). The Player Camera tab of the settings dialog.
- **Same drawing as 2D**: player state colors, cards, view cones, the body and face direction lines within the foot circle (kickable area or Player Size), the overlays of the Show tab (attention target, catch / tackle / kick acceleration areas, pointing, offside line, the ball's predicted path, player type, jersey number color for illegal defense). Area values (tackle probability, etc.) go to the HUD.
- **Jersey numbers and stamina**: the number above the head (limited on screen to twice the Player font). In the top camera, it is written to the right of the kickable area at the same size as in 2D, as 2D does. Stamina and stamina capacity as bars above the head (the S key cycles none → stamina → capacity → both).
- **Referee operations**: right-clicking a player targets that player (cards) and the point at their feet; right-clicking the ground targets a point on or around the pitch. Outside the pitch or above the horizon, there is no position.
- **Figures** (Players in the 3D tab): a CC0 mannequin. Walks and runs by speed; from the log, kicks, tackles, catches, points, and sits down after being fouled. The same pose whether stopped or rewound. Cylinders in the top camera and for players whose direction is unknown.
- **Telling the ends apart**: the goal nets, the advertising boards around the pitch, and the banners in the air behind the goals are in team colors (boards and banners can be switched off). A compass strip and a minimap can be overlaid.
- **Appearance** (3D tab, each switchable, on by default): sun shadows, mowing stripes on the grass, sky, stands along both touchlines, a rolling ball with a pentagon pattern.
- **Aids to find the ball** (Ball in the 3D tab): a ring on the ground (default 0.7 m) and a trail of the recent cycles (a contrail).
- **Ball height**: if the show has `ball.z`, the ball is drawn in the air with a shadow directly below (for the planned 2.5D simulator; not in the current server and logs).
- **Performance**: 60 fps at 1920×1080 on a desktop GPU with everything on (measured in Firefox).

### 4.2 Web version modules and their counterparts in rcssmonitor (tentative names)

| Web version (under `src/view/`) | Lines | Content | Counterpart in rcssmonitor |
|---|---|---|---|
| `render3d/render3d.ts` | 1927 | The core of 3D rendering: scene assembly, players, labels, bars, camera updates, hit testing for referee operations, inspection state | Split into `FieldView3D` (QOpenGLWidget) and `Scene3D` |
| `render3d/cameras.ts` | 230 | Camera kinds; broadcast, behind-the-goal, and close-up poses; viewpoints; smooth following | `Cameras3D` (3D math only; no GL dependency) |
| `render3d/player-camera.ts` | 204 | Player view styles and motion | `PlayerCamera3D` |
| `render3d/sight.ts` | 111 | What a player perceives (view cone, server rules) | `Sight` |
| `render3d/interpolate.ts` | 81 | Interpolation between frames | `Interpolate` |
| `render3d/scene-math.ts` | 279 | Coordinate mapping, distance to fit the whole pitch, stamina color, color shading, etc. | `SceneMath` |
| `render3d/overlays.ts`, `../overlay-math.ts` | 435, 235 | Ground overlays and the calculations shared with 2D | Extract the calculations from the 2D painters and share them (`OverlayMath`) |
| `render3d/stadium.ts` | 270 | Advertising boards and banners | `Stadium3D` |
| `render3d/scenery.ts` | 402 | Grass stripes, sky, stands | `Scenery3D` |
| `render3d/ball.ts`, `ball-aids.ts` | 189, 338 | Ball pattern and rolling; ring and trail | `Ball3D`, `BallAids3D` |
| `render3d/field-texture.ts` | 55 | Draws the ground image with the 2D field painter | Draw with `FieldPainter` into a `QImage` and use it |
| `render3d/figures/` (`figures.ts`, `pose.ts`, `gltf.ts`) | 152, 270, 413 | Figures: deciding actions, poses, glTF loading and bone manipulation | `Figures3D`, `Pose`, `GltfModel` |
| `hud/guides.ts`, `hud/hud.ts`, `hud/score-board.ts` | 261, 183, 62 | Compass strip, minimap, HUD text, score board | Drawn over 3D with QPainter (§5.5) |
| `player-style.ts` | 107 | Player colors, the same as 2D | Share how the 2D `PlayerPainter` decides colors |

### 4.3 Main constants (web version values. When in doubt, the web version's source is authoritative)

| Item | Value | Location in the web version |
|---|---|---|
| Coordinates | World X = field x, Z = field y, Y is up. A field angle d becomes `rotation.y = -d` (degrees → radians). The glTF model faces +Z, so `rotation.y = π/2 - body` | `scene-math.ts`, `figures/gltf.ts` |
| Vertical field of view | 35° (except the player view) | `cameras.ts` `CAMERA_FOV` |
| Broadcast | 0.55 × the distance that fits the whole pitch (about 72 m at 16:9), elevation 32°. Follows the ball's ground position, limited to 10 m inside the touchlines and goal lines | `broadcastPose` |
| Smooth following | Time constant 0.35 s. At most 0.1 s advanced per frame. Considered settled within 0.02 m | `smoothToward`, `SETTLED` |
| Behind the goal | x = ±(52.5 + 18), height 10 m. Looks at the midpoint between the goal and the ball (0.6× laterally). Field of view 45° | `goalPose` |
| Top | Orthographic. Fits the ground image area, leaving 15% of the height for the HUD | `topHalfSize` |
| Close-up | Distance 12 m (3–60), elevation 20° (5–85). For a player, looks at height 1 m. Time constant = 0.0089 s/m × distance (0.05–0.35 s). One wheel notch ≈ 1.1× | `closeupPose`, `closeupTimeConstant` |
| Viewpoints | 9 azimuth and elevation pairs (`VIEWPOINT_ANGLES`). The shortest distance at which the corners of the pitch plus 2 m around fit in 97% horizontally and 85% vertically, computed by a formula | `fitView` |
| Player view | Default: over the shoulder (6 m behind, 3 m up), head, smooth, fixed 120° (horizontal) field of view, server rules. τ 0.5 s, ω 90°/s. Eye height 1.65 m | `player-camera.ts` |
| Vision rules | Within the view cone, up to `player_max_observation_length`. Jersey numbers fade between `unum_far_length` 20 m and `unum_too_far_length` 40 m; without a number there is no direction either. The team turns gray between `team_far_length` 40 m and `team_too_far_length` 60 m. Outside the view cone, players within `visible_distance` 3 m are sensed | `sight.ts` |
| Jersey numbers | A label 1.3 m high (text 40/64). On-screen text height at most twice the Player font. When shrinking, shrink toward the top of the head | `fitLabels`, `LABEL_MAX_FONT` |
| Heights of the ground layers | Shadow plane 0.01, foot circle 0.02, ball ring 0.028, ball shadow 0.03, view cones 0.035–0.04, overlay lines 0.045 m | Each file |
| Advertising boards | 5.5 m from the touchlines and goal lines, 1 m high, the team name every 10 m, brightness 0.32 × the team color | `BOARDS`, `BOARD_SHADE` |
| Banners | 22 m behind the goal line, 50 m wide, 8 m high, bottom edge at 12 m | `BANNER` |
| Shadows | Sun direction (-60, 65, -30) (from the left rear, elevation 44°). Map of 4096 px (if the GPU handles 8192) or 2048 px. Covers the ground image area + 4 m, up to 3 m high. PCF, radius 2, bias -0.0005, normalBias 0.02. Opacity of the shadow-only plane 0.4. Shadows are cast by the players and the goal frames. Not drawn in the top camera | `render3d.ts` `fitSunShadow` |
| Grass stripes | 5.25 m wide (20 on the pitch). 1.05× and 0.92× the grass color | `GRASS` |
| Sky | Zenith #3d7ccc, horizon #d4e4f0, below the horizon #6f9f7f. A sphere of radius 1900 m attached to the camera. Fog 600–1500 m. The outer ground is 3000 m square | `SKY` |
| Stands | 16 rows from 2 m outside the boards (0.85 m deep, 0.42 m high, a 1.2 m wall below the front row). Same length as the boards. 45% of the spectators in the color of the team of that half | `STANDS` |
| Ball | Pattern: 12 black pentagons at the vertices of an icosahedron (corners at 1/3 of the way to the adjacent vertices). Rolls at most 0.8π per cycle. A move over 5 m is treated as a placement | `ball.ts` |
| Ball ring | Radius 0.7 m (0.1–5). The line is the thicker of 0.06 m and 2 px on screen. Inner opacity 0.12 | `ball-aids.ts` |
| Trail | 5 cycles (1–30). The head width is the larger of 1.5 × the ball diameter and 6 px on screen. Opacity 0.75. Older parts are thinner and fainter | `ball-aids.ts` |
| Figures | Height 1.85 m × 1.2. Clips Idle_Loop, Walk_Loop, Jog_Fwd_Loop, Sprint_Loop (1.4, 3.6, 6.5 m/s respectively). Bones Head, spine_02, thigh_l/r, calf_l/r, upperarm_l/r (the ones out of 65 that the poses move). Uniform material M_Main | `figures/gltf.ts` |

### 4.4 Log and rcssserver behavior that matters

- **Tackle bit**: rcssserver keeps the Tackle / TackleFault bit while it holds the player (`tackle_cycles`, default 10). A tackle that becomes a foul does not hold the player, so the bit lasts only 1 frame. Figures slide only during the frames in which the bit continues (`Player::tackle`, `resetState`).
- **FoulCharged bit**: a fouled player is held for `foul_cycles` (default 5), and the bit stays during that time.
- **Catch bit**: 1 frame only. The catch pose is kept for `catch_ban_cycle`.
- **Kick bit**: stays during the kick (`kick_cycles`). Usually 1 frame.
- Live shows also include unconnected players with `state: 0`, placed at x = -(unum × 3 × side), y = -37.
- Details in `rcssmonitor-web/docs/protocol.md` §5.

## 5. Design

### 5.1 Overall structure

```
MainWindow
 └ QStackedWidget (central)
     ├ FieldCanvas      (2D; as is)
     └ FieldView3D      (3D; QOpenGLWidget)
          ├ Scene3D      scene assembly and drawing (GL)
          ├ Cameras3D / PlayerCamera3D / Sight / Interpolate  (math only; no GL dependency)
          ├ Figures3D    figures (cgltf, pose computation, bone matrices)
          └ HUD          drawn over 3D with QPainter
DispHolder / LogPlayer / Options  (shared with 2D)
```

- 2D and 3D share `DispHolder` and the player selection. Switching just swaps the widgets; the playback position and the selection stay.
- The calculations needed for 3D (cameras, interpolation, perception, poses, trails, etc.) live in classes without a GL dependency and are checked with unit tests. The web version was split the same way.
- When `ENABLE_3D` is OFF, or an OpenGL 3.3 core context cannot be created at run time, disable the 3D menus and keys and show the reason in the status bar.

### 5.2 Time and interpolation

- The current `LogPlayer` only advances one frame at a time with a `QTimer` and has no position between frames. 3D draws from the current frame, the next frame, and the fraction between them (alpha). Add to `LogPlayer` a function that returns "time since the last advance / interval" (`QElapsedTimer`). When stopped, alpha = 0.
- Interpolation: positions linearly, angles the short way round. No interpolation around play mode changes and big jumps (a player moving over 2 m in one cycle, the ball over 5 m, etc.) (the web version's `interpolate.ts`).
- Live: draw one cycle behind and interpolate between the latest and the one before (to smooth the motion).
- Draw calls: while playing, while the camera moves, and while figure poses change, `update()` every frame (request the next one after `QOpenGLWidget::frameSwapped`). Stop calling when nothing moves.
- **Poses, ball rotation, and trails are determined only from the log frames and alpha** (no state accumulated per draw). Stopping, or stepping back, shows the same thing for the same frame. In the web version, accumulating time per draw made the legs slide frozen when rewinding, and the pose changed even for the same frame (§11).

### 5.3 Rendering infrastructure

OpenGL 3.3 core (requested with `QSurfaceFormat`; up to 4.1 core on macOS). Use `QOpenGLFunctions_3_3_Core`. 4× multisampling.

| Drawing kind | Used for |
|---|---|
| Unlit textured | Ground image, outer ground, banners, boards, stands (lit) |
| Lit solid color (Lambert), instanced | Player cylinders and heads, direction markers, goal frames |
| Translucent solid color, instanced | Foot circles, decay rings, selection rings, view-cone sectors |
| Skinned mesh (lighting close to Standard) | Figures (uniform color per material) |
| Lines (per-vertex color) | Overlays, large view-cone sectors, nets |
| Points | Points of the ball's predicted path |
| Billboards (screen-facing textured quads) | Jersey numbers, cards, stamina bars |
| Shadow depth (instanced and skinned) | Sun shadow map |
| Shadow-only plane | Shadows on the ground |
| Sky gradient | Sky sphere |
| Ball pattern | Ball (pentagons computed from the direction) |
| Ball ring | Ball ring (line and inside computed from the distance) |
| Fog | Blends the outer ground into the horizon |

- Buffers whose content changes every frame are created once with a fixed size and reused (`glBufferSubData`). In the web version, creating new buffers every frame once leaked GPU memory.
- The shadow map is not drawn in the top camera (the shadow plane is hidden too). It is not drawn when the shadow setting is off.
- Draw order: opaque → ground layers (stacked by height) → translucent → sky → billboards (no depth test).

### 5.4 Components

Read the corresponding web version file and build with the same values and the same ideas. Values in §4.3.

- **Ground**: draw with the 2D `FieldPainter` into a `QImage` at 24 px per meter with pens 3× thicker, and use it as a texture. Do not draw the goals (they are built in 3D). Grass stripes are drawn after the grass fill and before the lines (add a "draw above the grass, below the lines" argument to `FieldPainter`). Redraw the ground image only when colors or options change.
- **Goals**: frames (posts and crossbar, 2.44 m high) and translucent nets (team colors).
- **Players**: cylinder (body), sphere (head), foot circle, a black cone for the body direction, a red cone for the face direction, cards. Colors are decided the same way as the 2D `PlayerPainter` (state color, darker with lower stamina, selection). Instanced per part. Inside the foot circle, lines for the body direction (player line color) and the face direction (neck color; not drawn when view cones are drawn).
- **Ball**: the radius is the Ball Size option or `ball_size`. With a height, it is drawn in the air with the shadow directly below (fainter with height). Pattern and rolling per `ball.ts`.
- **Labels**: write the jersey number (and player type) into a `QImage`, use it as a texture, and draw it as a billboard. The upper size limit and the placement in the top camera per `fitLabels`, `labelPlace`.
- **Stamina bars**: offset in the camera's down direction, below the jersey number. Color by `staminaHue` (red when below `recover_dec_thr` and `effort_dec_thr`). Under the server rules of the player view, only the selected player's bar.
- **Overlays**: extract the calculations (catch area radius, tackle probability, kick acceleration, predicted path, offside line) from the 2D `PlayerPainter` and `BallPainter`, share them, and draw lines on the ground. Area values go to the HUD.
- **View cones**: a sector per player (view width, `visible_distance`); for the selected player, a 60 m sector in lines.
- **Boards and banners**: `stadium.ts`. Write the team names and logos (when Team Graphic is on) into textures.
- **Scenery**: `scenery.ts`. The faces of the stands point only toward the pitch and upward; the backs and ends face outward (single-sided). Even if a close-up camera near the touchline enters the stands, the pitch is not hidden.
- **Ball ring and trail**: `ball-aids.ts`. The minimum line width is decided by computing the length of 1 px at the depth from the camera.
- **Shadows**: §4.3.
- **Figures**: §5.8.
- **Referee operations**: cast a ray from the screen point and intersect it with the players (invisible hit cylinders; 1.2× for figures) or the ground. Outside the pitch and its surroundings, or above the horizon, there is no position, and Drop Ball and Free Kick are disabled (with the reason in a tooltip). Reuse the current right-click menu of `FieldCanvas` in 3D.

### 5.5 HUD

- At the end of `paintGL` of the `QOpenGLWidget`, draw the 2D overlay with `QPainter`: score board (the same painter as 2D), team logos, area values, compass strip, minimap, messages.
- Compass strip: top center, up to 480 px wide. The camera direction at the center, clockwise to the right. Both goals (L, R in team colors) and the ball direction (`guides.ts`).
- Minimap: bottom right, the same pitch as 2D. Players, ball, camera position, and the visible area.
- Both have the setting "off / player view only / all cameras". The compass strip is not shown in the top camera.

### 5.6 Input

- Mouse: in the free camera, left-drag orbits, Shift (or Ctrl) + left-drag pans, the wheel and the middle button zoom (the web version uses three.js OrbitControls; write the same operations ourselves). Dragging in any other camera switches to free. In the close-up cameras, the wheel changes the distance, which is kept in the settings. Right-click is the referee menu.
- Keys (the web version's `docs/parity.md` §8 and `src/ui/shortcuts.ts`):

| Key | Function | rcssmonitor now |
|---|---|---|
| D | Switch 2D / 3D | Free |
| E | Next camera | Free |
| I | In 3D, the viewpoint from the side of the pitch (free). In 2D, as before | Fit Field Size |
| Ctrl+I | In 3D, the next viewpoint. In 2D, tells that it is for 3D | Free |
| S | Cycle the stamina display none → stamina → capacity → both (also affects the 3D bars) | Same (2D) |

No shortcuts use Shift. A related separate operation gets Ctrl (the same pattern as G / Ctrl+G and Z / Ctrl+Z).

### 5.7 Settings

Add to `Options` and save to `~/.rcssmonitor.conf`. The names follow the web version (the same as its URL parameter names), and command line options with the same names are added. Out-of-range values are not loaded.

| Name | Default | Values |
|---|---|---|
| `view_3d` | off | Start in 3D |
| `camera_3d` | broadcast | free / broadcast / closeup-ball / closeup-player / top / goal-left / goal-right / player |
| `closeup_distance_3d`, `closeup_elevation_3d` | 12, 20 | 3–60 m, 5–85° |
| `player_camera_position` | shoulder | eye / shoulder |
| `player_camera_direction` | head | head / body / middle |
| `player_camera_motion` | smooth | instant / smooth |
| `player_camera_fov` | fixed | view / eased / fixed |
| `player_camera_sight` | server | all / view / server |
| `player_camera_time_constant`, `player_camera_max_turn`, `player_camera_fixed_fov` | 0.5, 90, 120 | 0–5 s, 1–3600 °/s, 20–120° |
| `compass_3d`, `minimap_3d` | off | off / player / all |
| `boards_3d`, `banners_3d` | on | |
| `player_model_3d` | simple | simple / figure |
| `shadows_3d`, `grass_stripes_3d`, `sky_3d`, `stands_3d`, `ball_pattern_3d` | on | |
| `ball_ring_3d`, `ball_ring_radius_3d` | on, 0.7 | 0.1–5 m |
| `ball_trail_3d`, `ball_trail_cycles_3d` | on, 5 | 1–30 (integer) |

Settings dialog:

- **3D tab**: Close-up Camera (distance, elevation), Guides (compass strip, minimap), Players (Model, Shadows), Ball (Pattern, Ring, Trail, Ring Radius, Trail cycles), Stadium (Boards, Banners, Stands, Grass Stripes, Sky), Default button.
- **Player Camera tab**: the 4 styles, time constant, maximum angular speed, fixed field of view, Player Sight, Default button.
- Every item gets a detailed tooltip. Numeric fields state their range in the tooltip, and out-of-range values are clamped to the ends (error color while typing). Numeric fields not used by the selected style are dimmed.
- In the web version, the 3D items grew and one tab became too tall, so Player Camera was split into its own tab. If the items grow further, split them likewise.

### 5.8 Figures

- Model: the mesh, 65 bones, and 4 clips of `Mannequin.glb` (Quaternius's Universal Animation Library [Standard], CC0). The web version's `src/view/render3d/figures/models/README.md` gives the source and how it was trimmed. As in D6, re-export it without compression and put it in qrc.
- Loading: with cgltf, read the mesh (positions, normals, bone indices and weights), the bone hierarchy and the inverse bind matrices, and the keyframes of the clips.
- **Poses are decided from the log every frame** (no animation player):
  1. Table of distance walked (`gaitAt`): for each player, compute once from the log and keep the distance walked up to each frame (movement opposite to the body direction is subtracted; a move over 2 m in one cycle is not counted).
  2. Clip time: for walk, jog, and sprint, "distance walked / the clip's speed"; for idle, the game time. Blend with weights from the speed (sprint (v-5)/1.5, jog (v-2)/1.5, walk v/0.8, clamped to 0–1).
  3. Compute the local bone rotations from the keyframes and blend them by weight.
  4. Layer the action poses (`pose.ts`): neck (rotate the head bone by the neck angle), kick (from the bits of the surrounding frames: wind-up, impact, follow-through; the kicking leg is on the ball's side), tackle (slide while the bit continues, hold for 0.3 cycles after the last one, get up over 1.5 cycles), fouled (sit down with the hands on the ground behind), catch (raise both hands), pointing (right arm). Actions are decided by the range of frames in which the bit continues (`actionRun`).
  5. Compute the bone matrices and pass them to the skinned drawing.
- The rotations are layered on top of the clip pose "from scratch every time". In the web version (three.js), the animation mixer does not rewrite bones whose values do not change, so the layered rotations accumulated per draw and the neck and body kept turning (§11). Computing everything ourselves every time avoids this.
- The size is 1.2× real. Drawn as cylinders in the top camera. Under the server rules of the player view, players whose direction is unknown are also drawn as cylinders (a figure would reveal the direction by its shape). Figures get no direction cones.
- Hit testing uses invisible cylinders for everyone (1.2× for figures).

## 6. Bug fixes

From the web version's `docs/parity.md` §7. The line numbers were checked on develop (01f880a) on 2026-11-11 (only B11 differs from `feature/support-tcp-ws`, where it is at 2394-2429). Use the web version's implementation as a reference for the fixes.

| # | Bug | Location | Fix |
|---|---|---|---|
| B1 | The live display stops after 65,535 frames | `src/disp_holder.cpp:65` (`MAX_SIZE = 65535`), `:249` | Do not stop accepting at the limit. Either raise the limit or drop the oldest frames (decide during implementation and record it in §14). The web version has no limit |
| B2 | JSON `vq` is stored as a bool. `view_quality_` is a char `'h'` / `'l'`, so for JSON logs the view quality is always judged low | `rcss/rcg/parser_simdjson.cpp:478` | Store `'h'` / `'l'` |
| B3 | An unknown JSON key aborts the whole parse | `rcss/rcg/parser_simdjson.cpp:85-164` | Skip unknown keys (§7) |
| B4 | JSON players are stored in array order | `rcss/rcg/parser_simdjson.cpp:447-525` | Store them by `side` and `unum` (§7) |
| B5 | `foul_push_r` becomes PM_Null (a typo: `{ "foul_push_l", PM_Foul_Push_Right }`) | `rcss/rcg/util.cpp:854-855` | Make it `foul_push_r` |
| B6 | In text rcg team graphics, strings of 16 or more characters break (they are read with `%15[^"]` into `char buf[16]`, so with 16 or more characters the closing `"` does not match and the read position shifts) | `rcss/rcg/parser_v4.cpp:625-709` (`:696-697`) | Read inside the quotes without a fixed length (a `std::string` up to the next `"`) |
| B7 | The parameter defaults are outdated (about 40 items such as `player_speed_max` 1.2 and `stamina_max` 4000). `effort_inc` is missing from the table | `rcss/rcg/types.cpp:547-760` | Align with the defaults of asio rcssserver 19.0.0 (the values in the web version's `src/core/param-defaults.ts`) |
| B8 | If a text rcg has even one line that does not start with `(name`, loading of the whole log stops | `rcss/rcg/parser_v4.cpp:95-102, 123-128` | Skip that line and continue |
| B9 | The offside line is drawn only between x = 0 and the ball (the player's side is compared against the enum value) | `src/player_painter.cpp:1034, 1061` | Fix the comparison |
| B10 | The catch area of the left goalie uses the right goalie's color (the same comparison error as B9) | `src/player_painter.cpp:545, 563` | Same as above |
| B11 | Change Playmode back_pass computes the position and then sends nothing | `src/main_window.cpp:2359-2394` | Send `back_pass_l` / `back_pass_r` (the web version's `src/io/live/commands.ts` `changePlayModeCommands`) |
| B12 | "Fix" in the settings dialog sets Auto-right (`setPlayerSelectType( SELECT_AUTO_RIGHT )`) | `src/config_dialog.cpp:2441` | Make it `SELECT_FIX` |
| B13 | The Illegal Defense entry in the color list uses the Pointto color (`pointtoPen().color()`) as its initial value | `src/config_dialog.cpp:1258-1260` | Use the Illegal Defense color |
| B14 | In the player type list, columns with two numbers do not sort (the cell text is read as one number) | `src/player_type_dialog.cpp:69-75` | Compare by the first number |

- Fix them one at a time, with a unit test where possible (Qt Test; parsing bugs with fixtures, drawing bugs by extracting the calculation and checking it).
- If merging into master is urgent, the bug fixes alone can go first as a separate PR.

## 7. JSON parser improvements

The targets are `rcss/rcg/parser_simdjson.cpp` (logs) and the live JSON messages (the same parser, used from `disp_holder.cpp`). Align with the web version's `src/io/rcg-json.ts` and `docs/protocol.md` §3 and §7.

1. Skip unknown keys (B3), so that logs can still be read when the server adds fields in the future.
2. Store `vq` as `'h'` / `'l'` (B2).
3. Store players by `side` and `unum` (B4), so that missing array elements and a different order are tolerated.
4. Read the log as one record per line. When the server terminates abnormally the final `]` is missing, so skip the `[` and `]` lines and strip the trailing `,`. If a line is broken, skip only that line and warn.
5. Accept fields that may be omitted: `stime` (only when greater than 0), `px` / `py` (only while pointing), `fside` / `fnum` (only with an attentionto), `pen_score` / `pen_miss` (only for teams that took penalty kicks), `null` team names (before connection), `capacity`.
6. Read the ball height `ball.z` and the vertical velocity `ball.vz` if present (for the 2.5D simulator; units m and m/cycle, up is positive. The key names are tentative and will follow the server format once it is decided). Add them to `BallT` in `types.h`.
7. Handle `timestamp` (`%FT%T%z`, offset without a colon) without breaking it. The current parser skips `version` and `timestamp` and does not pass them to the handler (`parseVersion` and `parseTimestamp` are empty; checked on 2026-11-11). Pass both to the handler (`handleServerVersion`, `handleTimestamp`).
8. Measure the speed: compare the load time of a full-game JSON log (`1-json.rcg.gz` placed at the top of the working tree, 6,209 frames; not in the repository) before and after the improvements, and record it in §14. The web version takes about 0.3–0.5 s in a browser.
9. Check the content of `origin/feature/json-reformat` (the old parser's support for the new format) and take anything that does not overlap with the above.
10. Also fix the text rcg bugs B6 and B8 at this stage.
11. Decide whether to update the bundled simdjson (3.8.0, with three lines of the 3.10.0 fix applied so that it builds with GCC 15; §14) to a newer release (the latest in 2026-11 is 5.1.1).

Tests: with fixtures (a short excerpt of `1-json.rcg.gz`, broken lines, unknown keys, swapped player order, a missing final `]`), check the number of frames, the values in specific frames, and the sequence of play modes. Models: the web version's `test/unit/rcg-json.test.ts`, `rcg-text.test.ts`, and `full-logs.test.ts`.

## 8. Qt6 support

1. CMake: select the version with `find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)` and link with `Qt${QT_VERSION_MAJOR}::Widgets` etc. Test in the same way (WebSockets when merging, per §3.5). For Qt6, add `OpenGL` and `OpenGLWidgets` (for 3D). In Qt5, `QOpenGLWidget` is in Widgets.
2. Make the replacements in §3.3, choosing forms that also work on Qt 5.15 (dual support, D2).
3. Remove `QGLWidget` under `USE_GLWIDGET` if it is unused. If it is used, make it `QOpenGLWidget`.
4. Remove the `QT_VERSION_CHECK` branches for versions older than Qt 5.15.
5. Eliminate deprecation warnings: build with `QT_DISABLE_DEPRECATED_BEFORE=0x050F00` on Qt5, and confirm that there are no deprecation warnings on Qt6.
6. High DPI: enabled by default in Qt6. Confirm that the 2D drawing (line widths, text sizes) does not break at scale 1, 1.5, and 2. 3D renders according to `devicePixelRatio`.
7. autotools stays on Qt5 as in D3 (qmake is dropped by D11); write in the README "Qt6 and 3D with CMake".
8. Build both Qt5 and Qt6 in CI (§10).
9. Rewrite the README installation instructions for both Qt5 and Qt6 on Ubuntu 22.04 / 24.04 / 26.04.
10. Raise `cmake_minimum_required(VERSION 3.5)`. The CMake in CI warns that "compatibility with CMake < 3.10 will be removed" (§14). Qt6 needs CMake 3.16 or later, so make it 3.16.

## 9. Phase plan

Each phase proceeds on a feature branch and is merged into develop when it is done (D9). The 3D view (Phases 4–8), however, is merged into develop only after it is complete and its official adoption is decided. Confirm with the user at each step.

### 9.1 Branches (approved by the user on 2026-11-11)

| Phase | Branch |
|---|---|
| Phase 0: CI and test infrastructure | `feature/ci` |
| Phase 0: 3D prototype | `proto/3d-gl` (not merged; how to create it is discussed when the prototype starts) |
| Phase 1: Bug fixes | `fix/parity-bugs` |
| Phase 2: Parser improvements | `feature/json-parser` |
| Phase 3: Qt6 support | `feature/qt6` |
| Phase 4–8: 3D view | `feature/3d-view` |

- Phases 0–3 are merged into develop as each one is done (the user does the merges). The next phase's branch is created from develop after the previous phase has been merged. To start before that merge, create it from the previous phase's branch (stacking). Phase 1 and Phase 3 both change `main_window.cpp`, `config_dialog.cpp`, and `player_painter.cpp`, so stack them in order rather than working on them in parallel.
- `feature/3d-view` is created from develop after the Qt6 support (Phase 3) has been merged. It is not merged into develop until it is complete and its official adoption is decided. Anything merged into develop in the meantime (the TCP / WebSocket support, etc.) is brought in by merging develop into `feature/3d-view`.
- The user creates the branches on GitHub (from 2026-11-11). Locally, run `git fetch origin` and then `git switch <branch>`, which creates a local branch tracking the GitHub branch and switches to it. Only `feature/ci` was created locally first, so it is created on GitHub with `git push -u origin feature/ci`.
- How to create `proto/3d-gl` (the prototype) is discussed when the prototype starts (2026-11-11).

### Phase 0: Foundation and validation

**Work**
- Confirm the decisions in §2.2 with the user.
- Create branches from develop, as in D7 (§9.1).
- Set up CI (GitHub Actions). On Ubuntu 26.04 (`runs-on: ubuntu-26.04`, as in the web version's CI), build with Qt5 and run the unit tests. The Qt6 job is added in Phase 3, when the Qt6 build passes. Use action versions that run on Node.js 24 (as in the web version: checkout@v5, upload-artifact@v7, etc.).
- Set up the test infrastructure (Qt Test, CTest) on develop, in the same shape as `feature/support-tcp-ws` (§3.5).
- Try option A of D10 (§2.4) (during the trial, it runs on pushes to `feature/ci`).
- 3D prototype (checking D1): with `QOpenGLWidget`, draw 22 skinned figures (walking with blended clips), a 4096 px shadow map, and instanced cylinders, and measure whether 60 fps holds at 1920×1080. Run it on both Qt 5.15 and Qt 6. Record the result in §14 and decide D1.
- Prepare the figure model as in D6.

**Done when**: everything in §2.2 is decided. CI passes. The prototype confirms 60 fps (if D1 is the recommendation). Whether to adopt option A of D10 is decided.

### Phase 1: Bug fixes

**Work**: B1, B5, and B9–B14 of §6 (the ones outside the parser), with unit tests.

**Done when**: each fixed bug has a test, and CI passes. The user checks with real logs.

### Phase 2: JSON and text parser improvements

**Work**: §7 (including B2–B4 and B6–B8).

**Done when**: the fixture tests pass. Two full-game logs (JSON and text v5) load, and the counts match the files. The load times are recorded.

### Phase 3: Qt6 support

**Work**: §8. Add the Qt6 job to CI.

**Done when**: it builds on both Qt 5.15 and Qt 6 without deprecation warnings. CI passes for both versions. The user checks the 2D display, the live connection (UDP; TCP / WebSocket after merging, per §3.5), log playback, referee operations, and the settings dialog on Qt6.

### Phase 4: 3D foundation

**Work**
- `FieldView3D` and switching with 2D (D key, menu, toolbar). `ENABLE_3D` and the handling when GL is unavailable.
- Time and interpolation (§5.2). The one-cycle delay for live.
- Ground image, goals, players (cylinders: state colors, cards, direction markers, foot circles and direction lines, view cones), ball (height and shadow).
- Cameras: free, broadcast, top, and the two behind the goals. E key. Mouse operations.
- HUD (score board, logos).
- Jersey number labels (upper size limit, placement in the top camera).

**Done when**: switching between 2D and 3D keeps the frame and the selection. With a show that has `ball.z` (made for testing), the ball is drawn in the air and its shadow appears on the ground. The unit tests for each camera's pose pass.

### Phase 5: Same information as 2D, and referee operations

**Work**: overlays (§5.4; extract the calculations from the 2D painters and share them), area values in the HUD, stamina bars and the S key, the illegal defense color, hit testing for referee operations (§5.4).

**Done when**: the Show tab switches work in 3D too. Referee commands are sent for players and ground points right-clicked in 3D (checked with a fake server; the web version's `tools/replay-server` is a reference).

### Phase 6: Player view and the other cameras

**Work**: the 4 player view styles and perception (§4.3, `player-camera.ts`, `sight.ts`), the two close-up cameras (distance by the wheel), the 9 whole-pitch viewpoints (I, Ctrl+I, menu, toolbar), compass strip and minimap, team colors at both ends (nets, boards, banners), the 3D tab and the Player Camera tab of the settings dialog.

**Done when**: every combination of the 4 player view items can be selected. `smooth` does not exceed the angular speed limit (unit test). `view` and `server` draw differently (unit test). The corners of the pitch fit on screen in every viewpoint (unit test).

### Phase 7: Figures

**Work**: §5.8.

**Done when**: walking, running, kicking, tackling (momentary for a foul), sitting down after being fouled, catching, and pointing are confirmed with figures on real logs. Stepping forward and back, and jumping back, give the same pose (checked with unit tests and the inspection state of the renderer). Drawing the same frame repeatedly does not change the neck or the legs.

### Phase 8: Appearance and ball aids

**Work**: shadows (§4.3), grass stripes, sky and fog, stands, ball pattern and rolling, ball ring and trail. Settings and the dialog.

**Done when**: each one can be switched in the settings. The first frame after switching to the top camera equals the next frame (§11 item 4). 60 fps holds at 1920×1080 on a desktop GPU with everything on (measurement recorded in §14).

### Phase 9: Documents and release

**Work**: README (3D, keys, settings, Qt6 build), ChangeLog and NEWS (including the removal of qmake, D11), the version per D8. According to the result of D10, delete master's `main.yml` and `utils/appimage/` or replace them with option A (done in the merge into master). The user merges into master and tags the release.

**Done when**: it is released.

## 10. Test plan

### 10.1 Unit tests (Qt Test)

Port the GL-independent calculations, using the web version's unit tests as models.

| Web version test | What it checks |
|---|---|
| `view3d-motion.test.ts` | Interpolation, camera poses (broadcast, behind the goal, top, close-up), following, camera order, settings |
| `viewpoints.test.ts` | Viewpoint formula (the corners fit, and some corner touches an edge) |
| `player-camera.test.ts` | Player view styles, angular speed limit, perception rules |
| `figures.test.ts` | Action ranges (a tackle bit lasting 10 frames and 1 frame), the fouled pose, the table of distance walked (the same regardless of drawing order, decreases when moving backward, ignores jumps) |
| `overlays.test.ts` | Overlay calculations, ends and colors of the direction lines, stamina bars |
| `scenery.test.ts` | Grass stripe bands, orientation and extent of the stand faces, ball pentagons and rolling |
| `ball-aids.test.ts` | Age and length of the trail points, breaking at a placement, direction / width / opacity of the band |
| `guides.test.ts`, `scene3d.test.ts` | Coordinates of the compass strip and the minimap, ball height and shadow opacity, colors |
| `rcg-json.test.ts`, `rcg-text.test.ts`, `full-logs.test.ts` | Parsers (§7) |

### 10.2 Rendering tests

- Give `FieldView3D` a function that returns an inspection state (like the web version's `Render3DState`: the numbers of players and figures drawn, camera position and direction, ball position and rotation, shadows, scenery, label positions and sizes, number of overlays, number of draw calls, etc.). Rendering tests check this.
- Use offscreen rendering (`QOffscreenSurface` and `QOpenGLFramebufferObject`, or `QT_QPA_PLATFORM=offscreen`), and software OpenGL in CI (Mesa llvmpipe, `LIBGL_ALWAYS_SOFTWARE=1`). `xvfb-run` if needed.
- Pixel checks use comparisons that do not depend on colors (e.g. the first frame after a switch equals the next frame). In the web version, a test that only counted black pixels missed the same bug when it turned white.
- **Software rendering is slow**. Tests draw without shadows (only the shadow tests turn them on explicitly), do not wait for smoothly moving cameras to arrive, and give generous time limits to tests with figures. In the web version's CI, these three caused timeouts when the runner was slow.

### 10.3 Manual checks

- fps on a real GPU (build a mechanism that logs the drawing time).
- Live connection with asio rcssserver (UDP; TCP / WebSocket after merging). Set `RCSS_CONF_DIR` and `HOME` to temporary directories (§0.1).
- Appearance with full-game logs (`1-json.rcg.gz` and `202207160432-HELIOS2022_1-vs-CYRUS_0.rcg.gz` placed at the top of the working tree; not in the repository). Compare side by side with the web version.

## 11. Lessons from the web version

1. **Decide from the log**: figure poses, ball rotation, and trails are computed every time from the frame and alpha only. Accumulating time or rotation per draw breaks stepping and rewinding (web version step 12).
2. **Look at the range in which a bit continues**: tackles and fouls decide the pose from the range of frames in which the bit continues (§4.4). At first, "tackle_cycles from the frame with the bit" made held tackles stay down twice as long, and fouls ran on while sliding. Tests with made-up data (a bit in 1 frame only) did not catch it, so check with real logs.
3. **Layered rotations from scratch every time**: when layering bone rotations on top of the clip pose, do not layer them on the previous frame's result.
4. **Check the first frame after a state switch**: in the web version, because of when three.js uploads data to the GPU, only the first frame after switching to the top camera drew the cylinders at their initial values (the origin). Add tests that compare the frame right after a camera or setting switch with the next frame.
5. **Upper and lower limits on the on-screen size**: jersey numbers and bars have an upper limit (twice the Player font) so that they do not grow too large up close. The ball ring line and the trail head have lower limits (2 px, 6 px) so that they do not vanish in the distance.
6. **Following lag**: the closer the camera, the smaller the lag (the close-up time constant is proportional to the distance). Cap the time advanced per frame.
7. **Stands are single-sided**: even if the camera enters or goes behind the stands, the pitch is not hidden.
8. **Sun direction**: light from the left rear so that shadows are visible from the near-side camera. No shadows in the top camera. The ball keeps its shadow directly below, for reading its height.
9. **When not to interpolate**: around play mode switches and jumps. Live is drawn one cycle behind.
10. **Software rendering is slow**: keep the CI tests light (§10.2).
11. **Choosing keys**: use keys that are free in rcssmonitor, no Shift, and Ctrl for related operations. Unlike a browser, the native version has no key interception problem.
12. **Presenting settings**: a tooltip per item, explicit ranges, handling of out-of-range values, dimming unused fields. Split tabs when they get long.
13. **Defaults decided by the user**: player view (over the shoulder, head, smooth, fixed 120°, server rules, τ 0.5, ω 90), figures default to cylinders (simple), shadows, scenery, and ball aids are on by default.

## 12. Files to change and add (expected)

**Add** (proposal: gather them in `src/view3d/`)

| File | Content |
|---|---|
| `field_view_3d.{h,cpp}` | QOpenGLWidget: input, time, draw calls, inspection state |
| `scene_3d.{h,cpp}` | Scene assembly and drawing |
| `gl_*.{h,cpp}`, `shaders/*.glsl` | Rendering infrastructure (shaders, buffers, textures, shadow map) |
| `cameras_3d.{h,cpp}`, `player_camera_3d.{h,cpp}`, `sight.{h,cpp}`, `interpolate.{h,cpp}`, `scene_math.{h,cpp}` | Calculations (no GL dependency) |
| `overlay_math.{h,cpp}` | Overlay calculations shared by 2D and 3D (moved from the 2D painters) |
| `stadium_3d`, `scenery_3d`, `ball_3d`, `ball_aids_3d` | Components |
| `figures_3d`, `pose`, `gltf_model` | Figures |
| `hud_3d`, `guides` | HUD, compass strip, minimap |
| The 3D tab and the Player Camera tab of `config_dialog` | Settings |
| `third_party/cgltf/cgltf.h` | glTF loading (MIT) |
| `resources/models/Mannequin.glb`, `.qrc` | Figure model (CC0) and its source |
| `test/test_*.cpp` | Unit tests and rendering tests |
| `.github/workflows/ci.yml` | CI |
| `.github/workflows/appimage.yml` → `release.yml` | Builds the AppImage. The trial version was added as `appimage.yml` in Phase 0. If option A of D10 is adopted, it becomes `release.yml` and attaches the AppImage to the release |
| `data/rcssmonitor.desktop`, `icons/rcssmonitor.png` | The desktop file and the icon, installed by CMake's `install()` (added in Phase 0; §2.4) |

**Change**: `main_window.cpp` (switching, menus, toolbar, keys), `field_canvas.cpp` (sharing the right-click menu), `log_player.cpp` (alpha), `options.{h,cpp}` (settings), `config_dialog.cpp`, `player_painter.cpp`, `ball_painter.cpp`, and `field_painter.cpp` (extracting the calculations, the grass stripe argument), `rcss/rcg/*` (§6, §7), `CMakeLists.txt` and `src/CMakeLists.txt` (Qt6, 3D), README.

**Delete**: `rcssmonitor.pro`, `src/src.pro`, `src/rcsslogplayer/` (D11; deleted in Phase 0).

## 13. Out of scope

- Things specific to the web version: distribution in the browser (single HTML, GitHub Pages), the file:// and https restrictions, browser key interception, URL parameters (rcssmonitor uses the command line and the settings file).
- Improving how binary rcg (v1–v3) is handled in 3D (being readable, as in 2D today, is enough).
- Matching the server-side format of the 2.5D simulator (after the format is decided).
- Future extensions of rcssmonitor-web (moving players by dragging, a list of msg, etc.).

## 14. Record

At each step, append the date, the decisions, what was built, the bugs found and their causes, measurements, and tests.

### Phase 0

#### 2026-11-11: Confirming the policy and the branches

**Decisions (user)**
- D7: the work derives from develop. `feature/support-tcp-ws` is merged once the TCP / WebSocket support is adopted (§3.5).
- Branches as in §9.1. `feature/ci` was created first, from develop (01f880a).
- D10: master's AppImage workflow is a maintenance burden. Disable it, or replace it with an easier-to-maintain method. The replacement options are in §2.4 (option A recommended; decide after trying it in Phase 0).
- The plan goes into the repository. The English version `PLAN_3D-VIEW.en.md` is kept in parallel (§0.1).
- D2–D6, D8, and D9 proceed with the recommendations. D1 is decided by the result of the prototype.

**Checked**
- The local develop was 2 commits behind origin/develop; the user updated it (01f880a). `feature/support-tcp-ws` also branches from 01f880a.
- master is ahead of develop (the AppImage workflow of PR #59; §3.2, §3.4). Fixed the first draft's "master and develop have the same content" and "there is no CI".
- `test/` was added on `feature/support-tcp-ws` and is not on develop (§3.1).
- Rechecked the §6 line numbers on develop. Only B11 differs. The §3.3 counts are also the same on develop.
- Work environment: Ubuntu 24.04, CMake 3.28, Qt 5.15.13 and Qt 6.4.2 (Core, Gui, Widgets, Network, OpenGL, OpenGLWidgets, and Test are available; Qt6 WebSockets is not). GPU: AMD (PCI ID 13c0).
- The rcssmonitor-web working tree is 1 commit after `v20.0.0-beta.2` (ed36941) (bcab423; a plan update only).
- The asio rcssserver working tree had moved from the top of the repository (`rcssserver-19.x-asio/`) to `../rcssserver` (`feature/asio-migration`, 19.0.0). Fixed the header table.
- The existing release tags have the form `rcssmonitor-X.Y.Z`. The 19.0.1 release has an AppImage attached.

#### 2026-11-11: Test infrastructure and CI (`feature/ci`)

**Built**
- Test infrastructure: the top-level `CMakeLists.txt` looks for Qt5 Test and adds `test/` (written the same way as `feature/support-tcp-ws`, per §3.5). The first test, `test/test_rcg_parser.cpp`, checks what the text rcg (v5) and JSON parsers pass to the handler: header, parameters, teams, play modes, and shows. The fixtures `test/data/head_v5.rcg` (20 KB) and `head_json.rcg` (41 KB) are cut from the start of the two full-game logs: the header and parameters, then the first play modes, team, and 3 shows (team graphics left out).
- `.github/workflows/ci.yml`: on ubuntu-26.04, the CMake (Qt5) build and ctest, and the autotools (Qt5) build. Runs on pushes to master, develop, `feature/**`, `fix/**`, `hotfix/**`, and on PRs.
- `.github/workflows/appimage.yml`: the trial of option A of D10 (§2.4). Runs on pushes to `feature/ci`. Builds with CMake in an `ubuntu:22.04` container, bundles the AppImage with linuxdeploy `1-alpha-20251107-1` and linuxdeploy-plugin-qt `1-alpha-20250213-1`, and checks that it starts by running `--version` under `xvfb-run` in `ubuntu:22.04`, `24.04`, and `26.04` containers.
- `data/rcssmonitor.desktop` and `icons/rcssmonitor.png` (48×48, made from `icons/rcss.xpm`). CMake's `install()` puts them in `share/applications` and `share/icons/hicolor/48x48/apps`.

**Tests (local, Ubuntu 24.04, Qt 5.15.13)**
- Ran the same steps as the two jobs of `ci.yml` on a clean copy of HEAD plus these changes. The CMake build and ctest (test functions `textV5` and `json` both pass) and the autotools build passed.
- Confirmed that the test catches failures (the first version expected something the parser does not do, and failed; see the third finding below).
- Installing into a `DESTDIR` puts the desktop file (no problems from `desktop-file-validate`) and the icon in place, and the installed `rcssmonitor --version` exits with 0.
- linuxdeploy's behavior (checked in the source of the pinned release): without `--desktop-file`, it uses the first desktop file in the AppDir, looks for the icon in the AppDir's hicolor, and also deploys the dependencies of executables already in the AppDir.
- `appimage.yml` has not been run locally (linuxdeploy was not run locally). It will be checked in CI after the push.

**Found**
- The qmake build fails on develop (§3.2, D11).
- The JSON parser skips `version` and `timestamp` (§7 item 7).
- Text rcg v4, v5, and v6 are all read by `ParserV4`, whose `Parser::version()` returns 4. The log's own version reaches the handler's `logVersion()` (by design; the test follows this).
- Ubuntu 26.04 (resolute) has Qt 5.15.18 (`qtbase5-dev`, `qt5-qmake`, `libqt5websockets5-dev`) and Qt 6.10.2 (`qt6-base-dev`, `qt6-websockets-dev`) (checked on packages.ubuntu.com).
- The web version's CI passes on ubuntu-26.04 runners, so the same label is used.
- GNUInstallDirs: in a build directory first configured with another prefix, changing `CMAKE_INSTALL_PREFIX` to `/usr` recomputes the libdir as `lib/x86_64-linux-gnu` even with `-DCMAKE_INSTALL_LIBDIR=lib`. CI configures in a fresh directory, so this does not matter there.

#### 2026-11-11: Dropping qmake, and the branch workflow

**Decisions (user)**
- D11: drop the qmake build, because no machine will use qmake any more.
- From now on, the user creates branches on GitHub before the work starts (§9.1).
- Phases 0–3 (up to the Qt6 support) are merged into develop as each one is done. The 3D view is merged into develop only after it is complete and its official adoption is decided.
- The 3D prototype is discussed again after `feature/ci` is on GitHub.

**Built**
- Deleted `rcssmonitor.pro`, `src/src.pro`, and `src/rcsslogplayer/` (an old copy of the rcsslogplayer parser that only the qmake build used; CMake and autotools use `rcss/rcg/`).

**Tests**
- On a copy of the working tree after the deletion, the CMake build and ctest, and the autotools build, passed.
- The local remote uses SSH (`git@github.com:`), so a push that contains workflow files does not need the token's `workflow` scope.

#### 2026-11-11: The first CI run (5fa9729) failed, and the fixes

**CI results**
- CI: both CMake and autotools stopped compiling the bundled simdjson 3.8.0. The runner's GCC 15.2.0 rejects three places in `base_formatter`, such as `this->print_newline()`, with `-Wtemplate-body` (the class has no such functions). This is not from these changes: develop itself does not build with GCC 15 (so not on newer distributions).
- AppImage: building and bundling passed (26 MB). The start check failed on all three releases because `libharfbuzz.so.0` was missing. linuxdeploy does not bundle harfbuzz and the like, so that the user's system copy is used, and the minimal containers did not have it (not a defect of the AppImage).

**Fixed**
- Changed the three places in `rcss/rcg/simdjson/simdjson.h` to the form upstream used to fix them in 3.10.0 (`static_cast<formatter*>(this)->print_newline()` etc.), with a comment saying the fix comes from upstream. There are no other places with the same error. Whether to update to a newer release is §7 item 11.
- Made the packages installed for the start check in `appimage.yml` match what the AppImage needs from the system. Downloaded and unpacked the CI's AppImage, and listed the libraries that are not bundled from the NEEDED entries of its ELF files: besides basic ones such as libc, libGL, libEGL, fontconfig, freetype, harfbuzz, libX11, libX11-xcb, libxcb, libICE, libSM, libcom_err, libgpg-error, and zlib. The xcb extensions, xkbcommon, glib, and dbus are in the AppImage. All the package names exist in Ubuntu 26.04 (checked on packages.ubuntu.com).

**Tests**
- Locally (GCC 13), the build and ctest passed. GCC 15 is not available locally, so the next CI run checks it. The first CI run stopped at `rcss/rcg`, and `src/` has not been compiled with GCC 15 yet; the next run may show GCC 15 errors in `src/`.
- The unpacked AppImage has only xcb as Qt's platform plugin (it cannot start with `offscreen`; the start check uses xcb under `xvfb-run`).

#### 2026-11-11: The second CI run (1fe26c6) passed

**CI results**
- CI: the CMake (GCC 15.2.0) build and ctest (`rcg_parser` passes), and the autotools build, passed. `src/` also compiles with GCC 15 without errors. The only warnings are 9 of `-Wtemplate-id-cdtor` (a form not allowed in C++20) in the bundled `simdjson.cpp`.
- CMake warns about `cmake_minimum_required(VERSION 3.5)` that "compatibility with CMake < 3.10 will be removed". Raise it to 3.16 in Phase 3 (§8 item 10).
- AppImage: 27 MB. In `ubuntu:22.04`, `24.04`, and `26.04` containers, running `--version` under `xvfb-run` printed `rcssmonitor 19.0.1` and exited on all three (the trial of option A of D10 succeeded).

**Decisions (user)**
- Changed who does the git operations: Claude does the commits and pushes, after the user has checked the proposed commit message (§0.1). The user still creates the branches on GitHub and does the PRs and merges.
