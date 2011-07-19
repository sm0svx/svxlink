# Import some general items
include $(top_srcdir)/common.mk

TEXT_BASED = \
		AsyncAudioIO_demo \
		AsyncDnsLookup_demo \
		AsyncFdWatch_demo \
		AsyncTimer_demo \
		AsyncTcpClient_demo \
		AsyncUdpSocket_demo \
		AsyncCppApplication_demo \
		AsyncTcpServer_demo \
		AsyncConfig_demo \
		AsyncSerial_demo

QT_BASED = \
		AsyncQtApplication_demo

TO_BE_BUILT = $(TEXT_BASED)

if HAVE_QT
TO_BE_BUILT += $(QT_BASED)
endif

# C++ programs to compile
bin_PROGRAMS = $(TO_BE_BUILT)

# required by automake
EXTRA_PROGRAMS = $(QT_BASED)

# file extension for default build process
AM_DEFAULT_SOURCE_EXT = .cpp

# compiler flags
AM_CPPFLAGS += $(ASYNC_QT_CFLAGS) $(ASYNC_CPP_CFLAGS) $(ASYNC_AUDIO_CFLAGS)

# linker flags
LDADD = $(ASYNC_QT_LIBS) $(ASYNC_CPP_LIBS) $(ASYNC_AUDIO_LIBS)

EXTRA_DIST = test.cfg Makefile.default

