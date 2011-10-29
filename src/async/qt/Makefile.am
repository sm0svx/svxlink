
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

DIST_QRC =

DIST_FORMS =

UIC_H = $(DIST_FORMS:.ui=.h)
MOC_CPP = $(DIST_H_MOC:.h=.moc.cpp)
RCC_CPP = $(DIST_QRC:.qrc=.qrc.cpp)

BUILT_SOURCES = $(UIC_H) $(MOC_CPP) $(RCC_CPP)
CLEANFILES = $(BUILT_SOURCES)

libasyncqt_la_SOURCES = $(DIST_CPP) $(DIST_H_MOC) $(DIST_H_NO_MOC) $(DIST_QRC)
nodist_libasyncqt_la_SOURCES = $(UIC_H) $(MOC_CPP) $(RCC_CPP)

SHLIB_RELEASE = @LIBASYNC_SHLIB_RELEASE@
SHLIB_INTERFACE = @LIBASYNC_SHLIB_INTERFACE@

# compiler flags
libasyncqt_la_CPPFLAGS = @AM_CPPFLAGS@ @ASYNC_QT_CFLAGS@
libasyncqt_la_LIBADD = @ASYNC_CORE_LIBS@ @QT_LIBS@
libasyncqt_la_LDFLAGS = @AM_LDFLAGS@ -version-info $(SHLIB_INTERFACE)

# Headers that define the API of this library
pkginclude_HEADERS = $(DIST_H_MOC_PUBLIC) $(DIST_H_NO_MOC)

# Files that should be packaged with the sources
EXTRA_DIST = $(DIST_FORMS) Makefile.default


# rules for QT processing
.ui.h:
	$(AM_V_GEN)@QT_UIC@ -o $(builddir)/$@ $(srcdir)/$<

.h.moc.cpp:
	$(AM_V_GEN)@QT_MOC@ -o $(builddir)/$@ $(srcdir)/$<

.qrc.qrc.cpp:
	$(AM_V_GEN)@QT_RCC@ -o $(builddir)/$@ $(srcdir)/$<

SUFFIXES = .h .ui .qrc .moc.cpp .qrc.cpp


# remove the .la file from the installation
install-exec-hook:
	$(AM_V_at)INSTDIR="$(DESTDIR)@libdir@" ; \
	for lafile in $(lib_LTLIBRARIES) ; \
	do \
	echo "Removing $${lafile} from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${lafile} ; \
	done

# libtool automatically handles uninstalls, unless one removes the .la file
uninstall-hook:
	-$(AM_V_at)INSTDIR="$(DESTDIR)@libdir@" ; \
	SHLIBREL=`echo $(SHLIB_RELEASE) | $(AWK) -F\: '{OFS="."; $$1=$$1; print $$0}'` ; \
	SHLIBIFC=`echo $(SHLIB_INTERFACE) | $(AWK) -F\: '{OFS="."; $$1=$$1; print $$0}'` ; \
	for lafile in $(lib_LTLIBRARIES) ; \
	do \
	LIBNAME=`echo $${lafile} | $(AWK) -F. '{NF--; print}'` ; \
	for binary in $${INSTDIR}/$${LIBNAME}.a $${INSTDIR}/$${LIBNAME}.so $${INSTDIR}/$${LIBNAME}.so.$${SHLIBIFC} ; \
	do \
	echo "Removing $${binary}" ; \
	$(RM) $${binary} ; \
	done ; \
	done
