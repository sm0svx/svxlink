SOURCES	+= MainWindow.cpp ComDialog.cpp 
unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}

TRANSLATIONS = translations/qtel_sv.ts

FORMS	= MainWindowBase.ui ComDialogBase.ui SettingsDialog.ui 
IMAGES	= images/filenew images/fileopen images/filesave images/print images/undo images/redo images/editcut images/editcopy images/editpaste images/searchfind images/exit.png images/configure.png 
TEMPLATE	=app
CONFIG	+= qt warn_on release
DBFILE	= qtel.db
LANGUAGE	= C++
