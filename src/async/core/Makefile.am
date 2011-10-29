
# The name of the library
lib_LTLIBRARIES	= libasynccore.la

DIST_CPP = \
		AsyncApplication.cpp \
		AsyncConfig.cpp \
		AsyncDnsLookup.cpp \
		AsyncFdWatch.cpp \
		AsyncIpAddress.cpp \
		AsyncSerial.cpp \
		AsyncSerialDevice.cpp \
		AsyncTcpClient.cpp \
		AsyncTcpConnection.cpp \
		AsyncTcpServer.cpp \
		AsyncTimer.cpp \
		AsyncUdpSocket.cpp

DIST_H_PUBLIC = \
		AsyncApplication.h \
		AsyncConfig.h \
		AsyncDnsLookup.h \
		AsyncFdWatch.h \
		AsyncIpAddress.h \
		AsyncSerial.h \
		AsyncTcpClient.h \
		AsyncTcpConnection.h \
		AsyncTcpServer.h \
		AsyncTimer.h \
		AsyncUdpSocket.h

DIST_H_PRIVATE = \
		AsyncDnsLookupWorker.h \
		AsyncSerialDevice.h

DIST_H = $(DIST_H_PUBLIC) $(DIST_H_PRIVATE)

# Sources needed to build the library
libasynccore_la_SOURCES	= $(DIST_CPP) $(DIST_H)

SHLIB_RELEASE = @LIBASYNC_SHLIB_RELEASE@
SHLIB_INTERFACE = @LIBASYNC_SHLIB_INTERFACE@

# Extra compiler & linker flags
libasynccore_la_CPPFLAGS = @AM_CPPFLAGS@ @ASYNC_CORE_CFLAGS@
libasynccore_la_LIBADD = @SIGC_LIBS@
libasynccore_la_LDFLAGS = @AM_LDFLAGS@ -version-info $(SHLIB_INTERFACE)

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
