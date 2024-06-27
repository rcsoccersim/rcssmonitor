#!/bin/bash
set -e

WORKDIR="$(pwd)"

BUILD_PWD="$WORKDIR/build"
MONITOR_BIN_PATH="$BUILD_PWD/monitor-bin"
mkdir -p $MONITOR_BIN_PATH
autoreconf -i
automake --add-missing
$WORKDIR/configure --prefix="$MONITOR_BIN_PATH"
make -j$(nproc)
make install
cp $WORKDIR/utils/appimage/rcssmonitor.desktop $BUILD_PWD
cp $WORKDIR/utils/appimage/rcssmonitor.png $BUILD_PWD