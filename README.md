# RoboCup Soccer Simulator Monitor
[![GitHub license](https://img.shields.io/github/license/rcsoccersim/rcssmonitor)](https://github.com/rcsoccersim/rcssmonitor/blob/master/COPYING)

![Logo](https://user-images.githubusercontent.com/1832537/49242985-f69a3c00-f3ea-11e8-97f5-9b0bfdfc4e1c.png)

**RoboCup Soccer Simulator Monitor** (rcssmonitor) is used to view the simulation as it takes place by connecting to the [RoboCup Soccer Simulator Server](https://github.com/rcsoccersim/rcssserver) (rcssserver) or to view the playback of a simulation by loading game log files.
It shows the game in 2D and in 3D.

The releases provide the source code, built as below (no AppImage is provided from version 20.0.0).
To view game logs without installing anything, the browser version, rcssmonitor-web, can also be used.

## :soccer: Quick Start

rcssmonitor is written in C++17 and depends on [Qt](https://www.qt.io/) 6 (6.4 or later) or Qt 5 (5.15). It is built with [CMake](https://cmake.org/) (3.16 or later).

First, install the tools and libraries. On Ubuntu:
```bash
sudo apt install build-essential cmake zlib1g-dev libdeflate-dev
```
and Qt 6, whose packages differ between the releases:

| Ubuntu | Qt 6 packages |
|---|---|
| 26.04 | `qt6-base-dev libqt6opengl6-dev qt6-websockets-dev qt6-multimedia-dev` |
| 24.04 | `qt6-base-dev qt6-websockets-dev qt6-multimedia-dev` |

e.g. `sudo apt install qt6-base-dev qt6-websockets-dev qt6-multimedia-dev` on Ubuntu 24.04, or Qt 5 on any of them: `sudo apt install qtbase5-dev libqt5websockets5-dev qtmultimedia5-dev libqt5multimedia5-plugins`.
libdeflate (faster reading of gzip-compressed game logs), Qt WebSockets (the WebSocket connection to the server) and Qt Multimedia ([the referee's whistle](#the-referees-whistle)) are optional.

Then, clone the repository, or download the tarball from the [releases section](https://github.com/rcsoccersim/rcssmonitor/releases) and extract it. From its directory, execute:

```bash
cmake -S . -B build
cmake --build build --parallel
```

`build/rcssmonitor` will be the binary for the monitor. CMake uses Qt 6 if it is found, and Qt 5 otherwise; to use Qt 5 when both are installed, pass `-DQT_VERSION_MAJOR=5` to the first command.

`cmake-helper.sh` in the top directory runs the frequently used CMake commands, printing each of them:

```bash
./cmake-helper.sh build      # configure (if needed) and build
./cmake-helper.sh test       # build and run the tests
./cmake-helper.sh install    # build and install (into ~/.local by default)
./cmake-helper.sh help       # show the usage
```

[docs/build.md](docs/build.md) describes the details: the requirements, the CMake options, `cmake-helper.sh`, installing and uninstalling, the tests and the source tarball.

## :package: Installing

```bash
cmake --install build
```

installs the monitor into `~/.local` by default (`--prefix DIR` installs into another directory).
It also installs the desktop entry, so that rcssmonitor appears in the application menu: search for `rcssmonitor`, `RoboCup` or `soccer` (e.g. in GNOME's Activities), and right-click it for `Connect to the Server`.
On Ubuntu, `~/.local/bin` is in `PATH` after you log in again, and `rcssmonitor` can be started by its name.
See [docs/build.md](docs/build.md#installing-and-uninstalling) for the details.

## :wastebasket: Uninstalling

```bash
cmake --build build --target uninstall
```

removes the files installed from the build directory.

## :play_or_pause_button: Using the Monitor

To start the monitor either type

```bash
./rcssmonitor
```

from the directory containing the executable or

```bash
rcssmonitor
```

If you invoke `rcssmonitor` with `--help` option, available options are displayed in your console.

### Opening a game log from a URL

`File` -> `Open Game Log from URL...` (`Ctrl+Shift+O`) downloads a game log (`.rcg` or `.rcg.gz`) from an `http://` or `https://` URL, and opens it.
A file page on GitHub (`https://github.com/OWNER/REPO/blob/BRANCH/PATH`) can be given as it is: it is read from its raw file, and a file stored with Git LFS from its content.
A URL can also be given on the command line, or dropped on the window from a browser:

```bash
rcssmonitor https://github.com/rcsoccersim/RoboCup2026/blob/main/logs/Finals/202607050404-HELIOS2026_0-vs-YuShan2026_1.rcg.gz
```

- The downloaded logs are kept in `~/.cache/rcssmonitor/logs`, and a URL opened again is opened from there. `Clear Cache` in the dialog removes them. The dialog also remembers the URLs opened recently.
- `File` -> `Save a Copy of Game Log...` (`Ctrl+Shift+S`) saves a copy of the file of the game log open, e.g. one downloaded.
- A game log file dropped on the window is opened too, as by `File` -> `Open Game Log`.
- A download is up to 200 MB. `https://` needs the TLS of Qt with OpenSSL ([docs/build.md](docs/build.md#notes)).

### Browsing game logs

`File` -> `Browse Game Logs...` (`Ctrl+Shift+B`) lists game logs in the directories of their sources: the date, the teams, the score (the penalty shootout in parentheses) and the size of each log.
The sources are, at first, the logs of RoboCup 2026 on GitHub ([rcsoccersim/RoboCup2026](https://github.com/rcsoccersim/RoboCup2026)) and the 2D logs of the archive ([archive.robocup.info](https://archive.robocup.info/Soccer/Simulation/2D/logs/), RoboCup 2004 to 2023 and RCAP).
A double click, or `Open`, downloads a log and opens it, as a URL does; the dialog stays open, to open one game after another.
The filter shows only the games of a team (`Team:`, a part of its name in any case) on either side or one side (`Side:`), against an opponent (`vs:`), with the result of the team (`Result:`: a win, a draw or a loss; the winner of a penalty shootout wins), and between the dates checked (`From:`, `To:`); `Clear` shows all the games again.
A click on a column's header sorts the logs by the column.

- `Add...` adds a source: a public repository on GitHub with game logs (`OWNER/REPO`, `OWNER/REPO@BRANCH`, or its URL), or the URL of a directory on a web server that lists its files: by [h5ai](https://larsjung.de/h5ai/), as the archive (e.g. a year of it), or by a plain page of links, as the autoindex of Apache or nginx (which one is found out by itself). `Remove` removes any source, a default one too; `Add...` offers the default ones removed. The sources are kept in the settings.
- The list of a repository is read by one request to the GitHub API, and kept in `~/.cache/rcssmonitor/github` for an hour (`Reload` reads it again). GitHub allows 60 requests an hour without an account: when they are used up, the dialog tells until when, and shows the list kept before, however old.
- The archive, and any directory on a web server, is read a directory at a time, when it is opened, and kept in `~/.cache/rcssmonitor/archive` for a day (`Reload` reads the directories again as they are opened). When it cannot be read, the list kept is shown, however old. The sizes on a page of links are as the page shows them: rounded by Apache (e.g. `5.0M`), and none by some servers.
- The filter applies to the logs listed: a directory of the archive not read yet is shown until it is opened.

### Saving images of the view

`File` -> `Save Image of This Frame...` (`Ctrl+Shift+I`) saves the frame shown in the 2D view to an image file (PNG, or JPEG by the extension), as large as the view: on a high-DPI screen, with as many pixels as the screen shows.
The default name is the game log's name and the cycle, e.g. `202607050404-HELIOS2026_0-vs-YuShan2026_1-301.png`.
The texts are drawn with grayscale antialiasing.
It is not available while the 3D view is shown yet.

`File` -> `Export Images / Video...` (`Ctrl+Shift+E`) exports a range of the frames of the game log as image files (PNG or JPEG), drawn by the 2D view:

- The range: the start and the end are cycles (`1234`) or stopped frames (`1234.3`, as the status bar shows them), empty for the first and the last frames. A cycle as the end includes its stopped frames. `Now` takes the position shown: the dialog stays open while you move through the log. `All` empties both.
- The size: as the view, 1280 × 720, 1920 × 1080, 3840 × 2160, or any. `Scale the texts with the image` draws the texts as large in the image as in the view.
- The rate: the images a second and the speed (10 images/s at × 1 draws each frame once).
- The images are named by their numbers (`image-00000.png`), or by the cycles (`image-c01234.00.png`) when an image is a frame. The files already there are told once before the export, and the disk space is estimated.
- It is not available while connected to a server. The view and the frame shown do not change.

### The 3D view

`D`, or the `3D View` button in the tool bar, switches between the 2D and 3D views (`--view-3d on` starts in 3D).
The 3D view has several cameras (`E` for the next one), draws the players as cylinders or as human figures moving by the game log, and can let the ball fly in the air.
It needs OpenGL 3.3, and a build with the 3D view ([docs/build.md](docs/build.md)).
[docs/3d-view.md](docs/3d-view.md) describes the cameras, the mouse and keys, and the settings, and [docs/FIGURE_PACKS.md](docs/FIGURE_PACKS.md) how to add figures of your own.

In a Wayland session with a screen of high resolution such as 4K, start the monitor with `QT_QPA_PLATFORM=wayland` for a smooth 3D view (Qt may run it through XWayland, which can slow the 3D view down):

```bash
QT_QPA_PLATFORM=wayland rcssmonitor
```

### The color scheme (light or dark)

`View` -> `UI Color Scheme` chooses the colors of the menus, the tool bars and the dialogs: `System` (the desktop's, by default), `Light` or `Dark`.
The choice is kept in `~/.rcssmonitor.conf`.
`--ui-color-scheme` chooses one for a run, and is not kept:

```bash
rcssmonitor --ui-color-scheme dark    # system, light or dark
```

- `Light` and `Dark` look the same with Qt 5 and Qt 6. `System` follows a dark desktop with Qt 6, but not with Qt 5: choose `Dark` there.
- The icons of the tool bar have images for both schemes, and switch with the scheme ([docs/icons_en.md](docs/icons_en.md) describes how they are made).
- The field (2D and 3D) keeps its own colors, set on the `Color` tab of the settings dialog.
- `View` -> `Window Style` chooses the style of the widgets (e.g. `Fusion`), which is kept too.

### Connecting via TCP or WebSocket

By default, the monitor connects to the server via UDP.
If the server supports the `server::tcp_port` and `server::ws_port` parameters, the monitor can also connect via TCP or WebSocket.
These connections are disabled in the server by default. Enable them by giving the port numbers:

```bash
rcssserver server::tcp_port=6000 server::ws_port=6080
```

Then, select the transport by `--server-transport` and give the corresponding port number by `--server-port`:

```bash
rcssmonitor --connect                                            # UDP (server::port)
rcssmonitor --connect --server-transport tcp --server-port 6000  # TCP (server::tcp_port)
rcssmonitor --connect --server-transport ws --server-port 6080   # WebSocket (server::ws_port)
```

The transport, the port number and the request path can also be given by `Monitor` -> `Connect to ...` in the menu bar.

Note the following points:

- The protocol versions 1 and 2 (`--client-version 1` or `2`) are available only via UDP, because they use a binary format. Use the version 3 or later, or -1 (JSON format), via TCP and WebSocket.
- The WebSocket connection uses the request path `/monitor` by default. It can be changed by `--server-ws-path` (e.g. `/player`).
- `wss://` (WebSocket over TLS) is not supported.

### Drawing a live game some seconds behind (Live Delay)

`Live Delay` (the clock button in the tool bar, or `Monitor` -> `Live Delay` in the menu bar) draws a live game some seconds behind the newest frame from the server, in the 2D and 3D views alike.
The tool bar shows how far behind (e.g. `LIVE -3.0 s`), and the time and the score shown are those of the frame drawn.

The 3D view can draw the ball flying in the air (`Flight` on the `Players & Ball` tab of the settings dialog, or `--ball-flight-3d on`; [docs/3d-view.md](docs/3d-view.md#the-flying-ball-flight)), although today's server has no height.
Each kick's flight is found from the next touch of the ball, which is known in a game log, but not yet in a live game as it comes.
Drawn some seconds behind, the touches ahead of the frame drawn have come, and the ball flies in a live game as in a game log, e.g. on the screen at a competition.
A kick whose next touch has not come when it is drawn stays on the ground.

The seconds are set by `Seconds` in `Live` on the `Show` tab of the settings dialog (1 to 7 seconds in steps of 0.5; 3 by default), where `Delay` also turns it on or off, or on the command line:

```bash
rcssmonitor --connect --live-delay 3    # 3 seconds behind; --live-delay 0 turns it off
```

- 3 seconds draw almost every flight as in the game log. With less, more kicks stay on the ground. 7 seconds, the largest, is about the time the ball takes from its top speed to an almost stop.
- Right after connecting, the first frame waits until the seconds' frames have come. When the game or the connection ends, the remaining frames are drawn to the end.
- The referee's operations (e.g. `Kick Off`) reach the server at once, while the view shows the game some seconds ago. To operate the game, turn the delay off, or use another monitor without it.
- The setting is kept when the monitor quits (`live_delay` and `live_delay_seconds` in `~/.rcssmonitor.conf`).

### The referee's whistle

`View` -> `Referee's Whistle` blows the referee's whistle when the play mode changes, as a referee in football does.
It is off by default, so that many monitors at a venue do not whistle at once. The choice is kept in `~/.rcssmonitor.conf`.

| When | Whistle |
|---|---|
| A kick-off: the start of the game, of the second half and of extra time, and after a goal | one long |
| A foul, an offside, a back pass and so on, which stop the play for a free kick | short |
| The free kick after one of them (the restart, after the defenders keep their distance) | short |
| A kick of the penalty shootout may be taken (`penalty_ready`) | short |
| The end of a half, in a live game | two long |
| The end of the game | three long |

As in the guidelines of the Laws of the Game, no whistle is blown for a goal, a kick-in, a corner kick, a goal kick, a free kick after the goalkeeper's catch, a dropped ball and so on.
In a game log, the next half starts with its kick-off right after the play, without the end of the half, so only the kick-off's whistle is blown there.

- In a game log, the whistle is blown when the game is played or stepped forward, not when it jumps (the slider, a cycle, the first or the last frame) nor when it goes back. Playing from the first frame blows the kick-off of the game.
- In a live game, it is blown when the change is drawn: some seconds later with `Live Delay`.
- `Volume` in `Sound` on the `Show` tab of the settings dialog sets the loudness (0 to 100; 50 by default). `Test` there blows the short whistle at that volume, while the whistle is on.
- `--whistle` and `--whistle-volume` set them for a run, and are not kept:

  ```bash
  rcssmonitor --whistle on --whistle-volume 80 game.rcg.gz
  ```

- It needs a build with Qt Multimedia ([docs/build.md](docs/build.md)); without it, the menu item and the settings are disabled. With Qt 5, the audio plugins of Qt Multimedia are needed at run time (`libqt5multimedia5-plugins` on Ubuntu).
- While the whistle is on, the monitor keeps its audio output open, playing silence between the whistles, so that the output is not put to sleep: a display's speakers on HDMI and the like lose the start of a sound while they wake up. The monitor is listed as an application playing sound then.
- Such speakers may still take a few seconds to start sounding after the whistle is turned on or the monitor starts. A whistle in those seconds may not be heard: wait a moment before playing.

## :incoming_envelope: Contributing

For bug reports, feature requests and latest updates, please goto
https://github.com/rcsoccersim/rcssmonitor and open an issue or a pull request.

> The RoboCup Soccer Server Maintainance Group
