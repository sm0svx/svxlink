
BUILD_ALWAYS = \
		core \
		cpp \
		audio

BUILD_WITH_QT = \
		qt

if HAVE_QT
MAYBE_QT = $(BUILD_WITH_QT)
endif

DEMO = \
		demo

# demo needs to be built after QT, if applicable
SUBDIRS = $(BUILD_ALWAYS) $(MAYBE_QT) $(DEMO)

DIST_SUBDIRS = $(BUILD_ALWAYS) $(BUILD_WITH_QT) $(DEMO)

EXTRA_DIST = ChangeLog mainpage.dox makefile.dirs
