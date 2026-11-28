# Count the Seconds of the Goal Scenes and Others by the Playback: Plan

| Item | Content |
|---|---|
| Created | 2026-11-28 |
| Last updated | 2026-11-28 |
| Target | rcssmonitor develop (f2898ed, after the export feature (https://github.com/rcsoccersim/rcssmonitor/pull/76) was merged) |
| Branch | `fix/goal-scene-length` (from f2898ed of develop; the user creates it on GitHub) |
| Plan ID | MON-01 |
| Plan Issue | https://github.com/rcsoccersim/rcssmonitor/issues/77 |
| Japanese version | [2026-11-28-00_goal-scene-length.md](2026-11-28-00_goal-scene-length.md) (same content) |
| Related | The export plan [2026-11-22-03_frame-export_en.md](2026-11-22-03_frame-export_en.md) (§4.3 the timing model, §4.4 the ranges to export, D8, D10). The port of the export feature to soccerwindow2 (where this bug was found) |

## Phases and Issues

| Phase | Content | Implementation record | Issue |
|---|---|---|---|
| 1 | The ranges counted by the seconds of the playback (§4.1; D1–D4, S1–S10) | 4.1.2 (2026-11-28) | https://github.com/rcsoccersim/rcssmonitor/issues/78 |

## 1. Purpose and Policy

- Count the seconds of the presets "goal scene" and "around the position shown" of the export dialog, and of `--export-range goals[:BEFORE:AFTER]` of the command line, by the seconds of the playback (the number of the frames), not by the cycles.
- As in the timing model of the export (§4.3 of the export plan: a frame an image at 1x), a stopped frame counts as a step too. A goal scene of the default seconds (10 s before to 5 s after the goal) is 15 s in a video at 1x.

## 2. Current State (develop f2898ed)

- `ExportRange::around( frames, cycle, before_s, after_s, step_ms )` (`src/export_range.cpp`) subtracts and adds the cycles of `cyclesOf()` to the center cycle, and turns them into the indices of the frames by `startIndex()` and `endIndex()`. All the frames stopped in the end cycle are included. The stopped frames are not counted as seconds.
- It has four callers:
  - `ExportDialog::chooseGoal()` and `chooseAllGoals()`: around `ExportGoal::cycle` of a goal
  - `ExportDialog::chooseAround()`: around the cycle of the frame shown
  - `goals[:BEFORE:AFTER]` of `ExportBatch::resolveRanges()`: around `ExportGoal::cycle` of a goal
- `ExportGoal::index` is an index of `DispHolder::scoreChangedIndex()` (the frame just before the new score; D10 of the export plan), and `ExportGoal::cycle` is the cycle of the frame after it.
- After a goal, 50 frames are stopped before the kick-off. At the first goal of the 2023 log `202307090956-Oxsy_1-vs-YuShan2023_3.rcg.gz`, the cycle 437 of `goal_r` has 51 frames (437, 437.1–437.50), and `kick_off_l` comes at 438.
  - Counted as now, "5 s after the goal" ends at 487 (5 s after the kick-off). The goal scene is the 201 frames of 337–487 (100 before the goal, the goal at 437, the 50 stopped frames, 50 after the kick-off), and in a video at 10 images/s it goes on 10 s after the goal.
  - The examples of `docs/export.md` (`goal1-c437: 337 - 487 (201 frames)`, 804 images for the four goals) are counted this way too.
- The timing model of the export, `ExportTiming`, makes a frame an image at 1x (10 images/s in 2D). The stopped frames are images too (an export skipping the stopped frames is T3 of the TODO).
- The dialog keeps the ranges as positions in text (`ExportRangeSpec`). `ExportDialog::specOf()` (the presets), "the end at the position shown" and `]` of the main window write an end by `positionText()`. As an end, `1234` is the last frame of the cycle, so the first frame of a cycle followed by frames stopped in it cannot be written as an end (checked during the implementation of Phase 1).
- The parsers of the text rcg (v4–v6) do not set the numbers of the stopped frames (`ShowInfoT::stime_`; only the JSON parser does). In a text log, the stopped frames of a cycle are all at `1234`, and `1234.3` cannot point at one. The names of the images by the cycles repeat too (same as above).

## 3. Decisions

### 3.1 User Decisions

| # | Decision | Date | When |
|---|---|---|---|
| D1 | The seconds of the goal scenes and around the position shown are the seconds of the playback: the frames before and after the center frame are taken by their number (the seconds × 1000 / the step), a stopped frame counting as a step too (as the images of the export, `ExportTiming`, at 1x). A goal scene is around the frame of the goal (the first frame with the new score), and around the position shown is around the frame shown | 2026-11-28 | The bug report (before the plan was made) |
| D2 | The default seconds (10 s before to 5 s after a goal, 5 s before to 5 s after the position shown; D8 of the export plan) stay. A goal scene by default is 151 frames, and ends at the last stopped frame before the kick-off (437.50 at the first goal of the 2023 log) | 2026-11-28 | Same as above |
| D3 | How an end of a range is written: the first frame of a cycle followed by frames stopped in it is written `1234.0` as an end (`ExportRange::endText()`). Used by the presets, and also by the existing `]` (End the Range Here), "the end at the position shown" of the dialog and the text beside the field of the end | 2026-11-28 | During the implementation of Phase 1 |
| D4 | In the logs without the numbers (the text rcg), the stopped frames are numbered when read: a frame of the same cycle as the frame before it gets the number of that frame plus 1 (`DispHolder::handleShow()`). The numbers of the JSON logs stay. The positions `1234.3`, the status bar and the names of the images by the cycles agree in every log | 2026-11-28 | During the implementation of Phase 1 |

### 3.2 Design Judgments (Claude)

| # | Judgment | Reason |
|---|---|---|
| S1 | `ExportRange::around()` takes the center as the index of a frame, not as a cycle: `around( frames, center, before_s, after_s, step_ms )`. It takes the number of the frames before and after the center, cut by the ends of the log. A center out of the log gives an invalid range | The frames are counted, so the index is the natural input. The callers know which frame is the center |
| S2 | The numbers of the frames before and after are given by `cyclesOf()` (the seconds × 1000 / the step, rounded), and its comment says they are the numbers of the frames at 1x too | The same formula: no new function |
| S3 | The frame of a goal is kept in a new member `frame` of `ExportGoal`: the frame after `index` (the frame of `cycle`), or `index` itself if it is the last frame of the log. `goals()` sets it | It is the same frame as `cycle`, so it agrees with the names of the ranges (`goal1-c437`). The frame of a goal is decided in one place (the same idea as D10 of the export plan) |
| S4 | Around the position shown is around the frame of `DispHolder::currentIndex()`. The name of the range stays `around-c<cycle>` | A stopped frame shown is the center itself |
| S5 | A log with cycles missing in a live game is counted by the frames too | It agrees with the images of the export and with the playback of the log (a frame at a time) |
| S6 | The keys of the settings (`goal_before`, `goal_after`, `around_before`, `around_after`) and the default values stay. The values mean the seconds of the playback | The values can be used as they are; no migration of the settings is needed |
| S7 | `docs/export.md` tells that the seconds are the seconds of the playback (the stopped frames counted too), and its example of the list and its example of the standard error of the batch are replaced by the values exported again from the real log. In the table of the sizes, "a goal scene (20 s)" becomes "a scene of 20 s", keeping the measured values | A goal scene by default becomes 15 s. The values of the table were measured on a scene of 20 s, and stay right |
| S8 | The export plan (2026-11-22-03_frame-export.md) is not changed. The comments of the source, such as of `around()`, point to this plan | The export plan stays as a record. The seconds of D8 do not change |
| S9 | The fields of the seconds in the dialog get a tooltip telling that they are the seconds of the playback (the stopped frames counted too). The tooltip of the fields of the positions tells what `1234.0` is | The meaning of the seconds and the new way of writing are shown in the dialog too |
| S10 | The entry of 20.0.0 of ChangeLog gets a line. NEWS is not changed | Recorded in ChangeLog as the PRs before. The item of the export in NEWS stays right |

## 4. Design

### 4.1 Phase 1: The Ranges Counted by the Seconds of the Playback

#### 4.1.1 Design

**`ExportRange::around()` (S1, S2)**

```
n_before = cyclesOf( before_s, step_ms )
n_after  = cyclesOf( after_s, step_ms )
first = center − min( center, n_before )
last  = min( frames.size() − 1, center + n_after )
```

- An invalid range is returned if the frames are empty or `center >= frames.size()`.
- The comment of the header changes from "around a cycle" to "around a frame (the seconds of the playback; a stopped frame a step too)".

**`ExportGoal::frame` (S3)**

- `goals()` puts the index of `after` (now `frames[min( index + 1, size − 1 )]`) in `frame`.

**How an end is written (D3; added during the implementation of Phase 1)**

- `ExportRange::endText( frames, index )`: `1234.0` if `stime_` is 0 and the next frame is of the same cycle, otherwise `positionText()`. `1234.0` is read back to that frame by `parsePosition()` and `endIndex()`.
- Used by: the end of `ExportDialog::specOf()`, `setEndToCurrent()`, the text beside the field of the end, and `]` of the main window.

**The numbers of the stopped frames (D4; same as above)**

- `DispHolder::handleShow()`: if `show.stime_` is 0 and the frame before is of the same cycle, `stime_` becomes the `stime_` of the frame before plus 1.

**The callers**

| Caller | The center now | The center after the change |
|---|---|---|
| `ExportDialog::chooseGoal()`, `chooseAllGoals()` | `goal.cycle` | `goal.frame` |
| `ExportDialog::chooseAround()` | the cycle of the frame shown | `M_disp_holder.currentIndex()` (S4) |
| `goals[:BEFORE:AFTER]` of `ExportBatch::resolveRanges()` | `goals[i].cycle` | `goals[i].frame` |

- The help of `--export-range` tells that the seconds are the seconds of the playback (the stopped frames counted too).

**At the first goal (437) of the 2023 log, with the default seconds**

| | Now | After the change |
|---|---|---|
| The goal scene | the 201 frames of 337–487 | the 151 frames of 337–437.50 |
| A video at 1x (10 images/s) | 20.1 s | 15.1 s |

**`docs/export.md` (S7)**

- The presets "Goal N" and "Around the position shown", and the rows `goals` and `goals:BEFORE:AFTER` of `--export-range`, tell that the seconds are the seconds of the playback, and that a goal scene by default ends before the kick-off.
- The positions tell that the stopped frames are numbered in every log, and what `1234.0` is (D3, D4).
- The example of the list (`goal1-c437: 337 - 487 (201 frames)`) and the example of the standard error of the batch (804 images) become the values exported from the real log after the change.
- "a goal scene (20 s)" of the table of the sizes becomes "a scene of 20 s".

#### 4.1.2 Implementation (2026-11-28)

- `ExportRange::around( frames, center, before_s, after_s, step_ms )`: takes the number of the frames of `cyclesOf()` before and after the index of the center, cut by the ends of the log. Invalid with a center out of the log (S1, S2).
- `ExportGoal::frame`: `goals()` puts the frame after `index` in it (`index` if it is the last frame) (S3).
- `ExportRange::endText()` (added): a position as an end. Used by `ExportDialog::specOf()`, `setEndToCurrent()`, the text beside the field of the end, and `]` of the main window (D3).
- `DispHolder::handleShow()`: a frame without a number gets the number of the frame before it in the same cycle plus 1 (D4).
- The callers: the goal scene and all the goals of the dialog center on `goal.frame`, around the position shown on `currentIndex()` (S4), and `goals[:BEFORE:AFTER]` of the batch on `goals[i].frame`.
- The dialog: the tooltip of the fields of the seconds, and `1234.0` in the tooltip of the fields of the positions (S9). For the tests, the texts beside the fields of the start and the end got object names (`start_info`, `end_info`).
- The help of `--export-range` tells that the seconds are the seconds of the playback (the stopped frames counted too).
- `docs/export.md`: the seconds of the playback, the numbers of the stopped frames and `1234.0`, the values of the examples (the list `goal1-c437: 337 - 437.50 (151 frames)`, 604 images and 60.4 s of the summary and the standard error, 1.6 s taken), and "a scene of 20 s" of the table of the sizes (S7).
- ChangeLog: a line in the entry of 20.0.0 (S10).
- The tests are as §6.
- Checked (2026-11-28):
  - All the 82 tests passed in the Qt6 (Debug) build. All the 82 tests passed in the Qt5 (Release) and Qt6 (Release) builds too.
  - With the 2023 log, `--export-range goals` on the command line made four ranges of 151 frames (604 images). The videos (`--export-video`) were all 151 images, 15.1 s.
  - With the same log, the preset "Goal 1" of the dialog was `goal1-c437: 337 - 437.50 (151 frames)`, and all the goals were 151 frames each. Around the position shown at 5 s before the goal was `337 - 437.0 (101 frames)`, and at 437.20 `407 - 457 (101 frames)`. The end at the position shown at the frame of the goal was `437.0` (read from the list of the ranges of the dialog by a temporary program, not committed).
  - Before D4, checked the same way, around the position shown at 5 s before the goal was 151 frames (`437.0` went to the last frame of the cycle 437).

## 5. Phases

| # | Work | Completion condition |
|---|---|---|
| 1 | Count `ExportRange::around()` by the indices of the frames, add `ExportGoal::frame`, and change the presets of the dialog and `goals[:BEFORE:AFTER]`. How an end of a range is written (D3), and the numbers of the stopped frames (D4). Change the tests, the help of `--export-range` and `docs/export.md` | All the tests pass in the Qt6 and Qt5 builds. With the 2023 log, a goal scene by default is 151 frames (337–437.50 for the first goal), in the dialog and on the command line. CI passes |

## 6. Tests

- `test_export_range`
  - `around()`: takes the frames before and after the index of the center by their number. With stopped frames after the center, the end cycle comes that much earlier. Cut by the ends of the log. With a step of 50 ms, twice the frames. Invalid with no frames and with a center out of the log.
  - `goals()`: `frame` is the frame after `index`, or `index` itself if it is the last frame.
  - `text()`: `endText()` writes `1234.0` for the first frame of a cycle followed by frames stopped in it. `endText()` of every frame is read back as an end to the same frame (D3).
- `test_export_dialog`
  - `presets`: the expected goal scene, all the goals and around the position shown are counted by the frames.
  - `presetsStopped` (added): with a game having stopped frames after a goal, a goal scene of the default seconds ends before the kick-off (151 frames). Around the position shown at 5 s before the goal ends at `200.0`. Around a stopped frame is counted by the frames too. The end at the position shown at the frame of the goal is `200.0` (D3).
- `ranges` of `test_export_batch`: the ranges of `goals` and `goals:BEFORE:AFTER` are checked by the frames. With a game having stopped frames after a goal, a goal scene by default is 151 frames.
- `stoppedFramesNumbered` of `test_disp_holder` (added): the frames without numbers are numbered, and the numbers of JSON stay (D4).
- The real behavior: with the 2023 log, `--export-range goals` on the command line makes four ranges of 151 frames, and the preset "Goal 1" of the dialog is `337 - 437.50 (151 frames)`.

## 7. Files to Change (Expected)

| File | Change |
|---|---|
| `src/export_range.h`, `src/export_range.cpp` | `around()` counted by the indices of the frames; `ExportGoal::frame`; `endText()` (D3) |
| `src/export_dialog.cpp` | the centers of the goal scene, all the goals and around the position shown; how an end is written (D3); the tooltips (S9) |
| `src/export_batch.cpp` | the center of `goals[:BEFORE:AFTER]`; the help of `--export-range` |
| `src/main_window.cpp` | how the end of `]` is written (D3) |
| `src/disp_holder.cpp` | the numbers of the stopped frames (D4) |
| `test/test_export_range.cpp`, `test/test_export_dialog.cpp`, `test/test_export_batch.cpp`, `test/test_disp_holder.cpp` | §6 |
| `docs/export.md` | §4.1.1 |
| `ChangeLog` | S10 |
| `docs/plan/2026-11-28-00_goal-scene-length.md`, `_en.md` | the records of the implementation |

## 8. Out of Scope

- An export skipping the stopped frames (stays T3 of the TODO).

## 9. Records

### 2026-11-28: The Plan Made

- Made the plan from the bug report found in the port of the export feature to soccerwindow2 (the seconds of the goal scenes and around the position shown are counted by the cycles, and the stopped frames are not counted, so a goal scene is longer than expected). The changes of the report are D1 and D2.
- Checked with the 2023 log that the cycle of the goal (437) has 51 frames, and that the kick-off comes at 438 (§2).
- Next: after the plan is reviewed, assign the plan ID, and create the plan Issue and the sub-Issue of Phase 1.

### 2026-11-28: The Plan Fixed

- The user reviewed the plan. The user created `fix/goal-scene-length` from develop (f2898ed).
- Assigned the plan ID MON-01 (the first plan Issue of this repository), and created the plan Issue https://github.com/rcsoccersim/rcssmonitor/issues/77 and the sub-Issue of Phase 1 https://github.com/rcsoccersim/rcssmonitor/issues/78.
- Next: start Phase 1 after the user's confirmation.

### 2026-11-28: Phase 1 Started and Implemented

- Started Phase 1 at the user's word.
- During the implementation, found that `ExportDialog::specOf()` writes an end by `positionText()`, so the first frame of a cycle followed by frames stopped in it becomes `1234` (the last frame of the cycle) as an end. The existing `]` and "the end at the position shown" have the same problem. The user chose to fix all the ends (D3).
- Checking the dialog with the 2023 log, found that the parsers of the text rcg do not set the numbers of the stopped frames, so `437.0` and `437.20` cannot point at one frame (§2). The names of the images by the cycles repeat too. The user chose to number them when read (D4).
- Implemented as 4.1.2. What the tests, the builds and the real behavior showed is in 4.1.2 too.
- Next: after the commit and the push, check CI and record it in https://github.com/rcsoccersim/rcssmonitor/issues/78. After the user's confirmation, close Phase 1 and go on to the merge.
