# RCSSSERVER Soccer Simulator 2d
# Dockerfile Author: Heitor Rapela Medeiros (https://github.com/heitorrapela)
# This file need to be inside this repository, to copy all lib dependencies: https://github.com/rcsoccersim/rcssmonitor
# Build command: docker build . -t "simulator2d_rcssmonitor:core"
# Run command: docker run -it --rm -e DISPLAY=${DISPLAY} -e QT_X11_NO_MITSHM=1 -v /tmp/.X11-unix:/tmp/.X11-unix --name=monitor --net mynet simulator2d_rcssmonitor:core

# Pull base image.
FROM ubuntu:12.04

# Install.
RUN \
  sed -i 's/# \(.*multiverse$\)/\1/g' /etc/apt/sources.list && \
  apt-get update && \
  apt-get -y upgrade && \
  apt-get install -y build-essential && \
  apt-get install -y software-properties-common && \
  apt-get install -y byobu curl git htop man unzip vim wget python-pip libqt4-dev libxt-dev iputils-ping && \
  apt-get install -y libfreetype6-dev libfontconfig1-dev python-gobject-dev libpng-dev gedit && \
  apt-get install -y flex gcc make automake libtool libboost-all-dev libboost-dbg && \
  apt-get install -y libaudio-dev libpng-dev libxi-dev libglib2.0-dev libfontconfig-dev libxrender-dev && \
  
  rm -rf /var/lib/apt/lists/*

WORKDIR /root/
RUN wget "http://ftp.gnu.org/gnu/bison/bison-2.5.1.tar.xz" -O bison-2.5.1.tar.xz
RUN tar -xvf bison-2.5.1.tar.xz
WORKDIR /root/bison-2.5.1
RUN ./configure
RUN make && make install

WORKDIR /root/
RUN mkdir /rcssmonitor
COPY . /root/rcssmonitor

WORKDIR /root/rcssmonitor/
RUN ./bootstrap
RUN ./configure 
RUN make
RUN make install

RUN apt-get update && apt-get install -qqy x11-apps
ENV DISPLAY :0
CMD xeyes

# Set environment variables.
ENV HOME /root

# Define working directory.
WORKDIR /root

# Define default command.
CMD ["bash"]