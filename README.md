# RoboCup Soccer Simulator Monitor
[![GitHub license](https://img.shields.io/github/license/rcsoccersim/rcssmonitor)](https://github.com/rcsoccersim/rcssmonitor/blob/master/COPYING)

![Logo](https://user-images.githubusercontent.com/1832537/49242985-f69a3c00-f3ea-11e8-97f5-9b0bfdfc4e1c.png)

**RoboCup Soccer Simulator Monitor** (rcssmonitor) is used to view the simulation as it takes place by connecting to the [RoboCup Soccer Simulator Server](https://github.com/rcsoccersim/rcssserver) (rcssserver) or to view the playback of a simulation by connecting to the [RoboCup Soccer Simulator LogPlayer](https://github.com/rcsoccersim/rcsslogplayer) (rcsslogplayer).

## ⚽️ Quick Start

Clone the repository. From its folder execute commands:

```
autoreconf -i
./configure
make
```

Alternatively you can download the tarball from [releases section](https://github.com/rcsoccersim/rcssmonitor/releases), extract it and run from its contents:

```
./configure
make
```

This will built the necessary binaries to get you up and running. `rcssmonitor/src/rcssmonitor` will be the binary for the monitor.

## 🛠 Configuring & Building

Before you can build RoboCup Soccer Simulator Monitor you will need to run the `configure` script located in the root of the distribution directory.

The default configuration will set up to install the monitor components in the following location:

`/usr/local/bin` for the executables

You may need administrator privileges to install the monitor into the default location. This locations can be modified by using configure's `--prefix=DIR` and related options.  See `configure --help` for more details.

The monitor has several features that can be enabled or disabled at configure time by using the `--enable-FEATURE[=ARG]` or `--disable-FEATURE` parameters to `configure`.  `--disable-FEATURE` is equivalent to `--enable-FEATURE=no` and `--enable-FEATURE` is equivlant to `--enable-FEATURE=yes`.  The only valid values for `ARG` are `yes` and `no`.

Once you have successfully configured the monitor, simply run `make` to build the sources.

## 📦 Installing

When you have completed building the monitor, it's components can be installed into their default locations or the locations specified during configuring by running `make install`. Depending on where you are installing the monitor, you may need special permissions.

The monitor can also be easily removed by entering the distribution directory and running `make uninstall`.  This will remove all the files that where installed, but not any directories that were created during the installation process.

## ⏯ Using LogPlayer

To start the monitor either type

```
./rcssmonitor
```

from the directory containing the executable or

```
rcssmonitor
```

If you invoke `rcssmonitor` with `--help` option, available options are displayed in your console.

## ✉ ️Making Contact

For bug reports, feature requests and latest updates, please goto
https://github.com/rcsoccersim/rcssmonitor/issues
or email bugs to sserver-bugs@lists.sourceforge.net

The RoboCup Soccer Server Maintainance Group
    sserver-admin@lists.sourceforge.net
