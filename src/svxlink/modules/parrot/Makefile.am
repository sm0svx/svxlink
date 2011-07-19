# Import some general items
include $(top_srcdir)/common.mk

# The name of the library
plugindir = @PLUGINDIR@
plugin_LTLIBRARIES = ModuleParrot.la

DIST_CPP = \
		ModuleParrot.cpp

DIST_H = \
		ModuleParrot.h

# Sources needed to build the library
ModuleParrot_la_SOURCES = $(DIST_CPP) $(DIST_H)

# Extra compiler flags
AM_CPPFLAGS += $(ASYNC_AUDIO_CFLAGS) $(SVXLINK_CFLAGS)

ModuleParrot_la_LIBADD = $(ASYNC_AUDIO_LIBS)
ModuleParrot_la_LDFLAGS = $(LIBTOOL_MODULE_OPTIONS)

# Files that should be packaged with the sources
EXTRA_DIST = help.txt help-se.txt Makefile.default

eventdir = @EVENTDIR@
dist_event_DATA = $(srcdir)/Parrot.tcl

moduleconfdir = @MODULECONFDIR@
dist_moduleconf_DATA = $(srcdir)/ModuleParrot.conf
