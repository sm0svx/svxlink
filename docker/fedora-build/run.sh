#!/bin/bash

set -euo pipefail

podman run -it --rm --hostname fedora-build --userns=keep-id \
  --privileged -v /dev/snd:/dev/snd \
  -v ${HOME}/.gitconfig:/home/svxlink/.gitconfig:ro \
  -e NUM_CORES=8 \
  svxlink-fedora-build

