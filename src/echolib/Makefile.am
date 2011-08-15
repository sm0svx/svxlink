
# The name of the library
lib_LTLIBRARIES = libecholib.la

ECHOLIB_CPP = \
		EchoLinkDirectory.cpp \
		EchoLinkQso.cpp \
		EchoLinkDispatcher.cpp \
		EchoLinkStationData.cpp \
		rtpacket.cpp

ECHOLIB_H_PUBLIC = \
		EchoLinkDirectory.h \
		EchoLinkDispatcher.h \
		EchoLinkQso.h \
		EchoLinkStationData.h

ECHOLIB_H_PRIVATE = \
		rtp.h \
		rtpacket.h

ECHOLIB_H = $(ECHOLIB_H_PUBLIC) $(ECHOLIB_H_PRIVATE)

# Sources needed to build the library
libecholib_la_SOURCES = $(ECHOLIB_CPP) $(ECHOLIB_H)

pkginclude_HEADERS = $(ECHOLIB_H_PUBLIC)

BUILT_SOURCES = libecholib.la

# Programs to compile
noinst_PROGRAMS = \
		echolib_test \
		EchoLinkDispatcher_demo \
		EchoLinkDirectory_demo \
		EchoLinkQso_demo

echolib_test_SOURCES = \
		echolib_test.cpp \
		EchoLinkQsoTest.cpp \
		EchoLinkQsoTest.h

EchoLinkDispatcher_demo_SOURCES = \
		EchoLinkDispatcher_demo.cpp

EchoLinkDirectory_demo_SOURCES = \
		EchoLinkDirectory_demo.cpp

EchoLinkQso_demo_SOURCES = \
		EchoLinkQso_demo.cpp

AM_CPPFLAGS += @ECHOLIB_CFLAGS@
LDADD = @ECHOLIB_LIBS@

VERSION = @ECHOLIB_SHLIB_VERSION@

libecholib_la_LDFLAGS = -version-number $(VERSION)

EXTRA_DIST = ChangeLog echolib_test.cfg mainpage.dox messages.txt \
		Makefile.default


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
