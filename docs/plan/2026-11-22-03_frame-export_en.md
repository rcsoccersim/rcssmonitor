# Exporting the Screen as an Image Sequence and a Video: Plan

| Item | Content |
|---|---|
| Created | 2026-11-22 |
| Last updated | 2026-11-25 |
| Target | rcssmonitor develop (5affd90, 20.0.0-beta.1) |
| Branch | `feature/frame-export` (from develop after the URLs and log lists (F5) are merged; the user creates it on GitHub) |
| Status | Plan written (2026-11-22). The user created `feature/frame-export` from develop (b3762c3). Stages 1 to 6 are committed. The CI of stage 6 failed in a 3D test, which is being fixed (2026-11-25). The last feature of the overall plan. The progress of each stage is in the table of "Progress" below |
| Japanese version | [2026-11-22-03_frame-export.md](2026-11-22-03_frame-export.md) (same content) |
| Related | F6 of the overall plan [2026-11-22-00_v20-features_en.md](2026-11-22-00_v20-features_en.md). soccerwindow2's `src/qt/image_save_dialog.cpp` (2D image sequences) |

## Progress

The work and the conditions of the end of each stage are in §5, and the history in §10.

| Stage | Content | Status | Checked by the user | CI | Commits | Pushed |
|---|---|---|---|---|---|---|
| 1 | A 2D image (File > Save Image) (§4.1, §4.10; D4, D5) | Done | Done | Passed | e9a9e96 | Done |
| 2 | The basics of the ranges and image sequences, the export dialog (§4.3 to §4.6, §4.11; D6) | Done | Done | Passed | d7adf47 | Done |
| 3 | Handy ways to give the ranges (§4.4, §4.12; D7 to D12) | Done | Done | Passed | d7a5cd1 | Done |
| 4 | Videos by ffmpeg (§4.5, §4.13; D13 to D16) | Done | Done | Passed | 83ab35e | Done |
| 5 | Rearranging the 3D drawing (refactoring only; §4.2, §4.14) | Done | Done | Passed | ad3687b | Done |
| 6 | Offscreen 3D drawing (§4.2, §4.15; D17, S10 to S15) | Done | Done | Failed | 89b5168 | Done |
| — | A fix of the CI (the test `sameInput` of stage 6: the pictures of two drawers compared within a color difference of 2) | In progress | — | Not yet | — | Not yet |
| 7 | The batch command line (§4.7) | Not started | Not yet | Not yet | — | Not yet |
| 8 | The Qt Multimedia prototype (§4.8) | Not started | Not yet | Not yet | — | Not yet |
| — | Merging into develop (PR, Squash Merge) | Not started | — | — | — | — |

## 1. Goal and approach

- Export what is drawn on the screen as it is, as an image sequence or a video, for both the 2D and the 3D view.
- Image sequences and videos by an external ffmpeg are the main way (Qt5 and Qt6; no new build dependency). In addition, for Qt6 (6.8 or later) only, direct video encoding by Qt Multimedia is prototyped, and whether to keep it is decided by measurement (the user, 2026-11-22).
- **Render offline**: instead of recording the screen while playing, each output frame is drawn again from (frame index, interpolation position) and a fixed time step. The result is the same however fast the machine is. Recording an interactive session is left to screen recorders such as OBS (out of scope).
- "As shown on the screen": the logical size of the output is kept close to the screen, and the resolution is changed by the devicePixelRatio (dpr). Zoom, focus, and the sizes of texts, labels and the HUD look as on the screen.

## 2. Current state (develop 5affd90)

### 2.1 2D view

- `FieldCanvas` (`src/field_canvas.{h,cpp}`) is a `QWidget`. `paintEvent` (line 250) calls the private `draw( QPainter & )` (line 312), which:
  1. `updateFocus()` (line 267): changes the automatic selection of the player nearest to the ball (the selected number in `Options`) and the focus point.
  2. `Options::updateFieldSize( width(), height() )` (options.cpp:2251): changes the canvas size, the field center and (when not zoomed) the scale.
  3. `FieldPainter`, then `ScoreBoardPainter`, `TeamGraphicPainter`, `PlayerPainter`, `BallPainter`, `DrawInfoPainter`.
- The field transform is global state in `Options` (a singleton). There is a pattern to save and restore it: `Options::FieldTransform` and `setFieldTransform()` (options.h:719), used by `GroundTexture` (`src/view3d/ground_texture.cpp:97-104`) to draw the `FieldPainter` into a `QImage`.
- The painters place things by `painter.window()` (field_painter, score_board_painter) and `opt.canvasWidth()` (team_graphic_painter).
- There is no image saving. A commented-out `M_save_image_act` remains (main_window.cpp:1998).

### 2.2 3D view

- `FieldView3D` (`src/view3d/field_view_3d.{h,cpp}`) is a `QOpenGLWindow` (`NoPartialUpdate`) embedded by `QWidget::createWindowContainer()`. It is a separate native window, so a `grab()` of the main window does not capture it.
- `paintGL()` (line 190): decides the interpolation position (the wall clocks `M_newest_clock` and `M_live_tick` in live mode, `LogPlayer::alpha()` for logs), builds the content with `FrameBuilder3D`, moves the camera (the time step of the glide is the wall clock `M_frame_clock`, line 313), draws by `Scene3D::render( defaultFramebufferObject(), … )`, and draws the HUD (score board, team logos, overlay texts, compass, minimap, FPS) with QPainter.
- `Scene3D::render` can draw into any framebuffer. The tests have an example of offscreen drawing with `QOffscreenSurface` and `QOpenGLFramebufferObject` (`test/test_scene_3d.cpp`).
- `test/test_gl_isolation.cpp` forbids raw GL calls (`glXxx(`, `GL_XXX`) outside `src/view3d/gl/`.
- `FieldView3D::unavailableReason()` (line 120) refuses the 3D view on the offscreen platform, etc.

### 2.3 Playback and frames

- One frame (one element of `DispHolder::dispCont()`) is one simulation step (100 ms by default). Frames while the game is stopped (`stime`) are also counted one each.
- `DispHolder::getIndex( cycle )` (disp_holder.cpp:672) returns the first index of a cycle. `setIndex()` (line 635) changes the index.
- In live mode, above 65535 frames, old frames are dropped and the indices shift (`trimLiveFrames`, line 261).

### 2.4 This environment

Ubuntu 24.04 with Qt 6.4.2 and Qt 5.15.13. Neither the development package of Qt Multimedia nor ffmpeg is installed (`qt6-multimedia-dev` 6.4.2 exists as a package). `QVideoFrameInput` appeared in Qt 6.8, so the prototype cannot be built with the Qt of this environment.

### 2.5 soccerwindow2's image saving (for reference)

File > Save Image of soccerwindow2 (`../../rctools/soccerwindow2`; `src/qt/image_save_dialog.cpp`, `ImageSaveDialog`):

- The range is two spin boxes of the start and end cycles (integers) and a "Select All" button. The default end is the first cycle (only one cycle).
- Cycles become indices by `ViewHolder::getIndexOf( cycle )` (the first frame of the cycle; `lower_bound`). The end is converted the same way, so **the stopped frames of the end cycle (the second and later ones) are not included**.
- The dialog is modal, and opening it stops the playback. The range cannot be decided by moving the playback position while the dialog is open.
- Before saving, it shows neither the number of images, the duration nor the disk use. When a file of the same name exists, it asks whether to overwrite for each image (No / Yes / Yes to All).
- The names are the log indices (`image-00042.png`). The image size (width and height), the prefix, the format and the directory can be chosen.
- It can be canceled in a progress dialog, and calls `processEvents` every 20 images. No video and no 3D. The command line (`--auto-image-save`) can save the whole log and quit.

## 3. Decisions

### 3.1 The user's decisions (2026-11-22)

| # | Content |
|---|---|
| D1 | Image sequences and videos by an external ffmpeg are the main way |
| D2 | Direct video encoding by Qt Multimedia is prototyped too, Qt6 only being fine |
| D3 | The range of frames to export can be specified. soccerwindow2's image saving may be referred to, but it should be improved (§4.4) |
| D4 | The design decisions S1 to S9 are as proposed (2026-11-24, decided by the user at the start of stage 1) |
| D5 | The shortcut of File > Save Image... is Ctrl+Shift+I (Ctrl+Shift+E is planned for Export Images / Video... of stage 2). While the 3D view is shown, it is disabled until stage 6 adds saving the 3D view (no image other than the view seen is saved) (the same) |
| D6 | Saving one image and exporting a range stay separate items of the File menu, with names that make their roles clear (option A): "Save Image of This Frame..." (Ctrl+Shift+I; the current frame saved in one step, without the fields of a dialog) and "Export Images / Video..." (Ctrl+Shift+E; a modeless dialog to choose the range, the size and the fps). Compared with one item (as soccerwindow2, one image with the same start and end), the frequent saving of one image takes one step, and the different defaults and kinds of dialogs stay apart (2026-11-24, decided by the user at the start of stage 2) |
| D7 | The shortcuts to mark the start and the end of a range are `[` and `]` (at the position shown). Clearing the marks, seldom used, is in the menu only (2026-11-24, decided by the user at the start of stage 3) |
| D8 | The default seconds of the handy ranges: a goal scene from 10 s before the goal to 5 s after it, and around the position shown from 5 s before to 5 s after. They can be changed in the dialog, and are kept in the settings (the same) |
| D9 | The playback slider shows the marks of the goals too, besides the band of the range chosen (the same) |
| D10 | The positions of the goals follow `DispHolder::scoreChangedIndex()` (the index of the last frame before the new score, kept when the record of the teams comes; "go to the next score" of the log player uses it too). The team and the score are read from the frame after. At first the goals were found again where a score goes up in the frames; at the user's question, it was changed for one way to decide the goals (option B; 2026-11-24, decided by the user before checking stage 3) |
| D11 | In the File menu, the items of the export are grouped in "Export Images / Video": Open the Export Dialog... (Ctrl+Shift+E), Start the Range Here (`[`), End the Range Here (`]`), Clear the Range. With the first "Export Images / Video..." and "Export Range" (Mark the Start, Mark the End, Clear the Marks), it was hard to see that they work together, and what they do. The user also proposed removing the items of the marks from the menu, leaving the dialog the only entry; but menus are where the shortcuts are found, and grouped in one item they are seen to work together, so this was chosen (2026-11-24, asked by the user checking stage 3, decided on Claude's advice) |
| D12 | "All" of the export dialog (the current range over the whole log) is renamed "Clear", as Clear the Range of the menu. The tooltips of the buttons Now tell `[` and `]` of the main window (and their menu items) (the same) |
| D13 | When a video export is stopped, the file being written is removed (an mp4 not written to the end may not play). Image sequences keep their images as before, as each image is complete. With a video for each range, the files finished are kept (2026-11-25, decided by the user at the start of stage 4) |
| D14 | The dialog has only the quality (CRF) and the speed of the compression (preset) as settings of the videos. The codec is H.264 (libx264), and the format is by the extension of the file (.mp4, .mov, .mkv). If ffmpeg has no libx264, another one it has (mpeg4 and so on) is used, and that is told (the same) |
| D15 | The default quality of the videos is CRF 18 and preset medium (as planned: hardly any loss to see; the 2D view has many flat colors, so the files stay small) (the same) |
| D16 | The behavior of "Scale the texts with the image" in the export dialog stays (the dpr of the images is "the height of the image ÷ the height of the 2D view", so the image looks as the view, enlarged), and the check is renamed "Enlarge the texts, logos and lines with the image". The tooltip and the README say that the team logos and the widths of the lines (such as the white circle of a kick) grow too, besides the texts, and that off, they keep their sizes in pixels (smaller and thinner in a large image), and the players move more smoothly. The user saw on the screen that the team logos and the circle of a kick grew larger and thicker too, and asked whether it was meant. Enlarging only the texts (enlarging only the fonts at dpr 1), and separate checks for the texts and the lines, were not taken: the height of the score board, the places of the uniform numbers and so on are laid out in pixels, so many places of the painters would have to change, and they would easily go out of place. The key of the setting `scale_texts` stays (plan A; 2026-11-25, decided by the user at the check of stage 4) |
| D17 | The export dialog chooses the 2D or the 3D view: the one shown on the screen when the dialog is opened, and then changed in the dialog. The choice before is not remembered (plan A; 2026-11-25, decided by the user at the start of stage 6) |

### 3.2 Design decisions (Claude; the user confirms them at the start of the stages)

| # | Content | Reason |
|---|---|---|
| S1 | Render offline (§1). Export is disabled while connected to a server; the frames held after disconnecting can be exported | The result does not depend on the machine's speed. In live mode the indices shift (2.3) |
| S2 | For 2D, the global state changed by drawing (the field transform, canvas size, focus point and selected number in `Options`, and the current index of `DispHolder`) is saved and restored around each frame | The screen's state is not disturbed, and a repaint of the screen between frames is safe |
| S3 | For 3D, the drawing is moved out of `paintGL` into a separate class, owned by both the screen's `FieldView3D` and the exporting `OffscreenView3D` (its own GL context and offscreen surface) | The screen's 3D state (camera, caches) is not disturbed. It can draw without the 3D view on the screen, and under xvfb |
| S4 | `grabFramebuffer()` is not used | With `NoPartialUpdate` it is only correct inside `paintGL` (Qt's documentation) |
| S5 | Raw frames are given to ffmpeg through the standard input of a `QProcess`. The arguments are passed as a list, never through a shell | No new dependency. Safe |
| S6 | The Qt Multimedia prototype is built behind a CMake option (default OFF). Not built by autotools | The prototype does not break the build on the minimum environments |
| S7 | The export settings are in the export dialog, not in a tab of the config dialog, and saved in a new group `Export` | `test_config_dialog.cpp` pins the tab list |
| S8 | The export dialog is modeless, so that the start and end of a range can be taken while moving the playback position with the dialog open. Only during the export, the progress dialog is application modal | soccerwindow2's is modal, and the cycle numbers must be looked up before opening it (2.5) |
| S9 | A range is held as an interval of log indices; it is shown and entered as "cycle" or "cycle.stopped number" (the same notation as the frame label on the screen, `1234.3`) | Cycles alone cannot tell the stopped frames apart |
| S10 | The export draws through a type of drawer, `ExportRenderer` (2D: a wrapper of `FieldCanvas::renderFrame()`; 3D: `OffscreenView3D`). `ExportPart` has the positions of 3D (`positions`) and the images that start a range (`range_starts`) (stage 6) | `ExportJob` does not need to know the drawer, and treats the starts of the ranges the same in any part. The calls and the tests of 2D stay as they were |
| S11 | At the start of a range, only the camera is snapped (`snap()`); no other cache is cleared (the plan's "clear the caches of the interpolation and the motions" was not needed) (stage 6) | The players' motions (`GaitTable`), the ball's turn (`BallSpin`) and the flying ball (`BallFlight`) are found from the frames alone, not by the order they are drawn in (§5.2 of the 3D plan). The image of the labels has a fixed slot for each player too. After other frames, only a few pixels differed by 1 or 2 in their colors, of the shadows (§4.15) |
| S12 | The framebuffer of the export is Qt's `QOpenGLFramebufferObject`; the plan's `gl/offscreen_target` is not made. The alpha of the image read back is not used (stage 6) | No raw GL call is needed (`test_gl_isolation`). The window on the screen has no alpha, so without the alpha that translucent things wrote, the colors are the screen's |
| S13 | The main window has one `OffscreenView3D`, made when first needed and kept, shared by Save Image of This Frame and the export (stage 6) | The figures are read, and the images sent to the GPU, once. Making it for each export takes about a second |
| S14 | The fps is kept for each view (the settings `fps` and `fps_3d`; 30 for 3D by default). The size "As the view" and the ratio of the enlarged texts are taken from the view shown (the 2D canvas or the 3D window). A video of 3D is not given `-tune animation` (stage 6) | 10 for 2D, a frame an image, and 30 for 3D, drawn between the frames too, are natural (4.3). The 3D view is not of flat colors |
| S15 | When an image is smaller than the view and the dpr is below 1, the image's dpr is 1, and the window of the logical size is mapped onto the viewport of the image's pixels. The 2D `FieldCanvas::renderFrame()` is fixed in the same way (stage 6) | QPainter draws a dpr below 1 as 1 (checked). In 2D too, an image smaller than the view (e.g. 1280×720 from a view 900 high) showed only the top left of the layout (a defect since stage 1) |

### 3.3 To decide at the start of the stages

- On cancel, whether to delete the partial file or keep it as a shorter video (stage 4).
- How to build the prototype with Qt 6.8 or later (stage 8): aqtinstall into `~/Qt`, an ubuntu:26.04 container, or a CI job with `continue-on-error`.
- Shortcuts of the menu items (ones that do not clash with the existing shortcuts), including those that set the start and end marks of a range (stage 3).
- The default seconds of goal scenes (proposed: from 10 seconds before to 5 seconds after the goal) and around the current position (stage 3).
- Whether the slider also shows marks at the goals, besides the band of the chosen range (stage 3).

## 4. Design

### 4.1 2D: offscreen drawing

- `Options::ViewState` (new, next to `FieldTransform`): the field scale, the field center, the canvas width and height, the focus point and the selected number. `viewState()` and `setViewState()`. Whether it is zoomed and the focus type are not changed by drawing, so they are not included.
- `src/frame_scope.{h,cpp}` (new): an RAII guard. It saves the current index of `DispHolder` and `Options::viewState()`, sets the given index, and restores both when destroyed. Shared by 2D and 3D. It wraps one frame.
- `FieldCanvas`:
  - `draw( QPainter & )` becomes `draw( QPainter &, const QSize & logical )`; `paintEvent` passes `size()`.
  - `QImage renderFrame( std::size_t index, const QSize & pixels, qreal dpr )` (new, public): fills a `QImage( pixels, Format_RGB32 )` with the field color, calls `setDevicePixelRatio( dpr )`, opens a `FrameScope`, sets the QPainter's window and viewport explicitly to the logical size, and calls `draw`.
  - When zoomed, the scale is adjusted by the ratio of the logical heights so that the same area as on the screen is shown (checked in stage 1).
  - The middle-button measurement is a transient display of an interaction on the screen, so it is not included.
- Limitation: `Options::scale()` rounds to whole logical pixels, so at dpr 2 positions move by 2 pixels (as on the screen). The dialog has a choice "Scale texts with the image"; without it, it draws at dpr 1 (smoother motion, smaller texts).

### 4.2 3D: offscreen drawing

What in `paintGL` depends on the wall clock or on accumulated state:

| Item | For export |
|---|---|
| `M_newest_clock`, `M_live_tick` (live) | Not used. The position is given by `Interpolate::positionAt( count, t )` |
| `LogPlayer::alpha()` | The fractional part of the output frame's time `t` |
| `M_frame_clock` of the camera glide | A fixed `dt = 1 / fps` |
| The state of `CameraController` | Copied from the screen's, then `snap()` (jump to the target pose) at the first frame |
| FPS measurement and display | Not used (no FPS in the output) |
| GPU caches (ground, stadium, stands, figure packs, minimap, label atlas, the MSAA target) | Held separately for export |
| `Options` selection and field size, the current index of `DispHolder` | `FrameScope` |

- **Refactoring (no behavior change)**: move the painters (`FieldPainter`, `ScoreBoardPainter`, `TeamGraphicPainter`), `LabelAtlas`, `FrameBuilder3D`, `Scene3D`, `SceneContent` and the image caches into `src/view3d/view_3d_renderer.{h,cpp}` (new): `initialize()`, `release()`, `build( at, live, delay, mode )`, `subject()`, `render( fbo, pose, logical, dpr, samples )`, `drawHud( painter, … )`. `FieldView3D` keeps the clocks, the camera, the mouse handling and FPS, and its `paintGL` becomes "decide the position → build → move the camera → render → HUD".
- `CameraController::snap()` (new), `FieldView3D::camera()` (new).
- Split `TeamGraphicPainter::update()` out of `draw()`. Today the logo images are made only when the HUD is drawn, so the boards of the first exported frame would have no logos. `build()` calls it first.
- `src/view3d/offscreen_view_3d.{h,cpp}` (new):
  - `unavailableReason()`: the checks of `FieldView3D::unavailableReason()` without the platform check.
  - `start( const CameraController & from, … )`: creates a GL context with `FieldView3D::surfaceFormat()` and a `QOffscreenSurface`, initializes the renderer, copies the camera and calls `snap()`. Without a 3D view on the screen, it makes a new camera from `Options`.
  - `render( at, pixels, dpr, dt, samples )`: `makeCurrent`, `FrameScope`, build, camera update, `render`, read back into an image and convert it to `Format_RGB32`, draw the HUD with a raster QPainter on the image, `doneCurrent`.
  - `renderStill( at, pose, … )`: for the 3D "Save Image", using the screen's current state as it is.
  - At the start of a range (§4.4; when several ranges are joined into one video, at the start of each range), the interpolation and motion caches (the gait table of `FrameBuilder3D`, the ball spin, the ball flight) are cleared and the camera is `snap()`ped. Drawing on from the previous range would make the camera glide or the ball fly between distant scenes.
- `src/view3d/gl/offscreen_target.{h,cpp}` (new): the framebuffer for export (remade when the size changes, with depth and stencil) and the read-back. Raw GL calls stay inside `gl/` (`test_gl_isolation`).
- The size is limited to 3840×2160 (MSAA memory).

### 4.3 Timing model

- `src/export_timing.{h,cpp}` (new, pure functions), for each range (indices `first` to `last`; §4.4):
  - `per_image = speed × 1000 / (step_ms × fps)` (frames advanced per output image)
  - `N = floor( (last − first) / per_image + ε ) + 1`, `t_n = first + n × per_image` (computed from n, not accumulated)
  - 2D: index `= min( last, floor( t_n + ε ) )`. 3D: `Interpolate::positionAt( frames.size(), t_n )`
  - Video duration `= N / fps`
- Defaults: 2D 10 fps (one image per frame), 3D 30 fps (60 can be chosen). A button "As the monitor plays" (`speed = step / the playback timer interval`).
- When several ranges are joined into one video, the frames of the ranges are put in order (the duration is the sum of the ranges).
- By default, images are named by the output frame number (from 0): `PREFIX%0Wd.ext`, `W = max( 5, digits of N−1 )`. They are contiguous, so ffmpeg's image2 can read them (soccerwindow2 names them by the log index, which coincides only for 2D at 10 fps and 1×). Naming by cycle is in §4.4.

### 4.4 The range to export

Based on soccerwindow2 (2.5), as follows.

**Notation of positions, and the meaning of a range**
- A position is written as "cycle" (`1234`) or "cycle.stopped number" (`1234.3`, the same notation as the frame label on the screen). The game time (minutes:seconds; cycle × step) is shown beside it.
- `1234` as the start is the first frame of that cycle, and `1234` as the end is the **last** frame of that cycle (including the stopped frames). `1234.3` is exactly that frame. Both the start and the end are included.
- Cycles not in the log (outside the log, or missing in live mode): the start moves to the next frame there is, and the end to the previous one, and the dialog says so.
- `src/export_range.{h,cpp}` (new, pure functions): parsing and writing positions, converting the start and end to indices (`DispHolder::getIndex( cycle )` and a new `DispHolder::lastIndex( cycle )`), checking ranges, and the common ranges below. A range is held as `{ first, last, name }` (an interval of indices).

**Taking the current position**
- The dialog is modeless (S8). With it open, move the position by the slider, stepping or the cycle field, and take it by "Start at the current position" and "End at the current position".
- The start and end marks of the range can also be set from the main window (menu items and shortcuts; no need to open the dialog), and cleared. The dialog reads the marks, and a range changed in the dialog is given back to the marks.
- The chosen range is shown as a colored band in the groove of the playback slider (`LogPlayerSlider`). The band follows the color scheme (F1).
- The marks and the ranges are kept only for the open log, and cleared when another log is opened or a server is connected (not saved in the settings).

**Common ranges (chosen by buttons or a menu)**
- The whole log.
- Periods: the first half, the second half, each half of extra time, the penalty shootout. Found from the play mode transitions (split at `first_half_over` and `time_over`; from the first `penalty_setup_*` on is the penalty shootout). Periods not in the log cannot be chosen.
- Goal scenes: for each goal, from N seconds before to M seconds after it (from the indices of `DispHolder::scoreChangedIndex()`, cut at the ends of the log). Choose from the list of goals (time, scoring team, the score then), or make one range per goal at once by "All goals".
- Around the current position: from N seconds before to M seconds after the current position.
- The seconds N and M are saved in the settings.

**Several ranges**
- A list of ranges: add, remove, reorder (e.g. "All goals" lists one range per goal).
- How to output them: (a) a separate file (video) or directory (images) per range, named with the range's name (e.g. `PREFIX-goal1-c1234`); (b) joined into one video (highlights). No transition effects (fades etc.) between the ranges (§8).
- With only one range, one file or directory as before.

**Checks before the export**
- Show, per range and in total, the number of frames, the number of output images, the video duration and the game-time interval.
- For image sequences, estimate the disk use from the compressed size of the first image drawn, and compare it with the free space of the destination (`QStorageInfo`). Warn if it is not enough.
- A start after the end, or an empty range, is reported at once, and the export button is disabled.
- Check at once, before the export, whether files with the names to be made already exist, and ask only once (overwrite / choose another place / stop). Do not ask for each image as soccerwindow2 does.

**Image names**
- By default, the output frame number (§4.3).
- In 2D with one image per frame (10 fps, 1×), naming by cycle can also be chosen (`PREFIX-c01234.03.png`), to relate scenes and files in analyses.

### 4.5 Outputs

- `src/frame_sink.h` (new): an interface. `open( pixels, fps )`, `write( QImage )` (accepted / busy now / error), `finish()`, `abort()`, signals `readyForFrame()` and `finished( ok, message )`.
- `src/image_sequence_sink.{h,cpp}` (new): `QImageWriter`. The formats come from `QImageWriter::supportedImageFormats()` (PNG, JPG, etc.).
- `src/ffmpeg_sink.{h,cpp}` (new):
  - Arguments (built by a static function, unit-tested):
    ```
    -hide_banner -loglevel error -nostats
    -f rawvideo -pixel_format bgr0 -video_size WxH -framerate F -i pipe:0
    -an -c:v libx264 -preset P -crf C [-tune animation for 2D]
    -vf scale=out_color_matrix=bt709:out_range=tv,format=yuv420p
    -colorspace bt709 -color_primaries bt709 -color_trc bt709
    [-movflags +faststart for .mp4/.mov] -y OUT
    ```
    The frames are `QImage::Format_RGB32` (`bgr0` on little endian, `0rgb` on big endian). The video size is rounded to even numbers (yuv420p). Defaults: CRF 18, preset `medium`.
  - Backpressure: the next frame is not drawn until `QProcess::bytesToWrite()` drops below two frames (waiting for `bytesWritten`). `waitForBytesWritten` blocks the event loop, so it is not used.
  - Where ffmpeg is: the setting (if empty, `QStandardPaths::findExecutable( "ffmpeg" )`). When the dialog opens, `-version` (show the first line) and `-encoders` (whether libx264 exists) are run, and only the available encoders are offered.
  - Errors: keep the tail (16 KB) of the standard error and show it as the details when it fails to start, exits with non-zero, or exits early.
  - Finishing: after `closeWriteChannel()`, wait for `finished` ("Finishing the video"). Cancel: `kill()` and the partial file is handled as decided (§3.3).
  - No audio.
- `src/export_job.{h,cpp}` (new): a `QObject` that takes a drawer (2D: `FieldCanvas`, 3D: `OffscreenView3D`) and an output, and draws and passes one frame at a time inside the event loop. Signals for progress and the end. Playback is stopped before exporting.

### 4.6 UI

- File > Save Image of This Frame... (D6; Save Image... when planned): saves the current view as one PNG or JPG. 2D: `renderFrame( current index, screen size × dpr, dpr )`; 3D: `renderStill`.
- File > Export Images / Video...: `src/export_dialog.{h,cpp}` (new). Modeless (S8).
  - View: 2D / 3D (3D only with `HAVE_3D_VIEW` and an empty `OffscreenView3D::unavailableReason()`).
  - Range (§4.4): the list of ranges, and the start and end of the chosen range (cycle or cycle.stopped number, and the time). "Start at the current position", "End at the current position", the common ranges (whole, periods, goal scenes, around the current position), adding and removing ranges, and the choice of one output per range or joined. The number of images and the duration per range and in total, and the estimated disk use.
  - Size: as shown (W×H), 1280×720, 1920×1080, 3840×2160, custom; "Scale texts with the image".
  - fps and speed; the "As the monitor plays" button.
  - Output: video (file, CRF, preset, the ffmpeg path and its status; when the prototype is built, the encoder choice "ffmpeg (external) / Qt Multimedia (prototype)") or images (directory, prefix, format).
  - A progress dialog (application modal): n/N, the remaining time, cancel.
- Range marks in the main window: "Set Range Start", "Set Range End", "Clear Range Marks" (menu items and shortcuts). The slider shows the band of the range (§4.4).
- Enabled only when there are frames and no server is connected (updated when a log is opened, and on connecting and disconnecting).

### 4.7 Command line (batch)

- `--export-video FILE` | `--export-images DIR`, `--export-view 2d|3d`, `--export-range RANGE` (may be given many times), `--export-join` (join the ranges into one video), `--export-fps`, `--export-speed`, `--export-size WxH`, `--export-scale R`, `--export-image-format`, `--export-image-prefix`, `--export-crf`, `--export-preset`, `--ffmpeg PATH`. Kept as settings that are not saved.
- `RANGE` is written as `START-END` (`1234-1500`, `1234.3-1500`, etc.; the notation of positions of §4.4), `all`, `first-half`, `second-half`, `extra-1`, `extra-2`, `penalties`, or `goals[:N:M]` (per goal, from N seconds before to M seconds after). Without it, `all`.
- In batch mode, it reads the log without showing the window (by the path that shows no message box), exports, and returns an exit code. The progress goes to the standard error.
- 2D runs with `QT_QPA_PLATFORM=offscreen`. 3D needs a GL context, so it runs under `xvfb-run` (`OffscreenView3D` makes no `QOpenGLWindow`, so xvfb is enough). On the offscreen platform the 3D view on the screen is refused, so the 3D choice is read before `init()`.

### 4.8 The Qt Multimedia prototype (Qt 6.8 or later)

- CMake: `option( ENABLE_QT_MULTIMEDIA_EXPORT "..." OFF )`. Only with Qt6 6.8 or later, `find_package( Qt6 QUIET COMPONENTS Multimedia )`. `#cmakedefine HAVE_QT_MULTIMEDIA_EXPORT` in `cmake-config.h.in`. Not built by autotools.
- `src/qt_multimedia_sink.{h,cpp}` (new; one `FrameSink`):
  - `QVideoFrameFormat` (the pixel format matching `Format_RGB32`, `setStreamFrameRate`), `QVideoFrameInput`, `QMediaCaptureSession`, `QMediaRecorder` (`QMediaFormat::MPEG4` and H.264, checking `isSupported( Encode )` first, `ConstantQualityEncoding`, `VeryHighQuality`, resolution and fps, `setAutoStop( true )`).
  - Backpressure: if `sendVideoFrame()` returns false, wait for `readyToSendVideoFrame` and send the same image again.
  - End: send an empty `QVideoFrame`.
- How to build it (decided in §3.3): aqtinstall (`aqt install-qt linux desktop 6.8.x linux_gcc_64 -m qtmultimedia qtwebsockets -O ~/Qt`, `CMAKE_PREFIX_PATH`), an ubuntu:26.04 container (first check that `qt6-multimedia-dev` is 6.8 or later and which package has the FFmpeg plugin), or a CI job with `continue-on-error`.
- **What to measure to decide whether to keep it**:
  1. Speed: total time, CPU time and peak memory for a full 2D game (1280×720, 10 fps) and one minute of 3D (1920×1080, 60 fps). Is it at least 80% of ffmpeg's speed?
  2. Quality and size: SSIM against the image sequence at the same size, and the file size.
  3. Correctness: frame count, fps, pixel format and color space, and faststart by ffprobe. Playback in Firefox, Chrome and VLC.
  4. Failure handling: no encoder, cancel in the middle, a full disk.
  5. Dependency weight and reach: Qt Multimedia and the FFmpeg libraries, Qt 6.8 or later only, the benefit of not needing ffmpeg on Windows and macOS.
- Record the results in this document, and decide whether to keep it or not to merge it (the prototype commits are moved to a separate branch).

### 4.9 Autotools

Only the 2D parts (stages 1-4 and 7) are built. The 3D and Qt Multimedia parts are inside `HAVE_3D_VIEW` and `HAVE_QT_MULTIMEDIA_EXPORT`.

### 4.10 The implementation of stage 1 (2026-11-24)

4.1, and File > Save Image (2D) of 4.6, were implemented as follows. What was decided beyond the plan is written too.

- `Options::ViewState` (`options.h`): the scale, the center of the field, the size of the canvas, whether the team logos and the score board were shown when the scale was fit, the focus point, and the selected number. `viewState()` and `setViewState()`.
  - `updateFieldSize()` kept whether the logos and the score board were shown when the scale was fit in static variables in the function. Drawing another frame changed them to the current items, so when an image was saved right after the items shown changed, the view set back used its old scale (found by the tests). They are members of `Options` (`M_sized_with_team_graphic`, `M_sized_with_score_board`) in `ViewState`.
- `DispHolder::restoreIndex()` (new): sets the current index back as it was, an invalid one too (`INVALID_INDEX`: the newest frame is drawn), because `setIndex()` does not take an invalid index.
- `src/frame_scope.{h,cpp}` (new): `FrameScope`. Made, it keeps the current index and `viewState()` and changes the index; destroyed, it sets both back. An index out of the frames is ignored (the current frame is drawn).
- `FieldCanvas`:
  - `draw( QPainter &, const QSize & logical )`. `paintEvent` gives `size()`.
  - `renderFrame( index, pixels, dpr )`: an image of `Format_RGB32` filled with the color of the field, `setDevicePixelRatio( dpr )`, and a `FrameScope`. When zoomed and the logical height differs from the view's, the scale follows the ratio of the heights. The window and the viewport of the QPainter are the logical size, and `draw` is called. The line of the measure by the middle button is not drawn. An empty size gives a null image.
  - The painters were checked: each looks only at `DispHolder::currentDisp()`, and keeps no state across frames (the cache of the team logos is by the team's name).
  - The antialiasing of the texts: Qt uses subpixel antialiasing, by the font settings, only when drawing for a widget (through `paintEvent`, even into an image by `QWidget::render()`), and grayscale when drawing straight into an image. So the texts of the image do not match the view's pixel by pixel, and with Qt 5 their advances differ a little too. Grayscale suits the exported images and videos better (no color fringes when scaled or compressed), so it is kept.
- `MainWindow`:
  - File > Save Image... (Ctrl+Shift+I, `save_image`; renamed "Save Image of This Frame..." by D6 at the start of stage 2), after Save a Copy of Game Log in the File menu. Enabled only while the 2D view is shown (D5). `updateSaveImageAction()` is called after `toggleView3D()` changes the view. While the 3D view is shown, the reason is in the status tip and the tooltip.
  - The current frame when chosen is drawn into an image first, and then the file dialog is shown (the log goes on playing behind the dialog). The image is the size of the 2D view times the screen's dpr (`renderImage()`).
  - The default name is the game log's name (without `.rcg` or `.rcg.gz`; `rcssmonitor` in live) and the cycle (`1234.3` when stopped), e.g. `head_v5-1.png` (`defaultImageName()`). Without an extension, PNG. The format is by the extension (`QImageWriter`). The directory saved to is remembered while the program runs (as Save a Copy).
  - It works in live too: one image does not meet the reasons to draw offline (S1: results independent of the machine's speed, shifting indexes). Without a frame, "There is no frame to save." in the status bar.
  - `saveImage( path, error )` is public (for the tests).
- The test `field_canvas_render` (new):
  - Without the texts (the player numbers and types, the score board), the image matches both `QWidget::render()` and `grab()` pixel by pixel. With the texts, they are drawn, and the same conditions give the same image.
  - After drawing another frame, the view is unchanged, and showing that frame gives the same image.
  - The state of the focus on the ball, the zoom, the automatic selection, and the current index (`INVALID_INDEX` too) are set back (by `FrameScope` alone too).
  - dpr 2: the size and the dpr of the image, the score board at the bottom, the goal line at twice the position. The scale when zoomed. An empty size.
  - Save Image of the main window: the shortcut, the File menu, no frame, the size and the content of the image, the name, PNG and JPEG, a place that cannot be written.
  - The test of the menu item is in this test, which uses the same log and images, not in `test_toolbar` as planned. `test_view_switch` got a check that Save Image stays enabled when the 3D view is not available and the 2D view stays. Being disabled while the 3D view is shown is checked by hand, as offscreen cannot make the 3D view.
- Checked (2026-11-24): with a real game log (RoboCup 2026), saved offscreen with `QT_SCALE_FACTOR` 1 and 2. With 2, the image was 2000×1226, with the texts and the team logos at twice the resolution, and the score board at the bottom.

### 4.11 The implementation of stage 2 (2026-11-24)

The basics of 4.3 and 4.4, the image sequences of 4.5, and the export dialog of 4.6 (2D, images, one range) were implemented as follows. What was decided beyond the plan is written too.

- `src/export_timing.{h,cpp}` (new): the static-only class `ExportTiming`. `framesPerImage()`, `imageCount()`, `position()`, `frameAt()`, `frames2D()` (the frame of each 2D image), `duration()`, `nameDigits()`, `imageName()`. The positions are computed from n (not added up).
- `src/export_range.{h,cpp}` (new): `ExportPosition` (a cycle and a stopped frame) and `ExportRange` (a range of indexes, and static functions). `parsePosition()`, `positionText()`, `startIndex()`, `endIndex()`, `gameTime()`.
  - `DispHolder::lastIndex( cycle )` of the plan was not made: pure functions on a `DispCont` (in the order of the cycles and the stopped frames) instead, as the tests can make the frames.
  - A start is the first frame at or after the position, and an end the last frame at or before it (for a cycle, up to its stopped frames). When there is none, it is moved, and that is told.
- `src/frame_sink.{h,cpp}` (new): the interface `FrameSink` (`open`, `write` answering ACCEPTED / BUSY / FAILED, `finish`, `abort`, `readyForFrame`, `finished`). `frame_sink.cpp` is for moc (AUTOMOC of CMake may miss a header without its .cpp).
- `src/image_sequence_sink.{h,cpp}` (new): `ImageSequenceSink`. Writes an image at a time with `QImageWriter` in a directory (made if it is not there) under the names given. When aborted, the images written are kept (the question of aborting in §3.3 is for the videos of stage 4). The formats are PNG and JPEG (`formats()`).
- `src/export_job.{h,cpp}` (new): `ExportJob`. After `start()`, draws an image at a time with `FieldCanvas::renderFrame()` in the event loop and gives it to the sink (`QTimer::singleShot( 0 )`). On BUSY, waits for `readyForFrame`. Signals of the progress and the end. `abort()` tells the end at once.
- `src/export_dialog.{h,cpp}` (new): `ExportDialog` (modeless).
  - The range: the fields of the start and the end (empty: the first and the last frames), "Now" (the position shown), "All" (both empty). Beside the fields: the position moved to and the game time, that it was moved, and errors.
  - The images: the size (as the view, 1280×720, 1920×1080, 3840×2160, any). "Scale the texts with the image" (renamed "Enlarge the texts, logos and lines with the image" in stage 4; D16) makes the dpr "the height of the image ÷ the height of the 2D view" (the texts in the same proportion as in the view); off, dpr 1. The speed and the fps (10 images/s and ×1 by default). The button "the same speed as the monitor's playback" is not made yet.
  - The output: the directory, the start of the names, the format. "Name the images by the cycles" can be chosen only when an image is a frame. The names are `PREFIXc01234.00.png` (the stopped frame always in 2 digits or more, for the order of the names to be the order of the frames).
  - The summary: the numbers of the images and the frames, the game time of the range, the size, the length of a video. On an error, its reason, and Export cannot be pressed.
  - Export: the files already there under the names are counted at once, and asked about once (overwrite / another directory / cancel). The first image drawn and compressed, times the images, times 1.1, is the estimate, and a warning is given if it is more than the free space (`QStorageInfo`). The settings are saved, the playback stopped (`stopRequested`), and the `ExportJob` started. The progress is an application-modal `QProgressDialog` (Stop aborts). As `setValue()` of a modal one processes the events, the text is set first and the value last, and at the end it is removed by `deleteLater()` (avoiding the crash found in stage 1 of F5).
  - Nothing is exported while connected (S1: the button disabled, with the reason).
  - With the 3D view shown, the 2D images are exported (the group is titled "Images (2D view)"). 3D in stage 6.
- `Options::ExportSettings` and `writeExportSettings()` (the group `Export`; S7): the directory, the start of the names, the format, the size, the custom size, scaling the texts, the fps, the speed, the names by the cycles. Written when an export starts.
- `MainWindow`: File > Export Images / Video... (Ctrl+Shift+E, `export`; after Save Image of This Frame). Enabled only with frames and while not connected (`updateExportAction()`, called when a log is opened, and when connecting and disconnecting). The dialog is made when first shown, and `viewUpdated` updates its summary.
- Tests:
  - `export_timing` (new): the frames an image advances (2D, 3D, speeds, steps), the counts, the frames of 2D (half the speed, 30 fps, the end of a long range), the length, the digits of the names.
  - `export_range` (new): reading positions (wrong ones too), writing them, the indexes of the starts and the ends (stopped frames, missing cycles, out of the log, moved), the ranges, the game time.
  - `image_sequence_sink` (new): files under the names (PNG, JPEG, the directory made), a directory that cannot be made, more images than names. `ExportJob`: the images the same as `renderFrame()`, drawn after `start()`, the view's state unchanged, aborted (the images written kept), an error of the sink, no frames.
  - `export_dialog` (new): the menu (the shortcut, after Save Image of This Frame, enabled with a log, modeless, only one), the range (empty fields, Now, moved, wrong ones cannot be exported), the export (the names and the sizes of the files, the playback stopped, the view unchanged, the settings saved, the status), the names by the cycles (when they can be chosen, half the speed), the sizes (as the view, 1920×1080, scaling the texts), the files already there (asked once; cancel writes nothing), while connected.
  - The test of the view unchanged waits for the rows of the tool bars laid out again after opening the log (a timer of 0 ms) before comparing. Without waiting, the size of the 2D view changed during the export.
- Checked (2026-11-24): 1000 to 1299 of a real game log (RoboCup 2026; 330 frames with the stopped ones) were exported as PNG of 1920×1080 in 10.3 s (31 ms an image), with about 110 MB of memory, and 35 MB on the disk (106 KB an image). The texts and the logos of the images were in the same proportion as in the view. The dialog was drawn into an image and looked at, and the widths of the fields were fixed (the start and the end as wide, the texts beside them in one line, cut when long).

### 4.12 The implementation of stage 3 (2026-11-24)

The handy ways to give the ranges of 4.4 were implemented as follows. What was decided beyond the plan is written too.

- The parts of a game are found in another way: rcssserver 19 makes the play mode `before_kick_off` at the end of a half (`Stadium::callHalfTime()`), and does not use `first_half_over`. Moreover, a real log had no `before_kick_off` recorded at the end of a half either (checked with a game of 2022 with extra time). So the plan's "split at `first_half_over` and `time_over`" was dropped: the parts are split at the cycles given by the server's parameters in the log (`half_time`, `nr_normal_halfs`, `extra_half_time`, `nr_extra_halfs`, `simulator_step`): the first half 0 to 3000, the second 3001 to 6000, and the extra time 1000 cycles each. The penalty shootout is from its first play mode on. Only the parts with frames are given. If the halves have no time, the whole game is one part.
- `ExportRange` (added): `periods()`, `goals()` (for each index of `DispHolder::scoreChangedIndex()`, the team and the score of the frame after it; the side that scored by the score of the goal before; changes where no score goes up left out; D10), `around()` (some seconds before and after a cycle, cut by the ends of the log), `cyclesOf()`.
- `src/export_range_list.{h,cpp}` (new): `ExportRangeList` (the model of the ranges; a `QObject`) and `ExportRangeSpec` (the texts of the positions of the start and the end, and a name). One or more ranges, and the current one. `resolve()` (from the texts to frames), `newName()`, `safeName()` (for a directory). The main window has it; the marks, the dialog and the slider see the same list. Cleared when another log is opened or a server is connected.
- `MainWindow`: in File > Export Images / Video, Open the Export Dialog... (Ctrl+Shift+E), Start the Range Here (`[`), End the Range Here (`]`), Clear the Range (D7, D11; at first Mark the Start, Mark the End and Clear the Marks of File > Export Range). The menu items show their tooltips too. They set the start and the end of the current range at the position shown, told in the status bar. Enabled as Export. The slider is a member, and whenever the list changes it gets the bands and the goals (`updateRangeMarks()`). The whole log (both ends empty) is not a band. Nothing is shown while connected.
- `LogPlayerSlider`: `setBands()` and `setGoals()`. After the groove, the bands of the ranges (the current one darker) and the ticks of the goals are drawn, and the handle last (with the parts drawn given explicitly, as `QSlider::paintEvent()`). The bands in the highlight of the scheme, the goals in an orange by the scheme (D9).
- `ExportDialog`:
  - The list of the ranges (`QListWidget`: the name, the start and the end, the frames or an error), with Add, Remove, Up and Down. Clear makes the current range the whole log (All at first; D12). The tooltips of the buttons Now tell `[` and `]` of the main window and their menu items (D12). The fields of the name, the start and the end of the current range follow the list both ways.
  - Presets (a menu): the whole log, the parts (the halves, the extra time, the penalty shootout, with the game time), each goal (the time, the team, the score), all the goals (a range each: in place of the list if it is the whole log alone, after it otherwise), around the position shown. The names are `first-half`, `goal1-c6713`, `around-c1234` and so on. The fields of the seconds (before and after a goal, before and after the position shown; D8) are kept in the settings.
  - With several ranges: each in its own directory (its name made safe by `safeName()`, `-2` and so on for the same names), or all in one sequence (the numbers go on over the ranges; an error if the names by the cycles repeat). With one range, one directory as before.
  - The summary is the total of all the ranges. An error is given with the name of its range.
- `ImageSequenceSink`: a name may have a directory (the directory of a range, made when its first image is written).
- `Options::ExportSettings` (added): `separate_ranges`, `goal_before`, `goal_after`, `around_before`, `around_after`.
- Tests:
  - `export_range` (added): the parts (two halves, the extra time and the shootout, a log starting in the second half, after the last end, halves without time, no frames), the goals (the indexes as `DispHolder` keeps them, indexes at the frames with the new scores, changes where no score goes up, indexes out of the frames), around (stopped frames, the ends of the log, a step of 50 ms).
  - `export_range_list` (new): editing, adding, removing and moving (the current one kept), the names, from the texts to frames.
  - `log_player_slider` (new): the order and the ends of the positions, the bands (the current one darker), the color of the goals (light and dark), unchanged outside the bands.
  - `export_dialog` (added): the marks (`[` and `]`, the menu, shown in the dialog and on the slider, cleared, gone with another log), the presets (a game of two halves and two goals made of live JSON frames), several ranges (directories of their own, the same names, one sequence, repeated names by the cycles, a wrong range, the buttons of the list).
- Checked (2026-11-24): with a log of a game of 2022 with extra time, the parts were the first half (frames 0 to 3147), the second half, extra time 1 and extra time 2, and the goal (6713, HELIOS2022) was found. All the goal scenes (6613 to 6763, 201 images with the stopped frames) were exported at 1280×720 in 3.1 s. After D10, a log of 2023 (Oxsy vs YuShan2023) had 4 goals found, whose cycles (437, 797, 1075, 3536) were those of `goal_l` and `goal_r` in the play modes of the log. The 4 ranges (804 images) were exported each in its own directory in 12.9 s. The dialog and the slider were drawn into images and looked at.

### 4.13 The implementation of stage 4 (2026-11-25)

The videos by ffmpeg of 4.5 and the fields of the videos of 4.6 were implemented as follows. What was decided beyond the plan is written too.

- `src/ffmpeg_sink.{h,cpp}` (new): `FfmpegSink`.
  - The arguments are those of 4.5 (the static `arguments()`). The frames are the lines of `QImage::Format_RGB32` written to the standard input as they are (`pixelFormat()`: `bgr0` on little-endian machines). It does not open for odd sizes of the video.
  - Backpressure: while two frames or more wait to be written, `write()` answers BUSY. When `bytesWritten` brings it below, it emits `readyForFrame()`.
  - The end: `finish()` closes the standard input, and when ffmpeg ends with 0, the end is told with the path of the file. When it ends with another code, crashes, or ends before the end of the input, the file being written is removed, and the error is told with the end of the standard error (16 KB kept, 1500 characters shown). A failure to start is told only by the return of `open()` (not told twice).
  - `abort()`: ffmpeg stopped and the file removed (D13).
  - Where (`findProgram()`): `ffmpeg` in the PATH when empty, an executable file for a path, and a name without `/` looked for in the PATH.
  - Probing (`probe()`): `-hide_banner -version` (the first line) and `-encoders` (the names of the video encoders), waiting 5 s at most for each. It chooses libx264, libopenh264 and mpeg4 in this order (the other encoders of D14). mpeg4 gets `-q:v 2`, and libopenh264 `-b:v 10M`.
- `ExportJob`: the export is a list of "parts" (a sink and its frames; `ExportPart`). The parts are opened, written and finished one after another. When stopped, only the sink of the current part is aborted, and the finished parts are kept (D13). The progress is of all the images, and `done == total` means "the sinks are finishing". The message of the end has a line for each part.
- `ExportDialog`:
  - The output: the radio buttons "Images" and "Video (ffmpeg)" switch between the fields of the images (the directory, the names, the format, naming by the cycles) and those of the video: the file and Browse, Quality (CRF; 0 to 51), Encoding speed (the preset), the path of ffmpeg and Browse, and the state of ffmpeg. The texts of the choice of the ranges change too (A video for each, by its name / All in one video). The radio buttons are put in `QButtonGroup`s of two (in the same group box, the choice of the output and that of the ranges were exclusive with each other, and reading the settings lost the choice of the ranges).
  - ffmpeg is probed when a video is chosen first (not run for those who export only images). It is probed again when its path is edited or chosen. The state shows the version and the encoder used. When it is not found, the reason and "install it, or choose it" are shown in red. Without libx264, the encoder used, and that the CRF and the preset are not used, are shown, and those two fields are disabled.
  - The video file: `.mp4` is added when it has no extension. Other than `.mp4`, `.mov` and `.mkv` is an error (in the summary, and Export disabled). The videos of the ranges are `FILE-NAME.ext` (the names of the ranges by `safeName()`, with `-2` and so on for the same names; the same `rangeFileNames()` as the directories of the images). With one range, it is `FILE` as it is.
  - The size: rounded down to even numbers for a video (yuv420p).
  - The button "As the Playback" (4.6): the speed is set to the step / the interval of the playback (`Options::timerInterval()`; the playback goes a frame each interval).
  - The summary: for a video, "a video of N s at F images/s" or "K videos, N s in all at F images/s". The errors of ffmpeg and of the extension are in the summary too.
  - Before the export: if the video files are there, it asks once (overwrite, another file, cancel). The disk space of the videos is not estimated. As it is 2D, `-tune animation` is given.
  - The progress: titled Export Video. After 2 s, the time left at the speed so far is shown (the "time left" of 4.6; for the images too). After all the images are given, "Finishing the video...".
  - The end: "Exported the video FILE." or "Exported K videos in DIR.". The status bar has the same one line.
  - The preparation of the export is split into the images (`prepareImages()`) and the videos (`prepareVideos()`).
  - The check to enlarge the texts (D16): renamed "Enlarge the texts, logos and lines with the image". The tooltip says that the image looks as the view, enlarged (the texts, the team logos and the widths of the lines grow with the image), and that off, they keep their sizes in pixels (smaller and thinner in a large image) and the players move more smoothly. The behavior, the object name and the key of the setting (`scale_texts`) stay.
- `Options::ExportSettings` (added): `video`, `video_file`, `crf`, `preset`, `ffmpeg`.
- CI: the Qt6 job of CMake installs `ffmpeg`, and `RCSSMONITOR_REQUIRE_FFMPEG=1` makes the test with the real ffmpeg run (failed without it). The other jobs skip it.
- Documents: README (a part on the videos), `docs/build.md` (ffmpeg, optional at run time), NEWS, ChangeLog.
- Tests:
  - `ffmpeg_sink` (new): the arguments (the whole of libx264, without tune, `.mkv`, 12.5 fps, mpeg4, libopenh264). Finding the program. Probing (the version, the choice of the three encoders, an audio "libx264" not taken, no encoder, failing, missing). Writing to a fake ffmpeg (a shell script made in the test): the arguments, the number of bytes, the colors. Backpressure (BUSY and `readyForFrame()`; no slow fake ffmpeg was made: checked by writing without running the event loop). Not opened (odd sizes, the directory, not started; a file there kept). Errors (at once, in the middle of the input; the end of the standard error, the file removed). Stopped (the file removed, no end told). Several videos (written one after another; the first kept when the second is stopped). The real ffmpeg (by ffprobe: h264, the size, yuv420p, the number of frames, a length of 0.4 s; mp4 and mkv; the error of a wrong preset).
  - `export_dialog` (added): a video (the fields switched, the default CRF and preset, the state, `.mp4` added, the CRF and the preset in the arguments, even sizes, the summary, the size of the file, the settings, a wrong extension, asking about a file there), several videos (the names of the files of the ranges, the same names, one for all, the settings read again), no ffmpeg (the state, the summary, Export disabled, the images still exported, the text and the arguments with mpeg4), the button of the playback speed.
- Checked (2026-11-25): the 4 goal scenes of a log of 2023 (Oxsy vs YuShan2023) were exported at 1280×720 as 4 mp4 files, one for each range, in 1.9 s. By ffprobe, they were H.264 (High), yuv420p, bt709 (the tv range), 10 fps, 201 frames and 20.1 s each. As one video, 804 frames and 80.4 s (the sum). The whole first half (3299 frames with the stopped ones) was exported at 1920×1080 in 14.3 s, 24 MB. Frames taken out of the videos were looked at, and were the same pictures as the view. The dialog was drawn into an image and looked at.
### 4.14 The implementation of stage 5 (2026-11-25)

The refactoring of 4.2 was implemented as follows. The 3D view looks the same on the screen (the only change: the team logos are on the boards from the first frame). What was decided beyond the plan is written too.

- `src/view3d/view_3d_renderer.{h,cpp}` (new): `View3DRenderer`.
  - It keeps: the 2D painters (`FieldPainter`, `ScoreBoardPainter`, `TeamGraphicPainter`), `LabelAtlas`, `FrameBuilder3D`, `Scene3D`, `SceneContent`, the caches of the images (the ground, the boards and banners of the ends, the stands, the pitch of the minimap) with their keys, and which figure pack's bodies were sent.
  - `initialize()` (makes the GPU's resources in the current context and reads the figures once; in a new context, the images are sent again), `release()`, `ready()`, `setTiming()`, `takeGpuTime()`.
  - `build( at, live, delay_frames, mode )`: the automatic selection of a player, the images (the ground, the ends, the stands; only when they change), `FrameBuilder3D::build()`, the bodies of the figures to the GPU.
  - `subject()`: what the cameras follow.
  - `render( fbo, camera, logical, dpr, samples )`: adds the number labels, the cards and the aids of the ball, sized for the camera and the height of the view, sends the image of the labels, and draws by `Scene3D::render()` (the arguments of the plan's `render`; the labels are made once the camera is known, so they are here, not in `build()`).
  - `drawHud( painter, camera, logical, dpr, mode )`: the score board, the team logos, the texts of the overlays, the compass and the minimap. The caller sets the window and the viewport of the QPainter to the logical size.
  - `minimapShown( mode )` (static): for where the FPS is shown (the FPS is of the screen only, so it stays in `FieldView3D`).
  - `buildTime()`, `submitTime()`: the CPU times, measured in the renderer (`RCSSMONITOR_3D_TIMING`). The build time is "the making part of `build()` + the labels of `render()`", and no longer includes the update of the camera (microseconds).
  - `state( camera )`, `targetAt( camera, size, point )`: the state for the tests, and the target of a right click.
  - The messages to the standard error (`(FieldView3D) OpenGL 3.3 core is not available` and so on) stay.
- `FieldView3D`: keeps the clocks (the position between the frames, the live clock, the time of the camera's glide), the camera, the mouse, the FPS and the timing, and `paintGL` is now "decide the position → `build` → move the camera → `render` → HUD (`drawHud`, FPS)". `camera()` (new) is for the export of stage 6 to copy the camera.
- `TeamGraphicPainter::update()` (new): the part making the images of the logos from the tiles is separated from `draw()`. `draw()` calls it and then draws (2D does not change). `View3DRenderer::build()` calls it before making the boards of the ends, so the boards have the logos from the first frame (before, from the frame after the HUD drew the logos).
- `CameraController::snap()` (new): the next `update()` jumps to where the camera goes (as after a change of the camera; the player camera starts again). The free and the top cameras stay as they are.
- Found and fixed on the way: the constructor of `DispHolder` did not initialize `M_playmode` (only `clear()` set it to `PM_Null`). For a frame before any play mode, the score board read the array of the names of the play modes at an undefined index. The new test crashed, which showed it. The constructor sets `PM_Null` now (the application calls `clear()` before reading, so the screen does not change).
- Tests:
  - `view_3d_renderer` (new; skipped without an OpenGL 3.3 context): initializing, the steps `build` → camera → `render`, the same frame is the same image, another frame is another image, the state (22 players, the position, labels), the color of the ground. `drawHud` draws the score board at the bottom, and nothing in the middle. The logos are on the boards from the first frame (with the camera behind the right goal, the first frame equals the next one after the HUD is drawn; checked to fail without `update()`).
  - `team_graphic_painter` (new): `update()` alone makes the logos (the size, the color, made again with more tiles), `draw()` draws at the top left, and nothing is made with the option off or without a frame.
  - `cameras_3d` (added): `snap()` (the broadcast and the close-up cameras jump, then glide again; the free camera does not move).
  - `disp_holder` (added): the play mode of a new `DispHolder` is `PM_Null`.
- Checked (2026-11-25): on Xvfb (llvmpipe), the monitors before (83ab35e) and after the change opened the same 2023 log, and the 3D view (1100×624) was read from its framebuffer and compared pixel by pixel. With the broadcast camera at the cycles 1, 437, 1000 and 2500, and the close-up (ball), top and player cameras (with the compass and the minimap) at the cycles 437 and 1000, no pixel differed.

### 4.15 The implementation of stage 6 (2026-11-25)

The offscreen drawing of 3D of 4.2, the 3D of the dialog of 4.6, and the 3D Save Image were implemented as follows. What was decided beyond the plan is in S10 to S15 too.

- `src/view3d/offscreen_view_3d.{h,cpp}` (new): `OffscreenView3D` (an `ExportRenderer`).
  - It has its own `QOpenGLContext`, `QOffscreenSurface` and `View3DRenderer`, made once by `initialize()` (which gives the reason if not). It needs no window, so it draws on the platform offscreen too.
  - `unavailableReason()` (static): tries a context of OpenGL 3.3 core and its functions. `FieldView3D::unavailableReason()` now calls it after checking the platform.
  - An image (`renderAt( position, pixels, dpr, dt )`): in a `FrameScope` (which sets back the current index and `Options::viewState()`), the position by `Interpolate::positionAt()`, `View3DRenderer::build()`, the camera's `update( dt )`, and `render()` into a `QOpenGLFramebufferObject` of the image's size (made again when the size changes); read back, without the alpha, as `Format_RGB32`, and the HUD drawn by QPainter (S12, S15).
  - The export: `start( camera, fps )` copies the screen's camera, moved 1/fps seconds an image. `startRange()` snaps the camera (S11). `render( part, i, … )` draws at the position of `ExportPart::positions`.
  - `renderStill( index, camera, pixels, dpr )`: for Save Image of This Frame. Draws with a copy of the camera, not moved (`dt = 0`).
  - The largest size `MAX_WIDTH` × `MAX_HEIGHT` (3840×2160).
- `ExportJob`: draws through the type of drawer `ExportRenderer` (`startRange()`, `render( part, i, pixels, dpr )`) (S10). For 2D, a wrapper of `FieldCanvas`, with the constructors as they were. At the start of a range (`ExportPart::startsRange()`: the first image of a part and `range_starts`), it calls `startRange()`. An image not drawn (null) stops it and tells so ("Could not draw the image N.").
- `ExportTiming`: `positions3D()` (the positions between the frames; within `EPSILON` of a frame, that frame; not after the end of the range), `DEFAULT_FPS_3D` (30).
- `ExportDialog`:
  - The box is named "Images", with "View:" (2D view / 3D view; a `QButtonGroup`) as its first row. The view shown on the screen when the dialog is opened is chosen (D17; `MainWindow::showExportDialog()` calls `chooseView3D()`).
  - `setView3DSources( view, offscreen )`: the 3D window (its size and camera) and the drawer. Only when the drawer changes, `OffscreenView3D::unavailableReason()` is checked; when it cannot draw, the choice of 3D is disabled, with the reason in its tooltip. Built without 3D: "This rcssmonitor is built without the 3D view."
  - The fps is kept for each view (S14): changing the view swaps the field's value and the other's. The settings `fps` and `fps_3d`.
  - The size "As the view" and the ratio of the enlarged texts are taken from the view shown (`viewPixels()`, `viewHeight()`; the 3D window's size and dpr when it is shown).
  - A 3D size over the largest shows "A 3D image is 3840 x 2160 pixels at most." in the summary, and disables Export.
  - Before the export, for 3D, the drawer's `initialize()` is checked (before the sinks are made). The parts get the positions of each range and the starts of the ranges. The camera is a copy of the screen's 3D camera (without the 3D window, a new one of the camera of the settings). A video of 3D is without `-tune animation`.
  - "the players move more smoothly" in the tooltip of the check of the enlarged texts is of 2D: "in 2D" is added.
- `MainWindow`:
  - Has one `OffscreenView3D`, made when first needed (S13). On closing, an export is stopped before it is deleted.
  - Save Image of This Frame: with the 3D view shown, drawn by `renderStill()` at the 3D window's size × dpr. Enabled while the 3D view is shown too (the end of D5's "disabled until stage 6").
  - When the 3D window is made, it is given to the export dialog.
- A fix of 2D (S15): with a dpr below 1, `FieldCanvas::renderFrame()` gives the image the dpr 1, and maps the logical window onto the viewport of the image's pixels.
- Documents: README (the 3D Save Image, the View of the export, the fps), `docs/3d-view.md`, NEWS, ChangeLog.
- Tests:
  - `offscreen_view_3d` (new; skipped without OpenGL 3.3): nothing before initializing or without frames; a position between the frames (the index, the next, the fraction, the players); the current index and the state of the view set back; the score board at the bottom (dpr 1, 2 and 0.5, and the ratio of the heights); the same input gives the same images (two drawers, with the starts of ranges); at the start of a range the camera jumps, to the same pose and picture as a drawer starting there (colors within 2); `renderStill()` does not move the camera nor change the export's; an export by `ExportJob`.
  - `image_sequence_sink` (added): `ExportJob` with another drawer (the positions, the starts of the ranges told, a part each, an image not drawn).
  - `export_timing` (added): `positions3D()`.
  - `export_dialog` (added): an export of 3D images (2D when opened, the fps swapped, 7 images, the largest size, the settings, 2D when opened again, no `animation` in the arguments of a video), 3D not available.
  - `field_canvas_render` (added): dpr 0.5 (the score board at the bottom, the goal line at half the place). Checked to fail with the code before the fix.
- Checked (2026-11-25, Xvfb and Mesa):
  - The 3D view on the screen was the same to the pixel before (ad3687b) and after the change (the window read from the X server).
  - The 3D Save Image against the screen: only the texts of the score board differ (the screen's OpenGL paint engine and the image's raster paint engine antialias the texts differently, as 4.11 for 2D). The 3D picture was the same to the pixel. From the second, it is drawn in about 5 ms (150 to 180 ms the first time).
  - In this environment, the screen's `grabFramebuffer()` gave a black image (as S4 says). To compare with the screen, the window was read from the X server after it was drawn again.
  - The 4 goal scenes of a 2023 log (Oxsy vs YuShan2023):
    - Images of 320×180 at 30 fps, exported per range and in one sequence: the 2404 images (601 each) were the same to the pixel. At the boundaries of the ranges the picture changes much, and little between the first two images of a range (the camera jumps).
    - 4 mp4 files, one for each range, at 1280×720 and 30 fps in 15.4 s: H.264, yuv420p, 601 frames and 20.03 s each.
    - A goal scene at 1920×1080 and 60 fps in 13.9 s (86 images a second): 1201 frames, 20.02 s, 13.5 MB.
  - With images smaller than the view (320×180), the HUD is in the view's proportion (seen; before S15, the logos were large and the score board was not drawn).

## 5. Stages

Every stage builds and passes the CI.

| # | Work | Completion criteria |
|---|---|---|
| 1 | `Options::ViewState`, `FrameScope`, `FieldCanvas::renderFrame`, File > Save Image (2D) | Tests that it equals the screen's `grab()` and that the state is restored pass with Qt5, Qt6 and autotools. The user checks HiDPI on the screen |
| 2 | `export_timing`, the basics of `export_range` (notation of positions, the meaning of start and end, moving to existing frames, checks), `FrameSink`, `ImageSequenceSink`, `ExportJob`, `ExportDialog` (modeless; 2D, images; one range: start and end, from the current position, whole), the checks before the export (number of images, duration, disk, one overwrite question), the `Export` settings | PNG sequences of a full game and of a chosen range can be exported. The stopped frames of the end cycle are included. Cancel works. The screen is unchanged after the export |
| 3 | Convenient range selection: marks in the main window and the band on the slider, common ranges (periods, goal scenes, around the current position), several ranges (one output per range, or joined), naming by cycle | The range tests pass. The user checks with real logs that all the goal scenes can be exported at once, and the band on the slider |
| 4 | `FfmpegSink`, finding ffmpeg, the arguments, the video part of the dialog, ffmpeg in one CI job | The tests with the fake and the real ffmpeg pass. The mp4 plays, and its duration is N/fps (the sum when ranges are joined). Clear error messages |
| 5 | Refactoring only: `View3DRenderer`, `TeamGraphicPainter::update()`, `CameraController::snap()`, `FieldView3D::camera()` | All tests pass. The 3D on the screen is unchanged (the user checks) |
| 6 | `OffscreenView3D`, `gl/offscreen_target`, 3D in the dialog, the 3D Save Image, clearing the caches and `snap()`ping the camera at the start of a range | The test that the same input gives the same image passes in the CI (llvmpipe). 1080p at 60 fps can be exported. In a video of joined ranges, the camera does not glide at the boundaries. No effect on the 3D on the screen |
| 7 | The batch command line (`--export-range` etc.), `docs/build.md` and README | A test of the 2D batch under offscreen. The user checks 3D under xvfb |
| 8 | The Qt Multimedia prototype (OFF), its test, the measurements of §4.8 | Decide whether to keep it, and record it |

Stage 5 may proceed in parallel with stages 2-4.

## 6. Tests

- `test_export_timing`: N, `t_n` and the duration for 2D at 10 fps, 3D at 30/60 fps, speeds 0.5 and 2, and a step other than 100 ms. The duration of joined ranges. Image names and the number of digits.
- `test_export_range`: parsing and writing positions (`1234`, `1234.3`, errors). The start and end indices on a `DispCont` with stopped frames (the stopped frames of the end cycle included). Moving cycles outside the log or missing ones to existing frames. Checking a start after the end. Finding the periods (first half, second half, extra time, penalty shootout) from a synthetic sequence of play modes (also logs without them). Goal scenes (the seconds before and after, cut at the ends of the log, overlapping ranges of close goals). Around the current position. Names by cycle.
- The dialog: with it open, the position can be moved and taken by "Start at the current position". The marks in the main window and the dialog's range agree. Opening another log clears the marks. The overwrite question appears only once.
- `test_field_canvas_render`: `renderFrame( current index, size, 1.0 )` equals `grab().toImage()` (with a small tolerance if text antialiasing differs). After drawing another index at another size, `Options::viewState()` and the current index are unchanged (ball focus, zoom, automatic selection). At dpr 2 the score board is on the bottom rows (the window is logical).
- `test_image_sequence_sink`: writes to a temporary directory and reads back.
- `test_ffmpeg_sink` (Unix): fake ffmpeg scripts are made in a temporary directory inside the test (no executable files committed): one that writes its arguments and copies the standard input to the last argument, one that reads a little and exits with an error, a slow one, and a nonexistent path. The byte count = N×w×h×4, the arguments passed, the standard error in the error, the failure to start, cancel removing the file, `bytesToWrite()` staying under the limit. A test with the real ffmpeg (QSKIP if missing) checks the frame count with ffprobe.
- `test_offscreen_view_3d` (QSKIP without GL 3.3, as `test_scene_3d.cpp`): drawing the same (index, position) again gives the same image, the image size, the state of `Options` and `DispHolder` restored, and the same sequence of fixed steps gives the same images twice.
- `test_cameras_3d`: `snap()` jumps to the target pose.
- The menu items (added to `test_view_switch` or `test_toolbar`).
- `test_qt_multimedia_sink` (only when the prototype is built): export 30 frames at 320×240 and get a non-empty file. QSKIP without an H.264 encoder.
- Checked by hand: compare the export with the screen on HiDPI (`QT_SCALE_FACTOR` 1.25, 1.5, 2). Each 3D camera (the broadcast glide, the player view, free, top), and the compass, minimap and labels at 4K. Playback and colors in browsers and VLC. Memory not growing over a full game. Cancel. No ffmpeg. The 3D on the screen after the refactoring. The 3D batch under xvfb.

## 7. Files to change (expected)

| File | Content |
|---|---|
| `src/options.{h,cpp}` | `ViewState`, the `Export` settings, the command line |
| `src/frame_scope.{h,cpp}`, `src/export_timing.{h,cpp}`, `src/export_range.{h,cpp}`, `src/frame_sink.h`, `src/image_sequence_sink.{h,cpp}`, `src/ffmpeg_sink.{h,cpp}`, `src/export_job.{h,cpp}`, `src/export_dialog.{h,cpp}` (new) | The export mechanism |
| `src/field_canvas.{h,cpp}` | `renderFrame` |
| `src/disp_holder.{h,cpp}` | `lastIndex( cycle )` |
| `src/log_player_slider.{h,cpp}` | The band of the range (and the goal marks; §3.3) |
| `src/view3d/view_3d_renderer.{h,cpp}`, `src/view3d/offscreen_view_3d.{h,cpp}`, `src/view3d/gl/offscreen_target.{h,cpp}` (new) | Offscreen 3D drawing |
| `src/view3d/field_view_3d.{h,cpp}`, `src/view3d/cameras_3d.{h,cpp}`, `src/team_graphic_painter.{h,cpp}` | Refactoring, `snap()`, `update()` |
| `src/qt_multimedia_sink.{h,cpp}` (new, prototype) | §4.8 |
| `src/main_window.{h,cpp}`, `src/main.cpp` | Menus, range marks, batch |
| `CMakeLists.txt`, `src/CMakeLists.txt`, `cmake-config.h.in`, `src/Makefile.am` | Build |
| `test/`, `.github/workflows/ci.yml` | §6, ffmpeg in the CI |
| README, `docs/build.md`, NEWS | Documents |

## 8. Out of scope

- Recording the screen while playing (left to OBS etc.).
- Audio in the videos (including the referee's whistle).
- Exports that skip the stopped frames (the interpolation assumes neighboring indices).
- Transition effects (fades, captions, etc.) when several ranges are joined into one video.
- Saving the list of ranges to a file.

## 9. Risks

- How QPainter treats a `QImage` with a dpr: set the window and viewport explicitly, and test it.
- Rounding to whole pixels in 2D (when dpr is above 1).
- ffmpeg builds without libx264: offer only what `-encoders` lists.
- Two GL contexts, and the memory of MSAA at 4K.
- Disk use of PNG sequences (1-2 GB per game for 2D at 1080p).

## 10. Record

### 2026-11-22: Writing the plan

- Written together with the overall plan (2026-11-22-00_v20-features_en.md). The user's decisions D1 and D2 were given.
- The design was made after examining how 2D and 3D are drawn, and soccerwindow2's image saving (`ImageSaveDialog`: draws with `FieldCanvas::draw` into an offscreen `QImage` and names the files by the log index; no video).

### 2026-11-22: Specifying the range to export (the user's request)

- The user's request (D3): the range of frames to export can be specified. soccerwindow2's image saving may be referred to, but it should be improved.
- soccerwindow2's range selection was examined (2.5), and the following were added (§4.4, S8, S9): the notation of positions (cycle.stopped number), including the stopped frames of the end cycle, a modeless dialog and marks in the main window, the band on the slider, common ranges (periods, goal scenes, around the current position), several ranges, the checks before the export, and naming by cycle.
- The convenient range selection was added as stage 3, and the later stages were moved back by one (ffmpeg is stage 4, the Qt Multimedia prototype stage 8).

### 2026-11-24: The branch created, and the unified format

- After F5 (PR #75) was merged into develop (b3762c3), the user created `feature/frame-export` on GitHub.
- Following the unified format of the plans ("計画書の書式" of CLAUDE.md; the record of 2026-11-24 of the plan of the URLs and the lists of logs), the last update was added to the header table, and the table of the progress was put. §4 (the design) is not in sections by stage, so it is not rearranged; the implementation of each stage is added after the sections of the design.
- Next: stage 1 is checked with the user before it starts (deciding the items of stage 1 in §3.3).

### 2026-11-24: Stage 1 started

- The first commit of the branch of F6, 9a1853b (the unified format of the plans), was pushed. In the CI, all 5 jobs passed.
- Stage 1 started at the user's instruction. Decided by the user at the start of the stage: D4 (S1 to S9 as proposed), D5 (the shortcut of Save Image is Ctrl+Shift+I; disabled while the 3D view is shown, until stage 6).
- Checked to choose the shortcut: the File menu uses Ctrl+O, Ctrl+Shift+O, Ctrl+Shift+B, Ctrl+Shift+S and Ctrl+Q. Ctrl+I is Next Viewpoint, and Ctrl+S is Show Score Board. Ctrl+Shift+I and Ctrl+Shift+E are free, and do not clash with the keys of the 3D view (which uses no keys with modifiers).
- Checked in the 2D painters: every painter looks only at `DispHolder::currentDisp()`, and keeps no state across frames (the cache of the team logos is by the team's name). The global state changed when drawing is the focus point and the selected number changed by `updateFocus()`, and the size of the canvas, the scale and the center of the field changed by `Options::updateFieldSize()` (`ViewState` of §4.1).

### 2026-11-24: Stage 1 implemented

- Implemented as in 4.10. The test `field_canvas_render` was added, and a check of Save Image to `test_view_switch`.
- Found by the tests and fixed: because of the static variables of `updateFieldSize()`, drawing another frame right after the items shown changed set the view back with its old scale (4.10). As the antialiasing of the texts differs between the view (subpixel) and the image (grayscale), the comparison with the view is made pixel by pixel without the texts (with Qt 5, the advances of the texts differed too).
- With the CMake builds of Qt 6 (6.4.2) and Qt 5 (5.15.13) with the whistle, and of `ENABLE_SOUND=OFF`, there were no warnings, and `ctest -j8` run 3 times on each passed all 71 tests. The autotools build (Qt 5) passed too, without warnings of the compiler.
- Saving with the scales 1 and 2 was checked with a real game log (4.10). README ("Saving an image of the view"), NEWS and ChangeLog were updated.
- The user checked Save Image on the screen (the end of stage 1).
- Next: the commit and the push, and the CI.

### 2026-11-24: Stage 1 committed, and the CI

- Stage 1 was committed as e9a9e96 and pushed. In the CI, all 5 jobs passed.
- Next: stage 2 (the basics of the ranges and image sequences, the export dialog) is checked with the user before it starts.

### 2026-11-24: Stage 2 started, and the names of the menu items (D6)

- The user asked why saving one image and exporting a range are separate menu items. The merits of separating them (one image in one step, different defaults, different kinds of dialogs) and the demerits (two similar items, unlike the one entry of soccerwindow2) were given, with three options (A: separate, with names that tell them apart; B: one item; C: one item, keeping saving in one step). The user chose A (D6). Save Image... was renamed "Save Image of This Frame..." (the menu, the title of the dialog, README, NEWS, ChangeLog).
- Stage 2 started at the user's instruction. The shortcut of Export Images / Video... is Ctrl+Shift+E, as planned with D5.

### 2026-11-24: Stage 2 implemented

- Implemented as in 4.11. The tests `export_timing`, `export_range`, `image_sequence_sink` and `export_dialog` were added.
- Found by the tests and on the screen, and fixed: the field of the end in the export dialog was narrowed by a long error text beside it (the fields got a fixed width, and the texts one line). The test of the view unchanged failed as the rows of the tool bars, laid out again after opening the log, changed the size of the 2D view (the test waits for the layout).
- With the CMake builds of Qt 6 (6.4.2) and Qt 5 (5.15.13) with the whistle, and of `ENABLE_SOUND=OFF`, there were no warnings, and `ctest -j8` run 3 times on each passed all 75 tests. The autotools build (Qt 5) passed too, without warnings of the compiler.
- Image sequences of PNG of 1920×1080 were checked with a real game log (4.11). README ("Saving images of the view"), NEWS and ChangeLog were updated.
- The user checked the export (the end of stage 2).
- Next: the commit and the push, and the CI.

### 2026-11-24: Stage 2 committed, the CI, and stage 3 started

- Stage 2 was committed as d7adf47 and pushed. At the user's instruction, the CI was not watched, and the work paused. When it resumed, the user told that the CI had passed.
- The scratchpad of the working environment moved, and the build directories of Qt 6 and Qt 5 were made again (Qt 6.4.2 and Qt 5.15.13, with the whistle; no warnings).
- Stage 3 started at the user's instruction. Decided by the user at the start of the stage: D7 (the marks by `[` and `]`, cleared in the menu only), D8 (from 10 s before a goal to 5 s after, and from 5 s before the position shown to 5 s after), D9 (the slider shows the goals too). Checked to choose the keys: of the single keys, F, J, M, Q, W, Y and `[` `]` were free (I and O are used).

### 2026-11-24: Stage 3 implemented

- Implemented as in 4.12. The tests `export_range_list` and `log_player_slider` were added, and stage 3 was added to `export_range` and `export_dialog`.
- Found and changed from the plan: the parts of a game are split by the cycles of the server's parameters, not by the play modes (4.12: rcssserver 19 does not use `first_half_over` at the end of a half, and the log had no `before_kick_off` recorded either).
- Found by the tests and fixed: drawing the bands of the slider, the groove was not drawn, as `initStyleOption()` gives no parts to draw (they are given explicitly). `ServerParamT` cannot be copied (the helper of the test was changed).
- With the CMake builds of Qt 6 (6.4.2) and Qt 5 (5.15.13) with the whistle, and of `ENABLE_SOUND=OFF`, there were no warnings, and `ctest -j8` run 3 times on each passed all 77 tests. The autotools build (Qt 5) passed too, without warnings of the compiler.
- The parts, the goals, exporting the goal scenes, the dialog and the slider were checked with a real game log (4.12). README, NEWS and ChangeLog were updated.
- Next: the user checks exporting the goal scenes at once with a real log, and the bands of the slider (the end of stage 3). Then the commit and the push, and the CI.

### 2026-11-24: The positions of the goals follow DispHolder (D10)

- The user asked whether the indexes of the goals kept by `DispHolder` were left unused on purpose. The reasons (the menu needs the team and the score; a pure function to test) and what was found (`scoreChangedIndex()` points to the last frame before the new score, one frame from `goals()`; "go to the next score" of the log player moves to 50 frames before it) were told, with A (as it is) and B (following `scoreChangedIndex()`). The user chose B (D10).
- `ExportRange::goals()` takes the list of the indexes, and the dialog and the main window give it `scoreChangedIndex()`. The tests were changed (4.12). With Qt 6, all the tests of the export passed. Checked with two real logs (4.12).
- With the CMake builds of Qt 6 and Qt 5 with the whistle, and of `ENABLE_SOUND=OFF`, there were no warnings, and `ctest -j8` run 3 times on each passed all 77 tests. The autotools build (Qt 5) passed too.
- Next: the user checks stage 3 (the end of stage 3). Then the commit and the push, and the CI.

### 2026-11-24: Stage 3 checked, and the menu and the dialog changed (D11, D12)

- The user checked it working (the end of stage 3), and asked for these: marking the range of Export Range only by the menu was hard to find, and the names in the menu did not tell what they do; the tooltips of the buttons Now should tell the shortcuts; the dialog should have a button to clear the range too.
- It was told that "All" of the dialog does the same as clearing the current range, and the user chose to rename it "Clear" (D12). For the menu, the user proposed two ways (removing the items of the marks from the menu, leaving the dialog the only entry; or grouping the dialog, the start and the end of the range, and clearing in Export Images), and Claude advised the latter (D11).
- Changed: the menu (four items in File > Export Images / Video, with tooltips), Clear of the dialog and the tooltips of Now, and the paths of the menu in README. The test `export_dialog` was changed for the menu, the names of the items, the button Clear and the tooltips of Now. With Qt 6, all the tests of the export passed. The menu and the dialog were drawn into images and looked at.
- With the CMake builds of Qt 6 and Qt 5 with the whistle, and of `ENABLE_SOUND=OFF`, there were no warnings, and `ctest -j8` run 3 times on each passed all 77 tests. The autotools build (Qt 5) passed too.
- Next: the user checks the menu and the dialog. Then the commit and the push, and the CI.

### 2026-11-25: The separators of the menu, and the end of checking stage 3

- The user checked the menu and the dialog. As the items of the File menu were all in one run and hard to read, the user added separators (between opening, the URL and the lists, saving a copy, the images and the export, and quitting). With the CMake builds of Qt 6 and Qt 5 with the whistle, and of `ENABLE_SOUND=OFF`, all 77 tests passed.
- Next: the commit and the push of stage 3, and the CI.

### 2026-11-25: Stage 3 committed, and the CI

- Stage 3 was committed as d7a5cd1 and pushed. In the CI, all 5 jobs passed.
- Next: stage 4 (videos by ffmpeg) is checked with the user before it starts (deciding the item of stage 4 in §3.3).

### 2026-11-25: Stage 4 started

- Stage 4 started at the user's instruction. Decided by the user at the start of the stage: D13 (a stopped video's file removed), D14 (only the quality and the speed as the settings of the videos in the dialog), D15 (CRF 18 and preset medium by default). The user asked whether the options of mp4 are decided later. The proposal of the plan (4.5) was given, and the two points that shape the dialog (D14, D15) were decided here.
- This working environment has ffmpeg and ffprobe (not at the time of 2.4). The tests with the real ffmpeg run here too.

### 2026-11-25: Stage 4 implemented

- Implemented as in §4.13. Decided beyond the plan: ffmpeg is probed when a video is chosen first. The extensions of the videos are only `.mp4`, `.mov` and `.mkv`. The videos of the ranges are named `FILE-NAME.ext`. The progress shows the time left (for the images too). The field of the preset is named Encoding speed (not to be taken for the Speed of the rate). The Qt6 job of the CI must run the test with the real ffmpeg.
- Checked: with the CMake builds of Qt 6 and Qt 5 with the whistle, and of `ENABLE_SOUND=OFF`, it built without warnings, and all 78 tests (with the new `ffmpeg_sink`; the test with the real ffmpeg ran too) passed three times each. It built with autotools (Qt 5) too (the only warning is the old one of `configure.ac`). The checks with real logs are in §4.13.
- Next: the user checks it on the screen (exporting and playing videos, the state without ffmpeg, stopping). Then the commit and the push of stage 4, and the CI.

### 2026-11-25: Stage 4 checked (the check to enlarge the texts)

- The user checked it on the screen, and saw that with "Scale the texts with the image", not only the texts of the score board and the players, but also the team logos grow larger, and the white circle of a kick (a pen 2 pixels wide) grows thicker, and asked whether it was meant. The answer: it works as made (the dpr of the images is "the height of the image ÷ the height of the 2D view", so the image looks as the view, enlarged, and everything sized in the pixels of the screen grows in the same proportion). The name, which spoke of the texts only, was not right.
- The user chose plan A (D16): the behavior stays, the check is renamed "Enlarge the texts, logos and lines with the image", and the tooltip, the README and the comments of the source are corrected.
- The user checked the corrected name and explanation, and asked for the commit and the push.
- Next: stage 4 is committed and pushed, and the CI runs.

### 2026-11-25: Stage 4 committed, and the CI

- Stage 4 was committed as 83ab35e and pushed to `feature/frame-export` (the first push was stopped with the command and did not arrive, so only the push was done again).
- The CI passed in all the jobs (autotools Qt5, CMake Qt6, Qt6 (Ubuntu 24.04), Qt5, distcheck). In the CMake Qt6 job, ffmpeg was installed, and the test `ffmpeg_sink` (with the real ffmpeg) passed with `RCSSMONITOR_REQUIRE_FFMPEG=1`.
- Next: stage 5 (tidying the 3D drawing; a refactoring only). The user is asked before it starts.

### 2026-11-25: Stage 5 started and implemented

- Stage 5 was started on the user's instruction. Nothing was to be decided at its start (§3.3).
- Implemented as in §4.14. Decided beyond the plan: the labels and the aids of the ball are made once the camera is known, so they are in `render()`. The FPS is of the screen only, so it stays in `FieldView3D`, and `minimapShown()` was added for its place. The CPU times are measured in the renderer (the build time no longer includes the update of the camera).
- Found and fixed on the way: the constructor of `DispHolder` did not initialize `M_playmode` (§4.14).
- Checked: with the CMake builds of Qt 6 and Qt 5 with the whistle, and of `ENABLE_SOUND=OFF`, it built without warnings, and all 80 tests (with the new `view_3d_renderer` and `team_graphic_painter`; the 3D tests ran, not skipped) passed three times each. It built with autotools (Qt 5, without 3D) too (the only warning is the old one of `configure.ac`). On Xvfb, the 3D view before and after the change was compared pixel by pixel, with no difference (§4.14).
- The user checked the 3D view on the screen, and asked for the commit and the push. The user moves to another working environment, so the work pauses here. The user checks the CI later.
- Next: check the CI of stage 5, and write the commit hash of stage 5 and its CI in the table of the progress (in the next commit). Then stage 6 (the offscreen drawing of 3D; the user is asked before it starts).

### 2026-11-25: The CI of stage 5, and resumed

- The CI of stage 5 (ad3687b) passed in all the jobs (autotools Qt5, CMake Qt6, Qt6 (Ubuntu 24.04), Qt5, distcheck). The user checked it and said so, and Claude checked it on GitHub too.
- The user moved to another working environment and resumed the work.
- Next: stage 6 (the offscreen drawing of 3D). The user is asked before it starts.

### 2026-11-25: Stage 6 started and implemented

- Stage 6 was started on the user's instruction. At its start the user decided D17 (the 2D / 3D of the export dialog is the view shown on the screen when it is opened).
- Implemented as in §4.15. Decided beyond the plan: S10 (the type of drawer `ExportRenderer`), S11 (only the camera's `snap()` at the start of a range), S12 (`QOpenGLFramebufferObject`; no `gl/offscreen_target`), S13 (the main window has one drawer), S14 (an fps for each view, the size from the view shown), S15 (a dpr below 1).
- Found and fixed on the way: QPainter draws a dpr below 1 as 1, so an image smaller than the view showed only the top left of the layout. It was a defect of 2D since stage 1 too (S15).
- Checked: with the CMake builds of Qt 6 and Qt 5 with the whistle, and of `ENABLE_SOUND=OFF`, it built without warnings, and all 81 tests (with the new `offscreen_view_3d`; the 3D tests ran, not skipped) passed three times each. It built with autotools (Qt 5, without 3D) too (the only warning is the old one of `configure.ac`). The comparison with the screen on Xvfb and the exports of a real log are in §4.15.
- The user checked it on the screen, and asked for the commit message.
- Next: stage 6 is committed and pushed, and the CI runs.

### 2026-11-25: Stage 6 committed, and the CI failed

- Stage 6 was committed as 89b5168 and pushed to `feature/frame-export`.
- The CI failed in the 4 CMake jobs (Qt6, Qt6 (Ubuntu 24.04), Qt5, distcheck); autotools passed. Only `sameInput` of `offscreen_view_3d` failed: the first images of two drawers with the same input were not the same. The other 3D tests passed (`rangeStart` compares two drawers too, within a color difference of 2).
- Here, both on the hardware and with Mesa's software drawing (`LIBGL_ALWAYS_SOFTWARE=1`), they were the same: it did not happen again. No place where a clock or a random number changes the picture, and no image used without being filled, was found.
- The fix: `sameInput` and `rangeStart` compare two images by the same function, the same picture within a color difference of 2 (the GPU's rounding). When they are not the same to the pixel, the number of the pixels, the largest difference and where are told in the log, and when it fails too. If the difference is large in the CI too, that log is looked into.
- Next: the fix committed and pushed, and the CI.
