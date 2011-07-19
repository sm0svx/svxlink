# Import some general items
include $(top_srcdir)/common.mk

bin_PROGRAMS = siglevdetcal

DIST_CPP = \
		siglevdetcal.cpp

DIST_H =

EXTRA_DIST = Makefile.default

siglevdetcal_SOURCES = $(DIST_CPP) $(DIST_H)

AM_CPPFLAGS += $(ASYNC_CPP_CFLAGS) $(TRX_CFLAGS)
LDADD = $(ASYNC_CPP_LIBS) $(TRX_LIBS)





