
bin_PROGRAMS = svxlink

DIST_CPP = \
		AudioSwitchMatrix.cpp \
		CmdParser.cpp \
		EventHandler.cpp \
		Logic.cpp \
		Module.cpp \
		MsgHandler.cpp \
		QsoRecorder.cpp \
		RepeaterLogic.cpp \
		SimplexLogic.cpp \
		svxlink.cpp

DIST_H = \
		AudioSwitchMatrix.h \
		CmdParser.h \
		EventHandler.h \
		Logic.h \
		LogicCmds.h \
		Module.h \
		MsgHandler.h \
		QsoRecorder.h \
		RepeaterLogic.h \
		SimplexLogic.h

EXTRA_DIST = \
		svxlink.conf.in \
		event_test.tcl \
		Makefile.default

svxlink_SOURCES = $(DIST_CPP) $(DIST_H)

DEFINES = -DSITECONFDIR='"@SITECONFDIR@"'

svxlink_CPPFLAGS = @AM_CPPFLAGS@ @TCL_CFLAGS@ @ECHOLIB_CFLAGS@ \
	@LOCATIONINFO_CFLAGS@ @TRX_CFLAGS@ @GSM_CFLAGS@ $(DEFINES)

svxlink_LDADD = @ECHOLIB_LIBS@ @LOCATIONINFO_LIBS@ @TRX_LIBS@
svxlink_LDFLAGS = @AM_LDFLAGS@ -export-dynamic

# includefiles to export, going into $(includedir) as defined by automake
# pkginclude_HEADERS = \
# 		Module.h \
# 		MsgHandler.h \
# 		EventHandler.h

siteconfdir = @SITECONFDIR@
nodist_siteconf_DATA = $(builddir)/svxlink.conf

# this goes into $(pkgdatadir) as defined by automake
dist_pkgdata_DATA = $(srcdir)/events.tcl

eventdir = @EVENTDIR@
dist_event_DATA = \
		$(srcdir)/RepeaterLogic.tcl \
		$(srcdir)/SimplexLogic.tcl \
		$(srcdir)/Module.tcl \
		$(srcdir)/Logic.tcl \
		$(srcdir)/CW.tcl \
		$(srcdir)/SelCall.tcl \
		$(srcdir)/locale.tcl

BUILT_SOURCES = $(builddir)/svxlink.conf
CLEANFILES = $(BUILT_SOURCES)

$(builddir)/svxlink.conf: $(srcdir)/Makefile

substitute_vars = $(SED) \
		-e 's|@PLUGINDIR[@]|@PLUGINDIR@|g' \
		-e 's|@PKGDATADIR[@]|$(pkgdatadir)|g' \
		-e 's|@QSORECORDERDIR[@]|@QSORECORDERDIR@|g'

.in:
	$(AM_V_at)rm -f $(builddir)/$(@F) $(builddir)/$(@F).tmp
	$(AM_V_GEN)$(substitute_vars) $(srcdir)/$< > $(builddir)/$(@F).tmp
	$(AM_V_at)mv $(builddir)/$(@F).tmp $(builddir)/$(@F)

SUFFIXES = .in

install-data-hook:
	-$(INSTALL) -d -m 755 -o @SVXLINK_USER@ -g @SVXLINK_GROUP@ \
		$(DESTDIR)@QSORECORDERDIR@

uninstall-hook:
	-$(RMDIR) $(DESTDIR)@QSORECORDERDIR@

