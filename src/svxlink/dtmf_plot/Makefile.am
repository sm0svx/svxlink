
# remove the comment for this and for the SOURCES lines to enable compilation
bin_PROGRAMS = dtmf_plot

DIST_CPP = \
		dtmf_plot.cpp \
		DtmfDecoder.cpp

DIST_H_MOC = 

DIST_H_NO_MOC = \
		DtmfDecoder.h

DIST_QRC =

DIST_FORMS = 	\
		DtmfPlot.ui

UIC_H = $(DIST_FORMS:.ui=.h)
MOC_CPP = $(DIST_H_MOC:.h=.moc.cpp)
RCC_CPP = $(DIST_QRC:.qrc=.qrc.cpp)

BUILT_SOURCES = $(UIC_H) $(MOC_CPP) $(RCC_CPP)
CLEANFILES = $(BUILT_SOURCES)

dtmf_plot_SOURCES = $(DIST_CPP) $(DIST_H_MOC) $(DIST_H_NO_MOC) $(DIST_QRC)
nodist_dtmf_plot_SOURCES = $(MOC_CPP) $(RCC_CPP)

EXTRA_DIST = $(DIST_FORMS) Makefile.default

# Set some flags for compilation and linking
AM_CPPFLAGS += @ASYNC_AUDIO_CFLAGS@ @ASYNC_QT_CFLAGS@
LDADD = @ASYNC_AUDIO_LIBS@ @ASYNC_QT_LIBS@

# rules for QT processing
.ui.h:
	$(AM_V_GEN)@QT_UIC@ -o $(builddir)/$@ $(srcdir)/$<

.h.moc.cpp:
	$(AM_V_GEN)@QT_MOC@ -o $(builddir)/$@ $(srcdir)/$<

.qrc.qrc.cpp:
	$(AM_V_GEN)@QT_RCC@ -o $(builddir)/$@ $(srcdir)/$<

SUFFIXES = .h .ui .qrc .moc.cpp .qrc.cpp
