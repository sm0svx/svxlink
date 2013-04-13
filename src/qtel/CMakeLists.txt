# The program name
set(PROG qtel)

# Program source
set(PRGSRC qtel.cpp MainWindow.cpp ComDialog.cpp Settings.cpp MsgHandler.cpp
	   Vox.cpp EchoLinkDirectoryModel.cpp)

# Header files that need to be run through moc
set(QTHEADERS MainWindow.h ComDialog.h MyMessageBox.h Vox.h
	      SettingsDialog.h EchoLinkDirectoryModel.h)

# Forms that need to be run through uic
set(FORMS MainWindowBase.ui ComDialogBase.ui SettingsDialogBase.ui)            

# Resource files
set(RESOURCES qtel.qrc)

# Language translations
set(TRANSLATIONS
  translations/qtel_sv.qm
  translations/qtel_de.qm
  translations/qtel_tr.qm
  translations/qtel_nl.qm
  translations/qtel_it.qm
  translations/qtel_uk.qm
  translations/qtel_ru.qm
  translations/qtel_hu.qm
  translations/qtel_ja.qm
  translations/qtel_fr.qm
  translations/qtel_es.qm
  )

# Libraries
set(LIBS ${LIBS} echolib asyncqt asyncaudio asynccore)

# Find Qt4
find_package(Qt4 4.2 REQUIRED QtCore QtGui QtNetwork)
include(${QT_USE_FILE})
set(LIBS ${LIBS} ${QT_LIBRARIES})

# Generate targets for moc, uic, rcc and translations
QT4_WRAP_CPP(QTHEADERS_MOC ${QTHEADERS})
QT4_WRAP_UI(FORMS_HEADERS ${FORMS})
QT4_ADD_RESOURCES(RESOURCES_RCC ${RESOURCES})
QT4_ADD_TRANSLATION(TRANSLATIONS_QM ${TRANSLATIONS})

# Compile the application
string(TOUPPER ${PROG} PROG_VERNAME)
add_executable(${PROG} ${PRGSRC} ${QTHEADERS_MOC} ${FORMS_HEADERS}
  ${RESOURCES_RCC} ${TRANSLATIONS_QM}
  ${CMAKE_CURRENT_BINARY_DIR}/version/${PROG_VERNAME}.h
  )
target_link_libraries(${PROG} ${LIBS})
set_target_properties(${PROG} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}  
  )
add_version_target(${PROG_VERNAME})

# Install targets
install(TARGETS ${PROG} DESTINATION ${BIN_INSTALL_DIR})
install(FILES connect.raw DESTINATION ${SVX_SHARE_INSTALL_DIR}/qtel/sounds)
install(FILES qtel.desktop DESTINATION ${SVX_SHARE_INSTALL_DIR}/applications)
install(FILES images/link.xpm DESTINATION ${SVX_SHARE_INSTALL_DIR}/icons)
install(FILES ${TRANSLATIONS}
  DESTINATION ${SVX_SHARE_INSTALL_DIR}/qtel/translations
  )
