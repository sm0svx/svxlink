#
# Build container image with:
#   podman build --pull -t svxlink-ubuntu-build .
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
#   podman run -it --rm --hostname ubuntu-build svxlink-ubuntu-build
#
# For using sound inside the docker container add:
#             --userns=keep-id --group-add=keep-groups --user=$(id -u):$(id -g)
#             --privileged --device /dev/snd
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

ARG FROM=ubuntu:latest
FROM ${FROM} AS downloader

# Install packages needed to unpack the sound pack archives
RUN apt -y update && \
    apt -y install bzip2 bash

# Add audio files
ADD https://github.com/sm0svx/svxlink-sounds-en_US-heather/releases/download/24.02/svxlink-sounds-en_US-heather-16k-24.02.tar.bz2 /tmp/
ADD https://github.com/sm0svx/svxlink-sounds-sv_SE-elin/releases/download/19.09.99.4/svxlink-sounds-sv_SE-elin-16k-19.09.99.4.tar.bz2 /tmp/
RUN mkdir -p /sounds && \
    cd /sounds && \
    for tar in /tmp/*.tar.bz2; do \
      tar xvaf "${tar}"; \
    done && \
    for lp in *; do \
      ln -s "${lp}" "${lp%%-*}"; \
    done


###############################################################################

ARG FROM=ubuntu:latest
FROM ${FROM}
MAINTAINER Tobias Blomberg <sm0svx@svxlink.org>

# Install audio files
COPY --from=downloader /sounds /usr/share/svxlink/sounds

# Install required packages and set up the svxlink user
RUN apt-get update && \
    export DEBIAN_FRONTEND=noninteractive && \
    apt-get -y install git cmake g++ make doxygen groff curl sudo \
                       libsigc++-2.0-dev libgsm1-dev libpopt-dev tcl8.6-dev \
                       libgcrypt20-dev libspeex-dev libasound2-dev alsa-utils \
                       vorbis-tools qtbase5-dev qttools5-dev \
                       qttools5-dev-tools libopus-dev librtlsdr-dev \
                       libjsoncpp-dev libcurl4-openssl-dev libgpiod-dev \
                       libogg-dev ladspa-sdk libssl-dev


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
