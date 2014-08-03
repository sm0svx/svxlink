#!/bin/sh

RSYNC_OPTS="-av --delete"

rsync ${RSYNC_OPTS} src/build/doc/async/html/ doxygen/async/
rsync ${RSYNC_OPTS} src/build/doc/echolib/html/ doxygen/echolib/
rsync ${RSYNC_OPTS} src/build/doc/man/*.5.html doc/man/man5/
rsync ${RSYNC_OPTS} src/build/doc/man/*.1.html doc/man/man1/

