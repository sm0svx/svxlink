# Import some general items
include $(top_srcdir)/common.mk

# Files that should be packaged with the sources
EXTRA_DIST = Makefile.default README

eventdir = @EVENTDIR@
dist_event_DATA = $(srcdir)/PropagationMonitor.tcl

tclmoduledir = @TCLMODULEDIR@
dist_tclmodule_DATA = $(srcdir)/ModulePropagationMonitor.tcl

moduleconfdir = @MODULECONFDIR@
dist_moduleconf_DATA = $(srcdir)/ModulePropagationMonitor.conf

siteconfdir = @SITECONFDIR@
dist_siteconf_DATA = .procmailrc