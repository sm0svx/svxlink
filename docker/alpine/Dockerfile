FROM alpine
MAINTAINER Tobias Blomberg <sm0svx@svxlink.org>

ADD svxlink.tar.gz /
ADD svxlink-sounds-en_US-heather-16k-18.03.1.tar.bz2 /usr/share/svxlink/sounds/
ADD svxlink-sounds-sv_SE-elin-16k-next.tar.bz2 /usr/share/svxlink/sounds/

RUN apk update && \
    apk add libsigc++ alsa-lib opus speex popt libgcrypt tcl curl gsm libusb \
            vorbis-tools jsoncpp mutt && \
    adduser -D -u 10000 svxlink && \
    chown -R svxlink:svxlink /var/spool/svxlink/* /etc/svxlink && \
    ln -s en_US-heather-16k /usr/share/svxlink/sounds/en_US && \
    ln -s sv_SE-elin-16k /usr/share/svxlink/sounds/sv_SE && \
    cp -a /etc/svxlink /usr/share/doc/svxlink/skel

ADD entrypoint /

WORKDIR /home/svxlink
ENTRYPOINT ["/entrypoint"]
