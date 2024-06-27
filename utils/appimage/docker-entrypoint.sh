#!/bin/sh
set -e

cd /rcssmonitor 

bash /rcssmonitor/utils/appimage/build_code.sh
bash /rcssmonitor/utils/appimage/build_appimage.sh