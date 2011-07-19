# Import some general items
include $(top_srcdir)/common.mk

# Files that should be packaged with the sources
EXTRA_DIST = help.txt help-se.txt Makefile.default

eventdir = @EVENTDIR@
dist_event_DATA = $(srcdir)/TclVoiceMail.tcl

tclmoduledir = @TCLMODULEDIR@
dist_tclmodule_DATA = $(srcdir)/ModuleTclVoiceMail.tcl

moduleconfdir = @MODULECONFDIR@
dist_moduleconf_DATA = $(srcdir)/ModuleTclVoiceMail.conf

siteconfdir = @SITECONFDIR@
dist_siteconf_DATA = $(srcdir)/TclVoiceMail.conf

install-data-hook:
	@MKDIR_P@ --mode=755 $(DESTDIR)@VOICEMAILDIR@
	@CHOWN@ @SVXLINK_USER@.@SVXLINK_GROUP@ $(DESTDIR)@VOICEMAILDIR@

uninstall-hook:
	@RMDIR@ $(DESTDIR)@VOICEMAILDIR@

