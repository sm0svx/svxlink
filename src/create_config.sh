#!/bin/sh

info()
{
  echo -en $1
}

output()
{
  echo $1 >> $OUTPUT_FILE
}

if [ $# -lt 1 ]; then
  echo "Usage: $0 <output file>"
  exit 1
fi

OUTPUT_FILE=$1
rm -f $OUTPUT_FILE

output "MAKEFILE_CONFIG_INCLUDED = 1"

# Check if the "-h" switch may be used with the chgrp command
info "--- Checking if chgrp understand the -h switch..."
ln -s . chgrp_test-$$.tmp
if chgrp -h `id -gn` chgrp_test-$$.tmp 2> /dev/null; then
  info "yes\n"
  output "CHGRP_H = chgrp -h"
else
  info "no\n"
  output "CHGRP_H = chgrp"
fi
rm -f chgrp_test-$$.tmp

# Check if the "-h" switch may be used with the chown command
info "--- Checking if chown understand the -h switch..."
ln -s . chown_test-$$.tmp
if chown -h `id -un` chown_test-$$.tmp 2> /dev/null; then
  info "yes\n"
  output "CHOWN_H = chown -h"
else
  info "no\n"
  output "CHOWN_H = chown"
fi
rm -f chown_test-$$.tmp

# Check for KDE
info "--- Checking for KDE..."
if [ "${KDEDIR}" != "" ]; then
  if [ -r "${KDEDIR}/include/kde/kdeversion.h" ]; then
    KDEINC=${KDEDIR}/include/kde
    KDE_VERSION_INC=${KDEINC}/kdeversion.h
  elif [ -r "${KDEDIR}/include/kapp.h" ]; then
    KDEINC=${KDEDIR}/include
    KDE_VERSION_INC=${KDEINC}/kapp.h
  elif [ -r "${KDEDIR}/include/kde/kapp.h" ]; then
    KDEINC=${KDEDIR}/include/kde
    KDE_VERSION_INC=${KDEINC}/kapp.h
  fi
  if [ "${KDEINC}" != "" ]; then
    KDE_VERSION_MAJOR=$(awk '/#define KDE_VERSION_MAJOR/ { print $3; }' ${KDE_VERSION_INC})
    output "KDE_VERSION_MAJOR = $KDE_VERSION_MAJOR"
    output "CFLAGS_DEFINES += -DKDE_VERSION_MAJOR=$KDE_VERSION_MAJOR"
    info "yes (version=$KDE_VERSION_MAJOR)\n"
  else
    info "no\n"
  fi
else
  info "no\n"
fi

# Checking for Red Hat release
info "--- Checking Red Hat release..."
if [ -r /etc/redhat-release ]; then
  REDHAT_RELEASE=$(awk '/^Red Hat Linux release/{ print $5 }' \
      	      	   < /etc/redhat-release)
  if [ -z "${REDHAT_RELEASE}" ]; then
    unset REDHAT_RELEASE
    info "not a Red Hat system\n"
  else
    info "${REDHAT_RELEASE}\n"
    REDHAT_RELEASE_MAJOR=${REDHAT_RELEASE%%.*}
    REDHAT_RELEASE_MINOR=${REDHAT_RELEASE##*.}
    output "REDHAT_RELEASE = ${REDHAT_RELEASE}"
    output "REDHAT_RELEASE_MAJOR = ${REDHAT_RELEASE_MAJOR}"
    output "REDHAT_RELEASE_MINOR = ${REDHAT_RELEASE_MINOR}"
    output "CFLAGS_DEFINES += -DREDHAT_RELEASE=\"${REDHAT_RELEASE}\""
    output "CFLAGS_DEFINES += -DREDHAT_RELEASE_MAJOR=${REDHAT_RELEASE_MAJOR}"
    output "CFLAGS_DEFINES += -DREDHAT_RELEASE_MINOR=${REDHAT_RELEASE_MINOR}"
  fi
fi


