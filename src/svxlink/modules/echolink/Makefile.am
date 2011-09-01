
# The name and location of the module
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
ModuleEchoLink_la_CPPFLAGS = @AM_CPPFLAGS@ @TCL_CFLAGS@ @ECHOLIB_CFLAGS@ \
		@LOCATIONINFO_CFLAGS@ @SVXLINK_MODULE_CFLAGS@

ModuleEchoLink_la_LIBADD = @ECHOLIB_LIBS@
ModuleEchoLink_la_LDFLAGS = @AM_LDFLAGS@ -module @LIBTOOL_PLUGIN_OPTIONS@

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