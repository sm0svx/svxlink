
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
		Tx.cpp \
		Tx.h \
		event_test.tcl \
		Makefile.default

svxlink_SOURCES = $(DIST_CPP) $(DIST_H)

AM_CPPFLAGS += @TCL_CFLAGS@ @ECHOLIB_CFLAGS@ @LOCATIONINFO_CFLAGS@ @TRX_CFLAGS@

svxlink_LDADD = @ECHOLIB_LIBS@ @LOCATIONINFO_LIBS@ @TRX_LIBS@

# includefiles to export, going into $(includedir) as defined by automake
# pkginclude_HEADERS = \
# 		Module.h \
# 		MsgHandler.h \
# 		EventHandler.h

siteconfdir = @SITECONFDIR@
dist_siteconf_DATA = $(srcdir)/svxlink.conf

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

install-data-hook:
	$(INSTALL) -d -m 755 -o @SVXLINK_USER@ -g @SVXLINK_GROUP@ \
		$(DESTDIR)@QSORECORDERDIR@

uninstall-hook:
	$(RMDIR) $(DESTDIR)@QSORECORDERDIR@

