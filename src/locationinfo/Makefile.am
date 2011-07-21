
# The name of the library
noinst_LIBRARIES = liblocationinfo.a

DIST_CPP = \
		AprsTcpClient.cpp \
		AprsUdpClient.cpp \
		LocationInfo.cpp

DIST_H = \
		AprsClient.h \
		AprsTcpClient.h \
		AprsUdpClient.h \
		LocationInfo.h

# Sources needed to build the library
liblocationinfo_a_SOURCES = $(DIST_CPP) $(DIST_H)

# Extra compiler flags
AM_CPPFLAGS += @ECHOLIB_CFLAGS@

# Headers that define the API of this library
#pkginclude_HEADERS = $(DIST_H)

# Files that should be packaged with the sources
EXTRA_DIST = Makefile.default
