
# required to access local macros in the m4 directory
ACLOCAL_AMFLAGS = -I m4

BUILD_ALWAYS = \
		version \
		async \
		echolib \
		locationinfo \
		svxlink \
		misc \
		doc

BUILD_WITH_QT = \
		qtel

if HAVE_QT
MAYBE_QTEL = $(BUILD_WITH_QT)
endif

SUBDIRS = $(BUILD_ALWAYS) $(MAYBE_QTEL)

DIST_SUBDIRS = $(BUILD_ALWAYS) $(BUILD_WITH_QT)

EXTRA_DIST = \
		avr \
		versions \
		COPYRIGHT \
		create_config.sh \
		doxygen.async \
		doxygen.echolib \
		makefile.cfg \
		makefile.dirs \
		makefile.inc \
		Makefile.orig \
		makefile.root \
		makefile.sub \
		Makefile.template \
		template.cpp \
		template.h


# this automatically keeps the latest libtool version in-tree
LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool
