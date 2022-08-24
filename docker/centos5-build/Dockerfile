# Build with:
#   docker build --pull -t svxlink-centos5-build .
#
# Run with:
#   docker run -it --rm --hostname centos5-build \
#              svxlink-centos5-build
#
# For using sound inside the docker container add:
#              --privileged -v /dev/snd:/dev/snd \
#              -e HOSTAUDIO_GID=$(stat -c "%g" /dev/snd/timer) \
#
# To import your git config add (mileage may vary):
#              -v ${HOME}/.gitconfig:/home/svxlink/.gitconfig:ro \
#
# To use a specific git repository instead of the default one:
#              -e GIT_URL=username@your.repo:/path/to/svxlink.git \
#
# To build another branch than master:
#              -e GIT_BRANCH=the_branch
#
# To use more than one CPU core when compiling:
#              -e NUM_CORES=8
#

FROM centos:5
MAINTAINER Tobias Blomberg <sm0svx@ssa.se>

# Install static Yum config since CentOS 5 is EOL
ADD *.repo /etc/yum.repos.d/

# Install required packages and set up the svxlink user
RUN yum -y install epel-release && \
    yum -y install git sudo cmake28 make gcc gcc-c++ libsigc++20-devel \
                   alsa-lib-devel speex-devel opus-devel qt4-devel \
                   libgcrypt-devel tcl-devel gsm-devel doxygen alsa-utils \
                   tar bzip2 \
                   vim-enhanced && \
    yum clean all

# && \
#    groupadd -g 92 hostaudio && useradd -G hostaudio svxlink
# rpm-build

# Install svxlink audio files
RUN mkdir -p /usr/share/svxlink/sounds && \
    cd /usr/share/svxlink/sounds && \
    curl -L -o svxlink-sounds-en_US-heather-16k-13.12.tar.bz2 https://github.com/sm0svx/svxlink-sounds-en_US-heather/releases/download/14.08/svxlink-sounds-en_US-heather-16k-13.12.tar.bz2 && \
    tar xvjf svxlink-sounds-* && \
    ln -s en_US-heather-16k en_US && \
    rm svxlink-sounds-*

# Set up password less sudo for user svxlink
ADD sudoers /etc/
RUN chmod 0440 /etc/sudoers

ENV GIT_URL=https://github.com/sm0svx/svxlink.git \
    GIT_BRANCH=master \
    NUM_CORES=1

RUN useradd svxlink
ADD build-svxlink.sh /home/svxlink/
RUN chown -R svxlink.svxlink /home/svxlink

#USER svxlink
#WORKDIR /home/svxlink
#CMD ["/bin/bash"]
ADD entrypoint.sh /
ENTRYPOINT ["/entrypoint.sh"]
#RUN git clone https://github.com/sm0svx/svxlink.git && \
#RUN git clone $GIT_URL svxlink && \
#    mkdir build && \
#    cd build && \
#    cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_SYSCONFDIR=/etc \
#          -DCMAKE_INSTALL_LOCALSTATEDIR=/var ../svxlink/src && \
#    make -j9 && \
#    sudo make install && \
#    sudo ldconfig
    #cd && \
    #rm -rf build svxlink


