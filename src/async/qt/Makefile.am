
# The name of the library
lib_LTLIBRARIES = libasyncqt.la

DIST_CPP = \
		AsyncQtApplication.cpp \
		AsyncQtDnsLookupWorker.cpp

DIST_H_MOC_PUBLIC = \
		AsyncQtApplication.h

DIST_H_MOC_PRIVATE = \
		AsyncQtDnsLookupWorker.h \
		AsyncQtTimer.h

DIST_H_MOC = $(DIST_H_MOC_PUBLIC) $(DIST_H_MOC_PRIVATE)

DIST_H_NO_MOC = 

FORMS =

FORMHEADERS = $(FORMS:.ui=.h)
MOC_CPP = $(FORMS:.ui=.moc.cpp) $(DIST_H_MOC:.h=.moc.cpp)
UIC_CPP = $(FORMS:.ui=.uic.cpp)

BUILT_SOURCES = $(FORMHEADERS) $(MOC_CPP) $(UIC_CPP)
CLEANFILES = $(BUILT_SOURCES)

libasyncqt_la_SOURCES = $(DIST_CPP) $(DIST_H_MOC) $(DIST_H_NO_MOC)
nodist_libasyncqt_la_SOURCES = $(FORMHEADERS) $(MOC_CPP) $(UIC_CPP)

# compiler flags
AM_CPPFLAGS += @QT_CFLAGS@ @ASYNC_CORE_CFLAGS@

VERSION = @LIBASYNC_SHLIB_VERSION@

libasyncqt_la_LDFLAGS = -version-number $(VERSION)

# Headers that define the API of this library
pkginclude_HEADERS = $(DIST_H_MOC_PUBLIC) $(DIST_H_NO_MOC)

# Files that should be packaged with the sources
EXTRA_DIST = $(FORMS) Makefile.default


# rules for QT processing
.ui.h:
	$(AM_V_GEN)@QT_UIC@ -o $@ $<

.h.moc.cpp:
	$(AM_V_GEN)@QT_MOC@ -o $@ $<

.ui.uic.cpp:
	$(AM_V_GEN)@QT_UIC@ -o $@ -impl $*.h $<

#.h.uic.cpp:
#	(AM_V_GEN)@QT_UIC@ -o $@ -impl $< $(srcdir)/$(<:.h=.ui)

SUFFIXES = .h .ui .moc.cpp .uic.cpp


# remove the .la file from the installation
install-exec-hook:
	@INSTDIR="$(DESTDIR)@libdir@" ; \
	for lafile in $(lib_LTLIBRARIES) ; \
	do \
	echo "Removing $${lafile} from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${lafile} ; \
	done

# libtool automatically handles uninstalls, unless one removes the .la file
uninstall-hook:
	@INSTDIR="$(DESTDIR)@libdir@" ; \
	SHLIBVER=`echo $(VERSION) | $(AWK) -F\: '{OFS="."; $$1=$$1; print $$0}'` ; \
	for lafile in $(lib_LTLIBRARIES) ; \
	do \
	LIBNAME=`echo $${lafile} | $(AWK) -F. '{NF--; print}'` ; \
	echo "Removing $${LIBNAME}.a from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${LIBNAME}.a ; \
	echo "Removing $${LIBNAME}.so from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${LIBNAME}.so ; \
	echo "Removing $${LIBNAME}.so.$(SHLIBVER) from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${LIBNAME}.so.$${SHLIBVER} ; \
	done


