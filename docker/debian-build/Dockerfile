#
# Build container image with:
#   podman build --pull -t svxlink-debian-build .
#
# Optional build arguments:
#             --build-arg=FROM=<base image>
#             --build-arg=SOUNDS_VER=<sound pack version>
#             --build-arg=SOUNDS_LANG=<sound pack language>
#             --build-arg=SOUNDS_VOICE=<sound pack voice>
#             --build-arg=SOUNDS_RATE=<sound pack sample rate>
#             --build-arg=SOUNDS_URL=<sound pack url>
#             --build-arg=GIT_URL_DEFAULT=<git repo>
#             --build-arg=GIT_SSL_NO_VERIFY_DEFAULT=<true/false>
#             --build-arg=GIT_BRANCH_DEFAULT=<git branch>
#             --build-arg=NUM_CORES_DEFAULT=<build with # cores>
#
# Run container with:
#   podman run -it --rm --hostname debian-build --userns=keep-id svxlink-debian-build
#
# For using sound inside the docker container add:
#             --privileged -v /dev/snd:/dev/snd
#
# To import your git config add:
#             -v ${HOME}/.gitconfig:/home/svxlink/.gitconfig:ro
#
# To use a specific git repository instead of the default one:
#             -e GIT_URL=username@your.repo.srv:/path/to/svxlink.git
#             -e GIT_URL=https://your.repo.srv/path/to/svxlink.git
#             -e GIT_SSL_NO_VERIFY=true
#
# To build another branch than master:
#             -e GIT_BRANCH=the_branch
#
# To use the local workingcopy rather then cloning the repo in the container:
#             -v $(pwd)/../..:/home/svxlink/svxlink:ro
#
# To use more than one CPU core when compiling:
#             -e NUM_CORES=8
#
# Build software with:
#   ./build-svxlink.sh
#
# Run software with:
#   svxlink

ARG FROM=debian:latest

FROM ${FROM}
MAINTAINER Tobias Blomberg <sm0svx@ssa.se>

# Install required packages and set up the svxlink user
RUN apt-get update && \
    apt-get -y install git cmake g++ make libsigc++-2.0-dev libgsm1-dev \
                       libpopt-dev tcl8.6-dev libgcrypt20-dev libspeex-dev \
                       libasound2-dev alsa-utils vorbis-tools qtbase5-dev \
                       qttools5-dev qttools5-dev-tools libopus-dev \
                       librtlsdr-dev libjsoncpp-dev libcurl4-openssl-dev \
                       libgpiod-dev libogg-dev ladspa-sdk curl sudo
#RUN apt-get -y install groff doxygen

ARG SOUNDS_VER="19.09.99.3"
ARG SOUNDS_LANG="en_US"
ARG SOUNDS_VOICE="heather"
ARG SOUNDS_RATE="16k"
ARG SOUNDS_URL="https://github.com/sm0svx/svxlink-sounds-${SOUNDS_LANG}-${SOUNDS_VOICE}/releases/download/${SOUNDS_VER}/svxlink-sounds-${SOUNDS_LANG}-${SOUNDS_VOICE}-${SOUNDS_RATE}-${SOUNDS_VER}.tar.bz2"

# Install svxlink audio files
RUN mkdir -p /usr/share/svxlink/sounds && \
    cd /usr/share/svxlink/sounds && \
    curl -LO ${SOUNDS_URL} && \
    tar xvaf svxlink-sounds-* && \
    ln -s ${SOUNDS_LANG}-${SOUNDS_VOICE}-${SOUNDS_RATE} ${SOUNDS_LANG} && \
    rm svxlink-sounds-*

# Set up password-less sudo for user svxlink
ADD sudoers-svxlink /etc/sudoers.d/svxlink
RUN chmod 0440 /etc/sudoers.d/svxlink

ARG GIT_URL_DEFAULT="https://github.com/sm0svx/svxlink.git"
ARG GIT_SSL_NO_VERIFY_DEFAULT="false"
ARG GIT_BRANCH_DEFAULT="master"
ARG NUM_CORES_DEFAULT="1"

ENV GIT_URL=${GIT_URL_DEFAULT} \
    GIT_SSL_NO_VERIFY=${GIT_SSL_NO_VERIFY_DEFAULT} \
    GIT_BRANCH=${GIT_BRANCH_DEFAULT} \
    NUM_CORES=${NUM_CORES_DEFAULT}

RUN useradd -s /bin/bash svxlink
ADD build-svxlink.sh /home/svxlink/
RUN chown -R svxlink:svxlink /home/svxlink

WORKDIR /home/svxlink
USER svxlink:svxlink
ENTRYPOINT ["/bin/bash"]
