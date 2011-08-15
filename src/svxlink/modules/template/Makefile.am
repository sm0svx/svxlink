# The name and location of the module
plugindir = @PLUGINDIR@
plugin_LTLIBRARIES = ModuleTemplate.la

DIST_CPP = \
		ModuleTemplate.cpp

DIST_H = \
		ModuleTemplate.h

# Sources needed to build the library
ModuleTemplate_la_SOURCES = $(DIST_CPP) $(DIST_H)

# Extra compiler flags
AM_CPPFLAGS += @ASYNC_AUDIO_CFLAGS@ @SVXLINK_MODULE_CFLAGS@

ModuleTemplate_la_LIBADD = @ASYNC_AUDIO_LIBS@
ModuleTemplate_la_LDFLAGS = -module @LIBTOOL_PLUGIN_OPTIONS@

# Files that should be packaged with the sources
EXTRA_DIST = Makefile.default

eventdir = @EVENTDIR@
dist_event_DATA = $(srcdir)/Template.tcl

moduleconfdir = @MODULECONFDIR@
dist_moduleconf_DATA = $(srcdir)/ModuleTemplate.conf





