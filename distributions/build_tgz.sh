#!/bin/sh

SVNROOT=https://svxlink.svn.sourceforge.net/svnroot/svxlink
DATE=$(date +%y%m%d)
NAME=svxlink-${DATE}
ARCH=/tmp/${NAME}.tar.gz

rm -rf /tmp/${NAME}
cd /tmp
svn export ${SVNROOT}/trunk/src ${NAME}
tar cvzf ${ARCH} ${NAME}
ret=$?

rm -rf /tmp/${NAME}

[ $ret -eq 0 ] && echo -e "\n--- Wrote $ARCH"

