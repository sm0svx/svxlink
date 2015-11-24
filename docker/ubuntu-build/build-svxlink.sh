#!/bin/bash -xe

# Make sure that we are in the home directory
cd

# Clone or update the repo
if [[ ! -d svxlink ]]; then
  git clone $GIT_URL svxlink
  cd svxlink
else
  cd svxlink
  git fetch
  git checkout master
  git reset --hard master
fi

# Checkout the wanted branch
if [ -n "$GIT_BRANCH" ]; then
  git checkout $GIT_BRANCH
fi

# Find out how many cores we've got
num_cores=${NUM_CORES:-1}

# Create a build directory and build svxlink
cd
[[ -d build ]] && rm -rf build
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_SYSCONFDIR=/etc \
      -DCMAKE_INSTALL_LOCALSTATEDIR=/var \
      -DCMAKE_BUILD_TYPE=Release ../svxlink/src
make -j$num_cores
sudo make install
sudo ldconfig
