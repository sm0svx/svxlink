#!/bin/bash

# Force new UID if specified
#if [ -n "$SVXLINK_UID" ]; then
#  usermod -u $SVXLINK_UID svxlink
#fi

# Force new GID if specified
#if [ -n "$SVXLINK_GID" ]; then
#  usermod -g $SVXLINK_GID svxlink
#  find /home/svxlink ! -gid $SVXLINK_GID -exec chgrp $SVXLINK_GID {} \;
#fi

# Create the hostaudio group if GID is specified
if [ -n "$HOSTAUDIO_GID" ]; then
  sudo groupadd -g $HOSTAUDIO_GID hostaudio
  sudo usermod -G $HOSTAUDIO_GID svxlink
fi

# Set up the sudo command line
SUDO_CMD="sudo -u svxlink "
SUDO_CMD+="PATH=$PATH:/usr/lib64/qt4/bin "
SUDO_CMD+="GIT_URL=$GIT_URL "
SUDO_CMD+="GIT_BRANCH=$GIT_BRANCH "
SUDO_CMD+="NUM_CORES=$NUM_CORES "

# If an argument is specified, run it as a command or else just start a shell
if [ $# -gt 0 ]; then
  exec $SUDO_CMD "$@"
else
  exec $SUDO_CMD -i
fi
