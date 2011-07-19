# Path to <common.h> and version headers
COMMON_H = -I$(top_srcdir)/misc -I$(top_builddir)

# Relative paths for the libasync series from top
ASYNC_CORE = async/core
ASYNC_CPP = async/cpp
ASYNC_AUDIO = async/audio

ASYNC_CORE_CFLAGS = -I$(top_srcdir)/$(ASYNC_CORE) @SIGC_CFLAGS@
ASYNC_CORE_LIBS = $(top_builddir)/$(ASYNC_CORE)/libasynccore.la @SIGC_LIBS@

ASYNC_CPP_CFLAGS = -I$(top_srcdir)/$(ASYNC_CPP) $(ASYNC_CORE_CFLAGS)
ASYNC_CPP_LIBS = $(top_builddir)/$(ASYNC_CPP)/libasynccpp.la $(ASYNC_CORE_LIBS)

ASYNC_AUDIO_CFLAGS = -I$(top_srcdir)/$(ASYNC_AUDIO) $(ASYNC_CORE_CFLAGS) \
		$(SPEEX_CFLAGS)
ASYNC_AUDIO_LIBS = $(top_builddir)/$(ASYNC_AUDIO)/libasyncaudio.la \
		$(ASYNC_CORE_LIBS) $(SPEEX_LIBS)

if HAVE_QT
ASYNC_QT = async/qt
ASYNC_QT_CFLAGS = -I$(top_srcdir)/$(ASYNC_QT) @QT_CFLAGS@ $(ASYNC_CORE_CFLAGS)
ASYNC_QT_LIBS = $(top_builddir)/$(ASYNC_QT)/libasyncqt.la \
		@QT_LIBS@ $(ASYNC_CORE_LIBS)
endif

# Paths for libecholib
ECHOLIB = echolib
ECHOLIB_CFLAGS = -I$(top_srcdir)/$(ECHOLIB) \
		$(ASYNC_AUDIO_CFLAGS) $(ASYNC_CPP_CFLAGS)
ECHOLIB_LIBS = $(top_builddir)/$(ECHOLIB)/libecholib.la \
		$(ASYNC_AUDIO_LIBS) $(ASYNC_CPP_LIBS)

# paths for liblocationinfo
LOCATIONINFO = locationinfo
LOCATIONINFO_CFLAGS = -I$(top_srcdir)/$(LOCATIONINFO)
LOCATIONINFO_LIBS = $(top_builddir)/$(LOCATIONINFO)/liblocationinfo.la

# paths for libtrx
TRX = svxlink/trx
TRX_CFLAGS = -I$(top_srcdir)/$(TRX) @SIGC_CFLAGS@ $(ASYNC_AUDIO_CFLAGS)
TRX_LIBS = $(top_builddir)/$(TRX)/libtrx.la @SIGC_LIBS@ $(ASYNC_AUDIO_LIBS)

SVXLINK_CFLAGS = -I$(top_srcdir)/svxlink/svxlink

LIBTOOL_MODULE_OPTIONS = -module -no-undefined -avoid-version -shared

# Some general definitions
LOCAL_DEFS = -DT_LINUX

# Set some standard build options
AM_CPPFLAGS = $(LOCAL_DEFS) $(COMMON_H)

# this is really important to avoid Tx.h confusion
DEFAULT_INCLUDES =

# rules for QT processing
.ui.h:
	@QT_UIC@ -o $@ $<

.h.moc.cpp:
	@QT_MOC@ -o $@ $<

.ui.uic.cpp:
	@QT_UIC@ -o $@ -impl $*.h $<

#.h.uic.cpp:
#$(QT_UIC) -o $@ -impl $< $(srcdir)/$(<:.h=.ui)

SUFFIXES = .h .ui .moc.cpp .uic.cpp
