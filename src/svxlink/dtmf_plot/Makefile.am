# Import some general items
include $(top_srcdir)/common.mk

# remove the comment for this and for the SOURCES lines to enable compilation
bin_PROGRAMS = dtmf_plot

DIST_CPP = \
		dtmf_plot.cpp \
		DtmfDecoder.cpp

DIST_H_MOC = 

DIST_H_NO_MOC = \
		DtmfDecoder.h

FORMS = 	\
		DtmfPlot.ui

FORMHEADERS = $(FORMS:.ui=.h)
MOC_CPP = $(FORMS:.ui=.moc.cpp) $(DIST_H_MOC:.h=.moc.cpp)
UIC_CPP = $(FORMS:.ui=.uic.cpp)

BUILT_SOURCES = $(FORMHEADERS) $(UIC_CPP) $(MOC_CPP)
CLEANFILES = $(BUILT_SOURCES)

dtmf_plot_SOURCES = $(DIST_CPP) $(DIST_H_MOC) $(DIST_H_NO_MOC)
nodist_dtmf_plot_SOURCES = $(MOC_CPP) $(UIC_CPP)

EXTRA_DIST = $(FORMS) Makefile.default

# Set some flags for compilation and linking
AM_CPPFLAGS += $(ASYNC_AUDIO_CFLAGS) $(ASYNC_QT_CFLAGS)
LDADD = $(ASYNC_AUDIO_LIBS) $(ASYNC_QT_LIBS)


