#!/bin/bash -x

set -euo pipefail

GIT_BRANCH=${GIT_BRANCH:-master}

# Make sure that we are in the home directory
cd

# Clone or update the repo
if [[ ! -d svxlink ]]; then
  git clone --branch=$GIT_BRANCH $GIT_URL svxlink
  cd svxlink
else
  cd svxlink
  if [[ -w . ]]; then
    git fetch
    git checkout $GIT_BRANCH
    git reset --hard origin/$GIT_BRANCH
  fi
fi

# How many cores to use during the build
num_cores=${NUM_CORES:-1}

# Create a build directory and build svxlink
cd
[[ -d build ]] && rm -rf build
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr \
      -DCMAKE_INSTALL_SYSCONFDIR=/etc \
      -DCMAKE_INSTALL_LOCALSTATEDIR=/var \
      -DCPACK_GENERATOR=DEB \
      -DCMAKE_BUILD_TYPE=Release \
      ../svxlink/src
make -j$num_cores
rm -f *.deb
make package
sudo dpkg -i *.deb
#sudo make install
#sudo ldconfig
