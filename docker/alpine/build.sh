#!/bin/sh

set -euo pipefail

IMG_NAME=${IMG_NAME:-"svxlink"}
IMG_TAG=${IMG_TAG:-"latest"}
#NUM_CORES=${NUM_CORES:-$(lscpu | awk '/^CPU\(s\):/ { print $2 }')}

LANGPACKS=(
  https://github.com/sm0svx/svxlink-sounds-en_US-heather/releases/download/24.02/svxlink-sounds-en_US-heather-16k-24.02.tar.bz2
  https://github.com/sm0svx/svxlink-sounds-sv_SE-elin/releases/download/19.09.99.4/svxlink-sounds-sv_SE-elin-16k-19.09.99.4.tar.bz2
  )

echo "--- Langpack setup"
for langpack in ${LANGPACKS[@]}; do
  filename=${langpack##*/}
  echo -n "---   $filename: "
  if [ -r "$filename" ]; then
    echo "Present"
  else
    echo "Downloading ${langpack}"
    curl -LO ${langpack}
  fi
done
echo

echo "--- Building image ${IMG_NAME}:dev"
podman build \
  ${NUM_CORES:+--build-arg NUM_CORES=$NUM_CORES} \
  ${RTLSDR_REPO:+--build-arg RTLSDR_REPO=$RTLSDR_REPO} \
  ${GIT_REPO:+--build-arg GIT_REPO=$GIT_REPO} \
  ${GIT_REF:+--build-arg GIT_REF=$GIT_REF} \
  ${GIT_SSL_NOVERIFY:+--build-arg GIT_SSL_NOVERIFY=$GIT_SSL_NOVERIFY} \
  ${BUILD_TYPE:+--build-arg BUILD_TYPE=$BUILD_TYPE} \
  -t ${IMG_NAME}:dev . -f Dockerfile.build

