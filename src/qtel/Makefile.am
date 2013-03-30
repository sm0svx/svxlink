
SUBDIRS = translations

# C++ programs to compile
bin_PROGRAMS = qtel

DIST_CPP = \
		qtel.cpp \
		MainWindow.cpp \
		ComDialog.cpp \
		Settings.cpp \
		EchoLinkDirectoryModel.cpp \
		MsgHandler.cpp \
		Vox.cpp

DIST_H_MOC = \
		MainWindow.h \
		ComDialog.h \
		SettingsDialog.h \
		EchoLinkDirectoryModel.h \
		MyMessageBox.h \
		Vox.h

DIST_H_NO_MOC = \
		MsgHandler.h \
		Settings.h \
		multirate_filter_coeff.h

DIST_QRC =	qtel.qrc

DIST_FORMS = 	MainWindowBase.ui \
		ComDialogBase.ui \
		SettingsDialogBase.ui

UIC_H = $(DIST_FORMS:.ui=.h)
MOC_CPP = $(DIST_H_MOC:.h=.moc.cpp)
RCC_CPP = $(DIST_QRC:.qrc=.qrc.cpp)

BUILT_SOURCES = $(UIC_H) $(MOC_CPP) $(RCC_CPP)
CLEANFILES = $(BUILT_SOURCES)

qtel_SOURCES = $(DIST_CPP) $(DIST_H_MOC) $(DIST_H_NO_MOC) $(DIST_QRC)
nodist_qtel_SOURCES = $(MOC_CPP) $(RCC_CPP)

# symbol definitions
DEFINES = -DQTELMESSAGEDIR='"@QTELMESSAGEDIR@"' \
		-DQTELSOUNDDIR='"@QTELSOUNDDIR@"'

# Set some flags for compilation and linking
# FIXME - why do we need -I$builddir?
qtel_CPPFLAGS = @AM_CPPFLAGS@ @ECHOLIB_CFLAGS@ @ASYNC_QT_CFLAGS@ \
		$(DEFINES) -I$(builddir)
qtel_LDADD = @ECHOLIB_LIBS@ @ASYNC_QT_LIBS@
qtel_LDFLAGS = @AM_LDFLAGS@

EXTRA_DIST = \
		$(DIST_FORMS) \
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
	$(AM_V_GEN)@QT_UIC@ -o $(builddir)/$@ $(srcdir)/$<

.h.moc.cpp:
	$(AM_V_GEN)@QT_MOC@ -o $(builddir)/$@ $(srcdir)/$<

.qrc.qrc.cpp:
	$(AM_V_GEN)@QT_RCC@ -o $(builddir)/$@ $(srcdir)/$<

SUFFIXES = .h .ui .qrc .moc.cpp .qrc.cpp
