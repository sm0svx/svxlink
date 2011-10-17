TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release

HEADERS += ComDialog.h MainWindow.h MyMessageBox.h Settings.h \
	EchoLinkDirectoryModel.h MsgHandler.h SettingsDialog.h Vox.h

SOURCES	+= MainWindow.cpp \
	ComDialog.cpp \
	Settings.cpp \
	EchoLinkDirectoryModel.cpp

FORMS	= MainWindowBase.ui \
	ComDialogBase.ui \
	SettingsDialogBase.ui

IMAGES	= images/exit.png \
	images/configure.png

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}

TRANSLATIONS = translations/qtel_tmpl.ts \
	       translations/qtel_sv.ts \
	       translations/qtel_de.ts \
	       translations/qtel_tr.ts \
	       translations/qtel_nl.ts \
	       translations/qtel_it.ts \
	       translations/qtel_uk.ts \
	       translations/qtel_ru.ts \
	       translations/qtel_hu.ts \
	       translations/qtel_ja.ts \
	       translations/qtel_fr.ts \
	       translations/qtel_es.ts

