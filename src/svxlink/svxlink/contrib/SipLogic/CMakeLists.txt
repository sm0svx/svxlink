pkg_check_modules(PJSIP "libpjproject")
message(
  "--   The pjproject libraries are used for the EXPERIMENTAL logic core\n"
  "--   SipLogic."
  )
if(PJSIP_FOUND)
  add_definitions(${PJSIP_CFLAGS})
  set(LIBS ${LIBS} ${PJSIP_STATIC_LIBRARIES} PARENT_SCOPE)
  include_directories(${PJSIP_INCLUDE_DIRS})
  add_definitions("-DPJSIP_MAJOR=${PJSIP_VERSION_MAJOR}")
  set(SVXLINK_SRCS ${SVXLINK_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/SipLogic.cpp PARENT_SCOPE)
  set(SVXLINK_TCL_EVENT_FILES ${SVXLINK_TCL_EVENT_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/SipLogic.tcl PARENT_SCOPE)
  add_manual_pages(SipLogic.conf.5)

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
