#!/bin/sh

SF_DL_URL="http://downloads.sourceforge.net/svxlink"
DOWNLOADS="libasync-0.15.0-1.fc8.i386.rpm echolib-0.12.1-1.fc8.i386.rpm \
      	   qtel-0.10.1-1.fc8.i386.rpm svxlink-server-0.9.0-1.fc8.i386.rpm"


fedora_release=$(rpm -q --qf="%{version}" fedora-release)
if [ "${fedora_release}" != "8" ]; then
  echo "*** ERROR: This is not a Fedora 8 system"
  exit 1
fi


if [ $(id -u) -ne 0 ]; then
  echo "*** ERROR: You must be root to run this script"
  exit 1
fi


if ! rpm -q wget > /dev/null; then
  echo "--- Installing wget..."
  if ! yum install wget; then
    echo "*** ERROR: Failed to install wget"
    exit 1
  fi
fi


tmp=$(mktemp -td svxlink-install-XXXXXX)
echo "--- Using temporary directory $tmp..."
cd $tmp

echo "--- Downloading SvxLink RPM:s..."
for file in $DOWNLOADS; do
  if ! wget $SF_DL_URL/$file; then
    echo "*** ERROR: Download of $file failed"
    exit 1
  fi
done


echo "--- Checking for SvxLink public key..."
if ! rpm -q gpg-pubkey-f4410fd4-46210f90 > /dev/null; then
  echo "--- Importing SvxLink public key..."
  if ! rpm --import http://svxlink.sourceforge.net/RPM-GPG-KEY-sm0svx; then
    echo "*** ERROR: Import of SvxLink public key failed"
    exit 1
  fi
fi


if rpm -q libasync > /dev/null; then
  echo "--- Updating SvxLink RPM:s..."
  if ! yum localupdate *.rpm; then
    echo "*** ERROR: Update of SvxLink failed!"
    exit 1
  fi
else
  echo "--- Installing SvxLink RPM:s and all their dependencies..."
  if ! yum localinstall *.rpm; then
    echo "*** ERROR: Installation of SvxLink failed!"
    exit 1
  fi
fi


echo "--- Done"
