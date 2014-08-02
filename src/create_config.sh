#!/bin/bash

SRC_ROOT=$(cd $(dirname $0); pwd)

info()
{
  /bin/echo -en $1
}

output()
{
  echo $1 >> $OUTPUT_FILE
}

exit_error()
{
  rm -f $OUTPUT_FILE
  exit 1
}

configure_file()
{
  infile=$1
  outfile=$2
  outdir=$(dirname "${outfile}")
  info "--- Generating $outfile...\n"
  [ -d "${outdir}" ] || mkdir -p "${outdir}"
  local IFS=""
  while read -r line; do
    if [[ "$line" =~ \@(.*)\@ ]]; then
      var="${BASH_REMATCH[1]}"
      echo "${line/@${var}@/${!var}}"
    else
      echo "${line}"
    fi
  done <"${SRC_ROOT}/${infile}" >"${SRC_ROOT}/${outfile}"
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
  output "CHGRP_H=chgrp -h"
else
  info "no\n"
  output "CHGRP_H=chgrp"
fi
rm -f chgrp_test-$$.tmp

# Check if the "-h" switch may be used with the chown command
info "--- Checking if chown understand the -h switch..."
ln -s . chown_test-$$.tmp
if chown -h `id -un` chown_test-$$.tmp 2> /dev/null; then
  info "yes\n"
  output "CHOWN_H=chown -h"
else
  info "no\n"
  output "CHOWN_H=chown"
fi
rm -f chown_test-$$.tmp

# Check if echo reacts to "--" as "stop processing arguments"
info "--- Checking if echo parses --..."
if [ -z "$(/bin/echo -ne --)" ]; then
  info "yes\n"
  output "ECHO=/bin/echo -e --"
else
  info "no\n"
  output "ECHO=/bin/echo -e"
fi

# Checking for QT
info "--- Checking for QT..."
if which pkg-config > /dev/null 2>&1; then
  if pkg-config QtCore; then
    QT_MODULES="QtCore QtGui QtNetwork"
    info "yes (pkg-config QtCore)\n"
    output "QT_LIBPATH=$(pkg-config $QT_MODULES --libs-only-L)"
    output "QT_LIBS=$(pkg-config $QT_MODULES --libs-only-l)"
    output "QT_INCPATH=$(pkg-config $QT_MODULES --cflags-only-I)"
    output "QT_CFLAGS=$(pkg-config $QT_MODULES --cflags-only-other)"
    QT_PREFIX=$(pkg-config QtCore --variable=prefix)
    QT_BIN="${QT_PREFIX}/bin"
    output "QT_BIN=${QT_BIN}"
    QT_MOC=$(pkg-config QtCore --variable=moc_location)
    if [ ! -x "$QT_MOC" ]; then
      QT_MOC="$QT_BIN/moc"
    fi
    QT_UIC=$(pkg-config QtCore --variable=uic_location)
    if [ ! -x "$QT_UIC" ]; then
      QT_UIC="$QT_BIN/uic"
    fi
    QT_RCC=$(pkg-config QtCore --variable=rcc_location)
    if [ ! -x "$QT_RCC" ]; then
      QT_RCC="$QT_BIN/rcc"
    fi
    QT_LRELEASE=$(pkg-config QtCore --variable=lrelease_location)
    if [ ! -x "$QT_LRELEASE" ]; then
      QT_LRELEASE="$QT_BIN/lrelease"
    fi
    output "QT_MOC=${QT_MOC}"
    output "QT_UIC=${QT_UIC}"
    output "QT_RCC=${QT_RCC}"
    output "QT_LRELEASE=${QT_LRELEASE}"
  else
    info "no (optional)\n"
  fi
else
  info "no (optional)\n"
fi

# Checking for libsigc++
sigc_version=2.0
info "--- Checking for sigc++ $sigc_version..."
if which pkg-config > /dev/null 2>&1; then
  if pkg-config sigc++-$sigc_version; then
    info "yes\n"
    output "SIGC_LIBPATH=$(pkg-config sigc++-$sigc_version --libs-only-L)"
    output "SIGC_LIBS=$(pkg-config sigc++-$sigc_version --libs-only-l)"
    output "SIGC_INCPATH=$(pkg-config sigc++-$sigc_version --cflags-only-I)"
  else
    info "no (required)\n"
    exit_error
  fi
else
  info "no (required)\n"
  exit_error
fi

# Checking for tcl development library
info "--- Checking for TCL development library..."
tclConfig=$(ls /usr/lib/tclConfig.sh /usr/lib/tcl8.*/tclConfig.sh \
	       /usr/lib64/tclConfig.sh /usr/lib64/tcl8.*/tclConfig.sh \
	    2>/dev/null | head -1)
if [ -n "$tclConfig" -a -r "$tclConfig" ]; then
  . $tclConfig
  info "${TCL_VERSION}\n"
  output "TCL_LIBS=${TCL_LIB_FLAG}"
  output "TCL_INCPATH=${TCL_INCLUDE_SPEC}"
else
  info "no (required)\n"
  exit_error
fi

# Checking for speex
info "--- Checking for speex..."
if which pkg-config > /dev/null 2>&1; then
  if pkg-config speex; then
    ver=$(pkg-config speex --modversion)
    ver_major=$(echo $ver | sed -r 's/^([0-9]+)\..*$/\1/')
    ver_minor=$(echo $ver | sed -r 's/^([0-9]+)\.([0-9]+).*$/\2/')
    info "$ver\n"
    output "SPEEX_LIBPATH=$(pkg-config speex --libs-only-L)"
    output "SPEEX_LIBS=$(pkg-config speex --libs-only-l)"
    output "SPEEX_INCPATH=$(pkg-config speex --cflags-only-I)"
    output "CFLAGS_DEFINES+=-DSPEEX_MAJOR=$ver_major -DSPEEX_MINOR=$ver_minor"
    output "USE_SPEEX=1"
  else
    info "no (optional)\n"
  fi
else
  info "no (optional)\n"
fi

# Checking for Opus
info "--- Checking for opus..."
if which pkg-config > /dev/null 2>&1; then
  if pkg-config opus; then
    ver=$(pkg-config opus --modversion)
    ver_major=$(echo $ver | sed -r 's/^([0-9]+)\..*$/\1/')
    ver_minor=$(echo $ver | sed -r 's/^([0-9]+)\.([0-9]+).*$/\2/')
    info "$ver\n"
    output "OPUS_LIBPATH=$(pkg-config opus --libs-only-L)"
    output "OPUS_LIBS=$(pkg-config opus --libs-only-l)"
    output "OPUS_INCPATH=$(pkg-config opus --cflags-only-I)"
    output "CFLAGS_DEFINES+=-DOPUS_MAJOR=$ver_major -DOPUS_MINOR=$ver_minor"
    output "USE_OPUS=1"
  else
    info "no (optional)\n"
  fi
else
  info "no (optional)\n"
fi

# Checking for libgcrypt
info "--- Checking for libgcrypt..."
if which libgcrypt-config > /dev/null 2>&1; then
  info "$(libgcrypt-config --version)\n"
  output "GCRYPT_LIBS=$(libgcrypt-config --libs)"
  output "CFLAGS+=$(libgcrypt-config --cflags)"
else
  info "no (required)\n"
  exit_error
fi

# Generate config.h
SYSCONF_INSTALL_DIR=${SYSCONF_INSTALL_DIR:-/etc}
SVX_SYSCONF_INSTALL_DIR=${SVX_SYSCONF_INSTALL_DIR:-$SYSCONF_INSTALL_DIR/svxlink}
SHARE_INSTALL_PREFIX=${SHARE_INSTALL_PREFIX:-/usr/share}
SVX_SHARE_INSTALL_DIR=${SVX_SHARE_INSTALL_DIR:-$SHARE_INSTALL_PREFIX/svxlink}
configure_file "config.h.in" "include/config.h"

# Generate svxlink.conf
LIB_INSTALL_DIR=${LIB_INSTALL_DIR:-/usr/lib}
SVX_MODULE_INSTALL_DIR=${SVX_MODULE_INSTALL_DIR:-$LIB_INSTALL_DIR/svxlink}
LOCAL_STATE_DIR=${LOCAL_STATE_DIR:-/var}
SVX_SPOOL_INSTALL_DIR=${SVX_SPOOL_INSTALL_DIR:-$LOCAL_STATE_DIR/spool/svxlink}
configure_file "svxlink/svxlink/svxlink.conf.in" "svxlink/svxlink/svxlink.conf"

# Generate ModuleTclVoiceMail.tcl and TclVoiceMail.conf
configure_file "svxlink/modules/tcl_voice_mail/ModuleTclVoiceMail.tcl.in" \
               "svxlink/modules/tcl_voice_mail/ModuleTclVoiceMail.tcl"
configure_file "svxlink/modules/tcl_voice_mail/TclVoiceMail.conf.in" \
               "svxlink/modules/tcl_voice_mail/TclVoiceMail.conf"

# Generate ModulePropagationMonitor.conf and .procmailrc
configure_file \
  "svxlink/modules/propagation_monitor/ModulePropagationMonitor.conf.in" \
  "svxlink/modules/propagation_monitor/ModulePropagationMonitor.conf"
configure_file \
  "svxlink/modules/propagation_monitor/procmailrc.in" \
  "svxlink/modules/propagation_monitor/.procmailrc"

exit 0

