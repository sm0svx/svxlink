
tclmoduledir = @TCLMODULEDIR@
dist_tclmodule_DATA = $(srcdir)/ModuleTclVoiceMail.tcl

moduleconfdir = @MODULECONFDIR@
dist_moduleconf_DATA = $(srcdir)/ModuleTclVoiceMail.conf

eventdir = @EVENTDIR@
dist_event_DATA = $(srcdir)/TclVoiceMail.tcl

siteconfdir = @SITECONFDIR@
nodist_siteconf_DATA = $(builddir)/TclVoiceMail.conf

# Files that should be packaged with the sources
EXTRA_DIST = $(srcdir)/TclVoiceMail.conf.in help.txt help-se.txt Makefile.default

BUILT_SOURCES = $(builddir)/TclVoiceMail.conf
CLEANFILES = $(BUILT_SOURCES)

$(builddir)/TclVoiceMail.conf: $(srcdir)/Makefile

substitute_vars = $(SED) \
		-e 's|@VOICEMAILDIR[@]|@VOICEMAILDIR@|g'

.in:
	$(AM_V_at)rm -f $(builddir)/$(@F) $(builddir)/$(@F).tmp
	$(AM_V_GEN)$(substitute_vars) $(srcdir)/$< > $(builddir)/$(@F).tmp
	$(AM_V_at)mv $(builddir)/$(@F).tmp $(builddir)/$(@F)

SUFFIXES = .in

install-data-hook:
	-$(INSTALL) -d -m 755 -o @SVXLINK_USER@ -g @SVXLINK_GROUP@ \
		$(DESTDIR)@VOICEMAILDIR@

uninstall-hook:
	-$(RMDIR) $(DESTDIR)@VOICEMAILDIR@

