#############################################################################
# SvxLink ROIP Repeater system Systemd setup
#############################################################################
set(SYSTEMD_CONFIGURATIONS_FILES_DIR "/lib/systemd/system" CACHE PATH
  "Path to systemd units")
set(SYSTEMD_DEFAULTS_FILES_DIR "${SVX_SHARE_INSTALL_DIR}/default" CACHE PATH
  "Path to systemd unit configuration files")
set(SYSTEMD_TMPFILES_DIR "${SYSCONF_INSTALL_DIR}/tmpfiles.d" CACHE PATH
  "Path to systemd tmpfiles.d configuration files")
set(SYSTEMD_SOURCE_DIR "${CMAKE_SOURCE_DIR}/open-link/systemd" CACHE PATH
  "Path to systemd source files")

configure_file(${SYSTEMD_SOURCE_DIR}/open-link_gpio_setup.service.in
               ${PROJECT_BINARY_DIR}/systemd/open-link_gpio_setup.service)
message(STATUS "Configured systemd unit file:open-link_gpio_setup.service")

configure_file(${SYSTEMD_SOURCE_DIR}/open-link.service.in
               ${PROJECT_BINARY_DIR}/systemd/open-link.service)
message(STATUS "Configured systemd unit file:open-link.service")

configure_file(${SYSTEMD_SOURCE_DIR}/open-link.conf.in
               ${PROJECT_BINARY_DIR}/systemd/open-link.conf)
message(STATUS "Configured systemd unit file:open-link.conf")

configure_file(${SYSTEMD_SOURCE_DIR}/svxreflector.service.in
               ${PROJECT_BINARY_DIR}/systemd/svxreflector.service)
message(STATUS "Configured systemd unit file:svxreflector.service")

configure_file(${SYSTEMD_SOURCE_DIR}/svxreflector.conf.in
               ${PROJECT_BINARY_DIR}/systemd/svxreflector.conf)
message(STATUS "Configured systemd unit file:svxreflector.conf")

configure_file(${SYSTEMD_SOURCE_DIR}/remotetrx.service.in
               ${PROJECT_BINARY_DIR}/systemd/remotetrx.service)
message(STATUS "Configured systemd unit file:remotetrx.service")

configure_file(${SYSTEMD_SOURCE_DIR}/remotetrx.conf.in
               ${PROJECT_BINARY_DIR}/systemd/remotetrx.conf)
message(STATUS "Configured systemd unit file:remotetrx.conf")

configure_file(${SYSTEMD_SOURCE_DIR}/open-link.default.in
               ${PROJECT_BINARY_DIR}/systemd/open-link)
message(STATUS "Configured systemd unit file:open-link")

configure_file(${SYSTEMD_SOURCE_DIR}/svxreflector.default.in
               ${PROJECT_BINARY_DIR}/systemd/svxreflector)
message(STATUS "Configured systemd unit file:sxvreflector")

configure_file(${SYSTEMD_SOURCE_DIR}/remotetrx.default.in
               ${PROJECT_BINARY_DIR}/systemd/remotetrx)
message(STATUS "Configured systemd unit file:remotetrx")

install(FILES ${PROJECT_BINARY_DIR}/systemd/open-link_gpio_setup.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
install(FILES ${PROJECT_BINARY_DIR}/systemd/open-link.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
install(FILES ${PROJECT_BINARY_DIR}/systemd/svxreflector.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
install(FILES ${PROJECT_BINARY_DIR}/systemd/remotetrx.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
message(STATUS "Systemd unit files will be installed to "
        "${SYSTEMD_CONFIGURATIONS_FILES_DIR} after make install")
install(FILES ${PROJECT_BINARY_DIR}/systemd/open-link.conf
        DESTINATION ${SYSTEMD_TMPFILES_DIR} )
install(FILES ${PROJECT_BINARY_DIR}/systemd/svxreflector.conf
        DESTINATION ${SYSTEMD_TMPFILES_DIR} )
install(FILES ${PROJECT_BINARY_DIR}/systemd/remotetrx.conf
        DESTINATION ${SYSTEMD_TMPFILES_DIR} )
message(STATUS "Systemd unit files will be installed to "
        "/etc/tmpfiles.d after make install")		
install(FILES ${PROJECT_BINARY_DIR}/systemd/open-link
        DESTINATION ${SYSTEMD_DEFAULTS_FILES_DIR} )
install(FILES ${PROJECT_BINARY_DIR}/systemd/svxreflector
        DESTINATION ${SYSTEMD_DEFAULTS_FILES_DIR} )
install(FILES ${PROJECT_BINARY_DIR}/systemd/remotetrx
        DESTINATION ${SYSTEMD_DEFAULTS_FILES_DIR} )
message(STATUS "Systemd unit configuration files will be "
               "installed to ${SYSTEMD_DEFAULTS_FILES_DIR} "
               "after make install")
