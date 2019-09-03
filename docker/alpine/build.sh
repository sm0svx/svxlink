#!/bin/sh

set -e

IMG_NAME=${IMG_NAME:-"svxlink"}
IMG_TAG=${IMG_TAG:-"latest"}
NUM_CORES=${NUM_CORES:-$(lscpu | awk '/^CPU\(s\):/ { print $2 }')}

LANGPACKS=(
  https://github.com/sm0svx/svxlink-sounds-en_US-heather/releases/download/14.08/svxlink-sounds-en_US-heather-16k-13.12.tar.bz2
  https://github.com/sm0svx/svxlink-sounds-en_US-heather/releases/download/18.03.1/svxlink-sounds-en_US-heather-16k-18.03.1.tar.bz2
  https://github.com/sm0svx/svxlink-sounds-sv_SE-elin/releases/download/next/svxlink-sounds-sv_SE-elin-16k-next.tar.bz2
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

echo "--- Building builder image ${IMG_NAME}:build"
docker build \
    --build-arg NUM_CORES=${NUM_CORES} \
    ${RTLSDR_REPO:+--build-arg RTLSDR_REPO=$RTLSDR_REPO} \
    ${GIT_REPO:+--build-arg GIT_REPO=$GIT_REPO} \
    ${GIT_REF:+--build-arg GIT_REF=$GIT_REF} \
    ${GIT_SSL_NOVERIFY:+--build-arg GIT_SSL_NOVERIFY=$GIT_SSL_NOVERIFY} \
    -t ${IMG_NAME}:build . -f Dockerfile.build

echo
echo "--- Extracting build artifacts from builder image"
docker container create --name svxlink-extract ${IMG_NAME}:build
docker container cp svxlink-extract:/tmp/svxlink.tar.gz ./
docker container rm -f svxlink-extract

echo
echo "--- Building runtime image ${IMG_NAME}:${IMG_TAG}"
docker build -t ${IMG_NAME}:${IMG_TAG} .
rm svxlink.tar.gz
