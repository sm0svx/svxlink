# The name and location of the module
plugindir = @PLUGINDIR@
plugin_LTLIBRARIES = ModuleHelp.la

DIST_CPP = \
		ModuleHelp.cpp

DIST_H = \
		ModuleHelp.h

# Sources needed to build the library
ModuleHelp_la_SOURCES = $(DIST_CPP) $(DIST_H)

# Extra compiler flags
AM_CPPFLAGS += @ASYNC_AUDIO_CFLAGS@ @SVXLINK_MODULE_CFLAGS@

ModuleHelp_la_LIBADD = @ASYNC_AUDIO_LIBS@
ModuleHelp_la_LDFLAGS = -module @LIBTOOL_PLUGIN_OPTIONS@

# Files that should be packaged with the sources
EXTRA_DIST = help.txt help-se.txt Makefile.default

eventdir = @EVENTDIR@
dist_event_DATA = $(srcdir)/Help.tcl

moduleconfdir = @MODULECONFDIR@
dist_moduleconf_DATA = $(srcdir)/ModuleHelp.conf

# this must be the 'data' hook (based on install path)
install-data-hook:
	@INSTDIR="$(DESTDIR)$(plugindir)" ; \
	for lafile in $(plugin_LTLIBRARIES) ; \
	do \
	echo "Removing $${lafile} from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${lafile} ; \
	done

uninstall-hook:
	@INSTDIR="$(DESTDIR)$(plugindir)" ; \
	for lafile in $(plugin_LTLIBRARIES) ; \
	do \
	LIBBASE=`echo $${lafile} | ${AWK} -F. '/^.*\.la$$/ {print $$1}'` ; \
	echo "Removing $${LIBBASE}.so from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${LIBBASE}.so ; \
	done