
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

RELEASE = @LIBASYNC_SHLIB_VERSION@

libasynccpp_la_LDFLAGS = -release $(RELEASE)

# Headers that define the API of this library
pkginclude_HEADERS = $(DIST_H_PUBLIC)

# Files that should be packaged with the sources
EXTRA_DIST = Makefile.default


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
