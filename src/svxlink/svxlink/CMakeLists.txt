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
if(TCL_LIBRARY)
  set(TCL_LIBRARY_CACHED TRUE)
endif(TCL_LIBRARY)
find_package(TCL QUIET)
if(TCL_FOUND)
  if (NOT TCL_LIBRARY_CACHED)
    message("-- Found TCL: ${TCL_LIBRARY}")
  endif(NOT TCL_LIBRARY_CACHED)
else(TCL_FOUND)
  message(FATAL_ERROR "-- Could NOT find the TCL scripting language")
endif(TCL_FOUND)
set(LIBS ${LIBS} ${TCL_LIBRARY})
include_directories(${TCL_INCLUDE_PATH})

# Find the GCrypt library
find_package(GCrypt REQUIRED)
set(LIBS ${LIBS} ${GCRYPT_LIBRARIES})
include_directories(${GCRYPT_INCLUDE_DIRS})
add_definitions(${GCRYPT_DEFINITIONS})

# Find the dl library - only for Linux, not required for FreeBSD
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  find_package(DL REQUIRED)
  set(LIBS ${LIBS} ${DL_LIBRARIES})
  include_directories(${DL_INCLUDES})
endif()

# Find libcurl libraries
FIND_PACKAGE(CURL)
if(CURL_FOUND)
   set(LIBS ${LIBS} ${CURL_LIBRARIES})
   set(LIBS ${LIBS} ${CURL_LIBRARY})
   set(CMAKE_REQUIRED_LIBRARIES "${CURL_LIBRARIES}")
   INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})
 else(CURL_FOUND)
   message(FATAL_ERROR "Could not find the CURL library and development files.")
endif(CURL_FOUND)

# Add project libraries
set(LIBS ${LIBS} trx locationinfo asynccpp asyncaudio asynccore svxmisc)

# Add targets for version files
set(VERSION_DEPENDS)
add_version_target(SVXLINK VERSION_DEPENDS)

# Build the executable
add_executable(svxlink
  MsgHandler.cpp Module.cpp Logic.cpp SimplexLogic.cpp RepeaterLogic.cpp
  EventHandler.cpp LinkManager.cpp CmdParser.cpp QsoRecorder.cpp svxlink.cpp
  DtmfDigitHandler.cpp ReflectorLogic.cpp
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
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gpio.conf.in
  ${CMAKE_CURRENT_BINARY_DIR}/gpio.conf
  @ONLY
  )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/svxlink_gpio_up.in
	${RUNTIME_OUTPUT_DIRECTORY}/svxlink_gpio_up)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/svxlink_gpio_down.in
	${RUNTIME_OUTPUT_DIRECTORY}/svxlink_gpio_down)

# Install targets
install(TARGETS svxlink DESTINATION ${BIN_INSTALL_DIR})
install_mkdir(${SVX_SPOOL_INSTALL_DIR}/qso_recorder ${SVXLINK_USER}:${SVXLINK_GROUP})
install_mkdir(${SVX_SHARE_INSTALL_DIR}/sounds)
install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/svxlink.conf
  ${SVX_SYSCONF_INSTALL_DIR}
  )
install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/gpio.conf
  ${SVX_SYSCONF_INSTALL_DIR}
  )
install(FILES events.tcl DESTINATION ${SVX_SHARE_INSTALL_DIR})
install(FILES RepeaterLogic.tcl SimplexLogic.tcl Module.tcl Logic.tcl CW.tcl
  SelCall.tcl locale.tcl
  DESTINATION ${SVX_SHARE_INSTALL_DIR}/events.d
  )
install(PROGRAMS ${RUNTIME_OUTPUT_DIRECTORY}/svxlink_gpio_up
  DESTINATION ${SBIN_INSTALL_DIR}
  )
install(PROGRAMS ${RUNTIME_OUTPUT_DIRECTORY}/svxlink_gpio_down
  DESTINATION ${SBIN_INSTALL_DIR}
  )
