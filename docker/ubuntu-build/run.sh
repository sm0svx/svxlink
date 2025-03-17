#!/bin/bash

set -euo pipefail

podman run -it --rm --hostname ubuntu-build \
  --privileged --device /dev/snd --userns=keep-id \
  --group-add=keep-groups --user $(id -u):$(id -g) \
  -v ${HOME}/.gitconfig:/home/svxlink/.gitconfig:ro \
  -e NUM_CORES=8 \
  svxlink-ubuntu-build
