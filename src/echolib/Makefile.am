
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

RELEASE = @ECHOLIB_SHLIB_VERSION@

libecholib_la_LDFLAGS = -release $(RELEASE)

EXTRA_DIST = ChangeLog echolib_test.cfg mainpage.dox messages.txt \
		Makefile.default


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
