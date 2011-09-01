# The name and location of the module
plugindir = @PLUGINDIR@
plugin_LTLIBRARIES = ModuleParrot.la

DIST_CPP = \
		ModuleParrot.cpp

DIST_H = \
		ModuleParrot.h

# Sources needed to build the library
ModuleParrot_la_SOURCES = $(DIST_CPP) $(DIST_H)

# Extra compiler flags
ModuleParrot_la_CPPFLAGS = @AM_CPPFLAGS@ @ASYNC_AUDIO_CFLAGS@ \
		@SVXLINK_MODULE_CFLAGS@

ModuleParrot_la_LIBADD = @ASYNC_AUDIO_LIBS@
ModuleParrot_la_LDFLAGS = @AM_LDFLAGS@ -module @LIBTOOL_PLUGIN_OPTIONS@

# Files that should be packaged with the sources
EXTRA_DIST = help.txt help-se.txt Makefile.default

eventdir = @EVENTDIR@
dist_event_DATA = $(srcdir)/Parrot.tcl

moduleconfdir = @MODULECONFDIR@
dist_moduleconf_DATA = $(srcdir)/ModuleParrot.conf

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