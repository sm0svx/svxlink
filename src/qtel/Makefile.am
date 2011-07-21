
SUBDIRS = translations

# C++ programs to compile
bin_PROGRAMS = qtel

DIST_CPP = \
		qtel.cpp \
		MainWindow.cpp \
		ComDialog.cpp \
		Settings.cpp \
		MsgHandler.cpp \
		Vox.cpp

DIST_H_MOC = \
		MainWindow.h \
		ComDialog.h \
		MyMessageBox.h \
		Vox.h

DIST_H_NO_MOC = \
		MainWindowBase.ui.h \
		SettingsDialog.ui.h \
		MsgHandler.h \
		Settings.h

FORMS = 	MainWindowBase.ui \
		ComDialogBase.ui \
		SettingsDialog.ui

FORMHEADERS = $(FORMS:.ui=.h)
MOC_CPP = $(FORMS:.ui=.moc.cpp) $(DIST_H_MOC:.h=.moc.cpp)
UIC_CPP = $(FORMS:.ui=.uic.cpp)

BUILT_SOURCES = $(FORMHEADERS) $(UIC_CPP) $(MOC_CPP)
CLEANFILES = $(BUILT_SOURCES)

qtel_SOURCES = $(DIST_CPP) $(DIST_H_MOC) $(DIST_H_NO_MOC)
nodist_qtel_SOURCES = $(MOC_CPP) $(UIC_CPP)

# Set some flags for compilation and linking
# FIXME - why do we need -I$builddir here?
AM_CPPFLAGS += @ECHOLIB_CFLAGS@ @ASYNC_QT_CFLAGS@ -I$(builddir)
LDADD = @ECHOLIB_LIBS@ @ASYNC_QT_LIBS@

EXTRA_DIST = \
		$(FORMS) \
		images \
		qtel.pro \
		Makefile.default

sounddir = @QTELSOUNDDIR@
dist_sound_DATA = connect.raw

desktopdir = @DESKTOPDIR@
dist_desktop_DATA = qtel.desktop

icondir = @ICONDIR@
dist_icon_DATA = images/link.xpm


# rules for QT processing
.ui.h:
	$(AM_V_GEN)@QT_UIC@ -o $@ $<

.h.moc.cpp:
	$(AM_V_GEN)@QT_MOC@ -o $@ $<

.ui.uic.cpp:
	$(AM_V_GEN)@QT_UIC@ -o $@ -impl $*.h $<

#.h.uic.cpp:
#	(AM_V_GEN)@QT_UIC@ -o $@ -impl $< $(srcdir)/$(<:.h=.ui)

SUFFIXES = .h .ui .moc.cpp .uic.cpp
