
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

# Extra compiler flags
AM_CPPFLAGS += @ASYNC_CPP_CFLAGS@

VERSION = @LIBASYNC_SHLIB_VERSION@

libasynccpp_la_LDFLAGS = -version-number $(VERSION)

# Headers that define the API of this library
pkginclude_HEADERS = $(DIST_H_PUBLIC)

# Files that should be packaged with the sources
EXTRA_DIST = Makefile.default


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
