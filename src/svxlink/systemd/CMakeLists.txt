#############################################################################
# SvxLink ROIP Repeater system Systemd setup
#############################################################################

set(SYSTEMD_CONFIGURATIONS_FILES_DIR "/lib/systemd/system" CACHE PATH
  "Path to systemd units")
set(SYSTEMD_DEFAULTS_FILES_DIR "/etc/default" CACHE PATH
  "Path to systemd unit configuration files")
set(SYSTEMD_SOURCE_DIR "${CMAKE_SOURCE_DIR}/svxlink/systemd")

configure_file(${SYSTEMD_SOURCE_DIR}/svxlink_gpio_setup.service.in
               ${PROJECT_BINARY_DIR}/systemd/svxlink_gpio_setup.service)
#message(STATUS "Configured systemd unit file:svxlink_gpio_setup.service")

configure_file(${SYSTEMD_SOURCE_DIR}/svxlink.service.in
               ${PROJECT_BINARY_DIR}/systemd/svxlink.service)
#message(STATUS "Configured systemd unit file:svxlink.service")

configure_file(${SYSTEMD_SOURCE_DIR}/remotetrx.service.in
               ${PROJECT_BINARY_DIR}/systemd/remotetrx.service)
#message(STATUS "Configured systemd unit file:remotetrx.service")

configure_file(${SYSTEMD_SOURCE_DIR}/svxreflector.service.in
               ${PROJECT_BINARY_DIR}/systemd/svxreflector.service)

configure_file(${SYSTEMD_SOURCE_DIR}/svxlink.default.in
               ${PROJECT_BINARY_DIR}/systemd/svxlink)
#message(STATUS "Configured systemd unit file:svxlink")

configure_file(${SYSTEMD_SOURCE_DIR}/remotetrx.default.in
               ${PROJECT_BINARY_DIR}/systemd/remotetrx)
#message(STATUS "Configured systemd unit file:remotetrx")

configure_file(${SYSTEMD_SOURCE_DIR}/svxreflector.default.in
               ${PROJECT_BINARY_DIR}/systemd/svxreflector)

install(FILES ${PROJECT_BINARY_DIR}/systemd/svxlink_gpio_setup.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
install(FILES ${PROJECT_BINARY_DIR}/systemd/svxlink.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
install(FILES ${PROJECT_BINARY_DIR}/systemd/remotetrx.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
install(FILES ${PROJECT_BINARY_DIR}/systemd/svxreflector.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
message(STATUS "Systemd unit files will be installed to "
        "${SYSTEMD_CONFIGURATIONS_FILES_DIR} after make install")

install(FILES ${PROJECT_BINARY_DIR}/systemd/svxlink
        DESTINATION ${SYSTEMD_DEFAULTS_FILES_DIR} )
install(FILES ${PROJECT_BINARY_DIR}/systemd/remotetrx
        DESTINATION ${SYSTEMD_DEFAULTS_FILES_DIR} )
install(FILES ${PROJECT_BINARY_DIR}/systemd/svxreflector
        DESTINATION ${SYSTEMD_DEFAULTS_FILES_DIR} )
message(STATUS "Systemd unit configuration files will be "
               "installed to ${SYSTEMD_DEFAULTS_FILES_DIR} "
               "after make install")
