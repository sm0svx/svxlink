
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

SHLIB_RELEASE = @ECHOLIB_SHLIB_RELEASE@
SHLIB_INTERFACE = @ECHOLIB_SHLIB_INTERFACE@

# the next three lines work for the programs
AM_CPPFLAGS = @AM_CPPFLAGS@ @ECHOLIB_CFLAGS@
AM_LDFLAGS = @AM_LDFLAGS@
LDADD = @ECHOLIB_LIBS@
libecholib_la_LIBADD = @ASYNC_CPP_LIBS@ @ASYNC_AUDIO_LIBS@
libecholib_la_LDFLAGS = @AM_LDFLAGS@ -release $(SHLIB_RELEASE) \
		-version-info $(SHLIB_INTERFACE)

EXTRA_DIST = ChangeLog echolib_test.cfg mainpage.dox messages.txt \
		Makefile.default


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
	for binary in $${INSTDIR}/$${LIBNAME}.a $${INSTDIR}/$${LIBNAME}-$${SHLIBREL}.so $${INSTDIR}/$${LIBNAME}-$${SHLIBREL}.so.$${SHLIBIFC} ; \
	do \
	echo "Removing $${binary}" ; \
	$(RM) $${binary} ; \
	done ; \
	done
