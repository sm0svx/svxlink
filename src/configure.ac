#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# macro definitions

# find location for a particular Qt4 executable
AC_DEFUN([QT4_PATH_PROG],
[
  # zero out return value
  AS_UNSET([$1])

  # make sure we have 'pkg-config'
  AS_IF([test -z "${PKG_CONFIG}"],
  [
    PKG_PROG_PKG_CONFIG()
    AS_IF([test -z "${PKG_CONFIG}"],
      [AC_MSG_ERROR(['pkg-config' is required])])
  ])

  # actual check
  AC_MSG_CHECKING([for $2])
  PROG=`"${PKG_CONFIG}" --variable="$2"_location QtCore`
  AS_IF([test -z "${PROG}" -o ! -f "${PROG}" -o ! -x "${PROG}"],
    [PROG=`"${PKG_CONFIG}" --variable=libdir QtCore`"/qt4/bin/$2"])
  AS_IF([test -z "${PROG}" -o ! -f "${PROG}" -o ! -x "${PROG}"],
  [
    AC_MSG_RESULT([no])
  ], [
    AC_MSG_RESULT([${PROG}])
    $1="${PROG}"
    AC_SUBST([$1])
  ])
])

# execution starts here

AC_PREREQ([2.66])
AC_INIT([svxlink], [snapshot], [svxlink-devel@lists.sourceforge.net])

AC_CONFIG_SRCDIR([svxlink/svxlink/svxlink.cpp])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11 no-define silent-rules tar-ustar no-dist-gzip dist-bzip2])
AM_SILENT_RULES([yes])

LT_PREREQ([2.2])
LT_INIT([dlopen])
AC_SUBST([LIBTOOL_DEPS])

# option to build user documentation for Qtel
AC_ARG_WITH([qteluserdocs],
  [AS_HELP_STRING([--with-qteluserdocs], [Build Qtel user documentation, too])])
AM_CONDITIONAL([WITH_QTELUSERDOCS], [test "x$with_qteluserdocs" = "xyes"])
AM_COND_IF([WITH_QTELUSERDOCS],
[
  # check for pdflatex
  AC_PATH_PROG([PDFLATEX], [pdflatex])
  AS_IF([test -z "${PDFLATEX}"],
    [AC_MSG_ERROR(['pdflatex' is required for Qtel documentation])])
  
  AC_PATH_PROG([KPSEWHICH], [kpsewhich])
  AS_IF([test -z "${KPSEWHICH}"],
    [AC_MSG_ERROR(['kpsewhich' is required for Qtel documentation])])

  RESULT=`${KPSEWHICH} scrartcl.cls`
  AS_IF([test -z "${RESULT}"],
    [AC_MSG_ERROR(['scrartcl.cls' is required for Qtel documentation])])
])

# check for required programs
AC_PROG_CPP
AS_IF([test -z "${CPP}"], [AC_MSG_ERROR(['cpp' is required])])

AC_PROG_CC
AS_IF([test -z "${CC}"], [AC_MSG_ERROR(['cc' is required])])

AC_PROG_CXX
AS_IF([test -z "${CXX}"], [AC_MSG_ERROR(['cxx' is required])])

AC_PROG_GREP
AS_IF([test -z "${GREP}"], [AC_MSG_ERROR(['grep' is required])])

AC_PROG_AWK
AS_IF([test -z "${AWK}"], [AC_MSG_ERROR(['awk' is required])])

AC_PROG_INSTALL
AS_IF([test -z "${INSTALL}"], [AC_MSG_ERROR(['install' is required])])

AC_PATH_PROG([RMDIR], [rmdir])
AS_IF([test -z "${RMDIR}"], [AC_MSG_ERROR(['rmdir' is required])])

AC_PATH_PROG([GZIP], [gzip])
AS_IF([test -z "${GZIP}"], [AC_MSG_ERROR(['gzip' is required])])

PKG_PROG_PKG_CONFIG()
AS_IF([test -z "${PKG_CONFIG}"], [AC_MSG_ERROR(['pkg-config' is required])])

# check for libc and libc-dev
AC_SEARCH_LIBS([perror], [c], [],
  [AC_MSG_ERROR([perror() is required (presumably in libc)])])
AC_CHECK_HEADER([stdio.h], [],
  [AC_MSG_ERROR(['stdio.h' not found: need libc development files])])

# check for libdl and libdl-dev
AC_SEARCH_LIBS([dlsym], [dl], [],
  [AC_MSG_ERROR([dlsym() is required (presumably in libdl)])])
AC_CHECK_HEADER([dlfcn.h], [],
  [AC_MSG_ERROR(['dlfcn.h' not found: need libdl development files])])

# check for libgsm and libgsm-dev
AC_SEARCH_LIBS([gsm_decode], [gsm], [],
  [AC_MSG_ERROR([gsm_decode() is required (presumably in libgsm)])])
AC_CHECK_HEADER([gsm/gsm.h], [],
  [AC_MSG_ERROR(['gsm/gsm.h' not found: need libgsm development files])])

# check for libm and libm-dev
AC_SEARCH_LIBS([cosf], [m], [],
  [AC_MSG_ERROR([cosf() is required (presumably in libm)])])
AC_CHECK_HEADER([math.h], [],
  [AC_MSG_ERROR(['math.h' not found: need libm development files])])

# check for libpopt and libpopt-dev
AC_SEARCH_LIBS([poptGetArg], [popt], [],
  [AC_MSG_ERROR([poptGetArg() is required (presumably in libpopt)])])
AC_CHECK_HEADER([popt.h], [],
  [AC_MSG_ERROR(['popt.h' not found: need libpopt development files])])

# check for libpthread and libpthread-dev
AC_SEARCH_LIBS([pthread_mutex_lock], [pthread],
[
  PTHREAD_CFLAGS="-pthread"
  AC_SUBST([PTHREAD_CFLAGS])
  PTHREAD_LIBS="-pthread -lpthread"
  AC_SUBST([PTHREAD_LIBS])
], [
  AC_MSG_ERROR([pthread_mutex_lock() is required (presumably in libpthread)])
])

AC_CHECK_HEADER([pthread.h], [],
  [AC_MSG_ERROR(['pthread.h' not found: need libpthread development files])])

# check for libgcrypt and libgcrypt-dev
AC_SEARCH_LIBS([gcry_md_setkey], [gcrypt], [],
  [AC_MSG_ERROR([gcry_md_setkey() is required (presumably in libgcrypt)])])
AC_CHECK_HEADER([gcrypt.h], [],
  [AC_MSG_ERROR(['gcrypt.h' not found: need libgcrypt development files])])

# check for ALSA and OSS
AC_SEARCH_LIBS([snd_pcm_readi], [asound],
  [AC_CHECK_HEADER([alsa/version.h], [HAVE_ALSA=1])])
AM_CONDITIONAL([HAVE_ALSA], [test ${HAVE_ALSA}])

AC_CHECK_HEADERS([sys/soundcard.h soundcard.h machine/soundcard.h],
  [HAVE_OSS=1])
AM_CONDITIONAL([HAVE_OSS], [test ${HAVE_OSS}])

AS_IF([test ! "${HAVE_ALSA}" -a ! "${HAVE_OSS}"],
  [AC_MSG_ERROR([found no way to access sound (neither ALSA nor OSS)])])

# look for speex and #define USE_SPEEX=1 if found
PKG_CHECK_MODULES([SPEEX], [speex],
  [AC_DEFINE([USE_SPEEX], [1])],
  [] # required to prevent exit for PKG_CHECK_MODULES
)
AM_CONDITIONAL([USE_SPEEX], [test ${USE_SPEEX}])

# look for libsigc++
PKG_CHECK_MODULES([SIGC], [sigc++-1.2],
  [],
  [AC_MSG_ERROR(['libsigc++-1.2 is required'])]
)

# locate Tcl install path
# we could use SC_PATH_TCLCONFIG if Fedora included tcl.m4 or tclX.X.m4
for i in `ls -d "${libdir}" 2>/dev/null` \
  `ls -d "${exec_prefix}"/lib 2>/dev/null` \
  `ls -d "${prefix}"/lib 2>/dev/null` \
  `ls -d /usr/local/lib 2>/dev/null` \
  `ls -d /usr/contrib/lib 2>/dev/null` \
  `ls -d /usr/lib 2>/dev/null` \
  `ls -d /usr/lib64 2>/dev/null` \
  `ls -d /usr/share/tcltk/tcl8.5 2>/dev/null` \
  `ls -d /usr/lib/tcl8.5 2>/dev/null` \
  `ls -d /usr/share/tcltk/tcl8.4 2>/dev/null` \
  `ls -d /usr/lib/tcl8.4 2>/dev/null` \
; do
  AS_IF([test -f "$i/tclConfig.sh"],
  [
    TCL_BIN_DIR=`(cd "$i"; pwd)`
    break
  ])
done
AS_IF([test x"${TCL_BIN_DIR}" = x],
  [AC_MSG_ERROR([Cannot find Tcl configuration definitions (tclConfig.sh)])])
AC_SUBST([TCL_CFLAGS], ["${TCL_INCLUDE_SPEC}"])
AC_SUBST([TCL_LIBS])

# load Tcl configuration definitions
source $TCL_BIN_DIR/tclConfig.sh

# check for Tcl library
AC_SEARCH_LIBS([Tcl_CreateCommand], ["tcl${TCL_VERSION}"], ,
  [AC_MSG_ERROR([Tcl_CreateCommand() is required (in libtcl"${TCL_VERSION}")])])

# check for Tcl include file
CPPFLAGS="$CPPFLAGS $TCL_INCLUDE_SPEC"
AC_CHECK_HEADER([tcl.h], ,
  [AC_MSG_ERROR(['tcl.h' not found: need libtcl development files])])

# check for Qt, also sets QT_CFLAGS and QT_LIBS via AC_SUBST
PKG_CHECK_MODULES([QT], [QtCore QtGui QtNetwork],
[
  HAVE_QT=1

  QT4_PATH_PROG([QT_MOC], [moc])
  AS_IF([test -z "${QT_MOC}"], [AC_MSG_ERROR(['moc' is required])])

  QT4_PATH_PROG([QT_UIC], [uic])
  AS_IF([test -z "${QT_UIC}"], [AC_MSG_ERROR(['uic' is required])])

  QT4_PATH_PROG([QT_RCC], [rcc])
  AS_IF([test -z "${QT_RCC}"], [AC_MSG_ERROR(['rcc' is required])])

  QT4_PATH_PROG([QT_LRELEASE], [lrelease])
  AS_IF([test -z "${QT_LRELEASE}"], [AC_MSG_ERROR(['lrelease' is required])])
],[
  # required to prevent exit for PKG_CHECK_MODULES
])
AM_CONDITIONAL([HAVE_QT], [test ${HAVE_QT}])

# Check for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h locale.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h syslog.h termios.h unistd.h], [],
  [AC_MSG_ERROR([required headers not found])]
)

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
AC_SUBST([VERSIONFILE])

# read version information and export as preprocessor defines
# generate list of version header files for make, as well
VERSIONHEADERS=
while read LINE
do
  AS_IF([`${ECHO} ${LINE} | ${GREP} -q -e "^.*=.*$" > /dev/null 2>&1`],
  [
    PKGNAME="`${ECHO} ${LINE} | ${AWK} -F\= '/^.*=.*$/ {print $1}'`" 
    PKGVERSION="`${ECHO} ${LINE} | ${AWK} -F\= '/^.*=.*$/ {print $2}'`" 
    PKGVAR="${PKGNAME}_VERSION"
    AC_DEFINE_UNQUOTED([${PKGVAR}], ["${PKGVERSION}"])
    export ${PKGVAR}=${PKGVERSION}
    VERSIONHEADERS="${VERSIONHEADERS} ${PKGNAME}.h"
  ])
done < "${srcdir}/${VERSIONFILE}"
AC_SUBST([VERSIONHEADERS])

# export version numbers to Makefiles for library versioning
shlib_version() {
  ${ECHO} "$1" | ${AWK} -F. '{OFS=":"; print $1,$2,$3}'
}
AC_SUBST([LIBASYNC_SHLIB_VERSION], [`shlib_version "${LIBASYNC_VERSION}"`])
AC_SUBST([ECHOLIB_SHLIB_VERSION], [`shlib_version "${ECHOLIB_VERSION}"`])

# program configuration definitions
AC_DEFINE([INTERNAL_SAMPLE_RATE], [8000], [Audio sample rate in Hz])

# build system definitions
TOP_SRCDIR='''$(top_srcdir)'''
TOP_BUILDDIR='''$(top_builddir)'''

# Path to <common.h> and version headers
COMMON_H="-I${TOP_SRCDIR}/misc -I${TOP_BUILDDIR}"
# Some general definitions
LOCAL_DEFS="-DT_LINUX"
# Set some standard cpp flags and export to make
AC_SUBST([AM_CPPFLAGS], ["${LOCAL_DEFS} ${COMMON_H}"])

# Set some standard linker flags and export to make
AC_SUBST([AM_LDFLAGS], [-Wl,--as-needed])

# this is needed to compile svxlink modules
AC_SUBST([SVXLINK_MODULE_CFLAGS], ["-fPIC -I${TOP_SRCDIR}/svxlink/svxlink"])

# libtool options for svxlink modules
AC_SUBST([LIBTOOL_PLUGIN_OPTIONS],
  ["-shared -export-dynamic -avoid-version -no-undefined"]) 

# Relative paths for the libasync series from top
ASYNC_SUBDIR="async"
ASYNC_CORE="${ASYNC_SUBDIR}/core"
ASYNC_CPP="${ASYNC_SUBDIR}/cpp"
ASYNC_AUDIO="${ASYNC_SUBDIR}/audio"
ASYNC_QT="${ASYNC_SUBDIR}/qt"

# async core
ASYNC_CORE_CFLAGS="-I${TOP_SRCDIR}/${ASYNC_CORE} ${SIGC_CFLAGS}"
ASYNC_CORE_LIBS="${TOP_BUILDDIR}/${ASYNC_CORE}/libasynccore.la ${SIGC_LIBS}"
AC_SUBST([ASYNC_CORE_CFLAGS])
AC_SUBST([ASYNC_CORE_LIBS])

# async cpp
ASYNC_CPP_CFLAGS="-I${TOP_SRCDIR}/${ASYNC_CPP} ${ASYNC_CORE_CFLAGS} ${PTHREAD_CFLAGS}"
ASYNC_CPP_LIBS="${TOP_BUILDDIR}/${ASYNC_CPP}/libasynccpp.la ${ASYNC_CORE_LIBS} ${PTHREAD_LIBS}"
AC_SUBST([ASYNC_CPP_CFLAGS])
AC_SUBST([ASYNC_CPP_LIBS])

# async audio
ASYNC_AUDIO_CFLAGS="-I${TOP_SRCDIR}/${ASYNC_AUDIO} ${ASYNC_CORE_CFLAGS} ${SPEEX_CFLAGS}"
ASYNC_AUDIO_LIBS="${TOP_BUILDDIR}/${ASYNC_AUDIO}/libasyncaudio.la ${ASYNC_CORE_LIBS} ${SPEEX_LIBS}"
AC_SUBST([ASYNC_AUDIO_CFLAGS])
AC_SUBST([ASYNC_AUDIO_LIBS])

# async qt
AM_COND_IF([HAVE_QT], [
  ASYNC_QT_CFLAGS="-I${TOP_SRCDIR}/${ASYNC_QT} ${QT_CFLAGS} ${ASYNC_CORE_CFLAGS}"
  ASYNC_QT_LIBS="${TOP_BUILDDIR}/${ASYNC_QT}/libasyncqt.la ${QT_LIBS} ${ASYNC_CORE_LIBS}"
  AC_SUBST([ASYNC_QT_CFLAGS])
  AC_SUBST([ASYNC_QT_LIBS])
])

# Paths for libecholib
ECHOLIB=echolib
ECHOLIB_CFLAGS="-I${TOP_SRCDIR}/${ECHOLIB} ${ASYNC_AUDIO_CFLAGS} ${ASYNC_CPP_CFLAGS}"
ECHOLIB_LIBS="${TOP_BUILDDIR}/${ECHOLIB}/libecholib.la ${ASYNC_AUDIO_LIBS} ${ASYNC_CPP_LIBS}"
AC_SUBST([ECHOLIB_CFLAGS])
AC_SUBST([ECHOLIB_LIBS])

# paths for liblocationinfo
LOCATIONINFO=locationinfo
LOCATIONINFO_CFLAGS="-I${TOP_SRCDIR}/${LOCATIONINFO}"
LOCATIONINFO_LIBS="${TOP_BUILDDIR}/${LOCATIONINFO}/liblocationinfo.a"
AC_SUBST([LOCATIONINFO_CFLAGS])
AC_SUBST([LOCATIONINFO_LIBS])

# paths for libtrx
TRX=svxlink/trx
TRX_CFLAGS="-I${TOP_SRCDIR}/${TRX} ${SIGC_CFLAGS} ${ASYNC_AUDIO_CFLAGS}"
TRX_LIBS="${TOP_BUILDDIR}/${TRX}/libtrx.a ${SIGC_LIBS} ${ASYNC_AUDIO_LIBS}"
AC_SUBST([TRX_CFLAGS])
AC_SUBST([TRX_LIBS])

# otherwise defaults to -I${srcdir}, needed to avoid Tx.h confusion in 'svxlink'
AC_SUBST([DEFAULT_INCLUDES], [])

# install paths
AC_SUBST([PLUGINDIR], ['$(libdir)/svxlink'])
AC_SUBST([SITECONFDIR], ['$(sysconfdir)/svxlink'])
AC_SUBST([MODULECONFDIR], ['$(SITECONFDIR)/svxlink.d'])
AC_SUBST([TCLMODULEDIR], ['$(pkgdatadir)/modules.d'])
AC_SUBST([EVENTDIR], ['$(pkgdatadir)/events.d'])
AC_SUBST([SPOOLDIR], ['$(localstatedir)/spool/svxlink'])
AC_SUBST([QSORECORDERDIR], ['$(SPOOLDIR)/qso_recorder'])
AC_SUBST([VOICEMAILDIR], ['$(SPOOLDIR)/voice_mail'])
AC_SUBST([PROPAGATIONDIR], ['$(SPOOLDIR)/propagation_monitor'])
AC_SUBST([VHFDXDIR], ['$(PROPAGATIONDIR)/vhfdx'])
AC_SUBST([DXROBOTDIR], ['$(PROPAGATIONDIR)/dxrobot'])
AC_SUBST([DESKTOPDIR], ['$(datadir)/applications'])
AC_SUBST([ICONDIR], ['$(datadir)/icons'])
AC_SUBST([QTELSHAREDIR], ['$(datadir)/qtel'])
AC_SUBST([QTELSOUNDDIR], ['$(QTELSHAREDIR)/sounds'])
AC_SUBST([QTELMESSAGEDIR], '[$(QTELSHAREDIR)/translations'])

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
