# Find the popt library
find_package(Popt REQUIRED)
set(LIBS ${LIBS} ${POPT_LIBRARIES})
include_directories(${POPT_INCLUDE_DIRS})
add_definitions(${POPT_DEFINITIONS})

# Find the GSM codec library
find_package(GSM REQUIRED)
set(LIBS ${LIBS} ${GSM_LIBRARY})
include_directories(${GSM_INCLUDE_DIR})

# Find the TCL library
find_package(TCL)
if(NOT TCL_FOUND)
  message(FATAL_ERROR "TCL not found")
endif(NOT TCL_FOUND)
set(LIBS ${LIBS} ${TCL_LIBRARY})
include_directories(${TCL_INCLUDE_PATH})

# Find the GCrypt library
find_package(GCrypt REQUIRED)
set(LIBS ${LIBS} ${GCRYPT_LIBRARIES})
include_directories(${GCRYPT_INCLUDE_DIRS})
add_definitions(${GCRYPT_DEFINITIONS})

# Find the dl library
find_package(DL REQUIRED)
set(LIBS ${LIBS} ${DL_LIBRARIES})
include_directories(${DL_INCLUDES})

# Add project libraries
set(LIBS ${LIBS} trx locationinfo asynccpp asyncaudio asynccore)

# Add targets for version files
set(VERSION_DEPENDS)
add_version_target(SVXLINK VERSION_DEPENDS)

# Build the executable
add_executable(svxlink
  MsgHandler.cpp Module.cpp Logic.cpp SimplexLogic.cpp RepeaterLogic.cpp
  EventHandler.cpp LinkManager.cpp CmdParser.cpp QsoRecorder.cpp svxlink.cpp
  DtmfDigitHandler.cpp
  ${VERSION_DEPENDS}
)
target_link_libraries(svxlink ${LIBS})
set_target_properties(svxlink PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}
)

# Generate config file with correct paths
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/svxlink.conf.in
  ${CMAKE_CURRENT_BINARY_DIR}/svxlink.conf
  @ONLY
  )

# Install targets
install(TARGETS svxlink DESTINATION ${BIN_INSTALL_DIR})
install_mkdir(${SVX_SYSCONF_INSTALL_DIR}/svxlink.d)
install_mkdir(${SVX_SPOOL_INSTALL_DIR}/qso_recorder svxlink.daemon)
install_mkdir(${SVX_SHARE_INSTALL_DIR}/sounds)
install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/svxlink.conf
  ${SVX_SYSCONF_INSTALL_DIR}
  )
install(FILES events.tcl DESTINATION ${SVX_SHARE_INSTALL_DIR})
install(FILES RepeaterLogic.tcl SimplexLogic.tcl Module.tcl Logic.tcl CW.tcl
  SelCall.tcl locale.tcl
  DESTINATION ${SVX_SHARE_INSTALL_DIR}/events.d
  )
