#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
LT_PREREQ([2.2])

AC_INIT([svxlink], [snapshot], [svxlink-devel@lists.sourceforge.net])

AC_CONFIG_SRCDIR([svxlink/svxlink/svxlink.cpp])
#AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([tar-ustar silent-rules no-dist-gzip dist-bzip2])
LT_INIT
AC_SUBST([LIBTOOL_DEPS])

# check for required programs
AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX
AC_PROG_GREP
AC_PROG_AWK
AC_PROG_MKDIR_P
AC_PATH_PROG([CHOWN], [chown])
AC_PATH_PROG([RMDIR], [rmdir])

if test -z "${CPP}" ; then
   AC_MSG_ERROR(['cpp' is required])
fi
if test -z "${CC}" ; then
   AC_MSG_ERROR(['cc' is required])
fi
if test -z "${CXX}" ; then
   AC_MSG_ERROR(['cxx' is required])
fi
if test -z "${GREP}" ; then
   AC_MSG_ERROR(['grep' is required])
fi
if test -z "${AWK}" ; then
   AC_MSG_ERROR(['awk' is required])
fi
if test -z "${MKDIR_P}" ; then
   AC_MSG_ERROR(['mkdir -p' is required])
fi
if test -z "${CHOWN}" ; then
   AC_MSG_ERROR(['chown' is required])
fi
if test -z "${RMDIR}" ; then
   AC_MSG_ERROR(['RMDIR' is required])
fi

# check for optional programs
AC_PATH_PROG([PDFLATEX], [pdflatex])
AM_CONDITIONAL([HAVE_PDFLATEX], [test -z "${PDFLATEX}"])

# check for libraries
AC_CHECK_LIB([c], [perror])
AC_CHECK_LIB([dl], [dlsym])
AC_CHECK_LIB([gsm], [gsm_decode])
AC_CHECK_LIB([m], [cosf])
AC_CHECK_LIB([popt], [poptGetArg])
AC_CHECK_LIB([pthread], [pthread_mutex_lock])
AC_CHECK_LIB([gcrypt], [gcry_md_setkey])

# check for ALSA and OSS
AC_CHECK_LIB([asound], [snd_pcm_readi])
AM_CONDITIONAL([HAVE_ALSA], [test "${HAVE_LIBASOUND}"])
AC_CHECK_HEADERS([sys/soundcard.h soundcard.h machine/soundcard.h],
    [HAVE_OSS=1], [HAVE_OSS=0])
AM_CONDITIONAL([HAVE_OSS], [test "${HAVE_OSS}"])

# check for Tcl
AC_CHECK_LIB([tcl8.4], [Tcl_CreateCommand], [TCL_VERSION=8.4])
AM_CONDITIONAL([HAVE_TCL], [test -n "${TCL_VERSION}"])
AM_COND_IF([HAVE_TCL], [
    AC_SUBST([TCL_CFLAGS], [-I/usr/include/tcl${TCL_VERSION}])
    AC_SUBST([TCL_LIBS], [-ltcl${TCL_VERSION}])
])

# look for libsigc++
PKG_CHECK_MODULES([SIGC], [sigc++-1.2])

# look for speex and #define USE_SPEEX=1 if found
PKG_CHECK_MODULES([SPEEX], [speex], [USE_SPEEX=1], [USE_SPEEX=0])
AM_CONDITIONAL([USE_SPEEX], [test "${USE_SPEEX}"])
AM_COND_IF([USE_SPEEX], AC_DEFINE([USE_SPEEX], [1], [Use SPEEX]))

# look for qt-mt, or otherwise just qt; find moc
PKG_CHECK_MODULES([QT], [qt-mt], [QT_VARIANT=qt-mt],
    [PKG_CHECK_MODULES([QT], [qt], [QT_VARIANT=qt])])
AM_CONDITIONAL([HAVE_QT], [test -n "${QT_VARIANT}"])
AM_COND_IF([HAVE_QT], [
    AC_PATH_PROG([QT_MOC], [moc])
    AC_PATH_PROG([QT_UIC], [uic])
    AC_PATH_PROG([QT_LRELEASE], [lrelease])
])

# Check for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h locale.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h syslog.h termios.h unistd.h])

# Check for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Check for library functions.
AC_FUNC_ERROR_AT_LINE
AC_HEADER_MAJOR
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([dup2 floor gettimeofday inet_ntoa memchr memmove memset pow regcomp select setlocale socket sqrt strchr strdup strerror strrchr strstr strtol])

# version numbers
# if this mechanism is ever used one would also have to export to make since
# the version are used for library versioning.
# probably best to define a shell variable and use it both in
# AC_DEFINE_UNQUOTED and AC_SUBST
#AC_DEFINE([QTEL_VERSION], ["0.11.3"])
#AC_DEFINE([ECHOLIB_VERSION], ["0.14.0.99rc1"])
#AC_DEFINE([LIBASYNC_VERSION], ["0.18.0.99rc1"])
#AC_DEFINE([SVXLINK_VERSION], ["0.12.99rc2"])
#AC_DEFINE([MODULE_HELP_VERSION], ["0.7.0"])
#AC_DEFINE([MODULE_PARROT_VERSION], ["0.7.0"])
#AC_DEFINE([MODULE_ECHOLINK_VERSION], ["0.10.0.99rc1"])
#AC_DEFINE([MODULE_TCL_VERSION], ["0.3.0"])
#AC_DEFINE([MODULE_DTMF_REPEATER_VERSION], ["0.2.0"])
#AC_DEFINE([MODULE_METARINFO_VERSION], ["0.0.2"])
#AC_DEFINE([REMOTE_TRX_VERSION], ["0.3.0"])
#AC_DEFINE([SIGLEV_DET_CAL_VERSION], ["0.1.0"])

# file name with version information
VERSIONFILE="versions"
AC_SUBST([VERSIONFILE], [${VERSIONFILE}])

# read version information and export as preprocessor defines
# generate list of version header files for make, as well
VERSIONHEADERS=
while read LINE
do
      if `echo ${LINE} | grep -q -e "^.*=.*$" > /dev/null 2>&1`; then 
          PKGNAME="`echo ${LINE} | ${AWK} -F\= '/^.*=.*$/ {print $1}'`" 
          PKGVAR="${PKGNAME}_VERSION"
          PKGVERSION="`echo ${LINE} | ${AWK} -F\= '/^.*=.*$/ {print $2}'`" 
          VERSIONHEADERS="${VERSIONHEADERS} ${PKGNAME}.h"
          AC_DEFINE_UNQUOTED([${PKGVAR}], ["${PKGVERSION}"])
          export ${PKGVAR}=${PKGVERSION}
      fi
done < "${srcdir}/${VERSIONFILE}"
AC_SUBST([VERSIONHEADERS], [${VERSIONHEADERS}])

# export version numbers to Makefiles for library versioning
shlib_version() {
  echo $1 | awk -F. '/^.*\..*\..*(\..*)?$/ {print $1 "." $2 "." $3}'
}
AC_SUBST([LIBASYNC_SHLIB_VERSION], [`shlib_version ${LIBASYNC_VERSION}`])
AC_SUBST([ECHOLIB_SHLIB_VERSION], [`shlib_version ${ECHOLIB_VERSION}`])

# local definitions
AC_DEFINE([INTERNAL_SAMPLE_RATE], [8000])

# install paths
AC_SUBST([PLUGINDIR], ['$(libdir)/svxlink'])
AC_SUBST([SITECONFDIR], ['$(sysconfdir)/svxlink'])
AC_SUBST([MODULECONFDIR], ['$(SITECONFDIR)/svxlink.d'])
AC_SUBST([TCLMODULEDIR], ['$(datadir)/modules.d'])
AC_SUBST([EVENTDIR], ['$(datadir)/events.d'])
AC_SUBST([SPOOLDIR], ['$(prefix)/spool/svxlink'])
AC_SUBST([QSORECORDERDIR], ['$(SPOOLDIR)/qso_recorder'])
AC_SUBST([VOICEMAILDIR], ['$(SPOOLDIR)/voice_mail'])

# auxiliary install information
#AC_SUBST([SVXLINK_USER], [svxlink])
AC_SUBST([SVXLINK_USER], [root])
AC_SUBST([SVXLINK_GROUP], [daemon])

# generate these output files
AC_CONFIG_FILES([Makefile
                 version/Makefile
                 async/Makefile
                 async/audio/Makefile
                 async/core/Makefile
                 async/cpp/Makefile
                 async/demo/Makefile
                 async/qt/Makefile
                 doc/Makefile
                 doc/man/Makefile
                 doc/qteluserdocs/Makefile
                 echolib/Makefile
                 locationinfo/Makefile
                 misc/Makefile
                 qtel/Makefile
                 qtel/translations/Makefile
                 svxlink/Makefile
                 svxlink/modules/Makefile
                 svxlink/modules/dtmf_repeater/Makefile
                 svxlink/modules/echolink/Makefile
                 svxlink/modules/help/Makefile
                 svxlink/modules/metarinfo/Makefile
                 svxlink/modules/parrot/Makefile
                 svxlink/modules/propagation_monitor/Makefile
                 svxlink/modules/selcallenc/Makefile
                 svxlink/modules/tcl/Makefile
                 svxlink/modules/tcl_voice_mail/Makefile
                 svxlink/remotetrx/Makefile
                 svxlink/siglevdetcal/Makefile
                 svxlink/svxlink/Makefile
                 svxlink/trx/Makefile])
AC_OUTPUT
