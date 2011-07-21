
bin_PROGRAMS = remotetrx

DIST_CPP = \
		NetTrxAdapter.cpp \
		NetUplink.cpp \
		RfUplink.cpp \
		TrxHandler.cpp \
		Uplink.cpp \
		remotetrx.cpp

DIST_H = \
		NetTrxAdapter.h \
		NetUplink.h \
		RfUplink.h \
		TrxHandler.h \
		Uplink.h

EXTRA_DIST = Makefile.default

remotetrx_SOURCES = $(DIST_CPP) $(DIST_H)

AM_CPPFLAGS += @ASYNC_CPP_CFLAGS@ @TRX_CFLAGS@
LDADD = @ASYNC_CPP_LIBS@ @TRX_LIBS@

siteconfdir = @SITECONFDIR@
dist_siteconf_DATA = remotetrx.conf





