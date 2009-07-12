TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release

SOURCES	+= MainWindow.cpp \
	ComDialog.cpp

FORMS	= MainWindowBase.ui \
	ComDialogBase.ui \
	SettingsDialog.ui

IMAGES	= images/exit.png \
	images/configure.png

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}

TRANSLATIONS = translations/qtel_sv.ts \
	       translations/qtel_de.ts \
	       translations/qtel_tr.ts \
	       translations/qtel_nl.ts \
	       translations/qtel_it.ts \
	       translations/qtel_uk.ts \
	       translations/qtel_es.ts

