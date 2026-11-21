# RoboCup Soccer Simulator Monitor
[![GitHub license](https://img.shields.io/github/license/rcsoccersim/rcssmonitor)](https://github.com/rcsoccersim/rcssmonitor/blob/master/COPYING)

![Logo](https://user-images.githubusercontent.com/1832537/49242985-f69a3c00-f3ea-11e8-97f5-9b0bfdfc4e1c.png)

**RoboCup Soccer Simulator Monitor** (rcssmonitor) is used to view the simulation as it takes place by connecting to the [RoboCup Soccer Simulator Server](https://github.com/rcsoccersim/rcssserver) (rcssserver) or to view the playback of a simulation by loading game log files.

## :soccer: Quick Start

rcssmonitor is written in C++17 and depends on [Qt](https://www.qt.io/) 6 (6.2 or later) or Qt 5 (5.15). It is built with [CMake](https://cmake.org/) (3.16 or later).

First, install the tools and libraries. On Ubuntu:
```bash
sudo apt install build-essential cmake zlib1g-dev libdeflate-dev
```
and Qt 6, whose packages differ between the releases:

| Ubuntu | Qt 6 packages |
|---|---|
| 26.04 | `qt6-base-dev libqt6opengl6-dev qt6-websockets-dev` |
| 24.04 | `qt6-base-dev qt6-websockets-dev` |
| 22.04 | `qt6-base-dev libqt6opengl6-dev libqt6websockets6-dev libgl-dev` |

e.g. `sudo apt install qt6-base-dev qt6-websockets-dev` on Ubuntu 24.04, or Qt 5 on any of them: `sudo apt install qtbase5-dev libqt5websockets5-dev`.
libdeflate (faster reading of gzip-compressed game logs) and Qt WebSockets (the WebSocket connection to the server) are optional.

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
It also installs the desktop entry, so that rcssmonitor appears in the application menu.
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

The 3D view can draw the ball flying in the air (`Flight` on the `Players & Ball` tab of the settings dialog, or `--ball-flight-3d on`), although today's server has no height.
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

## :incoming_envelope: Contributing

For bug reports, feature requests and latest updates, please goto
https://github.com/rcsoccersim/rcssmonitor and open an issue or a pull request.

> The RoboCup Soccer Server Maintainance Group
