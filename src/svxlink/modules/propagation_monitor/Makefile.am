
tclmoduledir = @TCLMODULEDIR@
dist_tclmodule_DATA = $(srcdir)/ModulePropagationMonitor.tcl

moduleconfdir = @MODULECONFDIR@
dist_moduleconf_DATA = $(srcdir)/ModulePropagationMonitor.conf

eventdir = @EVENTDIR@
dist_event_DATA = $(srcdir)/PropagationMonitor.tcl

siteconfdir = @SITECONFDIR@
dist_siteconf_DATA = .procmailrc

# Files that should be packaged with the sources
EXTRA_DIST = Makefile.default README


install-data-hook:
	-$(INSTALL) -d -m 755 -o @SVXLINK_USER@ -g @SVXLINK_GROUP@ \
		$(DESTDIR)@VHFDXDIR@
	-$(INSTALL) -d -m 755 -o @SVXLINK_USER@ -g @SVXLINK_GROUP@ \
		$(DESTDIR)@DXROBOTDIR@

uninstall-hook:
	-$(RMDIR) $(DESTDIR)@VHFDXDIR@
	-$(RMDIR) $(DESTDIR)@DXROBOTDIR@
