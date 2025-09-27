# Find the pjproject libraries and include files
pkg_check_modules(PJSIP "libpjproject")
message(
  "--   The pjproject libraries are used for the EXPERIMENTAL logic core\n"
  "--   SipLogic."
  )
if(PJSIP_FOUND)
  # Set up the build environment for the plugin
  add_definitions(${PJSIP_CFLAGS})
  set(LIBS ${PJSIP_STATIC_LIBRARIES})
  include_directories(${PJSIP_INCLUDE_DIRS})
  add_definitions("-DPJSIP_MAJOR=${PJSIP_VERSION_MAJOR}")
  add_manual_pages(SipLogic.conf.5)

  # Build the plugin
  add_library(SipLogic MODULE SipLogic.cpp)
  set_target_properties(SipLogic PROPERTIES PREFIX "")
  set_property(TARGET SipLogic PROPERTY NO_SONAME 1)
  target_link_libraries(SipLogic ${LIBS})

  # Install plugin, TCL event handler and config file
  install(TARGETS SipLogic DESTINATION ${SVX_LOGIC_CORE_INSTALL_DIR})
  install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/SipLogicType.tcl
          ${CMAKE_CURRENT_SOURCE_DIR}/SipLogic.tcl
    DESTINATION ${SVX_SHARE_INSTALL_DIR}/events.d
    )
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/SipLogic.conf.in
    ${CMAKE_CURRENT_BINARY_DIR}/SipLogic.conf
    @ONLY
    )
  install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/SipLogic.conf
    ${SVX_SYSCONF_INSTALL_DIR}/svxlink.d
    )
else (PJSIP_FOUND)
  message(FATAL_ERROR
    "Building of the contributed logic core SipLogic has been enabled "
    "but dependencies are missing. Aborting build."
  )
endif(PJSIP_FOUND)
