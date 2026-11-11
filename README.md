# RoboCup Soccer Simulator Monitor
[![GitHub license](https://img.shields.io/github/license/rcsoccersim/rcssmonitor)](https://github.com/rcsoccersim/rcssmonitor/blob/master/COPYING)

![Logo](https://user-images.githubusercontent.com/1832537/49242985-f69a3c00-f3ea-11e8-97f5-9b0bfdfc4e1c.png)

**RoboCup Soccer Simulator Monitor** (rcssmonitor) is used to view the simulation as it takes place by connecting to the [RoboCup Soccer Simulator Server](https://github.com/rcsoccersim/rcssserver) (rcssserver) or to view the playback of a simulation by loading game log files.

## :soccer: Quick Start

rcssmonitor is written in C++17 and depends on [Qt](https://www.qt.io/) 6 (6.2 or later) or Qt 5 (5.15). It is built with [CMake](https://cmake.org/) (3.16 or later); autotools can build it with Qt 5 only.

First, install the tools and libraries. On Ubuntu 22.04, 24.04 or 26.04, with Qt 6:
```bash
sudo apt install build-essential cmake qt6-base-dev zlib1g-dev libdeflate-dev
```
or with Qt 5:
```bash
sudo apt install build-essential cmake qtbase5-dev zlib1g-dev libdeflate-dev
```
zlib (`zlib1g-dev`) is needed to load gzip-compressed game logs (`.rcg.gz`). libdeflate (`libdeflate-dev`) is optional: if it is found, gzip game logs are loaded faster.

Then, clone the repository, or download the tarball from the [releases section](https://github.com/rcsoccersim/rcssmonitor/releases) and extract it. From its directory, execute:

```bash
cmake -S . -B build
cmake --build build
```

`build/rcssmonitor` will be the binary for the monitor. CMake uses Qt 6 if it is found, and Qt 5 otherwise; to use Qt 5 when both are installed, pass `-DQT_VERSION_MAJOR=5` to the first command.

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

Once you have successfully configured the monitor, simply run `make` to build the sources.

With CMake, the options are given to the first command, e.g. `cmake -S . -B build -DCMAKE_INSTALL_PREFIX=YOUR_INSTALLATION_DIR`:

- `CMAKE_INSTALL_PREFIX`: the installation directory (`/usr/local` by default)
- `QT_VERSION_MAJOR`: `5` or `6`, the Qt to use (Qt 6 if it is found, by default)
- `ENABLE_LIBDEFLATE`: `OFF` not to use libdeflate even if it is found (`ON` by default). With autotools, pass `--without-libdeflate` to `configure`.

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
in the case of CMake, or
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

## :incoming_envelope: Contributing

For bug reports, feature requests and latest updates, please goto
https://github.com/rcsoccersim/rcssmonitor and open an issue or a pull request.

> The RoboCup Soccer Server Maintainance Group
