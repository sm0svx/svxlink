# Import some general items
include $(top_srcdir)/common.mk

# The name of the library
lib_LTLIBRARIES	= libasynccore.la

DIST_CPP = \
		AsyncApplication.cpp \
		AsyncConfig.cpp \
		AsyncDnsLookup.cpp \
		AsyncFdWatch.cpp \
		AsyncIpAddress.cpp \
		AsyncSerial.cpp \
		AsyncSerialDevice.cpp \
		AsyncTcpClient.cpp \
		AsyncTcpConnection.cpp \
		AsyncTcpServer.cpp \
		AsyncTimer.cpp \
		AsyncUdpSocket.cpp

DIST_H_PUBLIC = \
		AsyncApplication.h \
		AsyncConfig.h \
		AsyncDnsLookup.h \
		AsyncFdWatch.h \
		AsyncIpAddress.h \
		AsyncSerial.h \
		AsyncTcpClient.h \
		AsyncTcpConnection.h \
		AsyncTcpServer.h \
		AsyncTimer.h \
		AsyncUdpSocket.h

DIST_H_PRIVATE = \
		AsyncDnsLookupWorker.h \
		AsyncSerialDevice.h

DIST_H = $(DIST_H_PUBLIC) $(DIST_H_PRIVATE)

# Sources needed to build the library
libasynccore_la_SOURCES	= $(DIST_CPP) $(DIST_H)

# Extra compiler flags
AM_CPPFLAGS += $(ASYNC_CORE_CFLAGS)

libasynccore_la_LDFLAGS = -release @LIBASYNC_SHLIB_VERSION@

# Headers that define the API of this library
pkginclude_HEADERS = $(DIST_H)

# Files that should be packaged with the sources
EXTRA_DIST = Makefile.default