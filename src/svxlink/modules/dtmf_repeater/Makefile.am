# Import some general items
include $(top_srcdir)/common.mk

# The name of the library
plugindir = @PLUGINDIR@
plugin_LTLIBRARIES = ModuleDtmfRepeater.la

DIST_CPP = \
		ModuleDtmfRepeater.cpp

DIST_H = \
		ModuleDtmfRepeater.h

# Sources needed to build the library
ModuleDtmfRepeater_la_SOURCES = $(DIST_CPP) $(DIST_H)

# Extra compiler flags
AM_CPPFLAGS += $(ASYNC_AUDIO_CFLAGS) $(SVXLINK_CFLAGS)

ModuleDtmfRepeater_la_LIBADD = $(ASYNC_AUDIO_LIBS)
ModuleDtmfRepeater_la_LDFLAGS = $(LIBTOOL_MODULE_OPTIONS)

# Files that should be packaged with the sources
EXTRA_DIST = help.txt help-se.txt Makefile.default

eventdir = @EVENTDIR@
dist_event_DATA = $(srcdir)/DtmfRepeater.tcl

moduleconfdir = @MODULECONFDIR@
dist_moduleconf_DATA = $(srcdir)/ModuleDtmfRepeater.conf
