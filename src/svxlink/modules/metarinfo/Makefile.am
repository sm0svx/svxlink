# The name and location of the module
plugindir = @PLUGINDIR@
plugin_LTLIBRARIES = ModuleMetarInfo.la

DIST_CPP = \
		ModuleMetarInfo.cpp

DIST_H = \
		ModuleMetarInfo.h

# Sources needed to build the library
ModuleMetarInfo_la_SOURCES = $(DIST_CPP) $(DIST_H)

# Extra compiler flags
ModuleMetarInfo_la_CPPFLAGS = @AM_CPPFLAGS@ @ASYNC_AUDIO_CFLAGS@ \
		@SVXLINK_MODULE_CFLAGS@

ModuleMetarInfo_la_LIBADD = @ASYNC_AUDIO_LIBS@
ModuleMetarInfo_la_LDFLAGS = @AM_LDFLAGS@ -module @LIBTOOL_PLUGIN_OPTIONS@

eventdir = @EVENTDIR@
dist_event_DATA = $(srcdir)/MetarInfo.tcl

moduleconfdir = @MODULECONFDIR@
dist_moduleconf_DATA = $(srcdir)/ModuleMetarInfo.conf

# man pages for section 5
SECTION_5 = \
		ModuleMetarInfo.conf.5

MAN5_COMPRESSED = $(SECTION_5:.5=.5.gz)

compressed5dir = $(mandir)/man5
compressed5_DATA = $(MAN5_COMPRESSED)

BUILT_SOURCES = $(MAN5_COMPRESSED)
CLEANFILES = $(BUILT_SOURCES)

# Files that should be packaged with the sources
EXTRA_DIST = $(SECTION_5) Makefile.default

SUFFIXES = .1 .5 .1.gz .5.gz

.1.1.gz:
	$(AM_V_GEN)$(GZIP) -9 -c $< > $@

.5.5.gz:
	$(AM_V_GEN)$(GZIP) -9 -c $< > $@

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