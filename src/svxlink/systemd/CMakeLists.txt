#######
# SvxLink - Svxlink ROIP Repeater system Systemd setup
########

if(WITH_SYSTEMD)

    set(SYSTEMD_CONFIGURATIONS_FILES_DIR "/lib/systemd/system" )
    set(SYSTEMD_DEFAULTS_FILES_DIR "/etc/default" )
    set(SVX_SYSCONF_INSTALL_DIR "//etc/svxlink" )
    set(SYSTEMD_SCRIPTS_FILES_DIR "/usr/sbin" )

    set(SYSTEMD_SOURCE_DIR "${CMAKE_SOURCE_DIR}/svxlink/systemd")

    configure_file(${SYSTEMD_SOURCE_DIR}/svxlink.service.cmake${PROJECT_BINARY_DIR}/systemd/svxlink.service)
    message( STATUS "Configured systemd unit file:svxlink.service" )

    configure_file(${SYSTEMD_SOURCE_DIR}/remotetrx.service.cmake ${PROJECT_BINARY_DIR}/systemd/remotetrx.service)
    message( STATUS "Configured systemd unit file:remotetrx.service" )

    configure_file(${SYSTEMD_SOURCE_DIR}/svxlink.default.cmake${PROJECT_BINARY_DIR}/systemd/svxlink)
    message( STATUS "Configured systemd unit file:svxlink" )

    configure_file(${SYSTEMD_SOURCE_DIR}/remotetrx.default.cmake ${PROJECT_BINARY_DIR}/systemd/remotetrx)
    message( STATUS "Configured systemd unit file:remotetrx" )   

    configure_file(${SYSTEMD_SOURCE_DIR}/svxlink_gpio_up.cmake ${PROJECT_BINARY_DIR}/systemd/svxlink_gpio_up)
    message( STATUS "Configured systemd unit file:svxlink_gpio_up" )

    configure_file(${SYSTEMD_SOURCE_DIR}/svxlink_gpio_down.cmake ${PROJECT_BINARY_DIR}/systemd/svxlink_gpio_down)
    message( STATUS "Configured systemd unit file:svxlink_gpio_down" )

    configure_file(${SYSTEMD_SOURCE_DIR}/svxlink_gpio.conf.cmake ${PROJECT_BINARY_DIR}/systemd/svxlink_gpio.conf)
    message( STATUS "Configured systemd unit file:svxlink_gpio.conf" )

    if(EXISTS ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
        install(FILES ${PROJECT_BINARY_DIR}/systemd/svxlink.service DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR} )
        install(FILES ${PROJECT_BINARY_DIR}/systemd/remotetrx.service DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR} )
        message(STATUS "Unit files will be installed to ${SYSTEMD_CONFIGURATIONS_FILES_DIR} after make install" )
     else(EXISTS ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
        message(STATUS "Unit files will not be installed to ${SYSTEMD_CONFIGURATIONS_FILES_DIR} after make install")
     endif(EXISTS ${SYSTEMD_CONFIGURATIONS_FILES_DIR})

     if(EXISTS ${SYSTEMD_DEFAULTS_FILES_DIR})
        install(FILES ${PROJECT_BINARY_DIR}/systemd/svxlink DESTINATION ${SYSTEMD_DEFAULTS_FILES_DIR} )
        install(FILES ${PROJECT_BINARY_DIR}/systemd/remotetrx DESTINATION ${SYSTEMD_DEFAULTS_FILES_DIR} )
        message(STATUS "Unit files will be installed to ${SYSTEMD_DEFAULTS_FILES_DIR} after make install" )
     else(EXISTS ${SYSTEMD_DEFAULTS_FILES_DIR})
        message(STATUS "Unit files will be installed to ${SYSTEMD_DEFAULTS_FILES_DIR} after make install" )
     endif(EXISTS ${SYSTEMD_DEFAULTS_FILES_DIR})

     if(EXISTS ${SYSTEMD_SCRIPTS_FILES_DIR})
        install(PROGRAMS ${PROJECT_BINARY_DIR}/systemd/svxlink_gpio_up DESTINATION ${SYSTEMD_SCRIPTS_FILES_DIR} )
        install(PROGRAMS ${PROJECT_BINARY_DIR}/systemd/svxlink_gpio_down DESTINATION ${SYSTEMD_SCRIPTS_FILES_DIR} )
        message(STATUS "Unit files will be installed to ${SYSTEMD_SCRIPTS_FILES_DIR} after make install" )
     else(EXISTS ${SYSTEMD_SCRIPTS_FILES_DIR})
        message(STATUS "Unit files will be installed to ${SYSTEMD_SCRIPTS_FILES_DIR} after make install" )
     endif(EXISTS ${SYSTEMD_SCRIPTS_FILES_DIR})
     
     if(EXISTS ${SVX_SYSCONF_INSTALL_DIR})
     	install_if_not_exists(${SYSTEMD_CURRENT_BINARY_DIR}/systemd/svxlink_gpio.conf ${SVX_SYSCONF_INSTALL_DIR} )
        message(STATUS "Unit files will be installed to ${SVX_SYSCONF_INSTALL_DIR} after make install" )
     endif(EXISTS ${SVX_SYSCONF_INSTALL_DIR})
     
endif(WITH_SYSTEMD)