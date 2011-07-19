# Import some general items
include $(top_srcdir)/common.mk

# The name of the library
plugindir = @PLUGINDIR@
plugin_LTLIBRARIES = ModuleHelp.la

DIST_CPP = \
		ModuleHelp.cpp

DIST_H = \
		ModuleHelp.h

# Sources needed to build the library
ModuleHelp_la_SOURCES = $(DIST_CPP) $(DIST_H)

# Extra compiler flags
AM_CPPFLAGS += $(ASYNC_AUDIO_CFLAGS) $(SVXLINK_CFLAGS)

ModuleHelp_la_LIBADD = $(ASYNC_AUDIO_LIBS)
ModuleHelp_la_LDFLAGS = $(LIBTOOL_MODULE_OPTIONS)

# Files that should be packaged with the sources
EXTRA_DIST = help.txt help-se.txt Makefile.default

eventdir = @EVENTDIR@
dist_event_DATA = $(srcdir)/Help.tcl

moduleconfdir = @MODULECONFDIR@
dist_moduleconf_DATA = $(srcdir)/ModuleHelp.conf




