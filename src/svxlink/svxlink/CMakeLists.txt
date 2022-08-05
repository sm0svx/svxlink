# C++ source files needed to build SvxLink
set(SVXLINK_SRCS
  MsgHandler.cpp Module.cpp Logic.cpp SimplexLogic.cpp RepeaterLogic.cpp
  EventHandler.cpp LinkManager.cpp CmdParser.cpp QsoRecorder.cpp svxlink.cpp
  DtmfDigitHandler.cpp ReflectorLogic.cpp
  )

# TCL event handler files to install in the events.d subdirectory
set(SVXLINK_TCL_EVENT_FILES
  RepeaterLogic.tcl SimplexLogic.tcl ReflectorLogic.tcl Module.tcl Logic.tcl
  CW.tcl SelCall.tcl locale.tcl
  )

# Find the popt library
find_package(Popt REQUIRED)
set(LIBS ${LIBS} ${POPT_LIBRARIES})
include_directories(${POPT_INCLUDE_DIRS})
add_definitions(${POPT_DEFINITIONS})

# Find the GSM codec library
find_package(GSM REQUIRED)
set(LIBS ${LIBS} ${GSM_LIBRARY})
include_directories(${GSM_INCLUDE_DIR})

# Find the pjsip library
pkg_check_modules(PJSIP "libpjproject")
message(
  "--   The pjproject libraries are used for the EXPERIMENTAL logic core\n"
  "--   SipLogic.\n"
  )
if(PJSIP_FOUND)
  add_definitions(${PJSIP_CFLAGS})
  set(LIBS ${LIBS} ${PJSIP_LIBRARIES})
  include_directories(${PJSIP_INCLUDE_DIRS})
  add_definitions("-DPJSIP_MAJOR=${PJSIP_VERSION_MAJOR}")
  set(SVXLINK_SRCS ${SVXLINK_SRCS} SipLogic.cpp)
  set(SVXLINK_TCL_EVENT_FILES ${SVXLINK_TCL_EVENT_FILES} SipLogic.tcl)
else (PJSIP_FOUND)
  message(
    "--   The pjproject libraries are an optional dependency.\n"
    "--   Without it you will be missing support for setting up SIP\n"
    "--   connections using the SipLogic logic core.\n"
    "--   SvxLink will work just fine without it though if SIP support\n"
    "--   is not needed.\n"
  )
endif(PJSIP_FOUND)

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

# Find jsoncpp library
#find_package(jsoncpp REQUIRED)
#get_target_property(JSONCPP_INC_PATH jsoncpp_lib INTERFACE_INCLUDE_DIRECTORIES)
#include_directories(${JSONCPP_INC_PATH})
#set(LIBS ${LIBS} jsoncpp_lib)
pkg_check_modules (JSONCPP REQUIRED jsoncpp)
include_directories(${JSONCPP_INCLUDE_DIRS})
set(LIBS ${LIBS} ${JSONCPP_LIBRARIES})

# Add project libraries
set(LIBS trx locationinfo asynccpp asyncaudio asynccore svxmisc ${LIBS})

# Build the executable
add_executable(svxlink ${SVXLINK_SRCS} ${VERSION_DEPENDS})
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
	${RUNTIME_OUTPUT_DIRECTORY}/svxlink_gpio_up
        @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/svxlink_gpio_down.in
	${RUNTIME_OUTPUT_DIRECTORY}/svxlink_gpio_down
        @ONLY)

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
install_if_not_exists(node_info.json ${SVX_SYSCONF_INSTALL_DIR})
install(FILES events.tcl DESTINATION ${SVX_SHARE_INSTALL_DIR})
install(FILES ${SVXLINK_TCL_EVENT_FILES}
  DESTINATION ${SVX_SHARE_INSTALL_DIR}/events.d
  )
install(PROGRAMS ${RUNTIME_OUTPUT_DIRECTORY}/svxlink_gpio_up
  DESTINATION ${SBIN_INSTALL_DIR}
  )
install(PROGRAMS ${RUNTIME_OUTPUT_DIRECTORY}/svxlink_gpio_down
  DESTINATION ${SBIN_INSTALL_DIR}
  )
