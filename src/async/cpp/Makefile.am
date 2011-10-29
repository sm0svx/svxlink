
# The name of the library
lib_LTLIBRARIES = libasynccpp.la

DIST_CPP = \
		AsyncCppApplication.cpp \
		AsyncCppDnsLookupWorker.cpp

DIST_H_PUBLIC = \
		AsyncCppApplication.h

DIST_H_PRIVATE = \
		AsyncCppDnsLookupWorker.h

DIST_H = $(DIST_H_PUBLIC) $(DIST_H_PRIVATE)

# Sources needed to build the library
libasynccpp_la_SOURCES = $(DIST_CPP) $(DIST_H)

SHLIB_RELEASE = @LIBASYNC_SHLIB_RELEASE@
SHLIB_INTERFACE = @LIBASYNC_SHLIB_INTERFACE@

# Extra compiler & linker flags
libasynccpp_la_CPPFLAGS = @AM_CPPFLAGS@ @ASYNC_CPP_CFLAGS@
libasynccpp_la_LIBADD = @ASYNC_CORE_LIBS@ @PTHREAD_LIBS@
libasynccpp_la_LDFLAGS = @AM_LDFLAGS@ -version-info $(SHLIB_INTERFACE)

# Headers that define the API of this library
pkginclude_HEADERS = $(DIST_H_PUBLIC)

# Files that should be packaged with the sources
EXTRA_DIST = Makefile.default


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
