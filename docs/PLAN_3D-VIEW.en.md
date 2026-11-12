# rcssmonitor 3D view and Qt6 support: development plan

| Item | Content |
|---|---|
| Created | 2026-11-11 |
| Target | rcssmonitor. The work is based on `develop` (01f880a, 19.0.1). It does not include the TCP / WebSocket support (`feature/support-tcp-ws`) (D7) |
| Reference implementation | rcssmonitor-web 20.0.0-beta.2 (tag `v20.0.0-beta.2`, ed36941). Its working tree is `../rcssmonitor-web`, next to this repository |
| Server referred to | The asio version of rcssserver 19.x. Its working tree is `../rcssserver` (branch `feature/asio-migration`), next to this repository |
| Japanese version | [PLAN_3D-VIEW.md](PLAN_3D-VIEW.md) (same content; §0.1) |
| Status | Phase 4 done (2026-11-12, `feature/3d-view`; steps 4-1 to 4-5, §14). Next: Phase 5 |

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
- Merges suited to a PR (such as merging into master) are done by the user. When a PR is made, Claude proposes its title and description (in English), with the recommended merge method (a merge commit by default, or a squash merge with the reason when that fits) (from 2026-11-11; §9.1).
- When running the program to try it, do not modify the user's settings file `~/.rcssmonitor.conf`. Start it with `HOME` set to a temporary directory (e.g. `HOME=$(mktemp -d) ./src/rcssmonitor`). Do the same even for a run with only `--version`, which also writes the settings file on exit (found on 2026-11-11; §14). For the unit tests, CTest sets `HOME` in the build tree (§10.1).
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

State on 2026-11-11: the user decided D7, D10, and D11 ("Decided" in the table; D11 is a problem found in Phase 0). D1 is decided by the result of the Phase 0 prototype. D2–D6, D8, and D9 proceed with the recommendations. 2026-11-12: D1 was decided as the recommendation by the result of the prototype (§14).

| # | Item | Recommendation | Alternatives | Reason |
|---|---|---|---|---|
| D1 | 3D rendering technology | **Decided**: `QOpenGLWidget` with our own rendering (OpenGL 3.3 core, our own shaders; 2026-11-12, by the result of the prototype; §14) | Qt Quick 3D (embedded with `QQuickWidget`) | §2.3 below |
| D2 | Supported Qt versions | Both Qt 5.15 and Qt 6.2 or later (one source, switched) | Qt6 only | Ubuntu 22.04 has both Qt 5.15 and Qt 6.2. After a period of supporting both, Qt5 can be dropped in the next version |
| D3 | Build systems | CMake is the main one. Qt6 is supported by CMake only; autotools stays on Qt5 (3D is built by CMake only). qmake (`.pro`) is dropped by D11 | Make all three support Qt6 / drop autotools and qmake | Maintaining three is a lot of work. autotools' `AX_HAVE_QT` finds Qt through qmake, which makes Qt6 support hard |
| D4 | Is 3D optional in the build | CMake option `ENABLE_3D` (default ON). Where OpenGL 3.3 is unavailable, 3D is disabled at run time and the program works in 2D only | Make it mandatory | Keep 2D monitoring usable as before on old GPUs and virtual machines |
| D5 | glTF loading | cgltf (single header, MIT) plus our own animation evaluation | tinygltf (MIT), Assimp (BSD, large) | As in §5.8, the pose is computed from the log every time, so no animation player is needed; reading the keyframes is enough |
| D6 | Figure model | Re-export the web version's `Mannequin.glb` (CC0) without compression and put it in the Qt resources (qrc) | Bundle the meshoptimizer decoder (MIT) and read the web version's file as is | The web version's file requires `EXT_meshopt_compression` and `KHR_mesh_quantization`. For a desktop app, fewer dependencies matter more than size |
| D7 | Base of the work | **Decided**: derive from `develop`. Leave `feature/support-tcp-ws` unmerged, and merge it once it is adopted (§3.5) | (Original recommendation) merge `feature/support-tcp-ws` first and branch from there / branch directly from `feature/support-tcp-ws` | Whether to adopt the TCP / WebSocket support officially has not been decided (user). The 3D view diff can also be reviewed separately |
| D8 | Version | 20.0.0 (aligned with asio rcssserver 20 and rcssmonitor-web 20) | 19.1.0 etc. | The Qt6 support and the 3D view are big changes |
| D9 | Branch policy | Same as the web version (master: releases, develop: integration, feature/*: work, hotfix/*). Merges use a merge commit by default (decided by the user on 2026-11-11; squash merges were also used before). When a squash merge fits, and the per-phase branches, in §9.1 | Follow rcssmonitor's current practice | The web version's `docs/release.md` |
| D10 | master's AppImage workflow (§3.2) | **Decided**: drop the AppImage (option B of §2.4, 2026-11-11). The current workflow is a maintenance burden (the user disabled it in the GitHub UI), and with the browser version of the monitor, the AppImage is less needed (user). Option A was tried and worked, but is not adopted | Keep it as is (the code would have to stay buildable with Qt 5.12, which conflicts with D2) | It requires maintaining an Ubuntu 20.04 (end of support) Docker image, `qt5-default`, and custom shell and Python scripts. It builds with autotools, so 3D would not be included (D3) |
| D11 | The qmake build (§3.2) | **Decided**: drop it. Delete `rcssmonitor.pro`, `src/src.pro`, and `src/rcsslogplayer/`, an old copy of a parser that only the qmake build used (`feature/ci`). Mention it in NEWS (Phase 9). autotools uses qmake to find Qt, so the `qt5-qmake` package is still needed | Fix it (one definition, the version, boost) and add it to CI | No machine will use qmake any more (user). It was already broken on develop, and nobody had noticed. CMake and autotools are enough |

### 2.3 Comparison for D1 (3D rendering technology)

| Option | Pros | Cons |
|---|---|---|
| **QOpenGLWidget with our own rendering** (recommended) | The same code works on Qt5 and Qt6. No QML is brought in (rcssmonitor is a QWidget-only app). Everything done in the web version (instanced drawing, shadow maps, custom shaders, deciding skeleton poses from the log every time, the upper limit on the jersey number size, managing state per draw) stays under our control. The only dependencies are Qt's OpenGL module and cgltf | A lot of rendering infrastructure to write (about 10 shaders, shadow map, skinned meshes, lines, billboards). OpenGL is deprecated on macOS (works up to 4.1 core) |
| Qt Quick 3D | Materials, shadows, fog, skinned meshes, instancing, and picking are all there | Requires Qt6 (skeletal animation and instancing are Qt 6 features). The scene is written in QML, and C++ passes data to it. For skinned glTF, the documented path is build-time conversion (Balsam); whether run-time loading (RuntimeLoader) handles skeletal animation has not been confirmed. Whether moving the bones ourselves every frame (§5.8) works naturally also needs testing |
| Qt 3D | Has a C++ API | Deprecated in Qt 6.8 and removed from the standard release. Not for new use |

Decide after confirming with the recommended option, in the Phase 0 prototype (§9), "22 figures with shadows at 1920×1080 at 60 fps". → On 2026-11-12, both options were prototyped and measured, and the recommendation was decided (§14).

### 2.4 Replacement options for D10 (AppImage)

Option B (drop it) was decided on 2026-11-11. Option A was tried in Phase 0 and worked (§14), but is not adopted. How to build it is kept in `.github/workflows/appimage.yml` of commit 1fe26c6, in the text below, and in §14.

| Option | Content | What to maintain |
|---|---|---|
| A. Build with CMake and linuxdeploy in a tag-triggered `release.yml` (tried and worked, but not adopted) | Same shape as the web version's `release.yml`. Runs only when a tag (`rcssmonitor-X.Y.Z`) is pushed or when run by hand. Builds with CMake in `container: ubuntu:22.04` (Qt 5.15.3 / Qt 6.2.4, glibc 2.35) on the runner, installs into `DESTDIR`, bundles an AppImage with linuxdeploy and linuxdeploy-plugin-qt, and attaches it to the release with `gh release`. The desktop file and the icon are installed by CMake's `install()` | One workflow (about 50 lines). Raise the base version when an Ubuntu LTS ends (22.04 → 24.04). Raise the linuxdeploy version (pinned) |
| **B. Drop the AppImage** (decided) | Delete the workflow and `utils/appimage/`. Distribution is the source and the per-distribution build instructions (README) only. For only viewing logs, point to the browser version | Nothing |

- Why option B (2026-11-11): (1) rcssserver is released as source only, so people who use the monitor live already have an environment to build the server. (2) For only viewing logs, the browser version, which needs no installation, is enough. (3) Standard support for 22.04 ends in April 2027, and with Qt6 and 3D (OpenGL), checking the combination with the user's graphics drivers adds work. On the other hand, the 19.0.1 downloads were 257 for the AppImage and 1,154 for the source (2026-11-11), so the AppImage had users. The release notes and the README point to building from source and to the browser version. Note that the browser version's live connection needs a server with WebSocket support (asio rcssserver 20); connecting to the current rcssserver 19 (UDP only) needs the native version.
- With either option, disable the current `main.yml` (the user disabled it on 2026-11-11). It runs on pushes to any branch and tag, and creates a release when there is none for the version in `CMakeLists.txt`. So merely raising the version on a work branch creates a release. Disabling it in the GitHub UI (Actions → ci → Disable workflow) stops it without a commit, and it can be undone. Delete the files when merging 20.0.0 into master (or replace them, with option A).
- Try option A while building the CI in Phase 0. A manually triggered workflow (`workflow_dispatch`) cannot be run unless the same file is on the default branch (master), so during the trial it runs on pushes to `feature/ci`; when adopted, it runs on tag pushes and by hand. Decide on adoption after confirming that the resulting AppImage starts on other environments (Ubuntu 22.04, 24.04, 26.04). Whether an AppImage with 3D works with the user's OpenGL (linuxdeploy does not bundle libGL) is checked in Phase 4 or later.
- The base is 22.04 because an AppImage does not run on a glibc older than the one it was built with. The Qt of 22.04 (5.15.3, 6.2.4) equals the lower bounds of D2, so this also checks that the code builds with the lowest versions.
- The current workflow also uses linuxdeploy (the unpinned `continuous` release). What option A changes: CMake instead of autotools, `container: ubuntu:22.04` instead of a custom Dockerfile, running on tags only instead of on every push, and a pinned linuxdeploy version.
- master's `utils/appimage/rcssmonitor.png` is actually a 512×512 JPEG (the RoboCup logo), so its extension is wrong. The trial uses `icons/rcssmonitor.png`, a PNG made from the app's window icon `icons/rcss.xpm` (48×48).

## 3. Current rcssmonitor

### 3.1 Structure

| File | Role |
|---|---|
| `src/main_window.cpp` | Menus, toolbar, keys (`createActions*`), referee operations. `createFieldCanvas()` makes `FieldCanvas` the central widget |
| `src/field_canvas.cpp` | 2D drawing target. A `QWidget` (a `QGLWidget` if `USE_GLWIDGET` is defined; `USE_GLWIDGET` was removed in Phase 3). Calls the painters in order. Mouse operations, right-click menu |
| `src/field_painter.cpp`, `player_painter.cpp`, `ball_painter.cpp`, `draw_info_painter.cpp` | 2D drawing. Calculations such as player state colors, catch / tackle / kick areas, and the offside line are also here |
| `src/disp_holder.cpp` | Holds the received frames, for both live and logs |
| `src/log_player.cpp` | Advances one frame at a time with a `QTimer`. No interpolation between frames |
| `src/monitor_client.cpp` | Live connection. UDP only on develop (TCP / WebSocket are in `*_monitor_transport.cpp` on `feature/support-tcp-ws`) |
| `src/options.cpp` | Settings, saved to `~/.rcssmonitor.conf` with `QSettings`. The command line options are here too |
| `src/config_dialog.cpp` | Settings dialog (Show, Canvas, Color, Font tabs) |
| `src/player_type_dialog.cpp` | Player type list |
| `rcss/rcg/` | Parsing of logs and messages. `parser_simdjson.cpp` (JSON, uses the bundled simdjson), `parser_v1–v4.cpp`, `types.cpp` (parameter defaults), `util.cpp` (play mode names, etc.). `nlohmann/json.hpp` was also bundled (unused; deleted in Phase 2) |
| `test/` | Not on develop (no unit tests). `feature/support-tcp-ws` has the Qt Test `test_message_splitter.cpp`. This plan added `test_rcg_parser.cpp` and fixtures (`test/data/`) on `feature/ci` (§14) |

### 3.2 Build

- There are three: CMake (`CMakeLists.txt`, `src/CMakeLists.txt`), autotools (`configure.ac`, `m4/ax_have_qt.m4`), and qmake (`rcssmonitor.pro`). qmake was dropped by D11 (`feature/ci`).
- All are hard-wired to Qt5 (`find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)`). `feature/support-tcp-ws` optionally looks for WebSockets and Test.
- C++17, CMake 3.5 or later.
- The qmake build fails on develop (checked on 2026-11-11). `src/src.pro` lacks the `HAVE_ARPA_INET_H` definition, so it stops at `ntohs` in `disp_holder.cpp`. Its version is still 16.0.0, and it links the unused boost. Dropped by D11. The CMake and autotools builds pass.
- develop has no CI (no `.github/workflows`).
- Only master has the AppImage workflow of PR #59 (`.github/workflows/main.yml`, named `ci`) and `utils/appimage/`. It runs on pushes to any branch and tag, builds with autotools in an Ubuntu 20.04 Docker image (Qt 5.12.8 from `qt5-default`), and creates a release when there is none for the version in `CMakeLists.txt`. It does not check the build or run tests. D10 decided to drop it, and the user disabled it in the GitHub UI on 2026-11-11. The files are deleted when 20.0.0 is merged into master.

### 3.3 Places that change for Qt6 (counted on 2026-11-11 in `src/` and `rcss/rcg/`, excluding moc output and bundled libraries. Recounting on develop gives the same numbers. `feature/support-tcp-ws` has one more `QT_VERSION_CHECK`)

| Content | Count | How to write it for Qt6 |
|---|---|---|
| `QKeySequence` built with `Qt::CTRL + Qt::Key_X` / `Qt::ALT + …` | 19 + 5 | `Qt::CTRL \| Qt::Key_X` (`+` is deprecated in Qt 6; `\|` also works in Qt 5.15) |
| `QLayout::setMargin()` | 34 | `setContentsMargins()` (removed in Qt6) |
| `QMouseEvent::pos()`, `globalPos()` | 13, 1 | In Qt6, `position().toPoint()`, `globalPosition().toPoint()` (`pos()` is deprecated in Qt 6). `position()` does not exist in Qt 5.15, so for dual support a small function switches with `QT_VERSION_CHECK`. → Correction (Phase 3): `QMouseEvent::pos()` is not deprecated in Qt 6.2–6.10 (deprecated are `globalPos()`, `x()`, `y()`, and `pos()` of the other events). The code uses only `QMouseEvent::pos()` and `QContextMenuEvent::globalPos()`, so nothing needs to change |
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
- `proto/3d-gl`: the 3D prototype (not merged). Created from develop (95965dc) on 2026-11-12 (§14).
- `fix/qt-selection`: the fix of how CMake selects Qt, and the record of the prototype. Created from develop (95965dc) on 2026-11-12 (§14).
- `origin/feature/json-reformat` (2024-03): 2 commits that adapt the old parser `rcss/rcg/parser_json.cpp` and `parser.cpp` to the new JSON format. The current develop uses `parser_simdjson.cpp`, so check the content and decide whether to take it into the §7 work or discard it. → Not taken (Phase 2, §14).
- `origin/develop_observation` (2022-11) and `origin/fix/qt-asprintf` (merged into master) are unrelated to this plan.

### 3.5 When merging the TCP / WebSocket support later

As in D7, the branches of this plan do not include `feature/support-tcp-ws`. When it is adopted and merged, the following happens.

- Files changed by both: `src/main_window.cpp`, `src/options.{h,cpp}`, `CMakeLists.txt`, `src/CMakeLists.txt`, `configure.ac`, `src/Makefile.am`, `src/src.pro`, `README.md`, `NEWS`, `test/CMakeLists.txt` (both create it). The features are separate, so resolving the conflicts is expected to be easy.
- This plan sets up the tests in the same shape as `feature/support-tcp-ws` (the top-level `CMakeLists.txt` looks for Test and, if found, calls `enable_testing()` and `add_subdirectory(test)`; `test/CMakeLists.txt` has an `add_executable` and an `add_test` per test). When merging, putting both sets of tests side by side should be enough. Since Phase 1, `test/CMakeLists.txt` adds tests with the function `rcssmonitor_add_test`, so add `message_splitter` with it too.
- In Phase 1, the application sources (all but `main.cpp`) were gathered into the library `rcssmonitor_core` in `src/CMakeLists.txt`. Add the sources of the TCP / WebSocket support to `rcssmonitor_core`, not to `add_executable`.
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

The structure of the rendering infrastructure (4-3b, §14).

- **Policy (user, 2026-11-12)**: OpenGL is hidden in the parts in `src/view3d/gl/`. Only the code in `gl/` uses the OpenGL functions (`glXxx()`) and constants (`GL_XXX`) directly; the usual code (`Scene3D`, `FieldView3D` etc.) uses the parts in `gl/`. `Scene3D` only lists which shapes to draw with which shader and in which way. When a new way of drawing is needed, add a part to `gl/`, or a function to a part.
- Test `gl_isolation` reads the C++ sources in `src/` (except `src/view3d/gl/`) without their comments and strings, looks for calls of OpenGL functions and OpenGL constants, and fails if there are any. Qt's types and functions (`GLuint`, `QOpenGLWidget`, `initializeOpenGLFunctions()`) are fine. The test code is not checked (it uses OpenGL directly to set up offscreen drawing).

| Part | Role |
|---|---|
| `shaders/*.vert`, `*.frag` (`view3d_shaders.qrc`) | The GLSL sources. `ShaderProgram` adds the `#version` line and the attribute locations (`A_POSITION` etc.) at the top |
| `gl/attributes.h` | The locations of the vertex attributes, shared by the vertex buffers and the shaders |
| `gl/shader_program` | Loads and links the shaders, and looks up the location of each uniform once and remembers it (`set( "uViewProj", m )`) |
| `gl/instanced_mesh` | The vertex buffer of a shape and the buffer of the instances (matrix and color), all drawn with one draw call. The instance buffer is made again only when it grows |
| `gl/texture_2d` | A texture of an image (mipmaps, anisotropic filtering) |
| `gl/billboard_mesh` | Instanced drawing of rectangles that face the camera (billboards): the numbers and the cards (4-5) |
| `gl/render_state` | How to draw (depth test and write, culling back faces, translucency). Every pass applies its whole state and does not depend on what the pass before left |
| `gl/frame` | The start and the end of a frame (`GLFrame::begin()`, `end()`): the framebuffer to draw into and clearing it, resetting the state that QPainter (the HUD) may leave, and the state handed to QPainter (4-5) |
| `meshes_3d` | The vertices of the shapes (GL-independent; unit tests check the winding of the triangles and the normals) |

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

- Mouse: in the free camera, left-drag orbits, Shift (or Ctrl) + left-drag pans, the wheel and the middle button zoom (the web version uses three.js OrbitControls; write the same operations ourselves). Dragging in any other camera switches to free. In the close-up cameras, the wheel changes the distance, which is kept in the settings. Right-click is the referee menu. → 4-4: in 2D the wheel steps the frames, so a 3D camera that does not take the wheel (the top camera) steps the frames as before.
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
| `view_3d` | off | Start in 3D (on the command line `--view-3d`, as the other options; 4-1 of Phase 4) |
| `camera_3d` | broadcast | free / broadcast / closeup-ball / closeup-player / top / goal-left / goal-right / player (`--camera-3d` on the command line; in 4-4, the five without the close-ups and the player camera; an unknown name is not read) |
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
| B1 | The live display stops after 65,535 frames | `src/disp_holder.cpp:65` (`MAX_SIZE = 65535`), `:249` | Do not stop accepting at the limit. Either raise the limit or drop the oldest frames (decide during implementation and record it in §14). The web version has no limit. → A live connection drops the oldest 4,096 frames at once when it has more than 65,535. A log file is kept whole (Phase 1, §14) |
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
- Phase 1 (2026-11-11, `fix/parity-bugs`) fixed B1, B5, and B9–B14. All but B10 have tests (§14).
- Phase 2 (2026-11-12, `feature/json-parser`) fixed B2–B4 and B6–B8, all with tests (§14).
- If merging into master is urgent, the bug fixes alone can go first as a separate PR.

## 7. JSON parser improvements

The targets are `rcss/rcg/parser_simdjson.cpp` (logs) and the live JSON messages (the same parser, used from `disp_holder.cpp`). Align with the web version's `src/io/rcg-json.ts` and `docs/protocol.md` §3 and §7.

1. Skip unknown keys (B3), so that logs can still be read when the server adds fields in the future. → Done in B-1 of Phase 2 (§14).
2. Store `vq` as `'h'` / `'l'` (B2). → Done in B-2.
3. Store players by `side` and `unum` (B4), so that missing array elements and a different order are tolerated. → Done in B-3.
4. Read the log as one record per line. When the server terminates abnormally the final `]` is missing, so skip the `[` and `]` lines and strip the trailing `,`. If a line is broken, skip only that line and warn. → Done in A4 of Phase 2 (§14).
5. Accept fields that may be omitted: `stime` (only when greater than 0), `px` / `py` (only while pointing), `fside` / `fnum` (only with an attentionto), `pen_score` / `pen_miss` (only for teams that took penalty kicks), `null` team names (before connection), `capacity`. → Done in B-4.
6. Read the ball height `ball.z` and the vertical velocity `ball.vz` if present (for the 2.5D simulator; units m and m/cycle, up is positive. The key names are tentative and will follow the server format once it is decided). Add them to `BallT` in `types.h`. → Done in B-5.
7. Handle `timestamp` (`%FT%T%z`, offset without a colon) without breaking it. The current parser skips `version` and `timestamp` and does not pass them to the handler (`parseVersion` and `parseTimestamp` are empty; checked on 2026-11-11). Pass both to the handler (`handleServerVersion`, `handleTimestamp`). → Done in B-6.
8. Measure the speed: compare the load time of a full-game JSON log (`1-json.rcg.gz` placed at the top of the working tree, 6,209 frames; not in the repository) before and after the improvements, and record it in §14. The web version takes about 0.3–0.5 s in a browser. → Measured in Phase 2 (§14).
9. Check the content of `origin/feature/json-reformat` (the old parser's support for the new format) and take anything that does not overlap with the above. → There was nothing to take (§14).
10. Also fix the text rcg bugs B6 and B8 at this stage. → Done in B-7 and B-8. B7 (the parameter defaults) is B-9.
11. Decide whether to update the bundled simdjson (3.8.0, with three lines of the 3.10.0 fix applied so that it builds with GCC 15; §14) to a newer release (the latest in 2026-11 is 5.1.1). → Updated to 5.1.1 (A2 of Phase 2, §14).

Tests: with fixtures (a short excerpt of `1-json.rcg.gz`, broken lines, unknown keys, swapped player order, a missing final `]`), check the number of frames, the values in specific frames, and the sequence of play modes. Models: the web version's `test/unit/rcg-json.test.ts`, `rcg-text.test.ts`, and `full-logs.test.ts`.

## 8. Qt6 support

1. CMake: select the version with `find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)` and link with `Qt${QT_VERSION_MAJOR}::Widgets` etc. Test in the same way (WebSockets when merging, per §3.5). For Qt6, add `OpenGL` and `OpenGLWidgets` (for 3D). In Qt5, `QOpenGLWidget` is in Widgets. → Done in Phase 3 (OpenGL is added in Phase 4; §14).
2. Make the replacements in §3.3, choosing forms that also work on Qt 5.15 (dual support, D2). → Done in Phase 3.
3. Remove `QGLWidget` under `USE_GLWIDGET` if it is unused. If it is used, make it `QOpenGLWidget`. → Removed.
4. Remove the `QT_VERSION_CHECK` branches for versions older than Qt 5.15. → Done.
5. Eliminate deprecation warnings: build with `QT_DISABLE_DEPRECATED_BEFORE=0x050F00` on Qt5, and confirm that there are no deprecation warnings on Qt6. → CMake always defines `QT_DISABLE_DEPRECATED_BEFORE` (0x050F00 for Qt5, 0x060000 for Qt6), and CI builds with `-Werror=deprecated-declarations`.
6. High DPI: enabled by default in Qt6. Confirm that the 2D drawing (line widths, text sizes) does not break at scale 1, 1.5, and 2. 3D renders according to `devicePixelRatio`. → Checked by drawing offscreen at scale 1, 1.5, and 2. Qt5 now scales by the screen's ratio as Qt6 does (§14).
7. autotools stays on Qt5 as in D3 (qmake is dropped by D11); write in the README "Qt6 and 3D with CMake". → The README says "autotools is for Qt5 only" (3D is written in Phase 4).
8. Build both Qt5 and Qt6 in CI (§10). → Done, also the oldest Qt 6.2 (an Ubuntu 22.04 container).
9. Rewrite the README installation instructions for both Qt5 and Qt6 on Ubuntu 22.04 / 24.04 / 26.04. → Done.
10. Raise `cmake_minimum_required(VERSION 3.5)`. The CMake in CI warns that "compatibility with CMake < 3.10 will be removed" (§14). Qt6 needs CMake 3.16 or later, so make it 3.16. → Done.

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
- PRs are merged with a merge commit by default (D9). The PR-level history is `git log --first-parent`, a whole PR is reverted with `git revert -m 1 <merge commit>`, and a search for the cause of a bug narrows down the PR with `git bisect start --first-parent` before going into its commits. Keep every commit buildable, and fold commits that were only tried and removed before pushing.
- A squash merge fits only a PR that meets all of the following, and Claude says so when proposing the PR: (1) a small change with one purpose, (2) its intermediate commits (fixes, trial and error) have no meaning of their own, (3) no other branch is stacked on it, (4) nothing such as the plan refers to its commit IDs. Merges from develop into master, the merge of `feature/3d-view`, and PRs with several independent changes use a merge commit.

### Phase 0: Foundation and validation

**Work**
- Confirm the decisions in §2.2 with the user.
- Create branches from develop, as in D7 (§9.1).
- Set up CI (GitHub Actions). On Ubuntu 26.04 (`runs-on: ubuntu-26.04`, as in the web version's CI), build with Qt5 and run the unit tests. The Qt6 job is added in Phase 3, when the Qt6 build passes. Use action versions that run on Node.js 24 (as in the web version: checkout@v5, upload-artifact@v7, etc.).
- Set up the test infrastructure (Qt Test, CTest) on develop, in the same shape as `feature/support-tcp-ws` (§3.5).
- Try option A of D10 (§2.4) (during the trial, it runs on pushes to `feature/ci`). → It was tried and worked, but D10 was decided as option B (drop it).
- 3D prototype (checking D1): with `QOpenGLWidget`, draw 22 skinned figures (walking with blended clips), a 4096 px shadow map, and instanced cylinders, and measure whether 60 fps holds at 1920×1080. Run it on both Qt 5.15 and Qt 6. Record the result in §14 and decide D1. → Done. Qt Quick 3D was also prototyped and compared (§14).
- Prepare the figure model as in D6. → Done (`resources/models/` of `proto/3d-gl`; §14).

**Done when**: everything in §2.2 is decided. CI passes. The prototype confirms 60 fps (if D1 is the recommendation). Whether to adopt option A of D10 is decided (decided: not adopted; option B). → Met (2026-11-12).

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
- Ground image, goals, players (cylinders: state colors, cards, direction markers, foot circles and direction lines, view cones), ball (height and shadow). → The cards are drawn in 4-5 with the number labels, and the direction lines in the foot circles and the view cones in Phase 5 with the drawing of lines (4-3, §14).
- Cameras: free, broadcast, top, and the two behind the goals. E key. Mouse operations. → Done in 4-4 (§14).
- HUD (score board, logos). → Done in 4-5 (§14).
- Jersey number labels (upper size limit, placement in the top camera). → Done in 4-5 (with the cards).

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

**Work**: README (3D, keys, settings, Qt6 build), ChangeLog and NEWS (including the removal of qmake, D11), the version per D8. Put the files that only CMake builds (`src/view3d/`, `test/`, `data/` etc.; missing as of 2026-11-12) into the tarball that autotools makes with `make dist`, or change how the tarball is made (the 19.0.1 tarball was made with `make dist`). Delete master's `main.yml` and `utils/appimage/` (D10; done in the merge into master). Write in NEWS and the README that the AppImage was dropped, how to build from source, and a pointer to the browser version. The user merges into master and tags the release.

**Done when**: it is released.

## 10. Test plan

### 10.1 Unit tests (Qt Test)

Port the GL-independent calculations, using the web version's unit tests as models.

- Tests are added with `rcssmonitor_add_test(<name> <libraries>...)` in `test/CMakeLists.txt` (it builds `test_<name>.cpp` and registers it as `<name>`). Tests that use the application's classes link `rcssmonitor_core`.
- `gl_isolation` checks that OpenGL is not used outside `src/view3d/gl/` (§5.3).
- CTest runs every test with `HOME` in the build tree (`test/home`) and `QT_QPA_PLATFORM=offscreen`. `Options` reads `~/.rcssmonitor.conf` when it is created (writing it if missing) and writes it on exit, so this keeps the user's settings file safe.

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
- The loading speed of logs is measured with `test/bench_load` (not run in CI; §14). In an environment with conda or the like in `PATH` or `LD_LIBRARY_PATH`, the build and the run pick up another Qt or zlib, so build and measure without them.
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
| `gl/*.{h,cpp}`, `shaders/*.vert`, `*.frag`, `meshes_3d.{h,cpp}` | Rendering infrastructure (shaders, buffers, textures, render states, shapes; §5.3; made in 4-3b; the shadow map in Phase 8) |
| `cameras_3d.{h,cpp}`, `player_camera_3d.{h,cpp}`, `sight.{h,cpp}`, `interpolate.{h,cpp}`, `scene_math.{h,cpp}` | Calculations (no GL dependency) |
| `overlay_math.{h,cpp}` | Overlay calculations shared by 2D and 3D (moved from the 2D painters). Created in `src/` in Phase 1, with the offside lines |
| `play_mode_position.{h,cpp}` | The ball position for Change Playmode (Phase 1; also used by the 3D referee operations) |
| `player_type_sort_proxy.h` | Sorting of the player type list (Phase 1) |
| `game_log_file.{h,cpp}` | Reading a log file into one buffer and parsing it. With libdeflate, gzip is decompressed with it (Phase 2) |
| `rcss/rcg/line_parser.{h,cpp}` | Parsing the lines of a log in parallel and passing them to the handler in order (Phase 2) |
| `test/bench_load.cpp` | Measuring the loading speed (Phase 2; not run in CI) |
| `stadium_3d`, `scenery_3d`, `ball_3d`, `ball_aids_3d` | Components |
| `figures_3d`, `pose`, `gltf_model` | Figures |
| `hud_3d`, `guides` | HUD, compass strip, minimap |
| The 3D tab and the Player Camera tab of `config_dialog` | Settings |
| `third_party/cgltf/cgltf.h` | glTF loading (MIT). v1.15 is in the prototype branch (§14) |
| `resources/models/Mannequin.glb`, `.qrc` | Figure model (CC0) and its source. The one without compression is in the prototype branch (§14) |
| `test/test_*.cpp` | Unit tests and rendering tests |
| `.github/workflows/ci.yml` | CI |
| `data/rcssmonitor.desktop`, `icons/rcssmonitor.png` | The desktop file and the icon, installed by CMake's `install()` (added in Phase 0 for the AppImage trial; kept after dropping the AppImage, because a normal installation also shows the app in the application menu) |

**Change**: `main_window.cpp` (switching, menus, toolbar, keys), `field_canvas.cpp` (sharing the right-click menu), `log_player.cpp` (alpha), `options.{h,cpp}` (settings), `config_dialog.cpp`, `player_painter.cpp`, `ball_painter.cpp`, and `field_painter.cpp` (extracting the calculations, the grass stripe argument), `rcss/rcg/*` (§6, §7), `CMakeLists.txt` and `src/CMakeLists.txt` (Qt6, 3D), README.

**Delete**: `rcssmonitor.pro`, `src/src.pro`, `src/rcsslogplayer/` (D11; deleted in Phase 0). The AppImage trial's `.github/workflows/appimage.yml` (D10; deleted in Phase 0). master's `.github/workflows/main.yml` and `utils/appimage/` (D10; deleted when 20.0.0 is merged into master).

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
- Work environment: Ubuntu 24.04, CMake 3.28, Qt 5.15.13 and Qt 6.4.2 (Core, Gui, Widgets, Network, OpenGL, OpenGLWidgets, and Test are available; Qt6 WebSockets is not). → Correction (Phase 3): only the runtime libraries of Qt6 were installed, without the development packages. In Phase 3 the user installed `qt6-base-dev` and `libqt6opengl6-dev`. GPU: AMD (PCI ID 13c0).
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

#### 2026-11-11: Dropping the AppImage (D10)

**Decisions (user)**
- D10: drop the AppImage (option B of §2.4). With the browser version of the monitor, the AppImage is less needed. The user disabled master's old workflow (`ci`) in the GitHub UI (confirmed as `disabled_manually` with `gh workflow list`).

**Checked**
- Downloads of the 19.0.1 release: AppImage 257, source (`rcssmonitor-19.0.1.tar.gz`) 1,154. 19.0.0 and 18.0.0 have the source only (142, 852).
- The rcssserver releases (19.0.0, 18.1.x) are source only, with no AppImage.
- rcssmonitor-web connects live over WebSocket only (README), which needs asio rcssserver (20).

**Built**
- Deleted the AppImage trial's `.github/workflows/appimage.yml` (the working version stays in commit 1fe26c6).
- Kept the desktop file and the icon (`data/rcssmonitor.desktop`, `icons/rcssmonitor.png`) and their `install()`.

#### 2026-11-11: How PRs are merged

**Decisions (user)**
- When a PR is made, Claude proposes its title and description (§0.1).
- PRs are merged with a merge commit by default (D9). Claude says when a squash merge fits a PR, when proposing it (the conditions are in §9.1).

**Checked**
- Past merges: into develop, merge commits (#60, #57, #53, etc.) and squash merges (#51, #54, #46) are mixed. From develop into master, merge commits (#61, #58, #55, etc.). The repository settings allow merge commits, squash, and rebase.
- A merge commit was recommended for the `feature/ci` PR, because it has independent changes (the GCC 15 build fix, dropping qmake, etc.) and the plan refers to commit 1fe26c6.

### Phase 1

#### 2026-11-11: Bug fixes (`fix/parity-bugs`)

After PR #63 (`feature/ci`) was merged into develop with a merge commit, the user created `fix/parity-bugs` on GitHub.

**Decisions**
- B1: when the frames of a live connection exceed 65,535 (`MAX_LIVE_FRAMES`, the same number as before), the oldest 4,096 frames (`LIVE_TRIM_FRAMES`, about 7 minutes) are dropped at once. A log file is kept whole, as the file bounds its size. It is not made configurable (user). The first version removed the limit, which was revised after the user pointed out that rcssserver can keep a game running as long as its time fits in 32 bits, so keeping everything in memory is not realistic. A frame is about 2.3 KB (`sizeof(DispInfoT)` = 2312): 65,535 frames are about 160 MB. The frames dropped are at the front of a `vector` of pointers, so dropping them at once has no performance problem. The care is in the indices: the current position (`M_current_index`; if the frame being shown was dropped, it moves to the oldest frame left) and the score positions (`M_score_changed_index`) shift by the number dropped. The draw data kept by time (points, circles, lines) before the oldest frame left are dropped too. Live frames come only through the live entry points of `DispHolder` (`addJSON`, `addDispInfoV1`–`V3`), which drop them there (`trimLiveFrames()`).
- For the tests, the application sources (all but `main.cpp`) are gathered into the CMake static library `rcssmonitor_core`, which both the application and the tests link.
- One commit per bug.

**Built (one commit each, in order)**
1. `rcssmonitor_core` and the function `rcssmonitor_add_test` that adds a test (it sets `HOME` and `QT_QPA_PLATFORM`; §10.1).
2. B5: `foul_push_r` in `util.cpp`. Test `rcg_util`: every name in `PLAYMODE_STRINGS` converts back to its play mode.
3. B1: a live connection drops the oldest frames. Test `disp_holder`: 65,536 frames through the live entry point leave 61,439, and the newest frame can be shown (the same after the same number again). The frame being shown keeps its position, and moves to the oldest frame if it was dropped. The score positions shift, and those dropped are removed. Old draw data are dropped. A log (`handleShow`) keeps all of 65,537 frames.
4. B9: moved the offside line calculation to `src/overlay_math.{h,cpp}` (`OverlayMath::offsideLines`), comparing with `side()` instead of `side_` (a character). Test `overlay_math`: the second last player, the ball nearer, fewer than two players in their own half, the pitch end, players not connected.
5. B10: the goalie's catch area color (two places, the same cause as B9). No test (it only chooses a drawing color).
6. B11: moved the ball position of each play mode to `src/play_mode_position.{h,cpp}` (`playModeBallPosition`), and `MainWindow::changePlayMode` sends both the ball move and the mode change for every mode it accepts. Test `play_mode_position`: the back_pass positions, the other modes' positions (as before), a point outside the pitch, a mode not offered.
7. B12: `ConfigDialog::clickSelectFix` sets `SELECT_FIX`. Test `config_dialog`: calls the slot and checks (the slot is called by name, as there are two "Fix" buttons, for the view and for the player selection).
8. B13: the initial value of Illegal Defense in the color list. Test `config_dialog`: the item's color is the illegal defense color (not the pointto color).
9. B14: moved the sort class to `src/player_type_sort_proxy.h`, comparing the first number in a cell (as the web version's `parseFloat`). Test `player_type_sort`: a column with two numbers, a column with one, how the first number is read.

**Tests (local, GCC 13, Qt 5.15.13)**
- The tests of B5, B1, B9, B12, B13, and B14 were confirmed to fail with the code temporarily put back as it was before the fix, and to pass with the fix. For B1, two tests were also confirmed to fail when only the index shifting was temporarily disabled. B11's original defect was "sends nothing", which a test of the function cannot check directly. The positions are the same as before for every mode, and every accepted mode now always sends both commands.
- Every stage 1–9 builds with CMake and passes its tests on a clean copy (1 to 7 tests per stage). The stages that add files for autotools (4, 6, 9) also build with autotools. No new warnings.

**Found**
- `DispHolder::handleTeam` records `M_disp_cont.size() - 1` as the score position when the score changes. With no frame yet (connecting in the middle of a game, with the team information before the first show), it is `size_t(-1)`. `trimLiveFrames()` removes positions not below the number before the drop, so this one is removed too.
- `player_painter.cpp:610` also has `side_ == LEFT`, but it is inside `#if 0` and unused, so it was not changed.
- `PLAYMODE_STRINGS` has two empty strings after the last play mode: 54 entries, two more than `PM_MAX` (52).
- `Options` is a static variable in `instance()`; it reads `~/.rcssmonitor.conf` when created (writing it if missing) and writes it when the program exits (`writeSettings(false)`: all but the window size). It writes it even when exiting after `--version`.
- On 2026-11-11 at 15:27, the installed `rcssmonitor --version` (run to check the AppImage) was run without setting `HOME` to a temporary directory, and it rewrote the user's `~/.rcssmonitor.conf` (against §0.1). It writes back the values it read, so the content is expected to be unchanged (items missing from the file may have been added with defaults, and the order may have changed). This was reported to the user, and §0.1 now says "the same for `--version`".

#### 2026-11-12: The user's checks and the merge

- PR #64 (`fix/parity-bugs`) was merged into develop with a merge commit (c269eee). CI on develop passed too.
- The user checked with real logs and live: B9, B10, B11, B12, B13, and B14 are all OK. B1 was checked by running fast with synch_mode: past about 65,535 frames, the oldest frames were dropped at once.
- The completion conditions of Phase 1 (a test per bug, CI, checks with real logs) are met.

### Phase 2

#### 2026-11-12: Faster loading (`feature/json-parser`, batch A)

**Decisions (user)**
- In Phase 2, besides improving the parsers, update simdjson and make loading faster still (user: it feels slower than the web version).
- Approach: (1) read the file into one buffer and parse from memory, (2) read JSON as one record per line, (3) parse the lines in parallel, (4) thin out the progress display, (5) update simdjson to 5.x. libdeflate (faster decompression) is decided after measuring with zlib. Loading in another thread (the window not freezing while loading) is left for later.
- The work is split into batch A (speed) and batch B (robustness: B2–B4, B6–B8, and the rest of §7), and the commit messages are confirmed per batch.

**Measured (before)**
- The app's `parsing elapsed` (decompression and parsing): offscreen, JSON about 378 ms and text about 222 ms. The user on a real screen: JSON 371 ms, text 220 ms.
- Breakdown (JSON 53.6 MB, 6,209 frames / text v5 28.7 MB, 8,406 frames): decompression through `gzifstream` 139 / 81 ms (65 / 46 ms with zlib's `gzread`, 1 MB at a time). The whole-text copy through `istreambuf_iterator` inside the JSON parser 99 ms. Parsing and storing about 100 / 80 ms. The progress display about 150 / 90 ms (updated every 32 cycles, and `processEvents()` every 512 cycles repaints the app's window).
- The web version parses in a Worker and shows the progress in the status bar (the page does not freeze). It does not show frames while loading.

**Built (one commit each, in order; times are the best of 5 runs of `bench_load`, without the progress display)**

| Stage | Content | JSON | Text |
|---|---|---|---|
| A1 | The benchmark `test/bench_load` (reads the same way as the app and prints the best time; `--progress` adds the progress dialog) | 235 ms | 130 ms |
| A2 | simdjson 5.1.1 (the GCC 15 fix put into 3.8.0 is upstream) | 222 ms | 130 ms |
| A3 | Read the file into one buffer with `gzread` (`src/game_log_file`). Added the in-memory entry points `Parser::create( data, size )` and `parse( data, size, handler )`. JSON makes only one copy, into simdjson | 179 ms | 121 ms |
| A4 | Parse JSON as one record per line, in place (no copy). The `[` and `]` lines and the trailing `,` are skipped, so a log without its last `]` is read. A line that cannot be parsed is skipped with a warning (§7 item 4) | 151 ms | — |
| A5 | Read the text format from memory line by line as well (without `std::getline`) | — | 122 ms |
| A6 | Parse the lines in parallel (`rcss/rcg/line_parser`): up to 8 threads, at least 1,024 lines per thread. Each thread passes its data to a recording handler, and afterwards the records go to the real handler in the order of the lines | 84 ms | 60 ms |
| A7 | Thin out the progress display (`RCGHandler`) to every 100 ms, and show it only when loading takes longer than 0.5 s | — | — |

**Result (the app's `parsing elapsed`, offscreen, 3 runs)**: JSON 275–365 ms → 85–118 ms, text 214–216 ms → 93–102 ms. The app reads only once, which is 30–35 ms slower than later reads (the first allocation of large memory, starting threads, etc.; `bench_load -n 1` gives 94–110 / 91–96 ms). Most of the remaining time is decompression (zlib, which cannot run in parallel).

**Tests**
- Test `rcg_parser`: the in-memory entry points (text, JSON), a missing last `]`, broken lines, JSON on one line (parsed as a whole document), CR LF, and large logs (5,000 frames; the parallel path) keeping the order of the shows and play modes, stopping at an illegal text line, and skipping a broken JSON line.
- Test `line_parser` (new): `split_lines`, the order of 20,000 lines, the numbers of the failed lines (including an exception), and not passing the later blocks when stopping at the first failure. Replaying the blocks in reverse was confirmed to fail.
- Test `game_log_file` (new): an uncompressed file, a gzip over 1 MB, a missing file, and reading and parsing a gzip log.
- All 9 tests pass. The A4 tests were confirmed to fail when going back to the whole-document reading of A3.

**Found**
- In the terminal of this work, conda (`anaconda3/envs/catboost`) is in `PATH` and `~/local/lib` etc. in `LD_LIBRARY_PATH`, and CMake picked up conda's Qt and zlib. The speed was measured again with a build and run without them (the system Qt 5.15.13 and zlib; §10.3). The values were about the same.
- `perf` cannot be used because of `perf_event_paranoid=4`. The time of each step was measured with the benchmark programs.
- In simdjson 5.1.1, `simdjson.h` is 10 MB (3.8 MB in 3.8.0). Compile times: `simdjson.cpp` 3.5 → 9.6 s, `parser_simdjson.cpp` 6.4 → 7.7 s. Not using the return value of `get_string()` now warns, which was fixed. The license differs only in the year.
- The app no longer uses `gzfstream` (`gzifstream`). The files are kept (whether to delete them is decided later). → Deleted later (below).
- The file path is passed with `QFile::encodeName()` instead of `toLatin1()` (paths with Japanese characters open too).
- The version in the text header is read with `std::atoi` instead of `std::stoi` (no exception for a broken header such as `ULGx`).
- As JSON is now read line by line, a record that cannot be parsed (including one with an unknown key; B3) skips only that line (before, the whole parse stopped). B3 will skip only the unknown keys.
- `Handler::handleTimestamp()` is not virtual, so the recording handler of the parallel path cannot record it. Fix it with §7 item 7 (passing the timestamp to the handler).
- In the parallel path, the return values of the handler's functions are not used (the app's handler always returns true).

#### 2026-11-12: libdeflate (`feature/json-parser`)

The 8 commits of batch A (d36e1a9 to 8b9d510) were pushed. CI passed both CMake and autotools. The warnings of the bundled simdjson (9 of `-Wtemplate-id-cdtor`) are gone with 5.1.1, and there are no compiler warnings. The CI run when the branch was created (c269eee) failed because the runner could not resolve `security.ubuntu.com` and could not install the packages (unrelated to the code).

**Decided (user)**
- Use libdeflate as an optional dependency to decompress faster (without it, zlib alone works).

**Measured (before)**: Decompression alone: zlib (`gzread`, 1 MB at a time) JSON 69 ms, text 48 ms. libdeflate (read the file, decompress at once) 34 ms, 22 ms (including reading, 7.5 ms and 6.5 ms).

**Made**
- `src/game_log_file.cpp`: With libdeflate, the file is read as it is and, if gzip, decompressed at once. The output size comes from the ISIZE at the end, and is doubled when it is not enough (up to the largest deflate ratio, 1032:1). Concatenated gzip members are read, and bytes that are not gzip after a member are ignored, as zlib does. Data that libdeflate cannot decompress (cut in the middle) are read again on the zlib path. zlib reads a gzip that is cut in the middle up to the cut (a log of a server that stopped abnormally; §7 item 4).
- CMake: `ENABLE_LIBDEFLATE` (default ON). If found, `HAVE_LIBDEFLATE` is defined (searched with `find_path` and `find_library`, because libdeflate on older distributions has no CMake config file). autotools: `--with-libdeflate` (by default it is used if found; `--without-libdeflate` does not use it; `--with-libdeflate` fails if it is not found). The library is added only to the app, with `LIBDEFLATE_LIBS` (in the global `LIBS`, it would also go into the test link of `AX_HAVE_QT`, which checks Qt).
- CI: the CMake job installs `libdeflate-dev` and the autotools job does not (both paths are compiled). README says it is an optional dependency.

**Results (the app's `parsing elapsed`, offscreen, 3 runs)**

| Build | JSON | Text |
|---|---|---|
| CMake, zlib only | 91–119 ms | 95–100 ms |
| CMake, libdeflate | 63–76 ms | 59–60 ms |
| autotools, zlib only | 117–119 ms | 76–106 ms |
| autotools, libdeflate | 76–77 ms | 59–64 ms |

`bench_load` (best of 5): zlib only 84–117 / 60 ms, libdeflate 53–74 / 33–42 ms.

**Tests**
- Added to test `game_log_file`: a gzip of several members (the last member is small, so the size from ISIZE is not enough and the growing path is taken), a gzip cut in the middle (read up to the cut), and extra bytes after the gzip.
- Checked on clean copies: CMake without and with libdeflate, and autotools in 3 cases (used if found, `--without-libdeflate`, and `--with-libdeflate` when it is missing). In both CMake builds, all 9 tests pass.
- As a failure falls back to zlib, passing tests alone do not show that the libdeflate path was taken. With a temporary print, it was confirmed that a plain gzip, several members, and extra bytes are read with libdeflate, and a cut one with zlib.
- `libdeflate-dev` is not installed on the work machine, so the package fetched with `apt-get download` was extracted into a temporary directory and used (the library is the system's `libdeflate.so.0`).

#### 2026-11-12: Making the parsers robust (`feature/json-parser`, batch B)

After pushing the 2 libdeflate commits (e0e21ef, 6328ac9), the rest of §7 and B2–B4, B6–B8 were fixed.

**Made (one commit each, in order)**

| Step | Content | Tests (`rcg_parser` etc.) |
|---|---|---|
| B-1 | JSON: skip unknown records and keys (B3). An unknown key is reported once in a process (on a live connection it is in every message). When reading a whole document, a record that cannot be read is also skipped and the parse goes on. The live entry point (`ParserSimdJSON::parseData`) catches simdjson's exceptions (before, it did not, and a broken message could crash the app; protocol.md §3.6: the server may send invalid JSON, e.g. with a newline in a string) | an unknown record, an unknown key and a known key in a record, unknown keys in a show and in a player (one record per line and whole document), a record that cannot be read in a whole document, broken live messages |
| B-2 | JSON: store `vq` as `'h'` / `'l'` (B2) | a player with `vq` `"l"` |
| B-3 | JSON: store players by `side` and `unum` (B4). An illegal player is skipped, and of a duplicated one, the last is kept | reversed order, a missing player, illegal side and unum, a duplicated player |
| B-4 | JSON: accept fields that may be omitted (§7.5). Only the show's `time` and `ball` and the player's `side` and `unum` are required; any other missing field keeps the default of `PlayerT` (the "no value" mark). Before, one missing field such as `capacity` lost that player and those after it, and a missing ball velocity dropped the whole show. The team in a show gets the show's time (was -1). A show is no longer dropped when its team cannot be read | a live-style show (with the mode and the teams, without the command counts), players without `capacity`, `px`, `fside` etc., `stime`, `pen_*`, `null` team names, a msg without `time` |
| B-5 | JSON: read the ball's `z` and `vz` if present (§7.6). Added `z_` and `vz_` (default 0) to `BallT` | shows with and without the height |
| B-6 | JSON: pass `version` and `timestamp` to the handler (§7.7). `Handler::handleTimestamp()` is now virtual, so that the recording handler of the parallel path records it | both sequential and parallel |
| B-7 | Text: read the strings of a team graphic at any length (B6). An XPM with 2 characters per pixel (16 characters for 8 pixels) was broken | an XPM with 16-character rows, the server's layout |
| B-8 | Text: skip the lines that cannot be read and go on (B8). A failure of parsing a show etc. also counts as a skipped line, and the first 10 line numbers and the total are printed (`report_skipped`, as for JSON). An empty line is just skipped. A `null` in a team line is an empty team name. A show line whose 4 ball values cannot be read is skipped (they were read as 0). A player with `unum` 0 wrote to `player_[-1]`; fixed. `parse_lines`'s "stop at the first failure" is no longer used and was removed | an illegal line, an empty line, an unknown record, a show with a broken ball, a show cut in the middle, a show with `unum` 0, a `null` team name |
| B-9 | Align the parameter defaults with the asio-based rcssserver 19.0.0 (B7; the web version's `param-defaults.ts`): 33 server_param, 8 player_param, and 4 player_type values (`dash_power_rate` 0.06 was a typo of 0.006; `team_far_length` became the pitch diagonal 125.096 in 2022). Added `tcp_port`, `tcp_coach_port`, `tcp_olcoach_port`, `ws_port`, and `ws_allowed_origins`, which the TCP / WebSocket servers send ("Unsupported parameter" was printed). Fixed `PlayerTypeT`'s output, which wrote the value of `dist_noise_rate` for all four v19 noise rates | new test `rcg_types` |

**Tests**
- All 10 tests pass. Each step's tests were confirmed to fail when going back to the file before the fix, or when only the fixed part was turned back to the old behavior. At first, with one record per line, skipping a whole line with an unknown record gave the same result, and there was no test of a record that cannot be read in a whole document, so those two tests were added.
- The two full-game logs were read with the parser at the end of batch A and with the current one, and everything passed to the handler (all show fields, play modes, teams, messages, team graphics) was written out and compared. The only differences are JSON's `vq` values (the bool 1 was stored as a character before) and that `version` and `timestamp` are now passed.
- Every step B1–B9 builds and passes its tests with CMake from a clean copy. B9 also builds with autotools without warnings.

**Found**
- `origin/feature/json-reformat` (§7.9) changes `parser_json.cpp` (the old parser using nlohmann/json), which no longer exists, and nothing that the current `parser_simdjson.cpp` lacks is left. Not taken. The bundled `rcss/rcg/nlohmann/json.hpp` is used by nothing.
- The text show parser does not check that the numbers of the player part were read (a line cut in the middle is caught by the check of the number of players). Only the ball and `unum` were fixed.
- The speed is unchanged. The app's `parsing elapsed` (offscreen, 3 runs, with libdeflate): CMake JSON 69–72 ms, text 59–61 ms; autotools 67–73 ms, 59–61 ms (before batch B: 63–76 / 59–64 ms).

#### 2026-11-12: The user's checks, and deleting unused files

- The 10 commits of batch B (0bf9ee3 to caf490b) were pushed.
- The user checked on the actual screen: the loading speed and the live connection are OK. The view quality (`vq`, B2) is not used by any display of rcssmonitor (nothing in `src/` uses `view_quality_`), so it cannot be checked on the screen. B2 was checked with the parser tests.
- Decided (user): delete the unused `rcss/rcg/nlohmann/json.hpp` (for the old JSON parser) and `src/gzfstream.{h,cpp}` (unused since batch A). With it, the empty `#ifdef HAVE_LIBZ` left in `main_window.cpp` is deleted, and the file dialog offers `*.rcg.gz` also with libdeflate alone, without zlib.

#### 2026-11-12: Merge

- PR #65 (`feature/json-parser`) was merged into develop with a merge commit (a6f2456). The CI of develop passed too.

### Phase 3

#### 2026-11-12: Qt6 support (`feature/qt6`)

The user created `feature/qt6` from develop (a6f2456) on GitHub.

**Preparation**
- The work machine had only the runtime libraries of Qt6 (the Phase 0 record was corrected). The user installed `qt6-base-dev` and `libqt6opengl6-dev` (6.4.2).
- Building with Qt5 and `QT_DISABLE_DEPRECATED_BEFORE=0x050F00` gave errors only for `setMargin` (27 places). Most of the others were deprecated in Qt 6, so they were found by building with Qt6: the sources before the replacements (Q1) with the current CMake gave 49 errors with Qt6 (`setMargin` 27, the `+` of `Qt::CTRL + Qt::Key_X` 21, the conversion of an element of `QKeySequence` to `int` 1).

**Decided**
- CMake uses Qt6 if it is found, and Qt5 otherwise. `-DQT_VERSION_MAJOR=5` selects Qt5. The oldest versions are Qt 6.2 and Qt 5.15 (D2).
- Deprecated APIs are made unavailable at build time: `QT_DISABLE_DEPRECATED_BEFORE` is 0x050F00 for Qt5 and 0x060000 for Qt6. What Qt 6.1 or later deprecates is caught in CI with `-Werror=deprecated-declarations`.
- `QMouseEvent::pos()` is not deprecated in any of Qt 6.2, 6.4, and 6.10 (checked in the qtbase headers; deprecated are `globalPos()`, `x()`, `y()`, and `pos()` of the other events). "`pos()` is deprecated in Qt 6" in §3.3 was wrong, so no function switching by version is made (one was made, and dropped).
- `QTime` was already replaced with `QElapsedTimer` in A7 of Phase 2.

**Made (one commit each, in order)**

| Step | Content | Tests |
|---|---|---|
| Q1 | Removed the branches for Qt older than 5.15 (22 `QT_VERSION_CHECK`s and 4 comparisons of Qt 4 versions in hex). `QFontMetrics::width()`, `QWheelEvent::delta()`, `setResizeMode` etc. were only in the old branches | — |
| Q2 | `setMargin( n )` became `setContentsMargins( n, n, n, n )` (removed in Qt6) | — |
| Q3 | Key combinations became `Qt::CTRL \| Qt::Key_X` (`+` is removed in Qt6). Removed the branches of the Qt 4 macro `Q_WS_MAC` (21 places; not defined since Qt 5, so never used) and `Q_WS_WIN`. The actions that select a player with a key read the key of their shortcut as an `int` (a `QKeyCombination` in Qt6), so they now have the number of the player with `setData()`. Also fixed the names "Select Left 0" of the 10th player's action and "Selct Right" | `player_select_keys` (new): the shortcuts 1–9, 0, - of the main window, with and without Ctrl. Fails with the old `main_window.cpp`, which has no numbers. All actions of the main window, with their names and shortcuts, were written out and compared with before: the same except the names and numbers of the player selection |
| Q4 | `QSignalMapper` (2 places, deprecated in Qt 5.15) became lambda connections | `play_mode_actions` (new): an item of "Change Playmode" in the field menu passes its mode to `FieldCanvas`. Fails when a wrong mode is passed |
| Q5 | Removed `USE_GLWIDGET` and `QGLWidget` (no build configuration defined it; Qt6 has no `QGLWidget`) | — |
| Q6 | CMake supports both Qt5 and Qt6 (`find_package(QT NAMES Qt6 Qt5)`, `Qt${QT_VERSION_MAJOR}::…`). `cmake_minimum_required` is 3.16 (§8 item 10). `QT_DISABLE_DEPRECATED_BEFORE` is defined | 12 tests with Qt6 and Qt5 |
| Q7 | CI: the CMake job became two, Qt5 and Qt6 (`-Werror=deprecated-declarations`), and a job checks the oldest Qt 6.2 (an `ubuntu:22.04` container, CMake 3.22) | — |
| Q8 | Rewrote the README's installation: CMake first, the Qt6 and Qt5 packages of Ubuntu 22.04 / 24.04 / 26.04, how to select the Qt, autotools for Qt5 only, CMake's options. Fixed C++14 to C++17, and dropped the unneeded packages such as boost | — |
| Q9 | Qt5 also scales by the device pixel ratio of the screen (`AA_EnableHighDpiScaling`, also a fractional one). Qt6 always does, so the same canvas size makes the same window (the user's check below) | On the work machine's screen, Qt5's ratio and logical DPI became the same as Qt6's |
| Q10 | The toolbar icons (XPMs in black) are drawn in the text color of the palette (white becomes transparent, gray half transparent), and again when the palette changes. With a dark palette, the black icons were hard to see and their white backgrounds stood out | `tool_icon` (new): the tinting, and drawing again when the palette changes. Fails when the redrawing is stopped. The toolbar was drawn offscreen with a dark and a light palette and looked at |

**Tests**
- Every step Q1–Q10 builds and passes its tests with CMake from a clean copy. From Q6 on, with both Qt 6.4.2 and Qt 5.15.13, without warnings even with `-Werror=deprecated-declarations` (12 tests, 13 from Q10). The last step also builds with autotools (Qt5).
- High DPI (§8 item 6): with Qt6, the main window with a log at frame 1,000 was drawn offscreen at scale 1, 1.5, and 2 into images. The layout, text, and buttons do not break. The field lines and player outlines stay 1 device pixel, so they are relatively thin at scale 2. The team logo (XPM) is scaled up and blurred. Offscreen, the text of Qt6's menus and score board is a little smaller than Qt5's (likely a difference of the default font). The check on an actual screen is the user's.
- Ubuntu 22.04 (Qt 6.2) was not tried locally, as there is no Docker. CI checks it.

**The user's checks (Qt6, on the actual screen)**
- The live connection, log playback, referee operations, and the config dialog are fine.
- With the same canvas size, the window size differs between Qt5 and Qt6, and so does the size of the team graphic. Cause: the work machine's screen is 3840×2160 at 124 dpi (GNOME on Wayland; Qt5 and Qt6 both run on XWayland). Qt6 always scales by the screen's ratio (124 / 96 = 1.29), while Qt5 did not scale pixels and made only the text larger for 124 dpi. Qt6 draws consistently; with Qt5, the team graphic and the lines were small compared with the text. Decided (user): Qt5 also scales as Qt6 does (Q9).
- With a dark desktop theme, the Qt6 window is dark too (background #323232; Qt5 stays light), and the toolbar icons are hard to see. Decided (user): the icons follow the palette (Q10).
- With Qt6, loading takes over 800 ms for JSON and over 70 ms for text. Cause: the user's build directory had `CMAKE_BUILD_TYPE=Debug` (simdjson is not optimized). A Debug build with Qt6 was confirmed to take 692–868 ms for JSON and 73 ms for text. In Release, Qt6 and Qt5 take the same: JSON 70 ms, text 60 ms. The user's machine has no `libdeflate-dev` either (zlib only).

#### 2026-11-12: Merge

- PR #66 (`feature/qt6`) was merged into develop with a merge commit (95965dc). The CI of develop passed too. The completion criteria of Phase 3 (builds with both Qt 5.15 and Qt 6, no deprecation warnings, CI, the user's checks) are met.

#### 2026-11-12: The bug of selecting Qt (`fix/qt-selection`)

- While building the 3D prototype (below), configuring without `-DQT_VERSION_MAJOR` selected Qt5 on the work machine, which has both Qt5 and Qt6 (in Phase 3, Qt6 was selected). Cause: `find_package(QT NAMES Qt6 Qt5)` finds them not in the order of the names but in the order in which it reads the directories in `/usr/lib/x86_64-linux-gnu/cmake/`. Installing the Qt Quick 3D packages added directories and changed the order. "Qt6 if it is found" in the README was not guaranteed. CI gives the version explicitly, so it is not affected.
- Fixed: without the option, look for Qt6 (6.2 or later) first, and use Qt5 if it is not found. The version is kept in the cache (`QT_VERSION_MAJOR`), and does not change when configuring again. Confirmed that no option, `5`, `6`, and not looking for Qt6 (`-DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON`) give Qt 6.4.2, 5.15.13, 6.4.2, and 5.15.13.

### Phase 0 (continued)

#### 2026-11-12: The 3D prototype and D1 (`proto/3d-gl`)

With Phases 1–3 merged, the 3D prototype started.

**Decided (user)**
- The prototype is made on `proto/3d-gl`, created from develop (95965dc), and not merged (§9.1). Its useful parts are brought into `feature/3d-view` later, cleaned up. The record goes into the plan on develop (this one through `fix/qt-selection`).
- Make both the recommendation (`QOpenGLWidget` with our own rendering) and Qt Quick 3D, and compare them.

**Made (bf554fe, e9a9b65, c54d791 on `proto/3d-gl`)**
- `resources/models/Mannequin.glb`: the model of the web version written again without EXT_meshopt_compression and KHR_mesh_quantization, with glTF-Transform 4.5.1 and meshoptimizer (346 kB to 684 kB; D6), with the script that wrote it (`decompress.mjs`) and a README of its source. One mesh (materials M_Main and M_Joints), 65 joints, 4 clips (the translation, rotation and scale of each joint).
- `third_party/cgltf/`: cgltf 1.15 (MIT; D5).
- `proto/3d-gl/` (CMake's `BUILD_PROTO_3D`, default OFF): two prototypes draw the same scene (the ground and lines, 22 human figures, the ball, the shadows of the sun) from the same log (`1-json.rcg.gz`). The poses are computed from the log every frame by shared code (`gltf_rig`, `proto_log`) (§5.8: idle, walk, run and sprint mixed by the speed, each at the time of the distance walked over the speed of the clip).
  - `proto_gl`: OpenGL 3.3 core, `QOpenGLWidget`, our own shaders. Qt5 and Qt6. The skin matrices are in an RGBA32F texture, and the 22 figures are drawn with one instanced call per material. A 4096 px shadow map with a 5 x 5 PCF. The drawing (`SceneGL`) is separate from the widget, so it can also draw without a display, with `QOffscreenSurface` and an FBO (`--offscreen`).
  - `proto_quick3d`: Qt Quick 3D, `QQuickWidget` (`QQuickView` with `--window`). Qt6 only. The model is converted to QML by balsam at build time, without the `Timeline`s of the clips (`make_quick3d_model.py`). The joint nodes are found by their QML ids, and their `position`, `rotation` and `scale` are written every frame.
  - `run_bench.sh`: measures both, one after the other, at 1920×1080 for 10 s each.

**Measured (the user's machine: the integrated GPU of a Ryzen 9 9950X3D (radeonsi, raphael), Mesa 25.2.8, OpenGL 4.6 core; 1920×1080, 10 s)**

| | fps (vsync) | fps (no vsync) | CPU / frame | GPU / frame |
|---|---|---|---|---|
| OpenGL (Qt6), figures, 4096 px shadows | 59.9 | 60.0 (note below) | 0.29 ms (poses 0.26 ms) | 4.44 ms |
| OpenGL (Qt6), figures, no shadows | — | 60.0 | 0.25 ms | 1.02 ms |
| OpenGL (Qt6), cylinders, shadows | — | 60.0 | 0.03 ms | 3.81 ms |
| OpenGL (Qt5), figures, 4096 px shadows | — | 158 | 0.27 ms | 4.42 ms |
| Qt Quick 3D (`QQuickWidget`), figures, shadows | 59.9 | 128.6 | 5.3–5.9 ms (p99 9.7 ms) | not measurable |
| Qt Quick 3D, only the rotations of the joints written | — | 128.6 | 4.2 ms | — |
| Qt Quick 3D (`QQuickView`) | — | 128.9 | 5.4 ms | — |
| Qt Quick 3D, no shadows | — | 161 | 5.2 ms | — |
| Qt Quick 3D, cylinders | — | 181 | 0.01 ms | — |

**Found**
- Both keep 60 fps on the integrated GPU. OpenGL takes 4.4 ms of GPU (3.4 ms of it for the shadows; 1.0 ms without them) and 0.3 ms of CPU: about four times the headroom.
- Qt Quick 3D takes 5–6 ms of CPU every frame (about 10 ms at p99) to write the joints through properties (22 figures x 65 joints x 3; 4.2 ms even with the rotations only): 30–60% of a frame at 60 fps (16.7 ms).
- In Qt Quick 3D 6.4, the skinned models cast almost no shadows (the cylinders do; it may select the shadow casters by the bounds of the bind pose, for example; not checked with a newer Qt). The values of Qt Quick 3D in the table are without the shadows of the figures.
- Qt6's `QOpenGLWidget` stays at 60 fps even with a `swapInterval` of 0 (likely because the window is composed through RHI; 158 fps with Qt5). The headroom is measured with the GPU time (timer queries; §10.3).
- Where Qt Quick 3D got stuck: on Ubuntu 24.04, `qt6-quick3d-dev` does not depend on `qt6-shadertools-dev` (and `qt6-shader-baker`, which has `qsb`), which must be installed separately. Most of balsam's output (5,332 lines) is the `Timeline`s of the clips. Qt 6.4's `QQuickWidget` has no `setInitialProperties`. The `lookAt` of a light (`Node`) cannot be called from C++ (that of a camera can). `QQuick3D::idealSurfaceFormat()` cannot be called before the application is created. The best shadow map of Qt 6.4 is VeryHigh.
- Without a display (`QT_QPA_PLATFORM=offscreen`), neither `QOpenGLWidget` nor a Qt Quick window can use OpenGL (Qt Quick falls back to the software renderer). The prototypes were checked by drawing with `QOffscreenSurface` and an FBO (`proto_gl`) and with `QQuickRenderControl` (`proto_quick3d`; `QT_QUICK_BACKEND=rhi`). The rendering tests of §10.2 take this form too, with the drawing separate from the widget.
- With software rendering (llvmpipe, 1280×720), `proto_gl` takes about 36 ms a frame with the figures and the shadows (0.2 ms for the poses), and `proto_quick3d` about 75–80 ms.

**D1 decided (user)**: the recommendation (`QOpenGLWidget` with our own rendering, OpenGL 3.3 core). Reasons: 60 fps with headroom even on the integrated GPU (GPU 4.4 ms, CPU 0.3 ms). The same code works with both Qt5 and Qt6 (D2). Deciding the poses from the log every frame (§5.8) is cheap to write. Few dependencies (Qt's OpenGL and cgltf). Phase 4 brings in, cleaned up, the prototype's `gltf_rig` (glTF loading and poses), the separation of `SceneGL`, the shadow map, and the instanced skinned drawing.

### Phase 4

#### 2026-11-12: Step 4-1, the foundation of the 3D view (`feature/3d-view`)

PR #67 (`fix/qt-selection`) was merged into develop with a merge commit (2a88791). The user created `feature/3d-view` from develop.

**Decided**
- Phase 4 is split into five steps: 4-1 the foundation (`ENABLE_3D`, `FieldView3D`, the ground, switching between 2D and 3D), 4-2 time and interpolation, 4-3 players, ball, goals, 4-4 cameras, 4-5 HUD and number labels. The user checks each step.
- The 3D code is in `src/view3d/`, built with CMake only (D3). `ENABLE_3D` (default ON; with Qt6 only when `OpenGL` and `OpenGLWidgets` are found) defines `HAVE_3D_VIEW` in `config.h`. autotools and `-DENABLE_3D=OFF` build without 3D.
- The drawing (`Scene3D`) is separate from the widget (`FieldView3D`), as `SceneGL` of the prototype. It takes the framebuffer to draw into, so the tests can draw into an offscreen FBO (§10.2).
- The 3D view is made when it is first shown (no OpenGL parts in the window of those who use 2D only).

**Made**
- `FieldPainter::drawGround()`: draws the ground without the goals, with the pens some times wider. `Options::fieldTransform()` / `setFieldTransform()`: change the scale and the center of the field for a while (without changing the zoom).
- `src/view3d/ground_texture`: the 2D `FieldPainter` draws the pitch and its margins (115 m x 78 m) into an image at 24 px/m with 3 times wider pens (12.5 cm) (the web version's `field-texture.ts`). It is drawn again only when the colors, the flags, the grid or the keepaway settings change (`key()`).
- `src/view3d/camera_3d.h`: the camera's position, target, up, field of view (or orthographic height), and its view and projection matrices. GL-independent (also for the cameras of 4-4).
- `src/view3d/scene_3d`: drawing with OpenGL 3.3 core. For now only the ground (an unlit texture, anisotropic filtering up to 8x) and the sky color.
- `src/view3d/field_view_3d`: a `QOpenGLWidget` (OpenGL 3.3 core, 4x multisampling). The camera for now is a fixed one, looking at the center from a broadcast position (72 m, 32° up) (replaced in 4-4). `unavailableReason()`: checks whether an OpenGL 3.3 context can be made and whether the platform shows OpenGL widgets (offscreen, minimal, vnc, linuxfb do not).
- Main window: the center is a `QStackedWidget` that switches the 2D view and the 3D view. View > 3D View, the "3D" button of the toolbar, key D (a checkable action). Where 3D is not available, the action is disabled with the reason in the status bar and the tooltip, and the 2D view stays. The setting `view_3d` (group `MainWindow`) and `--view-3d` on the command line start with the 3D view.
- CI: the Qt6 jobs install `libqt6opengl6-dev` and check in `config.h` that the 3D view is built. The tests run under Xvfb with Mesa's software OpenGL (`LIBGL_ALWAYS_SOFTWARE=1`).

**Tests**
- Test `ground_texture` (new): the size of the image, the colors of the grass, the center line and where the goal was, the width of the center line (about 3 px), the scale and the center of 2D set back, the key changing with the colors and keepaway. Confirmed to fail with a pen scale of 1 and with the goals drawn.
- Test `scene_3d` (new): draws the ground from straight above into an offscreen OpenGL 3.3 context and FBO, and checks the colors of the center line and the grass, and that two frames are the same. Skipped where an OpenGL 3.3 context cannot be made.
- Test `view_switch` (new): on the offscreen platform 3D is not available, so D disables the action, shows the reason, and keeps the 2D view and the selected player. The real switching is checked by the user on the screen.
- Every step of 4-1 builds and passes the tests from a clean copy with Qt 6.4.2 and Qt 5.15.13 (`-Werror=deprecated-declarations`; 16 tests). `-DENABLE_3D=OFF` (13 tests) and autotools pass too.
- The ground from the broadcast position was drawn offscreen and looked at (the same lines, penalty areas and center circle as 2D).

#### 2026-11-12: Steps 4-2 and 4-3, time and interpolation, players, ball, goals (`feature/3d-view`)

After the five commits of 4-1 (1956f13 to 27f1100) were pushed and the CI passed, the user checked them on the screen. 4-2 (time and interpolation) alone shows little on the screen, so the user checks it together with 4-3 (players, ball, goals).

**Decided**
- What is drawn where (the matrices and colors of the instances) is decided by the GL-independent `SceneBuilder` and checked by unit tests. `Scene3D` only draws them, one instanced draw call per shape.
- How a player is colored (the 2D `PlayerPainter::drawBody`) is extracted into a function `playerStyle()`, shared by 2D and 3D.
- The cards are drawn in 4-5 with the billboards (the number labels). The direction lines in the foot circles and the view cones need the drawing of lines, so they are drawn with the overlays of Phase 5.
- The lighting is not a copy of the three.js values but a simple formula: Lambert with the sun (the direction of §4.3) and a light from the sky (tuned by looking at it; the shadows come in Phase 8).

**Made**
- Tests: a test that Qt Test skips (`QSKIP`, e.g. when an OpenGL 3.3 context cannot be made) is reported by CTest as skipped, not as passed (`SKIP_REGULAR_EXPRESSION`). In the CI of 4-1, `scene_3d` passed in 0.22 s in the Ubuntu 22.04 job, and it was not clear whether it really drew.
- `LogPlayer::alpha()`: while playing, the time since the last step divided by the interval (0 to 1); 0 when stopped. `isPlaying()`.
- `src/view3d/interpolate`: the interpolation between frames (the web version's `interpolate.ts`). Positions and velocities are linear, the body direction turns the short way round, and the neck, an angle relative to the body, is linear. No interpolation across a play mode change, when the ball moved more than 4 m or a player more than 3 m, and for a player who is not connected or was replaced. `position()`: the frames to draw and the fraction between them. A log goes from the current frame to the next; live goes from the frame before the newest to the newest, by the time since the newest came divided by `simulator_step` (one cycle late).
- `src/player_style`: `playerStyle()`. The outline of the outer circle (width 2 for the selected player, a kick, a tackle, illegal defense), the fill (the team and state colors), the body (darker by steps of 1/8 of the stamina), and the decay ring (effort, recovery). `drawBody` uses it now.
- `src/view3d/scene_builder`: the goals (posts and crossbar, translucent nets, a dark floor), the players (a cylinder body, a sphere head, the foot circle (the kickable area or Player Size, opacity 0.45), the highlight ring, the decay ring, a black cone for the body direction, a red cone for the face direction), and the ball (in the air when the show has `z`, with a shadow right below that fades with the height). The values are the web version's (§4.3). All 22 players are drawn (one who is not connected in black, as in 2D).
- `Scene3D`: lit instanced drawing (cylinders, spheres, cones) and translucent instanced drawing (circles, rings, nets; no depth writes, both sides). The instance buffers are made again only when they grow, and are reused with `glBufferSubData` otherwise (§5.3).
- `FieldView3D`: takes the fraction of playing from `LogPlayer` and draws the interpolated show. It remembers when the newest frame came, for the fraction of live. While playing, and while waiting for the next frame live, it draws the next frame as soon as one is shown (`frameSwapped`). When nothing moves, it does not draw.

**Tests**
- Test `player_style` (new): the colors of the team and the goalie, a player who is not connected, the selected player, the order in which the state colors override (the tackle after the kick), the darkness by the stamina, the decay ring (effort first).
- Test `interpolate` (new): the short way round of angles, the position and direction halfway, the fractions at the ends and out of range, a play mode change, jumps, a player who is not connected or was replaced, `position()` for a log and live.
- Test `scene_builder` (new): the positions of the goal posts, crossbar and nets, the players' body, head, and the size and color of the foot circle, the Player Size option, the directions of the cones, the highlight and decay rings, the ball's height and the darkness of its shadow, and the options that hide the players and the ball.
- Added to test `scene_3d`: a player and a ball in the air drawn from straight above, with the pixel colors of the head, the ball, the foot circle, the ball's shadow and the grass, and two frames the same.
- Every test was confirmed to fail when what it checks is broken for a while (the sign of the direction, the size of the foot circle, the ball's height, the way round of angles, the play mode change, the steps of darkness, the order of the states, not drawing the spheres or the circles).
- The 2D drawing is unchanged: seven frames of the full game log were drawn offscreen with HEAD and with the current build, and the pixels are the same.
- Builds and passes the tests with Qt 6.4.2 and Qt 5.15.13 (`-Werror=deprecated-declarations`; 19 tests). `-DENABLE_3D=OFF` (14 tests) and autotools pass too.
- Frame 1,000 of the full game log was drawn offscreen from the broadcast position, close to the ball, and near a goal, and looked at. Xvfb is not installed here, so the 3D view in the application's window (smoothness while playing, live) is checked by the user on the screen.

#### 2026-11-12: Step 4-3b, tidying the rendering (`feature/3d-view`)

The five commits of 4-2 and 4-3 (81a8493 to c863113) were pushed, and the CI passed (`scene_3d` in the Ubuntu 22.04 job also "passed", not "skipped": it really draws).

**Decided (user)**
- The OpenGL rendering code is hard to follow, so before 4-4 it is abstracted without changing how it looks. The choice is a thin layer of our own (the table in §5.3). Qt's `QOpenGLBuffer` etc. would help only partly, as they do not manage instance attributes or render states; Qt RHI is public API only from Qt 6.6, which does not fit D2; an external rendering engine is a large dependency. None of these is taken.
- Efficiency: a frame is 10 to 20 draw calls and a few KB of uploads; the prototype measured CPU 0.3 ms and GPU 4.4 ms (§14). The abstraction adds a few microseconds per frame, which does not matter. The locations of the uniforms are remembered, so the lookups by name even go down.

**Made**
- The parts in `src/view3d/gl/`, `meshes_3d`, and the shader files (§5.3). `Instance3D` moved to `instance_3d.h`.
- A vertex has one form (`Vertex3D`: position, normal, texture coordinates), and the ground is drawn with `InstancedMesh` like the other shapes (with one instance).
- `Scene3D` went from about 540 lines to 177. `render()` only lists, pass by pass, "apply the render state, choose the shader, set the uniforms, draw the shapes".
- The resources in a static library are registered with `Q_INIT_RESOURCE` before use (`ShaderProgram::load()` does it once, at the first time).
- So that those who do not know OpenGL well can maintain it by hand, comments were added for each function and block (user). The GL parts, the shaders, `Scene3D`, `FieldView3D`, `CameraPose`, `Instance3D`, `Meshes3D` and `SceneBuilder` say what they do, explain the OpenGL terms (buffers, VAO, instanced drawing, uniforms, the depth buffer, culling, blending, mipmaps, matrices), and how to add something new to draw. The code without the comments is the same as before (compared with the comments removed).

**Tests**
- Test `meshes_3d` (new): every shape has triangles of some area, counterclockwise as seen from where their normals face, with unit normals. The extents and normal directions of the cylinder, sphere, cone, ring, quad and ground, the vertex counts, and the ground's texture coordinates. Confirmed to fail without the winding fix, with triangles of no area at the poles of the sphere, and with wrong texture coordinates of the ground. Breaking the name of an attribute in a shader prints the load error and fails the `scene_3d` test.
- The look is unchanged: six images, two frames of the full game log from three cameras, drawn offscreen, are the same as before (HEAD) down to the bytes of the PNG files.
- Clean builds pass the tests with Qt 6.4.2 and Qt 5.15.13 (`-Werror=deprecated-declarations`; 20 tests). `-DENABLE_3D=OFF` (14 tests) and autotools pass too.

**Found**
- The tarball that autotools makes with `make dist` lacks the files that only CMake builds (`src/view3d/`, `test/` etc.). The tarball of the 19.0.1 release was made with `make dist` (it has `configure` and `Makefile.in`, and no `.github`). Added to the work of Phase 9.

#### 2026-11-12: Step 4-4, cameras (`feature/3d-view`)

The two commits of 4-3b (2859390, d279615) were pushed, and the CI passed (with the shaders read from the resources, the Qt 6.2 job draws too).

**Decided**
- The kinds and names of the cameras (`CameraMode`) are in a GL-independent header, `src/camera_mode.h`, as the settings (`Options`) are also in the builds without 3D (autotools, `ENABLE_3D=OFF`). In 4-4 there are five: free, broadcast, top, and the two behind the goals. The two close-ups and the player camera are added in Phase 6 (E goes through them in the web version's order).
- The poses (`Cameras3D`) and which camera, where it is now and how it follows (`CameraController`) are in the GL-independent `src/view3d/cameras_3d`, checked by unit tests. The values are the web version's (§4.3).
- The mouse works as three.js's OrbitControls: left-drag orbits (a drag across the height of the view turns once round), Shift / Ctrl / Meta + left-drag pans (the ground at the target follows the mouse), middle-drag (5% per 100 px) and the wheel (5% per notch) zoom. Distance 3 to 400 m, down to 0.03 rad above the ground. Differences from the web version: (1) a following camera becomes free when the mouse moves it, not when the button is pressed. (2) It stops 0.01 rad short of straight above (the web version: 1e-6; straight above, the camera's up is along its view, and its direction is unstable).
- In 3D the camera takes the wheel (as planned). The top camera does not take the mouse, so there the wheel steps the frames as in 2D.
- E works in 2D too, choosing the camera for the next time the 3D view is shown (as the web version). The selector in the tool bar is shown with the 3D view only.

**Made**
- `src/camera_mode.h`: the list of the cameras (names in the settings and the menu), conversion from and to the names, the next camera.
- `Options`: the setting `camera_3d` (group `MainWindow`), `--camera-3d` on the command line. An unknown name is not read.
- `src/view3d/cameras_3d`: `fitView()` (the shortest distance that keeps the corners of the pitch and 2 m around it within 97% across and 85% up and down), `broadcast()`, `goal()`, `top()` (orthographic, the field's -y up as in 2D), `smoothToward()`, `orbit()`, `pan()`, `dolly()`. `CameraController`: a following camera jumps when the camera is changed and otherwise follows with a time constant of 0.35 s (at most 0.1 s per frame; it stops within 0.02 m). Moving a following camera with the mouse makes it free from there. The free camera starts from the whole pitch only when coming from the top camera.
- `FieldView3D`: the camera (`setCameraMode()`, `cameraModeChanged` when the mouse made it free), the mouse and wheel, and the time between frames. It keeps drawing while the camera follows.
- Main window: View > 3D Camera (the five cameras and Next 3D Camera), a selector in the tool bar, key E. Where 3D is not available, these are disabled too.

**Tests**
- Test `cameras_3d` (new): the names and the next camera; the corners within the view with one at the edge from 9 viewpoint directions and 2 aspect ratios; the side view equal to the web version's closed form; the broadcast distance, elevation, following and stopping at the edges; the pose behind the goals and its target; the top camera's extent and orientation; `smoothToward()` (1 - 1/e after the time constant, the same in two steps); orbit (distance and target unchanged, back after once round, never below the ground or straight above); pan (camera and target by the same amount, the height of the view for a drag across the height, the direction unchanged); dolly (stops at the limits); `CameraController` (jumps at first, follows, stops, only 0.1 s for a long gap, jumps on a change of camera, becomes free by the mouse and goes on from there, the top camera takes no mouse, free after top shows the whole pitch).
- Added to test `view_switch`: View > 3D Camera has the camera of the settings checked; E goes to the next (menu, selector, status bar); after the last comes the first; the cameras are disabled where 3D is not available.
- Added to test `scene_3d`: drawn with the top camera, the center line, both goals, and a player at the field's y = -20 drawn above the center.
- Every test was confirmed to fail when what it checks is broken for a while (the top camera's up, the 85% vertically, the broadcast stopping at the edges, not going below the ground, the direction of panning, the 0.1 s limit, the top camera taking no mouse, the menu following E).
- Clean builds pass the tests with Qt 6.4.2 and Qt 5.15.13 (`-Werror=deprecated-declarations`; 21 tests). `-DENABLE_3D=OFF` (14 tests) and autotools pass too.
- Frame 3,000 of the full game log was drawn offscreen with the five cameras (the free one turned and moved closer) and looked at.

**Found**
- With Qt5, `cameras_3d.h` used `M_PI` without `<cmath>` (the Qt6 headers include it indirectly). Found in the Qt5 build and fixed.
- In 2D, the wheel steps the frames (`MainWindow::wheelEvent()`).

#### 2026-11-12: Step 4-5, number labels, cards, HUD (`feature/3d-view`)

The two commits of 4-4 (e05064b, fdc2a96) were pushed, and the CI passed. The user checked them on the screen.

**Decided**
- The numbers and the cards are drawn as rectangles that face the camera (billboards), as in the web version. The texts of all the players are laid out in one image (`LabelAtlas`), drawn with one texture and one draw call. A text is drawn again only when it changes.
- A card is to the right of the number, as in the web version (in 2D, to its left).
- The HUD is drawn over the 3D view with QPainter by the 2D painters (`ScoreBoardPainter`, `TeamGraphicPainter`) (§5.5). The compass, the minimap and the numbers of the ranges come in Phases 5 and 6.
- The user pointed out that calls of OpenGL functions had come back into `scene_3d.cpp` (clearing and resetting the state at the start of a frame, unbinding the texture at its end). User's decision: OpenGL is hidden in `gl/`, and the usual code uses the abstraction layer (written as the policy in §5.3). The start and the end of a frame moved to `gl/frame` (`GLFrame`), and test `gl_isolation` was added to check the policy.

**Made**
- `src/view3d/label_atlas`: an image with slots for the 22 players and a white one for the cards (a slot is 192 x 64 px, the text 40 px, white with a black outline of 6 px). The text is centered and squeezed horizontally if wider than the slot. It returns the width of the text (m).
- `SceneBuilder::addLabels()` (the web version's `fitLabels()`, `labelPlace()`): the text is the number and the player type (Show tab; the stamina comes as bars in Phase 5), in the color of illegal defense. From the top camera, to the right of the circle on the ground as in 2D, the text as high as the 2D player font. From the other cameras, above the head (2.4 m) in meters, shrinking toward the head when it would be more than twice the player font high on the screen. A card (0.42 x 0.6 m) to the right of the number. Sorted from far to near (a nearer text over a farther one).
- `gl/billboard_mesh` and the shaders `billboard.vert`, `billboard.frag`: the rectangle is spread along the camera's right and up, facing it. Drawn as the fourth pass of `Scene3D`, over everything without the depth test (`RenderState::overlay()`).
- `FieldView3D`: builds the labels, and sends the image to the GPU when a text changed. Over the 3D view, QPainter draws the score board and the teams' logos. The right logo is placed by the canvas size in the options, so the 3D view's size is set there (the same size as 2D).
- `gl/frame` (`GLFrame::begin()`, `end()`): at the start of a frame, the scissor and stencil tests are turned off, and the color and depth writes and the depth comparison set back, before clearing. QPainter (the HUD of the last frame) may leave them changed, and clearing obeys them. At the end, the shader, the shape and the texture are unbound and the render state is what QPainter expects. `Scene3D::render()` only calls this and the other parts in `gl/`.
- Test `gl_isolation` (§5.3).

**Tests**
- Test `label_atlas` (new): the slots are in the image and do not overlap; the white slot; a text in its own slot, white with a black outline, centered; not drawn again for the same text; drawn again for another text or font; the width of the text; a wide text fits its slot.
- Added to test `scene_builder`: the label of a far player (above the head, 1.3 m, white, its slot); the order from far to near; a label 5 m in front shrinking to 24 px on the screen (twice the font of 12 px) toward the head; from the top camera, to the right of the circle with the text as high as the font; a wider text with the player type; no labels without the number and the type; the color of illegal defense; the colors, places and sizes of yellow and red cards; the options without cards and without players.
- Added to test `scene_3d`: from the top camera, a label drawn to the right of the player in white and black, upright and not mirrored (the whiteness of the drawn label on a grid compared with the image of its slot in the atlas, more alike than upside down or mirrored; see "Found" below); nothing on the other side; the same under the ground (over everything). Drawn from the state QPainter may leave (scissor, color and depth writes off, depth comparison), the same image as before.
- Test `gl_isolation` (new): no calls of OpenGL functions or OpenGL constants in the C++ sources in `src/` (except `gl/`). It also checks the detection itself (finds calls and constants, ignores comments, strings and names that only contain `gl`, gives the line numbers). Confirmed to fail with a call put back into `scene_3d.cpp`, and not with a call in a comment. Five images drawn offscreen are the same down to the bytes before and after the move to `gl/frame`.
- Every test was confirmed to fail when what it checks is broken for a while (the text upside down and mirrored, drawing over everything, the limit on the screen, the shrinking toward the head, the size from the top camera, the order from far to near, the place and color of a card, not drawing again, the horizontal squeeze, resetting the state before clearing). At first, breaking "over everything" passed, so the check under the ground was added.
- Clean builds pass the tests with Qt 6.4.2 and Qt 5.15.13 (`-Werror=deprecated-declarations`; 23 tests). `-DENABLE_3D=OFF` (15 tests) and autotools pass too.
- Frame 3,000 of the full game log was drawn offscreen with the five cameras, with the HUD drawn over it by the 2D painters, and looked at (numbers, cards, the score board, the left team's logo).

**Phase 4's conditions**: switching between 2D and 3D keeps the frame and the selection (4-1, checked by the user). With a show that has `ball.z`, the ball is drawn in the air and its shadow on the ground (tests `scene_builder`, `scene_3d`). Unit tests of each camera's pose (test `cameras_3d`). The user checked 4-5 on the screen (no problems). Phase 4's conditions are met.

**Found (CI)**
- In the CI of the three commits of 4-5 (2958c3f to 4ea0ff1), the label test of `scene_3d` failed in the three CMake jobs (it had passed here). Cause: the width of a label depends on the font, and the CI's font differs from the one here. The point that checks "nothing on the other side" fell on the center circle in the CI. The player was moved away from the lines (x = 20, y = 20), and the point is taken on the other side of the player from the label.
- Trying eight fonts here also showed that the check of the orientation by the shape of a "7" (bar at the top, foot at the left) failed with two of them (the shapes and vertical places of glyphs differ by font). The orientation is now checked by the correlation of the whiteness of the drawn label on a 24 x 8 grid with the image of its slot in the atlas, which must be 0.1 more alike than upside down or mirrored. The text is "7,t0" (with the player type), which no font makes symmetric. It passes with all eight fonts, and fails upside down or mirrored.


