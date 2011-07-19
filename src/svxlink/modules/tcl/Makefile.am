# Import some general items
include $(top_srcdir)/common.mk

# The name & location of the plugin
plugindir = @PLUGINDIR@
plugin_LTLIBRARIES = ModuleTcl.la

DIST_CPP = \
		ModuleTcl.cpp

DIST_H = \
		ModuleTcl.h

# Sources needed to build the library
ModuleTcl_la_SOURCES	= $(DIST_CPP) $(DIST_H)

# Extra compiler flags
AM_CPPFLAGS += $(ASYNC_AUDIO_CFLAGS) $(SVXLINK_CFLAGS)

ModuleTcl_la_LIBADD = $(ASYNC_AUDIO_LIBS)
ModuleTcl_la_LDFLAGS = $(LIBTOOL_MODULE_OPTIONS)

# Files that should be packaged with the sources
EXTRA_DIST = Makefile.default Tcl.tcl.example ModuleTcl.tcl.example
