
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

RELEASE = @LIBASYNC_SHLIB_VERSION@

libasyncqt_la_LDFLAGS = -release $(RELEASE)

# Headers that define the API of this library
pkginclude_HEADERS = $(DIST_H_MOC_PUBLIC) $(DIST_H_NO_MOC)

# Files that should be packaged with the sources
EXTRA_DIST = $(FORMS) Makefile.default


install-exec-hook:
	@INSTDIR="$(DESTDIR)@libdir@" ; \
	for lafile in $(lib_LTLIBRARIES) ; \
	do \
	echo "Removing $${lafile} from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${lafile} ; \
	done

uninstall-hook:
	@INSTDIR="$(DESTDIR)@libdir@" ; \
	for lafile in $(lib_LTLIBRARIES) ; \
	do \
	LIBBASE=`echo $${lafile} | ${AWK} -F. '/^.*\.la$$/ {print $$1}'` ; \
	echo "Removing $${LIBBASE}.a from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${LIBBASE}.a ; \
	echo "Removing $${LIBBASE}.so from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${LIBBASE}.so ; \
	echo "Removing $${LIBBASE}-$(RELEASE).so from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${LIBBASE}-$(RELEASE).so ; \
	done


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

