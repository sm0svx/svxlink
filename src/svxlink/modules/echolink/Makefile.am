# Import some general items
include $(top_srcdir)/common.mk

# The name of the library
plugindir = @PLUGINDIR@
plugin_LTLIBRARIES = ModuleEchoLink.la

DIST_CPP = \
		AprsTcpClient.cpp \
		AprsUdpClient.cpp \
		LocationInfo.cpp \
		ModuleEchoLink.cpp \
		QsoImpl.cpp

DIST_H = \
		AprsClient.h \
		AprsTcpClient.h \
		AprsUdpClient.h \
		LocationInfo.h \
		ModuleEchoLink.h \
		multirate_filter_coeff.h \
		QsoImpl.h

# Sources needed to build the library
ModuleEchoLink_la_SOURCES = $(DIST_CPP) $(DIST_H)

# Extra compiler flags
AM_CPPFLAGS += $(TCL_CFLAGS) $(ECHOLIB_CFLAGS) $(LOCATIONINFO_CFLAGS) \
		$(SVXLINK_CFLAGS)

ModuleEchoLink_la_LIBADD = $(TCL_LIBS) $(ECHOLIB_LIBS) $(LOCATIONINFO_LIBS)
ModuleEchoLink_la_LDFLAGS = $(LIBTOOL_MODULE_OPTIONS)

# Files that should be packaged with the sources
EXTRA_DIST = \
		Makefile.default \
		conf_info_msg.txt \
		timeout_chat_msg.txt \
		help.txt \
		help-se.txt

eventdir = @EVENTDIR@
dist_event_DATA = $(srcdir)/EchoLink.tcl

moduleconfdir = @MODULECONFDIR@
dist_moduleconf_DATA = $(srcdir)/ModuleEchoLink.conf




