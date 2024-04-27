#!/bin/bash
set -e


SCRIPT_DIR="$(pwd)/build"
M_BUILD_PWD="$SCRIPT_DIR/monitor-bin"
BIN_PATH="$M_BUILD_PWD/bin/rcssmonitor"
APP_IMAGE_DIR="$SCRIPT_DIR/appimage"

cd $SCRIPT_DIR

wget -c "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" -O linuxdeploy-x86_64.AppImage
wget -c "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" -O linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-plugin-qt-x86_64.AppImage


rm -rf $APP_IMAGE_DIR || true
mkdir -p $APP_IMAGE_DIR || true



# ------ LIBRARIES
LIBZ_PATH=$(ldd $BIN_PATH | grep libz.so | awk '{ print $3 }')
LIBRCSSRCG_PATH=$(ldd $BIN_PATH | grep librcssrcg.so | awk '{ print $3 }')

./linuxdeploy-x86_64.AppImage --appdir $APP_IMAGE_DIR \
                --plugin qt \
                -l $LIBZ_PATH \
                -l $LIBRCSSRCG_PATH \
                --executable $M_BUILD_PWD/bin/rcssmonitor \
                --desktop-file $SCRIPT_DIR/rcssmonitor.desktop \
                --icon-file $SCRIPT_DIR/rcssmonitor.png \
                --output appimage 

