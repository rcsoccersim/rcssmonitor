# TODO of rcssmonitor (future work)

| Item | Content |
|---|---|
| For | The developers and the maintainers of rcssmonitor |
| Created | 2026-11-25 |
| Last updated | 2026-11-25 |
| Japanese version | [todo.md](todo.md) (same content) |
| Distribution | This document is not in the distribution (tarball) (`docs/dev` in `.gitattributes`) |

The list of the work not done yet in the whole of rcssmonitor, gathered from what the plans (`docs/plan`) left out and what they put off. §2 has the features to bring to the web version (rcssmonitor-web) too.

## How to use it

- An item a row. The numbers (T1, T2, ... for the desktop version; W1, W2, ... for the web version) are not renumbered. Items are not removed: their state changes.
- States:
  - Not started: to be done some day.
  - In progress: with a link to its plan.
  - Done: with the number of its PR.
  - Put off: decided not to do, with the reason (so as not to consider it again).
  - Undecided: not decided whether to do, with what to decide.
- Sizes: small (about a day), medium (a few days), large (a week or more; a plan is written).
- When an item starts, a plan is written in `docs/plan`, and linked from here.
- What a plan leaves out or puts off is added here too (its plan and section as the origin).

## 1. rcssmonitor (the desktop version)

### 1.1 Exporting images and videos

| # | Item | Origin | Size | State |
|---|---|---|---|---|
| T1 | Videos by Qt Multimedia: made without ffmpeg by `QMediaRecorder` and `QVideoFrameInput` of Qt 6.8 or later. A prototype behind a CMake option (OFF by default), its measurements, and the decision whether to keep it | [F6 §4.8](../plan/2026-11-22-03_frame-export_en.md), D20 of F6 (stage 9 put off) | Large | Not started. First decide the environment of the prototype (aqtinstall into `~/Qt`, an ubuntu:26.04 container, or a CI job with `continue-on-error`) |
| T2 | Sound in the videos (the referee's whistle and so on) | F6 §8, out of scope of the [overall plan](../plan/2026-11-22-00_v20-features_en.md) | Medium | Not started |
| T3 | An export that skips the stopped frames | F6 §8 | Medium | Not started. The 3D interpolation expects adjacent indices |
| T4 | Saving the list of the ranges to export to a file, and reading it | F6 §8 | Small | Not started |
| T5 | Recording the screen while playing | F6 §8 | — | Put off: left to OBS and the like |
| T6 | Transitions in a video of joined ranges (fades, captions) | F6 §8 | — | Put off: edited afterwards with ffmpeg ([the Tips of export.md](../export.md#tips)) |

### 1.2 Logs and networks

| # | Item | Origin | Size | State |
|---|---|---|---|---|
| T7 | Authentication by a GitHub token: the API's limit raised from 60 to 5000 an hour, and private repositories read | [out of scope of F5](../plan/2026-11-22-02_remote-logs_en.md) | Medium | Not started. Decide how to keep the token |
| T8 | Playing while downloading (streaming) | out of scope of F5 | Medium | Not started |
| T9 | Downloading rcl (the logs of the commands) | out of scope of F5 | Medium | Not started |
| T10 | wss:// (TLS) connections (through a reverse proxy) | [out of scope of TCP / WebSocket](../plan/2026-11-06_transport_en.md) | Medium | Not started. The server has no TLS |

### 1.3 The 3D view

| # | Item | Origin | Size | State |
|---|---|---|---|---|
| T11 | Better handling of the binary rcg (v1 to v3) in 3D | [out of scope of the 3D view](../plan/2026-11-11_3d-view_en.md) | Small | Not started. Now only read as in 2D |
| T12 | Following the format of the server of the 2.5D simulator | out of scope of the 3D view | Medium | Undecided: after the format is decided |
| T13 | Reviewing drawing features of little value (the realistic grass; having both the detailed and the simple shadows), to have less drawing code of our own | out of scope of the 3D view (the conclusion of the study of Qt Quick 3D) | Medium | Undecided: decide what to drop |
| T14 | Motion matching of the human figures | out of scope of the 3D view | — | Put off: it relies on the poses before, against the principle that a pose is decided from the frames of the log only (§5.2 of the 3D plan) |
| T15 | Porting to Qt Quick 3D | out of scope of the 3D view | — | Put off: the code saved is small, and the port needs work and gives up either the look or the speed. To check it, a small prototype on 2 points (22 figures moved without a shader of our own; the colors of the uniforms) |

### 1.4 Tidying the code

| # | Item | Origin | Size | State |
|---|---|---|---|---|
| T16 | Tidying the 2D implementation: (1) `src/` into subdirectories by role (e.g. `model`, `geom`, `view`, `view2d`, `view3d`, `app`), with the direction of the dependencies kept by a test; (2) `Options` divided, separating the model and the view | out of scope of the 3D view (2026-11-17, the user) | Large | Not started. A plan is written |

### 1.5 Sound

| # | Item | Origin | Size | State |
|---|---|---|---|---|
| T17 | Sound effects besides the whistle (cheers and so on) | out of scope of the overall plan | Medium | Not started |

## 2. Porting to the web version (rcssmonitor-web)

Of the features added to the desktop version for 20.0.0 (F1 to F6 of the [overall plan](../plan/2026-11-22-00_v20-features_en.md)), whether they go into the web version. Decided by the user on 2026-11-25 (U21 of the overall plan).

| # | Item | Origin | Size | State |
|---|---|---|---|---|
| W1 | Icons: the new icons of the desktop version (a unified SVG design, for the light and the dark color schemes). The application's icon (the RoboCup logo) as the icon of the page (favicon) too | F2 ([plan](../plan/2026-11-22-01_icon-redesign_en.md)) | Medium | Not started (to be ported). The web version has its own 16×16 SVG icons (`currentColor`; `src/ui/icons.ts`) |
| W2 | The list of game logs (File > Browse Game Logs): repositories on GitHub (Git Trees API), archive.robocup.info (h5ai), pages of links (Apache, nginx). Adding and removing the sources, the filter (the team, the opponent, the result, the dates), the cache of the lists | F5 ([plan](../plan/2026-11-22-02_remote-logs_en.md)) | Large | Not started (to be ported). The web version already opens logs from URLs (the pages of GitHub, Git LFS). A browser needs CORS: the API and raw of GitHub can be read, but archive.robocup.info, Apache and so on cannot unless the server allows CORS (to be checked; if not, a relay, or only a guide). The cache in localStorage or IndexedDB |
| W3 | Choosing the color scheme (system, light, dark) | F1 | Small | Undecided: the web version follows the OS's scheme (`prefers-color-scheme`). Whether it can be chosen by hand |
| W4 | Starting from the desktop (the desktop entry) | F3 | — | Put off: not for the web version |
| W5 | The referee's whistle | F4 | — | Put off: not in the web version (the user's decision) |
| W6 | Exporting images and videos | F6 | — | Put off: not in the web version (the user's decision) |

Besides, features only the desktop version has (whether they go into the web version is undecided):

| # | Item | Origin | Size | State |
|---|---|---|---|---|
| W7 | The flying ball (`--ball-flight-3d`) | [§0.2 of the 3D plan](../plan/2026-11-11_3d-view_en.md#02-to-bring-into-the-web-version), [docs/3d-view.md](../3d-view.md#the-flying-ball-flight) | Medium | Undecided (under consideration) |
| W8 | The delayed live view (Live Delay) | [§0.2 of the 3D plan](../plan/2026-11-11_3d-view_en.md#02-to-bring-into-the-web-version), README | Medium | Undecided (under consideration with W7) |
| W9 | Figure packs (glTF, VRM, FBX, BVH; the human pack) | [§0.2 of the 3D plan](../plan/2026-11-11_3d-view_en.md#02-to-bring-into-the-web-version), [docs/FIGURE_PACKS.md](../FIGURE_PACKS.md) | Large | Undecided. Decide how a browser reads the files and where they are kept |

What the plan of the 3D view decided (see [§0.2 of the 3D plan](../plan/2026-11-11_3d-view_en.md#02-to-bring-into-the-web-version) for the details; the W numbers in the brackets of its items are the numbers here):

| # | Item | Origin | Size | State |
|---|---|---|---|---|
| W12 | A stamina capacity bar 0.15 m thick (0.1 m in the web version) | §0.2 of the 3D plan | Small | Not started |
| W13 | The limit of the catch directions (`min_catch_angle` to `max_catch_angle`) in the goalie's catch area | §0.2 of the 3D plan | Small | Not started |
| W14 | The operations of 3D as 2D (the wheel steps the frames; Z and X move the camera nearer and farther) | §0.2 of the 3D plan | Medium | Not started |
| W15 | The look of the focus point (the lines of the same error, the sight and the spotlight, the line from the eye height) | §0.2 of the 3D plan | Medium | Not started |
| W16 | The shadows' option `off` / `simple` / `detailed`, `simple` by default | §0.2 of the 3D plan | Medium | Not started |
| W17 | A compact settings dialog (the summaries and the description rows of the choices into tooltips) | §0.2 of the 3D plan | Small | Not started |
| W18 | The 3D tab of the settings dialog in three (3D, Players & Ball, Camera) | §0.2 of the 3D plan | Small | Not started |
| W19 | A goalie without its number in the field players' color in the player camera | §0.2 of the 3D plan | Small | Not started |
| W20 | The ball's sizes of 2D and 3D apart (`ball_size_3d`) | §0.2 of the 3D plan | Small | Not started |
| W21 | The realistic grass (`plain` / `stripes` / `realistic`) | §0.2 of the 3D plan | Medium | Not started |
| W22 | The frames per second shown (`fps_3d`) | §0.2 of the 3D plan | Small | Not started |
| W23 | Smooth edges on and off (`antialias_3d`) | §0.2 of the 3D plan | Small | Not started |
| W24 | How live games are drawn (2 cycles behind; the clock of the view catches up with the time of the arrivals by a time constant) | §0.2 of the 3D plan | Medium | Not started |
| W25 | The eye height of the player camera by the player's height (1.65 m fixed in the web version) | §0.2 of the 3D plan | Small | Not started |
| W26 | The ball's rolling and its trail broken by the rules of the interpolation (a play mode change, a placement over 4 m) | §0.2 of the 3D plan | Small | Not started |
| W27 | The height covered by the detailed shadows (4 m in rcssmonitor): check that the shadows of the heads and the hands of the web version's figures (1.2 times) are not cut | §0.2 of the 3D plan | Small | Not started |
| W28 | Rework the stands (stands at 4 places, closed solids, the rows and the colors of the people) | §0.2 of the 3D plan | Medium | Not started |
| W29 | Better figure motion (blending in phase, walking and running with directions, leaning, the kick's contact at the moment of the log, the neck divided into the neck, the head and the chest) | §0.2 of the 3D plan | Large | Not started |

Extensions of the web version itself (from what the plan of the 3D view left out):

| # | Item | Origin | Size | State |
|---|---|---|---|---|
| W10 | Moving players by dragging | out of scope of the 3D view | Medium | Not started |
| W11 | A list of the msg (messages) | out of scope of the 3D view | Medium | Not started |
