# Import some general items
include $(top_srcdir)/common.mk

# The name of the library
lib_LTLIBRARIES = libasynccpp.la

DIST_CPP = \
		AsyncCppApplication.cpp \
		AsyncCppDnsLookupWorker.cpp

DIST_H_PUBLIC = \
		AsyncCppApplication.h

DIST_H_PRIVATE = \
		AsyncCppDnsLookupWorker.h

DIST_H = $(DIST_H_PUBLIC) $(DIST_H_PRIVATE)

# Sources needed to build the library
libasynccpp_la_SOURCES = $(DIST_CPP) $(DIST_H)

# Extra compiler flags
AM_CPPFLAGS += $(ASYNC_CPP_CFLAGS)

libasynccpp_la_LDFLAGS = -release @LIBASYNC_SHLIB_VERSION@

# Headers that define the API of this library
pkginclude_HEADERS = $(DIST_H_PUBLIC)

# Files that should be packaged with the sources
EXTRA_DIST = Makefile.default
