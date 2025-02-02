#!/bin/bash

set -euo pipefail

# Find RTL USB stick device group if present
#RTLUSB=$(lsusb | awk '/RTL2838/ { print $2 "/" substr($4,1,3) }')
#if [[ -n "$RTLUSB" ]]; then
#  RTLSDR_GID=$(stat -c "%g" /dev/bus/usb/$RTLUSB)
#  echo "--- Setting RTLSDR_GID=$RTLSDR_GID"
#fi

VOL_BASE=alpine-svxlink-
VOL_HOME=${VOL_BASE}home
VOL_SPOOL=${VOL_BASE}spool
VOL_LIB=${VOL_BASE}lib

for vol in HOME SPOOL LIB; do
  volvar="VOL_${vol}"
  volname=${!volvar}
  if ! podman volume exists ${volname}; then
    podman volume create \
      --opt="o=uid=$(id -u)" \
      --opt="o=gid=$(id -g)" \
      ${volname}
  fi
done

podman run -it --rm \
  --name svxlink \
  --hostname svxlink \
  --device /dev/snd:/dev/snd:rw \
  --privileged \
  ${RTLSDR_GID:+--device /dev/bus/usb -e RTLSDR_GID=$RTLSDR_GID} \
  -v ${VOL_SPOOL}:/var/spool/svxlink \
  -v ${VOL_LIB}:/var/lib/svxlink \
  -v ${VOL_HOME}:/home/svxlink \
  --user $(id -u):$(id -g) \
  --userns keep-id \
  --group-add keep-groups \
  --workdir /home/svxlink \
  svxlink:${IMG_TAG:-dev} "$@"
