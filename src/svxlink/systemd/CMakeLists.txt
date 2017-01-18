#######
# SvxLink - Svxlink ROIP Repeater system Systemd setup
########

if(WITH_SYSTEMD)

    set(SYSTEMD_CONFIGURATIONS_FILES_DIR "/lib/systemd/system" )
    set(SYSTEMD_DEFAULTS_FILES_DIR "/etc/default" )
    set(SYSTEMD_SCRIPTS_FILES_DIR "/usr/sbin" )
    set(SVX_SYSCONF_INSTALL_DIR "//etc/svxlink" )

    configure_file(${CMAKE_SOURCE_DIR}/svxlink/systemd/svxlink.service.cmake ${PROJECT_BINARY_DIR}/systemd/svxlink.service)
    message( STATUS "Configured systemd unit file:svxlink.service" )
    
    configure_file(${CMAKE_SOURCE_DIR}/svxlink/systemd/remotetrx.service.cmake ${PROJECT_BINARY_DIR}/systemd/remotetrx.service)
    message( STATUS "Configured systemd unit file:remotetrx.service" )
    
    configure_file(${CMAKE_SOURCE_DIR}/svxlinksystemd/svxlink.default.cmake ${PROJECT_BINARY_DIR}/systemd/svxlink)
    message( STATUS "Configured systemd unit file:svxlink" )
    
    configure_file(${CMAKE_SOURCE_DIR}/svxlink/systemd/remotetrx.default.cmake ${PROJECT_BINARY_DIR}/systemd/remotetrx)
    message( STATUS "Configured systemd unit file:remotetrx" )   

    configure_file(${CMAKE_SOURCE_DIR}/svxlink/systemd/svxlink_gpio_up.cmake ${PROJECT_BINARY_DIR}/systemd/svxlink_gpio_up)
    message( STATUS "Configured systemd unit file:svxlink_gpio_up.cmake" )

    configure_file(${CMAKE_SOURCE_DIR}/svxlink/systemd/svxlink_gpio_down.cmake ${PROJECT_BINARY_DIR}/systemd/svxlink_gpio_down)
    message( STATUS "Configured systemd unit file:svxlink_gpio_down.cmake" )
    
    configure_file(${CMAKE_SOURCE_DIR}/svxlinkg/systemd/svxlink_gpio.conf.cmake ${PROJECT_BINARY_DIR}/systemd/svxlink_gpio.conf)
    message( STATUS "Configured systemd unit file:svxlink_gpio.conf.in.cmake" )

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
     	install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/systemd/svxlink_gpio.conf ${SVX_SYSCONF_INSTALL_DIR} )
        message(STATUS "Unit files will be installed to ${SVX_SYSCONF_INSTALL_DIR} after make install" )
     endif(EXISTS ${SVX_SYSCONF_INSTALL_DIR})
     
endif(WITH_SYSTEMD)