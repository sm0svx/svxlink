# Import some general items
include $(top_srcdir)/common.mk

# The name & location of the plugin library
plugindir = @PLUGINDIR@
plugin_LTLIBRARIES = ModuleTemplate.la

DIST_CPP = \
		ModuleTemplate.cpp

DIST_H = \
		ModuleTemplate.h

# Sources needed to build the library
ModuleTemplate_la_SOURCES = $(DIST_CPP) $(DIST_H)

# Extra compiler flags
AM_CPPFLAGS += $(ASYNC_AUDIO_CFLAGS) $(SVXLINK_CFLAGS)

ModuleTemplate_la_LIBADD = $(ASYNC_AUDIO_LIBS)
ModuleTemplate_la_LDFLAGS = $(LIBTOOL_MODULE_OPTIONS)

# Files that should be packaged with the sources
EXTRA_DIST = Makefile.default

eventdir = @EVENTDIR@
dist_event_DATA = $(srcdir)/Template.tcl

moduleconfdir = @MODULECONFDIR@
dist_moduleconf_DATA = $(srcdir)/ModuleTemplate.conf





