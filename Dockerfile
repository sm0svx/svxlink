FROM ubuntu:18.04

RUN apt update && \
    export DEBIAN_FRONTEND=noninteractive && \
    apt install -y \
    build-essential \
    make \
    cmake \
    groff \
    curl \
    sigc++ \
    libssl-dev \
    libasound2-dev \
    libogg-dev \
    libqt4-dev \
    libpopt-dev \
    libgcrypt-dev \ 
    libjsoncpp-dev \
    tcl8.6-dev \
    librtlsdr-dev \
    libgsm1-dev \
    libcurl4-openssl-dev 


