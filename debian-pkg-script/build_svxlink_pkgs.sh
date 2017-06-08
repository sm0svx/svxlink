#!/bin/bash

#Set Pkg Build Version
GH_NAME=kb3vgw
BRANCH=svxlink-reflector
PKG_NAME=svxlink
PKG_VER=1.5.99
BUILD=2
RELEASE=testing #testing/stable/devel
GIT_SRC=https://github.com/"$GH_NAME"/"$PKG_NAME".git
REPO=/home/repo/"$PKG_NAME"/"$RELEASE"/debian
WRK_DIR=/usr/src/"$PKG_NAME"-build
WRK_SRC_DIR=/usr/src/"$PKG_NAME"-build/"$PKG_NAME"-"$PKG_VER"

#remove old working dir
rm -rf $WRK_DIR

#get pkg system scripts
git clone -b $BRANCH $GIT_SRC "$WRK_SRC_DIR"/

cd $WRK_SRC_DIR || exit
#set version in the changelog files for core
dch -v "$PKG_VER"-"$BUILD"

#build core pkg
cd $WRK_SRC_DIR || exit
time dpkg-buildpackage -rfakeroot -i -b -j5 -us -uc

cd $WRK_DIR || exit

mkdir debs-$PKG_NAME-$PKG_VER || exit

mv *.deb debs-$PKG_NAME-$PKG_VER
mv *.changes debs-$PKG_NAME-$PKG_VER
mv *.xz debs-$PKG_NAME-$PKG_VER
mv *.dsc debs-$PKG_NAME-$PKG_VER
mv *.gz debs-$PKG_NAME-$PKG_VER

#cp -rp "$WRK_DIR"/debs-$PKG_NAME-$PKG_VER/* "$REPO"/incoming || exit

#cd "$REPO" || exit
#./import-new-pkgs.sh || exit
