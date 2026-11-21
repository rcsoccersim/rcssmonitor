# RoboCup Soccer Simulator Monitor
[![GitHub license](https://img.shields.io/github/license/rcsoccersim/rcssmonitor)](https://github.com/rcsoccersim/rcssmonitor/blob/master/COPYING)

![Logo](https://user-images.githubusercontent.com/1832537/49242985-f69a3c00-f3ea-11e8-97f5-9b0bfdfc4e1c.png)

**RoboCup Soccer Simulator Monitor** (rcssmonitor) is used to view the simulation as it takes place by connecting to the [RoboCup Soccer Simulator Server](https://github.com/rcsoccersim/rcssserver) (rcssserver) or to view the playback of a simulation by loading game log files.

## :soccer: Quick Start

rcssmonitor is written in C++17 and depends on [Qt](https://www.qt.io/) 6 (6.2 or later) or Qt 5 (5.15). It is built with [CMake](https://cmake.org/) (3.16 or later); autotools can build it with Qt 5 only.

First, install the tools and libraries. On Ubuntu 22.04, 24.04 or 26.04, with Qt 6:
```bash
sudo apt install build-essential cmake qt6-base-dev qt6-websockets-dev zlib1g-dev libdeflate-dev
```
or with Qt 5:
```bash
sudo apt install build-essential cmake qtbase5-dev libqt5websockets5-dev zlib1g-dev libdeflate-dev
```
zlib (`zlib1g-dev`) is needed to load gzip-compressed game logs (`.rcg.gz`). libdeflate (`libdeflate-dev`) is optional: if it is found, gzip game logs are loaded faster. Qt WebSockets (`qt6-websockets-dev`, `libqt6websockets6-dev` on Ubuntu 22.04, or `libqt5websockets5-dev` with Qt 5) is needed only for the WebSocket connection to the server. Without it, the monitor is built without the WebSocket connection.

Then, clone the repository, or download the tarball from the [releases section](https://github.com/rcsoccersim/rcssmonitor/releases) and extract it. From its directory, execute:

```bash
cmake -S . -B build
cmake --build build
```

`build/rcssmonitor` will be the binary for the monitor. CMake uses Qt 6 if it is found, and Qt 5 otherwise; to use Qt 5 when both are installed, pass `-DQT_VERSION_MAJOR=5` to the first command.

### cmake-helper.sh

`cmake-helper.sh` in the top directory runs the frequently used CMake commands.
It prints each command before running it, so you can also learn the actual cmake commands.
It can be run from any directory; it always works in the top directory of rcssmonitor.

```bash
./cmake-helper.sh build        # configure (if needed) and build
./cmake-helper.sh test         # build and run the tests
./cmake-helper.sh run          # build and run rcssmonitor
./cmake-helper.sh run -- -c    # build and run rcssmonitor with options (e.g. connect to rcssserver)
./cmake-helper.sh install      # build and install (into ~/.local by default)
./cmake-helper.sh uninstall    # remove the installed files
./cmake-helper.sh help         # show the usage
```

The second argument selects the Qt version and the build directory:

| Argument | Qt | Build directory |
|---|---|---|
| `auto` (default) | Qt 6 if available, otherwise Qt 5 | `build` |
| `qt6` | Qt 6 | `build-qt6` |
| `qt5` | Qt 5 | `build-qt5` |

For example, `./cmake-helper.sh build qt5` builds rcssmonitor with Qt 5 in `build-qt5`.

All the commands:

| Command | What it does | The main cmake command (for `qt6`) |
|---|---|---|
| `configure [QT]` | (re)configure the build directory | `cmake -S . -B build-qt6 -DQT_VERSION_MAJOR=6` |
| `build [QT]` | configure (if needed) and build | `cmake --build build-qt6 --parallel JOBS` |
| `test [QT]` | build and run the tests | `cd build-qt6 && ctest --output-on-failure` |
| `run [QT] [-- ARGS...]` | build and run rcssmonitor | `build-qt6/rcssmonitor ARGS...` |
| `install [QT] [--prefix DIR]` | build and install | `cmake --install build-qt6 [--prefix DIR]` |
| `uninstall [QT]` | remove the files installed from the build directory | `cmake --build build-qt6 --target uninstall` |
| `dist [QT]` | create the source tarball `rcssmonitor-VERSION.tar.gz` from the committed files of HEAD | `cmake --build build-qt6 --target dist` |
| `distcheck [QT]` | create the source tarball and check that it can be built, tested, installed and uninstalled | `cmake --build build-qt6 --target distcheck` |
| `clean [QT\|all]` | remove the build directory (`all`: all three) | `rm -rf build-qt6` |

Options for configure are given by the environment variable `CMAKE_ARGS` (configure is run again when it is set), e.g. `CMAKE_ARGS="-DENABLE_WEBSOCKET=OFF" ./cmake-helper.sh build`. `JOBS` sets the number of parallel build jobs (the number of CPUs by default).

### Building with autotools (Qt 5 only)

autotools needs the Qt 5 packages above and also `autoconf automake libtool qt5-qmake`. From the directory of the repository, execute:

```bash
./bootstrap
./configure
make
```

The tarball has `configure` already, so `./bootstrap` is not needed there. `src/rcssmonitor` will be the binary for the monitor.

## :hammer_and_wrench: Configuring & Building

Before building rcssmonitor you will need to run the `configure` script located in the root of the distribution directory.

The default configuration will set up to install the monitor components in the following location:

`/usr/local/bin` for the executables

You may need administrator privileges to install the monitor into the default location. This locations can be modified by using configure's `--prefix=DIR` and related options.  See `configure --help` for more details.
```bash
./configure --prefix=YOUR_INSTALLATION_DIR
```

The monitor has several features that can be enabled or disabled at configure time by using the `--enable-FEATURE[=ARG]` or `--disable-FEATURE` parameters to `configure`.  `--disable-FEATURE` is equivalent to `--enable-FEATURE=no` and `--enable-FEATURE` is equivlant to `--enable-FEATURE=yes`.  The only valid values for `ARG` are `yes` and `no`.

The WebSocket connection is enabled if Qt WebSockets is found. To disable it, use `--disable-websocket`.

Once you have successfully configured the monitor, simply run `make` to build the sources.

With CMake, the options are given to the first command, e.g. `cmake -S . -B build -DCMAKE_INSTALL_PREFIX=YOUR_INSTALLATION_DIR`:

- `CMAKE_INSTALL_PREFIX`: the installation directory (`~/.local` by default; a build directory configured before keeps its value). The installed `rcssmonitor` finds its library in the directory (`lib`) by itself, and the desktop entry starts it by its full path.
- `QT_VERSION_MAJOR`: `5` or `6`, the Qt to use (Qt 6 if it is found, by default)
- `ENABLE_LIBDEFLATE`: `OFF` not to use libdeflate even if it is found (`ON` by default). With autotools, pass `--without-libdeflate` to `configure`.
- `ENABLE_WEBSOCKET`: `OFF` not to build the WebSocket connection even if Qt WebSockets is found (`ON` by default). With autotools, pass `--disable-websocket` to `configure`.

If CMake is chosen, `ccmake` command is available for the configuration:

```bash
cd build
ccmake ..
```

## :package: Installing

When you have completed building the monitor, it's components can be installed into their default locations or the locations specified during configuring by running
```bash
cmake --install build
```
in the case of CMake (into `~/.local` by default; `cmake --build build --target uninstall` removes the installed files), or
```bash
make install
```
in the case of autotools.
Depending on where you are installing the monitor, you may need special permissions.

If you install the monitor under the location you specified, you may need to set the environment variable `PATH`.
Otherwise, you have to enter the full path to the executable when you execute the monitor.
In the case of Linux system, the recommended way is to add the following line at the bottom of `~/.profile`:
```
PATH="YOUR_INSTALLATION_DIR/bin:$PATH"
```
Then, log out and log in again.

You may also need to set the library path before executing the monitor.
In most Linux systems, you can modify the library path by editing `/etc/ld.so.conf` or the environment variable `LD_LIBRARY_PATH`.
The recommended way is to use `LD_LIBRARY_PATH` because all users can edit this variable without extra privileges.
Add the following line at the bottom of `~/.bashrc`:
```
LD_LIBRARY_PATH="YOUR_INSTALLATION_DIR/lib:$LD_LIBRARY_PATH"
export LD_LIBRARY_PATH
```
Then, open a new terminal.

## :wastebasket: Uninstalling
In the case of autotools, the monitor can also be easily removed by entering the distribution directory and running
```bash
make uninstall
```
This will remove all the files that where installed, but not any directories that were created during the installation process.

In the case of CMake, find `install_manifest.txt` under the build directory, then execute:
```bash
xargs rm < install_manifest.txt
```


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
