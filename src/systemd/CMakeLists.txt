#######
# SvxLink - Svxlink ROIP Repeater system Systemd setup
########

if(WITH_SYSTEMD)

    set(SYSTEMD_CONFIGURATIONS_FILES_DIR "/lib/systemd/system" )
    set(SYSTEMD_DEFAULTS_FILES_DIR "/etc/default" )
    set(SYSTEMD_SCRIPTS_FILES_DIR "/usr/sbin" )

    configure_file(${CMAKE_SOURCE_DIR}/systemd/svxlink.service.cmake ${PROJECT_BINARY_DIR}/systemd/svxlink.service)
    message( STATUS "Configured systemd unit file:svxlink.service" )

    configure_file(${CMAKE_SOURCE_DIR}/systemd/remotetrx.service.cmake ${PROJECT_BINARY_DIR}/systemd/remotetrx.service)
    message( STATUS "Configured systemd unit file:remotetrx.service" )

    configure_file(${CMAKE_SOURCE_DIR}/systemd/svxlink.default.cmake ${PROJECT_BINARY_DIR}/systemd/svxlink)
    message( STATUS "Configured systemd unit file:svxlink" )

    configure_file(${CMAKE_SOURCE_DIR}/systemd/remotetrx.default.cmake ${PROJECT_BINARY_DIR}/systemd/remotetrx)
    message( STATUS "Configured systemd unit file:remotetrx" )   

    configure_file(${CMAKE_SOURCE_DIR}/systemd/svxlink_gpio_setup.cmake ${PROJECT_BINARY_DIR}/systemd/svxlink_gpio_setup)
    message( STATUS "Configured systemd unit file:svxlink_gpio_setup" )

    configure_file(${CMAKE_SOURCE_DIR}/systemd/svvxlink_gpio_down.cmake ${PROJECT_BINARY_DIR}/systemd/svvxlink_gpio_down)
    message( STATUS "Configured systemd unit file:svvxlink_gpio_down" )

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
        install(PROGRAMS ${PROJECT_BINARY_DIR}/systemd/svxlink_gpio_setup DESTINATION ${SYSTEMD_SCRIPTS_FILES_DIR} )
        install(PROGRAMS ${PROJECT_BINARY_DIR}/systemd/svvxlink_gpio_down DESTINATION ${SYSTEMD_SCRIPTS_FILES_DIR} )
        message(STATUS "Unit files will be installed to ${SYSTEMD_SCRIPTS_FILES_DIR} after make install" )
     else(EXISTS ${SYSTEMD_SCRIPTS_FILES_DIR})
        message(STATUS "Unit files will be installed to ${SYSTEMD_SCRIPTS_FILES_DIR} after make install" )
     endif(EXISTS ${SYSTEMD_SCRIPTS_FILES_DIR})
 
endif(WITH_SYSTEMD)