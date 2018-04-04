# Introduction

The RoboCup Soccer Simulator Monitor (**rcssmonitor**) is used to view the simulation as it takes place by connecting to the **rcssserver** or to view the playback of a simulation by connecting to the **rcsslogplayer**.

# Quick Start

From the **rcssmonitor** directory execute:

	./configure
	make

This will built the necessary binaries to get you up and running. 
**rcssmonitor/src/rcssmonitor** is the binary for the monitor.

# Configuring

Before you can build **The RoboCup Soccer Simulator Monitor** you will need to run the **configure** script located in the root of the distribution directory.

The default configuration will set up to install the monitor components in the following location:

	/usr/local/bin  #for the executables

You may need administrator privileges to install the **rcssmonitor** into the default location.  This locations can be modified by using configure's `--prefix=DIR` and related options.  See `configure --help` for more details.

The **rcssmonitor** has several features that can be enabled or disabled at configure time by using the `--enable-FEATURE[=ARG]` or `--disable-FEATURE` parameters to `configure` .
`--disable-FEATURE` is equivalent to `--enable-FEATURE=no` and `--enable-FEATURE` is equivalent to `--enable-FEATURE=yes`.  The only valid values for `ARG` are `yes` and `no`.

# Building

Once you have successfully configured the **rcssmonitor**, simply run `make` to build the sources.

# Installing

When you have completed building the **rcssmonitor**, it's components can be installed into their default locations or the locations specified during configuring by running `make install`.  Depending on where you are installing the **rcssmonitor**, you may need special permissions.

# Uninstalling

The monitor can also be easily removed by entering the distribution
directory and running `make uninstall`.  This will remove all the files that
where installed, but not any directories that were created during the
installation process.

# Using the Monitor

To start the **rcssmonitor** either type

`./rcssmonitor`

from the directory containing the executable or

`rcssmonitor`

If you invoke `rcssmonitor` with `--help` option, available options are
displayed in your console.

## Run  Rcssmonitor w/ Docker Container (Ubuntu 12.04)

First, create a network to connect all containers (We use this version of Ubuntu, because its default for simulation 2d competition):

```
docker network create mynet
```
  
Inside this **GitHub** repository, build the **Dockerfile** in terminal:

	docker build . -t "rcssmonitor"

You need to make the Display exportable   
 
	xhost +local:root

Run the **rcssmonitor** client:

	docker run -it --rm -e DISPLAY=${DISPLAY} -e QT_X11_NO_MITSHM=1 -v /tmp/.X11-unix:/tmp/.X11-unix  --net mynet --name monitor rcssmonitor

Inside **docker image**, type to run client:
	
	rcssmonitor

You need to run the [server docker](https://github.com/rcsoccersim/rcssserver) first, and after [rcsslogplayer](https://github.com/rcsoccersim/rcsslogplayer) and **create each team container** to run separately.

To connect to the server docker, you will need to put **server** in connect to ip.

# Making Contact

For bug reports, feature requests and latest updates, please go to:

http://sourceforge.net/projects/sserver/

Or email bugs to:

sserver-bugs@lists.sourceforge.net

The RoboCup Soccer Server Maintainance Group:

sserver-admin@lists.sourceforge.net
