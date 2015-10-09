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

# Libraries
set(LIBS ${LIBS} echolib asyncqt asyncaudio asynccore)

# Find the GSM codec library and include directory
find_package(GSM REQUIRED)
include_directories(${GSM_INCLUDE_DIR})
set(LIBS ${LIBS} ${GSM_LIBRARY})

# Find Qt5
find_package(Qt5 REQUIRED Core Gui Widgets Network LinguistTools)
include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Qt5Core_LIBRARIES} ${Qt5Gui_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5Network_LIBRARIES})

# Generate targets for moc, uic and rcc
qt5_wrap_cpp(QTHEADERS_MOC ${QTHEADERS})
qt5_wrap_ui(FORMS_HEADERS ${FORMS})
qt5_add_resources(RESOURCES_RCC ${RESOURCES})

# We also need to add the current build directory to the include search path
# to find automatically generated h-files.
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Add targets for version files
set(VERSION_DEPENDS)
string(TOUPPER ${PROG} PROG_VERNAME)
add_version_target(${PROG_VERNAME} VERSION_DEPENDS)

# Compile the application
add_executable(${PROG} ${PRGSRC} ${QTHEADERS_MOC} ${FORMS_HEADERS}
  ${RESOURCES_RCC} ${VERSION_DEPENDS}
  )
target_link_libraries(${PROG} ${LIBS})
set_target_properties(${PROG} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}  
  )

# Install targets
install(TARGETS ${PROG} DESTINATION ${BIN_INSTALL_DIR})
install(FILES connect.raw DESTINATION ${SHARE_INSTALL_PREFIX}/qtel/sounds)
install(FILES qtel.desktop DESTINATION ${SHARE_INSTALL_PREFIX}/applications)
install(FILES images/link.xpm DESTINATION ${SHARE_INSTALL_PREFIX}/icons)

# Build the translations subdirectory
add_subdirectory(translations)

