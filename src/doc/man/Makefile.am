
# man pages for section 1
SECTION_1 = \
		svxlink.1 \
		remotetrx.1 \
		siglevdetcal.1 \
		qtel.1

# man pages for section 5
SECTION_5 = \
		svxlink.conf.5 \
		ModuleHelp.conf.5  \
		ModuleParrot.conf.5 \
		ModuleEchoLink.conf.5 \
		ModuleTclVoiceMail.conf.5 \
		ModuleDtmfRepeater.conf.5 \
		ModulePropagationMonitor.conf.5 \
		ModuleSelCallEnc.conf.5 \
		remotetrx.conf.5

MAN1_COMPRESSED = $(SECTION_1:.1=.1.gz)
MAN5_COMPRESSED = $(SECTION_5:.5=.5.gz)

#dist_man1_MANS = $(SECTION_1)
#dist_man5_MANS = $(SECTION_5)

compressed1dir = $(mandir)/man1
compressed1_DATA = $(MAN1_COMPRESSED)
compressed5dir = $(mandir)/man5
compressed5_DATA = $(MAN5_COMPRESSED)

BUILT_SOURCES = $(MAN1_COMPRESSED) $(MAN5_COMPRESSED)

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = $(SECTION_1) $(SECTION_5) Makefile.default

SUFFIXES = .1 .5 .1.gz .5.gz

.1.1.gz:
	$(AM_V_GEN)$(GZIP) -9 -c $< > $@

.5.5.gz:
	$(AM_V_GEN)$(GZIP) -9 -c $< > $@
