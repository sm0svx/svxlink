TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release

SOURCES	+= MainWindow.cpp \
	ComDialog.cpp

FORMS	= MainWindowBase.ui \
	ComDialogBase.ui \
	SettingsDialog.ui

IMAGES	= images/exit.png \
	images/configure.png \
	images/033.xpm \
	images/035.xpm \
	images/036.xpm \
	images/038.xpm \
	images/045.xpm \
	images/096.xpm \
	images/092.xpm \
	images/120.xpm \
	images/040.xpm \
	images/045.xpm \
	images/048.xpm 
	

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}

TRANSLATIONS = translations/qtel_sv.ts \
	       translations/qtel_de.ts \
	       translations/qtel_it.ts \
	       translations/qtel_es.ts

