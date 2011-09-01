# The name and location of the module
plugindir = @PLUGINDIR@
plugin_LTLIBRARIES = ModuleDtmfRepeater.la

DIST_CPP = \
		ModuleDtmfRepeater.cpp

DIST_H = \
		ModuleDtmfRepeater.h

# Sources needed to build the library
ModuleDtmfRepeater_la_SOURCES = $(DIST_CPP) $(DIST_H)

# Extra compiler flags
ModuleDtmfRepeater_la_CPPFLAGS = @AM_CPPFLAGS@ @ASYNC_AUDIO_CFLAGS@ \
		@SVXLINK_MODULE_CFLAGS@

ModuleDtmfRepeater_la_LIBADD = @ASYNC_AUDIO_LIBS@
ModuleDtmfRepeater_la_LDFLAGS = @AM_LDFLAGS@ -module @LIBTOOL_PLUGIN_OPTIONS@

# Files that should be packaged with the sources
EXTRA_DIST = help.txt help-se.txt Makefile.default

eventdir = @EVENTDIR@
dist_event_DATA = $(srcdir)/DtmfRepeater.tcl

moduleconfdir = @MODULECONFDIR@
dist_moduleconf_DATA = $(srcdir)/ModuleDtmfRepeater.conf

# this must be the 'data' hook (based on install path)
install-data-hook:
	$(AM_V_at)INSTDIR="$(DESTDIR)$(plugindir)" ; \
	for lafile in $(plugin_LTLIBRARIES) ; \
	do \
	echo "Removing $${lafile} from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${lafile} ; \
	done

uninstall-hook:
	-$(AM_V_at)INSTDIR="$(DESTDIR)$(plugindir)" ; \
	for lafile in $(plugin_LTLIBRARIES) ; \
	do \
	LIBBASE=`echo $${lafile} | ${AWK} -F. '/^.*\.la$$/ {print $$1}'` ; \
	echo "Removing $${LIBBASE}.so from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${LIBBASE}.so ; \
	done