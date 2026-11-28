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
| 1 | The ranges counted by the seconds of the playback (§4.1; D1, D2, S1–S8) | — | https://github.com/rcsoccersim/rcssmonitor/issues/78 |

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

## 3. Decisions

### 3.1 User Decisions

| # | Decision | Date | When |
|---|---|---|---|
| D1 | The seconds of the goal scenes and around the position shown are the seconds of the playback: the frames before and after the center frame are taken by their number (the seconds × 1000 / the step), a stopped frame counting as a step too (as the images of the export, `ExportTiming`, at 1x). A goal scene is around the frame of the goal (the first frame with the new score), and around the position shown is around the frame shown | 2026-11-28 | The bug report (before the plan was made) |
| D2 | The default seconds (10 s before to 5 s after a goal, 5 s before to 5 s after the position shown; D8 of the export plan) stay. A goal scene by default is 151 frames, and ends at the last stopped frame before the kick-off (437.50 at the first goal of the 2023 log) | 2026-11-28 | Same as above |

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
- The example of the list (`goal1-c437: 337 - 487 (201 frames)`) and the example of the standard error of the batch (804 images) become the values exported from the real log after the change.
- "a goal scene (20 s)" of the table of the sizes becomes "a scene of 20 s".

## 5. Phases

| # | Work | Completion condition |
|---|---|---|
| 1 | Count `ExportRange::around()` by the indices of the frames, add `ExportGoal::frame`, and change the presets of the dialog and `goals[:BEFORE:AFTER]`. Change the tests, the help of `--export-range` and `docs/export.md` | All the tests pass in the Qt6 and Qt5 builds. With the 2023 log, a goal scene by default is 151 frames (337–437.50 for the first goal), in the dialog and on the command line. CI passes |

## 6. Tests

- `test_export_range`
  - `around()`: takes the frames before and after the index of the center by their number. With stopped frames after the center, the end cycle comes that much earlier. Cut by the ends of the log. With a step of 50 ms, twice the frames. Invalid with no frames and with a center out of the log.
  - `goals()`: `frame` is the frame after `index`, or `index` itself if it is the last frame.
- `presets` of `test_export_dialog`: the expected goal scene, all the goals and around the position shown are counted by the frames. With a game having stopped frames after a goal, a goal scene of the default seconds ends before the kick-off.
- `ranges` of `test_export_batch`: the ranges of `goals` and `goals:BEFORE:AFTER` are checked by the frames.
- The real behavior: with the 2023 log, `--export-range goals` on the command line makes four ranges of 151 frames, and the preset "Goal 1" of the dialog is `337 - 437.50 (151 frames)`.

## 7. Files to Change (Expected)

| File | Change |
|---|---|
| `src/export_range.h`, `src/export_range.cpp` | `around()` counted by the indices of the frames; `ExportGoal::frame` |
| `src/export_dialog.cpp` | the centers of the goal scene, all the goals and around the position shown |
| `src/export_batch.cpp` | the center of `goals[:BEFORE:AFTER]`; the help of `--export-range` |
| `test/test_export_range.cpp`, `test/test_export_dialog.cpp`, `test/test_export_batch.cpp` | §6 |
| `docs/export.md` | §4.1.1 |
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
