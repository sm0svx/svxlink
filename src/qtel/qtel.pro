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

TRANSLATIONS = translations/qtel_sv.ts

