
tclmoduledir = @TCLMODULEDIR@
dist_tclmodule_DATA = $(srcdir)/ModuleTclVoiceMail.tcl

moduleconfdir = @MODULECONFDIR@
dist_moduleconf_DATA = $(srcdir)/ModuleTclVoiceMail.conf

eventdir = @EVENTDIR@
dist_event_DATA = $(srcdir)/TclVoiceMail.tcl

siteconfdir = @SITECONFDIR@
dist_siteconf_DATA = $(srcdir)/TclVoiceMail.conf

# Files that should be packaged with the sources
EXTRA_DIST = help.txt help-se.txt Makefile.default


install-data-hook:
	-$(INSTALL) -d -m 755 -o @SVXLINK_USER@ -g @SVXLINK_GROUP@ \
		$(DESTDIR)@VOICEMAILDIR@

uninstall-hook:
	-$(RMDIR) $(DESTDIR)@VOICEMAILDIR@

