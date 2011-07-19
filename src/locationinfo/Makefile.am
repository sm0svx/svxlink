# Import some general items
include $(top_srcdir)/common.mk

# The name of the library
lib_LTLIBRARIES = liblocationinfo.la

DIST_CPP = \
		AprsTcpClient.cpp \
		AprsUdpClient.cpp \
		LocationInfo.cpp

DIST_H = \
		AprsClient.h \
		AprsTcpClient.h \
		AprsUdpClient.h \
		LocationInfo.h

# Sources needed to build the library
liblocationinfo_la_SOURCES = $(DIST_CPP) $(DIST_H)

# Extra compiler flags
AM_CPPFLAGS += $(ECHOLIB_CFLAGS)

# Headers that define the API of this library
pkginclude_HEADERS = $(DIST_H)

# Files that should be packaged with the sources
EXTRA_DIST = Makefile.default
