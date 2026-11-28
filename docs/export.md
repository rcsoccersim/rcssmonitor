# Exporting images and videos

rcssmonitor saves what it shows as images, and exports ranges of a game log as images or videos, drawn by the 2D view or the 3D view as they look on the screen.
The same export runs from the command line, without a window.

- [What you need](#what-you-need)
- [Saving the frame shown](#saving-the-frame-shown)
- [The export dialog](#the-export-dialog)
  - [Ranges](#ranges)
  - [Marking a range in the main window](#marking-a-range-in-the-main-window)
  - [Presets](#presets)
  - [Images: the view, the size and the rate](#images-the-view-the-size-and-the-rate)
  - [Output: images or a video](#output-images-or-a-video)
  - [Exporting](#exporting)
- [The 3D view](#the-3d-view)
- [Rates, speeds and lengths](#rates-speeds-and-lengths)
- [The command line](#the-command-line)
  - [Options](#options)
  - [Ranges on the command line](#ranges-on-the-command-line)
  - [Examples](#examples)
  - [Without a display](#without-a-display)
- [Sizes of the files](#sizes-of-the-files)
- [Tips](#tips)
  - [Still frames at the start and the end](#still-frames-at-the-start-and-the-end)
  - [Pauses between the scenes](#pauses-between-the-scenes)
  - [One compression from the images](#one-compression-from-the-images)
  - [Fading from and to black](#fading-from-and-to-black)
- [When something goes wrong](#when-something-goes-wrong)

## What you need

- Images: nothing more.
- Videos: [ffmpeg](https://ffmpeg.org/), installed separately (`sudo apt install ffmpeg` on Ubuntu), with libx264 for H.264 (most builds have it). rcssmonitor runs it as a program: in the PATH, or where you choose.
- The 3D view: rcssmonitor built with the 3D view, and OpenGL 3.3 ([3d-view.md](3d-view.md#requirements)).
- Exports are made from a game log, not while connected to a server: the frames kept after disconnecting can be exported.

## Saving the frame shown

`File` -> `Save Image of This Frame...` (`Ctrl+Shift+I`) saves the frame shown, in the 2D view or the 3D view, to a PNG file (or JPEG by the extension `.jpg`).

- The image is as large as the view: on a high-DPI screen, with as many pixels as the screen shows.
- The default name is the game log's name and the cycle, e.g. `202607050404-HELIOS2026_0-vs-YuShan2026_1-301.png` (`-301.2` for a stopped frame).
- The 3D view is drawn again offscreen with its camera as it is, without the frames per second (the option `fps_3d`).
- The texts are drawn with grayscale antialiasing: they may look a little lighter than on the screen.

## The export dialog

`File` -> `Export Images / Video` -> `Open the Export Dialog...` (`Ctrl+Shift+E`) opens the dialog.
It stays open while you move through the game log, so that you can take the positions shown.
It and the items of the range are enabled when there are frames (a game log open, or the frames of a live game kept after disconnecting) and the monitor is not connected to a server; while connected, they and `Export` are disabled.

The dialog has three parts, `Ranges`, `Images` and `Output`, and a summary of what will be exported above the `Export` button, e.g.:

```
604 images of 604 frames, in 4 ranges. 1280 x 720 pixels; 4 videos, 60.4 s in all at 10 images/s.
```

A mistake (a position not in the log, a name used twice, ffmpeg not found, ...) is shown there in red instead, and `Export` is disabled.

The fields are kept in the settings (the group `Export` of `~/.rcssmonitor.conf`) when an export starts.

### Ranges

The list shows the ranges to export, e.g. `goal1-c437: 337 - 437.50 (151 frames)`. Under it, the range chosen in the list:

- `Name:` its name: the name of its directory or its video when the ranges are exported each in its own.
- `Start:` and `End:` its first and last frames, written as the status bar shows them:
  - `1234`: the cycle 1234. As the start, its first frame; as the end, its last frame, the frames stopped in it too.
  - `1234.3`: the frame stopped in the cycle 1234 with the stopped count 3. The frames stopped in a cycle (e.g. after a goal, before the kick-off) are counted 1, 2, ... in every game log.
  - `1234.0`: the first frame of the cycle 1234, before the frames stopped in it. `Now`, `]` and the presets write an end there so.
  - empty: the first frame (`Start`) or the last frame (`End`) of the log.
- Beside each, the frame and the game time it is, e.g. `337 (0:33)`. A position with no frame (out of the log, or a cycle missing in it) is moved to the next frame for a start and to the previous one for an end, and told: `: no frame at 9000`.
- `Now` takes the position shown in the main window.
- `Clear` empties both: the whole log.
- `Presets` gives ranges made for you ([Presets](#presets)).

`Add` adds a range of the whole log at the end of the list (`range2`, `range3`, ...), `Remove` removes the range chosen, and `Up` and `Down` move it.
Several ranges are exported in one go: each in its own directory or video, or all in one sequence of images or one video (`Ranges:` in `Output`).

The ranges belong to the game log: they are forgotten when another game log is opened or the monitor connects to a server.

### Marking a range in the main window

The range chosen can also be marked in the main window, with the dialog closed:

| Key | Menu (`File` -> `Export Images / Video`) | |
|---|---|---|
| `[` | `Start the Range Here` | the position shown as the start |
| `]` | `End the Range Here` | the position shown as the end |
| | `Clear the Range` | the whole log |

The slider of the game log shows the ranges as bands, the one chosen darker, and the goals as orange ticks.

### Presets

`Presets` gives the range chosen (or the list) of:

- `The whole log`.
- A part of the game: `First half`, `Second half`, `Extra time 1`, `Extra time 2`, `Penalty shootout`, with their game times. The halves are found by the cycles of the server's parameters (rcssserver records no play mode at the end of a half).
- `Goal N: TIME TEAM (SCORE)`, e.g. `Goal 1: 0:43 HELIOS2026 (1 - 0)`: the scene of a goal, from 10 seconds before it to 5 seconds after it. The frames stopped after a goal are 5 seconds, so the scene ends before the kick-off, e.g. `goal1-c437: 337 - 437.50 (151 frames)`.
- `All the goals, a range each`: a range for each goal (`goal1-c437`, `goal2-c797`, ...), in place of the whole log, or after the ranges already in the list.
- `Around the position shown`: from 5 seconds before the frame shown to 5 seconds after it (`around-c1234`).

The seconds are set in the dialog: `Presets: Goal scenes: 10.0 s before, 5.0 s after. Around now: 5.0 s before, 5.0 s after.`
They are the seconds of the playback, as in a video at x 1: a frame is a step (100 ms), a stopped frame too.

### Images: the view, the size and the rate

- `View:` the `2D view` or the `3D view` draws the images. When the dialog is opened, the view shown in the main window is chosen. The 3D view is disabled, with the reason in its tooltip, when it cannot be drawn ([The 3D view](#the-3d-view)).
- `Size:` `As the view` (the size of the view shown, in its pixels on a high-DPI screen), `1280 x 720`, `1920 x 1080`, `3840 x 2160`, or `Custom` (16 x 16 to 7680 x 4320). A video has even sizes: an odd one is made one smaller.
- `Enlarge the texts, logos and lines with the image`: the image looks as the view, enlarged (or made smaller) to the size of the image. The texts, the team logos and the widths of the lines (such as the circle of a kick) are as large in the image as in the view. Off, they keep their sizes in pixels: smaller and thinner in a large image, and in 2D the players move more smoothly.
- `Rate:` the images a second of the video (10 for the 2D view, 30 for the 3D view by default, kept for each view), and `Speed:` how many times as fast as the game is played. `As the Playback` sets the speed at which the monitor plays the game log (the simulation step / the interval of the playback). See [Rates, speeds and lengths](#rates-speeds-and-lengths).

### Output: images or a video

`Images`:

- `Directory:` where the images are written (made if it is not there).
- `Names:` the start of the names and the format (`PNG` or `JPG`). The images are numbered from 0, `image-00000.png`, with 5 digits or more as needed: a tool such as ffmpeg reads them as a sequence.
- `Name the images by the cycles`: `image-c01234.00.png` (the cycle and the stopped count), when each image is a frame (10 images/s at x 1 in 2D).

`Video (ffmpeg)`:

- `File:` the video. Its extension gives the format: `.mp4`, `.mov` or `.mkv` (`.mp4` is added when there is none).
- `Quality (CRF):` 18 by default. Smaller is better and larger; 18 shows hardly any loss, 23 is ffmpeg's default.
- `Encoding speed:` `medium` by default. Slower makes smaller files of the same quality, and takes longer.
- `ffmpeg:` its path, empty for `ffmpeg` in the PATH. The line under it shows its version and the H.264 encoder it has, e.g. `ffmpeg version 6.1.1; H.264 by libx264.`. Without libx264, another encoder is used and told (MPEG-4 Part 2 by mpeg4, larger and of lower quality); `Quality` and `Encoding speed` are then not used.
- The video is H.264 in yuv420p (the colors of 2 x 2 pixels, as most players expect), without sound. A video of the 2D view is tuned for its flat colors.

`Ranges:` with several ranges, `Each in its own directory, by its name` (`DIRECTORY/NAME/image-00000.png`) or `A video for each, by its name` (`FILE-NAME.mp4`), or `All in one sequence` / `All in one video` (the numbers going on over the ranges).

### Exporting

`Export` starts the export:

1. The playback stops: the frame shown stays.
2. The files already there are told once: overwrite them, choose another file (a video), or cancel. For images, the disk space is estimated, and asked about when it may not be enough.
3. A progress dialog shows the images exported and the time left. `Stop` stops it: the images written are kept; a video being written is removed (one not finished may not play), and the videos of the ranges finished are kept.
4. At the end, the dialog and the status bar tell what was exported, e.g. `Exported 4 videos in /home/me/videos.`

The view shown does not change while exporting.

## The 3D view

The 3D view is drawn offscreen, in an OpenGL context of its own, so that the view on the screen does not change.

- It is drawn between the frames, as on the screen: at 30 images/s (60 for smoother motion), the players and the ball move smoothly.
- The camera starts as the 3D view's camera when the export starts (the camera chosen, where it is, its zoom); without the 3D view shown yet, it is the camera of the settings. It follows the ball as on the screen, gliding as much as it would in the time of an image (1 / the images a second).
- At the start of each range, the camera jumps to where it goes: in one video of several ranges, it does not glide from one scene to the next.
- What is drawn follows the settings of the 3D view: the figures, the shadows, the stands, the flying ball, the compass and the minimap.
- The images are 3840 x 2160 at most.
- Without OpenGL 3.3, or in a monitor built without the 3D view, `3D view` is disabled, and its tooltip tells why.

## Rates, speeds and lengths

An image advances `speed × 1000 / ( step × fps )` frames, where `step` is the simulation step (100 ms) and `fps` the images a second.
The images of a range are at its first frame and every that many frames after it, up to its last frame.

| View | Images/s | Speed | Frames an image | 1 minute of the game (600 frames) |
|---|---|---|---|---|
| 2D | 10 | x 1 | 1 | 600 images, a video of 60 s |
| 2D | 10 | x 2 | 2 (every other frame) | 300 images, 30 s |
| 2D | 20 | x 2 | 1 | 600 images, 30 s |
| 2D | 10 | x 0.5 | 0.5 (each frame twice) | 1200 images, 120 s |
| 3D | 30 | x 1 | 1/3 (between the frames) | 1800 images, 60 s |
| 3D | 60 | x 1 | 1/6 | 3600 images, 60 s |

The 2D view draws the frame at or before each image's position; the 3D view draws between the frames.
Several ranges in one video last as long as all of them.

## The command line

The export runs from the command line without a window, and quits:

```bash
rcssmonitor --export-video FILE [OPTIONS] GAME_LOG
rcssmonitor --export-images DIR [OPTIONS] GAME_LOG
```

It reads the game log, exports it as the dialog does, tells the progress on the standard error, and quits with the exit code 0 when it is done, or 1 with the reason (`rcssmonitor: ...`) when it cannot be done.

- It uses the settings of the monitor (the colors, the fonts, the score board, the options of the 3D view) with the options on the command line (e.g. `--camera-3d`), and saves nothing.
- The defaults are fixed, not the fields of the dialog: a script gives the same result whatever was last done in the dialog.
- The files there are overwritten without asking.

### Options

| Option | Default | |
|---|---|---|
| `--export-video FILE` | | a video by ffmpeg: `.mp4`, `.mov` or `.mkv` (`.mp4` when there is no extension) |
| `--export-images DIR` | | images in DIR |
| `--export-view 2d\|3d` | `2d` | the view that draws the images |
| `--export-range RANGE` | `all` | a range ([below](#ranges-on-the-command-line)); given again for more |
| `--export-join` | off | the ranges in one video (or one sequence of images), not each in its own |
| `--export-fps N` | 10 (2D), 30 (3D) | the images a second |
| `--export-speed N` | 1 | how many times as fast as the game is played |
| `--export-size WxH` | `1280x720` | the size of the images (even for a video) |
| `--export-scale R` | the height / 720 | the images look as a view of their size / R, enlarged R times (the texts, the logos, the lines): by default, as a view 720 pixels high |
| `--export-image-format png\|jpg` | `png` | |
| `--export-image-prefix STR` | `image-` | the start of the names of the images |
| `--export-crf N` | 18 | the quality of the video, 0 to 51 |
| `--export-preset STR` | `medium` | the encoding speed: `ultrafast`, `superfast`, `veryfast`, `faster`, `fast`, `medium`, `slow`, `slower`, `veryslow` |
| `--ffmpeg PATH` | `ffmpeg` in the PATH | |

`rcssmonitor --help` lists them too. Giving one of them without `--export-video` or `--export-images` is a mistake.

### Ranges on the command line

| `RANGE` | |
|---|---|
| `START-END` | from START to END, written as in the dialog (`1234`, `1234.3`). An end may be empty: `-300` from the first frame, `3000-` to the last one |
| `all` | the whole log |
| `first-half`, `second-half`, `half-N` | a half (`half-3` and on with more than two halves) |
| `extra-N` | a half of the extra time |
| `penalties` | the penalty shootout |
| `game` | the whole game, when the halves have no time |
| `goals` | a range for each goal, from 10 s before it to 5 s after it (the seconds of the playback, as the [presets](#presets)), named `goal1-c437`, ... |
| `goals:BEFORE:AFTER` | the same, from BEFORE seconds before each goal to AFTER seconds after it |

Several ranges are written each in its own video (`FILE-NAME.ext`, e.g. `goals-goal1-c437.mp4`) or directory (`DIR/NAME/`), named by the ranges (a `START-END` range by its text), or in one with `--export-join`.
A part not in the log is a mistake; `goals` on a log without a goal gives no range, and is a mistake when it is the only range.

### Examples

```bash
# the goals, a video each: goals-goal1-c437.mp4, goals-goal2-c797.mp4, ...
rcssmonitor --export-video goals.mp4 --export-range goals GAME_LOG.rcg.gz

# the first half as one video at twice the speed of the game
rcssmonitor --export-video first-half.mp4 --export-range first-half --export-speed 2 GAME_LOG.rcg.gz

# images of the cycles 1200 to 1500 in full HD
rcssmonitor --export-images frames --export-range 1200-1500 --export-size 1920x1080 GAME_LOG.rcg.gz

# the goals in 3D, from 5 s before to 3 s after each, in one video, with the close-up camera of the ball
rcssmonitor --export-video goals-3d.mp4 --export-view 3d --export-range goals:5:3 --export-join \
  --camera-3d closeup-ball GAME_LOG.rcg.gz

# every game log of a directory
for log in logs/*.rcg.gz; do
  rcssmonitor --export-video "videos/$(basename "$log" .rcg.gz).mp4" --export-range goals --export-join "$log" || echo "failed: $log"
done
```

The standard error tells, e.g.:

```
ffmpeg version 6.1.1-3ubuntu5; H.264 by libx264
Exporting 604 images of 604 frames in 4 ranges, 1280 x 720 pixels, 10 images/s, by the 2D view, to 4 videos (goals-goal1-c437.mp4, ...)
Exported 61 of 604 images (10%)
...
Exported 604 images; finishing the video
Exported goals-goal1-c437.mp4
Exported goals-goal2-c797.mp4
Exported goals-goal3-c1075.mp4
Exported goals-goal4-c3536.mp4
Finished in 1.6 s
```

### Without a display

The command line export shows no window: it runs on Qt's platform `offscreen`, whatever the desktop sets in `QT_QPA_PLATFORM` (Qt's option `-platform` still chooses another).

- The 2D view needs no display: it runs on a server, over ssh, in a container.
- The 3D view needs OpenGL 3.3, which it gets from an X display: on the desktop it simply runs; on a machine without a display, run it under Xvfb, a virtual X display, with Mesa's software drawing (slower, but it works without a GPU):

  ```bash
  sudo apt install xvfb xauth libgl1-mesa-dri
  xvfb-run -a rcssmonitor --export-video goals-3d.mp4 --export-view 3d --export-range goals GAME_LOG.rcg.gz
  ```

  Without one, it tells `The 3D view cannot be drawn: OpenGL 3.3 is not available. ... run it under xvfb-run.` and quits with 1.

## Sizes of the files

Some sizes, to plan the disk space (they depend on the game, the settings and the machine):

| What | Size | Time to export |
|---|---|---|
| 2D, a PNG of 1920 x 1080 | about 100 KB | |
| 3D, a PNG of 1280 x 720 | about 400 KB | |
| 2D video, 1280 x 720, 10 images/s, CRF 18: a half (5.5 min) | about 11 MB | a few seconds |
| 3D video, 1280 x 720, 30 images/s, CRF 18: a scene of 20 s | 6 to 9 MB | a few seconds with a GPU |
| 3D video, 1920 x 1080, 60 images/s, CRF 18: a scene of 20 s | about 13 MB | about 15 s with a GPU |

A whole game in 3D images at 30 images/s is about 19,000 images: several GB. A video is much smaller than its images.

## Tips

rcssmonitor exports the frames of the game as they are. Editing the videos is left to ffmpeg: the recipes below use its filters (ffmpeg 4.2 or later).
They encode again with the settings of rcssmonitor's videos: drop `-tune animation` for a video of the 3D view, and give `-framerate` the images a second of the export (10 for 2D, 30 for 3D by default).

### Still frames at the start and the end

A video starts with the game moving and ends as the play does. The filter `tpad` holds its first frame and its last frame for some seconds (here 2 and 3):

```bash
ffmpeg -i goals.mp4 \
  -vf "tpad=start_mode=clone:start_duration=2:stop_mode=clone:stop_duration=3" \
  -c:v libx264 -crf 18 -preset medium -tune animation -pix_fmt yuv420p \
  -movflags +faststart goals-still.mp4
```

`-movflags +faststart` lets a browser play the video while it is loading.
A longer range (e.g. `goals:15:8`, from 15 s before each goal) gives a longer lead-in, with the game moving.

### Pauses between the scenes

To have still frames around each scene of several ranges in one video, export a video for each range (without `--export-join`), hold the ends of each, and join them without encoding again:

```bash
rcssmonitor --export-video goals.mp4 --export-range goals GAME_LOG.rcg.gz
for f in $(ls -v goals-goal*.mp4); do
  ffmpeg -y -i "$f" \
    -vf "tpad=start_mode=clone:start_duration=2:stop_mode=clone:stop_duration=2" \
    -c:v libx264 -crf 18 -preset medium -tune animation -pix_fmt yuv420p "pad-$f"
done
printf "file '%s'\n" $(ls -v pad-goals-goal*.mp4) > list.txt
ffmpeg -f concat -safe 0 -i list.txt -c copy -movflags +faststart goals-all.mp4
```

`ls -v` puts the scenes in the order of their numbers (`goal10` after `goal9`).
The videos joined by `-c copy` must have the same size, rate and encoding, as these do.

### One compression from the images

Each recipe above compresses a compressed video again. At CRF 18 it is hard to see, but the images exported by rcssmonitor can be made into the video at once instead:

```bash
rcssmonitor --export-images frames --export-range 1200-1500 GAME_LOG.rcg.gz
ffmpeg -framerate 10 -i frames/image-%05d.png \
  -vf "tpad=start_mode=clone:start_duration=2:stop_mode=clone:stop_duration=3" \
  -c:v libx264 -crf 18 -preset medium -tune animation -pix_fmt yuv420p \
  -movflags +faststart scene.mp4
```

Or export the video without loss first (`--export-crf 0`, a large file), and compress it once at the end.

### Fading from and to black

The filter `fade` fades in at the start and out at the end; the fade out needs the length of the video. With the still frames, in one go (a video of 20.1 s, made 25.1 s long):

```bash
ffmpeg -i goals.mp4 \
  -vf "tpad=start_mode=clone:start_duration=2:stop_mode=clone:stop_duration=3,fade=t=in:st=0:d=1,fade=t=out:st=24.1:d=1" \
  -c:v libx264 -crf 18 -preset medium -tune animation -pix_fmt yuv420p goals-faded.mp4
```

The length of a video, to find where the fade out starts (its length - 1 s):

```bash
ffprobe -v error -show_entries format=duration -of csv=p=0 goals-still.mp4
```

## When something goes wrong

**`Export` is disabled.**
The summary above it tells why in red: no game log, a position not in the log, the same frames named by the cycles twice in one sequence, ffmpeg not found, a video file of another extension, a 3D size over 3840 x 2160. While connected to a server, the tooltip of `Export` tells so: disconnect, and the frames kept can be exported.

**The images look cut, or the texts too large or too small.**
Check `Enlarge the texts, logos and lines with the image`: on, the image is laid out as the view; off, the texts keep their sizes in pixels. A size of another shape than the view shows more or less of the field around it.

**`ffmpeg ... is not found`, or the video cannot be made.**
Install ffmpeg (`sudo apt install ffmpeg`), or choose it with `Browse...` (`--ffmpeg PATH` on the command line). When ffmpeg fails, the message ends with what it said.

**`no H.264: MPEG-4 Part 2 by mpeg4`.**
This ffmpeg has no H.264 encoder: the video is made, larger and of lower quality, and some players do not play it. An ffmpeg with libx264 makes H.264.

**The video does not play.**
A video stopped before its end is removed, as it may not play. `.mp4` plays in most players and browsers; some browsers do not play `.mkv`.

**`3D view` is disabled.**
Its tooltip tells why: no OpenGL 3.3, or a monitor built without the 3D view. On the command line, the 3D view needs an X display ([Without a display](#without-a-display)).

**The 3D export is slow.**
The 3D view draws every image with the GPU: without one (Mesa's software drawing, e.g. under Xvfb), a large size and 60 images/s take long. A smaller size, 30 images/s, or the smooth edges off (the option `antialias_3d`) are faster.
