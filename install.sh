#!/usr/bin/env bash
# Run this with sudo
apt-get install  build-essential tcl-dev libpopt-dev libgcrypt20-dev libasound2-dev libspeex-dev libqt4-dev librtlsdr-dev opus-tools libsigc++-2.0-dev doxygen cmake make git libgsm1-dev groff
useradd svxlink
groupadd daemon
cd ./src
mkdir build
cd build
cmake ..
make
make doc
make install # or checkinstall
