#!/bin/bash

set -euo pipefail

# Create configuration and spool area
[[ -d conf ]] || mkdir --mode 2775 conf
[[ -d spool ]] || mkdir --mode 2775 spool

# Find RTL USB stick device group if present
RTLUSB=$(lsusb | awk '/RTL2838/ { print $2 "/" substr($4,1,3) }')
[[ -n "$RTLUSB" ]] && RTLSDR_GID=$(stat -c "%g" /dev/bus/usb/$RTLUSB)

# Find pasuspender for suspending the pulse audio server
PASUSPENDER=$(which pasuspender 2>/dev/null)

if [[ -z "$@" ]]; then
  DOCKER_ARGS="-it --rm"
else
  DOCKER_ARGS="-d"
fi

exec ${PASUSPENDER:+$PASUSPENDER --} \
  sudo docker run ${DOCKER_ARGS} --hostname svxlink --name svxlink \
    --device /dev/snd -e HOSTAUDIO_GID=$(stat -c "%g" /dev/snd/timer) \
    ${RTLSDR_GID:+--device /dev/bus/usb -e RTLSDR_GID=$RTLSDR_GID} \
    -v $(pwd)/conf:/etc/svxlink:z \
    -v $(pwd)/spool:/var/spool/svxlink:z \
    svxlink:latest "$@"
