# Build with:
#   podman build --pull -t svxlink-rocky-build .
#
# Run with:
#   podman run -it --rm --hostname rocky-build svxlink-rocky-build
#
# For using sound inside the container add:
#              --privileged --device /dev/snd:/dev/snd
#              --user $(id -u):$(id -g) --userns keep-id --group-add keep-groups
#
# To import your git config add (mileage may vary):
#              -v ${HOME}/.gitconfig:/home/svxlink/.gitconfig:ro
#
# To use a specific git repositoty instead of the default one:
#              -e GIT_URL=username@your.repo:/path/to/svxlink.git
#
# To build another branch than master:
#              -e GIT_BRANCH=the_branch
#
# To use more than one CPU core when compiling:
#              -e NUM_CORES=8
#

ARG FROM=rockylinux:9
FROM ${FROM} AS downloader

RUN dnf -y install bzip2

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

ARG FROM=rockylinux:9
FROM ${FROM}
MAINTAINER Tobias Blomberg <sm0svx@svxlink.org>

# Install audio files
COPY --from=downloader /sounds /usr/share/svxlink/sounds

# Install required packages and set up the svxlink user
RUN dnf -y install epel-release && \
    /usr/bin/crb enable && \
    dnf -y install git sudo cmake make gcc gcc-c++ libsigc++20-devel \
           alsa-lib-devel speex-devel opus-devel qt5-devel rtl-sdr-devel \
           popt-devel libgcrypt-devel tcl-devel gsm-devel libcurl-devel \
           jsoncpp-devel openssl-devel ladspa-devel libusb1-devel \
           doxygen alsa-utils wget tar bzip2 vim-enhanced && \
    dnf clean all

# Set up password less sudo for user svxlink
ADD sudoers-svxlink /etc/sudoers.d/svxlink
RUN chmod 0440 /etc/sudoers.d/svxlink

ENV GIT_URL=https://github.com/sm0svx/svxlink.git \
    GIT_BRANCH=master \
    NUM_CORES=1

# Add a user and a build script
RUN useradd svxlink
ADD build-svxlink.sh /home/svxlink/
RUN chown -R svxlink:svxlink /home/svxlink

# Use an entrypoint script
ADD entrypoint.sh /
ENTRYPOINT ["/entrypoint.sh"]

