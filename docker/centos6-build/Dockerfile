FROM centos:6
MAINTAINER Tobias Blomberg <sm0svx@ssa.se>

# Install required packages and set up the svxlink user
RUN yum -y install epel-release && \
    yum -y install git sudo cmake gcc gcc-c++ libsigc++20-devel \
                   alsa-lib-devel speex-devel opus-devel qt-devel \
                   popt-devel libgcrypt-devel tcl-devel gsm-devel \
                   doxygen alsa-utils wget tar bzip2 \
                   vim-enhanced && \
    yum clean all

# && \
#    groupadd -g 92 hostaudio && useradd -G hostaudio svxlink
# rpm-build

# Set up password less sudo for user svxlink
#ADD svxlink-sudoers.conf /etc/sudoers.d/svxlink
ADD sudoers /etc/

# Install svxlink audio files
RUN mkdir -p /usr/share/svxlink/sounds && \
    cd /usr/share/svxlink/sounds && \
    wget https://github.com/sm0svx/svxlink-sounds-en_US-heather/releases/download/14.08/svxlink-sounds-en_US-heather-16k-13.12.tar.bz2 && \
    tar xvaf svxlink-sounds-* && \
    ln -s en_US-heather-16k en_US && \
    rm svxlink-sounds-*

ENV GIT_URL https://github.com/sm0svx/svxlink.git \
    GIT_BRANCH master \
    NUM_CORES 1

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


