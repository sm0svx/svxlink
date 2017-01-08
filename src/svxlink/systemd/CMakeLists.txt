#############################################################################
# SvxLink ROIP Repeater system Systemd setup
#############################################################################

if(WITH_SYSTEMD)
  set(SYSTEMD_CONFIGURATIONS_FILES_DIR "/lib/systemd/system" )
  set(SYSTEMD_DEFAULTS_FILES_DIR "/etc/default" )
  set(SYSTEMD_SCRIPTS_FILES_DIR "/usr/sbin" )

  set(SYSTEMD_SOURCE_DIR "${CMAKE_SOURCE_DIR}/svxlink/systemd")

  configure_file(${SYSTEMD_SOURCE_DIR}/svxlink.service.in
                 ${PROJECT_BINARY_DIR}/systemd/svxlink.service)
  message( STATUS "Configured systemd unit file:svxlink.service" )

  configure_file(${SYSTEMD_SOURCE_DIR}/remotetrx.service.in
                 ${PROJECT_BINARY_DIR}/systemd/remotetrx.service)
  message( STATUS "Configured systemd unit file:remotetrx.service" )

  configure_file(${SYSTEMD_SOURCE_DIR}/svxlink.default.in
                 ${PROJECT_BINARY_DIR}/systemd/svxlink)
  message( STATUS "Configured systemd unit file:svxlink" )

  configure_file(${SYSTEMD_SOURCE_DIR}/remotetrx.default.in
                 ${PROJECT_BINARY_DIR}/systemd/remotetrx)
  message( STATUS "Configured systemd unit file:remotetrx" )

  configure_file(${SYSTEMD_SOURCE_DIR}/gpio_setup.sh.in
                 ${PROJECT_BINARY_DIR}/systemd/gpio_setup.sh)
  message( STATUS "Configured systemd unit file:gpio_setup.sh" )

  configure_file(${SYSTEMD_SOURCE_DIR}/gpio_tear_down.sh.in
                 ${PROJECT_BINARY_DIR}/systemd/gpio_tear_down.sh)
  message( STATUS "Configured systemd unit file:gpio_tear_down.sh" )

  if(EXISTS ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
    install(FILES ${PROJECT_BINARY_DIR}/systemd/svxlink.service
            DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR} )
    install(FILES ${PROJECT_BINARY_DIR}/systemd/remotetrx.service
            DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR} )
    message(STATUS "Systemd unit files will be installed to "
                   "${SYSTEMD_CONFIGURATIONS_FILES_DIR} after make install")
  endif(EXISTS ${SYSTEMD_CONFIGURATIONS_FILES_DIR})

  if(EXISTS ${SYSTEMD_DEFAULTS_FILES_DIR})
    install(FILES ${PROJECT_BINARY_DIR}/systemd/svxlink
            DESTINATION ${SYSTEMD_DEFAULTS_FILES_DIR} )
    install(FILES ${PROJECT_BINARY_DIR}/systemd/remotetrx
            DESTINATION ${SYSTEMD_DEFAULTS_FILES_DIR} )
    message(STATUS "Systemd unit configuration files will be "
                   "installed to ${SYSTEMD_DEFAULTS_FILES_DIR} "
                   "after make install")
  endif(EXISTS ${SYSTEMD_DEFAULTS_FILES_DIR})

  if(EXISTS ${SYSTEMD_SCRIPTS_FILES_DIR})
    install(PROGRAMS ${PROJECT_BINARY_DIR}/systemd/gpio_setup.sh
            DESTINATION ${SYSTEMD_SCRIPTS_FILES_DIR} )
    install(PROGRAMS ${PROJECT_BINARY_DIR}/systemd/gpio_tear_down.sh
            DESTINATION ${SYSTEMD_SCRIPTS_FILES_DIR} )
    message(STATUS "Systemd script files will be installed to "
                   "${SYSTEMD_SCRIPTS_FILES_DIR} after make install")
  endif(EXISTS ${SYSTEMD_SCRIPTS_FILES_DIR})
endif(WITH_SYSTEMD)
