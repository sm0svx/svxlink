# Import some general items
include $(top_srcdir)/common.mk

# The name of the library
lib_LTLIBRARIES = libasyncqt.la

DIST_CPP = \
		AsyncQtApplication.cpp \
		AsyncQtDnsLookupWorker.cpp

DIST_H_MOC_PUBLIC = \
		AsyncQtApplication.h

DIST_H_MOC_PRIVATE = \
		AsyncQtDnsLookupWorker.h \
		AsyncQtTimer.h

DIST_H_MOC = $(DIST_H_MOC_PUBLIC) $(DIST_H_MOC_PRIVATE)

DIST_H_NO_MOC = 

FORMS =

FORMHEADERS = $(FORMS:.ui=.h)
MOC_CPP = $(FORMS:.ui=.moc.cpp) $(DIST_H_MOC:.h=.moc.cpp)
UIC_CPP = $(FORMS:.ui=.uic.cpp)

BUILT_SOURCES = $(FORMHEADERS) $(MOC_CPP) $(UIC_CPP)
CLEANFILES = $(BUILT_SOURCES)

libasyncqt_la_SOURCES = $(DIST_CPP) $(DIST_H_MOC) $(DIST_H_NO_MOC)
nodist_libasyncqt_la_SOURCES = $(FORMHEADERS) $(MOC_CPP) $(UIC_CPP)

# compiler flags
AM_CPPFLAGS += $(QT_CFLAGS) $(ASYNC_CORE_CFLAGS)

libasyncqt_la_LDFLAGS = -release @LIBASYNC_SHLIB_VERSION@

# Headers that define the API of this library
pkginclude_HEADERS = $(DIST_H_MOC_PUBLIC) $(DIST_H_NO_MOC)

# Files that should be packaged with the sources
EXTRA_DIST = $(FORMS) Makefile.default

