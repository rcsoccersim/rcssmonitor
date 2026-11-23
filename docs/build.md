# Building rcssmonitor

This document describes how to build, test, install and uninstall rcssmonitor.

CMake is the official build of rcssmonitor: it builds the monitor with Qt 6 or Qt 5, with the 3D view, and its tests.
The repository also keeps the autotools build, which builds the monitor with Qt 5 only, without the 3D view and the tests ([Building with autotools](#building-with-autotools-qt-5-only)).

- [Requirements](#requirements)
- [Installing the dependencies on Ubuntu](#installing-the-dependencies-on-ubuntu)
- [Building with CMake](#building-with-cmake)
  - [cmake-helper.sh](#cmake-helpersh)
  - [Using cmake directly](#using-cmake-directly)
  - [CMake options](#cmake-options)
  - [Installing and uninstalling](#installing-and-uninstalling)
  - [Running the tests](#running-the-tests)
  - [Creating the source tarball](#creating-the-source-tarball)
- [Building with autotools (Qt 5 only)](#building-with-autotools-qt-5-only)
- [Notes](#notes)

## Requirements

| What | Version | Notes |
|---|---|---|
| A C++ compiler | C++17 | GCC or Clang |
| [CMake](https://cmake.org/) | 3.16 or later | |
| [Qt](https://www.qt.io/) | Qt 6 (6.4 or later) or Qt 5 (5.15) | Core, Gui, Widgets, Network; OpenGL and OpenGLWidgets (Qt 6) for the 3D view; Test for the tests |
| zlib | | to read gzip-compressed game logs (`.rcg.gz`) |
| libdeflate | | optional: if it is found, gzip-compressed game logs are read faster |
| Qt WebSockets | | optional: the WebSocket connection to the server. Without it, the monitor is built without the WebSocket connection |
| Qt Multimedia | | optional: the referee's whistle. Without it, the monitor is built without the whistle. With Qt 5, its audio plugins are needed at run time ([Notes](#notes)) |
| OpenGL 3.3 | at run time | the 3D view. Without it, the monitor runs with the 2D view only |
| OpenSSL | at run time | `https://` URLs of game logs (File > Open Game Log from URL) and the lists of game logs on GitHub (File > Browse Game Logs), through the TLS of Qt Network ([Notes](#notes)) |

## Installing the dependencies on Ubuntu

The tools and the common libraries:

```bash
sudo apt install build-essential cmake zlib1g-dev libdeflate-dev
```

and Qt 6, whose packages differ between the releases:

| Ubuntu | Qt 6 packages |
|---|---|
| 26.04 | `qt6-base-dev libqt6opengl6-dev qt6-websockets-dev qt6-multimedia-dev` |
| 24.04 | `qt6-base-dev qt6-websockets-dev qt6-multimedia-dev` (OpenGL and OpenGLWidgets are in `qt6-base-dev`) |

For example, on Ubuntu 24.04:

```bash
sudo apt install qt6-base-dev qt6-websockets-dev qt6-multimedia-dev
```

Or Qt 5, on any of them (`libqt5multimedia5-plugins` is for the referee's whistle at run time):

```bash
sudo apt install qtbase5-dev libqt5websockets5-dev qtmultimedia5-dev libqt5multimedia5-plugins
```

When both Qt 6 and Qt 5 are installed, CMake uses Qt 6 (see `QT_VERSION_MAJOR` in [CMake options](#cmake-options)).

## Building with CMake

Clone the repository, or download the tarball from the [releases section](https://github.com/rcsoccersim/rcssmonitor/releases) and extract it.
Then build in its directory, by `cmake-helper.sh` or by `cmake` directly.

### cmake-helper.sh

`cmake-helper.sh` in the top directory runs the frequently used CMake commands.
It prints each command before running it, so that you can also learn the actual cmake commands.
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

Options for configure are given by the environment variable `CMAKE_ARGS` (configure is run again when it is set), e.g. `CMAKE_ARGS="-DENABLE_WEBSOCKET=OFF" ./cmake-helper.sh build`.
`JOBS` sets the number of parallel build jobs (the number of CPUs by default).

### Using cmake directly

```bash
cmake -S . -B build
cmake --build build --parallel
```

`build/rcssmonitor` is the monitor. The options are given to the first command, e.g. `cmake -S . -B build -DQT_VERSION_MAJOR=5`.
To change the options of a build directory configured before, run the first command again with them, or use `ccmake build` (or `cmake-gui`).

### CMake options

| Option | Default | Description |
|---|---|---|
| `CMAKE_INSTALL_PREFIX` | `~/.local` | The installation directory. A build directory configured before keeps its value |
| `CMAKE_BUILD_TYPE` | `Release` | `Release`, `Debug`, `RelWithDebInfo` or `MinSizeRel` |
| `QT_VERSION_MAJOR` | Qt 6 if it is found | `5` or `6`: the Qt to use |
| `ENABLE_3D` | `ON` | `OFF` not to build the 3D view. It is also left out if Qt 6's OpenGL or OpenGLWidgets is not found |
| `ENABLE_LIBDEFLATE` | `ON` | `OFF` not to use libdeflate even if it is found |
| `ENABLE_WEBSOCKET` | `ON` | `OFF` not to build the WebSocket connection even if Qt WebSockets is found |
| `ENABLE_SOUND` | `ON` | `OFF` not to build the referee's whistle even if Qt Multimedia is found |
| `RCSSMONITOR_DIST_DIR` | the top source directory | Where the `dist` target creates the source tarball |

CMake tells what it found when it configures, e.g. `-- The 3D view is built` and `-- The referee's whistle is built`.
`build/config.h` also tells it: `HAVE_3D_VIEW`, `HAVE_QT_WEBSOCKETS`, `HAVE_QT_MULTIMEDIA` and `HAVE_LIBDEFLATE` are defined when they are built.

### Installing and uninstalling

```bash
cmake --install build                     # into CMAKE_INSTALL_PREFIX (~/.local by default)
cmake --install build --prefix /opt/rcss  # into another directory
```

The files installed:

| File | |
|---|---|
| `PREFIX/bin/rcssmonitor` | the monitor |
| `PREFIX/lib/librcssrcg.so*` | the library that reads game logs |
| `PREFIX/include/rcss/rcg/*.h` | its headers |
| `PREFIX/share/applications/rcssmonitor.desktop` | the desktop entry: rcssmonitor in the application menu |
| `PREFIX/share/icons/hicolor/SIZExSIZE/apps/rcssmonitor.png` | its icon (SIZE: 16, 32, 48, 64, 128, 256) |
| `PREFIX/share/icons/hicolor/scalable/apps/rcssmonitor.svg` | its icon, scalable |

- The installed `rcssmonitor` finds `librcssrcg` in `PREFIX/lib` by itself (its RPATH is `$ORIGIN/../lib`): `LD_LIBRARY_PATH` is not needed.
- The desktop entry starts the installed `rcssmonitor` by its full path, so it works even if `PREFIX/bin` is not in `PATH`.
- In the desktop's search (e.g. GNOME's Activities), the entry is found by `rcssmonitor`, `RoboCup`, `soccer`, `simulator`, `monitor` or `logplayer`. Its menu (a right click) has `Connect to the Server`, which starts `rcssmonitor --connect`.
- The window of `rcssmonitor` is linked to the entry (the Wayland app_id by `QGuiApplication::setDesktopFileName()`, and `StartupWMClass` on X11), so that the dash shows it with the entry's icon.
- To start `rcssmonitor` from a terminal by its name, `PREFIX/bin` must be in `PATH`. On Ubuntu, `~/.local/bin` is added to `PATH` by `~/.profile` when you log in, if the directory exists then (log in again after the first installation). For another directory, add a line like this at the bottom of `~/.profile`, and log in again:
  ```
  PATH="/opt/rcss/bin:$PATH"
  ```
- Installing into a system directory such as `/usr/local` needs `sudo`: `sudo cmake --install build --prefix /usr/local`.
- `DESTDIR` installs into a staging directory, e.g. for a package: `DESTDIR=/tmp/stage cmake --install build`.

To uninstall, run the `uninstall` target of the build directory that installed the files:

```bash
cmake --build build --target uninstall
```

It removes the files listed in `build/install_manifest.txt` (written by the installation), and the directories of the headers (`include/rcss/rcg` and `include/rcss`) if they become empty.
The other directories (e.g. `bin`, `lib`) are kept.
Run it with `sudo` if the files were installed with `sudo`, and with the same `DESTDIR` if one was given.

### Running the tests

```bash
cd build
ctest --output-on-failure
```

or `./cmake-helper.sh test`. The tests need Qt Test (in `qt6-base-dev` or `qtbase5-dev`).

- The tests run with `HOME` in the build directory, so that your settings file (`~/.rcssmonitor.conf`) is neither read nor written.
- They use Qt's `offscreen` platform: no window is shown.
- The drawing test of the 3D view (`scene_3d`) draws offscreen with OpenGL. Without OpenGL 3.3 (e.g. on a server without a GPU), it is skipped. With Mesa's software renderer, it runs under Xvfb, as in the CI:
  ```bash
  sudo apt install xvfb xauth libgl1-mesa-dri
  LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a ctest --output-on-failure
  ```

### Creating the source tarball

```bash
cmake --build build --target dist        # or: ./cmake-helper.sh dist
cmake --build build --target distcheck   # or: ./cmake-helper.sh distcheck
```

- `dist` creates `rcssmonitor-VERSION.tar.gz` in the top source directory (VERSION is the version that `rcssmonitor --version` tells, e.g. `20.0.0-beta.1`: `RCSSMONITOR_VERSION` in `CMakeLists.txt`), from the committed files of HEAD by `git archive`. The release tarballs are made by it. It extracts to `rcssmonitor-VERSION/`. Uncommitted changes are not included (a warning tells of them). It needs a git working tree.
- The files with `export-ignore` in `.gitattributes` (the CI settings, the plans and the documents for developers) are left out.
- `distcheck` creates the tarball, extracts it into `build/distcheck`, makes the sources read-only, then builds, tests, installs and uninstalls it there. It removes `build/distcheck` if all of them succeed (it is left for inspection if one fails).

## Building with autotools (Qt 5 only)

The autotools build is kept in the repository, but it is not the official build:

- It builds the monitor with Qt 5 only.
- It does not build the 3D view, nor the tests.

### Requirements

The Qt 5 packages and the libraries above, and the autotools and Qt 5's qmake:

```bash
sudo apt install build-essential zlib1g-dev libdeflate-dev qtbase5-dev libqt5websockets5-dev qtmultimedia5-dev libqt5multimedia5-plugins
sudo apt install autoconf automake libtool qt5-qmake
```

`libdeflate-dev`, `libqt5websockets5-dev` and `qtmultimedia5-dev` (with `libqt5multimedia5-plugins` at run time) are optional, as with CMake.

### Building

`./bootstrap` creates `configure` first, both in a clone of the repository and in a release tarball (made by the `dist` target of CMake, without `configure`):

```bash
./bootstrap
./configure
make
```

`src/rcssmonitor` is the monitor.

`configure` finds Qt by `qmake` (or `qmake-qt6`, `qmake-qt5`) in `PATH`. If it is not Qt 5's (e.g. Qt 6's qmake, or another Qt found first in `PATH`), give Qt 5's qmake by its full path:

```bash
./configure QMAKE=/usr/lib/qt5/bin/qmake
```

`qmake --version` tells which Qt it uses.

### Options of configure

| Option | Description |
|---|---|
| `--prefix=DIR` | The installation directory (`/usr/local` by default) |
| `--without-libdeflate` | Do not use libdeflate even if it is found |
| `--disable-websocket` | Do not build the WebSocket connection even if Qt WebSockets is found |
| `--disable-sound` | Do not build the referee's whistle even if Qt Multimedia is found |

`./configure --help` shows all the options, e.g. those of the installation directories.

### Installing and uninstalling

```bash
make install     # sudo make install, for /usr/local
make uninstall   # in the same directory, after configure with the same options
```

`make install` installs the same files as the CMake build: `PREFIX/bin/rcssmonitor`, `PREFIX/lib/librcssrcg.so*`, `PREFIX/include/rcss/rcg/*.h`, the desktop entry and its icons ([Installing and uninstalling](#installing-and-uninstalling)).
`make uninstall` removes them, but not the directories created by the installation.

The installed `rcssmonitor` finds `librcssrcg` in `PREFIX/lib` by itself: libtool puts that directory in it (RPATH), so `LD_LIBRARY_PATH` is not needed.
It is the full path of the directory (unlike `$ORIGIN/../lib` of CMake), so install again after moving the installation.

As with CMake, `PREFIX/bin` must be in `PATH` to start `rcssmonitor` by its name ([Installing and uninstalling](#installing-and-uninstalling)).

### The tarball of autotools

`make dist` creates a tarball for the autotools build: it does not have the files that only CMake builds (e.g. the 3D view and the tests).
The release tarballs are made by the `dist` target of CMake ([Creating the source tarball](#creating-the-source-tarball)), which has the whole sources.

## Notes

- The settings are kept in `~/.rcssmonitor.conf`. To try the monitor without changing them, start it with another `HOME`, e.g. `HOME=$(mktemp -d) build/rcssmonitor`.
- `rcssmonitor --help` shows the command line options.
- Started from the desktop (the application menu, the dash), rcssmonitor gets the environment of the desktop session, not that of `~/.bashrc`. For example, `QT_QPA_PLATFORM=xcb` exported in `~/.bashrc` does not apply there, and Qt 6 may run it on Wayland instead of X11 (XWayland), with another scale: on Wayland the desktop's scale (e.g. 100%), on X11 the DPI of X (`Xft.dpi`). To give a variable to the programs started from the desktop too, write it in a file in `~/.config/environment.d/` (e.g. `~/.config/environment.d/90-qt.conf` with the line `QT_QPA_PLATFORM=xcb`), and log in again.
- The icons are PNG files in the repository, made from SVG files by `icons/generate.sh` with Inkscape. Inkscape is needed only to change an icon, not to build ([docs/icons_en.md](icons_en.md)).
- Game logs from `https://` URLs (File > Open Game Log from URL) and the lists of game logs on GitHub (File > Browse Game Logs) use the TLS of Qt Network, which loads OpenSSL (libssl) at run time: with Qt 6, the plugin `tls/libqopensslbackend.so` (in `libqt6network6` on Ubuntu), and with Qt 5, Qt Network itself. Ubuntu has OpenSSL by default. Without TLS, rcssmonitor tells that `https://` is not available; `http://` URLs still work. The tests of the downloads and of the lists use a server on the local machine, and need no network.
- A build directory configured without an optional library (e.g. Qt WebSockets) and configured again after installing it may fail to link. Remove the build directory (`./cmake-helper.sh clean`) and build again.
- The referee's whistle ([README](../README.md#the-referees-whistle)) plays on the default audio output, through Qt Multimedia: `QAudioSink` with Qt 6, which needs nothing more at run time (checked with Qt 6.4 on Ubuntu 24.04), and `QAudioOutput` with Qt 5, which needs the audio plugins of Qt Multimedia (`libqt5multimedia5-plugins` on Ubuntu). Without an audio output, rcssmonitor prints `no audio output for the referee's whistle` once, and blows no whistle. While the whistle is on, the audio output is kept open, playing silence between the whistles: PipeWire puts an output that has been silent for 5 seconds to sleep, and a display's speakers on HDMI lose the start of a sound while they wake up. The tests need no audio device.
