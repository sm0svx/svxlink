#!/bin/bash

set -euo pipefail

podman run -it --rm --hostname debian-build --userns=keep-id \
  --privileged -v /dev/snd:/dev/snd \
  -v ${HOME}/.gitconfig:/home/svxlink/.gitconfig:ro \
  -e NUM_CORES=8 \
  svxlink-debian-build

